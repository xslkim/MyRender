# -*- coding: utf-8 -*-
# 生成 EP1 教学幻灯片 HTML（共享 style.css），随后用 Chrome 渲成 PNG。
HEAD='<!doctype html><html><head><meta charset="utf-8"><link rel="stylesheet" href="style.css"></head><body>'
TAIL='</body></html>'
S={}

S['slide_series_map']='''<div class="slide">
<h1 class="title">从 Unity 导出到 CPU 软渲染</h1>
<div class="subtitle">把 URP 一比一搬到 CPU，看懂渲染一帧的每一步</div>
<div class="bar" style="width:820px"></div>
<div class="row" style="margin-top:64px">
  <div class="card hl" style="width:640px"><div class="tag">上集</div><div class="ct">导出与几何管线</div>
    <div class="cd">Unity 场景 → JSON + mesh<br>顶点变换 · 裁剪<br>光栅化 · 透视校正 · 深度</div></div>
  <div class="card" style="width:640px"><div class="tag">下集</div><div class="ct">光照阴影与成像</div>
    <div class="cd">材质 · PBR · 环境光<br>阴影 · 雾 · 色调<br>与 Unity 对账</div></div>
</div></div>'''

S['slide_gpu_vs_cpu']='''<div class="slide">
<div class="row" style="gap:80px">
  <div class="card" style="width:600px;height:600px;background:#14171c">
    <div class="icon" style="color:#6e7681">&#128274;</div>
    <div style="font-size:64px;font-weight:800;color:#8b949e;margin-top:14px">GPU</div>
    <div class="small" style="margin-top:6px">渲染跑在显卡里</div>
    <div class="bullets" style="margin-top:46px">
      <div class="li"><span class="cx">&#10007;</span><span class="muted">不能设断点</span></div>
      <div class="li"><span class="cx">&#10007;</span><span class="muted">不能打印变量</span></div>
      <div class="li"><span class="cx">&#10007;</span><span class="muted">出问题只能猜</span></div></div>
  </div>
  <div class="card hl" style="width:600px;height:600px">
    <div class="icon accent">&#128269;</div>
    <div style="font-size:64px;font-weight:800;margin-top:14px">CPU</div>
    <div class="small" style="margin-top:6px">管线搬到 C++ 一行行重写</div>
    <div class="bullets" style="margin-top:46px">
      <div class="li"><span class="ck">&#10003;</span><span>随便设断点</span></div>
      <div class="li"><span class="ck">&#10003;</span><span>随便打印</span></div>
      <div class="li"><span class="ck">&#10003;</span><span>直接截图看见</span></div></div>
  </div>
</div>
<div class="foot">慢一点没关系——每个中间量都看得见，还能拿 Unity 当标准答案对照。</div></div>'''

def node(t,d): return '<div class="node"><div class="nt">'+t+'</div><div class="nd">'+d+'</div></div>'
A='<div class="arrow">&#8594;</div>'
S['slide_dataflow']='<div class="slide"><div class="sectionhead">端到端的数据流</div><div class="flow">'+A.join([node('Unity 场景','&nbsp;'),node('C# 导出器','Editor 扩展'),node('JSON + .mesh','&nbsp;'),node('C++ 加载器','&nbsp;'),node('渲染管线','顶点→光栅化→着色'),node('一帧画面','&nbsp;')])+'</div><div class="foot">上集走链路的左半段（导出）和中段（几何）；下集讲右半段（着色）。最后拿一帧去和 Unity 截图对照。</div></div>'

S['slide_scene_json']='''<div class="slide">
<div class="codewin">
  <div class="codebar"><span class="dot" style="background:#ff5f56"></span><span class="dot" style="background:#ffbd2e"></span><span class="dot" style="background:#27c93f"></span><span class="fn">scene.json</span></div>
<pre class="code"><span class="p">{</span>
  <span class="k">"coordinateSystem"</span><span class="p">:</span> <span class="s">"unity-lh-yup-zforward-meters"</span><span class="p">,</span>
  <span class="k">"camera"</span><span class="p">:</span>    <span class="p">{</span> <span class="k">"position"</span>:<span class="n">[2.32,1.2,3.3]</span>, <span class="k">"fovVertical"</span>:<span class="n">60</span>, <span class="k">"far"</span>:<span class="n">1000</span> <span class="p">},</span>
  <span class="k">"mainLight"</span><span class="p">:</span> <span class="p">{</span> <span class="k">"direction"</span>:<span class="p">[...]</span>, <span class="k">"color"</span>:<span class="n">[1,0.90,0.67]</span>, <span class="k">"intensity"</span>:<span class="n">2</span> <span class="p">},</span>
  <span class="k">"environment"</span><span class="p">:</span> <span class="p">{</span> <span class="k">"ambientMode"</span>:<span class="s">"skybox"</span>, <span class="k">"sh"</span>:<span class="p">[</span> ...27 个系数... <span class="p">]</span> <span class="p">},</span>
  <span class="k">"fog"</span><span class="p">:</span>       <span class="p">{</span> <span class="k">"enabled"</span>:<span class="k">true</span>, <span class="k">"mode"</span>:<span class="s">"exp2"</span>, <span class="k">"density"</span>:<span class="n">0.05</span> <span class="p">},</span>
  <span class="k">"objects"</span><span class="p">:</span> <span class="p">[</span> <span class="p">{</span> <span class="k">"name"</span>:<span class="s">"Drywall Panel"</span>, <span class="k">"mesh"</span>:<span class="s">"meshes/..."</span> <span class="p">},</span> ... <span class="p">]</span>
<span class="p">}</span></pre>
</div>
<div class="foot">相机 · 主光 · 环境光(球谐) · 雾 · 物体——一份纯文本描述整个场景。</div></div>'''

def card3(icon,t,lines):
    return '<div class="card" style="width:480px;height:430px"><div class="icon accent">'+icon+'</div><div class="ct" style="margin-top:18px">'+t+'</div><div class="cd">'+'<br>'.join(lines)+'</div></div>'
S['slide_align3']='<div class="slide"><div class="sectionhead" style="font-size:54px;color:var(--fg)">三处必须对齐</div><div class="row">'+card3('&#129517;','坐标系',['左手系','Y 朝上，Z 朝前','单位：米'])+card3('&#128208;','矩阵约定',['行 / 列主序','MVP 乘法顺序','投影 NDC 范围'])+card3('&#128161;','光照单位',['光强 / 颜色空间','线性 vs sRGB','BRDF 归一化'])+'</div><div class="foot">任何一处不一致，对照 Unity 就失去意义。</div></div>'

S['slide_geo_pipeline']='<div class="slide"><div class="sectionhead">几何管线</div><div class="flow">'+A.join([node('顶点变换','摆到屏幕坐标'),node('裁剪','切掉看不见的'),node('光栅化','铺成像素'),node('插值 + 深度','填颜色·比远近')])+'</div><div class="foot">进来的是一大堆三角形——盯住一个，看它怎么一步步变成像素。</div></div>'

def step(n,t,sub):
    return '<div class="card" style="width:400px;height:360px;align-items:center;text-align:center"><div class="tag" style="font-size:40px">'+n+'</div><div class="ct" style="font-size:30px;margin-top:18px">'+t+'</div><div class="cd" style="text-align:center">'+sub+'</div></div>'
S['slide_transform']='<div class="slide"><div class="sectionhead">第一步：顶点变换 —— 换四次"身份"</div><div class="flow">'+A.join([step('&#9312;','模型坐标','物体自己的局部空间'),step('&#9313;','摆进世界','乘模型矩阵 M'),step('&#9314;','换相机视角','乘视图矩阵 V'),step('&#9315;','压成屏幕','乘投影矩阵 P')])+'</div><div class="foot">和 Unity 里的 M / V / P 矩阵一一对应——只是我们看得见每一步结果。</div></div>'

S['slide_clip']='''<div class="slide">
<div class="sectionhead">第二步：裁剪掉看不见的</div>
<div style="position:relative;width:1100px;height:560px;border:3px solid var(--accent);border-radius:8px;background:#0f141b">
  <div style="position:absolute;left:30px;top:20px;font-size:28px;color:var(--muted)">屏幕边界</div>
  <svg width="1100" height="560" style="position:absolute;left:0;top:0">
    <polygon points="760,60 1340,280 680,500" fill="rgba(110,118,129,.22)" stroke="#6e7681" stroke-width="3"/>
    <polygon points="760,60 1080,182 1080,392 680,500" fill="rgba(88,166,255,.32)" stroke="#58a6ff" stroke-width="4"/>
    <line x1="1080" y1="90" x2="1080" y2="470" stroke="#ff7b72" stroke-width="3" stroke-dasharray="10 8"/>
    <text x="700" y="540" fill="#58a6ff" font-size="26">蓝色 = 裁剪后留在屏幕内的部分</text>
  </svg>
</div>
<div class="foot">沿 7 个边界逐个裁——上下左右 + 远近 + 相机背后；切出的缺口补成小三角形，全看不见的整块丢掉。</div></div>'''

def raster_svg():
    tri=[(220,60),(560,360),(120,360)]
    def inside(px,py):
        def sign(a,b,c):return (px-c[0])*(b[1]-c[1])-(b[0]-c[0])*(py-c[1])
        d1=sign(tri[0],tri[1],tri[2]);d2=sign(tri[1],tri[2],tri[0]);d3=sign(tri[2],tri[0],tri[1])
        hasneg=(d1<0)or(d2<0)or(d3<0);haspos=(d1>0)or(d2>0)or(d3>0);return not(hasneg and haspos)
    cells=''
    for gy in range(0,400,40):
        for gx in range(80,600,40):
            f='rgba(88,166,255,.45)' if inside(gx+20,gy+20) else 'none'
            cells+='<rect x="%d" y="%d" width="40" height="40" fill="%s" stroke="#30363d" stroke-width="1"/>'%(gx,gy,f)
    cells+='<polygon points="%d,%d %d,%d %d,%d" fill="none" stroke="#58a6ff" stroke-width="4"/>'%(tri[0][0],tri[0][1],tri[1][0],tri[1][1],tri[2][0],tri[2][1])
    cells+='<rect x="120" y="60" width="440" height="300" fill="none" stroke="#58a6ff" stroke-width="2" stroke-dasharray="8 6"/>'
    return cells
S['slide_raster']='<div class="slide"><div class="sectionhead">第三步：把三角形铺成像素</div><svg width="680" height="420" style="background:#0f141b;border-radius:8px">'+raster_svg()+'</svg><div class="foot">先框出包围盒（虚线），再逐个检查盒内像素：中心在三角形里就填，在外面就跳过。</div></div>'

S['slide_depth']='''<div class="slide">
<div class="sectionhead">第四步之二：谁挡住了谁（深度缓冲）</div>
<div style="position:relative;width:900px;height:480px">
  <div style="position:absolute;left:110px;top:160px;width:430px;height:280px;background:rgba(110,118,129,.32);border:3px solid #6e7681;border-radius:14px;display:flex;align-items:center;justify-content:center;font-size:30px;color:#8b949e">远（距离大）</div>
  <div style="position:absolute;left:360px;top:50px;width:430px;height:280px;background:rgba(88,166,255,.45);border:3px solid #58a6ff;border-radius:14px;display:flex;align-items:center;justify-content:center;font-size:30px">近（距离小）&#8594; 保留</div>
</div>
<div class="foot">每个像素记一个到相机的距离；新三角形比已有的近才覆盖，远的就被挡住不画。</div></div>'''

S['slide_p1_recap']='''<div class="slide">
<div class="title" style="font-size:60px">上集小结</div>
<div class="bullets" style="margin-top:50px;width:1200px">
  <div class="li"><span class="ck">&#10003;</span><span>链路：Unity → 导出器 → JSON + mesh → 渲染</span></div>
  <div class="li"><span class="ck">&#10003;</span><span>坐标系 / 矩阵 / 光照单位三处对齐，Unity 才能当标准答案</span></div>
  <div class="li"><span class="ck">&#10003;</span><span>顶点变换：四步摆到屏幕坐标</span></div>
  <div class="li"><span class="ck">&#10003;</span><span>裁剪：切掉画面外和背后的</span></div>
  <div class="li"><span class="ck">&#10003;</span><span>光栅化 + 透视校正插值 + 深度遮挡</span></div>
</div>
<div style="margin-top:60px;font-size:40px" class="accent">下集 → 像素的颜色怎么算</div></div>'''

for name,body in S.items():
    open(name+'.html','w',encoding='utf-8').write(HEAD+body+TAIL)
print('wrote',len(S),'files')
