// 살란 전서 빌더 — 기획/01_세계관/4_체계/언어와_지명/세계관_언어와_문자.md + 살란_문자도감.html + 기획/01_세계관/4_체계/능력과_마력/세계관_회로와_기술_체계.md
// 를 한 페이지(살란_전서.html)로 합친다. 정본은 언제나 md/도감 쪽이고 이 페이지는 산출물.
// 재생성: node _build_살란_전서.js
const fs = require('fs');
// 도구/웹빌더/ 에서 두 단계 위 = 프로젝트 루트 (2026-07-25 루트 정리)
const P = __dirname + '\\..\\..\\';

const SRC = {
  lang:  P + '기획/01_세계관/4_체계/언어와_지명/세계관_언어와_문자.md',
  dogam: P + '웹\\살란_문자도감.html',
  circ:  P + '기획/01_세계관/4_체계/능력과_마력/세계관_회로와_기술_체계.md',
  hist:  P + '기획/01_세계관/0_색인/세계관_노트북LM_단일소스_강대국세계사_20260714.md',
};
const OUT = P + '웹\\살란_전서.html';

function esc(s){ return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }
function inline(s){
  s = esc(s);
  s = s.replace(/\*\*([^*]+)\*\*/g, '<b>$1</b>');
  s = s.replace(/`([^`]+)`/g, '<code>$1</code>');
  return s;
}

// ---------- 마크다운 → HTML (이 두 정본이 쓰는 문법만: 제목/굵게/표/인용/목록/구분선) ----------
function mdToHtml(md, prefix){
  const lines = md.split(/\r?\n/);
  const out = [];
  const toc = []; // {level, text, id}
  let hn = 0;
  let para = [], list = null /* 'ul'|'ol' */, quote = [], table = [];

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
      if(lv === 1) continue; // 파일 제목은 파트 머리로 대체
      const id = prefix + '-h' + (++hn);
      if(lv <= 3) toc.push({level: lv, text: txt.replace(/\*\*/g,''), id});
      out.push(`<h${lv+1} id="${id}">${inline(txt)}</h${lv+1}>`); // 페이지에서 한 단계 내림(h2→h3…)
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
    // 일반 문단 (직전이 목록 항목이면 그 항목의 이어짐으로 붙임)
    if(list){ out[out.length-1] = out[out.length-1].replace(/<\/li>$/, ' ' + inline(line.trim()) + '</li>'); continue; }
    flushQuote(); flushTable();
    para.push(line.trim());
  }
  flushAll();
  return { html: out.join('\n'), toc };
}

// ---------- 읽기 ----------
const langMd  = fs.readFileSync(SRC.lang, 'utf8');
const circMd  = fs.readFileSync(SRC.circ, 'utf8');
const histMd  = fs.readFileSync(SRC.hist, 'utf8');
const dogamHtml = fs.readFileSync(SRC.dogam, 'utf8');

// 기술첩 → 포켓몬 도감식 카드 (사용자 지시 2026-07-18 — 스킬 하나하나 그림+해설)
const SKCARD = require('./_기술카드.js');
const skTrans = SKCARD.transform(circMd);

const lang = mdToHtml(langMd, 'l');
const circ = mdToHtml(skTrans.md, 'c');
const hist = mdToHtml(histMd, 'hh');

// ---------- 도해 주입 (사용자 지시 07-17: 모든 부를 도감처럼 그림과 함께 읽히게) ----------
// _전서_도해.js 의 키(절 제목 속 고유 문자열)와 일치하는 제목 바로 뒤에 SVG 도해를 끼운다.
const DOHAE = require('./_전서_도해.js');
function injectDiagrams(html, map){
  let used = [];
  html = html.replace(/<h(\d) id="([^"]+)">([\s\S]*?)<\/h\1>/g, (m, d, id, inner) => {
    const plain = inner.replace(/<[^>]+>/g, '');
    for (const key of Object.keys(map)) {
      if (plain.includes(key) && !used.includes(key)) { used.push(key); return m + '\n' + map[key]; }
    }
    return m;
  });
  const missed = Object.keys(map).filter(k => !used.includes(k));
  if (missed.length) console.warn('도해 앵커 못 찾음:', missed.join(' | '));
  return html;
}
lang.html = injectDiagrams(lang.html, DOHAE.lang);
circ.html = injectDiagrams(circ.html, DOHAE.circ);
// 기술 카드 자리 치환
circ.html = circ.html.replace(/<p>%%SK(\d+)%%<\/p>/g, (m, n) => skTrans.cards[n] || m);
// 역사 도해([{match,html,caption}] 배열)를 같은 방식으로 주입
const HDOHAE = {};
for (const d of require('./_역사_도해.js')) {
  HDOHAE[d.match] = `<div class="panel" style="text-align:center">${d.html}<div class="caption" style="text-align:left;max-width:720px;margin:8px auto 0">${d.caption}</div></div>`;
}
hist.html = injectDiagrams(hist.html, HDOHAE);

// 도감: <style>와 <body> 안쪽(렌더 스크립트 포함)을 그대로 가져온다
const dogamStyle = dogamHtml.match(/<style>([\s\S]*?)<\/style>/)[1];
let dogamBody = dogamHtml.slice(dogamHtml.indexOf('<body>')+6, dogamHtml.lastIndexOf('</body>'));
dogamBody = dogamBody.replace(/<h1>[\s\S]*?<\/h1>/, ''); // 파트 머리로 대체

// 도감 차례(Ⅰ~ⅩⅣ)를 사이드바용으로 추출
const dogamToc = [];
for(const m of dogamBody.matchAll(/<h2>([^<]+)<\/h2>/g)){
  const id = 'd-h' + (dogamToc.length+1);
  dogamToc.push({level:2, text:m[1], id});
}
{ let i = 0; dogamBody = dogamBody.replace(/<h2>/g, () => `<h2 id="d-h${++i}">`); }

// ---------- 사이드바 차례 ----------
function tocList(toc){
  return '<ul>' + toc.map(t =>
    `<li class="lv${t.level}"><a href="#${t.id}">${esc(t.text)}</a></li>`).join('') + '</ul>';
}

const now = new Date();
const stamp = `${now.getFullYear()}-${String(now.getMonth()+1).padStart(2,'0')}-${String(now.getDate()).padStart(2,'0')}`;

// ---------- 조립 ----------
const page = `<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>살란 전서 — 언어·문자·회로·세계사 종합</title>
<style>
${dogamStyle}
  /* ---- 전서 추가 스타일 ---- */
  body{max-width:none;padding:0}
  .layout{display:flex;align-items:flex-start}
  nav#side{position:sticky;top:0;height:100vh;overflow-y:auto;width:270px;flex:0 0 270px;
    background:#ece4d2;border-right:1px solid #cfc4a8;padding:18px 14px;font-size:.86em;box-sizing:border-box}
  nav#side .pt{font-weight:bold;margin:14px 0 4px;color:#5a4f3a;border-bottom:1px solid #cfc4a8;padding-bottom:3px}
  nav#side ul{list-style:none;margin:0;padding:0}
  nav#side li{margin:2px 0;line-height:1.4}
  nav#side li.lv3{padding-left:14px;font-size:.93em}
  nav#side a{color:#6b5d40;text-decoration:none}
  nav#side a:hover{color:#a13b2e}
  main#doc{flex:1;min-width:0;max-width:1100px;padding:24px 36px;box-sizing:border-box}
  .parthead{margin-top:60px;padding:14px 18px;background:#e7dcc3;border:1px solid #c9bc9c;border-radius:10px}
  .parthead:first-of-type,#p1{margin-top:10px}
  .parthead h2{margin:0;border:none;padding:0;font-size:1.3em}
  .parthead .sub{font-size:.85em;color:#6b6353;margin-top:4px}
  main h3{font-size:1.12em;margin-top:38px;border-left:6px solid #8a7a5c;padding-left:10px}
  main h4{font-size:1.0em;margin-top:26px;color:#5a4f3a}
  main h5{font-size:.95em;margin-top:20px;color:#5a4f3a}
  main p{line-height:1.75;font-size:.95em}
  main li{line-height:1.7;font-size:.95em;margin:4px 0}
  main blockquote{background:#ece2c8;border-left:5px solid #b3a67f;margin:14px 0;padding:10px 14px;border-radius:0 8px 8px 0}
  main blockquote p{margin:4px 0;font-size:.88em;color:#5f5744}
  .tblwrap{overflow-x:auto;margin:14px 0}
  main table{border-collapse:collapse;font-size:.9em;min-width:60%}
  main th{background:#e7dcc3;border-bottom:2px solid #b3a67f;padding:7px 10px;text-align:left}
  main td{border-bottom:1px solid #d8cdb0;padding:7px 10px;vertical-align:top}
  main code{background:#e7dcc3;border-radius:4px;padding:1px 5px;font-size:.9em}
  main hr{border:none;border-top:1px dashed #c9bc9c;margin:30px 0}
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
${SKCARD.CSS}
</style>
</head>
<body>
<button id="sideBtn" onclick="document.getElementById('side').classList.toggle('open')">☰ 차례</button>
<div class="layout">
<nav id="side">
  <div class="pt" style="margin-top:0;font-size:1.05em">📜 살란 전서 + 세계사</div>
  <div style="margin:8px 0 4px;padding:8px 10px;background:#e7dcc3;border:1px solid #c9bc9c;border-radius:8px">
    <a href="#p1" style="display:block;font-weight:bold">Ⅰ 언어 · <a href="#p2" style="font-weight:bold">Ⅱ 문자꼴</a> · <a href="#p3" style="font-weight:bold">Ⅲ 회로·기술</a></a>
    <a href="#p4" style="display:block;font-weight:bold">Ⅳ 강대국 세계사 (이만 년)</a>
    <a href="세계관_세계지도.html" style="display:block;font-weight:bold">↗ 세계 지도</a>
  </div>
  <div class="pt"><a href="#p1">Ⅰ부 · 언어와 문자</a></div>
  ${tocList(lang.toc)}
  <div class="pt"><a href="#p2">Ⅱ부 · 문자도감(꼴)</a></div>
  ${tocList(dogamToc)}
  <div class="pt"><a href="#p3">Ⅲ부 · 회로와 기술</a></div>
  ${tocList(circ.toc)}
  <div class="pt"><a href="#p4">Ⅳ부 · 강대국 세계사</a></div>
  ${tocList(hist.toc)}
</nav>
<main id="doc">
<h1>살란 전서 — 언어 · 문자 · 회로 · 세계사 종합</h1>
<p class="note">네 정본을 한 페이지에 합쳐 통독·검수용으로 만든 판이다(${stamp} 생성 · <code>node _build_살란_전서.js</code>로 재생성).
<b>정본은 언제나 원본 쪽</b> — 규칙은 <b>기획/01_세계관/4_체계/언어와_지명/세계관_언어와_문자.md</b>·<b>기획/01_세계관/4_체계/능력과_마력/세계관_회로와_기술_체계.md</b>, 꼴은 <b>살란_문자도감.html</b>, 역사는 <b>기획/01_세계관/0_색인/세계관_노트북LM_단일소스_강대국세계사_20260714.md</b>. 이 페이지는 산출물이라 직접 고치지 않는다.</p>

<div class="parthead" id="p1"><h2>Ⅰ부 — 언어와 문자 (살란과 살란 글)</h2>
<div class="sub">소리·낱말·문법·영창·새김·살란 글·서고의 학문. 글자와 진의 <b>꼴</b>은 Ⅱ부 도감에 그림으로 있다.</div></div>
${lang.html}

<div class="parthead" id="p2"><h2>Ⅱ부 — 살란 문자도감 (꼴의 정본)</h2>
<div class="sub">닿소리 스물둘·홀소리 열·결표 여섯부터 진·술식·부적·문신까지 — Ⅰ부 규칙이 실제로 어떤 <b>모양</b>인가.</div></div>
${dogamBody}

<div class="parthead" id="p3"><h2>Ⅲ부 — 회로와 기술 (다섯 길과 조합의 셈)</h2>
<div class="sub">회로 다섯 길(몸·소리·그림·매개·비는)·조련의 법·일곱 값·기술첩·병서·그릇의 셈.</div></div>
${(() => {
  const t = skTrans.tally || {};
  const rows = [['언령',t.언령],['진·술식',t.진],['연공',t.연공],['주술',t.주술],['신앙',t.신앙],['겹 얹기',t.겹],['아이템',t.아이템],['연공서',t.연공서],['기타',t.기타]];
  const max = Math.max(1, ...rows.map(r => r[1] || 0));
  const bars = rows.map(([k,v]) => `<div style="display:flex;align-items:center;gap:8px;margin:3px 0">
    <span style="flex:0 0 128px;font-size:.82em;color:#5a4f3a">${k}</span>
    <span style="flex:1;background:#e0d6bd;border-radius:5px;height:15px;position:relative">
      <span style="display:block;height:15px;width:${Math.round((v||0)/max*100)}%;background:#8a7a5c;border-radius:5px"></span></span>
    <span style="flex:0 0 42px;text-align:right;font-size:.82em;color:#4a3b2a">${v||0}</span></div>`).join('');
  return `<div class="panel" style="background:#f3ecda;border:1px solid #c9bc9c;border-radius:10px;padding:14px 16px;margin:10px 0 6px">
    <div style="font-weight:bold;color:#5a4f3a;margin-bottom:6px">📊 기술 카드 도감 — 방식별 (총 <b>${skTrans.count}</b>장 · 이 중 전투 <b>${skTrans.tally.전투}</b>장 · 자동 집계)</div>
    ${bars}
    <div style="font-size:.76em;color:#6b6353;margin-top:6px">포켓몬 도감식 — 기술 하나하나에 그림·효과·습득처·습득조건·기능방식. 재생성 <code>node _build_살란_전서.js</code></div></div>`;
})()}
${circ.html}

<div class="parthead" id="p4"><h2>Ⅳ부 — 강대국 세계사 (이만 년의 정본)</h2>
<div class="sub">다섯 대륙 · 여섯 시대 · 가문과 영웅 · 보물과 신화 — 세계가 굴러온 이만 년 전문.</div></div>
${hist.html}

</main>
</div>
<div class="top" onclick="window.scrollTo({top:0,behavior:'smooth'})">↑</div>
</body>
</html>
`;

fs.writeFileSync(OUT, page, 'utf8');
console.log('built:', OUT, Math.round(page.length/1024)+'KB',
  '| Ⅰ부 h:', lang.toc.length, '| 도감 절:', dogamToc.length, '| Ⅲ부 h:', circ.toc.length);
