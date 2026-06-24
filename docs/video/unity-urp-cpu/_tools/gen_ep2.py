# -*- coding: utf-8 -*-
HEAD='<!doctype html><html><head><meta charset="utf-8"><link rel="stylesheet" href="style.css"></head><body>'
TAIL='</body></html>'
S={}

def panel(img,label,cls='',w=820):
    return ('<div class="panel '+cls+'" style="width:%dpx"><div class="pl">'%w)+label+'</div><img src="'+img+'"></div>'

# 渲染器真实截图现位于 ep2/assets/（从 _tools/ 看进去是 ../ep2/assets/）
IMG='../ep2/assets/'
S['slide_materials']='<div class="slide"><div class="sectionhead">材质的两张关键贴图</div>'\
 +'<div class="row" style="align-items:center">'+panel(IMG+'albedo.png','基础色贴图 (albedo)','',760)\
 +'<div class="plus">+</div>'+panel(IMG+'normal_mapped.png','法线贴图 (normal)','',760)+'</div>'\
 +'<div class="foot">基础色 = 这块表面本来什么颜色；法线 = 表面凹凸的朝向细节。两者合起来才有立体感。</div></div>'

S['slide_bug']='<div class="slide"><div class="sectionhead" style="font-size:50px">一个真实的 bug：环境光暗了约 π 倍</div>'\
 +'<div class="row" style="align-items:flex-end">'\
 +panel(IMG+'lights_off.png','① 灭灯：纯黑','',560)\
 +'<div class="panel" style="width:380px"><div class="pl red">② 有 bug：墙发黑</div><img src="'+IMG+'cyan_bug_crop.png" style="height:380px;width:auto"></div>'\
 +'<div class="plus accent">&#8594;</div>'\
 +'<div class="panel hl" style="width:380px"><div class="pl accent">③ 修复后：恢复冷青色</div><img src="'+IMG+'cyan_fixed_crop.png" style="height:380px;width:auto"></div>'\
 +'</div><div class="foot">同一个归一化亮度系数（1/π），被直接光和环境光共用；直接光有补偿、环境光没有 → 平白暗了约 π 倍（≈3 倍）。</div></div>'

S['slide_shadow_diff']='<div class="slide"><div class="sectionhead">找不同：有没有影子</div>'\
 +'<div class="row">'+panel(IMG+'shadow_off.png','没有阴影','',820)+panel(IMG+'final.png','有阴影','hl',820)+'</div>'\
 +'<div class="foot">没有影子，物体像浮在地上；有了影子，工作台和工具才真正"落"到地面上。</div></div>'

S['slide_shadow_idea']='''<div class="slide">
<div class="sectionhead">阴影的核心思路：两遍法</div>
<div class="row">
  <div class="card" style="width:720px;height:500px">
    <div class="ct" style="color:var(--accent)">第一遍 · 站到太阳的位置</div>
    <svg width="640" height="340" style="margin-top:18px">
      <circle cx="90" cy="80" r="40" fill="#ffbd2e"/><text x="60" y="150" fill="#8b949e" font-size="24">太阳视角</text>
      <line x1="130" y1="100" x2="320" y2="220" stroke="#58a6ff" stroke-width="3"/>
      <line x1="130" y1="120" x2="320" y2="280" stroke="#58a6ff" stroke-width="3"/>
      <rect x="320" y="200" width="150" height="120" fill="rgba(110,118,129,.4)" stroke="#6e7681" stroke-width="3"/>
      <text x="330" y="190" fill="#e6edf3" font-size="24">记下每个方向</text>
      <text x="330" y="170" fill="#e6edf3" font-size="24">最近的东西多远</text>
    </svg>
  </div>
  <div class="card" style="width:720px;height:500px">
    <div class="ct" style="color:var(--accent)">第二遍 · 着色时反问</div>
    <svg width="640" height="340" style="margin-top:18px">
      <circle cx="540" cy="60" r="34" fill="#ffbd2e"/>
      <line x1="120" y1="300" x2="520" y2="80" stroke="#58a6ff" stroke-width="3" stroke-dasharray="8 6"/>
      <rect x="300" y="150" width="120" height="100" fill="rgba(110,118,129,.5)" stroke="#6e7681" stroke-width="3"/>
      <circle cx="120" cy="300" r="12" fill="#58a6ff"/>
      <text x="60" y="330" fill="#e6edf3" font-size="24">这个像素</text>
      <text x="150" y="200" fill="#ff7b72" font-size="24">中间被挡 → 在影子里</text>
    </svg>
  </div>
</div>
<div class="foot">第一遍从太阳眼里记一张"远近地图"；第二遍上色时问：我和太阳之间有没有东西挡着。</div></div>'''

S['slide_shadow_fit']='''<div class="slide">
<div class="sectionhead">把"太阳的视野"框住整个场景</div>
<svg width="1000" height="520">
  <circle cx="120" cy="90" r="40" fill="#ffbd2e"/><text x="80" y="160" fill="#8b949e" font-size="24">平行光</text>
  <g stroke="#ffbd2e" stroke-width="2" opacity=".5">
    <line x1="150" y1="120" x2="420" y2="300"/><line x1="160" y1="100" x2="700" y2="180"/><line x1="155" y1="140" x2="500" y2="430"/></g>
  <rect x="400" y="170" width="430" height="270" fill="none" stroke="#58a6ff" stroke-width="3"/>
  <g stroke="#30363d" stroke-width="1">
    <line x1="400" y1="260" x2="830" y2="260"/><line x1="400" y1="350" x2="830" y2="350"/>
    <line x1="500" y1="170" x2="500" y2="440"/><line x1="615" y1="170" x2="615" y2="440"/><line x1="730" y1="170" x2="730" y2="440"/></g>
  <text x="430" y="200" fill="#e6edf3" font-size="24">正交取景框 = 一张阴影深度图</text>
</svg>
<div class="foot">平行光用正交方框套住场景渲一张深度图。本场景小，一张够用；超大场景才需分成好几级。</div></div>'''

S['slide_pcf']='<div class="slide"><div class="sectionhead">硬边变软边：PCF</div>'\
 +'<div class="row">'\
 +'<div class="panel" style="width:740px"><div class="pl">只采样一次 → 硬、带锯齿</div><img src="'+IMG+'shadow_hard_crop.png"></div>'\
 +'<div class="panel hl" style="width:740px"><div class="pl accent">采样一小片再平均 → 软</div><img src="'+IMG+'shadow_soft_crop.png"></div>'\
 +'</div><div class="foot">查一个点，影子边非黑即白；查周围一小片按比例平均，边缘就柔和了。</div></div>'

S['slide_bias']='''<div class="slide">
<div class="sectionhead">顺带一提：自己挡自己（阴影痤疮）</div>
<div class="row">
  <div class="card" style="width:640px;height:420px;align-items:center;justify-content:center">
    <div class="pl red" style="font-size:28px;margin-bottom:18px">没修正</div>
    <svg width="500" height="280"><rect width="500" height="280" fill="#1b2330"/>
      <g fill="#0a0d12">''' + ''.join('<rect x="%d" y="%d" width="14" height="14"/>'%((i*37)%480,(i*53)%260) for i in range(60)) + '''</g>
      <text x="120" y="150" fill="#ff7b72" font-size="24">表面冒出黑斑</text></svg>
  </div>
  <div class="plus accent">&#8594;</div>
  <div class="card hl" style="width:640px;height:420px;align-items:center;justify-content:center">
    <div class="pl accent" style="font-size:28px;margin-bottom:18px">加一点深度偏移</div>
    <svg width="500" height="280"><rect width="500" height="280" fill="#1b2330"/>
      <text x="170" y="150" fill="#3fb950" font-size="26">干净了</text></svg>
  </div>
</div>
<div class="foot">深度图精度有限，平面会误判成自己挡自己；判断时把深度往光的方向抬一点点，留出容差即可。</div></div>'''

S['slide_fog_hook']='<div class="slide"><div class="sectionhead">最后一块拼图：雾</div>'\
 +'<div class="row">'+panel(IMG+'fog_off.png','没有雾','',820)+panel(IMG+'final.png','加上雾之后','hl',820)+'</div>'\
 +'<div class="foot">和 Unity 一比，远处的墙、天地交界，那边蒙着一层淡淡的雾。把雾、色调、天空盒补齐再对账。</div></div>'

S['slide_fog']='''<div class="slide">
<div class="sectionhead">雾：远处褪向雾色</div>
<svg width="1200" height="460">
  <defs><linearGradient id="fg" x1="0" y1="0" x2="1" y2="0">
    <stop offset="0" stop-color="#3a4250"/><stop offset="1" stop-color="#aeb6c2"/></linearGradient></defs>
  <polygon points="100,420 1100,420 760,120 440,120" fill="url(#fg)"/>
  <g stroke="#0d1117" stroke-width="2" opacity=".4">
    <line x1="220" y1="420" x2="488" y2="120"/><line x1="420" y1="420" x2="560" y2="120"/>
    <line x1="620" y1="420" x2="640" y2="120"/><line x1="820" y1="420" x2="712" y2="120"/></g>
  <rect x="150" y="350" width="80" height="70" fill="#d29922"/><text x="120" y="445" fill="#e6edf3" font-size="22">近：颜色饱满</text>
  <rect x="600" y="150" width="40" height="34" fill="#9aa3b0"/><text x="980" y="150" fill="#8b949e" font-size="22">远：几乎只剩雾色</text>
</svg>
<div class="foot">离相机越远，混入的雾色越多。雾的模式 / 颜色 / 浓度都从 Unity 导出，写在 JSON 里，所以能对上。</div></div>'''

S['slide_aces']='''<div class="slide">
<div class="sectionhead">色调映射：把过亮压回来（ACES）</div>
<svg width="760" height="500">
  <line x1="90" y1="430" x2="730" y2="430" stroke="#8b949e" stroke-width="2"/>
  <line x1="90" y1="430" x2="90" y2="40" stroke="#8b949e" stroke-width="2"/>
  <text x="300" y="475" fill="#8b949e" font-size="24">算出来的亮度（可以很大）</text>
  <text x="30" y="240" fill="#8b949e" font-size="24" transform="rotate(-90 30,240)">屏幕亮度 0–1</text>
  <path d="M90,430 C 250,430 320,120 730,90" fill="none" stroke="#58a6ff" stroke-width="5"/>
  <line x1="600" y1="430" x2="600" y2="105" stroke="#ff7b72" stroke-width="2" stroke-dasharray="6 6"/>
  <line x1="90" y1="105" x2="600" y2="105" stroke="#ff7b72" stroke-width="2" stroke-dasharray="6 6"/>
  <circle cx="600" cy="105" r="8" fill="#58a6ff"/>
  <text x="430" y="415" fill="#ff7b72" font-size="22">很亮的输入</text>
  <text x="120" y="95" fill="#3fb950" font-size="22">被柔和压回可显示范围</text>
</svg>
<div class="foot">不压，亮处一片死白；用这条 S 曲线把过亮柔和压回，亮处保留层次。这套叫 ACES，电影常用。</div></div>'''

for name,body in S.items():
    open(name+'.html','w',encoding='utf-8').write(HEAD+body+TAIL)
print('wrote',len(S),'EP2 files:',list(S.keys()))
