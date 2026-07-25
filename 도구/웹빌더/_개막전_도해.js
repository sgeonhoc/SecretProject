// 개막 전 현대 배경 도해 모듈 — _build_md_열람.js 의 6번째 인자로 쓴다.
// 사용자 지시: 모든 세계관 페이지는 도감처럼 그림과 함께.
// 빌드: node 도구/웹빌더/_build_md_열람.js 기획/01_세계관/3_현대/세계관_현대_개막전_배경_정본.md 웹/세계관_페이지_개막전배경.html "제목" "부제" "링크" 도구/웹빌더/_개막전_도해.js
// export = [{match:"제목 일부", html:"<svg…>", caption:"…"}] — 정본 md는 손대지 않는다.

function bx(x, y, w, h, title, subs, opt) {
  opt = opt || {};
  let s = `<rect x="${x}" y="${y}" width="${w}" height="${h}" rx="9" style="fill:${opt.fill || '#f2ecdf'};stroke:${opt.stroke || '#b3a67f'};stroke-width:2${opt.dash ? ';stroke-dasharray:' + opt.dash : ''}"/>`;
  s += `<text x="${x + w / 2}" y="${y + 21}" text-anchor="middle" style="font-size:12.5px;font-weight:bold;fill:${opt.tcolor || '#4a3b2a'}">${title}</text>`;
  (subs || []).forEach((t, i) => {
    s += `<text x="${x + w / 2}" y="${y + 39 + i * 15}" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">${t}</text>`;
  });
  return s;
}
function ln(x1, y1, x2, y2, opt) {
  opt = opt || {};
  return `<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" style="stroke:${opt.color || '#8a7a5c'};stroke-width:${opt.w || 2}${opt.dash ? ';stroke-dasharray:' + opt.dash : ''}"/>`;
}
function arrow(x1, y1, x2, y2, opt) {
  opt = opt || {};
  let s = ln(x1, y1, x2, y2, opt);
  const a = Math.atan2(y2 - y1, x2 - x1);
  const hl = opt.head || 9;
  s += `<path d="M${x2},${y2} L${x2 - hl * Math.cos(a - 0.4)},${y2 - hl * Math.sin(a - 0.4)} M${x2},${y2} L${x2 - hl * Math.cos(a + 0.4)},${y2 - hl * Math.sin(a + 0.4)}" style="stroke:${opt.color || '#8a7a5c'};stroke-width:${opt.w || 2};fill:none"/>`;
  return s;
}
function lbl(x, y, t, opt) {
  opt = opt || {};
  return `<text x="${x}" y="${y}" text-anchor="${opt.anchor || 'middle'}" style="font-size:${opt.size || 10.5}px;fill:${opt.color || '#5f5744'}${opt.bold ? ';font-weight:bold' : ''}">${t}</text>`;
}
const svg = (w, h, inner) => `<svg width="${w}" height="${h}" viewBox="0 0 ${w} ${h}">${inner}</svg>`;

const GOLD = '#a8862f', RED = '#a13b2e', BLUE = '#3f6a86', GREY = '#8d8577', GREEN = '#4d7a52';

// ── A. 응답 조건 — 고대와 지금 (§1) ──
const respond = (() => {
  let s = '';
  s += lbl(370, 22, '마력은 줄어든 것이 아니라 까다로워졌다 (§1-2)', { size: 13, bold: true, color: '#3d382e' });

  // 고대 패널
  s += `<rect x="16" y="38" width="340" height="230" rx="10" style="fill:#f6efdc;stroke:${GOLD};stroke-width:2"/>`;
  s += lbl(186, 60, '고대 — 마력이 체계에 길들여져 있었다', { size: 12, bold: true, color: '#7a6320' });
  // 넓은 깔때기(여러 입구 → 다 통과)
  const goldIn = ['길든 길을 밟기', '몸에 밴 손버릇', '뜻만 세우기 (흐릿)'];
  goldIn.forEach((t, i) => {
    s += bx(36, 78 + i * 46, 130, 34, t, [], { fill: '#fdf8e8', stroke: GOLD });
    s += arrow(170, 95 + i * 46, 246, 150, { color: GOLD, w: 1.8 });
  });
  s += `<circle cx="278" cy="150" r="40" style="fill:#fdf3d3;stroke:${GOLD};stroke-width:2.5"/>`;
  s += lbl(278, 146, '마력', { size: 12, bold: true, color: '#7a6320' });
  s += lbl(278, 162, '응답', { size: 11, color: '#7a6320' });
  s += lbl(186, 248, '★체계가 숨과 손버릇이 되어 있어서 즉흥으로도 섰다', { size: 10.5, color: '#7a6320' });

  // 지금 패널
  s += `<rect x="384" y="38" width="340" height="230" rx="10" style="fill:#eceff2;stroke:${BLUE};stroke-width:2"/>`;
  s += lbl(554, 60, '지금 — 적힌 체계만 응답한다', { size: 12, bold: true, color: '#2f5066' });
  const blueIn = [['길든 길을 밟기', true], ['새로 낸 길', false], ['뜻만 세우기', false]];
  blueIn.forEach(([t, ok], i) => {
    s += bx(404, 78 + i * 46, 130, 34, t, [], { fill: ok ? '#e6f0e6' : '#f0eeea', stroke: ok ? GREEN : GREY, dash: ok ? null : '4,3', tcolor: ok ? '#2f5c34' : '#8d8577' });
    if (ok) s += arrow(538, 95 + i * 46, 614, 150, { color: GREEN, w: 2.2 });
    else {
      s += ln(538, 95 + i * 46, 588, 122 + i * 12, { color: GREY, w: 1.6, dash: '4,3' });
      s += lbl(600, 100 + i * 46, '✕', { size: 15, bold: true, color: RED });
    }
  });
  s += `<circle cx="646" cy="150" r="40" style="fill:#e8eef3;stroke:${BLUE};stroke-width:2.5"/>`;
  s += lbl(646, 146, '마력', { size: 12, bold: true, color: '#2f5066' });
  s += lbl(646, 162, '응답', { size: 11, color: '#2f5066' });
  s += lbl(554, 234, '★줄어든 게 아니다 — 안 밟으면 크든 작든 아무것도 안 선다.', { size: 10.5, color: '#2f5066' });
  s += lbl(554, 250, '밟으면 지금도 선다.', { size: 10.5, bold: true, color: '#2f5066' });
  return svg(740, 280, s);
})();

// ── B. 그래서 힘은 발굴로만 는다 (§1-4) ──
const onlyDig = (() => {
  let s = '';
  s += lbl(370, 22, '새로 못 쓰니 힘이 발명으로 안 늘고 발굴로만 는다 (§1-4)', { size: 13, bold: true, color: '#3d382e' });
  s += bx(30, 46, 170, 62, '마력이 길든 대상', ['= 고대의 체계'], { fill: '#f6efdc', stroke: GOLD });
  s += arrow(200, 77, 268, 77, { color: '#8a7a5c' });
  s += bx(268, 46, 190, 62, '새 소리·새 회로를 지어도', ['응답하지 않는다'], { fill: '#f0eeea', stroke: GREY, tcolor: '#7d7364' });
  s += arrow(458, 77, 522, 77, { color: '#8a7a5c' });
  s += bx(522, 40, 190, 74, '현대가 할 수 있는 일', ['옮기기 · 맞추기 · 주석'], { fill: '#eceff2', stroke: BLUE, tcolor: '#2f5066' });

  const res = [
    ['위조가 치명적', '획 하나 틀리면 아예 안 선다'],
    ['낱장도 값이 있다', '나머지가 있었다는 증거'],
    ['익힘이 물건보다 귀하다', '끝까지 밟는 사람이 드묾'],
    ['아래층이 유일한 원천', '땅 파는 일 = 힘 얻는 일']
  ];
  res.forEach(([t, u], i) => {
    const x = 26 + i * 178;
    s += arrow(x + 78, 130, x + 78, 156, { color: RED, w: 1.8 });
    s += bx(x, 156, 160, 62, t, [u], { fill: '#fbf1ec', stroke: RED, tcolor: '#8d3225' });
  });
  s += ln(104, 130, 626, 130, { color: RED, w: 1.6, dash: '5,4' });
  s += arrow(370, 114, 370, 130, { color: RED, w: 1.8 });
  return svg(740, 232, s);
})();

// ── C. 은닉은 합의가 아니라 각자의 셈 (§2) ──
const hiding = (() => {
  let s = '';
  s += lbl(370, 22, '아무도 서명하지 않았는데 다 같은 답이 나온다 (§2-2)', { size: 13, bold: true, color: '#3d382e' });
  const people = [
    ['오래된 가문의 당주', '① 독점', '알려지면 값이 내린다', GOLD],
    ['검은물을 아는 학자', '② 원인 의심', '느는 것 자체가 위험할지 모른다', BLUE],
    ['한 줄 물려받은 사람', '③ 옛일의 기억', '할아버지가 왜 입을 다물었는지 안다', '#6b5a86'],
    ['아무 이유 없는 사람', '— 없음 —', '알려서 얻는 게 뭔데?', GREY]
  ];
  people.forEach(([who, why, think, col], i) => {
    const y = 44 + i * 60;
    s += bx(20, y, 150, 48, who, [why], { fill: '#f7f2e6', stroke: col, tcolor: col });
    s += lbl(184, y + 29, think, { size: 10.5, anchor: 'start', color: '#6b6353' });
    s += arrow(470, y + 24, 536, 164, { color: col, w: 1.5, dash: '4,3' });
  });
  s += `<rect x="540" y="132" width="180" height="66" rx="10" style="fill:#efe7d2;stroke:#6b5a3a;stroke-width:2.5"/>`;
  s += lbl(630, 158, '은닉', { size: 15, bold: true, color: '#4a3b2a' });
  s += lbl(630, 178, '= 기본값', { size: 11.5, color: '#6b5a3a' });
  s += lbl(370, 296, '★네 번째 줄이 가장 흔하다 — 신념도 겁도 아니라 티낼 이유가 없어서 안 알린다.', { size: 11, bold: true, color: '#8d3225' });
  s += lbl(370, 314, '합의가 아니므로 강제하는 조직이 없고, 그래서 개막 전에는 유지비가 거의 안 들었다.', { size: 10.5, color: '#6b6353' });
  return svg(740, 326, s);
})();

// ── D. 세상이 아는 것 — 네 겹 (§3) ──
const fourLayers = (() => {
  let s = '';
  s += lbl(370, 22, '대부분은 대절멸이 있었는지도 모른다 (§3)', { size: 13, bold: true, color: '#3d382e' });
  const tiers = [
    ['1. 아무것도 모름', '위층의 대부분 — 홍수 전설조차 실제 사건으로 안 여긴다', 660, '#e9e4d6', '#9c9280'],
    ['2. 사건으로는 앎', '극소수 — "능력의 포화가 검은물을 불렀다"고 믿는다', 420, '#efe3cf', GOLD],
    ['3. 체계가 달라졌음을 앎', '더 극소수 — 왜인지는 모른다', 250, '#e4e8ee', BLUE],
    ['4. 참', '우리·시스템만 — 현대의 누구도 못 닿는다', 120, '#efdedb', RED]
  ];
  let y = 44;
  tiers.forEach(([t, u, w, fill, stroke]) => {
    const x = 370 - w / 2;
    s += `<rect x="${x}" y="${y}" width="${w}" height="56" rx="7" style="fill:${fill};stroke:${stroke};stroke-width:2"/>`;
    s += lbl(370, y + 23, t, { size: 12, bold: true, color: stroke });
    s += lbl(370, y + 41, u, { size: 10.5, color: '#6b6353' });
    y += 66;
  });
  s += lbl(370, 316, '★그래서 「다시 넘치면 다시 온다」는 대중 정서가 아니라 소수의 내부 교리다.', { size: 11, bold: true, color: '#8d3225' });
  s += lbl(370, 334, '위층 인물이 검은물을 자연스럽게 입에 올리는 장면은 틀린 장면이다.', { size: 10.5, color: '#6b6353' });
  return svg(740, 346, s);
})();

// ── E. 간격 — 물건이 실제로 새는 길 (§7) ──
const leak = (() => {
  let s = '';
  s += lbl(370, 22, '공사장에서 나온 것은 그냥 덮이지 않는다 (§7-1)', { size: 13, bold: true, color: '#3d382e' });
  s += bx(292, 40, 156, 48, '굴착에서 나옴', ['옛 석축 · 새김 · 함'], { fill: '#efe7d2', stroke: '#6b5a3a' });
  s += arrow(340, 88, 210, 122, { color: '#8a7a5c' });
  s += arrow(400, 88, 530, 122, { color: '#8a7a5c' });

  // 왼쪽 — 글자 없는 돌
  s += bx(120, 122, 180, 54, '글자가 없는 돌', ['읽을 게 없으니 힘이 안 된다'], { fill: '#e9e4d6', stroke: GREY, tcolor: '#7d7364' });
  s += arrow(210, 176, 210, 206, { color: GREY, w: 1.8 });
  s += bx(112, 206, 196, 54, '신고 → 조사 → 학계', ['"미상 유적"으로 남는다'], { fill: '#eceff2', stroke: BLUE, tcolor: '#2f5066' });
  s += lbl(210, 282, '은닉하는 쪽이 손댈 이유가 없다', { size: 10.5, color: '#6b6353' });

  // 오른쪽 — 읽을 수 있는 것
  s += bx(440, 122, 190, 54, '읽을 수 있는 것', ['새김 든 판 · 함 속의 서'], { fill: '#fbf1ec', stroke: RED, tcolor: '#8d3225' });
  s += arrow(535, 176, 535, 206, { color: RED, w: 2 });
  s += bx(430, 206, 210, 66, '현장 사람이 먼저 판다', ['공기가 밀리면 위약금이 목을 조른다', '일당보다 큰 값을 아는 인부'], { fill: '#fdf6ee', stroke: RED, tcolor: '#8d3225' });
  s += arrow(535, 272, 535, 300, { color: RED, w: 2 });
  s += bx(452, 300, 166, 46, '저잣거리 뒷금고', [], { fill: '#f7f2e6', stroke: '#6b5a3a' });
  s += arrow(452, 322, 320, 322, { color: '#6b5a3a', w: 1.8 });
  s += bx(150, 300, 170, 46, '세력의 손이 붙는다', [], { fill: '#efe7d2', stroke: '#6b5a3a' });
  s += lbl(235, 366, '★조직이 덮는 게 아니라 현장이 먼저 판다.', { size: 11, bold: true, color: '#8d3225' });
  s += lbl(235, 384, '세력의 손은 대개 그 뒤에 붙는다.', { size: 10.5, color: '#6b6353' });
  return svg(740, 396, s);
})();

// ── F. 개막이 깨는 네 조건 (§9-2) ──
const opening = (() => {
  let s = '';
  s += lbl(370, 22, '개막은 능력자를 만들지 않았다 — 셈을 바꿨다 (§9-2)', { size: 13, bold: true, color: '#3d382e' });
  s += `<rect x="18" y="38" width="326" height="250" rx="10" style="fill:#f4f1e6;stroke:#9c9280;stroke-width:2"/>`;
  s += lbl(181, 60, '개막 전 — 네 조건', { size: 12, bold: true, color: '#6b6353' });
  const before = ['물건이 드물다', '읽을 사람이 몇 없다', '은닉이 각자의 셈이었다', '표적이 드물어 정교할 여유'];
  before.forEach((t, i) => s += bx(38, 76 + i * 50, 286, 38, t, [], { fill: '#fbf8ef', stroke: '#b3a67f' }));

  s += `<rect x="396" y="38" width="326" height="250" rx="10" style="fill:#fbf0ec;stroke:${RED};stroke-width:2"/>`;
  s += lbl(559, 60, '개막 후 — 동시에 깨진다', { size: 12, bold: true, color: '#8d3225' });
  const after = ['회수의 채산이 깨진다', '가짜와 진짜가 손으로 갈린다', '규칙 모르는 참가자가 는다', '싼 방법이 종이에 자국을 남긴다'];
  after.forEach((t, i) => {
    s += bx(416, 76 + i * 50, 286, 38, t, [], { fill: '#fdf3ef', stroke: RED, tcolor: '#8d3225' });
    s += arrow(330, 95 + i * 50, 410, 95 + i * 50, { color: RED, w: 1.8 });
  });
  s += lbl(370, 312, '개막 = 마력의 발현체계를 따르는 방법들이 갑자기 라셀에 퍼진 것. 원인은 모른다.', { size: 11.5, bold: true, color: '#4a3b2a' });
  s += lbl(370, 330, '★사람이 는 것은 결과다 — 「각성」이 아니라 습득이다.', { size: 10.5, color: '#8d3225' });
  return svg(740, 342, s);
})();

// ── G. 조련의 법 세 조항 (§1-3) ──
const taming = (() => {
  let s = '';
  s += lbl(370, 22, '알아듣는 것은 마력이 아니라 길이다 (조련의 법 · §1-3)', { size: 13, bold: true, color: '#3d382e' });
  const items = [
    ['① 오래 조련될수록 너그럽다', '만 년 길든 자르는 서툰 혀도 알아듣는다', GOLD, '#fdf8e8'],
    ['② 잘 가르칠수록 밝다', '비전의 첫 획이 독학 십 년을 이긴다', '#6b5a86', '#f4f0f8'],
    ['③ 똑바로 청하지 못한 것은', '못 알아듣는다 — 흐릿한 길엔 아무것도 안 온다', BLUE, '#eceff2']
  ];
  items.forEach(([t, u, col, fill], i) => {
    const x = 22 + i * 240;
    s += bx(x, 44, 220, 74, t, [u], { fill, stroke: col, tcolor: col });
    s += arrow(x + 110, 118, 370, 152, { color: col, w: 1.6, dash: '4,3' });
  });
  s += `<rect x="200" y="152" width="340" height="52" rx="9" style="fill:#efe7d2;stroke:#6b5a3a;stroke-width:2.5"/>`;
  s += lbl(370, 175, '길든 깊이가 알아듣는 깊이', { size: 14, bold: true, color: '#4a3b2a' });
  s += lbl(370, 195, '마력 자체는 셈도 귀도 없다', { size: 10.5, color: '#6b5a3a' });
  s += lbl(370, 232, '★그래서 체계 없는 천재보다 체계 위의 범재가 멀리 간다.', { size: 11.5, bold: true, color: '#8d3225' });
  s += lbl(370, 250, '이 법은 능력자를 묶는 사슬이 아니라 그들이 딛는 계단이다.', { size: 10.5, color: '#6b6353' });
  return svg(740, 262, s);
})();

// ── H. 조련의 계층 — 개막은 넓고 얕다 (§1-10) ──
const layerDepth = (() => {
  let s = '';
  s += lbl(370, 22, '개막은 넓게 퍼지면서 얕다 (§1-10)', { size: 13, bold: true, color: '#3d382e' });
  // 가로축 = 길든 깊이, 세로 = 사람 수
  s += ln(60, 210, 700, 210, { color: '#8a7a5c', w: 2 });
  s += ln(60, 210, 60, 50, { color: '#8a7a5c', w: 2 });
  s += lbl(380, 236, '길든 깊이 (오래 길든 흔한 길 → 덜 길든 잊힌 갈래)', { size: 10.5, color: '#6b6353' });
  s += `<text x="34" y="130" text-anchor="middle" transform="rotate(-90 34 130)" style="font-size:10.5px;fill:#6b6353">세우는 사람 수</text>`;
  // 곡선 — 왼쪽 높고 오른쪽 낮음
  s += `<path d="M70,70 C160,72 250,150 380,182 C500,200 600,206 695,208" style="fill:none;stroke:${RED};stroke-width:2.5"/>`;
  // 구역
  s += `<rect x="66" y="56" width="150" height="152" rx="6" style="fill:#fbf1ec;stroke:${RED};stroke-width:1.5;stroke-dasharray:5,4"/>`;
  s += lbl(141, 96, '불·물·바람', { size: 11.5, bold: true, color: '#8d3225' });
  s += lbl(141, 114, '만 년 길든 길', { size: 10, color: '#8d3225' });
  s += lbl(141, 136, '낱장만 있으면', { size: 10, color: '#6b6353' });
  s += lbl(141, 152, '서툰 혀도 선다', { size: 10, color: '#6b6353' });
  s += lbl(141, 176, '= 요아의 한 뼘 불', { size: 10, bold: true, color: '#8d3225' });
  s += lbl(141, 192, 'T1', { size: 10, color: '#8d3225' });

  s += `<rect x="470" y="56" width="228" height="152" rx="6" style="fill:#eceff2;stroke:${BLUE};stroke-width:1.5;stroke-dasharray:5,4"/>`;
  s += lbl(584, 96, '덜 길든 · 잊힌 갈래', { size: 11.5, bold: true, color: '#2f5066' });
  s += lbl(584, 118, '낱장이 있어도', { size: 10, color: '#6b6353' });
  s += lbl(584, 134, '벼림 없이는 안 선다', { size: 10, color: '#6b6353' });
  s += lbl(584, 158, '= 셀란이 몇 해를 찾는 것', { size: 10, bold: true, color: '#2f5066' });
  s += lbl(584, 176, '미르사가 물건보다', { size: 10, color: '#2f5066' });
  s += lbl(584, 192, '귀한 까닭', { size: 10, color: '#2f5066' });

  s += lbl(370, 272, '★넓이(도시가 눈뜨는 속도)와 깊이(판을 뒤집는 힘)가 따로 움직인다.', { size: 11.5, bold: true, color: '#4a3b2a' });
  s += lbl(370, 290, '세이르가 태워도 확산을 못 잡는 것과, 그래도 판을 뒤집는 건 몇 사람 손인 것 — 같은 법의 양면.', { size: 10.5, color: '#6b6353' });
  return svg(740, 302, s);
})();

module.exports = [
  { match: '마력과 길', html: respond, caption: '고대에도 체계는 있었다. 바뀐 것은 체계의 위상이다 — 한 방식에서 유일한 방식으로.' },
  { match: '조련의 법', html: taming, caption: '2026-07-17 사용자 확정. 현대 배경 전체가 이 세 조항 위에 앉는다.' },
  { match: '기술은 베껴지지 않는다', html: onlyDig, caption: '새로 못 쓰는 것 하나에서 현대 경제·사건·성장 구조가 전부 갈라져 나온다.' },
  { match: '조련의 계층', html: layerDepth, caption: '길마다 길든 깊이가 달라서, 개막은 넓게 퍼지면서 얕다.' },
  { match: '은닉 — 왜 티내지 않는가', html: hiding, caption: '이유의 비중은 사람마다 다른데, 어느 조합으로 셈해도 답이 은닉으로 나온다.' },
  { match: '세상이 아는 것', html: fourLayers, caption: '아는 사람이 적은 것은 숨겨서가 아니라 원래 몇 없어서다.' },
  { match: '물건이 실제로 오가는 길', html: leak, caption: '위층의 문화재 절차는 실제로 돌아간다. 은닉은 그 절차를 피해 가는 손으로만 성립한다.' },
  { match: '무엇이 퍼졌고 무엇이 깨졌나', html: opening, caption: '개막 전 세계는 네 조건 위에 서 있었고, 개막은 그 넷을 한꺼번에 무너뜨린다.' }
];
