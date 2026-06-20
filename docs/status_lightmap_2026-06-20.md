# MyRender Lightmap 管道实现状态
**日期：2026-06-20**

---

## 一、当前渲染质量

| 指标 | 数值 |
|------|------|
| MSE  | 972  |
| PSNR | 18.25 dB |
| 参考基准（本 session 起点） | MSE=1139, PSNR=17.57 dB |
| 累计改善（从 MSE=1700 起） | ΔPSNR = +2.43 dB |

---

## 二、本 Session 已完成的工作

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
| `src/gpu/LitShader.hpp` | 顶点着色器：计算 `lightmapUV = uv2 * ST.xy + ST.zw`；片元着色器：有 lightmap 时用贴图代替 SH9 作为 `bakedGI` |
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

## 三、还差一步：Unity 导出器未同步

**问题根因：**  
`unity-exporter/MyRenderExport/` 里的 `.cs` 文件是"源码备份"，不是 Unity 项目直接使用的文件。用户 Unity 项目 Assets/Editor 目录里的 `MyRenderExporter.cs` 还是旧版（无 lightmap 导出逻辑），所以导出的 scene.json 里没有 `lightmapIndex` 等字段。

**验证方法：**
```
grep "lightmapIndex" assets/unity_export/SampleScene/scene.json
# 输出 0 行 = 旧版导出器
# 有输出 = 新版导出器已生效
```

---

## 四、下次启动需要做的事

### 步骤 1：同步 Unity 导出器
找到 Unity 项目里的 `MyRenderExporter.cs`（通常在 `Assets/Editor/MyRenderExport/`），  
用以下文件替换：
```
G:\MyRender\unity-exporter\MyRenderExport\MyRenderExporter.cs
```
等 Unity 自动重新编译（Console 无报错）。

### 步骤 2：重新导出场景
Unity 菜单：**MyRender → Export Active Scene**  
导出完成后验证：
- `scene.json` 里每个 object 有 `"lightmapIndex": 0`（或其他数字）
- `assets/unity_export/SampleScene/textures/` 里有 `Lightmap-0_comp_light.tga`（或类似名称）

### 步骤 3：重新 Build
```powershell
cmake --build G:\MyRender\build --config Release --target MyRender
```

### 步骤 4：运行并测量 MSE
运行渲染器，用 `--capture` 截图，然后跑 Python MSE 脚本对比参考图。

**预期效果：**  
地板的空间 AO 变化（左侧 vs 中心 vs 右侧）将被 lightmap 正确还原，  
预计 MSE 从 972 大幅下降（地板区域当前误差约占总 MSE 的 50%+）。

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
LitShader::LitInitializeInputData
  if (_LIGHTMAP) → inputData.bakedGI = lightmap.sample(UV2 * ST.xy + ST.zw)
  else           → inputData.bakedGI = SH9 或 _AmbientColor（当前逻辑）
         ↓
UniversalFragmentPBR 使用 bakedGI 作为间接漫反射
```

当 lightmap 未激活时（`_LIGHTMAP = false`），完全 fallback 到当前逻辑，`bakedGIColor` 乘数照常生效。

---

## 六、其他待优化项（优先级较低）

1. **天空梯度过亮**：`skyboxVisualMid/Bot` 是 Unity sRGB 输出值，经 `lin3 → ACES` 后双重处理，地平线方向偏亮 ~50 单位。需要在 SkyboxPass 或 ACES 步骤处理。
2. **bakedGIColor 归一**：lightmap 生效后，所有材质的 `bakedGIColor` 应重置为 `[1,1,1]`（lightmap 已接管 GI 计算，不需要再乘以近似系数）。
3. **SimpleLitShader**：目前只修改了 LitShader，SimpleLitShader 没有 lightmap 支持（影响较小的材质）。

---

*文件路径：`G:\MyRender\docs\status_lightmap_2026-06-20.md`*
