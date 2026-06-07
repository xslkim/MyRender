# output1 — CPU软渲染 复刻Unity URP（调试驱动版）

> 三个备选产出之一。三份内容主题一致（同一个软渲染器、同样三集划分），但风格、结构、视觉侧重完全独立，最终只选一份制作成视频。

## 这一版的定位

**面向 C++ 工程师的"调试驱动"讲法。** 卖点是：整条 GPU 管线被搬到 CPU 上，每一个阶段都能断点、能打印、**能直接截图看见**。所以本版几乎全程用**程序真实截图**当主角——线框、深度图、法线图、albedo、多线程分条，都是这个渲染器跑出来的真画面。

- **视觉优先级**：程序截图 >> 必要的图解动画 > AI 概念图（本版 AI 图用得最少）。
- **语气**：务实、像同事在你旁边讲源码，少废话。
- **数学**：只在必要处出现，且都配真实截图佐证。

## 三集划分

| 集 | 主题 | 核心截图 |
|----|------|---------|
| EP1 总览 | 架构 / JSON 场景 / 一帧五步 | `final_front` `wire` `unity_inspector` |
| EP2 管线 | 坐标变换 / 裁剪 / 光栅化 / 深度 | `wire` `wire_clip` `depth` |
| EP3 光照 | albedo→法线→PBR / 多线程 | `albedo` `uv` `normal_geom` `normal_mapped` `threads` |

## 截图素材怎么来的

为了这一版，我给渲染器加了一个 `--capture` 无窗口截图模式和一组调试可视化（`gpu::g_debugView`：线框 / 深度 / 几何法线 / 贴图法线 / albedo / UV / 多线程分条）。直接 `MyRender.exe --capture <dir>` 一次性导出全部 PNG。素材都在 `assets/`。

## 目录

```
output1/
├── assets/        所有截图 + Unity 对照图 + 环绕动画 mp4
├── ep1/  meta.md + script.md
├── ep2/  meta.md + script.md
└── ep3/  meta.md + script.md
```

每个 `epN/` 是一个独立可构建的 AutoVideo 工程（≈10 分钟）。
