/* ═══════════════════════════════════════════════════════════════
   _md_to_page.js — 세계관 정본 .md → 설정집 위성 HTML(세계관_페이지_*.html)
   목적: 확장 진단 등에서 쏟아지는 정본 md를 손 변환 없이 위성 페이지로.
        위성 페이지는 _build_map_docs.js가 세계지도 드로어로 추출한다.
   사용: node _md_to_page.js
   원칙: 본문 무손실. md 문법만 페이지 HTML 클래스(h2.part/h3/.warn/ul)로 옮긴다.
   규칙 대응: ## → h2.part, ### → h3, > → .warn, - → ul/li,
             "1. " → <p>(명제 스타일), **bold** → <b>, `code` → <code>.
   ═══════════════════════════════════════════════════════════════ */
"use strict";
const fs = require("fs");
const path = require("path");
const DIR = path.join(__dirname, '..', '..');
const WEB = path.join(DIR, "웹");   // 열람용 html 출력처 (2026-07-25 루트 → 웹/ 이동)

/* 변환 대상 (정본 md → 위성 html · 제목 이모지) */
const JOBS = [
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_탐험과_여백_통사.md",  html:"세계관_페이지_탐험여백.html",   emoji:"🧭" },
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_전쟁사_통사.md",       html:"세계관_페이지_전쟁사.html",     emoji:"⚔️" },
  { md:"기획/01_세계관/4_체계/능력과_마력/세계관_능력이_바꾼_물질사.md", html:"세계관_페이지_능력물질사.html", emoji:"⚒️" },
  { md:"기획/01_세계관/4_체계/능력과_마력/세계관_바깥의_힘.md",          html:"세계관_페이지_바깥의힘.html",   emoji:"🐎" },
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_성별과_권력.md",        html:"세계관_페이지_성별과권력.html", emoji:"⚖️" },
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_믿음은_어떻게_퍼졌나.md", html:"세계관_페이지_믿음의확산.html", emoji:"🕊️" },
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_제도의_구멍.md",         html:"세계관_페이지_제도의구멍.html", emoji:"📒" },
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_사회의_금.md",           html:"세계관_페이지_사회의금.html",   emoji:"🧵" },
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_사상의_다툼.md",         html:"세계관_페이지_사상의다툼.html", emoji:"⚖" },
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_역사의_실수.md",         html:"세계관_페이지_역사의실수.html", emoji:"🎲" },
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_문명의_여럿.md",         html:"세계관_페이지_문명의여럿.html", emoji:"🌍" },
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_신화와_전승.md",         html:"세계관_페이지_신화와전승.html", emoji:"🐉" },
  { md:"기획/01_세계관/4_체계/유물과_사물/세계관_전설의_물건들.md",       html:"세계관_페이지_전설의물건들.html", emoji:"👑" },
  { md:"기획/01_세계관/4_체계/언어와_지명/세계관_지명_사전.md",           html:"세계관_페이지_지명사전.html",    emoji:"🗺️" },
  { md:"기획/01_세계관/4_체계/언어와_지명/세계관_에오라말_어휘집.md",     html:"세계관_페이지_에오라말.html",    emoji:"🗣️" },
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_역병_연대기.md",         html:"세계관_페이지_역병연대기.html",  emoji:"🕯️" },
  { md:"기획/01_세계관/4_체계/사회와_문화/세계관_혼인과_외교_통사.md",    html:"세계관_페이지_혼인외교.html",    emoji:"🤝" },
];

function esc(s){ return s.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;"); }
/* 인라인: 텍스트 이스케이프 후 **굵게**·`코드` 마커만 태그로 */
function inline(s){
  return esc(s)
    .replace(/\*\*([^*]+)\*\*/g, "<b>$1</b>")
    .replace(/`([^`]+)`/g, "<code>$1</code>");
}

function convert(mdText){
  const lines = mdText.split(/\r?\n/);
  let i = 0, title = "", out = [], listBuf = [], warnBuf = [];
  const flushList = () => { if(listBuf.length){ out.push("<ul>\n"+listBuf.join("\n")+"\n</ul>"); listBuf=[]; } };
  const flushWarn = () => { if(warnBuf.length){ out.push('<div class="warn">'+warnBuf.join("<br>")+"</div>"); warnBuf=[]; } };
  const flushAll = () => { flushList(); flushWarn(); };

  for(; i<lines.length; i++){
    let ln = lines[i];
    const t = ln.trim();
    if(t === ""){ continue; }                          // 빈 줄=블록 경계(리스트/워른은 유지)
    if(t === "---"){ flushAll(); continue; }            // 구분선 = 드롭
    if(t.startsWith("# ")){ title = t.slice(2).trim(); flushAll(); continue; }
    // 인용(>) = warn 박스 (연속 누적)
    if(t.startsWith(">")){ flushList(); const c=t.replace(/^>\s?/,""); if(c!=="") warnBuf.push(inline(c)); continue; }
    flushWarn();
    if(t.startsWith("## ")){ flushList(); out.push('<h2 class="part">'+inline(t.slice(3).trim())+"</h2>"); continue; }
    if(t.startsWith("### ")){ flushList(); out.push("<h3>"+inline(t.slice(4).trim())+"</h3>"); continue; }
    if(t.startsWith("- ")){ listBuf.push("<li>"+inline(t.slice(2).trim())+"</li>"); continue; }
    // 표(|셀|셀|) — 헤더 다음 구분줄(|---|)은 드롭
    if(t.startsWith("|")){
      flushAll();
      const rows = [];
      for(; i<lines.length; i++){
        const rt = lines[i].trim();
        if(!rt.startsWith("|")) { i--; break; }
        if(/^[\s|:-]+$/.test(rt)) continue;
        const cells = rt.replace(/^\|/,"").replace(/\|$/,"").split("|").map(c=>inline(c.trim()));
        rows.push(cells);
      }
      if(rows.length){
        const head = "<tr>"+rows[0].map(c=>"<th>"+c+"</th>").join("")+"</tr>";
        const body = rows.slice(1).map(r=>"<tr>"+r.map(c=>"<td>"+c+"</td>").join("")+"</tr>").join("\n");
        out.push('<div class="tbl-wrap"><table>'+head+(body?"\n"+body:"")+"</table></div>");
      }
      continue;
    }
    const num = t.match(/^(\d+)\.\s+(.*)$/);            // "1. " = 명제 스타일 <p>
    if(num){ flushList(); out.push("<p>"+num[1]+". "+inline(num[2])+"</p>"); continue; }
    flushList();                                        // 일반 문단
    out.push("<p>"+inline(t)+"</p>");
  }
  flushAll();
  return { title, body: out.join("\n") };
}

const CSS = `  :root{
    --bg:#0a141b; --surface:#161d23; --surface-2:#1c242b;
    --ink:#ece5d4; --ink-soft:#cbc3b1; --muted:#8f8877;
    --line:#2b343c;
    --accent:#d3b064; --accent-deep:#ecd79a; --accent-soft:rgba(211,176,100,.10);
    --serif:'Nanum Myeongjo','Gowun Batang','Batang','바탕',serif;
  }
  *{ box-sizing:border-box; }
  body{ background:var(--bg); color:var(--ink); margin:0; padding:0;
        font-family:'Pretendard','Malgun Gothic',system-ui,-apple-system,sans-serif; -webkit-font-smoothing:antialiased; }
  .page{ max-width:800px; margin:0 auto; background:var(--surface);
         border-left:1px solid var(--line); border-right:1px solid var(--line);
         min-height:100vh; padding:40px 56px 72px; font-size:16.5px; line-height:1.9; color:var(--ink-soft); }
  h1{ font-family:var(--serif); font-size:29px; font-weight:800; color:var(--ink); margin:6px 0 14px; letter-spacing:-.01em; line-height:1.3; }
  h2.part{ font-family:var(--serif); margin:36px 0 8px; font-size:21px; font-weight:800; color:var(--ink); }
  h3{ font-family:var(--serif); margin:30px 0 10px; font-size:19px; font-weight:700; color:var(--accent-deep); border-bottom:1px solid var(--line); padding-bottom:5px; }
  p{ margin:0 0 17px; } b{ color:var(--ink); font-weight:700; }
  ul{ margin:0 0 17px; padding-left:22px; } li{ margin:0 0 10px; }
  code{ background:var(--surface-2); border:1px solid var(--line); border-radius:5px; padding:1px 6px; font-size:14px; color:var(--accent-deep); }
  .warn{ background:var(--accent-soft); border:1px solid rgba(211,176,100,.28); border-left:3px solid var(--accent);
         border-radius:10px; padding:13px 18px; font-size:14px; line-height:1.75; margin:0 0 18px; color:var(--ink-soft); }
  .warn b{ color:var(--accent-deep); }
  .tbl-wrap{ overflow-x:auto; margin:0 0 18px; }
  table{ border-collapse:collapse; width:100%; font-size:14.5px; line-height:1.6; }
  th,td{ border:1px solid var(--line); padding:8px 12px; text-align:left; vertical-align:top; }
  th{ background:var(--surface-2); color:var(--accent-deep); font-weight:700; white-space:nowrap; }
  .nav{ font-size:13.5px; margin:0 0 16px; padding:0 0 12px; border-bottom:1px solid var(--line); }
  .nav a{ color:var(--accent-deep); text-decoration:none; margin-right:16px; font-weight:600; }
  @media (max-width:820px){ .page{ padding:26px 20px; } }`;

function pageHtml(emoji, title, body){
  const full = emoji + " " + title;
  return `<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="UTF-8">
<title>${full}</title>
<style>
${CSS}
</style>
</head>
<body>
<div class="page">
<div class="nav"><a href="세계관_세계지도.html">🗺️ 세계지도</a></div>
<h1>${full}</h1>
${body}

</div>
</body>
</html>
`;
}

for(const j of JOBS){
  const md = fs.readFileSync(path.join(DIR, j.md), "utf8");
  const { title, body } = convert(md);
  const html = pageHtml(j.emoji, title, body);
  fs.writeFileSync(path.join(WEB, j.html), html, "utf8");
  const kb = (Buffer.byteLength(html,"utf8")/1024).toFixed(1);
  console.log("✓ " + j.html + "  (" + kb + "KB) — " + j.emoji + " " + title.slice(0,40));
}
console.log("완료: " + JOBS.length + "개 위성 페이지 생성. 다음: node _build_map_docs.js");
