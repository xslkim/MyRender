>>> 开场 · 两张图能不能对上 #B01
@enter: fade-up
@exit: fade
@visual: video(../assets/orbit.mp4)

--- visual ---
（实际使用 ../assets/orbit.mp4：相机环绕小车一圈，CPU 软渲染画面。）

--- narration ---
这辆车
是用 C++ 在 CPU 上一行行算出来的
没有用任何显卡
而我们的目标只有一个
让它的画面
能和 **Unity URP** 渲染的结果对上
这三集
我们就对着 Unity
一步步把这条管线复刻出来
并且每一步
都拿真实截图来验收


>>> 这一版怎么讲 · 三段式 #B02
@enter: fade
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)。
[0s] 顶部居中出现系列标题 "CPU软渲染 复刻Unity URP"，字号 50px 粗体 #e6edf3，距顶 56px；下方副标题 "对照图解版" 30px #8b949e。
中央三张横向卡片（位于标题下方），等距，间距 40px，总宽占画布 90%，每张高 300px，圆角 16px，背景 #161b22，边框 1px solid #30363d，内边距 30px：
  卡片1 顶部大数字 "1" 56px accent，标题 "Unity 怎么做" 36px，描述 "拿官方结果当标准答案" 26px #8b949e
  卡片2 顶部大数字 "2" 56px accent，标题 "我们怎么复刻" 36px，描述 "图解原理" 26px
  卡片3 顶部大数字 "3" 56px accent，标题 "证明对上了" 36px，描述 "MyRender 调试截图" 26px
[0.6s] 三卡依次滑入，间隔 0.4s。
[2.5s] 三卡之间出现 accent 箭头 1→2→3，形成一个循环回路。

--- narration ---
这一版每讲一个环节
都走同一套流程
第一步，看 Unity 是怎么做的
把它当标准答案
第二步，讲我们怎么复刻这个环节
把原理画清楚
第三步，亮出我们渲染器的真实截图
证明结果真的对上了
这一集
我们先从整体结构开始对


>>> URP 的管线 vs 我们的管线 #B03
@enter: fade-up
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)，上下两行流程对照。
上行标题 "Unity URP" 30px accent，右侧六个圆角节点用箭头连接（节点高 100px，圆角 10px，背景 #161b22，文字 26px）："顶点数据"→"顶点着色"→"裁剪"→"光栅化"→"片元着色"→"帧缓冲"。
下行标题 "MyRender（CPU）" 30px #58a6ff，节点与上行一一对齐，文字相同，但每个节点下方加一行 22px #8b949e 的对应函数名："mesh"→"vertex_shader"→"ClipWithPlane"→"RasterizeTri"→"fragment_shader"→"ColorBuffer"。
[0s] 上行节点逐个淡入。
[2s] 下行节点逐个对齐淡入，并用竖直虚线把上下对应节点连起来。

--- narration ---
先看最关键的对照
URP 的渲染
是这样一条流水线
顶点数据、顶点着色、裁剪、光栅化、片元着色、帧缓冲
我们的渲染器
一个阶段都不少
而且**一一对应**
URP 的顶点着色器
对应我们的 vertex_shader 函数
它的裁剪
对应我们的 ClipWithPlane
它的片元着色器
对应我们的 fragment_shader
管线的骨架
我们是照着 URP 搭的


>>> 代码也故意写得像 HLSL #B04
@enter: fade
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)，左右对照。
左侧标题 "URP HLSL" 28px accent，下方一小段着色器伪代码（等宽 24px，语法高亮）：
  half4 frag(Varyings i){
    half4 c = SampleAlbedoAlpha(i.uv, _BaseMap, ...);
    ...
  }
右侧标题 "MyRender C++" 28px #58a6ff，下方对应的 C++ 伪代码（等宽 24px，结构几乎一致）：
  half4 LitFragmentShader(Varyings i){
    half4 c = SampleAlbedoAlpha(i.uv, _BaseMap, ...);
    ...
  }
中间用 accent 双向箭头连接，底部 24px #8b949e："变量名、函数名、结构体，尽量一一对应。"

--- narration ---
不光结构像
我们连代码都故意写得像 HLSL
变量名、函数名、结构体
尽量和 URP 一模一样
比如取底色这一句
两边几乎长得一样
为什么这么较真
因为这样
你在 Unity 里读过的 Shader
搬到这里
几乎不用重新学
而且能设断点、能打印
URP 里看不见的中间值
在这里全都看得见


>>> 工程的三层结构 #B05
@enter: fade-up
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)。
顶部标题 "三层结构" 56px 粗体 #e6edf3，距顶 70px。
下方三张卡片横向等距，总宽占画布 92%，间距 40px，每张高 360px，圆角 16px，背景 #161b22，边框 1px solid #30363d，内边距 30px：
  卡片1 图标 📦 accent，标题 "core / 场景" 36px，三行 26px #8b949e："JSON 场景" / "相机·光源·物体" / "对应 Unity 的 Scene"
  卡片2 图标 ⚙️ accent，标题 "Render / 管线" 36px，三行："变换·裁剪·光栅化" / "深度·混合" / "对应 URP 的 SRP"
  卡片3 图标 💡 accent，标题 "gpu / 着色" 36px，三行："Lit·SimpleLit·UnLit" / "BRDF·法线·GI" / "对应 URP 的 ShaderLibrary"
[0s] 三卡依次淡入上移，间隔 0.4s。

--- narration ---
工程分三层
也都能在 Unity 里找到对应
core 层，管场景
对应 Unity 的 Scene，相机、光源、物体
Render 层，管管线
对应 URP 那条可编程渲染管线
gpu 层，管着色
对应 URP 的 Shader 函数库
我们实现了三种着色器
Lit、SimpleLit、UnLit
名字和 Unity 完全一致


>>> 对照一：场景参数 = Inspector #B06
@enter: fade
@exit: fade
@visual: image(../assets/unity_inspector.jpg)

--- visual ---
（实际使用 ../assets/unity_inspector.jpg：Unity 相机的 Transform / 参数面板与对应画面。）

--- narration ---
来看第一个对照
场景
我们的场景是一段 JSON
但里面每一个数字
都对着 Unity 的 Inspector 面板
相机的位置、旋转、视野角
near、far
甚至坐标系和旋转顺序
全都和 Unity 对齐
左手坐标系
欧拉角
旋转顺序也一样
只有先对齐这些
后面比画面才有意义


>>> 对照二：最终画面 #B07
@enter: fade
@exit: fade
@visual: image(../assets/unity_ref_1.jpg)

--- visual ---
（实际使用 ../assets/unity_ref_1.jpg：Unity 原生渲染的小车，标准答案。）

--- narration ---
这是 Unity 渲染出来的车
是这一整个系列
要追的标准答案
车漆的金属反光
车窗的半透明
地面的法线细节
记住这张
接下来
是我们 CPU 算出来的版本


>>> 我们复刻的画面 #B08
@enter: fade
@exit: fade
@visual: image(../assets/myrender_1.jpg)

--- visual ---
（实际使用 ../assets/myrender_1.jpg：MyRender 软渲染输出，与 Unity 高度接近。）

--- narration ---
这是我们的结果
同样的相机、同样的光
你可以来回比对刚才那张 Unity
车漆、车窗、地面
基本对上了
当然透明物体的排序、混合
还有些细微差别
但作为一个纯 CPU 渲染器
能复刻到这个程度
已经说明
管线的每一步
方向都是对的


>>> 一帧的五个步骤 #B09
@enter: fade-up
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)。
左侧竖排时间线，占左 55%，五个圆角标签（背景 #161b22，accent 左边框 4px，高 88px，圆角 8px，文字 30px 白色），竖直 accent 虚线连接：
  "1  清空颜色 / 深度缓冲"
  "2  上传相机·光源到全局变量"
  "3  逐物体：模型矩阵 → Draw"
  "4  逐三角形：着色 → 裁剪 → 光栅化"
  "5  颜色缓冲 → sRGB → 屏幕"
右侧占 40%：标注 "对应 URP 的一次 Render 调用" 28px accent，下方一个简化的循环框图。
[0s] 右侧出现。
[1s] 左侧五个节点自上而下依次高亮，间隔 0.6s。

--- narration ---
最后建立一个全局地图
我们的渲染器每画一帧
就做五件事
清空缓冲
上传相机和光源
逐个物体算矩阵、调用 Draw
逐个三角形做着色、裁剪、光栅化
最后转 sRGB 上屏
这五步
其实就是 URP 里一次渲染调用
做的事情
只不过我们把它摊开
变成了你能逐行调试的 C++


>>> 本集小结 · 下集预告 #B10
@enter: fade
@exit: fade
@visual: animation

--- visual ---
全屏深色背景 (#0d1117)，垂直居中。
[0s] 顶部标题 "EP1 小结" 60px 粗体 #e6edf3。
[0.5s] 三行要点，每行前带 accent ✓，字号 34px，行距 28px，块宽 72%：
  ✓ 管线阶段与 URP 一一对应
  ✓ 三层结构对应 Scene / SRP / ShaderLibrary
  ✓ 场景参数严格对齐 Inspector
[2.5s] 底部 accent 文字 40px："下一集 → 逐项对照：顶点到像素"。

--- narration ---
这一集我们对完了整体结构
管线阶段和 URP 一一对应
三层结构也能在 Unity 里找到对应
场景参数严格对齐 Inspector
下一集
我们进入管线内部
把坐标变换、裁剪、光栅化、深度
一项一项地
对着原理图讲清楚
再用调试截图
证明它们真的算对了
下集见
