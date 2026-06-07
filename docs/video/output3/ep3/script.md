>>> 开场 · 三种 Shader 对照 #B01
@enter: fade-up
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)。
顶部标题 "复刻 URP 的三种 Shader" 50px 粗体 #e6edf3，距顶 64px。
下方三张卡片横向等距，总宽占画布 92%，间距 36px，每张高 340px，圆角 16px，背景 #161b22，边框 1px solid #30363d，内边距 28px：
  卡片1 标题 "Lit" 40px accent，三行 26px #8b949e："基于物理 PBR" / "金属度 / 粗糙度" / "车身用它"
  卡片2 标题 "SimpleLit" 40px accent，三行："Blinn-Phong" / "便宜的高光" / "内饰用它"
  卡片3 标题 "UnLit" 40px accent，三行："不受光" / "纯底色" / "天空盒用它"
[0s] 三卡依次淡入上移，间隔 0.4s。

--- narration ---
这一集讲颜色
我们对照 URP
实现了三种着色器
名字也完全一样
Lit，基于物理的 PBR，车身用它
SimpleLit，便宜的高光模型，内饰用它
UnLit，不受光，纯底色，天空盒用它
这一集
我们主要拆 Lit
看一个像素的颜色
怎么一层层算出来
每一层
都拿截图说话


>>> 第一层：albedo 底色 #B02
@enter: fade
@exit: fade
@visual: image(../assets/albedo.png)

--- visual ---
（实际使用 ../assets/albedo.png：小车纯 albedo 底色，无光照。）

--- narration ---
最底层
是 albedo，物体本来的颜色
我把光照整段关掉
只输出底色
就是这张
完全没有明暗、没有高光
和 URP 里 surfaceData.albedo
是同一个东西
它来自一张基础贴图
乘上一个颜色
后面所有效果
都加在这层之上


>>> 底色定位靠 UV #B03
@enter: fade
@exit: fade
@visual: image(../assets/uv.png)

--- visual ---
（实际使用 ../assets/uv.png：小车 UV 可视化，红到绿渐变。）

--- narration ---
贴图怎么贴准
靠 UV 坐标
这张
我把每个像素的 UV 直接画成了颜色
红色是横向、绿色是纵向
和 Unity 里
你给模型展的那套 UV
是同一套
有了它
采样贴图这一步
两边结果就能对上


>>> 第二层：法线 · 几何法线 #B04
@enter: fade-up
@exit: fade
@visual: image(../assets/normal_geom.png)

--- visual ---
（实际使用 ../assets/normal_geom.png：几何法线可视化，平滑色块。）

--- narration ---
光照要算明暗
先看法线
这张
是模型本身的几何法线
按朝向染成颜色
和 URP 里顶点法线插值出来的
是一回事
但你看
现在表面还挺光滑
真实世界的粗糙细节
还没出来


>>> 法线贴图：和 Unity 一样的难点 #B05
@enter: fade
@exit: fade
@visual: image(../assets/normal_mapped.png)

--- visual ---
（实际使用 ../assets/normal_mapped.png：叠加法线贴图后，表面凹凸、锈斑、焊缝丰富。）

--- narration ---
加上法线贴图
细节一下就出来了
凹凸、锈迹、焊缝
模型没变
是贴图在每个像素
扰动了法线方向
这里有个和 Unity 死磕了很久的点
切线方向
要和 Unity 完全一致
我们最后用了一个叫 **mikktspace** 的算法
专门算切线
才让法线贴图的效果
和 URP 对得上


>>> 法线贴图原理 · 切线空间 #B06
@enter: fade-up
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)，左右布局。
左侧占 46%：一小块曲面，竖一根 accent "几何法线 N" 箭头；旁边一张紫蓝法线贴图小图，一个弯箭头把法线"掰歪"，标 "扰动"。
右侧占 46%：三根正交箭头组成 TBN 坐标架，标 "切线 T" "副切线 B" "法线 N" 30px；下方 26px #8b949e："贴图存切线空间方向，用 TBN 转回世界空间。"
[1.5s] 扰动出现。
[3s] TBN 坐标架出现。

--- narration ---
原理和 URP 完全一致
法线贴图里存的
不是世界里的绝对方向
而是相对表面的切线空间方向
每个点有一组坐标架
切线、副切线、法线
合起来叫 TBN
把贴图读出的方向用 TBN 转回世界
就得到被扰动后的法线
切线对不对
直接决定法线贴图像不像
这就是刚才说 mikktspace 那么关键的原因


>>> 第三层：PBR 光照 #B07
@enter: fade
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)。
顶部标题 "UniversalFragmentPBR" 等宽 40px accent，距顶 64px。
中央球体剖面：左上 accent 光箭头打到球面；反射出两组箭头——散开的标 "漫反射 Diffuse"，集中的标 "高光 Specular"。
球下方三个标签卡（高 88px，圆角 8px，背景 #161b22，文字 26px）："法线 N" / "视线 V" / "金属度 / 粗糙度"。
[1.5s] 漫反射散开。
[2.5s] 高光集中。
[3.5s] 三参数卡滑入。

--- narration ---
有了底色和法线
就能算光照了
我们照着 URP 的 BRDF
实现了同名的函数
一束光打到表面
分成漫反射和高光两部分
漫反射四面散开，是哑光
高光集中反射，是亮斑
散得多还是聚得紧
由金属度和粗糙度决定
这一整套
就是 URP 的 PBR
我们一行行复刻了一遍


>>> 验收 · Unity vs 我们 #B08
@enter: fade
@exit: fade
@visual: image(../assets/unity_ref_2.jpg)

--- visual ---
（实际使用 ../assets/unity_ref_2.jpg：Unity 原生渲染的小车，另一角度，标准答案。）

--- narration ---
算对了没有
还是对照 Unity
这一张
是 Unity 渲染的结果
车漆的高光、明暗的分布
就是标准答案
看清楚
下一张
是我们 CPU 算的同一个场景


>>> 我们复刻的结果 #B09
@enter: fade
@exit: fade
@visual: image(../assets/myrender_2.jpg)

--- visual ---
（实际使用 ../assets/myrender_2.jpg：MyRender 软渲染输出，另一角度，与 Unity 接近。）

--- narration ---
这是我们的结果
来回比对
车漆的金属高光
明暗的层次
法线带来的表面细节
基本和 Unity 对上了
从底色
到法线扰动
到 PBR 光照
每一层我们都对照着复刻
最后
两张图能贴到几乎重合
这就是复刻成功的标志


>>> 最后一题 · 太慢怎么办 #B10
@enter: fade-up
@exit: fade
@visual: image(../assets/threads.png)

--- visual ---
（实际使用 ../assets/threads.png：渲染画面叠加彩色水平条带，每条是一个线程负责的区域。）

--- narration ---
纯 CPU 复刻
最后要解决速度
我们把光栅化并行化
关键是分两趟
第一趟单线程
做顶点着色和裁剪
因为它会写全局变量
第二趟多线程
做光栅化
它只读全局变量、不写
再把屏幕按横条分给不同线程
互不重叠
这张图里
每一道颜色
就是一个线程的地盘


>>> 提速效果 · 实测 #B11
@enter: fade
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)，居中柱状对比。
两根纵向柱，底对齐，标签 30px：
  左柱 "单线程" 按 194 等比，顶标 "≈194 ms / 帧"，#ff7b72。
  右柱 "多线程 · 20 线程" 按 42 等比，顶标 "≈42 ms / 帧"，#58a6ff。
右上方圆角徽标（背景 #161b22，accent 边框）："≈ 4.6 倍" 56px accent。
[0s] 两柱自底向上长出。
[1.5s] 倍数徽标弹入。
底部 24px #8b949e："同机同帧实测；不会等于线程数倍，受串行段与调度限制。"

--- narration ---
效果有多大
这台机器上实测
同一帧
单线程要差不多一百九十多毫秒
开多线程
降到四十多毫秒
快了大约四点六倍
它不会等于线程数那么多倍
因为第一趟是串行的
还有线程调度的开销
但对一个纯 CPU 渲染器来说
这一步几乎是白捡的提速


>>> 系列收尾 #B12
@enter: fade
@exit: fade
@visual: image(../assets/final_front.png)

--- visual ---
（实际使用 ../assets/final_front.png：完整渲染结果，作为系列收尾。）

--- narration ---
三集到这里就结束了
我们对着 Unity
从整体结构
到顶点变换、裁剪、光栅化、深度
再到 albedo、法线、PBR 光照
一项一项地复刻
又一张一张地验收
最后能和 Unity 几乎对上
而这中间的每一个变量
在 GPU 上你看不见
在这里
全都能断点、能打印、能截图
这
就是把 URP 搬到 CPU 上复刻一遍
最大的收获
感谢观看


>>> 源码开放 · 自己来验收 #B13
@enter: fade-up
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)，垂直居中。
[0s] 顶部 GitHub 图标 72px 白色 + 下方仓库路径 "github.com/xslkim/MyRender" 48px accent 色 (#58a6ff) 等宽字体淡入。
[0.8s] 中部一张圆角卡片（背景 #161b22，边框 1px solid #30363d，圆角 16px，宽占画布 74%，内边距 28px）内三行 28px，每行左列 #8b949e、右列白色对照：
  "Unity 面板  →  JSON 场景"
  "HLSL Shader  →  gpu 层"
  "GPU 黑盒  →  可截图调试"
[1.8s] 底部 30px：左侧 #8b949e "作者：项思炼"，右侧 accent 色 "⭐ 来仓库复刻验收，顺手 Star"。

--- narration ---
整个项目是开源的
仓库在 GitHub 上
搜 xslkim 斜杠 MyRender
这三集里
每一处和 Unity 的对照
每一张调试截图背后的代码
全都在里面
你完全可以自己 clone 下来
对着 Unity 再验收一遍
看它到底对没对上
作者是项思炼
如果这次复刻对你有用
去仓库点个 star
我们下个项目见
