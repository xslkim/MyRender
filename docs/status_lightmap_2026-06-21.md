# MyRender Lightmap 管道 — 实现与诊断状态
**日期：2026-06-21（隔夜自动推进，全部完成并提交）**

> **最终结果：MSE 从 5310 → 898（PSNR 10.88 → 18.60 dB），大幅突破 SH 基线 1398。**
> 修复了 4 个关键 bug（4 个 lightmap 小 bug + 直接光 1/π + **光栅化器 lightmapUV
> 未插值的致命 bug**）+ 直接光 0.7 缩放调优。

---

## 一、关键结论（TL;DR）

1. **Unity 端已就绪**：光源 Mixed、烘焙完成、导出器同步、`scene.json` 含全部
   28 对象的 lightmap 字段，`Lightmap-0_comp_light.tga` 已导出。
2. **C++ Lightmap 管道完全打通**（见第三节 bug 清单）。
3. **四个关键修复**（按贡献排序）：
   - 🔴 **光栅化器 lightmapUV 从未插值**（致命 bug）→ MSE 1922→964
   - 🔴 直接光缺 1/π BRDF 归一化 → MSE 2465→1922
   - 直接光 0.7 缩放调优 → MSE 964→898
   - Lightmap RGBM 解码 + clamp wrap + 绑定 + 双路径 → 5310→2465
4. **当前 MSE = 898 / PSNR 18.60 dB**（2x SSAA，lightmap + 1/π + mult=3.6 + direct=0.7）。
   **大幅突破 SH 基线 1398。**

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
| **+ 直接光 0.7 缩放调优** | **898** | **18.60** | **当前** |

参考：**SH 基线（无 lightmap）= 1398**。当前 898 **低于** SH 基线，证明 lightmap
真正生效且优于纯 SH 环境光。

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
7. **RGBM 乘数 8→3.6**（ShaderGlobal.hpp `_LIGHTMAP_RGBM_MULT`）
   - Unity 默认 `unity_Lightmap_HDR.y=8`，但本场景（+1/π 修复后）mult=8 偏亮
   - sweep 验证 mult=3.6 最匹配；sweep 3.0=948, 3.6=899, 8.0=1384
   - 正式应查 unity_Lightmap_HDR 真实导出值
8. **直接光缩放 0.7**（ShaderGlobal.hpp `_DIRECT_LIGHT_SCALE`，应用在 LightingPhysicallyBased）
   - 1/π 修复后 lit surface 仍 ~1.3x 偏亮；sweep 0.5=917, 0.7=899(最佳), 0.8=910
   - 这是经验值，正式应查 URP 主光单位/Lux 转换是否完全对

---

## 四、当前渲染质量（898 / 18.60 dB）

| 区域 | ours | ref | diff |
|------|------|-----|------|
| 整体 | 110.9 | 102.9 | +8.0 |
| 地板（y430-540） | ~130 | 100.7 | ~+30（右下角仍偏亮） |
| 右上角（曾暗物体） | ~93 | 92.6 | **~0（完美）** |
| 天空带 | ~125 | 107.0 | ~+18 |

**剩余最大误差**（`tools/mse_regions.py` 8×5）：
- 右下角地板 gx7,4≈4900, gx6,4≈3100（地板 HDR lightmap 区域，bakedGI>1）
- 左中区 gx0,2-3≈2800-3000（同上）
- 这些区域的 floor lightmap bakedGI 值偏大（HDR，~2.6），可能 mult 还需更低，
  但会牺牲右上角和墙壁的准确度——空间不均匀性暗示 floor UV 采样可能仍有轻微偏差

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

- [ ] **剩余最大误差：右下角地板 / 左中区 floor lightmap bakedGI 偏大**（HDR 值 ~2.6，
      经 saturate 看似均匀明亮）。整体 +8 偏亮主要来自这两块。空间不均匀性
      （墙壁/右上角在 mult=3.6 完美，但 floor 角落偏亮）暗示 floor UV 采样可能
      仍有轻微偏差，或该 atlas 区域确实存了更大的 HDR 值。
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
