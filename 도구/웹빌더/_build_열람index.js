// 웹/ 안의 열람 페이지를 훑어 루트 열람.html (진입점 하나)을 만든다.
// 실행: node 도구/웹빌더/_build_열람index.js
// 2026-07-25 루트 정리로 html 49개가 웹/ 으로 들어가면서, 어디로 들어가는지가 안 보이게 되어 만듦.
const fs = require('fs');
const path = require('path');

const ROOT = path.join(__dirname, '..', '..');
const WEB = path.join(ROOT, '웹');
const OUT = path.join(ROOT, '열람.html');

/* 묶음 = [제목, 이모지, 판별함수] — 위에서부터 먼저 걸리는 묶음으로 간다 */
const GROUPS = [
  ['들머리', '🚪', (f) => ['세계관_세계지도.html', '세계관_현대지도.html', '세계관_역사흐름.html'].includes(f)],
  ['게임', '🎮', (f) => f.startsWith('게임') || f === '전투시뮬.html'],
  ['살란 — 문자와 기술', '🔮', (f) => f.startsWith('살란_')],
  ['세계관 — 통사·본선', '📜', (f) => f.startsWith('역사본선') || f.includes('강대국세계사') || f.includes('안개너머')],
  ['세계관 — 갈래', '🌍', (f) => f.startsWith('세계관_')],
  ['그 밖', '📄', () => true],
];

const titleOf = (html, fallback) => {
  const m = html.match(/<title>([\s\S]*?)<\/title>/i);
  return m ? m[1].replace(/\s+/g, ' ').trim() : fallback;
};

const files = fs.readdirSync(WEB).filter((f) => f.toLowerCase().endsWith('.html')).sort();
const buckets = GROUPS.map(([name, ic]) => ({ name, ic, items: [] }));

for (const f of files) {
  const full = path.join(WEB, f);
  const html = fs.readFileSync(full, 'utf8');
  const kb = (fs.statSync(full).size / 1024).toFixed(0);
  const gi = GROUPS.findIndex(([, , test]) => test(f));
  buckets[gi].items.push({ f, title: titleOf(html, f.replace(/\.html$/, '')), kb });
}

const esc = (s) => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
const stamp = new Date().toISOString().slice(0, 16).replace('T', ' ');

let body = '';
for (const b of buckets) {
  if (!b.items.length) continue;
  body += `<section><h2>${b.ic} ${esc(b.name)} <span class="n">${b.items.length}</span></h2><ul>`;
  for (const it of b.items) {
    body += `<li><a href="웹/${encodeURI(it.f)}"><span class="t">${esc(it.title)}</span>`
          + `<span class="f">${esc(it.f)} · ${it.kb}KB</span></a></li>`;
  }
  body += '</ul></section>';
}

const page = `<!DOCTYPE html>
<html lang="ko"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>📚 Secret Project — 열람</title>
<style>
:root{--bg:#f6f2e9;--ink:#2e2a24;--dim:#6b6353;--line:#ded4c0;--card:#fffdf8;--accent:#8a5a2b}
@media (prefers-color-scheme:dark){:root{--bg:#171512;--ink:#e9e2d5;--dim:#9c9384;--line:#332e26;--card:#1f1c18;--accent:#c99b62}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);
  font-family:"Pretendard","Malgun Gothic",system-ui,sans-serif;line-height:1.6}
.wrap{max-width:1080px;margin:0 auto;padding:48px 24px 80px}
h1{font-size:1.9rem;margin:0 0 6px;letter-spacing:-.02em}
.sub{color:var(--dim);font-size:.9rem;margin:0 0 36px}
.sub code{background:var(--card);border:1px solid var(--line);border-radius:4px;padding:1px 5px;font-size:.86em}
section{margin:0 0 30px}
h2{font-size:1.02rem;font-weight:700;color:var(--accent);margin:0 0 10px;
  padding-bottom:7px;border-bottom:1px solid var(--line);display:flex;align-items:center;gap:8px}
h2 .n{font-size:.76rem;color:var(--dim);font-weight:400;background:var(--card);
  border:1px solid var(--line);border-radius:10px;padding:0 7px}
ul{list-style:none;margin:0;padding:0;display:grid;gap:7px;
  grid-template-columns:repeat(auto-fill,minmax(268px,1fr))}
a{display:block;text-decoration:none;color:inherit;background:var(--card);
  border:1px solid var(--line);border-radius:8px;padding:10px 12px;transition:.13s}
a:hover{border-color:var(--accent);transform:translateY(-1px)}
.t{display:block;font-size:.92rem;font-weight:600;margin-bottom:2px}
.f{display:block;font-size:.72rem;color:var(--dim);font-family:ui-monospace,Consolas,monospace}
footer{margin-top:44px;padding-top:16px;border-top:1px solid var(--line);
  color:var(--dim);font-size:.78rem}
</style></head><body><div class="wrap">
<h1>📚 Secret Project — 열람</h1>
<p class="sub">열람 페이지 ${files.length}장. 실제 파일은 <code>웹/</code> 에 있고, 정본 md 는 <code>기획/</code> 에 있다.
이 페이지는 <code>node 도구/웹빌더/_build_열람index.js</code> 로 재생성한다 — 직접 고치지 말 것.</p>
${body}
<footer>${stamp} 생성 · 페이지가 늘거나 이름이 바뀌면 위 명령을 다시 돌리면 된다.</footer>
</div></body></html>
`;

fs.writeFileSync(OUT, page, 'utf8');
console.log(`열람.html 생성: ${files.length}장 / ${buckets.filter(b=>b.items.length).length}묶음 (${(Buffer.byteLength(page,'utf8')/1024).toFixed(1)}KB)`);
