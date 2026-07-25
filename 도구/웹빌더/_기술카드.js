// 기술 카드 생성기 — 회로·기술 정본(§5 기술첩)의 기술 하나하나를
// 포켓몬 도감처럼 "그림 + 해설 카드"로 바꾼다 (사용자 지시 2026-07-18 00:4x).
// 정본 md는 손대지 않는다 — 빌더가 md를 통과시키기 전에 이 모듈로 카드 토큰 치환.
// 그림은 손그림이 아니라 문장 토큰(케 렌/케 마이/수린/엔/사란…)과 설명 키워드에서
// 발현의 꼴(범위·시간·과녁)을 읽어 자동 조립한다 — 문장이 곧 스펙이라서(§4-1 슬롯).

// ── 계열(부름말)별 화풍 ──
const ELEM = {
  '자르': { ko: '불',      hue: '#b5482a', bg: '#f6e3d2' },
  '시스': { ko: '서리',    hue: '#4a7f9c', bg: '#e2edf2' },
  '후르': { ko: '바람',    hue: '#6d8f6d', bg: '#e7efe2' },
  '마이': { ko: '물',      hue: '#3f6d99', bg: '#dfe9f2' },
  '켈':   { ko: '돌·땅',   hue: '#7d6a4d', bg: '#ece4d2' },
  '실란': { ko: '빛',      hue: '#b3902e', bg: '#f7efd4' },
  '무네': { ko: '그늘',    hue: '#4d4a5a', bg: '#e0dee6' },
  '잔':   { ko: '소리',    hue: '#96702e', bg: '#f1e8d4' },
  '람':   { ko: '묶음',    hue: '#8a5a3a', bg: '#efe4d6' },
  '루':   { ko: '잠·고요', hue: '#6a6d8f', bg: '#e6e6ef' },
  '옴':   { ko: '무게',    hue: '#5f5f5f', bg: '#e6e4e0' },
  '카르': { ko: '깨짐',    hue: '#9c3a3a', bg: '#f2dede' },
  '오르': { ko: '열림',    hue: '#6f8a5a', bg: '#e8efe0' },
  '니르': { ko: '아묾',    hue: '#5a8a7a', bg: '#e0efe9' },
  '코르': { ko: '닫힘',    hue: '#6a5a4a', bg: '#e9e2d8' },
  '두르': { ko: '벽·감쌈', hue: '#5a6a7a', bg: '#e2e7ec' },
  '그림': { ko: '진·술식', hue: '#6a5f7a', bg: '#e7e2ee' },
  '겹':   { ko: '겹 얹기', hue: '#8a6a8a', bg: '#efe4ef' },
  '몸길': { ko: '연공',    hue: '#7a5a3a', bg: '#f0e6d6' },
  '매개': { ko: '주술',    hue: '#4d4a5a', bg: '#e3e0ea' },
  '신앙': { ko: '신앙',    hue: '#6a6d8f', bg: '#e6e6ef' },
};

// ── 문장 토큰 → 발현의 꼴 ──
function readSentence(sent, desc) {
  const s = sent || '', d = desc || '';
  // 시간(재는말)
  let time = '맺는 순간';
  if (s.includes('엔')  && /(^|\s|·)엔($|\s|·)/.test(s)) time = '숨 한 번';
  if (s.includes('미엔')) time = '노래 한 곡';
  if (s.includes('사란')) time = '해 질 때까지';
  // 맺음말
  let ending = '';
  const last = s.split('·').map(t => t.trim()).pop() || '';
  if (/타$/.test(last)) ending = '-타 · 세상에 넘긴다';
  else if (/사$/.test(last)) ending = '-사 · 마력이 붙든다';
  else if (/노$/.test(last)) ending = '-노 · 세운 것을 허문다';
  // 꼴(범위) — 문장 우선, 설명 보조
  let shape = 'point';
  if (s.includes('케 렌')) shape = 'line';
  if (s.includes('케 마이')) shape = 'mist';
  if (s.includes('케 두르') || /벽처럼|벽이 |보루/.test(d)) shape = 'wall';
  if (s.includes('수린') || /들판만/.test(d)) shape = 'field';
  if (s.includes('모 안') || /제 (등|몸|둘레)|술자 둘레|둘레 사람/.test(d)) shape = 'self';
  if (s.includes('모 네 이')) shape = 'one';
  return { time, ending, shape };
}

// ── 그림 조각들 ──
function motif(el, x, y, k) { // 계열 무늬 한 점 (k=크기 배율)
  const h = el.hue; k = k || 1;
  switch (el.ko) {
    case '불': return `<path d="M${x},${y} q${3*k},${-8*k} 0,${-13*k} q${5*k},${6*k} ${2*k},${13*k} z M${x+7*k},${y} q${2*k},${-6*k} 0,${-9*k} q${4*k},${4*k} ${1.5*k},${9*k} z" fill="${h}" opacity=".85"/>`;
    case '서리': return `<g stroke="${h}" stroke-width="${1.6*k}"><line x1="${x-6*k}" y1="${y}" x2="${x+6*k}" y2="${y}"/><line x1="${x}" y1="${y-6*k}" x2="${x}" y2="${y+6*k}"/><line x1="${x-4*k}" y1="${y-4*k}" x2="${x+4*k}" y2="${y+4*k}"/><line x1="${x-4*k}" y1="${y+4*k}" x2="${x+4*k}" y2="${y-4*k}"/></g>`;
    case '바람': return `<g stroke="${h}" stroke-width="${1.6*k}" fill="none"><path d="M${x-8*k},${y} q${8*k},${-5*k} ${16*k},0"/><path d="M${x-6*k},${y+5*k} q${7*k},${-4*k} ${14*k},0"/></g>`;
    case '물': return `<path d="M${x-8*k},${y} q${4*k},${-4*k} ${8*k},0 q${4*k},${4*k} ${8*k},0" stroke="${h}" stroke-width="${1.8*k}" fill="none"/>`;
    case '돌·땅': return `<g fill="${h}" opacity=".8"><rect x="${x-6*k}" y="${y-4*k}" width="${7*k}" height="${5*k}"/><rect x="${x+1*k}" y="${y-7*k}" width="${5*k}" height="${4*k}"/></g>`;
    case '빛': return `<g stroke="${h}" stroke-width="${1.4*k}"><line x1="${x}" y1="${y-7*k}" x2="${x}" y2="${y-2*k}"/><line x1="${x-5*k}" y1="${y-5*k}" x2="${x-2*k}" y2="${y-2*k}"/><line x1="${x+5*k}" y1="${y-5*k}" x2="${x+2*k}" y2="${y-2*k}"/><circle cx="${x}" cy="${y}" r="${2.2*k}" fill="${h}"/></g>`;
    case '그늘': return `<ellipse cx="${x}" cy="${y}" rx="${8*k}" ry="${4*k}" fill="${h}" opacity=".55"/>`;
    case '소리': return `<g stroke="${h}" stroke-width="${1.4*k}" fill="none"><path d="M${x},${y} q${5*k},${-5*k} 0,${-10*k}" transform="rotate(90 ${x} ${y})"/><circle cx="${x}" cy="${y}" r="${3*k}"/><circle cx="${x}" cy="${y}" r="${6.5*k}" opacity=".6"/></g>`;
    case '묶음': return `<path d="M${x-6*k},${y} q${6*k},${-6*k} ${12*k},0 q${-6*k},${6*k} ${-12*k},0 z" stroke="${h}" stroke-width="${1.6*k}" fill="none"/>`;
    case '잠·고요': return `<g stroke="${h}" stroke-width="${1.3*k}" fill="none" opacity=".8"><path d="M${x-7*k},${y} h${14*k}"/><path d="M${x-5*k},${y+4*k} h${10*k}"/></g>`;
    case '무게': return `<g stroke="${h}" stroke-width="${1.8*k}"><line x1="${x}" y1="${y-8*k}" x2="${x}" y2="${y+2*k}"/><path d="M${x-4*k},${y-2*k} L${x},${y+3*k} L${x+4*k},${y-2*k}" fill="none"/></g>`;
    case '깨짐': return `<path d="M${x-7*k},${y-5*k} l${5*k},${4*k} l${-3*k},${3*k} l${6*k},${4*k} l${2*k},${-3*k}" stroke="${h}" stroke-width="${1.8*k}" fill="none"/>`;
    case '열림': return `<g stroke="${h}" stroke-width="${1.6*k}" fill="none"><rect x="${x-5*k}" y="${y-8*k}" width="${10*k}" height="${14*k}"/><path d="M${x-5*k},${y-8*k} L${x+2*k},${y-11*k} L${x+2*k},${y+3*k} L${x-5*k},${y+6*k}" fill="${el.bg}"/></g>`;
    case '아묾': return `<g stroke="${h}" stroke-width="${1.5*k}"><path d="M${x-7*k},${y} q${7*k},${3*k} ${14*k},0" fill="none"/><line x1="${x-4*k}" y1="${y-3*k}" x2="${x-2*k}" y2="${y+3*k}"/><line x1="${x+1*k}" y1="${y-3*k}" x2="${x+3*k}" y2="${y+3*k}"/></g>`;
    case '닫힘': return `<g stroke="${h}" stroke-width="${1.7*k}" fill="none"><rect x="${x-6*k}" y="${y-7*k}" width="${12*k}" height="${12*k}"/><line x1="${x-9*k}" y1="${y}" x2="${x+9*k}" y2="${y}"/></g>`;
    default: return `<circle cx="${x}" cy="${y}" r="${4*k}" fill="${h}" opacity=".7"/>`;
  }
}
function figure(x, y, hue, aura) { // 술자/사람
  let s = '';
  if (aura) s += `<ellipse cx="${x}" cy="${y-11}" rx="13" ry="19" fill="none" stroke="${hue}" stroke-width="1.6" stroke-dasharray="3 3"/>`;
  s += `<circle cx="${x}" cy="${y-22}" r="4.5" fill="#5a4f3a"/><line x1="${x}" y1="${y-17}" x2="${x}" y2="${y-4}" stroke="#5a4f3a" stroke-width="2.4"/>`;
  s += `<line x1="${x}" y1="${y-14}" x2="${x-7}" y2="${y-8}" stroke="#5a4f3a" stroke-width="2"/><line x1="${x}" y1="${y-14}" x2="${x+8}" y2="${y-11}" stroke="#5a4f3a" stroke-width="2"/>`;
  s += `<line x1="${x}" y1="${y-4}" x2="${x-5}" y2="${y+4}" stroke="#5a4f3a" stroke-width="2"/><line x1="${x}" y1="${y-4}" x2="${x+5}" y2="${y+4}" stroke="#5a4f3a" stroke-width="2"/>`;
  return s;
}

// ── 발현 장면 (210×150) ──
function scene(el, f, bodyPart) {
  const W = 210, H = 150, gy = 118; // 지면
  let s = `<rect width="${W}" height="${H}" fill="${el.bg}" rx="10"/>`;
  s += `<line x1="8" y1="${gy}" x2="${W-8}" y2="${gy}" stroke="#a99a78" stroke-width="1.6"/>`;
  const cx = 42; // 술자 자리
  if (el.ko === '연공') {
    // 몸길: 몸 실루엣 + 터(아랫배 심)에서 그 자리까지의 마력 흐름 경로
    // (사용자 지시 2026-07-18: 신체 회로 전부가 아니라 "마력의 흐름"이 그림으로 보여야 한다)
    const bx0 = 105, hue = el.hue;
    // 실루엣 (정면)
    s += `<circle cx="${bx0}" cy="30" r="8" fill="none" stroke="#5a4f3a" stroke-width="2"/>`;
    s += `<line x1="${bx0}" y1="38" x2="${bx0}" y2="88" stroke="#5a4f3a" stroke-width="2.4"/>`;
    s += `<path d="M${bx0},48 L${bx0 - 26},74 M${bx0},48 L${bx0 + 26},74" stroke="#5a4f3a" stroke-width="2"/>`;
    s += `<path d="M${bx0},88 L${bx0 - 15},138 M${bx0},88 L${bx0 + 15},138" stroke="#5a4f3a" stroke-width="2"/>`;
    // 터·심 (아랫배)
    const core = [bx0, 82];
    s += `<circle cx="${core[0]}" cy="${core[1]}" r="4.5" fill="${hue}"/>`;
    s += `<circle cx="${core[0]}" cy="${core[1]}" r="8" fill="none" stroke="${hue}" stroke-width="1" opacity=".6"/>`;
    s += `<text x="${bx0 + 14}" y="${core[1] + 4}" font-size="8.5" fill="#6b6353">심·터</text>`;
    const flow = (dPath, dots) => {
      let t = `<path d="${dPath}" stroke="${hue}" stroke-width="2.2" fill="none" stroke-dasharray="6 3"/>`;
      dots.forEach(p => { t += `<circle cx="${p[0]}" cy="${p[1]}" r="2.6" fill="${hue}"/>`; });
      const last = dots[dots.length - 1];
      t += `<circle cx="${last[0]}" cy="${last[1]}" r="5" fill="none" stroke="${hue}" stroke-width="1.6"/>`;
      return t;
    };
    const bp = bodyPart || '';
    if (/손끝|손/.test(bp)) {
      s += flow(`M${core[0]},${core[1]} C${bx0},60 ${bx0 - 8},52 ${bx0 - 26},74`, [[bx0 - 8, 56], [bx0 - 18, 64], [bx0 - 26, 74]]);
      s += flow(`M${core[0]},${core[1]} C${bx0},60 ${bx0 + 8},52 ${bx0 + 26},74`, [[bx0 + 8, 56], [bx0 + 18, 64], [bx0 + 26, 74]]);
    } else if (/어깨|등/.test(bp)) {
      s += flow(`M${core[0]},${core[1]} C${bx0 - 4},64 ${bx0 - 6},52 ${bx0 - 12},48`, [[bx0 - 4, 66], [bx0 - 12, 48]]);
      s += flow(`M${core[0]},${core[1]} C${bx0 + 4},64 ${bx0 + 6},52 ${bx0 + 12},48`, [[bx0 + 4, 66], [bx0 + 12, 48]]);
    } else if (/발|걸음|무릎/.test(bp)) {
      s += flow(`M${core[0]},${core[1]} L${bx0},88 L${bx0 - 15},138`, [[bx0 - 6, 106], [bx0 - 11, 122], [bx0 - 15, 136]]);
      s += flow(`M${core[0]},${core[1]} L${bx0},88 L${bx0 + 15},138`, [[bx0 + 6, 106], [bx0 + 11, 122], [bx0 + 15, 136]]);
    } else if (/눈|옌|귀|귓/.test(bp)) {
      s += flow(`M${core[0]},${core[1]} L${bx0},40 L${bx0},32`, [[bx0, 64], [bx0, 46], [bx0 - 3, 29]]);
      s += `<circle cx="${bx0 + 3}" cy="29" r="1.6" fill="${hue}"/>`;
    } else if (/피|속길/.test(bp)) {
      s += flow(`M${core[0]},${core[1]} C${bx0 - 14},70 ${bx0 - 14},50 ${bx0},44 C${bx0 + 14},50 ${bx0 + 14},70 ${core[0]},${core[1]}`, [[bx0 - 13, 60], [bx0, 45], [bx0 + 13, 60], [core[0], core[1]]]);
    } else if (/숨길|숨/.test(bp)) {
      s += flow(`M${bx0},32 L${bx0},56`, [[bx0, 40], [bx0, 48], [bx0, 56]]);
      s += `<path d="M${bx0 + 10},30 q6,-2 10,2 M${bx0 + 10},34 q5,2 9,0" stroke="${hue}" stroke-width="1.2" fill="none" opacity=".7"/>`;
    } else if (/심/.test(bp)) {
      s += `<circle cx="${core[0]}" cy="${core[1]}" r="12" fill="none" stroke="${hue}" stroke-width="1.4" opacity=".7"/>`;
      s += `<circle cx="${core[0]}" cy="${core[1]}" r="17" fill="none" stroke="${hue}" stroke-width="1" opacity=".4"/>`;
    } else if (/살갗|겉길/.test(bp)) {
      s += `<path d="M${bx0},19 C${bx0 + 22},22 ${bx0 + 20},60 ${bx0 + 28},76 L${bx0 + 18},142 L${bx0 - 18},142 C${bx0 - 26},76 ${bx0 - 22},22 ${bx0},19" fill="none" stroke="${hue}" stroke-width="1.8" stroke-dasharray="4 3"/>`;
      s += `<circle cx="${bx0 + 24}" cy="60" r="2.4" fill="${hue}"/><circle cx="${bx0 - 24}" cy="60" r="2.4" fill="${hue}"/><circle cx="${bx0}" cy="20" r="2.4" fill="${hue}"/>`;
    } else {
      s += flow(`M${core[0]},${core[1]} L${bx0},50`, [[bx0, 66], [bx0, 50]]);
    }
    s += `<text x="14" y="${H - 10}" font-size="8.5" fill="#6b6353">점선 = 마력이 도는 길 · 점 = 마디</text>`;
    return { w: W, h: H, svg: s };
  }
  // 비는 길(신앙): 문장 없이 위에서 내려오는 청 — 소리 표시 대신 강림 도해 (§2-5)
  if (el.ko === '신앙') {
    const hue = el.hue, tx = 150;
    s += figure(cx, gy, hue, true);                          // 청하는 자(둘레 가호)
    s += `<path d="M18,20 q10,-8 22,-4" stroke="${hue}" stroke-width="1.4" fill="none" opacity=".5"/>`; // 성지 자취(위)
    s += `<path d="M${cx},${gy-30} C70,60 100,34 ${tx},18" stroke="${hue}" stroke-width="1.3" fill="none" stroke-dasharray="4 3"/>`; // 올리는 청
    s += `<circle cx="${tx}" cy="16" r="3" fill="${hue}"/>`;
    s += `<path d="M${tx-9},18 L${tx-13},${gy-6} M${tx},18 L${tx},${gy-4} M${tx+9},18 L${tx+13},${gy-6}" stroke="${hue}" stroke-width="2" opacity=".55"/>`; // 내려오는 기둥
    s += `<rect x="${tx-14}" y="20" width="28" height="${gy-24}" fill="${hue}" opacity=".10"/>`;
    // 못 고르는 낙점(흩어진 표) — 신앙의 무차별·변수
    s += `<text x="${tx-24}" y="${gy-8}" font-size="11" fill="${hue}" opacity=".8">✕</text>`;
    s += `<text x="${tx+14}" y="${gy-2}" font-size="9" fill="${hue}" opacity=".55">✕</text>`;
    s += `<rect x="8" y="8" width="86" height="16" rx="8" fill="#fff" opacity=".55"/><text x="51" y="20" text-anchor="middle" font-size="9.5" fill="#5a4f3a">⧗ ${f.time}</text>`;
    s += `<text x="14" y="${H-10}" font-size="8.5" fill="#6b6353">빌린 길 — 위에서 내려온다(낙점 못 고름)</text>`;
    return { w: W, h: H, svg: s };
  }
  // 매개(주술): 문장 없이 인연 실이 과녁을 문다 — 소리 표시 대신 실 도해 (§2-4)
  if (el.ko === '주술') {
    const hue = el.hue, tx = 178;
    s += figure(cx, gy, hue, false);
    s += `<circle cx="${cx+9}" cy="${gy-13}" r="3" fill="none" stroke="${hue}" stroke-width="1.4"/>`; // 실꾸리(손)
    s += `<path d="M${cx+11},${gy-13} C90,${gy-34} 150,${gy-34} ${tx-6},${gy-16}" stroke="${hue}" stroke-width="1.6" fill="none" stroke-dasharray="5 3"/>`; // 인연 실
    s += `<circle cx="108" cy="${gy-27}" r="2.2" fill="${hue}"/><circle cx="140" cy="${gy-28}" r="2.2" fill="${hue}"/>`; // 매듭
    s += figure(tx, gy, hue, false);                          // 먼 과녁
    s += `<path d="M${tx-7},${gy-24} l14,14 M${tx+7},${gy-24} l-14,14" stroke="${hue}" stroke-width="1.6" opacity=".7"/>`; // 걸림 표
    s += `<rect x="8" y="8" width="86" height="16" rx="8" fill="#fff" opacity=".55"/><text x="51" y="20" text-anchor="middle" font-size="9.5" fill="#5a4f3a">⧗ ${f.time}</text>`;
    s += `<text x="14" y="${H-10}" font-size="8.5" fill="#6b6353">인연 실 — 거리·벽 무관, 매개가 회로의 반</text>`;
    return { w: W, h: H, svg: s };
  }
  s += figure(cx, gy, el.hue, false);
  if (el.ko === '진·술식') {
    // 진(그림길): 소리가 아니라 새긴 고리 — 술자 곁에 도는 진 글리프
    const jx = 150, jy = gy - 26, hue = el.hue;
    s += `<circle cx="${jx}" cy="${jy}" r="22" fill="none" stroke="${hue}" stroke-width="1.6"/>`;
    s += `<circle cx="${jx}" cy="${jy}" r="14" fill="none" stroke="${hue}" stroke-width="1.2"/>`;
    for (let i = 0; i < 6; i++) { const a = -Math.PI/2 + i*Math.PI/3; s += `<line x1="${(jx+14*Math.cos(a)).toFixed(1)}" y1="${(jy+14*Math.sin(a)).toFixed(1)}" x2="${(jx+22*Math.cos(a)).toFixed(1)}" y2="${(jy+22*Math.sin(a)).toFixed(1)}" stroke="${hue}" stroke-width="1.2"/>`; }
    s += `<circle cx="${jx}" cy="${jy}" r="3" fill="${hue}"/>`;
    s += `<path d="M${cx+14},${gy-18} q40,-6 74,-6" stroke="${hue}" stroke-width="1.2" stroke-dasharray="4 3" fill="none"/>`;
  } else if (el.ko !== '겹 얹기') {
    // 입에서 나가는 소리 표시 — 언령 계열(부름말)만
    s += `<path d="M${cx+7},${gy-21} q6,-2 10,2" stroke="${el.hue}" stroke-width="1.2" fill="none" opacity=".7"/>`;
  }
  switch (f.shape) {
    case 'line': { // 실·줄
      s += `<line x1="${cx+14}" y1="${gy-14}" x2="${W-24}" y2="${gy-16}" stroke="${el.hue}" stroke-width="2.4"/>`;
      s += motif(el, 120, gy-24, 1); s += motif(el, 160, gy-26, .8);
      break; }
    case 'mist': { // 스밈
      s += `<ellipse cx="128" cy="${gy-22}" rx="52" ry="26" fill="${el.hue}" opacity=".14"/>`;
      s += motif(el, 108, gy-22, 1); s += motif(el, 140, gy-32, .8); s += motif(el, 152, gy-14, .9);
      break; }
    case 'wall': { // 벽
      s += `<rect x="128" y="${gy-52}" width="16" height="52" fill="${el.hue}" opacity=".28" stroke="${el.hue}" stroke-width="2"/>`;
      s += motif(el, 136, gy-58, 1);
      break; }
    case 'field': { // 들판
      s += `<rect x="${cx+22}" y="${gy-8}" width="${W-cx-40}" height="8" fill="${el.hue}" opacity=".25"/>`;
      s += motif(el, 100, gy-12, .9); s += motif(el, 135, gy-12, .9); s += motif(el, 170, gy-12, .9);
      break; }
    case 'self': { // 제 몸·둘레
      s += `<ellipse cx="${cx}" cy="${gy-12}" rx="20" ry="24" fill="${el.hue}" opacity=".12" stroke="${el.hue}" stroke-width="1.4" stroke-dasharray="4 3"/>`;
      s += motif(el, cx, gy-40, .9);
      break; }
    case 'one': { // 한 사람·한 자리
      s += figure(158, gy, el.hue, false);
      s += motif(el, 158, gy-34, 1);
      s += `<path d="M${cx+14},${gy-18} q50,-22 96,-14" stroke="${el.hue}" stroke-width="1.3" stroke-dasharray="4 3" fill="none"/>`;
      break; }
    default: { // 한 점
      s += motif(el, 140, gy-18, 1.4);
      s += `<path d="M${cx+14},${gy-18} q40,-14 84,-2" stroke="${el.hue}" stroke-width="1.3" stroke-dasharray="4 3" fill="none"/>`;
    }
  }
  // 시간 눈금
  s += `<rect x="8" y="8" width="86" height="16" rx="8" fill="#fff" opacity=".55"/>`;
  s += `<text x="51" y="20" text-anchor="middle" font-size="9.5" fill="#5a4f3a">⧗ ${f.time}</text>`;
  return { w: W, h: H, svg: s };
}

// ── 진꼴(마법진) 그림 — §5-15 그림길 기술첩 ──
// 캐논(§2-3): 글 진=살란 문장을 고리로 새김(홈 한 줄, 부름 칸에서 깨움, -사=도는 진/-타=방아쇠 진),
// 술식=겹 고리+건널목+심(갈래 중복), 부적=먹의 한 번짜리 방아쇠.
// 낱말→획은 결정적 해시 — 같은 낱말은 어느 진에서나 같은 획(같은 문장=같은 꼴).
const NEUTRAL = { ko: '술식', hue: '#6b5d49', bg: '#ece5d4' };
function tokenGlyph(tok, cx, cy, r, a0, a1, hue) {
  let h = 0; for (const c of tok) h = (h * 31 + c.charCodeAt(0)) >>> 0;
  const n = 2 + (h % 3); let s = '';
  for (let i = 0; i < n; i++) {
    const t = a0 + (a1 - a0) * ((i + 0.5) / n);
    const rr = r - 4, r2 = r - 12 - ((h >> (i * 3)) % 4);
    const bend = (((h >> (i * 2)) % 5) - 2) * 0.05;
    s += `<line x1="${(cx + rr * Math.cos(t)).toFixed(1)}" y1="${(cy + rr * Math.sin(t)).toFixed(1)}" x2="${(cx + r2 * Math.cos(t + bend)).toFixed(1)}" y2="${(cy + r2 * Math.sin(t + bend)).toFixed(1)}" stroke="${hue}" stroke-width="1.5"/>`;
    if ((h >> i) & 1) s += `<circle cx="${(cx + (r - 8) * Math.cos(t + 0.09)).toFixed(1)}" cy="${(cy + (r - 8) * Math.sin(t + 0.09)).toFixed(1)}" r="1.3" fill="${hue}"/>`;
  }
  return s;
}
function jinScene(el, sent, type, desc) {
  const W = 210, H = 150, cx = 105, cy = 78, hue = el.hue;
  let s = `<rect width="${W}" height="${H}" fill="#e9e1cd" rx="10"/>`;
  let cap = '';
  if (type === '부적') {
    s += `<g transform="rotate(2 105 75)"><rect x="79" y="12" width="52" height="120" rx="4" fill="#f6efdd" stroke="#c9bc9c" stroke-width="1.4"/></g>`;
    s += `<circle cx="106" cy="28" r="6" fill="#a13b2e" opacity=".8"/>`;
    const tok = (sent || '').trim();
    let h = 0; for (const c of tok) h = (h * 31 + c.charCodeAt(0)) >>> 0;
    const n = 3 + (h % 3);
    for (let i = 0; i < n; i++) {
      const x = 92 + ((h >> (i * 4)) % 26), y1 = 44 + i * 16, dy = 10 + ((h >> (i * 2)) % 7);
      s += `<path d="M${x},${y1} q${(((h >> i) % 5) - 2) * 2},${dy / 2} ${(((h >> (i * 3)) % 5) - 2)},${dy}" stroke="#3d382e" stroke-width="2" fill="none"/>`;
    }
    s += motif(el, 106, 122, .7);
    cap = '먹의 그림길 — 한 번짜리 방아쇠';
  } else if (type === '술식') {
    const three = /셋|세 겹/.test(desc), rings = three ? [52, 39, 26] : [52, 32];
    const spokes = /여섯/.test(desc) ? 6 : 4;
    rings.forEach((r, i) => {
      s += `<circle cx="${cx}" cy="${cy}" r="${r}" fill="none" stroke="${hue}" stroke-width="${i === rings.length - 1 ? 2 : 1.4}"${i === 0 && /틈/.test(desc) ? ' stroke-dasharray="150 14"' : ''}/>`;
    });
    for (let i = 0; i < spokes; i++) {
      const a = -Math.PI / 2 + i * 2 * Math.PI / spokes + 0.26;
      s += `<line x1="${(cx + rings[0] * Math.cos(a)).toFixed(1)}" y1="${(cy + rings[0] * Math.sin(a)).toFixed(1)}" x2="${(cx + rings[rings.length - 1] * Math.cos(a)).toFixed(1)}" y2="${(cy + rings[rings.length - 1] * Math.sin(a)).toFixed(1)}" stroke="${hue}" stroke-width="1.3"/>`;
    }
    s += `<circle cx="${cx}" cy="${cy}" r="4.5" fill="${hue}"/><circle cx="${cx}" cy="${cy}" r="9" fill="none" stroke="${hue}" stroke-width="1" opacity=".6"/>`;
    s += `<text x="${cx + 14}" y="${cy + 3}" font-size="8.5" fill="#6b6353">심</text>`;
    cap = '갈래로 돈다 — 홈 하나쯤은 딴 갈래가 산다';
  } else {
    const toks = sent.split('·').map(t => t.trim()).filter(Boolean);
    const R = 54, n = toks.length;
    s += `<circle cx="${cx}" cy="${cy}" r="${R}" fill="none" stroke="${hue}" stroke-width="2"/>`;
    s += `<circle cx="${cx}" cy="${cy}" r="${R - 16}" fill="none" stroke="${hue}" stroke-width="1" opacity=".5"/>`;
    for (let i = 0; i < n; i++) {
      const a0 = -Math.PI / 2 + i * 2 * Math.PI / n, a1 = -Math.PI / 2 + (i + 1) * 2 * Math.PI / n;
      s += `<line x1="${(cx + (R - 16) * Math.cos(a0)).toFixed(1)}" y1="${(cy + (R - 16) * Math.sin(a0)).toFixed(1)}" x2="${(cx + R * Math.cos(a0)).toFixed(1)}" y2="${(cy + R * Math.sin(a0)).toFixed(1)}" stroke="${hue}" stroke-width="1.1" opacity=".7"/>`;
      s += tokenGlyph(toks[i], cx, cy, R, a0 + 0.1, a1 - 0.1, hue);
    }
    // 부름 칸 — 손을 짚어 붓는 자리
    s += `<circle cx="${cx}" cy="${cy - R}" r="6" fill="#e9e1cd" stroke="${hue}" stroke-width="1.6"/><circle cx="${cx}" cy="${cy - R}" r="2" fill="${hue}"/>`;
    s += `<text x="${cx + 11}" y="${cy - R + 3}" font-size="8" fill="#6b6353">부름 칸</text>`;
    if (type === '도는 진') {
      s += `<circle cx="${cx}" cy="${cy}" r="${R - 28}" fill="none" stroke="${hue}" stroke-width="1.5" opacity=".85"/>`;
      cap = '홈 한 줄 — 부은 동안 돌며 붙든다(-사)';
    } else {
      s += `<line x1="${cx}" y1="${cy - R + 7}" x2="${cx}" y2="${cy - 12}" stroke="${hue}" stroke-width="1.4" stroke-dasharray="3 2"/>`;
      cap = '홈 한 줄 — 깨울 때마다 한 번 맺고 잠든다(-타)';
    }
    s += motif(el, cx, cy + 3, .8);
  }
  s += `<text x="12" y="${H - 8}" font-size="8.5" fill="#6b6353">${cap}</text>`;
  return { w: W, h: H, svg: s };
}
function jinCard(no, el, name, sent, srcNote, type, effect, learn) {
  const sc = jinScene(el, sent, type, effect);
  const chips = [];
  if (/(때리|꿰뚫|찌르|찌른|베인|박히|튀어|떨군|꿇리|꿇린|문다|터지|터뜨)/.test(effect || '')) {
    chips.push(`<span class="skc" style="background:#a13b2e;color:#fff">전투</span>`);
  }
  chips.push(`<span class="skc" style="background:${el.hue};color:#fff">${el.ko}</span>`);
  chips.push(`<span class="skc">${type === '부적' ? '부적 · 한 번짜리' : type}</span>`);
  if (srcNote) chips.push(`<span class="skc">${srcNote.split('—')[0].trim()}</span>`);
  const sentHtml = sent
    ? `<div class="sksent">${sent.split('·').map(w => `<span>${w.trim()}</span>`).join('<i>·</i>')}</div>`
    : `<div class="sksent"><span style="opacity:.65">살란 문장 없음 — 회로를 먼저 놓고 꼴을 깎은 술식</span></div>`;
  const [where, cond] = splitLearn(learn);
  const jinMech = {
    '도는 진': '그림길 — 홈 한 줄을 새기고, 부름 칸을 짚어 부은 동안 돌며 붙든다(-사)',
    '방아쇠 진': '그림길 — 홈 한 줄을 새기고, 깨울 때마다 한 번 맺고 잠든다(-타)',
    '부적': '그림길 — 먹으로 그린 한 번짜리 방아쇠(쓰면 사라짐)',
    '술식': '그림길 — 살란 문장 없이 회로째 깎은 겹 고리(갈래 중복으로 홈 하나쯤은 딴 갈래가 산다)',
  }[type] || '그림길 — 진꼴';
  const [effMain, effRest] = splitEffect(effect);
  const grid = fieldGrid([
    ['효과', effMain],
    ['기능방식', jinMech],
    ['자리', type === '부적' ? '몸에 지니거나 붙인 자리' : '새겨 둔 바닥·벽·매개'],
    ['습득처', bookOf(where)],
    ['습득조건', type === '부적' ? '부적방 도제 — 먹·꼴·족보(§2-3)' : '진 새김 저울 — 꼴의 길들임에 따라(§2-3)'],
    ['익힘', learn],
    ['풀이', effRest],
  ]);
  return `<div class="skcard">
<div class="skart"><svg width="${sc.w}" height="${sc.h}" viewBox="0 0 ${sc.w} ${sc.h}">${sc.svg}</svg><div class="skno">진 No.${String(no).padStart(2, '0')}</div></div>
<div class="skbody">
<div class="skname">「${name}」 <span class="sktier">그림길 · 진꼴</span></div>
<div class="skchips">${chips.join('')}</div>
${sentHtml}
${grid}
</div></div>`;
}

// ── 연공서(숙련도 책) 카드 — §5-21 (사용자 지시 2026-07-18 01:3x: 기술 다음은 길별 T 올리는 연공서) ──
const PATHS = {
  '소리길': { ko: '언령',   hue: '#96702e', bg: '#f1e8d4' },
  '몸길':   { ko: '연공',   hue: '#7a5a3a', bg: '#f0e6d6' },
  '그림길': { ko: '진 새김', hue: '#6b5d49', bg: '#ece5d4' },
  '매개길': { ko: '주술',   hue: '#4d4a5a', bg: '#e0dee6' },
  '비는 길': { ko: '신앙',  hue: '#6a6d8f', bg: '#e6e6ef' },
};
function bookScene(p, tier) {
  const W = 210, H = 150, hue = p.hue;
  let s = `<rect width="${W}" height="${H}" fill="${p.bg}" rx="10"/>`;
  s += `<path d="M105,26 C80,20 40,20 26,26 L26,122 C40,116 80,116 105,122 C130,116 170,116 184,122 L184,26 C170,20 130,20 105,26 z" fill="#faf6ec" stroke="#a99a78" stroke-width="1.6"/>`;
  s += `<line x1="105" y1="25" x2="105" y2="122" stroke="#a99a78" stroke-width="1.2"/>`;
  // 왼쪽 장: 길별 저울(T) 사다리 — 이 책이 올려 주는 마디를 칠한다
  const tm = tier.match(/T(\d)(?:\s*→\s*T(\d))?/);
  const tgt = tm ? (tm[2] ? +tm[2] : +tm[1]) : 1;
  for (let i = 0; i < 4; i++) {
    const x = 36 + i * 15, y = 100 - i * 15, on = (i + 1) === tgt, was = (i + 1) < tgt;
    s += `<rect x="${x}" y="${y}" width="13" height="${112 - y}" fill="${on || was ? hue : '#ddd3bb'}" opacity="${on ? '0.95' : was ? '0.4' : '0.75'}"/>`;
    s += `<text x="${x + 6.5}" y="${y - 4}" text-anchor="middle" font-size="8" fill="${on ? hue : '#8a7d63'}">T${i + 1}</text>`;
  }
  const tx = 36 + (tgt - 1) * 15 + 6.5, ty = 100 - (tgt - 1) * 15;
  s += `<circle cx="${tx}" cy="${ty - 14}" r="3.2" fill="${hue}"/>`;
  // 오른쪽 장: 글줄 + 길 표식
  for (let i = 0; i < 4; i++) s += `<line x1="118" y1="${42 + i * 13}" x2="174" y2="${42 + i * 13}" stroke="#b3a67f" stroke-width="1.6" opacity="${1 - i * 0.12}"/>`;
  if (p.ko === '언령') s += `<g stroke="${hue}" stroke-width="1.5" fill="none"><path d="M146,104 q6,-6 0,-12"/><path d="M152,107 q9,-9 0,-18"/></g>`;
  else if (p.ko === '연공') s += `<circle cx="148" cy="94" r="4" fill="none" stroke="${hue}" stroke-width="1.5"/><line x1="148" y1="98" x2="148" y2="110" stroke="${hue}" stroke-width="1.5"/><circle cx="148" cy="106" r="2" fill="${hue}"/>`;
  else if (p.ko === '진 새김') s += `<circle cx="148" cy="100" r="8" fill="none" stroke="${hue}" stroke-width="1.5"/><circle cx="148" cy="92" r="1.8" fill="${hue}"/>`;
  else s += `<path d="M142,98 q6,-6 12,0 q-6,6 -12,0 z" stroke="${hue}" stroke-width="1.5" fill="none"/>`;
  s += `<path d="M160,22 l0,24 5,-6 5,6 0,-24 z" fill="${hue}" opacity=".85"/>`;
  s += `<text x="12" y="${H - 8}" font-size="8.5" fill="#6b6353">저울 올리는 책 — ${p.ko} 마디 사다리(§4-2-1)</text>`;
  return { w: W, h: H, svg: s };
}
function bookCard(no, p, name, tier, effect, learn) {
  const sc = bookScene(p, tier);
  const chips = [
    `<span class="skc" style="background:${p.hue};color:#fff">${p.ko}</span>`,
    `<span class="skc">연공서 · 숙련도 책</span>`,
    `<span class="skc">올림 ${tier}</span>`,
  ];
  const [where, cond] = splitLearn(learn);
  const [effMain, effRest] = splitEffect(effect);
  const from = (tier.match(/T(\d+)\s*→/) || [])[1];
  const grid = fieldGrid([
    ['효과', effMain],
    ['기능방식', `${p.ko} 저울(길별 T)을 올리는 연공서 — 익히면 ${p.ko} 마디가 ${tier}로 오른다(§4-2-1)`],
    ['올림', tier],
    ['습득처', bookOf(where)],
    ['습득조건', from ? `${p.ko} T${from}에서 시작` : `${p.ko} 저울 첫 걸음(입문)`],
    ['익힘', learn],
    ['풀이', effRest],
  ]);
  return `<div class="skcard">
<div class="skart"><svg width="${sc.w}" height="${sc.h}" viewBox="0 0 ${sc.w} ${sc.h}">${sc.svg}</svg><div class="skno">서 No.${String(no).padStart(2, '0')}</div></div>
<div class="skbody">
<div class="skname">「${name}」 <span class="sktier">${p.ko} ${tier}</span></div>
<div class="skchips">${chips.join('')}</div>
<div class="sksent"><span style="opacity:.75">숙련도 책 — ${p.ko} 저울(길별 T)을 올린다</span></div>
${grid}
</div></div>`;
}

// ── 아이템 카드 — §5-22 아이템첩 (장비·소모품: 진 새김 병장·방아쇠 소모품·아티팩트·재료) ──
function itemScene(kind, name) {
  const W = 210, H = 150;
  let s = `<rect width="${W}" height="${H}" fill="#efe8d6" rx="10"/>`;
  s += `<ellipse cx="105" cy="118" rx="52" ry="9" fill="#d9cdae"/>`;
  const g = [];
  if (/살촉/.test(name)) {
    g.push(`<line x1="70" y1="108" x2="140" y2="52" stroke="#8a7a5a" stroke-width="3"/>`);
    g.push(`<path d="M140,52 l14,-11 -4,17 z" fill="#4a7f9c"/>`);
    g.push(`<g stroke="#4a7f9c" stroke-width="1.3"><line x1="146" y1="47" x2="154" y2="39"/><line x1="150" y1="51" x2="158" y2="47"/></g>`);
  } else if (/구슬/.test(name)) {
    g.push(`<circle cx="105" cy="88" r="26" fill="#cbb98e" stroke="#8a7a5a" stroke-width="2"/>`);
    g.push(`<path d="M92,80 l10,6 -6,7 12,6" stroke="#96702e" stroke-width="2" fill="none"/>`);
    g.push(`<g stroke="#96702e" stroke-width="1.2" fill="none"><path d="M138,78 q6,10 0,20"/><path d="M146,72 q9,16 0,32"/></g>`);
  } else if (/말뚝/.test(name)) {
    g.push(`<rect x="98" y="52" width="14" height="64" fill="#8a7a5a"/><path d="M98,52 h14 l-7,-12 z" fill="#8a7a5a"/>`);
    g.push(`<circle cx="105" cy="66" r="5" fill="#e9e1cd" stroke="#7d6a4d" stroke-width="1.6"/>`);
    g.push(`<g fill="#7d6a4d" opacity=".5"><rect x="52" y="76" width="20" height="14"/><rect x="64" y="60" width="20" height="14"/><rect x="138" y="76" width="20" height="14"/><rect x="126" y="60" width="20" height="14"/></g>`);
  } else if (/칼|검/.test(name)) {
    g.push(`<path d="M70,110 L134,46 l8,8 -64,64 z" fill="#c9c2b2" stroke="#8a7a5a" stroke-width="1.4"/>`);
    g.push(`<rect x="60" y="104" width="22" height="8" rx="3" transform="rotate(-45 71 108)" fill="#5a4f3a"/>`);
    g.push(`<path d="M120,60 q6,-8 2,-14 M128,66 q8,-6 6,-14" stroke="#b5482a" stroke-width="1.6" fill="none"/>`);
  } else if (/신|장화/.test(name)) {
    g.push(`<path d="M76,100 q0,-18 12,-18 l10,0 q6,0 8,10 l2,8 q22,2 26,8 q2,6 -6,6 l-46,0 q-6,0 -6,-14 z" fill="#6b5d49"/>`);
    g.push(`<g stroke="#6a6d8f" stroke-width="1.3" opacity=".8"><path d="M84,116 h44"/><path d="M90,120 h34"/></g>`);
  } else if (/먹/.test(name)) {
    g.push(`<rect x="88" y="58" width="34" height="52" rx="4" fill="#3d382e"/>`);
    g.push(`<path d="M96,70 q6,6 0,12 q8,2 6,10" stroke="#b5482a" stroke-width="1.5" fill="none"/>`);
    g.push(`<ellipse cx="105" cy="118" rx="30" ry="6" fill="#5a4f3a" opacity=".5"/>`);
  } else if (/패/.test(name)) {
    g.push(`<rect x="80" y="52" width="50" height="62" rx="6" fill="#a98e5f" stroke="#7d6a4d" stroke-width="2"/>`);
    g.push(`<circle cx="105" cy="60" r="3" fill="#5a4f3a"/>`);
    g.push(`<g stroke="#6a5a4a" stroke-width="1.6" fill="none"><rect x="90" y="72" width="30" height="30"/><line x1="85" y1="87" x2="125" y2="87"/></g>`);
  } else {
    g.push(`<rect x="82" y="70" width="46" height="34" rx="5" fill="#a98e5f" stroke="#7d6a4d" stroke-width="2"/>`);
  }
  s += g.join('');
  s += `<text x="12" y="${H - 8}" font-size="8.5" fill="#6b6353">${kind}</text>`;
  return { w: W, h: H, svg: s };
}
function itemCard(no, name, kind, price, effect, get) {
  const sc = itemScene(kind, name);
  const [effMain, effRest] = splitEffect(effect);
  const chips = [];
  if (/(굳는다|터져|엇갈리|돌벽|빨리 붙|박힌)/.test(effect || '')) chips.push(`<span class="skc" style="background:#a13b2e;color:#fff">전투</span>`);
  chips.push(`<span class="skc" style="background:#7a6440;color:#fff">아이템</span>`);
  chips.push(`<span class="skc">${kind}</span>`);
  const grid = fieldGrid([
    ['효과', effMain],
    ['종류', kind],
    ['값', price],
    ['얻는 곳', get],
    ['풀이', effRest],
  ]);
  return `<div class="skcard">
<div class="skart"><svg width="${sc.w}" height="${sc.h}" viewBox="0 0 ${sc.w} ${sc.h}">${sc.svg}</svg><div class="skno">物 No.${String(no).padStart(2, '0')}</div></div>
<div class="skbody">
<div class="skname">「${name}」 <span class="sktier">${kind}</span></div>
<div class="skchips">${chips.join('')}</div>
${grid}
</div></div>`;
}

// ── 기능요소 그리드 (사용자 지시 2026-07-18: 포켓몬 도감식 — 효과·기능방식·습득처·습득조건을 라벨 행으로) ──
const SHAPEKO = {
  point: '한 점 직격', line: '줄기로 뻗음', mist: '물처럼 스밈',
  wall: '벽으로 섬', field: '들판을 덮음', self: '제 둘레', one: '한 사람·한 자리',
};
const TARGETKO = {
  point: '한 과녁 — 눈으로 겨눔(과녁의 법)', one: '한 과녁 — 사람 하나·한 자리',
  line: '훑는 선 — 겨눈 자리를 지나며', field: '자리 전체 — 겨눔 없이 땅에 깖',
  mist: '퍼지는 자리 — 둘레에 스밈', wall: '막는 선 — 세운 자리', self: '술자 자신·둘레',
};
// 익힘 프로즈 → 습득처 / 습득조건 (정본 문체: "습득처 — 조건·과정")
function splitLearn(learn) {
  if (!learn) return ['', ''];
  const i = learn.indexOf(' — ');
  if (i >= 0) return [learn.slice(0, i).trim(), learn.slice(i + 3).trim()];
  const j = learn.indexOf('—');
  if (j >= 0) return [learn.slice(0, j).trim(), learn.slice(j + 1).trim()];
  return ['', learn.trim()];
}
// 습득처 = 사용법이 적힌 책 + 그 책이 있는 자리 (사용자 지시 2026-07-18 오후: "어느 책에 있나 + 어느 위치에 있는가")
function bookOf(where) {
  const w = where || '';
  const pick = [
    [/예경/, '예경(비는 길의 책) — 성지 예방 안(성지마다 딴 책 · §2-5)'],
    [/나라의 손|명부 계보/, '「명부 계보서」 — 나라 서고 명부 안(필사 금지)'],
    [/군문|병가/, '「군문 병서」 — 군영 병서고'],
    [/학당/, '「학당 문장첩」 — 학당 서고'],
    [/의방/, '「의방 첩」 — 의방 서가'],
    [/산문/, '「산문 연공첩」 — 산문 서가'],
    [/부적방/, '「부적방 족보」 — 부적방 궤(도제만 연다)'],
    [/조합|진장이|항구|진쟁이/, '「조합 꼴첩」 — 조합 회관 장부 곁'],
    [/가문/, '「가문 비전서」 — 가문 서고(담 안)'],
    [/유파/, '유파의 차례 문서 — 유파 담 안'],
    [/상단|대상/, '「상단 길 문서」 — 상단 궤'],
    [/관(?!습|례)/, '「관아 명부첩」 — 관아 서고'],
  ];
  for (const [re, v] of pick) if (re.test(w)) return v;
  if (w) return `책 없음 — ${w}에서 손에서 손으로`;
  return '';
}
// 효과 프로즈 → [직관 효과 한 문장, 나머지 풀이] (사용자 지시 2026-07-18 오후: 효과 칸은 "돌벽이 솟아오른다"식 직관 효과만)
function splitEffect(eff) {
  if (!eff) return ['', ''];
  const m = eff.match(/^(.*?[다까음]\.)\s+([\s\S]+)$/);
  if (m) return [m[1].trim(), m[2].trim()];
  return [eff.trim(), ''];
}
function fieldGrid(rows) {
  const body = rows.filter(r => r[1]).map(([k, v]) =>
    `<div class="skrow"><span class="skk">${k}</span><span class="skv">${v}</span></div>`).join('');
  return `<div class="skgrid">${body}</div>`;
}

// ── 카드 HTML ──
function card(no, el, name, sent, medium, tier, effect, learn, bodyPart) {
  const f = readSentence(sent, effect);
  const sc = scene(el, f, bodyPart || '');
  const tMin = (String(tier).match(/T(\d+)/) || [])[1] || '';
  // 마디는 길마다 센다(정본 §4-2-1) — T 앞에 바탕 길의 저울 이름을 붙인다
  let scale = '언령';
  if (el.ko === '연공') scale = '연공';
  else if (el.ko === '신앙' || /신앙|비는 길/.test(medium)) scale = '신앙';
  else if (/소리.*그림|그림.*소리/.test(medium)) scale = '언령+진';
  else if (/그림/.test(medium)) scale = '진';
  else if (/매개/.test(medium)) scale = '주술';
  else if (/몸/.test(medium)) scale = '연공';
  tier = scale + ' ' + tier;
  const chips = [];
  // 전투/살림 식별 칩 — 효과 문장의 타격 동사로 판별 (게임 스킬 우선 정렬·검수용)
  if (/(때리|꿰뚫|찌르|찌른|베인|베어|긋고|긋는|태우|태운|터지|터뜨|쏟아|무너|넘긴|넘어뜨|꿇리|꿇린|물어|문다|삼키|박히|쪼개|후려|올려친|짓이|떨군|끊는다|부러진)/.test(effect || '')) {
    chips.push(`<span class="skc" style="background:#a13b2e;color:#fff">전투</span>`);
  }
  chips.push(`<span class="skc" style="background:${el.hue};color:#fff">${el.ko}${el.ko === '연공' || el.ko === '신앙' ? '' : ' · ' + Object.keys(ELEM).find(k => ELEM[k] === el)}</span>`);
  if (medium) chips.push(`<span class="skc">${medium}</span>`);
  if (f.ending) chips.push(`<span class="skc">${f.ending}</span>`);
  chips.push(`<span class="skc">⧗ ${f.time}</span>`);
  const sentHtml = sent
    ? `<div class="sksent">${sent.split('·').map(w => `<span>${w.trim()}</span>`).join('<i>·</i>')}</div>`
    : `<div class="sksent"><span style="opacity:.65">${el.ko === '신앙' ? '문장 없음 — 남의 길을 빌려 청한다(비는 길 §2-5)' : el.ko === '주술' ? '문장 없음 — 매개가 회로의 반을 진다(주술 §2-4)' : /되받기/.test(medium || '') ? '문장 없음 — 혀에 물고 기다리는 -노(§5-18)' : /겹 얹기/.test(medium || '') ? '문장 없음 — 두 길이 문장을 나눠 진다(§3-2)' : '문장 없음 — 몸에 낸 길(연공)'}</span></div>`;
  // 기능요소 (효과·기능방식·과녁·습득처·습득조건)
  const [where] = splitLearn(learn);
  const isBody = el.ko === '연공';
  const mech = isBody
    ? `${scale}길 — ${bodyPart ? bodyPart.trim() + '에 마력의 길을 냄' : '몸을 회로로 벼림'} · ${f.time}`
    : `${medium || scale + '길'} — ${SHAPEKO[f.shape] || '발현'} · ${f.time}${f.ending ? ' · ' + f.ending : ''}`;
  const isMedium = /매개/.test(medium || '');
  const target = isBody ? '술자 자신(몸)'
    : isMedium ? '인연이 잡는 과녁 — 매개가 닿은 것이면 눈이 없어도 닿는다(§2-4)'
    : el.ko === '신앙' ? '청이 닿는 자리 — 술자가 못 고른다, 제 편도 든다(비는 길 §2-5)'
    : (TARGETKO[f.shape] || '');
  // ★카드값 (사용자 2026-07-18 "모든 기술들은 카드로 쓸거니까") — 효과 프로즈에서 뽑아 맨 윗 행으로
  let stat = '';
  effect = (effect || '').replace(/<b>카드값:?<\/b>:?\s*([^<]*?)\s*(?:<br\s*\/?>|(?=<b>)|$)/, (_, v) => { stat = v.trim(); return ''; })
                         .replace(/\*\*카드값:?\*\*:?\s*([^\n]*)/, (_, v) => { stat = v.trim(); return ''; }).trim();
  // 금기 값(있으면) 별도 추출 — 카드값 뒤 <b>금기 값</b> 조각을 풀이로 넘기지 않고 라벨화
  let taboo = '';
  effect = effect.replace(/<b>금기 값:?<\/b>:?\s*([^<]*?)\s*(?:<br\s*\/?>|(?=<b>)|$)/, (_, v) => { taboo = v.trim(); return ''; }).trim();
  const [effMain, effRest] = splitEffect(effect);
  const grid = fieldGrid([
    ['카드값', stat],
    ['효과', effMain],
    ['기능방식', mech],
    ['과녁', target],
    ['습득처', bookOf(where)],
    ['습득조건', tMin ? `${scale} T${tMin} 이상` : ''],
    ['금기 값', taboo],
    ['익힘', learn],
    ['풀이', effRest],
  ]);
  return `<div class="skcard">
<div class="skart"><svg width="${sc.w}" height="${sc.h}" viewBox="0 0 ${sc.w} ${sc.h}">${sc.svg}</svg><div class="skno">No.${String(no).padStart(3, '0')}</div></div>
<div class="skbody">
<div class="skname">「${name}」 <span class="sktier">${tier}</span></div>
<div class="skchips">${chips.join('')}</div>
${sentHtml}
${grid}
</div></div>`;
}

// ── 정본 md 변환 ──
function transform(md) {
  const lines = md.split(/\r?\n/);
  const out = [];
  const cards = {};
  let el = null, bodyMode = false, jinMode = false, bookM = false, itemM = false, no = 0, section = '';
  const inline = t => t.replace(/\*\*([^*]+)\*\*/g, '<b>$1</b>');

  for (const line of lines) {
    let m;
    if ((m = line.match(/^### 5-([\d-]+)\.\s*(.+)$/))) {
      section = m[1];
      el = null; bodyMode = false;
      jinMode = /^15(-|$)/.test(section); bookM = /^21(-|$)/.test(section); itemM = /^22(-|$)/.test(section);
      const t = m[2];
      // "불 — 자르" / "아묾·닫힘 — 니르·코르" / "몸길 기술첩 …"
      const em = t.match(/—\s*([가-힣·]+)\s*(?:\([^)]*\))?\s*$/);
      if (em) {
        const key = em[1].split('·')[0].trim();
        if (ELEM[key]) el = ELEM[key];
      }
      if (/^몸길 기술첩/.test(t)) { el = ELEM['몸길']; bodyMode = true; }
      out.push(line);
      continue;
    }
    // §5-15 그림길 — 글 진·부적 (문장 있는 꼴)
    if (jinMode && (m = line.match(/^- \*\*「(.+?)」\*\* — \*(.+?)\*\s*(?:\((.+?)\))?\s*·\s*(.+)$/))) {
      const [, name, sentRaw, srcNote, rest] = m;
      const sent = sentRaw.trim();
      const [effRaw, learnRaw] = rest.split(/\*\*익힘[^*]*\*\*:?\s*/);
      let elx = NEUTRAL;
      const sm2 = sent.match(/에 ([가-힣]+)/);
      if (sm2 && ELEM[sm2[1]]) elx = ELEM[sm2[1]];
      else { const stem = sent.split('·').pop().trim().replace(/[타사]$/, ''); if (ELEM[stem]) elx = ELEM[stem]; }
      const lastTok = sent.split('·').map(t => t.trim()).pop() || '';
      const type = /부적/.test(name) ? '부적' : (/사$/.test(lastTok) ? '도는 진' : '방아쇠 진');
      no++;
      cards[no] = jinCard(no, elx, name, sent, srcNote || '', type, inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    // §5-15 술식 (문장 없는 꼴)
    if (jinMode && (m = line.match(/^- \*\*「(.+?)」\*\* — 살란 문장 없음\s*(?:\((.+?)\))?\s*·\s*(.+)$/))) {
      const [, name, srcNote, rest] = m;
      const [effRaw, learnRaw] = rest.split(/\*\*익힘[^*]*\*\*:?\s*/);
      no++;
      cards[no] = jinCard(no, NEUTRAL, name, '', srcNote || '', '술식', inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    // §5-21 연공서첩
    if (bookM && (m = line.match(/^- \*\*「(.+?)」\*\* — (소리길|몸길|그림길|매개길|비는 길) · (T[^—]+?) — (.+)$/))) {
      const [, name, pathKey, tier, rest] = m;
      const [effRaw, learnRaw] = rest.split(/\*\*익힘[^*]*\*\*:?\s*/);
      no++;
      cards[no] = bookCard(no, PATHS[pathKey], name, tier.trim(), inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    if (!jinMode && !bookM && (m = line.match(/^- \*\*「(.+?)」\*\* — \*(.+?)\* · ([^·]+(?:·[^·]+)*?) · (T[^—]+?) — (.+)$/))) {
      const [, name, sent, medium, tier, rest] = m;
      // 계열은 절 머리 우선, 없으면(§5-13-2 직격기 모둠 등) 문장 첫 부름말("에 X")로 잡는다
      let elx = el;
      const sm = sent.match(/에 ([가-힣]+)/);
      if (sm && ELEM[sm[1]]) elx = ELEM[sm[1]];
      if (elx) {
        const [effRaw, learnRaw] = rest.split(/\*\*익힘:?\*\*:?\s*/);
        no++;
        cards[no] = card(no, elx, name, sent, medium.trim(), tier.trim(), inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
        out.push(''); out.push(`%%SK${no}%%`); out.push('');
        continue;
      }
    }
    // §5-18-1 되받기 모둠 — 부름말별 -노 카운터: 「이름」 — *X노* (조건) · 소리 · T# — 효과
    if (!jinMode && !bookM && !bodyMode && (m = line.match(/^- \*\*「(.+?)」\*\* — \*([가-힣]+노)\* \((.+?)\) · 소리 · (T[^—]+?) — (.+)$/))) {
      const [, name, tok, cond, tier, rest] = m;
      const elx = ELEM[tok.replace(/노$/, '')] || ELEM['잔'];
      const [effRaw, learnRaw] = rest.split(/\*\*익힘:?\*\*:?\s*/);
      no++;
      cards[no] = card(no, elx, name, tok, '소리 · 되받기(' + cond + ')', tier.trim(), inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    // §5-17 겹 얹기 — 「이름」 — 몸+소리(바늘꼴) · T4 — 효과 (두 바탕이 한 발현에)
    if (!jinMode && !bookM && !itemM && (m = line.match(/^- \*\*「(.+?)」\*\* — ([가-힣]+\+[가-힣]+[^·]*) · (T[^—]+?) — (.+)$/))) {
      const [, name, mix, tier, rest] = m;
      const first = mix.match(/^([가-힣]+)/)[1];
      const elx = ELEM[first === '몸' ? '몸길' : first] || ELEM['겹'];
      const [effRaw, learnRaw] = rest.split(/\*\*익힘:?\*\*:?\s*/);
      no++;
      cards[no] = card(no, elx, name, '', '겹 얹기 · ' + mix.trim(), tier.trim(), inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    // §5-22 아이템첩 — 「이름」 — 종류 · 값 — 효과 … **얻는 곳:** …
    if (itemM && (m = line.match(/^- \*\*「(.+?)」\*\* — ([^·]+) · ([^—]+) — (.+)$/))) {
      const [, name, kind, price, rest] = m;
      const [effRaw, getRaw] = rest.split(/\*\*얻는 곳:?\*\*:?\s*/);
      no++;
      cards[no] = itemCard(no, name, kind.trim(), price.trim(), inline(effRaw.trim()), getRaw ? inline(getRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    // §5-16 매개길(주술) — 문장 없는 꼴: 「이름」 — 닿음/닮음/파훼(…) · T# — 효과
    if (!jinMode && !bookM && !bodyMode && (m = line.match(/^- \*\*「(.+?)」\*\* — ((?:닿음|닮음|파훼)[^·]*|[^—]{0,40}매개[^·]*) · (T[^—]+?) — (.+)$/))) {
      const [, name, spot, tier, rest] = m;
      const [effRaw, learnRaw] = rest.split(/\*\*익힘:?\*\*:?\s*/);
      no++;
      cards[no] = card(no, ELEM['매개'], name, '', '매개 · ' + spot.trim(), tier.trim(), inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    // §5-24 조각첩(노코스트 기본 카드): 「이름」 — 조각(갈래) · T# — 효과
    if (/^24$/.test(section) && (m = line.match(/^- \*\*「(.+?)」\*\* — ([^·]+) · (T[^—]+?) — (.+)$/))) {
      const [, name, kind, tier, rest] = m;
      const [effRaw, learnRaw] = rest.split(/\*\*익힘:?\*\*:?\s*/);
      no++;
      cards[no] = card(no, NEUTRAL, name, '', '조각 · ' + kind.trim(), tier.trim(), inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    // §5-17-2 이름난 비전(게임도감 Ⅰ 연동): 「이름」 — 가문 · 길 · T# — 효과
    if (/^17-2$/.test(section) && (m = line.match(/^- \*\*「(.+?)」\*\* — ([^·]+) · ([^·]+) · (T[^—]+?) — (.+)$/))) {
      const [, name, house, path, tier, rest] = m;
      const p = path;
      const elx = /신앙|비는/.test(p) ? ELEM['신앙'] : /연공/.test(p) ? ELEM['몸길'] : /언령/.test(p) ? ELEM['잔'] : /진|마법진/.test(p) ? ELEM['그림'] : /주술|매개/.test(p) ? ELEM['매개'] : NEUTRAL;
      const [effRaw, learnRaw] = rest.split(/\*\*익힘:?\*\*:?\s*/);
      no++;
      cards[no] = card(no, elx, name, '', '이름난 비전 · ' + house.trim() + ' · ' + p.trim(), tier.trim(), inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    // §5-18-2 되받기의 칸 싸움 — 전술 카드(계열 문장이 없는 꼴): 「이름」 — 바탕 설명 · T# — 효과
    if (/^18-2$/.test(section) && (m = line.match(/^- \*\*「(.+?)」\*\* — (.+?) · (T[^—]+?) — (.+)$/))) {
      const [, name, spot, tier, rest] = m;
      const s = spot;
      const elx = /몸길/.test(s) ? ELEM['몸길'] : /매개길/.test(s) ? ELEM['매개'] : /그림길/.test(s) ? ELEM['그림'] : ELEM['잔'];
      const [effRaw, learnRaw] = rest.split(/\*\*익힘:?\*\*:?\s*/);
      no++;
      cards[no] = card(no, elx, name, '', '되받기 · ' + s.trim(), tier.trim(), inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    // §5-23 비는 길(신앙) — 문장 없는 꼴: 「이름」 — 빎(무엇에) · T# — 효과 (남의 길을 빌린다 §2-5)
    if (/^23(-|$)/.test(section) && (m = line.match(/^- \*\*「(.+?)」\*\* — (빎[^·]*|비는[^·]*|청[^·]*|삼[^·]*|예식[^·]*) · (T[^—]+?) — (.+)$/))) {
      const [, name, spot, tier, rest] = m;
      const [effRaw, learnRaw] = rest.split(/\*\*익힘:?\*\*:?\s*/);
      no++;
      cards[no] = card(no, ELEM['신앙'], name, '', '비는 길 · ' + spot.trim(), tier.trim(), inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    // §5-16 매개(주술) 폴백 — 바탕 형식이 제각각인 나머지(괄호 안 ·, '· 매개 ·' 중간 필드, 프로즈 바탕)
    // 위 특정 매개 매처(닿음/닮음/파훼|매개)가 못 잡은 것을 섹션 게이트로 포괄한다(§5-16 불릿은 전부 · T# 를 가짐)
    if (/^16(-|$)/.test(section) && (m = line.match(/^- \*\*「(.+?)」\*\* — (.+?) · (T[^—]+?) — (.+)$/))) {
      const [, name, spotRaw, tier, rest] = m;
      const spot = spotRaw.replace(/\s*·\s*매개\s*$/, '').trim();
      const [effRaw, learnRaw] = rest.split(/\*\*익힘:?\*\*:?\s*/);
      no++;
      cards[no] = card(no, ELEM['매개'], name, '', '매개 · ' + spot, tier.trim(), inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    // §5-18(허묾)·§5-20(금기) 폴백 — 티어가 T#가 아닐 수 있는 특수 형식(마디 셈 불가·세운 그릇 그대로 등)
    if (/^(18|20)(-|$)/.test(section) && (m = line.match(/^- \*\*「(.+?)」\*\* — (.+?) — ([\s\S]+)$/))) {
      const [, name, head, rest] = m;
      const elx = /매개|주술|인연/.test(head) ? ELEM['매개']
        : /몸|겉길|연공|살갗/.test(head) ? ELEM['몸길']
        : /그림|진(?![가-힣])/.test(head) && !/소리/.test(head) ? ELEM['그림']
        : ELEM['잔'];
      const tm = head.match(/T\d+/);
      const tier = tm ? tm[0] : (/^20/.test(section) ? '금기' : '허묾');
      const label = (/^20/.test(section) ? '금기 · ' : '허묾 · ') + head.split('·').pop().trim();
      const [effRaw, learnRaw] = rest.split(/\*\*익힘:?\*\*:?\s*/);
      no++;
      cards[no] = card(no, elx, name, '', label, tier, inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '');
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    if (el && bodyMode && (m = line.match(/^- \*\*「(.+?)」\*\* — (.+?) · (T[^—]+?) — (.+)$/))) {
      const [, name, spot, tier, rest] = m;
      const [effRaw, learnRaw] = rest.split(/\*\*익힘:?\*\*:?\s*/);
      no++;
      cards[no] = card(no, el, name, '', spot.trim(), tier.trim(), inline(effRaw.trim()), learnRaw ? inline(learnRaw.trim()) : '', spot);
      out.push(''); out.push(`%%SK${no}%%`); out.push('');
      continue;
    }
    out.push(line);
  }
  // 방식별 집계(검수·밸런스 패널용) — 카드의 첫 비-전투 색칩으로 분류
  const SPOKEN = ['불','서리','바람','물','돌·땅','빛','그늘','소리','묶음','잠·고요','무게','깨짐','열림','아묾','닫힘','벽·감쌈'];
  const tally = { 언령:0, 진:0, 연공:0, 주술:0, 신앙:0, 겹:0, 아이템:0, 연공서:0, 기타:0 };
  let combat = 0;
  for (const k in cards) {
    const html = cards[k];
    if (/background:#a13b2e;color:#fff">전투</.test(html)) combat++;
    // skno 접두로 진/아이템/연공서를 먼저 가른다(진 카드는 계열색이라 칩만으론 언령과 헷갈림)
    if (/skno">진 No\./.test(html)) { tally.진++; continue; }
    if (/skno">物 No\./.test(html)) { tally.아이템++; continue; }
    if (/skno">서 No\./.test(html)) { tally.연공서++; continue; }
    const chips = [...html.matchAll(/background:[^;"]*;color:#fff">([^<·]+)/g)].map(x => x[1].trim());
    const meth = chips.find(c => c !== '전투') || '';
    if (SPOKEN.includes(meth)) tally.언령++;
    else if (meth === '연공') tally.연공++;
    else if (meth === '진·술식' || meth === '술식') tally.진++;
    else if (meth === '주술') tally.주술++;
    else if (meth === '신앙') tally.신앙++;
    else if (meth === '겹 얹기') tally.겹++;
    else tally.기타++;
  }
  tally.전투 = combat;
  return { md: out.join('\n'), cards, count: no, tally };
}

const CSS = `
.skcard{display:flex;gap:16px;background:#faf6ec;border:1.5px solid #c9bc9c;border-radius:12px;padding:14px;margin:14px 0;align-items:flex-start}
.skart{flex:0 0 210px;position:relative}
.skart svg{display:block;border-radius:10px;border:1px solid #cfc4a8}
.skno{position:absolute;top:6px;right:6px;background:#5a4f3a;color:#f6f1e4;font-size:10px;border-radius:8px;padding:1px 7px;opacity:.85}
.skbody{flex:1;min-width:0}
.skname{font-weight:bold;font-size:1.06em;color:#4a3b2a;margin-bottom:4px}
.sktier{font-size:.78em;color:#a13b2e;border:1px solid #a13b2e;border-radius:8px;padding:0 6px;margin-left:6px;vertical-align:1px}
.skchips{margin:2px 0 6px}
.skc{display:inline-block;font-size:.72em;background:#e7dcc3;color:#5a4f3a;border-radius:8px;padding:1px 8px;margin-right:5px}
.sksent{font-size:.85em;background:#efe8d5;border:1px dashed #b3a67f;border-radius:8px;padding:4px 10px;margin:4px 0 8px;color:#4a3b2a}
.sksent i{opacity:.45;font-style:normal;margin:0 4px}
.sktext{font-size:.9em;line-height:1.65;color:#3d382e}
.sklearn{font-size:.84em;line-height:1.6;color:#6b6353;margin-top:6px;border-top:1px dashed #d8cdb0;padding-top:5px}
.skgrid{margin-top:6px;border-top:1px dashed #d8cdb0}
.skrow{display:flex;gap:0;border-bottom:1px solid #ece2c8;align-items:baseline}
.skrow:last-child{border-bottom:none}
.skk{flex:0 0 62px;font-size:.76em;font-weight:bold;color:#8a5a2a;padding:5px 8px 5px 0;letter-spacing:.02em}
.skv{flex:1;min-width:0;font-size:.87em;line-height:1.6;color:#3d382e;padding:5px 0}
@media (max-width:760px){.skcard{flex-direction:column}.skart{flex:none}}
`;

module.exports = { transform, CSS };
