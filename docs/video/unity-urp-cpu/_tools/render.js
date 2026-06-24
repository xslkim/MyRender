// 用已安装的 Chrome 顺序把 _tools/*.html 渲成对应集 assets/slide_*.png（1920x1080）。
// execSync 阻塞到每个 chrome 退出，避免并发竞态。用法：node render.js a b c...
//
// 输出目录按 slide 所属集路由：
//   EP1 幻灯片 → ../ep1/assets/，其余（EP2）→ ../ep2/assets/
//   白名单（EP1_SLIDES）取自 gen_ep1.py 实际生成的 slide 名。
const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');
const os = require('os');

const CHROME = 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const HTMLDIR = __dirname;                                    // _tools/
const ROOT    = path.resolve(__dirname, '..');                // unity-urp-cpu/

// gen_ep1.py 生成的 slide 名（去掉 slide_ 前缀与 .png 后缀）
const EP1_SLIDES = new Set([
  'slide_series_map', 'slide_gpu_vs_cpu', 'slide_dataflow', 'slide_scene_json',
  'slide_align3', 'slide_geo_pipeline', 'slide_transform', 'slide_clip',
  'slide_raster', 'slide_depth', 'slide_p1_recap',
]);

function outDirFor(name) {
  return path.join(ROOT, EP1_SLIDES.has(name) ? 'ep1/assets' : 'ep2/assets');
}

const names = process.argv.slice(2);
if (names.length === 0) { console.error('no slide names'); process.exit(1); }

let ok = 0;
names.forEach((name, idx) => {
  const html = path.join(HTMLDIR, name + '.html');
  const outdir = outDirFor(name);
  fs.mkdirSync(outdir, { recursive: true });
  const png  = path.join(outdir, name + '.png');
  const prof = path.join(os.tmpdir(), 'mrslide_cprof_' + idx); // 系统临时目录，不污染仓库
  try { fs.unlinkSync(png); } catch (e) {}
  const cmd = `"${CHROME}" --headless=new --disable-gpu --no-sandbox --hide-scrollbars `
            + `--force-device-scale-factor=1 --user-data-dir="${prof}" `
            + `--screenshot="${png}" --window-size=1920,1080 "file:///${html.replace(/\\/g,'/')}"`;
  try {
    execSync(cmd, { stdio: 'ignore', timeout: 60000 });
  } catch (e) { /* chrome 有时非零退出但已出图 */ }
  const good = fs.existsSync(png) && fs.statSync(png).size > 2000;
  if (good) ok++;
  const dest = path.relative(ROOT, png).replace(/\\/g, '/');
  console.log((good ? '  OK   ' : '  MISS ') + name + '  →  ' + dest);
});
console.log(`rendered ${ok}/${names.length}`);
