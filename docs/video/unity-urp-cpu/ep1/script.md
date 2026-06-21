>>> 开场钩子 #B01
@enter: fade-up
@exit: fade
@visual: image(../assets/final.png)

--- visual ---
（实际使用 ../assets/final.png：MyRender 软渲染器输出的工地场景，960x540——工作台、电钻、木方框架、青色墙、地面，远处带一层淡淡的雾。）

--- narration ---
你看到的这个工地场景
**没有用到任何显卡**
每一个像素都是 CPU 算出来的
但它不是凭空建模的
它原本是一个 **Unity** 场景
我们把它从 Unity 里导出
再用 C++ 在 CPU 上重新渲染了一遍


>>> 这个系列要干什么 #B02
@enter: fade
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)，垂直居中布局，内容占画布约 88% 宽度。
[0s] 顶部主标题 "从 Unity 导出到 CPU 软渲染" 淡入，白色 (#e6edf3)，粗体，字号 80px，居中。
[0.5s] 主标题下方 28px 处副标题 "把 URP 一比一搬到 CPU，看懂渲染一帧的每一步"，颜色 #8b949e，字号 46px。
[1s] 副标题下方 24px 处一条 4px 粗的 accent 色 (#58a6ff) 横线，从左到右扫入，宽度等于主标题。
[1.6s] 下方五个小卡片横向等距排列，间距 28px，总宽占画布 94%，每张高 230px，圆角 14px，背景 #161b22，边框 1px solid #30363d：
  卡片1 顶部 "EP1" 48px accent 色 + 标题 "总览·导出" 30px + 描述 "Unity→JSON" 22px #8b949e
  卡片2 顶部 "EP2" 48px accent 色 + 标题 "几何管线" 30px + 描述 "变换·裁剪·光栅化" 22px
  卡片3 顶部 "EP3" 48px accent 色 + 标题 "光照材质" 30px + 描述 "PBR·环境光" 22px
  卡片4 顶部 "EP4" 48px accent 色 + 标题 "阴影" 30px + 描述 "shadow map·PCF" 22px
  卡片5 顶部 "EP5" 48px accent 色 + 标题 "大气后处理" 30px + 描述 "雾·ACES·天空盒" 22px
卡片依次从下淡入上移，间隔 0.25s。第一张卡片有一圈 accent 色高亮边框表示"当前这一集"。

--- narration ---
我们分成五集
第一集，总览和导出
建立全局地图，再看 Unity 场景怎么变成数据
第二集，几何管线
第三集，光照和材质
第四集，阴影
第五集，大气和后处理
每一集都对着真实跑出来的画面讲
看完你会知道 URP 渲染一帧
数据到底怎么流


>>> 为什么要这么折腾 #B03
@enter: fade-up
@exit: fade
@visual: image

--- visual ---
一张左右对比的概念插画，深色科技风，背景接近 #0d1117 的深蓝黑，点缀色科技蓝 (#58a6ff)，3D 渲染质感，干净、有景深、轻微辉光。
左半边代表 GPU：一个封闭的黑色磨砂玻璃立方体，表面冰冷反光，内部隐约透出暗红微光却完全无法窥探，正面浮现一个发光的挂锁意象，氛围神秘、封闭。
右半边代表 CPU：一块揭开了顶盖的银色处理器芯片，内部电路纹理清晰、泛着明亮的科技蓝光，几个悬浮的发光圆球像调试断点一样停在电路上空，一束放大镜般的探查光打在芯片上，氛围通透、尽在掌控。
左暗右亮、左封闭右敞开，对比强烈。画面中不要出现任何文字或字母；主体居中偏上，画面最底部保留一条干净的深色空间方便叠加字幕。

--- narration ---
URP 的渲染跑在 **GPU** 上
你没办法设断点
没办法打印一个中间变量
出了问题只能靠猜
那不如把整条管线搬到 **CPU** 上
用 C++ 一行行重写
慢一点没关系
每一个中间量你都能打印、能截图
而且有 Unity 当 **标准答案** 对照
错了多少，一测就知道


>>> 端到端的数据流 #B04
@enter: fade
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)。
中央横向流程图，总宽占画布 94%，节点从左到右用 accent 色箭头连接，节点高 120px，圆角 12px，背景 #161b22，边框 1px solid #30363d，节点内文字 28px 白色：
  "Unity 场景" → "C# 导出器" → "JSON + .mesh" → "C++ 加载器" → "渲染管线" → "一帧画面"
"C# 导出器" 节点上方挂一个标签 24px #8b949e："Editor 扩展"；"渲染管线" 节点上方标签："顶点→裁剪→光栅化→着色"。
最右 "一帧画面" 节点下方再画一条 accent 虚线指向一个小框 "对照 Unity 截图" 24px。
[0s] 节点逐个从左淡入，间隔 0.3s，箭头跟着画出。
[2.4s] 一个 accent 色高亮光点从 "Unity 场景" 流到 "一帧画面"，循环两次。
底部一行小字 24px 颜色 #8b949e："本集讲左半段（导出）；EP2–EP5 讲右半段（渲染）。"

--- narration ---
整条链路就这么几步
左边是 Unity 里的场景
一个 C# 导出器把它写成两种文件
一份 JSON 描述场景
一批 mesh 文件装几何数据
C++ 这边的加载器把它们读进来
喂给渲染管线
最后输出一帧画面
再拿这一帧去和 Unity 的截图对照
这一集我们走左半段
导出
后面四集走右半段
渲染


>>> Unity 里的原始场景 #B05
@enter: fade-up
@exit: fade
@visual: video(../assets/orbit.mp4)

--- visual ---
（实际使用 ../assets/orbit.mp4：真实 Unity URP 渲染的工地场景，相机绕场景缓慢转一圈——工作台、工具、木方框架、墙、地面，是后面要在 CPU 上还原的"标准答案"。静帧版见 ../assets/unity_scene_game.png。）

--- narration ---
先看源头
这是场景在 Unity 里的样子
一个工作台
几样工具
后面是木方搭的框架和墙
用的是 URP 的标准光照
我们的目标
就是把这一画面
在 CPU 上还原出来


>>> 导出器拉出了什么 #B06
@enter: fade
@exit: fade
@visual: image(../assets/unity_exporter_menu.png)

--- visual ---
<!-- TODO Unity 截图：你的导出器菜单项/导出按钮所在位置。文件名 unity_exporter_menu.png 放入 assets/ -->
（实际使用 ../assets/unity_exporter_menu.png：Unity 编辑器里导出器扩展的菜单/按钮。）

--- narration ---
导出器是一个 Unity 的 **Editor 扩展**
点一下
它就遍历整个场景
把相机、光源、每个物体
连同材质、贴图、网格
还有环境光的球谐系数和雾参数
统统抓出来
写成磁盘上的文件


>>> 场景写成一段 JSON #B07
@enter: fade
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)。中央一个代码窗口，宽度占画布 88%，高度占画布 78%，圆角 14px，背景 #161b22，边框 1px solid #30363d，顶部有三个红黄绿圆点和文件名标签 "scene.json" 24px。
窗口内等宽字体 26px，语法高亮（键名 #79c0ff，字符串 #a5d6ff，数字 #d2a8ff，标点 #8b949e），展示精简后的 JSON：
  {
    "coordinateSystem": "unity-lh-yup-zforward-meters",
    "camera":    { "position": [...], "rotation": [...], "fov": 60, "near": 0.3, "far": 1000 },
    "mainLight": { "direction": [...], "color": [1, 0.90, 0.67], "intensity": 2 },
    "environment": { "ambientMode": "skybox", "sh": [ ...27 个系数... ] },
    "fog":       { "enabled": true, "mode": "exp2", "color": [...], "density": 0.05 },
    "objects": [ { "name": "Bench", "mesh": "meshes/bench_top.mesh", "material": {...} }, ... ]
  }
[0s] 代码窗口淡入。
[1s] 用 accent 色高亮框依次扫过 "camera" / "mainLight" / "environment" / "fog" / "objects" 五个字段，每个停 0.8s，右侧浮出一个 24px 小标签说明它对应的渲染阶段。

--- narration ---
JSON 长这样
相机的位置、旋转、视野角
平行光的方向、颜色、强度
环境光用 27 个 **球谐系数** 表示
雾的模式、颜色、浓度
还有一个 objects 数组
每个物体记着它的网格路径和材质
这一份纯文本
就是整个场景的全部描述


>>> 几何放进 mesh 文件 #B08
@enter: fade
@exit: fade
@visual: image(../assets/wire.png)

--- visual ---
（实际使用 ../assets/wire.png：工地场景的深度测试线框图，亮蓝色三角形网格，能看到工作台、框架、地面的三角形结构。）

--- narration ---
顶点数据不适合塞进 JSON
太大了
所以每个网格单独存成一个 **二进制 mesh 文件**
里面是顶点的位置、法线、切线
UV、以及三角形索引
加载器按字节读进来
就是你现在看到的这一堆三角形
JSON 负责"场景里有什么"
mesh 文件负责"每个东西长什么样"


>>> 较真：让 Unity 能当标准答案 #B09
@enter: fade-up
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)。顶部居中标题 "三处必须对齐" 字号 58px 粗体 #e6edf3，距顶 70px。
下方三张卡片横向等距排列，总宽占画布 92%，间距 40px，每张高 380px，圆角 16px，背景 #161b22，边框 1px solid #30363d，内边距 32px：
  卡片1 图标 🧭 (72px) accent 色，标题 "坐标系" 38px，下方三行 26px #8b949e："左手系" / "Y 朝上，Z 朝前" / "单位：米"
  卡片2 图标 📐 (72px) accent 色，标题 "矩阵约定" 38px，下方三行 26px："行/列主序" / "MVP 乘法顺序" / "投影 NDC 范围"
  卡片3 图标 💡 (72px) accent 色，标题 "光照单位" 38px，下方三行 26px："光强 / 颜色空间" / "线性 vs sRGB" / "BRDF 归一化"
卡片依次从下淡入上移，间隔 0.4s。
底部小字 24px #8b949e："任何一处不一致，对照 Unity 就失去意义。"

--- narration ---
要拿 Unity 当标准答案
有三件事必须和它**一模一样**
第一是坐标系
左手系、Y 朝上、Z 朝前、单位是米
第二是矩阵约定
行列主序、乘法顺序、投影的 NDC 范围
第三是光照单位
光强怎么算、在线性还是 sRGB 空间、BRDF 怎么归一化
这三处只要错一点
对照就没意义了
所以导出时把这些约定全部写死


>>> 第一次对照 #B10
@enter: fade
@exit: fade
@visual: image(../assets/compare_ours_ref.png)

--- visual ---
（实际使用 ../assets/compare_ours_ref.png：左右并排，左为 MyRender 的 CPU 输出，右为 Unity 参考图，同一工地场景、同一机位。）

--- narration ---
对齐之后
左边是我们 CPU 跑出来的
右边是 Unity 的截图
同一个机位
肉眼几乎看不出差别
我们还有一个数值指标
**PSNR** 大约 24 分贝
后面每加一个特性
都会用这个数字检验
到底更像了还是更糟了


>>> 本集小结 · 下集预告 #B11
@enter: fade-up
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)，垂直居中。
[0s] 顶部标题 "EP1 小结" 字号 60px 粗体 #e6edf3。
[0.5s] 下方三行要点，每行前带 accent 色 ✓，字号 34px，行距 28px，左对齐居中块（块宽占画布 72%）：
  ✓ 链路：Unity → 导出器 → JSON + mesh → 渲染
  ✓ JSON 记场景，mesh 记几何
  ✓ 坐标系 / 矩阵 / 光照单位三处对齐，Unity 才能当标准答案
[2.5s] 底部出现一张半透明的 ../assets/wire.png 缩略图（宽 40% 画布，圆角 12px，opacity 0.5）作为背景点缀，上方叠加一行 accent 色文字 40px："下一集 → 三角形怎么变成像素"。

--- narration ---
这一集记住三件事
链路是 Unity 到导出器，再到 JSON 和 mesh，最后渲染
JSON 记场景，mesh 记几何
坐标系、矩阵、光照单位三处对齐
Unity 才能当标准答案
数据已经进来了
下一集
我们就跟着一个三角形
看它怎么经过变换、裁剪、光栅化
变成屏幕上一个个像素
我们下集见
