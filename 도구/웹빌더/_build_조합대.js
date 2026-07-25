// 살란 조합대 v2 — 부품 카드로 이능 짓기 (사용자 2026-07-18)
// 발현방식(살란·문자·문양·회로·몸) + 내용 조각(부름말·꼴·크기·과녁·재는말·맺음말·부위)을 놓으면
// 그 조합으로 지금 발현 가능한 기술이 실시간 리스트로 뜬다.
// ★같은 내용 조각도 방식 카드를 바꾸면 다른 기술(언령↔진), 방식을 둘 얹으면 겹 기술.
const fs = require('fs');
const SRC = 'C:/Secret_Project/기획/01_세계관/4_체계/능력과_마력/세계관_회로와_기술_체계.md';
const OUT = 'C:/Secret_Project/웹/살란_조합대.html';
const md = fs.readFileSync(SRC, 'utf8');

const ELEM = { 자르:'불', 시스:'서리', 후르:'바람', 마이:'물', 켈:'돌·땅', 실란:'빛', 무네:'그늘', 잔:'소리', 람:'묶음', 루:'고요', 옴:'무게', 카르:'깨짐', 오르:'열림', 니르:'아묾', 코르:'닫힘', 두르:'벽' };
const SHAPE = { '케 카나':'여문 직격', '케 렌':'실처럼 조임', '케 마이':'물처럼 스밈', '케 후르':'흩음', '케 두르':'벽처럼' };
const SIZE  = { '도르':'솟는 대', '가르':'집채', '수린':'들판', '킨 도르':'긴 대' };
const TARGET= { '모 이':'한 점', '모 이 하':'여럿', '모 네 이':'한 사람', '모 안':'제 둘레', '모 하':'여럿·넓게', '누 안':'아군 비껴' };
const TIME  = { '엔':'숨 한 번', '미엔':'노래 한 곡', '사란':'해 질 때까지', '핀':'한 순간' };
const ENDKO = { '타':'세상에 넘김', '사':'마력이 붙듦', '노':'세운 것을 허묾' };
// 발현방식(길) — 캐논: 그림길=글 진(룬 갈래, 문장 있음)+술식(문장 없는 순수 회로)
const METHOD = { 살란:'언령 · 소리로 뿜음', 문자:'룬 · 글 진(살란 글자 새김)', 문양:'술식 · 문장 없는 순수 회로', 회로:'주술 · 매개로 닿음', 몸:'연공 · 몸에 낸 길' };
const HUE = { 방식:'#a13b2e', 부름말:'#8a5a3a', 꼴:'#4d6a7a', 크기:'#6a7a4d', 과녁:'#6a5f7a', 재는말:'#6b6353', 맺음말:'#7a4a4a', 부위:'#7a5a3a', 매개물:'#4d4a5a' };

// 매개물 캐논화(닿음/닮음/파훼 괄호 → 부품)
function medMaterial(desc){
  if (/박힌/.test(desc)) return '박힌 살';
  if (/피/.test(desc)) return '흘린 피';
  if (/흘린|수건|머리카락|겉옷/.test(desc)) return '흘리고 간 것';
  if (/밟은 흙|흙/.test(desc)) return '밟은 흙';
  if (/신 /.test(desc) || /신$|신\b/.test(desc)) return '오래 신은 신';
  if (/자루|고삐|쥔/.test(desc)) return '오래 쥔 것';
  if (/젖니|한 몸이었|한 상에서|혼례|옷고름/.test(desc)) return '한 몸이었던 것';
  if (/반쪽|토막|밧줄/.test(desc)) return '한 물건의 반쪽';
  if (/군단|군기/.test(desc)) return '한 군단이 지고 다닌 것';
  if (/재/.test(desc)) return '남은 재';
  if (/꼴|문지방|경계/.test(desc)) return '닮은 꼴';
  return '인연 매개';
}

// 부위(몸길) 캐논 토큰
const PARTS = ['손','발','어깨','등','눈','귀','피','숨','무릎','팔','목','살갗','허리','정강이','주먹','발꿈치','목덜미','손날'];
const PART_CANON = { 발꿈치:'발', 정강이:'무릎', 목덜미:'목', 손날:'손', 손끝:'손', 주먹:'손', 등:'어깨' };
function bodyPart(spot){
  for (const p of PARTS) if (spot.includes(p)) return PART_CANON[p] || p;
  return '몸통';
}

function classify(tok){
  if (ELEM[tok]) return { cat:'부름말', tok, ko:ELEM[tok] };
  if (SHAPE[tok]) return { cat:'꼴', tok, ko:SHAPE[tok] };
  if (SIZE[tok]) return { cat:'크기', tok, ko:SIZE[tok] };
  if (TARGET[tok]) return { cat:'과녁', tok, ko:TARGET[tok] };
  if (TIME[tok]) return { cat:'재는말', tok, ko:TIME[tok] };
  const end = tok.slice(-1), stem = tok.slice(0,-1);
  if (ENDKO[end] && ELEM[stem]) return { cat:'맺음말', tok:'-'+end, ko:ENDKO[end], elem:stem };
  return null;
}
function tokensOf(sentBody){ // "자르 · 케 카나 · ..." (에 뒤)
  const toks = sentBody.split(' · ').map(t=>t.replace(/^에\s+/,'').trim());
  const recipe = new Set(), parts = [];
  for (const t of toks){
    const c = classify(t); if (!c) continue;
    if (c.cat==='맺음말'){ recipe.add('부름말:'+c.elem); recipe.add('맺음말:'+c.tok); parts.push('맺음말:'+c.tok); }
    else { recipe.add(c.cat+':'+c.tok); parts.push(c.cat+':'+c.tok); }
  }
  return { recipe:[...recipe], parts };
}
function firstEffect(rest){
  let eff = rest.split(/\*\*(?:카드값|익힘)/)[0].trim();
  const em = eff.match(/^(.*?[다까음]\.)/); if (em) eff = em[1];
  return eff.replace(/\*\*/g,'').trim();
}

// ── 라인별 파싱(구역 추적) ──
const techs = [];
let sec = '';
for (const line of md.split(/\r?\n/)){
  let m;
  if ((m = line.match(/^### 5-([\d-]+)\./))) { sec = m[1]; continue; }
  const jinSec = /^15(-|$)/.test(sec), bodySec = /^14(-|$)/.test(sec), gapSec = /^17(-|$)/.test(sec), medSec = /^16(-|$)/.test(sec);

  // 소리(언령) — *에 TOKENS* · 소리 · T#
  if ((m = line.match(/^- \*\*「(.+?)」\*\* — \*에 ([^*]+?)\* · ([^·]*소리[^·]*) · (T\d+)[^—]*— (.+)$/)) && !jinSec) {
    const { recipe, parts } = tokensOf(m[2]);
    techs.push({ name:m[1], method:'살란', mko:'언령', tier:m[4], effect:firstEffect(m[5]), recipe:['방식:살란',...recipe], parts });
    continue;
  }
  // 글 진(문자·룬) — *에 TOKENS* (…) · … (§5-15·5-15-1) — 살란 문장을 글자로 새긴 진
  if (jinSec && (m = line.match(/^- \*\*「(.+?)」\*\* — \*에 ([^*]+?)\*\s*(?:\([^)]*\))?\s*·\s*(.+)$/))) {
    const { recipe, parts } = tokensOf(m[2]);
    let eff = m[3].split(/\*\*익힘/)[0];
    const seg = eff.split(/ · | — /).map(s=>s.trim()).filter(Boolean);
    const effSeg = seg.reverse().find(s=>/[다까음]\.?$/.test(s)) || seg[0] || '';
    techs.push({ name:m[1], method:'문자', mko:'룬', tier:'룬', effect:effSeg.replace(/\*\*/g,'').replace(/[·—]$/,'').trim()+(/[다까음]$/.test(effSeg)?'.':''), recipe:['방식:문자',...recipe], parts });
    continue;
  }
  // 술식(문양) — 살란 문장 없음 → 문장 없는 순수 회로. 술식꼴(역할)로 갈린다.
  if (jinSec && (m = line.match(/^- \*\*「(.+?)」\*\* — 살란 문장 없음[^·]*·\s*(.+)$/))) {
    const seg = m[2].split(/\*\*익힘/)[0].split(/ · | — /).map(s=>s.trim()).filter(Boolean);
    const effSeg = seg.find(s=>/[다까음]\.?$/.test(s)) || seg.find(s=>/방벽|막이|도는|덮|받/.test(s)) || seg[seg.length-1] || '';
    const txt = m[1]+' '+effSeg;
    let role = '구조';
    if (/방패|받아치|되튕|방벽/.test(txt)) role='방벽';
    else if (/버팀|떠받치|버티/.test(txt)) role='버팀';
    else if (/가둠|둘러|못 넘|못 나가/.test(txt)) role='가둠';
    else if (/북|울리|알린|경보/.test(txt)) role='경보';
    else if (/빗장|잠그|봉인|봉하/.test(txt)) role='봉쇄';
    else if (/저울|눌리|이어|이음|전한|연동/.test(txt)) role='연동';
    else if (/물레|풀무|동력|돌린다/.test(txt)) role='동력';
    else if (/닻|물막이|방파제/.test(txt)) role='물막이';
    else if (/거둠|모으|마당/.test(txt)) role='살림';
    techs.push({ name:m[1], method:'문양', mko:'술식', tier:'술식', effect:effSeg.replace(/\*\*/g,'').trim(), recipe:['방식:문양','술식꼴:'+role], parts:['술식꼴:'+role] });
    continue;
  }
  // 몸길(몸) — name — spot · T# — effect
  if (bodySec && (m = line.match(/^- \*\*「(.+?)」\*\* — (.+?) · (T\d+)[^—]*— (.+)$/))) {
    const part = bodyPart(m[2]);
    techs.push({ name:m[1], method:'몸', mko:'연공', tier:m[3], effect:firstEffect(m[4]), recipe:['방식:몸','부위:'+part], parts:['부위:'+part] });
    continue;
  }
  // 겹 — name — A+B · T# — effect
  if (gapSec && (m = line.match(/^- \*\*「(.+?)」\*\* — ([가-힣]+)\+([가-힣]+)[^·]* · (T\d+)[^—]*— (.+)$/))) {
    const map = { 소리:'살란', 몸:'몸', 그림:'문양', 매개:'회로', 글:'문자' };
    const a = map[m[2]]||m[2], b = map[m[3]]||m[3];
    techs.push({ name:m[1], method:'겹', mko:'겹 '+m[2]+'+'+m[3], tier:m[4], effect:firstEffect(m[5]), recipe:['방식:'+a,'방식:'+b], parts:['방식:'+a,'방식:'+b] });
    continue;
  }
  // 매개(회로) — name — 닿음/닮음/파훼(…) · T# — effect · 매개물 부품으로 갈린다
  if (medSec && (m = line.match(/^- \*\*「(.+?)」\*\* — ([^·]+(?:·[^·]+)*?) · (T\d+)[^—]*— (.+)$/))) {
    const mat = medMaterial(m[2]);
    techs.push({ name:m[1], method:'회로', mko:'주술', tier:m[3], effect:firstEffect(m[4]), recipe:['방식:회로','매개물:'+mat], parts:['매개물:'+mat] });
    continue;
  }
}

// 팔레트
const palette = { 방식:new Map() };
for (const k of Object.keys(METHOD)) palette.방식.set(k, '방식:'+k);
for (const t of techs) for (const r of t.recipe){
  const [cat, ...rest] = r.split(':'); const tok = rest.join(':');
  if (cat==='방식') continue;
  (palette[cat] = palette[cat] || new Map()).set(tok, r);
}
const catOrder = ['방식','부름말','꼴','크기','과녁','재는말','맺음말','부위','매개물','술식꼴'];
const paletteData = catOrder.filter(c=>palette[c]).map(cat=>({
  cat,
  items:[...palette[cat].keys()].map(tok=>{
    let ko='';
    if (cat==='방식') ko=METHOD[tok];
    else if (cat==='부름말') ko=ELEM[tok];
    else if (cat==='꼴') ko=SHAPE[tok];
    else if (cat==='크기') ko=SIZE[tok];
    else if (cat==='과녁') ko=TARGET[tok];
    else if (cat==='재는말') ko=TIME[tok];
    else if (cat==='맺음말') ko=ENDKO[tok.slice(-1)];
    else if (cat==='부위') ko='몸의 자리';
    else if (cat==='매개물') ko='주술의 매개';
    else if (cat==='술식꼴') ko='술식의 역할';
    return { tok, ko, id:cat+':'+tok };
  })
}));
const DATA = JSON.stringify({ techs, palette:paletteData, hue:HUE });

const html = `<!doctype html><html lang="ko"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>살란 조합대 — 부품 카드로 이능 짓기</title>
<style>
:root{--bg:#f3ecdc;--ink:#3d382e;--card:#faf6ec;--line:#c9bc9c;--accent:#8a5a2a}
@media(prefers-color-scheme:dark){:root{--bg:#221f1a;--ink:#e7ddc8;--card:#2b2721;--line:#4a4436;--accent:#c79a5a}}
*{box-sizing:border-box}
body{margin:0;font-family:'Malgun Gothic',system-ui,sans-serif;background:var(--bg);color:var(--ink);line-height:1.5}
header{padding:14px 18px;border-bottom:1px solid var(--line)}
header h1{margin:0;font-size:1.15em}
header p{margin:4px 0 0;font-size:.82em;opacity:.75}
.wrap{display:grid;grid-template-columns:1.15fr .85fr;min-height:calc(100vh - 62px)}
@media(max-width:820px){.wrap{grid-template-columns:1fr}}
.left{padding:14px 16px;border-right:1px solid var(--line)}
.right{padding:14px 16px;background:color-mix(in srgb,var(--card) 55%,transparent)}
.catrow{margin:0 0 12px}
.cathead{font-size:.72em;font-weight:bold;letter-spacing:.03em;opacity:.7;margin:0 0 5px}
.chips{display:flex;flex-wrap:wrap;gap:6px}
.chip{cursor:pointer;user-select:none;border:1.5px solid var(--line);background:var(--card);border-radius:9px;padding:5px 9px;display:flex;align-items:center;gap:7px;transition:.12s}
.chip:hover{border-color:var(--accent)}
.chip.on{border-color:var(--accent);box-shadow:0 0 0 2px color-mix(in srgb,var(--accent) 35%,transparent) inset}
.chip.method{border-width:2px}
.chip.method.on{background:color-mix(in srgb,#a13b2e 14%,var(--card))}
.chip b{font-size:.9em}
.chip small{font-size:.72em;opacity:.62}
.placed{position:sticky;top:8px;background:var(--card);border:1.5px dashed var(--line);border-radius:12px;padding:10px 12px;margin-bottom:14px}
.placed h2{margin:0 0 7px;font-size:.82em;opacity:.8}
.placed .empty{font-size:.82em;opacity:.5;padding:4px 0}
.btnrow{margin-top:8px;display:flex;gap:8px;flex-wrap:wrap}
button{cursor:pointer;font:inherit;font-size:.78em;border:1px solid var(--line);background:transparent;color:var(--ink);border-radius:8px;padding:4px 10px}
button:hover{border-color:var(--accent)}
.count{font-size:.8em;opacity:.75;margin:0 0 8px;font-weight:bold}
.tech{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:9px 12px;margin-bottom:8px}
.tech.hot{border-color:var(--accent)}
.tech .tn{font-weight:bold;font-size:.98em}
.tech .mb{font-size:.66em;border-radius:6px;padding:0 6px;margin-left:6px;color:#fff}
.tech .tt{font-size:.7em;color:#a13b2e;border:1px solid #a13b2e;border-radius:7px;padding:0 5px;margin-left:5px}
.tech .te{font-size:.85em;opacity:.85;margin-top:3px}
.tech .tk{font-size:.68em;opacity:.5;margin-top:4px}
.near{opacity:.5}
.hint{font-size:.8em;opacity:.6;margin-top:10px;line-height:1.6}
</style></head><body>
<header>
<h1>살란 조합대 <span style="font-weight:normal;opacity:.6;font-size:.8em">— 부품 카드를 놓으면 발현 가능한 이능이 뜬다</span></h1>
<p>먼저 <b>발현 방식</b>(언령·룬·진·주술·연공)을 고르고, <b>내용 조각</b>(부름말·꼴·과녁·맺음말·부위)을 놓아 보라. 같은 조각도 방식을 바꾸면 다른 기술이 나오고, 방식을 둘 얹으면 겹 기술이 뜬다.</p>
</header>
<div class="wrap">
<div class="left" id="palette"></div>
<div class="right">
<div class="placed"><h2>놓은 부품 <span id="pn" style="opacity:.6"></span></h2><div class="chips" id="placed"></div>
<div class="btnrow"><button onclick="clearAll()">전부 치우기</button><button onclick="showNear=!showNear;render()">한 조각 모자란 것도 보기</button></div></div>
<div class="count" id="count"></div>
<div id="list"></div>
</div>
</div>
<script>
const D = ${DATA};
const MHUE = { 살란:'#96702e', 문자:'#6b5d49', 문양:'#6a5f7a', 회로:'#4d4a5a', 몸:'#7a5a3a', 겹:'#8a6a8a' };
const placed = new Set();
let showNear = false;
function rune(tok, hue, size){
  let h=0; for(const c of tok) h=(h*31+c.charCodeAt(0))>>>0;
  const cx=size/2, cy=size/2, s=size/34;
  let out='<line x1="'+cx+'" y1="'+(cy-13*s)+'" x2="'+cx+'" y2="'+(cy+13*s)+'" stroke="'+hue+'" stroke-width="'+(2.2*s)+'"/>';
  const n=2+(h%3);
  for(let i=0;i<n;i++){
    const y=cy-9*s + i*(18*s)/(n>1?n-1:1);
    const dir=((h>>(i*2))&1)?1:-1, len=(5+((h>>(i*3))%6))*s, drop=((((h>>i)%5)-2))*2*s;
    out+='<line x1="'+cx+'" y1="'+y.toFixed(1)+'" x2="'+(cx+dir*len).toFixed(1)+'" y2="'+(y+drop).toFixed(1)+'" stroke="'+hue+'" stroke-width="'+(1.8*s)+'"/>';
    if((h>>i)&1) out+='<circle cx="'+(cx+dir*len).toFixed(1)+'" cy="'+(y+drop).toFixed(1)+'" r="'+(1.5*s)+'" fill="'+hue+'"/>';
  }
  if(h&1) out+='<path d="M'+cx+','+(cy+13*s)+' q'+(5*s)+','+(4*s)+' '+(10*s)+',0" stroke="'+hue+'" stroke-width="'+(1.8*s)+'" fill="none"/>';
  return '<svg width="'+size+'" height="'+size+'" viewBox="0 0 '+size+' '+size+'">'+out+'</svg>';
}
function chip(it, cat){
  const hue = cat==='방식' ? (MHUE[it.tok]||'#a13b2e') : (D.hue[cat]||'#777');
  return '<div class="chip'+(cat==='방식'?' method':'')+(placed.has(it.id)?' on':'')+'" onclick="toggle(\\''+it.id+'\\')">'+
    rune(it.tok, hue, 24)+'<span><b>'+it.tok+'</b> <small>'+(it.ko||'')+'</small></span></div>';
}
function toggle(id){ placed.has(id)?placed.delete(id):placed.add(id); render(); }
function clearAll(){ placed.clear(); render(); }
function catDesc(c){return{방식:'어떻게 뿜을까(길)',부름말:'무엇을(속성)',꼴:'어떻게(형태·조임)',크기:'얼마나',과녁:'어디에',재는말:'언제까지',맺음말:'어찌 맺나',부위:'몸의 어디',매개물:'무엇을 매개로',술식꼴:'술식의 역할'}[c]||'';}
function render(){
  document.getElementById('palette').innerHTML = D.palette.map(g=>
    '<div class="catrow"><div class="cathead">'+g.cat+' — '+catDesc(g.cat)+'</div><div class="chips">'+
    g.items.map(it=>chip(it,g.cat)).join('')+'</div></div>').join('');
  const pl=[...placed];
  document.getElementById('pn').textContent = pl.length? '('+pl.length+')':'';
  document.getElementById('placed').innerHTML = pl.length? pl.map(id=>{
    const cat=id.split(':')[0], tok=id.split(':').slice(1).join(':');
    return chip({id,tok,ko:''},cat);
  }).join('') : '<div class="empty">아직 놓은 부품이 없다 — 발현 방식부터 골라 보라.</div>';
  const hits=[], near=[];
  for(const t of D.techs){
    const miss=t.recipe.filter(r=>!placed.has(r));
    if(miss.length===0 && placed.size>0) hits.push(t);
    else if(showNear && miss.length===1 && placed.size>0) near.push({t,miss:miss[0]});
  }
  hits.sort((a,b)=> a.recipe.length-b.recipe.length || String(a.tier).localeCompare(String(b.tier)));
  document.getElementById('count').textContent =
    placed.size===0? '부품을 놓으면 발현 가능한 이능이 여기 뜬다.' :
    '발현 가능한 이능 '+hits.length+'가지'+(showNear? ' · 한 조각 모자란 것 '+near.length+'가지':'');
  let out = hits.map(t=>techCard(t,false)).join('');
  if(showNear) out += near.map(o=>techCard(o.t,true,o.miss)).join('');
  document.getElementById('list').innerHTML = out ||
    (placed.size>0? '<div class="hint">이 조합으로 완성되는 이능이 없다.<br>세움-부름-꼴-과녁-맺음을 갖추거나, 발현 방식을 바꿔 보라(같은 조각 + 진 = 함정, + 다른 방식 = 겹).</div>':'');
}
function techCard(t, isNear, miss){
  const mh = MHUE[t.method]||'#a13b2e';
  return '<div class="tech'+(isNear?' near':' hot')+'"><div class="tn">「'+t.name+'」'+
    '<span class="mb" style="background:'+mh+'">'+t.mko+'</span>'+
    (t.tier&&t.tier!=='진'?'<span class="tt">'+t.tier+'</span>':'')+'</div>'+
    '<div class="te">'+t.effect+'</div>'+
    '<div class="tk">'+t.parts.map(k=>k.split(':').slice(1).join(':')||k).join(' · ')+'</div>'+
    (isNear?'<div class="tk" style="color:var(--accent)">모자란 조각: '+miss.split(':').slice(1).join(':')+'</div>':'')+'</div>';
}
render();
</script>
</body></html>`;

fs.writeFileSync(OUT, html, 'utf8');
const byM = {}; techs.forEach(t=>byM[t.mko]=(byM[t.mko]||0)+1);
console.log('built:', OUT, '| 기술', techs.length, '가지', JSON.stringify(byM), '| 부품', paletteData.reduce((a,g)=>a+g.items.length,0), '종');
