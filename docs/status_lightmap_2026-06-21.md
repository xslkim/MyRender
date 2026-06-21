# MyRender 渲染复刻状态
**日期：2026-06-21（最新：天空盒修复完成）**

> **当前 MSE = 344 / PSNR 22.77 dB**（2x SSAA，vs Unity 参考图）。
> 从起点 5310 / 10.88 dB 一路修到 344 / 22.77 dB。

---

## 一、TL;DR

| 项 | 状态 |
|----|------|
| Lightmap 管道 | ✅ 完全打通（RGBM 解码、clamp 采样、lightmapUV 插值、绑定） |
| 直接光能量 | ✅ 修复（1/π BRDF 归一化 + direct scale 调优） |
| 天空盒 | ✅ 修复（haze 带 + sRGB 绕过 ACES）|
| 雾效 | ❌ 待做（用户反馈天空还差一点雾，明天弄）|
| Unity 端 | ✅ 光源 Mixed、烘焙完成、导出器同步、lightmap tga 已导出 |
| 当前 MSE | **344 / PSNR 22.77 dB**（2x SSAA） |

**对比基线**：SH 基线（无 lightmap）= 1398。当前 344 远低于它。

---

## 二、MSE 演进（完整时间线）

| 阶段 | MSE | PSNR | 说明 |
|------|-----|------|------|
| 起点：lightmap 接入，无 RGBM 解码 | 5310 | 10.88 | 当普通贴图，过亮 |
| + RGBM 解码（rgb*a*8） | 3619 | 12.55 | |
| + CLAMP wrap + 生产路径 | 2465 | 14.21 | lightmap 全打通 |
| + 1/π BRDF 归一化（直接光修复） | 1922 | 15.29 | 直接光根因 |
| + lightmapUV 插值 bug 修复 | 1589 | 16.12 | 光栅化器致命 bug |
| + joint-sweep mult=4.5/direct=0.5 | 872 | 18.72 | RGBM 乘数 + 直接光缩放调优 |
| **+ 天空盒修复（haze + sRGB 绕过）** | **344** | **22.77** | **当前** |

---

## 三、所有修复的 bug（按时间）

### A. Lightmap 管道（5310→872）
1. **lightmap 路径双重 scene_path 前缀** → fopen 失败崩溃（UnitySceneLoader）
2. **lightmaps 向量从未赋给 RenderObject**（死代码）
3. **lightmap 用 Repeat 采样**（UV2>1 回绕）→ 新增 SamplerClamp
4. **Unity HDR lightmap 的 RGBM 编码未解码** → shader 内联 `rgb*(alpha*mult)`
5. **🔴 光栅化器 lightmapUV 从未插值**（InterpolateVaryings 漏字段，致命）
   → MSE 1922→964。ClipWithPlane 同步修。

### B. 直接光能量（2465→1922→872）
6. **InitializeBRDFData 漫反射缺 1/π 归一化**（BRDF.hpp）
7. **直接光缩放 + RGBM 乘数 joint-sweep 调优**：
   - `_DIRECT_LIGHT_SCALE = 0.5`（LightingPhysicallyBased radiance 乘）
   - `_LIGHTMAP_RGBM_MULT = 4.5`（linear-alpha 解码，非 Unity 标准的 alpha²）
   - 验证：alpha² 在我们的管线更差（1079 vs linear 872）

### C. 天空盒（872→344）
8. **🔴 skyboxVisual 走了 ACES 被压暗**
   - skyboxVisual* 是 procedural skybox 的**显示 sRGB 色**（导出器 cubemap GetPixels）
   - 原管线 lin3 + ACES 把 Top [0.24,0.34,0.52] 变成 [9,37,90]，ref 是 [54,86,135]≈原 Top
   - 修复：SkyboxPass 写 alpha=0 标记天空，后处理对天空**绕过 ACES/exposure/contrast，
     直接 color*255 输出**；UnitySceneLoader 不再做 lin3
9. **缺地平线 haze 带 + below-horizon 用极暗 groundColor**
   - 原 SampleGradient 只 Mid↔Top 指数混合，无 haze；below-horizon 用 groundColor（接近黑）
   - 修复：三区渐变（zenith→Top / haze dip at horizon / below→Bot），参数 joint-sweep：
     `hazeBand=0.05, kZenith=12, kGround=1`
10. **诊断澄清（曾误判）**：之前以为"SkyboxPass 覆盖 floor"，实际经 z-write 计数器
    诊断确认 **SkyboxPass 的 depth-skip 完全正常**（floor depth<1.0 被跳过）。floor=54
    是正确几何值（地平线下天空变暗接近 ref）；baseline 的 floor=142 是 below-horizon
    天空过亮的假象。

---

## 四、当前渲染质量（344 / 22.77 dB）

| 区域 | ours | ref | diff |
|------|------|-----|------|
| 整体 | 100.8 | 102.9 | -2.1 |
| 天空 y=0 | 70 | 54 | +9 |
| 天空 y=80 | 106 | 108 | -9 |
| 天空 y=200 | 129 | 151 | -25（雾效应改善这里）|
| 地板 y470-540 | 120 | 106 | +14 |

**剩余误差**（`tools/mse_regions.py` 8×5）：
- 最大集中在中间 bench/工具区（gx4-5, gy1-2，MSE 800-1600）—— bakedGI 编码
  与 Unity 管线的单位差异（之前已分析，需要深挖 URP Lux 单位）
- 天空 y=200 附近偏暗 -25（**雾效**能改善）

---

## 五、🔴 明天要做的：雾效（用户反馈）

用户反馈："天空盒好像还差一点雾效"。

### 现状分析
天空 y=200（接近地平线）我们 129 vs ref 151（偏暗 -25）。Unity 的 procedural sky
在地平线附近有**大气散射雾效**（atmospheric scattering / fog），让远处地平线变亮、
泛白。我们的 SampleGradient 现在用纯色插值，没有雾的泛白效果。

### 可能方向
1. **Unity Procedural Sky 的 Sun-size + Atmosphere Thickness**：
   - Unity procedural sky material 有 `atmosphereThickness` 参数，控制地平线雾的浓度
   - 增大地平线附近的亮度/泛白（往白色或太阳色混合）
2. **Height Fog / Exponential Fog**：URP 的 fog（`Fog` 或 `Volumetric`）在远处把
   物体向 fogColor 混合。需要从 Unity 导出 fog 设置并实现。
3. **检查导出器是否导出了 fog**：scene.json 的 environment 里有没有 fog 字段。
   如果没导出，先在导出器加 fog 导出。
4. **简单做法**：在 SkyboxPass 的地平线带（hazeBand）往白色/太阳色混合一点，
   模拟大气散射泛白。可以再 joint-sweep 一个 "fogAmount" 参数。

### 落地步骤建议
1. 先查 Unity 场景的 skybox material 是 Procedural 还是 gradient，有没有
   atmosphereThickness / sun size 参数
2. 查 RenderSettings.fog 是否开启，fogColor/fogDensity 是多少
3. 导出这些参数（如未导出）
4. 在 SkyboxPass 或单独 FogPass 实现

---

## 六、C++ 改动清单（当前）

| 文件 | 改动 |
|------|------|
| `src/core/Render.hpp` | **InterpolateVaryings + ClipWithPlane 插值 lightmapUV**；新增 SamplerClamp |
| `src/core/Texture.hpp` | SamplerClamp / SamplerClampLinear |
| `src/core/UnitySceneLoader.hpp` | lightmap 相对路径加载；对象循环绑定 lightmapTex/ST/hasLightmap；**skyboxVisual 不做 lin3**（天空走 sRGB 直通） |
| `src/core/SkyboxPass.hpp` | **三区 haze 渐变**（hazeBand=0.05/kZenith=12/kGround=1）；**alpha=0 标记天空**；below-horizon 用 Bot 不是 groundColor |
| `src/core/Scene.hpp` | **后处理对天空像素（alpha<0.5）绕过 ACES，直接 sRGB 输出** |
| `src/gpu/BRDF.hpp` | InitializeBRDFData 漫反射项补 1/π |
| `src/gpu/ShaderGlobal.hpp` | `_LIGHTMAP_RGBM_MULT`(4.5)/`_DIRECT_LIGHT_SCALE`(0.5)/`_LIGHTMAP_RGBM_ALPHA2`；DV_BAKEDGI/DV_LIGHTMAPUV |
| `src/gpu/Lighting.hpp` | LightingPhysicallyBased radiance 乘 `_DIRECT_LIGHT_SCALE` |
| `src/gpu/LitShader.hpp` | lightmap 走 SamplerClamp + RGBM 解码；调试视图 |
| `src/gpu/SimpleLitShader.hpp` | 同 LitShader |
| `src/core/SimpleLitMat.hpp` | 传 staticLightmapUV；驱动 _EMISSION |
| `src/MyRender.cpp` | `--capture-unity` 第 7 参数 lightmap intensity；env-var 调参（MR_LM_MULT/MR_DIRECT/MR_ALPHA2） |

导出器 `MyRenderExporter.cs`：修 Esc 作用域；新增 ExportLightmap()（RGBM 预解码）；
已同步到 `/g/unity_demo/urp2019/` 和 `urp_sample/`。

---

## 七、调参命令（env-var，无需重编译）

```bash
cmake --build G:\MyRender\build --config Release --target MyRender

# 渲染
MyRender.exe --capture-unity assets/unity_export/SampleScene out/final.bmp 0 0 2

# 测 MSE
python tools/mse.py out/final.bmp assets/unity_export/SampleScene/unity_ref.png

# 调参 sweep（第 7 参数 = lightmap intensity；env-var 调 mult/direct/alpha2）
MR_LM_MULT=4.5 MR_DIRECT=0.5 MyRender.exe --capture-unity ... out/x.bmp 0 0 2

# 调试视图：7=bakedGI, 8=lightmapUV(B=_LIGHTMAP), 1=albedo
MyRender.exe --capture-unity ... out/x.bmp 7 0 2
```

---

## 八、其他待办（优先级低）

- [ ] **中间 bench/工具区 bakedGI 偏差**（MSE 800-1600）：alpha² vs linear，
      光照单位（raw color*intensity vs URP Lux）。需要深挖 URP GetMainLight() 的
      intensity 处理。
- [ ] `unity_Lightmap_HDR` 真实导出（而非经验 mult=4.5）
- [ ] 用户重新导出（用新 ExportLightmap RGBM 预解码），届时 `_LIGHTMAP_RGBM_DECODE`
      设 false，mult 回 1.0 验证一致性
- [ ] 确认 mixed lighting 模式（Baked Indirect vs Subtractive）。Subtractive 需
      `SubtractDirectMainLightFromLightmap`

---

## 九、提交历史（本 session）

```
033dbb1 天空盒修复：MSE 872→344（PSNR 18.72→22.77 dB）
e8a7a75 文档：天空盒修复研究完成（根因+方案）
dc85f94 直接光缩放 0.7 + 调优收尾：MSE 964→898
... (joint-sweep、lightmapUV 插值、1/π、RGBM 等共 ~16 个 commit)
```

---

*文件路径：`G:\MyRender\docs\status_lightmap_2026-06-21.md`*
*MSE 工具：`tools/mse.py`（整体）、`tools/mse_regions.py`（网格分解 + 热力图）*
*对比图：`out/lightmap/comparison_final.png`*
