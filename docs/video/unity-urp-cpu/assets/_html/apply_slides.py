# -*- coding: utf-8 -*-
# 把 script.md 里指定 #id 的 @visual:animation 块替换为 @visual:image(../assets/slide.png)，
# 旁白(narration)原样保留，visual 段换成一行文档说明。
import re, io

MAP = {
 'ep1': {
  'B02':('slide_series_map','系列两集卡片：上集(导出+几何，高亮) / 下集(光照阴影成像)'),
  'B03':('slide_gpu_vs_cpu','GPU(封闭·锁) vs CPU(可调试·放大镜) 左右对比面板，✗/✓ 列表'),
  'B04':('slide_dataflow','端到端数据流：Unity 场景→C# 导出器→JSON+mesh→C++ 加载器→渲染管线→一帧画面'),
  'B07':('slide_scene_json','scene.json 代码窗口，高亮相机/主光/环境光(球谐)/雾/物体五个字段'),
  'B09':('slide_align3','三处必须对齐：坐标系 / 矩阵约定 / 光照单位，三张卡片'),
  'B10':('slide_geo_pipeline','几何管线四步流程图：顶点变换→裁剪→光栅化→插值+深度'),
  'B11':('slide_transform','顶点变换四步：模型坐标→摆进世界(M)→相机视角(V)→压成屏幕(P)'),
  'B12':('slide_clip','裁剪示意：三角形跨屏幕边界被裁，蓝色=保留在屏幕内的部分'),
  'B13':('slide_raster','光栅化：三角形覆盖像素网格，包围盒内逐像素判定，里面填、外面跳过'),
  'B15':('slide_depth','深度缓冲：近的卡片挡住远的卡片，每像素只留最近'),
  'B17':('slide_p1_recap','上集小结五要点 + "下集→像素的颜色怎么算" 预告'),
 },
 'ep2': {
  'B02':('slide_materials','材质两张贴图面板：基础色 albedo + 法线 normal'),
  'B07':('slide_bug','1/π bug 三联：①灭灯纯黑 ②有bug墙发黑(红) ③修复恢复青色'),
  'B09':('slide_shadow_diff','找不同：没有阴影 vs 有阴影 两图并排'),
  'B10':('slide_shadow_idea','阴影两遍法：第一遍从太阳视角记远近 / 第二遍着色时查遮挡'),
  'B11':('slide_shadow_fit','正交取景框套住整个场景，渲一张阴影深度图'),
  'B12':('slide_pcf','硬阴影(采样一次) vs PCF 软阴影(采样一小片平均) 边缘对比'),
  'B14':('slide_bias','阴影痤疮：表面自己挡自己冒黑斑 → 加一点深度偏移修复'),
  'B15':('slide_fog_hook','找不同：没有雾 vs 有雾 两图并排'),
  'B16':('slide_fog','雾按距离褪向雾色的示意（近饱满→远只剩雾色）'),
  'B17':('slide_aces','ACES 色调映射 S 曲线：把过亮输入柔和压回 0–1'),
 },
}

for ep,bm in MAP.items():
    path='../../%s/script.md'%ep
    txt=open(path,encoding='utf-8').read()
    # split into blocks keeping the '>>> ' delimiter
    parts=re.split(r'(?=^>>> )', txt, flags=re.M)
    out=[]
    for blk in parts:
        m=re.match(r'>>> .*?#(B\d+)', blk)
        if m and m.group(1) in bm:
            slide,desc=bm[m.group(1)]
            # replace @visual line
            blk=re.sub(r'@visual:\s*animation', '@visual: image(../assets/%s.png)'%slide, blk, count=1)
            # replace the visual body (between '--- visual ---' and '--- narration ---')
            doc='（实际使用 ../assets/%s.png：%s。本图由 HTML 渲染生成，源文件 _html/%s.html）'%(slide,desc,slide)
            blk=re.sub(r'(--- visual ---\n).*?(\n--- narration ---)',
                       lambda mm: mm.group(1)+doc+mm.group(2), blk, count=1, flags=re.S)
        out.append(blk)
    open(path,'w',encoding='utf-8').write(''.join(out))
    print('updated',ep)
print('done')
