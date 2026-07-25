// 역사 본선 1~3권 → 통독용 단일 HTML 생성기 (읽기 전용 · 정본 파이프라인과 무관)
// 실행: node _build_역사본선_통독.js  → 역사본선_통독.html
const fs = require('fs');
const path = require('path');

const ROOT = path.join(__dirname, '..', '..');
const BOOKS = [
  { file: '기획/01_세계관/1_고대사/본선/세계관_역사본선_1_초고대.md',    label: '1권' },
  { file: '기획/01_세계관/1_고대사/본선/세계관_역사본선_2_여명.md',      label: '2권' },
  { file: '기획/01_세계관/1_고대사/본선/세계관_역사본선_3_찬란한시대.md', label: '3권' },
];

function esc(s) {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

function inline(s) {
  s = esc(s);
  s = s.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
  s = s.replace(/`([^`]+)`/g, '<code>$1</code>');
  // [열림 …] / [잠정 …] 주석은 옅게
  s = s.replace(/\[(열림|잠정|확정)([^\]]*)\]/g, '<span class="note">[$1$2]</span>');
  return s;
}

let toc = [];
let bodyHtml = '';
let anchorSeq = 0;

for (const book of BOOKS) {
  const raw = fs.readFileSync(path.join(ROOT, book.file), 'utf8');
  const lines = raw.split(/\r?\n/);
  let html = '';
  let bookTitle = book.label;
  let tocEntry = { book: '', parts: [] };
  let para = [];

  const flush = () => {
    if (para.length) {
      html += `<p>${inline(para.join(' '))}</p>\n`;
      para = [];
    }
  };

  for (const line of lines) {
    const t = line.trim();
    if (!t) { flush(); continue; }

    if (t.startsWith('# ')) {              // 권 제목·부 제목 (전부 h1, 목차 t1)
      flush();
      bookTitle = t.slice(2);
      const id = `a${anchorSeq++}`;
      tocEntry.parts.push({ title: bookTitle, id, level: 1 });
      html += `<h1 id="${id}">${inline(bookTitle)}</h1>\n`;
    } else if (t.startsWith('## §') && /조사 대기/.test(t)) { // 대기 칸: 통독에서 접기
      flush();
      html += `<details class="meta"><summary>${inline(t.slice(3))} (작업 메모 — 펼치기)</summary>\n`;
    } else if (t.startsWith('# ')) {
      flush();
    } else if (t.startsWith('## ')) {      // 절
      flush();
      const title = t.slice(3);
      const id = `a${anchorSeq++}`;
      tocEntry.parts.push({ title, id, level: 2 });
      html += `<h2 id="${id}">${inline(title)}</h2>\n`;
    } else if (/^#{1,2} /.test(t) === false && t.startsWith('#')) {
      flush();
      // (h3 이하 없음 — 안전망)
      html += `<h3>${inline(t.replace(/^#+\s*/, ''))}</h3>\n`;
    } else if (t.startsWith('> ')) {
      flush();
      html += `<blockquote>${inline(t.slice(2))}</blockquote>\n`;
    } else if (t === '---') {
      flush();
      html += '<hr>\n';
    } else if (t.startsWith('- ')) {
      flush();
      html += `<p class="li">· ${inline(t.slice(2))}</p>\n`;
    } else {
      para.push(t);
    }
  }
  flush();
  // 열린 details 닫기(대기 칸이 파일 끝까지 가는 구조)
  const opens = (html.match(/<details/g) || []).length;
  const closes = (html.match(/<\/details>/g) || []).length;
  for (let i = closes; i < opens; i++) html += '</details>\n';

  toc.push(tocEntry);
  bodyHtml += `<section class="book">\n${html}\n</section>\n`;
}

// 부(部) 제목(# 2부 …)이 h1으로 잡히는 문제 방지: 위 파서는 첫 '# '만 권 제목으로 쓰지 않으므로
// 실제로는 각 '# ' 줄이 전부 h1로 나감 — 부 제목도 h1이 되는데 통독에선 그게 자연스러움.

let tocHtml = '<nav class="toc"><div class="toc-title">차례</div>\n';
for (const e of toc) {
  for (const p of e.parts) {
    if (/조사 대기/.test(p.title)) continue;
    tocHtml += `<a class="${p.level === 1 ? 't1' : 't2'}" href="#${p.id}">${esc(p.title)}</a>\n`;
  }
}
tocHtml += '</nav>\n';

const page = `<!doctype html>
<html lang="ko">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>역사 본선 통독 — 초고대 · 여명 · 찬란한 시대</title>
<style>
  :root { --bg:#14120e; --paper:#1c1914; --ink:#e8e0d0; --dim:#9a8f7a; --accent:#d4a24e; --line:#3a3428; }
  * { margin:0; padding:0; box-sizing:border-box; }
  body { background:var(--bg); color:var(--ink); font-family:'Noto Serif KR','Nanum Myeongjo',serif; line-height:1.95; }
  .wrap { display:flex; max-width:1200px; margin:0 auto; gap:0; }
  .toc { position:sticky; top:0; align-self:flex-start; max-height:100vh; overflow-y:auto;
         width:300px; flex:none; padding:36px 20px 60px 24px; border-right:1px solid var(--line);
         font-family:'Pretendard','Malgun Gothic',sans-serif; font-size:13px; }
  .toc-title { color:var(--accent); font-weight:700; margin-bottom:14px; letter-spacing:.2em; }
  .toc a { display:block; color:var(--dim); text-decoration:none; padding:3px 0; }
  .toc a:hover { color:var(--ink); }
  .toc .t1 { color:var(--accent); font-weight:600; margin-top:16px; line-height:1.5; }
  main { flex:1; min-width:0; padding:48px 56px 120px; background:var(--paper); }
  h1 { font-size:26px; color:var(--accent); margin:70px 0 10px; line-height:1.5; font-weight:700; }
  h1:first-child { margin-top:0; }
  h2 { font-size:20px; margin:54px 0 18px; padding-bottom:8px; border-bottom:1px solid var(--line); line-height:1.6; }
  p { margin:0 0 1.15em; text-align:justify; }
  p.li { margin:0 0 .4em; color:var(--dim); font-size:15px; }
  blockquote { color:var(--dim); font-size:14.5px; border-left:3px solid var(--line); padding:2px 0 2px 14px; margin:0 0 1em; }
  hr { border:0; border-top:1px solid var(--line); margin:44px auto; width:40%; }
  strong { color:var(--accent); font-weight:700; }
  code { color:var(--dim); font-size:.9em; }
  .note { color:var(--dim); font-size:.85em; }
  details.meta { margin:30px 0; font-size:14px; color:var(--dim); }
  details.meta summary { cursor:pointer; color:var(--dim); }
  @media (max-width:900px){ .wrap{flex-direction:column} .toc{position:static;width:100%;max-height:none;border-right:0;border-bottom:1px solid var(--line)} main{padding:28px 20px 80px} }
</style>
</head>
<body>
<div class="wrap">
${tocHtml}
<main>
${bodyHtml}
</main>
</div>
</body>
</html>`;

fs.writeFileSync(path.join(ROOT, '웹', '역사본선_통독.html'), page, 'utf8');
console.log('OK → 역사본선_통독.html (' + Math.round(page.length / 1024) + 'KB)');
