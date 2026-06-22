// 用已安装的 Chrome 顺序把 _html/*.html 渲成 ../<name>.png（1920x1080）。
// execSync 阻塞到每个 chrome 退出，避免并发竞态。用法：node render.js a b c...
const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const CHROME = 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const HTMLDIR = __dirname;                       // assets/_html
const OUTDIR  = path.resolve(__dirname, '..');   // assets
const names = process.argv.slice(2);
if (names.length === 0) { console.error('no slide names'); process.exit(1); }

let ok = 0;
names.forEach((name, idx) => {
  const html = path.join(HTMLDIR, name + '.html');
  const png  = path.join(OUTDIR, name + '.png');
  const prof = path.join(OUTDIR, '..', '..', '..', 'out', '_cprof_' + idx); // out/_cprof_N
  try { fs.unlinkSync(png); } catch (e) {}
  const cmd = `"${CHROME}" --headless=new --disable-gpu --no-sandbox --hide-scrollbars `
            + `--force-device-scale-factor=1 --user-data-dir="${prof}" `
            + `--screenshot="${png}" --window-size=1920,1080 "file:///${html.replace(/\\/g,'/')}"`;
  try {
    execSync(cmd, { stdio: 'ignore', timeout: 60000 });
  } catch (e) { /* chrome 有时非零退出但已出图 */ }
  const good = fs.existsSync(png) && fs.statSync(png).size > 2000;
  if (good) ok++;
  console.log((good ? '  OK   ' : '  MISS ') + name);
});
console.log(`rendered ${ok}/${names.length}`);
