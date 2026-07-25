// 범용 md → 열람 페이지 빌더 (살란_전서와 같은 양피지 스타일 + 사이드바 차례)
// 사용: node _build_md_열람.js <입력.md> <출력.html> "<제목>" "<부제(선택)>"
// 정본은 언제나 md 쪽이고 출력 html은 산출물(직접 수정 금지).
const fs = require('fs');
const path = require('path');

const [, , inFile, outFile, title, subtitle, linksArg, diagramsArg] = process.argv;
if (!inFile || !outFile || !title) {
  console.error('사용: node _build_md_열람.js <입력.md> <출력.html> "<제목>" "<부제>" "라벨=파일.html;라벨2=파일2.html"');
  process.exit(1);
}
// linksArg: 다른 페이지로 가는 이동 링크(선택) — "라벨=href;라벨=href"
const navLinks = (linksArg || '').split(';').filter(Boolean).map(s => {
  const i = s.indexOf('=');
  return { label: s.slice(0, i), href: s.slice(i + 1) };
});

function esc(s){ return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }
function inline(s){
  s = esc(s);
  s = s.replace(/\*\*([^*]+)\*\*/g, '<b>$1</b>');
  s = s.replace(/`([^`]+)`/g, '<code>$1</code>');
  return s;
}

function mdToHtml(md, prefix){
  const lines = md.split(/\r?\n/);
  const out = [];
  const toc = [];
  let hn = 0;
  let para = [], list = null, quote = [], table = [];

  function flushPara(){ if(para.length){ out.push('<p>'+inline(para.join(' '))+'</p>'); para=[]; } }
  function flushList(){ if(list){ out.push('</'+list+'>'); list=null; } }
  function flushQuote(){ if(quote.length){ out.push('<blockquote>'+quote.map(q=>'<p>'+inline(q)+'</p>').join('')+'</blockquote>'); quote=[]; } }
  function flushTable(){
    if(!table.length) return;
    const rows = table.filter(r => !/^\|[\s:|-]+\|$/.test(r.replace(/\s/g,'')));
    let h = '<div class="tblwrap"><table>';
    rows.forEach((r,i)=>{
      const cells = r.replace(/^\|/,'').replace(/\|\s*$/,'').split('|').map(c=>inline(c.trim()));
      const tag = i===0 ? 'th' : 'td';
      h += '<tr>' + cells.map(c=>`<${tag}>${c}</${tag}>`).join('') + '</tr>';
    });
    h += '</table></div>';
    out.push(h); table=[];
  }
  function flushAll(){ flushPara(); flushList(); flushQuote(); flushTable(); }

  for(const raw of lines){
    const line = raw.replace(/\s+$/,'');
    if(!line.trim()){ flushAll(); continue; }
    let m;
    if((m = line.match(/^(#{1,4})\s+(.*)$/))){
      flushAll();
      const lv = m[1].length, txt = m[2];
      if(lv === 1) continue;
      const id = prefix + '-h' + (++hn);
      if(lv <= 3) toc.push({level: lv, text: txt.replace(/\*\*/g,''), id});
      out.push(`<h${lv} id="${id}">${inline(txt)}</h${lv}>`);
      continue;
    }
    if(/^---+$/.test(line.trim())){ flushAll(); out.push('<hr>'); continue; }
    if(/^>/.test(line)){ flushPara(); flushList(); flushTable(); quote.push(line.replace(/^>\s?/,'')); continue; }
    if(/^\|/.test(line.trim())){ flushPara(); flushList(); flushQuote(); table.push(line.trim()); continue; }
    if((m = line.match(/^\s*[-*]\s+(.*)$/))){
      flushPara(); flushQuote(); flushTable();
      if(list !== 'ul'){ flushList(); out.push('<ul>'); list='ul'; }
      out.push('<li>'+inline(m[1])+'</li>'); continue;
    }
    if((m = line.match(/^\s*\d+\.\s+(.*)$/))){
      flushPara(); flushQuote(); flushTable();
      if(list !== 'ol'){ flushList(); out.push('<ol>'); list='ol'; }
      out.push('<li>'+inline(m[1])+'</li>'); continue;
    }
    if(list){ out[out.length-1] = out[out.length-1].replace(/<\/li>$/, ' ' + inline(line.trim()) + '</li>'); continue; }
    flushQuote(); flushTable();
    para.push(line.trim());
  }
  flushAll();
  return { html: out.join('\n'), toc };
}

const md = fs.readFileSync(inFile, 'utf8');
const doc = mdToHtml(md, 'm');

// 도해 주입(선택): diagramsArg = 도해 모듈 경로. 모듈은 [{match:"제목 일부", html:"<svg…>"}] 배열을 export.
// 각 도해는 제목 텍스트에 match가 처음 포함되는 h2/h3 바로 뒤에 <div class="fig">로 꽂힌다.
let figCount = 0;
if (diagramsArg) {
  const diagrams = require(path.resolve(diagramsArg));
  for (const d of diagrams) {
    const re = new RegExp('(<h[23] id="m-h\\d+">[^<]*' + d.match.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '[^<]*</h[23]>)');
    if (re.test(doc.html)) {
      doc.html = doc.html.replace(re, '$1\n<div class="fig">' + d.html + (d.caption ? '<div class="figcap">' + d.caption + '</div>' : '') + '</div>');
      figCount++;
    } else {
      console.warn('도해 매칭 실패:', d.match);
    }
  }
}
const now = new Date();
const stamp = `${now.getFullYear()}-${String(now.getMonth()+1).padStart(2,'0')}-${String(now.getDate()).padStart(2,'0')}`;

const tocHtml = '<ul>' + doc.toc.map(t =>
  `<li class="lv${t.level}"><a href="#${t.id}">${esc(t.text)}</a></li>`).join('') + '</ul>';

const page = `<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>${esc(title)}</title>
<style>
  body{font-family:'Malgun Gothic',sans-serif;background:#f2ecdf;color:#33302a;margin:0;padding:0}
  .layout{display:flex;align-items:flex-start}
  nav#side{position:sticky;top:0;height:100vh;overflow-y:auto;width:280px;flex:0 0 280px;
    background:#ece4d2;border-right:1px solid #cfc4a8;padding:18px 14px;font-size:.85em;box-sizing:border-box}
  nav#side .tt{font-weight:bold;font-size:1.05em;color:#5a4f3a;border-bottom:2px solid #b3a67f;padding-bottom:6px;margin-bottom:8px}
  nav#side ul{list-style:none;margin:0;padding:0}
  nav#side li{margin:2px 0;line-height:1.4}
  nav#side li.lv3{padding-left:14px;font-size:.93em}
  nav#side a{color:#6b5d40;text-decoration:none}
  nav#side a:hover{color:#a13b2e}
  nav#side .xlinks{margin:0 0 10px;padding:8px 10px;background:#e7dcc3;border:1px solid #c9bc9c;border-radius:8px}
  nav#side .xlinks a{display:block;font-weight:bold;color:#5a4f3a;margin:3px 0}
  main#doc{flex:1;min-width:0;max-width:1050px;padding:24px 36px;box-sizing:border-box}
  main h1{font-size:1.5em;border-bottom:3px solid #8a7a5c;padding-bottom:8px}
  main h2{font-size:1.18em;margin-top:44px;border-left:6px solid #8a7a5c;padding-left:10px}
  main h3{font-size:1.05em;margin-top:32px;color:#5a4f3a;border-left:3px solid #c9bc9c;padding-left:8px}
  main h4{font-size:.98em;margin-top:24px;color:#5a4f3a}
  main p{line-height:1.75;font-size:.94em}
  main li{line-height:1.7;font-size:.94em;margin:4px 0}
  main blockquote{background:#ece2c8;border-left:5px solid #b3a67f;margin:14px 0;padding:10px 14px;border-radius:0 8px 8px 0}
  main blockquote p{margin:4px 0;font-size:.88em;color:#5f5744}
  .tblwrap{overflow-x:auto;margin:14px 0}
  main table{border-collapse:collapse;font-size:.9em;min-width:60%}
  main th{background:#e7dcc3;border-bottom:2px solid #b3a67f;padding:7px 10px;text-align:left}
  main td{border-bottom:1px solid #d8cdb0;padding:7px 10px;vertical-align:top}
  main code{background:#e7dcc3;border-radius:4px;padding:1px 5px;font-size:.9em}
  main hr{border:none;border-top:1px dashed #c9bc9c;margin:30px 0}
  .fig{background:#faf6ec;border:1px solid #cfc4a8;border-radius:10px;padding:14px;margin:16px 0;text-align:center;overflow-x:auto}
  .fig svg{max-width:100%;height:auto}
  .figcap{font-size:.85em;color:#6b6353;margin-top:8px;line-height:1.5;text-align:left}
  .note{color:#6b6353;font-size:.9em;line-height:1.6}
  .top{position:fixed;right:20px;bottom:20px;background:#8a7a5c;color:#f6f1e4;border-radius:50%;
    width:44px;height:44px;line-height:44px;text-align:center;cursor:pointer;font-size:18px;user-select:none;opacity:.9}
  #sideBtn{display:none;position:fixed;left:14px;bottom:20px;z-index:30;background:#8a7a5c;color:#f6f1e4;
    border:none;border-radius:22px;padding:10px 16px;font-size:14px;cursor:pointer;opacity:.92}
  @media (max-width:1000px){
    nav#side{position:fixed;left:0;top:0;z-index:20;transform:translateX(-100%);transition:transform .2s;box-shadow:4px 0 18px rgba(0,0,0,.2)}
    nav#side.open{transform:none}
    #sideBtn{display:block}
    main#doc{padding:18px 14px}
  }
</style>
</head>
<body>
<button id="sideBtn" onclick="document.getElementById('side').classList.toggle('open')">☰ 차례</button>
<div class="layout">
<nav id="side">
  <div class="tt">${esc(title)}</div>
  ${navLinks.length ? '<div class="xlinks">' + navLinks.map(l => `<a href="${esc(l.href)}">↗ ${esc(l.label)}</a>`).join('') + '</div>' : ''}
  ${tocHtml}
</nav>
<main id="doc">
<h1>${esc(title)}</h1>
<p class="note">${subtitle ? esc(subtitle) + ' · ' : ''}정본은 <b>${esc(path.basename(inFile))}</b> — 이 페이지는 열람용 산출물이다(${stamp} 생성 · <code>node _build_md_열람.js</code>로 재생성, 직접 수정 금지).</p>
${doc.html}
</main>
</div>
<div class="top" onclick="window.scrollTo({top:0,behavior:'smooth'})">↑</div>
</body>
</html>
`;

fs.writeFileSync(outFile, page, 'utf8');
console.log('built:', outFile, Math.round(page.length/1024)+'KB', '| 차례 항목:', doc.toc.length);
