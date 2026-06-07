# output3 — CPU软渲染 复刻Unity URP（对照图解版）

> 三个备选产出之一。与 output1 / output2 主题相同（同一个软渲染器、同样三集划分），但这一版的风格、结构、视觉侧重完全独立。

## 这一版的定位

**以"对照 Unity"为主轴的图解讲法。** 每讲一个环节，都走同一套三段式：

1. **Unity 是怎么做的**（Unity 截图 / 面板，当标准答案）
2. **我们怎么复刻**（图解动画讲原理）
3. **证明对上了**（MyRender 调试截图当证据）

适合学过一点 Unity / URP、想搞清楚"Shader 里到底发生了什么"的观众。这一版图解动画用得比另外两版多，因为它的卖点就是把抽象原理画清楚——但**结论永远落在真实截图上**。

- **视觉优先级**：真实截图（Unity 对照图 + MyRender 调试截图）承担"结论/证据"；图解动画承担"原理/流程"（坐标空间、管线、TBN 等必须图解的点）；AI 概念图用得最少。
- **语气**：精确、对照、像在做一次"复刻验收"。
- **数学/代码**：在对照需要时出现，点到为止，且都有截图佐证。

## 三集划分（每段都对照 Unity）

| 集 | 主题 | 对照主线 |
|----|------|---------|
| EP1 总览 | 复刻 URP 的整体结构 | URP 管线 vs 我们的管线；gpu 层 vs HLSL；JSON vs Inspector |
| EP2 管线 | 顶点到像素 | MVP / 裁剪 / 光栅化 / 深度，逐项对照 + 调试截图验证 |
| EP3 光照 | 着色与光照 | Lit·SimpleLit·UnLit 对应 URP；albedo→法线→PBR；多线程提速 |

## 视觉素材（`assets/`）

- Unity 对照：`unity_ref_1/2.jpg` `unity_inspector.jpg`
- MyRender 结果：`myrender_1/2.jpg` `final_front.png` `orbit*.png` `orbit.mp4`
- 调试证据：`wire` `wire_clip` `depth` `albedo` `uv` `normal_geom` `normal_mapped` `threads`

## 目录

```
output3/
├── assets/        Unity 对照图 + MyRender 截图 + 调试截图 + mp4
├── ep1/  meta.md + script.md
├── ep2/  meta.md + script.md
└── ep3/  meta.md + script.md
```

每个 `epN/` 是一个独立可构建的 AutoVideo 工程（≈10 分钟）。
