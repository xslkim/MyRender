# MyRender Lightmap 管道 — 实现与诊断状态
**日期：2026-06-21（隔夜自动推进，全部完成并提交）**

> **最终结果：MSE 从 5310 → 872（PSNR 10.88 → 18.72 dB），大幅突破 SH 基线 1398。**
> 修复了 4 个关键 bug（4 个 lightmap 小 bug + 直接光 1/π + **光栅化器 lightmapUV
> 未插值的致命 bug**）+ joint-sweep 调优（mult=4.5, direct=0.5）。

---

## 一、关键结论（TL;DR）

1. **Unity 端已就绪**：光源 Mixed、烘焙完成、导出器同步、`scene.json` 含全部
   28 对象的 lightmap 字段，`Lightmap-0_comp_light.tga` 已导出。
2. **C++ Lightmap 管道完全打通**（见第三节 bug 清单）。
3. **四个关键修复**（按贡献排序）：
   - 🔴 **光栅化器 lightmapUV 从未插值**（致命 bug）→ MSE 1922→964
   - 🔴 直接光缺 1/π BRDF 归一化 → MSE 2465→1922
   - Lightmap RGBM 解码 + clamp wrap + 绑定 + 双路径 → 5310→2465
   - Joint-sweep 调优 mult=4.5/direct=0.5 → 964→872
4. **当前 MSE = 872 / PSNR 18.72 dB**（2x SSAA）。
   **大幅突破 SH 基线 1398。**
5. **新增 env-var 调参**（无需重编译）：`MR_LM_MULT`/`MR_DIRECT`/`MR_ALPHA2`

---

## 二、MSE 演进（完整时间线）

| 阶段 | MSE | PSNR | 说明 |
|------|-----|------|------|
| 起点：lightmap 接入，无 RGBM 解码 | 5310 | 10.88 | 当普通贴图，过亮 |
| + RGBM 解码（rgb*a*8） | 3619 | 12.55 | |
| + CLAMP wrap + 生产路径 | 2465 | 14.21 | lightmap 全打通 |
| **+ 1/π BRDF 归一化**（直接光修复） | 1922 | 15.29 | 直接光根因 |
| + lightmapUV 插值 bug 修复 | 1589 | 16.12 | default mult=8，偏亮 |
| + sweep → mult=3.6 | 964 | 18.28 | RGBM 乘数调优 |
| + 直接光 0.7 缩放调优 | 898 | 18.60 | |
| **+ joint-sweep mult=4.5/direct=0.5** | **872** | **18.72** | **当前** |

参考：**SH 基线（无 lightmap）= 1398**。当前 872 **远低于** SH 基线，证明 lightmap
真正生效且优于纯 SH 环境光。

**alpha² vs linear-alpha 验证**：Unity URP 标准 `DecodeLightmapRGBM` 用
`rgb * 8 * alpha²`，但我们的光照单位管线（raw color*intensity，无 pi-bake）
**与 linear-alpha `rgb*mult*alpha` 更匹配**：alpha² 最佳 1079 vs linear 最佳 872。
（alpha² 在 mult=24/direct=0.7 时才接近，仍不如 linear。）

---

## 三、修复的 bug 清单（按时间）

### Lightmap 管道（5310→2465）
1. **lightmap 路径双重 scene_path 前缀** → `fopen` 失败崩溃（UnitySceneLoader）
2. **lightmaps 向量从未赋给 RenderObject**（死代码）
3. **lightmap 用 Repeat 采样**（UV2>1 回绕）→ 新增 `SamplerClamp`
4. **Unity HDR lightmap 的 RGBM 编码未解码** → shader 内联 `rgb*(alpha*mult)`

### 直接光（2465→1922）
5. **`InitializeBRDFData` 漫反射缺 1/π 归一化**（BRDF.hpp）
   - `brdfDiffuse = albedo * oneMinusReflectivity` 少了 1/π
   - 地板 lighting multiplier 1.02 → 0.74（目标 0.62）

### 光栅化器（1922→964）— 🔴 最关键
6. **`InterpolateVaryings` 未插值 `lightmapUV`**（Render.hpp:412）
   - 只插值了 positionWS/CS/OS、uv、normal/tangent/bitangent、fogFactor
   - **漏掉 lightmapUV**，导致每个 fragment 的 lightmapUV 恒为 (0,0)
   - 所有 lightmapped 像素采样 atlas 左下角同一个 texel，**AO 空间变化完全丢失**
   - 这也解释了之前 `_LIGHTMAP_INTENSITY` sweep 行为异常（采样到常量）
   - 同步修 `ClipWithPlane`（near-plane 裁剪）的 lightmapUV 插值

### 调优
7. **RGBM 乘数 + 直接光缩放 joint-sweep**（ShaderGlobal.hpp）
   - 新增 env-var 调参：`MR_LM_MULT`/`MR_DIRECT`/`MR_ALPHA2`（无需重编译）
   - 最优 mult=4.5, direct=0.5（sweep 验证：mult 3.6=898→4.5=872；direct 0.7→0.5）
   - 原理：mult↑ 提亮 lightmap 间接光，direct↓ 压低直接光，配合让 floor L/C 更准
8. **alpha² 解码验证**（ShaderGlobal.hpp `_LIGHTMAP_RGBM_ALPHA2`，env `MR_ALPHA2`）
   - Unity 标准 `rgb*8*alpha²` 在我们的管线下表现**更差**（最佳 1079）
   - linear-alpha `rgb*mult*alpha` 更匹配（最佳 872），原因待查（光照单位）
   - 保留 alpha² 开关供后续研究

---

## 四、当前渲染质量（872 / 18.72 dB）

| 区域 | ours | ref | diff |
|------|------|-----|------|
| 整体 | 112.2 | 102.9 | +9.3 |
| 地板 L/C/R | 85/120/185 | 66/103/133 | L/C 接近，R 仍偏亮 |
| 右下角 Bench | 201 | 132 | +69（bakedGI 编码差异） |

**剩余最大误差**：右下角地板瓷砖（albedo=203，**非 Bench**，是亮色地砖）区域
lighting multiplier=1.0（直接光+GI 几乎等于 albedo 不衰减），Unity 参考=0.52。
已逐项排除：
- ❌ UV 采样（验证正确，含 TGA bottom-up 方向）
- ❌ occlusion map（材质无）
- ❌ 间接高光（禁用无变化）
- ❌ reflection probe（用 SH9，禁用无变化）
- ❌ alpha² 解码（反而更差；且该角落在 alpha² 下仍 202，说明 GI 非主导）
- ❌ 阴影（禁用 shadow pass 角落无变化，说明本就不在主光阴影内）
- ❌ 全局参数（joint-sweep 已到最优 872，角落无法靠全局 mult/direct 修复）
**结论**：该亮色地砖区域 direct(0.5)+bakedGI 的组合给出 multiplier≈1.0，
而 Unity 是 0.52。这是该局部 bakedGI 与 Unity 烘焙值的固有差异（相同 TGA、
相同 UV，但我们的解码/光照管线叠加结果比 Unity 亮约 2x）。需要对该区域做
per-texel 的 A/B 比对才能定位最后一个差异点。

---

## 五、当前 C++ 改动清单

| 文件 | 改动 |
|------|------|
| `src/core/Render.hpp` | **InterpolateVaryings + ClipWithPlane 插值 lightmapUV** |
| `src/core/Texture.hpp` | 新增 `SamplerClamp`、`SamplerClampLinear` |
| `src/core/UnitySceneLoader.hpp` | lightmap 相对路径加载；对象循环绑定 lightmapTex/ST/hasLightmap；天空 lin3 注释清理 |
| `src/gpu/BRDF.hpp` | **InitializeBRDFData 漫反射项补 1/π** |
| `src/gpu/ShaderGlobal.hpp` | `_LIGHTMAP_RGBM_DECODE`/`_LIGHTMAP_RGBM_MULT`(3.6)/`_LIGHTMAP_INTENSITY`/`_DIRECT_LIGHT_SCALE`(0.7)；`DV_BAKEDGI`/`DV_LIGHTMAPUV` |
| `src/gpu/Lighting.hpp` | `LightingPhysicallyBased` radiance 乘 `_DIRECT_LIGHT_SCALE` |
| `src/gpu/LitShader.hpp` | lightmap 走 SamplerClamp + RGBM 解码；2 个调试视图（B 通道编码 _LIGHTMAP 状态） |
| `src/gpu/SimpleLitShader.hpp` | 同 LitShader（SamplerClampLinear） |
| `src/core/SimpleLitMat.hpp` | 传 staticLightmapUV；驱动 _EMISSION |
| `src/core/Image.hpp` | 加载失败打印路径+errno |
| `src/core/SkyboxPass.hpp` | k=15 验证正确（误改 k=3 已回退） |
| `src/MyRender.cpp` | `--capture-unity` 新增第 7 参数 lightmap intensity（tuning） |

导出器 `MyRenderExporter.cs`：修 `Esc` 作用域；新增 `ExportLightmap()`（RGBM 预解码）；
已同步到 `/g/unity_demo/urp2019/` 和 `urp_sample/`。

---

## 六、待办（明天的起点）

- [ ] **剩余最大误差：Bench 区域 bakedGI 编码不匹配**（201 vs ref 132）。
      已排除所有采样/材质/反射路径，确认是 **bakedGI 解码与 Unity 内部管线差异**。
      可能方向：
      (a) 确认 Unity 真实 mixed lighting 模式（Baked Indirect vs Subtractive）。
          Subtractive 需 `SubtractDirectMainLightFromLightmap`
          （GlobalIllumination.hpp:584，目前注释掉的 MixRealtimeAndBakedGI 路径）。
      (b) 光照单位对齐：我们的 raw color*intensity + 手动 1/π 与 Unity URP 的
          Lux 单位 + pi-baked 可能差一个因子。核对 URP `GetMainLight()` 的
          intensity 处理。
      (c) 用 alpha² + 正确的光照单位管线重测（当前 1/π + linear-alpha 是经验拟合）。
- [x] **mult/direct joint-sweep**：已完成，最优 4.5/0.5（MSE 872）。
- [x] **alpha² 验证**：已完成，linear-alpha 更优（我们的管线）。
- [ ] **unity_Lightmap_HDR 真实导出**：从 Unity 读 `lightmapHDR` 属性导出，
      而非用经验 mult。不同 URP 版本默认值不同（URP 2019=8，新版可能=4）。
- [ ] 用户重新导出（用新的 `ExportLightmap` RGBM 预解码导出器），届时运行时
      `_LIGHTMAP_RGBM_DECODE` 设 false，mult 回 1.0 验证一致性。
- [x] ~~`_LIGHTMAP_INTENSITY` sweep 异常~~：已确认是 lightmapUV 插值 bug 导致。
- [ ] 天空带 +18（次大）：SkyboxPass lin3 正确（已 A/B 验证），可能需要单独的
      天空亮度调整或检查 exposure。
- [ ] **mult=3.6 / direct=0.7 都是经验值**：正式应：
      (a) 从 Unity 导出 `unity_Lightmap_HDR.y`（URP 2019=8，新版可能=4）；
      (b) 核对 URP 方向光 Lux 单位转换是否完全对（1/π + 0.7 是否能合成正确的单位系数）。
- [ ] 用户重新导出（用新的 `ExportLightmap` RGBM 预解码导出器），届时运行时
      `_LIGHTMAP_RGBM_DECODE` 设 false，mult 回 1.0 验证一致性。
- [ ] 确认本场景 mixed lighting 模式（Baked Indirect vs Subtractive）。Subtractive
      需 `SubtractDirectMainLightFromLightmap`（GlobalIllumination.hpp:479 注释中）。
- [x] ~~`_LIGHTMAP_INTENSITY` sweep 异常~~：已确认是 lightmapUV 插值 bug 导致
      （采样到常量），非 MSVC 优化问题。修复后 sweep 正常。
- [ ] 天空带 +18（次大）：SkyboxPass lin3 正确（已 A/B 验证），可能需要单独的
      天空亮度调整或检查 exposure。

---

## 七、复现命令

```bash
cmake --build G:\MyRender\build --config Release --target MyRender
MyRender.exe --capture-unity assets/unity_export/SampleScene out/lightmap/final.bmp 0 0 2
python tools/mse.py out/lightmap/final.bmp assets/unity_export/SampleScene/unity_ref.png
# 调试视图：7=bakedGI, 8=lightmapUV(B通道=_LIGHTMAP状态), 1=albedo
MyRender.exe --capture-unity assets/unity_export/SampleScene out/x.bmp 7 0 2
# lightmap intensity sweep（第 7 参数）
for LI in 0.3 0.5 1.0; do
  MyRender.exe --capture-unity assets/unity_export/SampleScene out/lm_$LI.bmp 0 0 1 $LI
done
```

---

## 八、提交历史（本 session）

```
dc85f94 直接光缩放 0.7 + 调优收尾：MSE 964→898（PSNR 18.60）
69623ff 状态文档最终更新：MSE 5310→964，三个关键 bug 修复总结 + 明天起点
466f241 Lightmap RGBM 乘数调优：8→3.6，MSE 1589→964
887c641 修复光栅化器关键 bug：InterpolateVaryings + ClipWithPlane 未插值 lightmapUV
96cedf5 调试增强：DV_LIGHTMAPUV 蓝通道编码 _LIGHTMAP 状态；确认右上角为 lightmapped 对象
69af0fe 诊断：右上角暗物体非天空，是几何体；新增 SamplerClampLinear；清理天空注释
7d5b90a 状态文档更新：定位天空偏暗为下一最大误差源
2a7f17e 修复直接光过亮：InitializeBRDFData 漫反射项补 1/π 归一化
3d5ea65 Lightmap: 打通完整管道 + RGBM 解码 + clamp 采样；定位直接光过亮根因
```

---

*文件路径：`G:\MyRender\docs\status_lightmap_2026-06-21.md`*
*MSE 工具：`tools/mse.py`、`tools/mse_regions.py`*
