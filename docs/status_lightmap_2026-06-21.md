# MyRender Lightmap 管道 — 实现与诊断状态
**日期：2026-06-21**

> 本文档为隔夜自动推进的完整记录。**Lightmap 管道打通 + 直接光过亮根因已修复
> （缺失的 1/π BRDF 归一化）。MSE 从 2465 → 1922。**

---

## 一、关键结论（TL;DR）

1. **Unity 端已就绪**：光源改 Mixed、烘焙完成、导出器已同步、`scene.json`
   含全部 28 个对象的 `lightmapIndex`/`lightmapScaleOffset` + `lightmaps` 数组，
   `Lightmap-0_comp_light.tga` 已导出。
2. **C++ 端 Lightmap 管道全部打通且可运行**（4 个 bug 全修，见第三节）。
3. **🔴 最大单一改善 = 找到并修复缺失的 1/π**：直接光过亮的根因是
   `InitializeBRDFData` 的 diffuse 项 `albedo * oneMinusReflectivity` 少了 1/π
   归一化。补上后地板 lighting multiplier 从 1.02 → 0.74（目标 0.62），
   **MSE 2465 → 1922（PSNR 14.21 → 15.29 dB）**。
4. 当前平均误差 R=-2.4 G=-9 B=-15（轻微偏暗），剩余误差集中在**天空/背景区域**
   （右上、右下），地板已基本达标。

---

## MSE 演进（更新）

| 状态 | MSE | PSNR | 说明 |
|------|-----|------|------|
| SH 基线（无 lightmap） | 1398 | 16.68 | lightmap 接入前 |
| lightmap 接入，无 RGBM 解码 | 5310 | 10.88 | 当成普通贴图，过亮 |
| + RGBM 解码 | 3619 | 12.55 | |
| + CLAMP wrap + 生产路径 | 2465 | 14.21 | lightmap 全打通 |
| **+ 1/π BRDF 归一化（直接光修复）** | **1922** | **15.29** | **当前** |



---

## 二、MSE 演进

| 状态 | MSE | PSNR | 说明 |
|------|-----|------|------|
| SH 基线（无 lightmap） | 1398 | 16.68 | lightmap 接入前 |
| lightmap 接入，**无 RGBM 解码** | 5310 | 10.88 | 当成普通贴图，过亮 |
| + RGBM 解码 | 3619 | 12.55 | 改善 |
| + CLAMP wrap 修复 + 生产路径 | **2465** | **14.21** | 当前 |

注意：MSE 比 SH 基线高，**不是 lightmap 让画面变差**，而是 lightmap 暴露了
一个更深的直接光过亮问题（见第五节）。SH 基线碰巧在这个场景上"错得对"。

---

## 三、本 session 修复的 4 个 bug

### Bug 1：lightmap 路径双重 `Config::scene_path` 前缀（崩溃）
`UnitySceneLoader.hpp` 旧代码：
```cpp
TextureCache::Get().GetTexture(Config::scene_path + lp, ...)  // 双重前缀！
```
而 `TextureCache::GetTexture` 内部已经 prepend `Config::scene_path`，导致路径变成
`assets/.../SampleScene/assets/.../SampleScene/textures/Lightmap-0...tga`，
`fopen` 失败触发 `assert(file != NULL)` → 进程崩溃（Release 下表现为
`--capture-unity` 静默退出 exit 127，Debug 下才看到 assert）。

**修复**：只传相对路径 `GetTexture(lp, ...)`。

### Bug 2：`lightmaps` 向量是死代码，从未赋给 RenderObject
预加载了 `std::vector<Texture2D*> lightmaps`，但对象循环里**没有任何代码**
设置 `ro.lightmapTex`/`ro.lightmapST`/`ro.hasLightmap`。`Scene.hpp` 读的全是
默认值（`nullptr`/`false`），lightmap 形同未启用。

**修复**：对象循环中按 `o.lightmapIndex` + `o.lightmapScaleOffset` 正确绑定。

### Bug 3：lightmap 用 Repeat 采样（UV>1 时回绕，AO 模式被毁）
`SamplerPoint`/`SamplerLinear` 都用 `frac(u)`（Repeat wrap）。但 Unity 烘焙
atlas 用 **CLAMP**：本场景 Ground 的 UV2 范围 `u∈[0.020, 1.075]`，u=1.075
应 clamp 到 atlas 边缘（u≈0.486），却被回绕成 0.075，导致整个地板塌缩到
atlas 一小块、读到错误/平坦的 texel，AO 空间变化丢失。

**修复**：`Texture.hpp` 新增 `SamplerClamp()`，lightmap 采样专用它。

### Bug 4：Unity HDR lightmap 的 RGBM 编码未解码
Unity HDR lightmap 是 **RGBM**：`final = rgb × (alpha × multiplier)`，
multiplier（`unity_Lightmap_HDR.y`）URP 默认 8。导出器旧注释错误地以为
`GetPixels()` 会解码（实际 `Graphics.Blit` 只拷贝编码字节），导致运行时
读到的是未解码的 rgb（太小）。

**修复（运行时，已生效）**：shader 内联解码
`bakedGI = lm.rgb * (lm.a * _LIGHTMAP_RGBM_MULT)`。
**修复（导出器，待重新导出生效）**：`MyRenderExporter.cs` 新增 `ExportLightmap()`
通过 `GetPixels()` 真正解码 RGBM→linear，写出已解码的 TGA（届时运行时
`_LIGHTMAP_RGBM_DECODE` 可设 false）。

---

## 四、当前 C++ 改动清单

| 文件 | 改动 |
|------|------|
| `src/core/UnitySceneLoader.hpp` | lightmap 相对路径加载；对象循环绑定 lightmapTex/ST/hasLightmap |
| `src/core/Texture.hpp` | 新增 `SamplerClamp()`（lightmap 专用） |
| `src/gpu/ShaderGlobal.hpp` | 新增 `_LIGHTMAP_RGBM_DECODE`/`_LIGHTMAP_RGBM_MULT`/`_LIGHTMAP_INTENSITY`；新增 `DV_BAKEDGI`/`DV_LIGHTMAPUV` 调试视图 |
| `src/gpu/LitShader.hpp` | lightmap 走 `SamplerClamp` + RGBM 解码；新增 2 个调试视图 |
| `src/gpu/SimpleLitShader.hpp` | 同 LitShader（lightmap 一致性） |
| `src/core/SimpleLitMat.hpp` | `InitAttributes` 传 `staticLightmapUV`；驱动 `_EMISSION` |
| `src/core/Image.hpp` | 加载失败时打印路径+errno（诊断用，无害） |
| `src/MyRender.cpp` | `--capture-unity` 新增第 7 个参数 lightmap intensity（tuning sweep 用） |

导出器：`unity-exporter/MyRenderExport/MyRenderExporter.cs`
- 修 `Esc` 作用域（CS0103）
- 新增 `ExportLightmap()`（RGBM 预解码）
- 已同步到 `/g/unity_demo/urp2019/` 和 `urp_sample/`

---

## 五、🔴 真正的根因：直接光照过亮（下一步重点）

**地板的 lighting multiplier 测量（关键证据）：**
```
albedo view 地板均值: 163.7   （纯材质色，无光照）
final render 地板均值: 164.7   → lighting multiplier = 1.01
Unity 参考图地板均值: 100.7   → target multiplier    = 0.62
```

我们的渲染对地板**几乎不施加任何明暗变化**（multiplier≈1.0），而 Unity 施加了
0.62（AO + 阴影 + 正确的间接光衰减）。lightmap 提供的间接光（diffuse GI ~0.1-0.5）
相对这个过强的直接光太小，被淹没，所以 sweep intensity 时 MSE 几乎不变。

**为什么直接光这么强？需要排查的方向（按优先级）：**

1. **主光强度/方向**：scene.json `mainLight.intensity=2`。检查 `model.light.color`
   是否被乘了 intensity 两次，或 lambert/BRDF 计算时多乘了 π。
   `UniversalFragmentPBR` → `LightingPhysicallyBased` 的能量项是否偏大。
2. **环境光/SH 重复**：`bakedGI`（lightmap）之外，`GlossyEnvironmentReflection`
   （line 405-409 of GlobalIllumination.hpp）**独立**用 SH9 算了间接高光。
   对粗糙地板这个值应很小，但若 SH9 偏亮会叠加。检查 `_SH9_VALID` 和
   `EvaluateAmbientProbe` 输出量级。
3. **ACES tonemapping**：是否对中间灰做了额外提亮。
4. **bakedGIColor 材质乘数**：`LitShader` line ~156 `color.rgb *= _BakedGIColor`，
   lightmap 生效后应重置为 1（status 旧文档第 6.2 项）。

**建议的第一个实验（✅ 已做，结论明确）**：临时把 `bakedGI` 强制为 0，
看地板 multiplier。结果：**bakedGI=0（纯直接光）地板 multiplier 仍 = 1.02**。
→ **直接光本身过亮，与 GI 无关。明天应从方向 1/2 入手（主光强度/能量、
   lambert/π 系数），而不是继续调 lightmap。**

---

## 六、复现命令

```bash
# 构建
cmake --build G:\MyRender\build --config Release --target MyRender

# 渲染（2x SSAA，lightmap 开）
MyRender.exe --capture-unity assets/unity_export/SampleScene out/lightmap/final_lm.bmp 0 0 2

# 测 MSE
python tools/mse.py out/lightmap/final_lm.bmp assets/unity_export/SampleScene/unity_ref.png

# 调试视图（dv=7 bakedGI，dv=8 lightmapUV，dv=1 albedo）
MyRender.exe --capture-unity assets/unity_export/SampleScene out/x.bmp 7 0 1

# lightmap intensity sweep（第 7 参数）
for LI in 0 0.3 0.6 1.0; do
  MyRender.exe --capture-unity assets/unity_export/SampleScene out/lm_$LI.bmp 0 0 1 $LI
done
```

---

## 七、待办（明天的起点）

- [x] ~~直接光过亮~~：已修复（1/π BRDF 归一化）。MSE 2465→1922。
- [ ] **🔴 天空太暗（下一个最大单一误差源）**：
      测量：天空右上区域 ours=34.6 vs ref=97.2（差 -62.6）；整条天空带 -24.3。
      `skyboxVisualTop`=[0.24,0.34,0.52]（sRGB），lin3 后约 [0.045,0.092,0.22]（均值 0.12），
      经 ACES+sRGB 输出后偏暗。**与 1/π 修复无关**（SkyboxPass 不走 BRDF），
      是独立的预存问题。
      建议方向：
      (a) 检查 `SkyboxPass::SampleGradient`：`skyboxVisualMid`=[0.62,0.72,0.83]（sRGB）
          才是天空主色（地平线以上大部分像素），`dir.y` 大的区域应更多用 Mid 而非 Top。
          当前 k=15 让 Top 在 t>0.2 时几乎全占——可能混合权重过快。
      (b) 检查这些 visual 颜色是否该直接当 sRGB 用（不经 lin3）——因为它们是
          "visual appearance"（已经是屏幕上的颜色），而 UnitySceneLoader 把它们
          当 linear 转。如果 Unity 导出的就是屏幕色，应跳过 lin3。
      (c) 检查 ACES 对低值（0.12）的压缩是否过强。
- [ ] **整体轻微偏暗**（平均 -2.4/-9/-15）：1/π 略有过度，主要也是天空贡献。
      地板（+20.7）仍偏亮一点点。修天空后可能整体平衡更好。
- [x] Legacy car 场景已 sanity check（仍能渲染）。
- [ ] 用户重新导出场景（用新的 `ExportLightmap` RGBM 预解码导出器），届时运行时
      把 `_LIGHTMAP_RGBM_DECODE` 设 false 验证一致性。
- [ ] 确认本场景 mixed lighting 模式（Baked Indirect vs Subtractive）。Subtractive
      需 `SubtractDirectMainLightFromLightmap`（目前 commented out）。
- [ ] `_LIGHTMAP_INTENSITY` sweep 行为异常（疑似 MSVC 优化缓存全局）——用
      `volatile` 或 `-Od` 验证。DV_BAKEDGI 已确认 lightmap 路径本身正常。

---

*文件路径：`G:\MyRender\docs\status_lightmap_2026-06-21.md`*
*MSE 工具：`tools/mse.py`（整体）、`tools/mse_regions.py`（网格分解 + 热力图）*
