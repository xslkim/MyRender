# unity-urp-cpu — 从 Unity 导出到 CPU 软渲染：把 URP 一比一搬到 CPU

> 两集系列教学（每集偏长，≈18–20 分钟）。面向**懂图形/渲染管线的程序员**
> （看得懂 shader、知道 MVP/光栅化）。看完后，你会知道 Unity URP 渲染一帧里
> **数据怎么流、每一步怎么算**——因为我们把整条管线用 C++ 在 CPU 上重写了一遍，
> 每个中间量都能截图看见。

## 定位与风格

- **调试驱动 + 真实截图**：几乎所有"结果"都用渲染器**实跑出来的截图**（debug 视图、
  功能开关对比、与 Unity 对照），概念用动画图解，少量 AI 概念图。
- **端到端**：Unity C# 导出器 → JSON/mesh 数据 → 加载 → 渲染管线 → 与 Unity 对比验证。
- **本场景**：一个工地/工作台场景（bench、电钻、studs 框架、干墙、青色墙、地面），
  实时光 + 雾，最终与 Unity 参考图对齐到 PSNR ≈ 24 dB。
- **语气**：像同事在你旁边讲源码，务实、有据（每个结论都有截图/数字佐证）。

## 两集划分

| 集 | 主题 | 讲什么 | 核心素材 |
|----|------|--------|---------|
| **上集** `ep1` | 导出 + 几何管线 | 架构地图；一帧数据流；C# 导出器把 Unity 场景写成 JSON+mesh；坐标系/矩阵/光照单位对齐；然后顶点变换(MVP) → 裁剪 → 光栅化 → 透视校正插值 → 深度 → 多线程 | `final` `orbit.mp4` `unity_exporter_menu` `wire` `uv` `threads` |
| **下集** `ep2` | 光照阴影与成像 | albedo/法线 → PBR 直接光(主光+spot) → SH 环境光 → **真实 1/π bug(环境光暗 π 倍)**；阴影(正交 shadow map + 5×5 PCF 软阴影 + spot)；雾(exp2) → ACES → 天空盒；用热力图/对比图与 Unity 对账收尾 | `albedo` `normal_mapped` `direct_only` `ambient_*` `cyan_*_crop` `shadow_*` `fog_off` `diff_heatmap` `compare_ours_ref` |

> 上集 17 块、下集 20 块，每集约 18–20 分钟。按"形状 → 颜色"切分：上集把三角形送上屏幕，下集给像素上色。

## 截图素材怎么来的

渲染器自带 debug 可视化（`gpu::g_debugView`）和一组 env 调参旋钮。批量生成命令：

```bash
SC=assets/unity_export/SampleScene; EXE=./build/Release/MyRender.exe
# debug 视图：0 final / 1 albedo / 2 normalGeom / 3 normalMapped / 4 uv / 5 wire / 6 threads / 7 bakedGI(SH环境光)
$EXE --capture-unity $SC out.bmp <view> 0 2
# 功能隔离：雾关 / 模拟 1π bug(环境光×0.625) / 灭灯
MR_FOG_MODE=0 $EXE --capture-unity $SC fog_off.bmp 0 0 2
MR_GI=0.625  $EXE --capture-unity $SC ambient_bug.bmp 0 0 2
```

素材都在 [`assets/`](assets/)。**Unity 编辑器界面截图需你自行补充**——脚本里凡是
`@visual: image(../assets/unity_*.png)` 的块，都是占位，已在该块 narration 上方用
`<!-- TODO Unity 截图 -->` 标注，请截图后按文件名放进 `assets/`。

## Unity 侧素材：脚本一键生成

在 urp2019 打开 `SampleScene`，点菜单 **MyRender ▸ Capture Video Assets**（脚本
[`MyRenderVideoCapture.cs`](../../../unity-exporter/MyRenderExport/MyRenderVideoCapture.cs)，
输出路径写死为本 `assets/` 目录），自动产出：

| 产物 | 用在哪 | 说明 |
|----|----|----|
| `orbit.mp4` | EP1 #B05 | 真实 URP 环绕 hero 视频（120 帧序列由仓库侧 ffmpeg 合成） |
| `unity_scene_game.png` | 备用静帧 | 主相机机位"标准答案"静图 |
| `unity_fog_off.png` | 备用 | Unity 关雾对照（呼应 EP5） |

**仅剩 1 张需手动截（可选）**：`unity_exporter_menu.png`（EP1 #B06，导出器菜单 UI，
脚本拍不到）。不想截就把 #B06 改成动画图解即可。

## 目录

```
unity-urp-cpu/
├── README.md         本文件
├── assets/           渲染器真实截图 + Unity 对照图/视频
├── ep1/ meta.md + script.md   上集：导出 + 几何管线（17 块）
└── ep2/ meta.md + script.md   下集：光照阴影与成像（20 块）
```

`ep1/` 与 `ep2/` 各是一个独立可构建的 AutoVideo 工程（每集 ≈18–20 分钟）。构建见 [`../../authoring/BUILD.md`](../../authoring/BUILD.md)。
