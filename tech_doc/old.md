软渲染引擎 复刻 Unity URP Shader 效果
项思炼
项思炼
3 人赞同了该文章
在学习 Unity URP 渲染的时候， 对于shader 里面是怎么计算和运行的总是隔着一层沙，似懂非懂。 在URP 里面的Shader 工程源码也是体积非常庞大，IDE也不支持函数跳转。要看懂和掌握效率低下也非常难。特别是当想调试和查看一个变量的值的时候就非常痛苦，没办法设断点，也没办法打log。 于是萌生了写一个纯软的渲染引擎来复刻Unity的渲染效果，主要用于学习和调试。渲染慢一点也没关系。

现在初步实现了Lit、SimpleLit 和 UnLit 三个Shader的渲染效果。
Unity 原生的渲染效果

车身采用PBR材质，其他采用SimpleLit 材质

没有环境光，只靠一个主光源，这个换了一张法线贴图
MyRender 软渲染的效果

光源方向、摄像头方向和Unity保持一致

透明渲染的排序和混合的效果还需要调整一下
MyRender 软渲染引擎的架构
采用json 文件来组织场景
物体位姿、材质
摄像头参数
光源参数
{
	"camera": {
		"position": [-3.618466,2.595639,5.103661],
		"rotation": [11.437,136.582,0],
		"fov": 60,
		"near": 0.3,
		"far": 1000
	},
	"light": {
		"position": [0,0,0],
		"rotation": [-239,-93.7,0],
		"color": [1,1,1,1],
		"intensity": 1,
		"indirect_mul": 1
	},
	"game_objects": [
		{
			"position": [0,0,0],
			"rotation": [-90,0,0],
			"scale": [0.01,0.01,0.01],
			"mesh": "body.obj",
			"material": "body_material2.json"
		}
         ]
}
所有参数保持和Unity UI面板一致，左手坐标系

旋转采用的是欧拉角度，旋转顺序也保持和Unity一致
Shader 代码实现 和Unity 一致，只不过是用c++改写的。

MyRender 软渲染引擎碰到的问题
网上已经有非常多的软渲染教程和github工程源码了，我实现的时候也参考了各种大佬的教程和源码。这里非常感谢。像齐次空间剪裁，光照模型等技术细节，有这些资料认真学习是可以啃下来的。

切线空间的法线贴图一致困扰我很长时间
现有的源码中，要不就是没有采用切线空间，要不就是模型自带切线坐标。 我在计算切线方向的时候一直没办法和Unity做到完全一致，最后发现是要采用一种 mikktspace 算法。幸运的是还有源码，就果断集成进来了。

MikkTSpace.com
www.mikktspace.com/
修改URP源码
在参考urp 渲染管线的时候，有时候需要手动添加一下debug代码，不过Unity 会自动还原。 我被迫花了很大的精力把urp的源码移植到Assets目录下面了。 但最后发现只要把 “Library/PackageCache”里面的源码直接剪切到 Packages 目录里面就可以了。

下一步需要继续做的事情
支持多线程渲染
目前单线程渲染在一万多面的时候fps只有6帧，利用多线程多核渲染应该可以提高一个数量级。

支持直接从Unity里面加载场景，而不是现在的手撸json
支持导入FBX
支持烘焙贴图
支持阴影
支持反射探针
当然最终的目的是能渲染，任意Unity URP 搭建出来的场景。
源代码
https://github.com/xslkim/MyRender
github.com/xslkim/MyRender
居于SDL做跨平台显示，所有代码都用hpp编写。

目前只测试过windows平台，在build目录下面，采用vs2022 工程可以直接编译运行。

也可以直接下载编译好的包运行。