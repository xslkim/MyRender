# unity-urp-cpu — 从 Unity 导出到 CPU 软渲染：把 URP 一比一搬到 CPU

> 五集系列教学。面向**懂图形/渲染管线的程序员**（看得懂 shader、知道 MVP/光栅化）。
> 看完后，你会知道 Unity URP 渲染一帧里**数据怎么流、每一步怎么算**——因为我们把
> 整条管线用 C++ 在 CPU 上重写了一遍，每个中间量都能截图看见。

## 定位与风格

- **调试驱动 + 真实截图**：几乎所有"结果"都用渲染器**实跑出来的截图**（debug 视图、
  功能开关对比、与 Unity 对照），概念用动画图解，少量 AI 概念图。
- **端到端**：Unity C# 导出器 → JSON/mesh 数据 → 加载 → 渲染管线 → 与 Unity 对比验证。
- **本场景**：一个工地/工作台场景（bench、电钻、studs 框架、干墙、青色墙、地面），
  实时光 + 雾，最终与 Unity 参考图对齐到 PSNR ≈ 24 dB。
- **语气**：像同事在你旁边讲源码，务实、有据（每个结论都有截图/数字佐证）。

## 五集划分

| 集 | 主题 | 讲什么 | 核心截图 |
|----|------|--------|---------|
| **EP1** | 总览 + Unity 导出 | 架构地图；一帧的数据流；C# 导出器怎么把 Unity 场景写成 JSON+mesh；坐标系/矩阵/光照单位如何对齐 | `final` `wire` `compare_ours_ref` + Unity 编辑器截图 |
| **EP2** | 几何管线 | 加载 → 顶点变换(MVP) → Sutherland-Hodgman 裁剪 → 光栅化(包围盒+重心) → 透视校正插值 → 深度测试 | `wire` `uv` `normal_geom` |
| **EP3** | 光照与材质 | albedo/法线贴图 → PBR BRDF → 直接光(主光+spot) → SH 环境光；**一个真实的 1/π bug** 怎么把环境光暗了 π 倍 | `albedo` `normal_mapped` `ambient_sh` `ambient_bug` |
| **EP4** | 阴影 | 平行光 shadow map（正交拟合场景 AABB）→ 深度 pass → 5×5 PCF 软阴影；spot 透视阴影 | `wire`(光源视角) + 阴影开关对比 |
| **EP5** | 大气与后处理 + 验证 | 雾(exp2，URP fog 公式) → ACES tonemapping → 天空盒(三区渐变+绕过 ACES)；用 MSE/热力图与 Unity 逐区对比收尾 | `fog_off`↔`final` `diff_heatmap` `compare_ours_ref` |

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
├── assets/           渲染器真实截图 + Unity 对照图（你补）
├── ep1/ meta.md + script.md   总览 + 导出
├── ep2/ meta.md + script.md   几何管线
├── ep3/ meta.md + script.md   光照与材质
├── ep4/ meta.md + script.md   阴影
└── ep5/ meta.md + script.md   大气与后处理 + 验证
```

每个 `epN/` 是一个独立可构建的 AutoVideo 工程（≈10 分钟）。构建见 [`../../authoring/BUILD.md`](../../authoring/BUILD.md)。
