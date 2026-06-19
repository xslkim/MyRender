# 地板左下角不超出屏幕的 Bug 修复记录

> 场景：SampleScene（Unity 2019.4 URP 导出），仅一个 Ground 地板对象。
> 现象：Unity 渲染的地板左下角会超出屏幕；MyRender 渲染的地板左下角没有超出屏幕，整体偏小、缩在画面右上。
> 修复日期：2026-06-20

## 一、Bug 现象

用 `--capture-unity` 渲染 `assets/unity_export/SampleScene`，与同目录下的
`unity_ref.png`（Unity 在 960×540 下截的参考图）对比：

| 指标 | MyRender（修复前） | Unity 参考 |
|------|------|------|
| 地板像素覆盖的列范围 X | **[154, 959]** | [0, 959] |
| 地板像素覆盖的行范围 Y | **[230, 490]** | [..., 539] |
| 左边缘 x=0 | 无地板 | 有地板 |
| 最底行 y=539 | 无地板 | 有地板 |

直观说：Unity 的地板一直延伸到画面最左列(x=0)和最底行(y=539)，左下角被
裁出屏幕外；而 MyRender 的地板从 x=154 才开始、y=490 就结束，左下空一大片，
看起来“小一圈”。

## 二、根因（一句话）

`src/core/Mesh.hpp` 的 `Mesh::loadBinary` 用了
`Vec3f pos(f32(), f32(), f32())` 这种写法读取二进制顶点。C++ 标准规定函数
参数的求值顺序是**未指定（unspecified）**的，而每个 `f32()` 都会从文件流
顺序读 4 字节（有副作用）。MSVC Release 优化下三个 `f32()` 的执行顺序被打乱，
导致 position 的 X/Y/Z、normal 的分量、uv 的 u/v 全部错位。顶点坐标错位后，
经过 M 矩阵投影，地板几何整体偏移，左下角本该超出屏幕的部分跑到了屏幕内。

修复方式：把内联构造改成显式顺序赋值，强制从左到右求值：

```cpp
// 错误写法（求值顺序未指定）：
Vec3f pos(f32(), f32(), f32());

// 正确写法（强制顺序）：
float px = f32(), py = f32(), pz = f32();
Vec3f pos(px, py, pz);
```

## 三、详细排查过程（含错误方向）

### 第 1 步：怀疑 aspect / FOV / 投影矩阵 —— ❌ 排除

最先想到的是相机参数不一致。检查 `scene.json`：

- `fovVertical: 60`，`aspect: 1.7778`，`near: 0.3`，`far: 1000`
- MyRender 渲染分辨率 `Config::kScreenWidth/Height = 960/540`，aspect = 1.7778，一致。
- 导出器 `MyRenderExporter.cs` 的 `CameraJson` 是 verbatim 复制 Unity 的
  `worldToCameraMatrix` 和 `projectionMatrix`，且导出时强制 `cam.aspect = 960/540`。
- `out/unity_diag.txt`（Unity 端 `SceneDiagnostics.cs` 生成）显示，在 960×540 下
  Ground 的 NDC 范围是 `X:[-1.234, 1.186] Y:[-1.354, 0.126]`——**左下角 NDC
  X=-1.234、Y=-1.354 都 < -1，确实超出屏幕**。这是 Unity 自身矩阵算出来的，
  说明矩阵数据本身没问题。

用 Python 拿 `scene.json` 里的 `P*V*M` 矩阵乘 Ground 的 8 个 AABB 角点，算出
NDC X:[-1.234, 1.186]、Y:[-1.354, 0.125]，与 `unity_diag.txt` 完全一致，对应
屏幕坐标 X:[-112, 1048]、Y:[236, 634]（左边缘在 x=-112，底部到 y=634，都超出
屏幕）。**这正是 Unity 截图的样子**。

**结论**：投影/视图/模型矩阵、aspect、FOV 都正确。bug 不在这里。
（这是第一个走错的方向，花了不少时间核对矩阵和 aspect。）

### 第 2 步：怀疑导出的几何体顶点缺失 —— ❌ 排除

既然矩阵对，但 MyRender 画出来偏小，怀疑 `.mesh` 文件里地板顶点被截断了。

用 Python 解析 `ground_11702.mesh`：
- vertexCount=1536、indexCount=3252，与 `unity_diag.txt` 里 `verts=1536 tris=1084` 一致。
- 顶点 object-space 边界 X:[-1.830, 3.171]、Y:[-0.150, 0]、Z:[-1.340, 3.660]，
  与 Unity 报告的 `obj AABB lo=(-1.830, -0.150, -1.340) hi=(3.170, 0, 3.660)` **完全一致**。

**结论**：导出的几何是完整的，顶点没丢。bug 不在导出端。

### 第 3 步：怀疑视锥裁剪（frustum culling）误杀 —— ❌ 排除

`Scene::Render` 里有 `Frustum::TestAABB` 做对象级剔除。怀疑 AABB 测试把
部分在视锥外的 Ground 整个剔了。

临时把 `if (obj.hasAABB && !frustum.TestAABB(...)) continue;` 改成
`if (false && ...)` 禁用剔除，重新渲染：地板范围依然是 x[154,959] y[230,490]，
**没有任何变化**。

**结论**：frustum culling 不是原因。

### 第 4 步：怀疑 Sutherland-Hodgman 裁剪过度 —— ❌ 排除

`Render.hpp` 的 `CollectTriangle` 对跨视锥边界的三角形做 7 平面裁剪。
怀疑裁剪平面方程符号写反，把视锥内的部分也裁掉了。

用 wireframe 模式（`DV_WIRE=5`）抓图，测得地板几何范围仍是 x[154,959]
y[230,490]——连线框都没画到 x<154。再对照 Python 算的结果：地板最左的
“在视锥内的”顶点 NDC.x=-0.834，对应屏幕 x=79.5。也就是说 **x=79.5~154 这段
本该有视锥内的三角形（不需裁剪），却完全没画出来**。

**结论**：裁剪不是原因（裁剪只影响跨边界的三角形，不影响全可见三角形）。

### 第 5 步：怀疑面剔除（back-face culling）方向反了 —— ❌ 排除

`RasterizeTri` 里有 `IsFrontFace` + `mat.cull` 测试，`Scene::LoadUnity` 设
`SetFrontFaceSign(-1.0f)`。怀疑 winding 判断反了，把朝相机的三角形当背面剔了。

临时把整个 `switch (mat.cull)` 注释掉禁用面剔除，重新渲染：地板范围依然
x[154,959] y[230,487]，**没有变化**。

**结论**：面剔除不是原因。

### 第 6 步：在顶点变换阶段加日志，发现 NDC 与 Python 不符 —— ✅ 关键转折

在第 3~5 步都排除后，问题只剩下“顶点变换链本身算出的坐标不对”。在
`CollectTriangle` 里加日志，打印每个三角形的 object-space position 和变换后的
NDC。筛选 NDC.x < -0.5 的三角形，发现 MyRender 实际算出的 NDC 和 Python 用
同样矩阵算出的**完全对不上**：

- Python：最左的视锥内顶点 NDC.x = **-0.834**
- MyRender 日志：左区域三角形的 NDC.x 最小才 **-0.545**

矩阵一样、顶点索引一样，NDC 却不同——**只能是喂给矩阵的顶点坐标不一样**。

### 第 7 步：打印第一个三角形的 object-space 顶点，发现 X/Z 交换 —— ✅ 定位

在 `CollectTriangle` 打印 `vtx0.position`（即 `mesh.triangles[0][0].position`，
对应顶点索引 0）：

- MyRender 读到：`v0 = (3.660, -0.050, 3.170)`
- `.mesh` 文件实际（Python 逐字节解析）：`v[0] = (3.170, -0.050, 3.660)`

**X 和 Z 互换了**。继续看 normal：MyRender 读到 `(0,1,0)`，实际是 `(0,0,1)`，
Y/Z 也互换了；uv：MyRender 读到 `(-0.01, 0)`，实际是 `(~0, -0.01)`，u/v 互换了。
**所有用 `Vec?(f32(), f32(), ...)` 构造的字段都发生了分量错位**。

### 第 8 步：在 Mesh::loadBinary 里逐字段打印，确认是读取阶段错位 —— ✅ 锁定

在 `Mesh::loadBinary` 读取循环里，对前 3 个顶点打印 `pos`、`nrm`、`tan`、`uv0`
四个局部变量，以及构造后的 `Vertex` 字段。发现：

- 局部 `pos=(3.660, -0.050, 3.170)` ← 已经错了（X/Z 交换）
- 局部 `nrm=(0, 1, 0)` ← 已经错了（Y/Z 交换）
- 局部 `uv0=(-0.01, 0)` ← 已经错了（u/v 交换）

错误发生在 `Vec3f pos(f32(), f32(), f32())` 这一行**构造之前**，即三个 `f32()`
的求值顺序被打乱，从流里读字节的次序错了。

### 第 9 步：根因确认 —— C++ 参数求值顺序未指定

`Vec3f pos(f32(), f32(), f32())` 中，三个 `f32()` 是带副作用（推进文件流）的
函数调用。C++ 标准（C++17 之前）规定函数参数的求值顺序是 **unspecified**，
编译器可以任意顺序求值。MSVC 在 Release `/O2` 下会重排这些调用，导致：

- 期望顺序：读字节0→x, 字节1→y, 字节2→z
- 实际可能：读字节2→x, 字节1→y, 字节0→z（或其它排列）

结果 position 的 X/Z 互换、normal 的 Y/Z 互换、uv 的 u/v 互换。顶点坐标错位
后，地板的 object-space 范围从真实的 X∈[-1.83,3.17]/Z∈[-1.34,3.66] 变成了
X∈[-1.34,3.66]/Z∈[-1.83,3.17]，再经过 90° Y 旋转的 M 矩阵，整体几何偏移，
左下角不再超出屏幕。

> 注：C++20 起，运算符 `<<`/`>>` 和部分场景的求值顺序有调整，但**函数参数
> 的求值顺序至今仍是 unspecified**，这个 bug 在 C++20 下依然存在。

### 第 10 步：修复并验证 —— ✅ 完成

把内联构造改成显式顺序赋值：

```cpp
float px = f32(), py = f32(), pz = f32();
Vec3f pos(px, py, pz);
float nx = f32(), ny = f32(), nz = f32();
Vec3f nrm(nx, ny, nz);
float tx = f32(), ty = f32(), tz = f32(), tw = f32();
Vec4f tan(tx, ty, tz, tw);
float uu = f32(), vv = f32();
Vec2f uv0(uu, vv);
```

逗号表达式 `,` 保证左到右求值，`f32()` 顺序固定。

重新编译渲染，日志确认 `v[0] = (3.170, -0.050, 3.660)`，与 `.mesh` 文件一致。
测量渲染输出：

| 指标 | 修复前 | 修复后 | Unity 参考 |
|------|--------|--------|------------|
| 地板 X 范围 | [154, 959] | **[0, 959]** | [0, 959] |
| 地板 Y 范围 | [230, 490] | **[236, 539]** | [..., 539] |
| 左边缘 x=0 | 无 | **有（y487..539）** | 有 |
| 底部 y=539 | 无 | **有（x0..368）** | 有 |

左下角正确超出屏幕，与 Unity 参考一致。

## 四、错误方向总结

| # | 怀疑点 | 验证方法 | 结果 |
|---|--------|----------|------|
| 1 | aspect/FOV/投影矩阵不一致 | 对比 scene.json、unity_diag.txt、Python 矩阵运算 | ❌ 排除（矩阵完全一致） |
| 2 | 导出的几何顶点缺失 | Python 解析 .mesh 顶点数和边界 | ❌ 排除（与 Unity 一致） |
| 3 | 视锥裁剪误杀 | 临时禁用 frustum culling | ❌ 排除（范围不变） |
| 4 | Sutherland-Hodgman 裁剪过度 | wireframe 模式 + Python 算视锥内顶点位置 | ❌ 排除（全可见三角形也没画） |
| 5 | 面剔除方向反了 | 临时禁用 face culling | ❌ 排除（范围不变） |
| 6 | 顶点变换链算错 | CollectTriangle 加日志打印 NDC | ✅ 转折（NDC 与 Python 不符） |
| 7 | 顶点坐标本身读错 | 打印第一个三角形 object-space position | ✅ 定位（X/Z 交换） |
| 8 | Mesh 读取阶段错位 | loadBinary 逐字段打印 | ✅ 锁定（f32() 求值顺序） |
| 9 | C++ 参数求值顺序未指定 | 改成显式顺序赋值 | ✅ 根因确认 |

**教训**：前 5 步都在“渲染管线后半段”找原因，但真正的 bug 在最前面的“资源加载”
阶段。转折点是第 6 步——把变换后的 NDC 和独立计算对比，发现“同样的矩阵+同样的
顶点索引，算出的坐标不同”，这才把怀疑从渲染管线转向了顶点数据本身。

## 五、影响范围

这个 bug 不只影响 SampleScene。**所有用 `--capture-unity` / `--unity` 加载的
Unity 导出场景**（GardenScene、ValidationScene 等）的几何都受影响——顶点
position/normal/tangent/uv 全部错位。修复后这些场景的几何精度都会提升。

## 六、涉及的文件

- `src/core/Mesh.hpp` —— `Mesh::loadBinary`：核心修复（求值顺序）
- `src/core/Render.hpp` —— `CollectTriangle`：仅用于调试，已还原
- `src/core/Scene.hpp` —— `Scene::Render`：仅用于调试（禁用 culling），已还原
- `unity-exporter/MyRenderExport/SceneDiagnostics.cs` —— Unity 端诊断工具（生成 `unity_diag.txt`）
- `unity-exporter/MyRenderExport/ReferenceCapture.cs` —— Unity 端参考图截取（生成 `unity_ref.png`）

## 七、相关产物

- `out/sample_fixed.bmp` —— 修复后 MyRender 渲染输出
- `out/_probe.bmp` —— 修复前 MyRender 渲染输出（bug 复现）
- `out/sample_compare.png` —— 三图并排对比（左=修复前，中=修复后，右=Unity参考）
- `out/unity_diag.txt` —— Unity 端矩阵/顶点诊断报告
- `assets/unity_export/SampleScene/unity_ref.png` —— Unity 960×540 参考图
