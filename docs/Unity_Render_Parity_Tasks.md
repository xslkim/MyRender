# MyRender × Unity URP 渲染复刻 — 任务清单

Status: v1 · 2026-06-20
关联文档：[Unity_Importer_Design.md](Unity_Importer_Design.md)（导入设计）· [MyRender_AssetFormat.md](MyRender_AssetFormat.md)（资源格式契约）· [Unity_Importer_Tasks.md](Unity_Importer_Tasks.md)（导入器任务）

本文是**复刻 Unity URP 画面**的执行文档。几何对齐已完成（见 [Bug_FloorLeftCorner_MeshLoadEvalOrder.md](Bug_FloorLeftCorner_MeshLoadEvalOrder.md)），接下来补齐光照/环境/抗锯齿/后处理等缺失项，目标是 MyRender 渲染 `SampleScene` 与 `unity_ref.png` 在观感上逐步收敛。

---

## 0. 使用约定

- **自上而下**做。每个任务 `Tn.m` 有「交付」和「验证」两行，验证不绿不进下一个。
- 验证有三类：
  - **Visual（视觉）** —— 用 `--capture-unity` 出 BMP，与 `unity_ref.png` 并排对比 / 像素 diff。
  - **Unit（单元）** —— 在 `test.exe` 里加断言（矩阵、SH 系数、cubemap 采样的 golden 值）。
  - **Smoke（冒烟）** —— 能编译、加载、运行不崩，日志符合预期。
- 一个任务 ≈ 一次提交，信息如 `A1: 天空盒 pass`。
- 改动同时涉及**导出器(C#)**和**渲染器(C++)**的任务会标 `[两端]`。

### 构建 / 运行参考

```powershell
cmake --build build --config Release
# 渲染 SampleScene 到 BMP（dv: 0=正常 1=albedo 2=几何法线 3=贴图法线 4=uv 5=wireframe）
.\build\Release\MyRender.exe --capture-unity assets\unity_export\SampleScene out\sample_render.bmp
.\build\Release\test.exe
```

对比基准：`assets/unity_export/SampleScene/unity_ref.png`（Unity 960×540 截图）。
建议每个 Visual 任务沉淀一张 `out/parity_<task>.png` 留档。

### 验证工具（先建，T0）

当前对比靠肉眼。先做一个量化 diff 脚本，后续每个任务都用它。

---

## 阶段 T0：对比基建（前置）

### T0.1 — 渲染输出 ↔ 参考图 diff 脚本
- **任务**：写 `tools/compare.py`：读 MyRender 的 BMP/PNG 与 `unity_ref.png`，输出 (a) 逐像素 MSE / PSNR，(b) 并排+差值热力图 PNG。复用已有 PIL/numpy。
- **交付**：`tools/compare.py`；`out/parity_baseline.png`（当前状态的差值图）。
- **验证(Smoke)**：`python tools/compare.py out/sample_render.png assets/unity_export/SampleScene/unity_ref.png` 打印 MSE 并生成热力图。记录当前 baseline MSE 作为后续每阶段的下降基准。

---

## 阶段 A：环境光照（收益最大）

> 现状：背景纯色填充；`bakedGI = _AmbientColor` 是常量平光（[LitShader.hpp:70](../src/gpu/LitShader.hpp:70)）；
> 反射 `SAMPLE_TEXTURECUBE_LOD` 直接返回黑（[GlobalIllumination.hpp:351](../src/gpu/GlobalIllumination.hpp:351)）。
> → 金属/光滑表面反射全黑、环境光无方向、背景与 Unity 不符。

### A1 — 天空盒 pass [两端]
- **任务**：
  - 导出器：在 `MyRenderExporter.cs` 导出环境天空信息。优先做**程序化渐变**（导出 Unity Gradient/Skybox 的 top/horizon/bottom 颜色 + 曝光），cubemap 留到 A3。写进 `scene.json` 的新字段 `environment`。
  - 渲染器：新增 `SkyboxPass`，在所有不透明物体绘制后、按相机逆投影对每个背景像素（depth 未写入处）算世界射线方向，采样渐变。替换 `Scene::Render` 中的纯色清屏背景。
- **文件**：`unity-exporter/.../MyRenderExporter.cs`、`src/core/Scene.hpp`、新增 `src/core/SkyboxPass.hpp`、`src/core/UnitySceneLoader.hpp`（解析 environment）。
- **交付**：能渲出与 Unity 接近的天空背景；`out/parity_A1.png`。
- **验证(Visual)**：背景区域（无几何处）与 `unity_ref.png` 的天空色差明显下降；compare.py 背景区 MSE 下降。

### A2 — 球谐 (SH) 环境光替换平光 [两端]
- **任务**：
  - 导出器：把 `RenderSettings.ambientProbe` 的 **L2 SH9 系数**原样导出（停止 `ComputeFlatAmbient` 的 6 轴平均），写进 `scene.json` 的 `environment.sh` (27 个 float)。
  - 渲染器：在 `ShaderGlobal.hpp` 定义 `unity_SHAr..unity_SHC`，实现 `GlobalIllumination.hpp` 的 `SampleSHPixel` / `EvaluateAmbientProbe`（L2 求值），把 `LitInitializeInputData` 的 `bakedGI = _AmbientColor` 换成 `SampleSH(normalWS)`。
- **文件**：`MyRenderExporter.cs`、`src/gpu/ShaderGlobal.hpp`、`src/gpu/GlobalIllumination.hpp`、`src/gpu/LitShader.hpp`、`UnitySceneLoader.hpp`。
- **交付**：环境光随法线方向变化（朝天的面偏天空色、朝下偏地色）；`out/parity_A2.png`。
- **验证(Unit + Visual)**：单元——给定 SH 系数 + 几个法线，`SampleSH` 输出与 Python/Unity 参考值在 1e-3 内一致。视觉——地板/墙的环境色出现方向梯度，与 Unity 一致。

### A3 — 反射探针 / IBL 间接高光 [两端]
- **任务**：
  - 导出器：导出天空盒/反射探针的预卷积 cubemap（按粗糙度 7 级 mip，等距柱状或 6 面），及 HDR decode 参数；写 `environment.specCube`。
  - 渲染器：实现真正的 `SAMPLE_TEXTURECUBE_LOD`（cubemap 采样 + mip 三线性），接回 `GlossyEnvironmentReflection` / `GlobalIllumination`，删掉返回黑的桩。
- **文件**：`TextureExporter.cs`、`src/gpu/GlobalIllumination.hpp`、新增 cubemap 采样到 `src/core/Texture.hpp` 或 `src/gpu/ShaderFunction.hpp`。
- **交付**：金属/光滑表面（油漆桶金属、安全帽）出现环境反射；`out/parity_A3.png`。
- **验证(Visual)**：金属件不再死黑，高光/反射与 Unity 方向一致；compare.py 全图 MSE 进一步下降。

---

## 阶段 B：抗锯齿

> 现状：单采样，无 AA。`Render` 用浮点 framebuffer，适合超采样。

### B1 — SSAA（超采样基线）
- **任务**：在内部以 N×（先 2×：1920×1080）渲染整帧，最后 box 降采样到 960×540 再做 sRGB 输出。加 `--ss <n>` 或复用现有分辨率缩放路径。注意 shadow map、`_ScaledScreenParams`、屏幕空间 UV 都要按超采样分辨率走。
- **文件**：`src/core/Config.hpp`、`src/core/Render.hpp`、`src/core/Scene.hpp`、`src/MyRender.cpp`。
- **交付**：边缘锯齿明显减弱；`out/parity_B1.png`。
- **验证(Visual)**：stud 框架、长椅腿等斜边的阶梯感消失；与 Unity（默认 MSAA/抗锯齿）边缘接近。

### B2 — FXAA 后处理（可选，性能向）
- **任务**：在 tonemap 后加一个 FXAA 全屏 pass，作为 SSAA 的低成本替代/叠加。
- **交付/验证(Visual)**：单采样 + FXAA 的边缘质量记录对比，确认作为快速预览选项可用。

---

## 阶段 C：后处理 / 色调映射

> 现状：仅 linear→sRGB（[Scene.hpp:145](../src/core/Scene.hpp:145)）。Unity URP 常带 Tonemapping/Bloom/颜色分级。

### C1 — 后处理链骨架 + Tonemapping [两端]
- **任务**：导出器导出场景 Volume 的后处理参数（曝光、tonemap 模式 None/Neutral/ACES、对比度/饱和度）。渲染器在 sRGB 之前插入 `PostProcess`：曝光 → Tonemap → 颜色分级。
- **文件**：`MyRenderExporter.cs`、新增 `src/core/PostProcess.hpp`、`src/core/Scene.hpp`。
- **交付**：整体色调（高光滚降、对比）向 Unity 收敛；`out/parity_C1.png`。
- **验证(Visual)**：亮部不再硬切到白；与 Unity 的整体明度/对比一致。

### C2 — Bloom（可选）
- **任务**：亮度阈值提取 → 降采样高斯模糊 → 叠加。导出 Bloom 参数。
- **验证(Visual)**：自发光/高光区域辉光与 Unity 接近。

### C3 — Vignette / 其它（可选）
- 视参考图需要再决定。

---

## 阶段 D：附加光源 + 雾

> 现状：只导出/处理一个方向主光；`_ADDITIONAL_LIGHTS` 宏未定义（Lighting.hpp 整段编译不进）；雾 `fogFactor` 写死 0。

### D1 — 附加光源（点光/聚光）[两端]
- **任务**：导出器导出光源列表（类型、位置、方向、颜色、强度、range、spot 角度）到 `scene.json.lights[]`。渲染器定义 `_ADDITIONAL_LIGHTS`，填 `_AdditionalLights*` 全局数组，实现 `GetAdditionalLightsCount` / `GetAdditionalLight`（距离 + 聚光衰减）。
- **文件**：`MyRenderExporter.cs`、`src/gpu/ShaderGlobal.hpp`、`src/gpu/ShaderFunction.hpp`、`UnitySceneLoader.hpp`。
- **交付**：场景中点光/聚光照亮局部；`out/parity_D1.png`。
- **验证(Visual)**：若 SampleScene 有附加光（如工作灯 Light_Heads/Bulbs），其照明出现且位置正确。

### D2 — 雾 [两端]
- **任务**：导出 `RenderSettings` 雾参数（模式 Linear/Exp/Exp2、颜色、密度/起止）。渲染器顶点算真实 `fogFactor`，接通 `MixFog`、设 `unity_FogColor/unity_FogParams`。
- **文件**：`MyRenderExporter.cs`、`src/gpu/LitShader.hpp`、`src/gpu/ShaderFunction.hpp`、`ShaderGlobal.hpp`。
- **交付/验证(Visual)**：远处物体按 Unity 雾色淡出（SampleScene 若未开雾，则做开雾的小测试场景验证后默认关闭）。

---

## 阶段 E：正确性收尾

### E1 — Alpha 裁剪 (Cutout)
- **任务**：恢复 `LitShader.hpp` 的 `AlphaDiscard`（[LitShader.hpp:11](../src/gpu/LitShader.hpp:11) 被注释），按 `_Cutoff` 在光栅化丢弃片元。
- **验证(Visual)**：cutout 材质（如镂空网格/树叶类）正确镂空；不影响不透明物体。

### E2 — 透明物体排序
- **任务**：`Scene::Render` 绘制前，把透明物体按相机距离从后往前排序（不透明先画）。
- **验证(Visual)**：多个半透明叠加（油漆桶盖/护目镜镜片等）无穿插错误。

### E3 — 纹理 mipmap / 三线性
- **任务**：`Texture.hpp` 生成 mip 链，光栅化按 UV 导数选 mip + 三线性采样。
- **验证(Visual)**：远处/掠射角纹理不再闪烁、摩尔纹减少。

### E4 — 阴影级联（可选，大场景）
- **任务**：方向光 2~4 级级联，解决 `kMaxShadowSceneDiagonal>60m` 直接关阴影的问题。
- **验证(Visual)**：GardenScene 等大场景恢复阴影且无明显漏光。

---

## 优先级与依赖

```
T0 (diff 工具) ──► A1 天空盒 ──► A2 SH 环境光 ──► A3 反射探针/IBL
                                     │
                  B1 SSAA ◄──────────┘(可并行)
                  C1 后处理 (A 之后做，色调才有意义)
                  D1/D2、E* 视参考图差距按需
```

- **必做主线**：T0 → A1 → A2 → A3 → B1 → C1。做完这条线，与 `unity_ref.png` 的主要差异（背景、反射、环境光方向、锯齿、色调）基本消除。
- **按需**：D（取决于 SampleScene 是否有附加光/雾）、E（质量细节）。

## 进度表

| 任务 | 状态 | parity MSE | 备注 |
|------|------|-----------|------|
| T0.1 diff 工具 | ✅ | 3018 / PSNR 13.3 dB | baseline |
| A1 天空盒 | ✅(C++)/⏳(Unity重导) | 1675 / PSNR 15.9 dB | 需 Unity 重导获取真实天空色值 |
| A2 SH 环境光 | ✅(C++)/⏳(Unity重导) | 1667 (测试数据) | 导出器+C++ 均完成；等 Unity 重导 |
| A3 反射/IBL | ✅(SH近似)/⏳(真实cubemap) | 1785 (测试数据) | 用 SH@reflect 近似；除以π修正能量；真实提升等 Unity 重导 |
| B1 SSAA 2× | ✅ | 1767 (ss=2×) | `--capture-unity ... 0 0 2` 启用；box 降采样 960×540 |
| C1 后处理/Tonemapping | ✅(框架)/⏳(Unity重导) | 1785 (无 pp 数据) | ACES/Neutral/exposure/contrast/saturation 均实现；等 Unity 重导激活 |
| D1 附加光源 | ☐ | | |
| D2 雾 | ☐ | | |
| E1 Alpha 裁剪 | ☐ | | |
| E2 透明排序 | ☐ | | |
| E3 mipmap | ☐ | | |
| E4 阴影级联 | ☐ | | |

### Unity 重导操作清单（用户操作）

以下功能代码已完成，但效果依赖真实 Unity 数据。重导一次后 A1~C1 的所有改进将同时激活：

1. 打开 Unity SampleScene
2. 将 `unity-exporter/MyRenderExport/MyRenderExporter.cs` 复制到 Unity 项目的 `Assets/Editor/` 下
3. 菜单 **MyRender → Export Active Scene**
4. 把导出的 `scene.json` 替换 `assets/unity_export/SampleScene/scene.json`
5. 重新运行 `compare.py` 对比真实效果
