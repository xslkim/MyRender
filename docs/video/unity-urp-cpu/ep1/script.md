>>> 开场钩子 #B01
@enter: fade-up
@exit: fade
@visual: image(./assets/final.png)

--- visual ---
（实际使用 ./assets/final.png：MyRender 软渲染器输出的工地场景，960x540——工作台、电钻、木方框架、干墙、地面，远处带一层淡淡的雾。）

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
@visual: image(./assets/slide_series_map.png)

--- visual ---
（实际使用 ./assets/slide_series_map.png：系列两集卡片：上集(导出+几何，高亮) / 下集(光照阴影成像)。本图由 HTML 渲染生成，源文件 _tools/slide_series_map.html）

--- narration ---
我们分成两集
上集，导出和几何
先看 Unity 场景怎么变成数据
再看这些三角形怎么变成屏幕上的像素
下集，光照、阴影和成像
讲清楚一个像素的颜色到底怎么算出来
每一个知识点
都对着真实跑出来的画面讲
看完你会知道 URP 渲染一帧
数据到底怎么流


>>> 为什么要这么折腾 #B03
@enter: fade-up
@exit: fade
@visual: image(./assets/slide_gpu_vs_cpu.png)

--- visual ---
（实际使用 ./assets/slide_gpu_vs_cpu.png：GPU(封闭·锁) vs CPU(可调试·放大镜) 左右对比面板，✗/✓ 列表。本图由 HTML 渲染生成，源文件 _tools/slide_gpu_vs_cpu.html）

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
@visual: image(./assets/slide_dataflow.png)

--- visual ---
（实际使用 ./assets/slide_dataflow.png：端到端数据流：Unity 场景→C# 导出器→JSON+mesh→C++ 加载器→渲染管线→一帧画面。本图由 HTML 渲染生成，源文件 _tools/slide_dataflow.html）

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
这一集
我们从最左边的导出开始


>>> Unity 里的原始场景 #B05
@enter: fade-up
@exit: fade
@visual: video(./assets/orbit.mp4)

--- visual ---
（实际使用 ./assets/orbit.mp4：真实 Unity URP 渲染的工地场景，相机绕场景缓慢转一圈——工作台、工具、木方框架、墙、地面，是后面要在 CPU 上还原的"标准答案"。静帧版见 ./assets/unity_scene_game.png。）

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
@visual: image(./assets/unity_exporter_menu.png)

--- visual ---
（实际使用 ./assets/unity_exporter_menu.png：Unity 编辑器里 MyRender 菜单展开，可见 Capture Video Assets、Export Active Scene 等导出器扩展菜单项。）

--- narration ---
导出器是一个 Unity 的 **Editor 扩展**
点一下导出
它就遍历整个场景
把相机、光源、每个物体
连同材质、贴图、网格
还有环境光的球谐系数和雾参数
统统抓出来
写成磁盘上的文件


>>> 场景写成一段 JSON #B07
@enter: fade
@exit: fade
@visual: image(./assets/slide_scene_json.png)

--- visual ---
（实际使用 ./assets/slide_scene_json.png：scene.json 代码窗口，高亮相机/主光/环境光(球谐)/雾/物体五个字段。本图由 HTML 渲染生成，源文件 _tools/slide_scene_json.html）

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
@visual: image(./assets/wire.png)

--- visual ---
（实际使用 ./assets/wire.png：工地场景的深度测试线框图，亮蓝色三角形网格，能看到工作台、框架、地面的三角形结构。）

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
@visual: image(./assets/slide_align3.png)

--- visual ---
（实际使用 ./assets/slide_align3.png：三处必须对齐：坐标系 / 矩阵约定 / 光照单位，三张卡片。本图由 HTML 渲染生成，源文件 _tools/slide_align3.html）

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


>>> 数据进来了，开始走管线 #B10
@enter: fade
@exit: fade
@visual: image(./assets/slide_geo_pipeline.png)

--- visual ---
（实际使用 ./assets/slide_geo_pipeline.png：几何管线四步流程图：顶点变换→裁剪→光栅化→插值+深度。本图由 HTML 渲染生成，源文件 _tools/slide_geo_pipeline.html）

--- narration ---
数据已经从 Unity 进来了
接下来轮到几何管线登场
进来的是一大堆 **三角形**
我们盯住其中一个
看它怎么一步步变成屏幕上的像素
这条管线就四步
顶点变换、裁剪、光栅化
最后给每个像素填颜色、比远近
我们一步一步来


>>> 第一步：把三角形摆到屏幕上 #B11
@enter: fade-up
@exit: fade
@visual: image(./assets/slide_transform.png)

--- visual ---
（实际使用 ./assets/slide_transform.png：顶点变换四步：模型坐标→摆进世界(M)→相机视角(V)→压成屏幕(P)。本图由 HTML 渲染生成，源文件 _tools/slide_transform.html）

--- narration ---
每个顶点要换四次"身份"
先是模型自己的坐标
乘一个矩阵摆进世界
再换到相机的视角
最后压扁成屏幕上的二维坐标
这套流程
和你在 Unity 里见过的模型、视图、投影矩阵
完全是同一回事
我们只是把每一步的结果
都能拿出来看
摆好之后连成三角形
就是刚才那张线框图


>>> 第二步：裁剪掉看不见的 #B12
@enter: fade
@exit: fade
@visual: image(./assets/slide_clip.png)

--- visual ---
（实际使用 ./assets/slide_clip.png：裁剪示意：三角形跨屏幕边界被裁，蓝色=保留在屏幕内的部分。本图由 HTML 渲染生成，源文件 _tools/slide_clip.html）

--- narration ---
不是所有三角形都该画
有的伸到了画面外
有的干脆在相机背后
裁剪就是沿着画面的边界
把伸出去的部分切掉
切出来的缺口
再补成几个小三角形
完全看不见的
直接整块丢掉
这样后面就不会白算一堆没用的像素


>>> 第三步：把三角形铺成像素 #B13
@enter: fade
@exit: fade
@visual: image(./assets/slide_raster.png)

--- visual ---
（实际使用 ./assets/slide_raster.png：光栅化：三角形覆盖像素网格，包围盒内逐像素判定，里面填、外面跳过。本图由 HTML 渲染生成，源文件 _tools/slide_raster.html）

--- narration ---
光栅化就是把三角形"涂"成像素
做法很朴素
先框出三角形的包围盒
然后挨个检查盒子里的像素
在三角形里面的
就填上
在外面的
就跳过
一个连续的三角形
就这样变成了一格格离散的像素


>>> 第四步之一：颜色怎么过渡 #B14
@enter: fade-up
@exit: fade
@visual: image(./assets/uv.png)

--- visual ---
（实际使用 ./assets/uv.png：UV 坐标可视化，地面和物体表面呈红绿渐变，地砖的格子线笔直、不扭曲。）

--- narration ---
三角形只有三个顶点有数据
中间的像素得"插"出来
比如这张图
显示的是每个像素的贴图坐标
难点在于
近处的东西看起来大、远处小
如果只是简单平均
地砖的直线会扭曲变形
所以要按远近做 **透视校正**
你看这些格子线
笔直
没有歪
说明校正对了


>>> 第四步之二：谁挡住了谁 #B15
@enter: fade
@exit: fade
@visual: image(./assets/slide_depth.png)

--- visual ---
（实际使用 ./assets/slide_depth.png：深度缓冲：近的卡片挡住远的卡片，每像素只留最近。本图由 HTML 渲染生成，源文件 _tools/slide_depth.html）

--- narration ---
同一个像素
可能被好几个三角形盖到
到底显示哪个
靠一块 **深度缓冲**
它给每个像素记一个到相机的距离
新三角形要写进来
先比一下距离
比已有的近
才覆盖
比它远
就挡在后面、不画
这样远处的东西
就被近处正确地遮住了


>>> 顺带一提：它是多线程跑的 #B16
@enter: fade
@exit: fade
@visual: image(./assets/threads.png)

--- visual ---
（实际使用 ./assets/threads.png：多线程分条可视化，画面被按横向条带分给不同线程，每条带有不同的淡色调。）

--- narration ---
这些计算量不小
所以光栅化是多线程的
画面按横向条带切开
每个线程负责几条
你看这张图的淡色分条
就是不同线程各自的地盘
CPU 慢
但靠多核
也能把一帧压到可接受的时间


>>> 上集小结 · 下集预告 #B17
@enter: fade-up
@exit: fade
@visual: image(./assets/slide_p1_recap.png)

--- visual ---
（实际使用 ./assets/slide_p1_recap.png：上集小结五要点 + "下集→像素的颜色怎么算" 预告。本图由 HTML 渲染生成，源文件 _tools/slide_p1_recap.html）

--- narration ---
上集到这里
我们从 Unity 导出了场景
也把三角形变成了屏幕上有形状、有遮挡关系的像素
记住两件事
链路是 Unity 到 JSON 和 mesh，再到渲染
三处对齐让 Unity 能当标准答案
但这些像素现在还是"素颜"的
没有光、没有阴影、没有雾
下集
我们就给每个像素上色
讲清楚光照、阴影、大气
最后拿出数字
跟 Unity 认真对一次账
我们下集见
