# MyRender 渲染复刻状态（实时光 + 雾）
**日期：2026-06-21（最新：实时光 + 真实雾 + 修复间接光 1/π bug + SH GI scale）**

> **当前 MSE = 241 / PSNR 24.32 dB**（2x SSAA, vs Unity 实时光参考图，全部真实参数）

---

## 一、重大变化：场景改为实时光
用户在 Unity 关闭了 lightmap 烘焙，改为**实时光**，重新导出场景 + 参考图。
- `lightmapIndex = -1`，无 `lightmaps` 数组 → 所有物体走 **SH 环境光 + 实时直接光**
- lightmap/bakedGI 那一整套（RGBM 解码、mult=4.5 等）在本场景**不再生效**

## 二、本 session 进展
| 阶段 | MSE | PSNR | 说明 |
|------|-----|------|------|
| 实时重导出基线 | 2327 | 14.46 | 全图偏暗（−29/−37/−40），蓝最缺 |
| + 实时雾（真实 RenderSettings.fog 值） | ~835 | 18.9 | 见下 #2 |
| + **修复间接光 1/π bug** + direct 0.5 | 351 | 22.68 | 见下 #1、#3 |
| + **SH GI scale 1.6** | 257 | 24.03 | 见下 #4 |
| + **软阴影 5×5 PCF（softness 4）** | **241** | **24.32** | 见下 #5 |

> ⚠️ 中途用"经验雾"曾到 508，但那是**过拟合**（偏强偏蓝的假雾掩盖了下面的 1/π bug）。
> 用真实雾 + 修 bug 才是正解。

### 1. `_DIRECT_LIGHT_SCALE`（[ShaderGlobal.hpp](../src/gpu/ShaderGlobal.hpp)）= 0.5
- 含义变了：现在它承载**直接光的 Lambert 1/π**（因为 #3 把 1/π 从 BRDF 移走了）
- 实时场景扫描最优 0.5（地板几乎完美，无过曝）；同时作用主光 + spot

### 3. 🔴 间接光 1/π bug（[BRDF.hpp](../src/gpu/BRDF.hpp)，本 session 核心修复）
- `brdfData.diffuse` 之前被乘了 `1/π`，但它**同时用于直接光和间接光**
  （[BRDF.hpp](../src/gpu/BRDF.hpp) `EnvironmentBRDF`: `indirectDiffuse * brdfData.diffuse`）
- 直接光有 `_DIRECT_LIGHT_SCALE` 补偿，**间接光（SH 环境光）没有 → 暗了 π≈3.14×**
- 症状：面朝天空开口的**青色墙**（albedo (3,176,209)、纯环境光照亮）暗 3 倍、失去青色
  （我们 (12,26,43) vs ref (0,67,112)）
- 修复：移除 BRDF 的 1/π，改由 `_DIRECT_LIGHT_SCALE` 只补偿直接光。青墙恢复 (12,50,88)

### 4. SH 环境光 scale 1.6（[ShaderGlobal.hpp](../src/gpu/ShaderGlobal.hpp) `_GI_SCALE`）
- 修完 #3 后仍有均匀 ~0.78× 欠光（白 studs、青墙都偏暗、均值 G/B −7.6/−9）
- 我们的 `EvaluateAmbientProbe` 原始求值约为 Unity 漫反射环境响应的 0.63×
  （疑似 probe 卷积/单位偏移）。`_GI_SCALE=1.6` 扫描最优，均值全部归零
- env 调参 `MR_GI`
- ⚠️ 仍是经验常量；若以后想根除，需对齐 Unity 的 SH→shader 常量卷积（A_l 各 band 权重）

### 5. 软阴影 5×5 PCF（[ShaderFunction.hpp](../src/gpu/ShaderFunction.hpp) `MainLightRealtimeShadow`）
- 原 3×3 PCF 步长仅 1 texel ≈ 硬阴影；Unity 软平行阴影 penumbra 更宽
- 改 5×5、核宽 `_ShadowSoftnessTexels`（env `MR_PCF`），扫描最优 **4**（MSE 257→241）
- 注意：中部 bench 簇主要是阴影**位置错位**而非软度，软化只是小幅收益；过软（>10）反而变差

### 2. 雾效（用户观察"天地分界模糊像雾"）
确认是 Unity 实时雾：远处暗物体被整体抬亮、出现蓝色霾、天地分界柔化。
**从头接通了整条雾路径（运行时开关，非编译宏）：**
- [ShaderFunction.hpp](../src/gpu/ShaderFunction.hpp)：实现 `ComputeFogFactorZ0ToFar` / `ComputeFogIntensity` / `IsFogEnabled`（linear/exp/exp2，匹配 URP）；`MixFogColor` 改运行时门控
- [LitShader.hpp](../src/gpu/LitShader.hpp) / [SimpleLitShader.hpp](../src/gpu/SimpleLitShader.hpp)：fragment 内按世界坐标算 `inputData.fogCoord`
- [ShaderGlobal.hpp](../src/gpu/ShaderGlobal.hpp)：新增 `int _FOG_MODE`（0=off,1=lin,2=exp,3=exp2）
- 一等环境字段链路：[SceneAsset.hpp](../src/core/SceneAsset.hpp)(FogAsset+解析) → [SceneModel.hpp](../src/core/SceneModel.hpp)(Fog) → [UnitySceneLoader.hpp](../src/core/UnitySceneLoader.hpp)(映射) → [Render.hpp](../src/core/Render.hpp)(`SetFog`→gpu)
- 导出器 [MyRenderExporter.cs](../unity-exporter/MyRenderExport/MyRenderExporter.cs)：新增 `FogJson()` 导出 `RenderSettings.fog`
- env-var 调参：`MR_FOG_MODE/DENSITY/START/END/R/G/B`（改 model，被 SetFog 读取）

> ✅ **scene.json 已是真实 `RenderSettings.fog`**：exp2, color=(0.381,0.402,0.459), density=0.05。
> （导出器两份工程拷贝 urp_sample/urp2019 都补了 `FogJson`，重导出自动带出。）

## 三、剩余 gap（~241，集中在中部 bench/studs 簇，已是长尾）
- 均值基本归零（−0.3 / −3.3 / −1.7）；最大 cell 全在中央 (3,1)=592/(2,1)=517/(2,2)=485
- 性质：**局部接触阴影 + 细几何**，正负相抵（所以均值平衡）：
  - bench/地板接触处缺投影/接触阴影（如 x520,y268 偏亮 +58）
  - 白色 studs 轻微欠光（0.70× vs ref 0.81×，约 −14%）+ 细杆亚像素锯齿
  - 平行光阴影边缘仍有轻微位置错位（软化已尽力，剩下是 shadow map 对齐）
- 这些**无单一全局杠杆**（GI/direct/PCF 都已扫到最优），需逐特征处理（接触阴影/AO、阴影对齐）

**下一步候选**（均为长尾、递减回报）：①接触阴影/SSAO；②平行光 shadow map 对齐（光向/depth bias）；
③根除 GI scale（对齐 Unity SH→shader 常量的 A_l band 卷积）。

## 四、关键文件改动（本 session）
- `src/gpu/BRDF.hpp`：🔴 移除 `brdfData.diffuse` 的 1/π（修复间接光暗 π 倍）
- `src/gpu/ShaderGlobal.hpp`：`_DIRECT_LIGHT_SCALE`=0.5（现承载直接光 1/π）；`_GI_SCALE`=1.6；新增 `_FOG_MODE`
- `src/gpu/{LitShader,SimpleLitShader}.hpp`：SH bakedGI 乘 `_GI_SCALE`
- `src/gpu/ShaderFunction.hpp`：主光阴影 3×3→5×5 PCF，核宽 `_ShadowSoftnessTexels`
- `src/MyRender.cpp`：env 调参 `MR_GI` / `MR_PCF`
- `src/gpu/ShaderFunction.hpp`：实现雾因子/强度/门控
- `src/gpu/LitShader.hpp` / `SimpleLitShader.hpp`：per-pixel fogCoord
- `src/core/{SceneAsset,SceneModel,UnitySceneLoader,Render,Scene}.hpp`：fog 一等字段链路
- `src/MyRender.cpp`：MR_FOG_* env 调参（改 model）
- `unity-exporter/MyRenderExport/MyRenderExporter.cs`：`FogJson()`
- `assets/unity_export/SampleScene/scene.json`：注入经验 fog 块（重导出后会被覆盖）

*对比图：`out/lightmap/compare_fog.png`（ours | ref | diff×2）*
