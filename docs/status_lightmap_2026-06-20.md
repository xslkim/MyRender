# MyRender Lightmap 管道实现状态
**日期：2026-06-20（当 session 末更新）**

---

## 一、当前渲染质量（重新校准）

| 指标 | 数值 |
|------|------|
| MSE  | **1398**（无 lightmap，SSAA=2x；1x 为 1474，结论一致） |
| PSNR | 16.68 dB |
| 参考图 | `assets/unity_export/SampleScene/unity_ref.png`（668 KB，960×540） |

> ⚠️ 文档前版的 MSE=972 / PSNR=18.25 dB **已过期**。本 session 开始时发现
> `scene.json` 与 `unity_ref.png` 在上一次 commit（9e65799）后已被重新导出
> （scene.json +347 行，ref 491 KB → 668 KB），所以那是另一个场景版本。
> **当前真实基线 = MSE 1398。** 所有"改善"应以这个新基线为参照。

### 1.1 误差分布（`tools/mse_regions.py`，8×5 网格）

- 整体偏亮 **+22.3**（ours 129.6 vs ref 107.3）
- **地板条（y=430..540）偏亮 +54.3** ← 最大单一误差源
- 天空条（y=0..80）偏亮 +10.4（次要）
- 最差网格单元集中在底部行（y=4）和左/右边缘，MSE 高达 4300~4900

**结论：地板过亮正是 lightmap 缺失的直接症状**——Unity 烘焙的 AO/间接光空间变化
在当前 SH9 常量环境光下丢失，所以地板被照成均匀亮色。lightmap 接入后这部分
预计大幅下降。

---

## 二、本 Session 已完成的工作

### 2.0 本次新增（继续开发）

| 动作 | 结果 |
|------|------|
| 同步 Unity 导出器 | 把 `unity-exporter/MyRenderExport/MyRenderExporter.cs`（含 lightmap 逻辑）拷到 **两个** Unity 项目：`/g/unity_demo/urp2019/...`（SampleScene 所在）、`/g/unity_demo/urp_sample/...` |
| 新增 MSE 工具 | `tools/mse.py`（整体 MSE/PSNR + 可选 region）、`tools/mse_regions.py`（网格分解 + 热力图） |
| SimpleLit 接入 lightmap | `src/core/SimpleLitMat.hpp` 传 `staticLightmapUV` + 驱动 `_EMISSION`；`src/gpu/SimpleLitShader.hpp` 计算 `lightmapUV` 并在 `_LIGHTMAP` 时用贴图替代 SH9（与 LitShader 完全对齐）。**编译通过，MSE 无回归（SampleScene 无 SimpleLit 材质，暂不可验）** |
| 验证 C++ 全链路 | Mesh.hpp 读 UV2 ✅ Vertex.uv2 ✅ Varyings.lightmapUV ✅ ShaderGlobal `_Lightmap/_LightmapST/_LIGHTMAP` ✅ SceneAsset.lightmapIndex/ScaleOffset/lightmapPaths ✅ SceneModel.lightmapTex/ST/hasLightmap ✅ UnitySceneLoader 预加载 ✅ Scene::Render 每对象设置 ✅ LitShader VS/FS ✅ |

### 2.1 C++ Lightmap 管道（全部代码已到位、可编译）

| 文件 | 改动内容 |
|------|---------|
| `src/base/Vertex.hpp` | 新增 `Vec2f uv2`（UV2/lightmap UV 字段） |
| `src/base/Varyings.hpp` | 新增 `float2 lightmapUV`（光照贴图 UV 插值传递） |
| `src/core/Mesh.hpp` | 二进制 .mesh 加载时实际读取 UV2 数据（之前跳过） |
| `src/gpu/ShaderGlobal.hpp` | 新增全局变量：`_Lightmap`、`_LightmapST`（UV 缩放偏移）、`_LIGHTMAP`（开关） |
| `src/core/SceneAsset.hpp` | `ObjectAsset` 新增 `lightmapIndex`、`lightmapScaleOffset`；`SceneAsset` 新增 `lightmapPaths[]` |
| `src/core/SceneModel.hpp` | `RenderObject` 新增 `lightmapTex`、`lightmapST`、`hasLightmap` |
| `src/core/LitMat.hpp` | `InitAttributes` 中将 `vertex.uv2` 传入 `staticLightmapUV` |
| `src/core/SimpleLitMat.hpp` | **(新)** `InitAttributes` 传 `staticLightmapUV`；`UpdateGpuParameter` 驱动 `_EMISSION` |
| `src/gpu/LitShader.hpp` | VS 计算 `lightmapUV`；FS 有 lightmap 时用贴图代替 SH9 作为 `bakedGI` |
| `src/gpu/SimpleLitShader.hpp` | **(新)** VS 计算 `lightmapUV`；FS 同样 lightmap 优先 |
| `src/core/Render.hpp` | 新增 `SetLightmap(tex, st, enabled)` 方法 |
| `src/core/UnitySceneLoader.hpp` | 预加载所有 lightmap 纹理；为每个 RenderObject 赋值 `lightmapTex`/`lightmapST`/`hasLightmap` |
| `src/core/Scene.hpp` | 每个对象绘制前设置 `gpu::_Lightmap`、`gpu::_LightmapST`、`gpu::_LIGHTMAP` |

**构建状态：编译通过（无错误，仅已有警告）。**

### 2.2 Unity C# 导出器（`unity-exporter/MyRenderExport/MyRenderExporter.cs`）

- `Jb` 类新增 `Int()` 和 `Vec4()` 方法
- 每个对象 JSON 新增字段：`"lightmapIndex"` 和 `"lightmapScaleOffset"`
- `ExportActiveScene` 中新增：导出 `LightmapSettings.lightmaps` 所有 lightmap 纹理（TGA 格式，linear）
- `BuildSceneJson` 签名扩展：接受 `lightmapPaths` 并写入 `"lightmaps": [...]` 数组

---

## 三、当前阻塞：Unity 端尚未烘焙 lightmap

**已解决的部分（导出器同步）：**  
之前文档说"Unity 项目里的 .cs 是旧版"——本 session 已把含 lightmap 逻辑的新版
`MyRenderExporter.cs` 拷进两个 Unity 项目的 `Assets/Editor/MyRenderExport/`：
- `/g/unity_demo/urp2019/...`（← SampleScene 在这里）
- `/g/unity_demo/urp_sample/...`

**验证：**
```
grep -c lightmapIndex /g/unity_demo/urp2019/Assets/Editor/MyRenderExport/MyRenderExporter.cs
# = 1 （已含 lightmap 逻辑）
```

**仍未解决的部分（场景没有烘焙数据）：**  
`/g/unity_demo/urp2019/Assets/Scenes/` 目录下**没有任何 lightmap 纹理、
也没有 .lighting 文件**，即 SampleScene 从来没有烘焙过 GI。所以即使导出器同步了，
现在导出也只能得到空的 lightmaps 数组。

```
scene.json 现状（已确认）：
  has "lightmaps" key: False
  每个 object 的 lightmapIndex: MISSING（导出器同步前的旧导出）
```

---

## 四、下次启动需要做的事（顺序敏感）

### 步骤 1：在 Unity 里烘焙 GI（**必须，GUI 操作**）
1. 打开 `/g/unity_demo/urp2019/` 工程
2. 打开场景 `Assets/Scenes/SampleScene.unity`
3. **Window → Rendering → Lighting Settings**
   - Lighting Mode: **Baked Indirect**（或 Directional，看效果）
   - Lightmapper: **Progressive GPU**（快）/ CPU（稳）
   - Direct Samples / Indirect Samples 给个适中值
   - 勾选 **Auto Generate** 或点 **Generate Lighting**
4. 烘焙完应出现 `Assets/Scenes/SampleScene/Lightmap-0_comp_light.exr` 等

### 步骤 2：重新导出（导出器已是新版）
Unity 菜单：**MyRender → Export Active Scene**
导出完成后验证：
```
grep lightmapIndex <export>/scene.json        # 应有大量输出
grep "lightmaps"   <export>/scene.json        # 应有 "lightmaps": [...]
ls <export>/textures/ | grep -i lightmap      # 应有 Lightmap-0_comp_light.tga
```

### 步骤 3：重新 Build（C++ 侧已就绪，无需改动）
```
cmake --build G:\MyRender\build --config Release --target MyRender
```

### 步骤 4：运行并测量
```
MyRender.exe --capture-unity assets/unity_export/SampleScene out/lightmap/with_lm.bmp 0 0 2
python tools/mse.py out/lightmap/with_lm.bmp assets/unity_export/SampleScene/unity_ref.png
```

**预期：** 地板偏亮 +54 的症状被 lightmap 的空间 AO 还原，MSE 从 1398 显著下降。

---

## 五、Lightmap 管道工作原理（简述）

```
Unity Bake → lightmap 纹理(.tga) + 每对象 lightmapScaleOffset
         ↓
scene.json 解析 → ObjectAsset.lightmapIndex/ScaleOffset
         ↓
UnitySceneLoader → 加载纹理 → RenderObject.lightmapTex/ST/hasLightmap
         ↓
Scene::Render 每对象 → gpu::_Lightmap = obj.lightmapTex
         ↓
LitShader / SimpleLitShader :: InitializeInputData
  if (_LIGHTMAP) → inputData.bakedGI = lightmap.sample(UV2 * ST.xy + ST.zw)
  else           → inputData.bakedGI = SH9 或 _AmbientColor（当前逻辑）
         ↓
UniversalFragmentPBR / UniversalFragmentBlinnPhong 使用 bakedGI 作为间接漫反射
```

当 lightmap 未激活时（`_LIGHTMAP = false`），完全 fallback 到当前逻辑，
`bakedGIColor` 乘数照常生效。

---

## 六、其他待优化项（优先级较低）

1. **天空梯度过亮**：`skyboxVisualMid/Bot` 是 Unity sRGB 输出值，经 `lin3 → ACES` 后双重处理，地平线方向偏亮 ~10 单位（本次实测，比文档前版的 ~50 小）。需要在 SkyboxPass 或 ACES 步骤处理。
2. **bakedGIColor 归一**：lightmap 生效后，所有材质的 `bakedGIColor` 应重置为 `[1,1,1]`（lightmap 已接管 GI 计算，不需要再乘以近似系数）。
3. ~~**SimpleLitShader**：目前只修改了 LitShader，SimpleLitShader 没有 lightmap 支持。~~ **(本 session 已修复)**

---

*文件路径：`G:\MyRender\docs\status_lightmap_2026-06-20.md`*
*MSE 工具：`tools/mse.py`、`tools/mse_regions.py`*
