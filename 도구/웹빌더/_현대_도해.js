// 현대전 배경 도해 모듈 — _build_md_열람.js 의 도해 인자(6번째)로 쓴다.
// 사용자 지시(07-17 21:01): 모든 페이지를 도감처럼 그림과 함께. + 07-18 현대전 배경 채우기.
// export = [{match:"제목 일부", html:"<svg…>", caption:"…"}] — 정본 md는 손대지 않는다.
// 빌드: node _build_md_열람.js 기획/01_세계관/3_현대/세계관_현대전_배경_정본.md 세계관_페이지_현대전배경.html "제목" "부제" "링크" _현대_도해.js

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

// ── A. 두 겹으로 된 지금 — 위층/아래층 단면 (§1) ──
const twoLayers = (() => {
  let s = '';
  s += lbl(360, 22, '현대 = 한 세계가 아니라 두 세계가 겹쳐 있다 (§1-1)', { size: 13, bold: true, color: '#3d382e' });
  // 위층 — 현대 도시 (밝은 하늘)
  s += `<rect x="14" y="40" width="692" height="118" rx="10" style="fill:#dfe6ee;stroke:#8a95a2;stroke-width:2"/>`;
  s += lbl(28, 62, '위층 — 사람들이 사는 세계', { size: 12, bold: true, color: '#3a4757', anchor: 'start' });
  // 도시 실루엣
  const bld = [[70, 96], [110, 70], [150, 88], [196, 60], [244, 84], [300, 66], [356, 92], [410, 58], [470, 80], [540, 68], [610, 90], [664, 72]];
  bld.forEach(([bx0, by0]) => {
    s += `<rect x="${bx0}" y="${by0}" width="30" height="${136 - by0}" style="fill:#c6ccd5;stroke:#8a95a2;stroke-width:1"/>`;
    for (let wy = by0 + 8; wy < 132; wy += 12) for (let wx = bx0 + 5; wx < bx0 + 26; wx += 9) s += `<rect x="${wx}" y="${wy}" width="4" height="5" style="fill:#8a95a2"/>`;
  });
  s += lbl(360, 152, '도시 · 행정 · 통신 · 시장 · 법 — 마력은 옛이야기·미신·마술쇼로만 안다', { size: 9.5, color: '#4a5460' });
  // 경계선(지표)
  s += ln(14, 168, 706, 168, { w: 2.5, color: '#5a4f3a' });
  s += lbl(700, 182, '지표 — 절멸이 끊은 기억의 경계', { size: 9, anchor: 'end', color: '#6b6353' });
  // 아래층 — 잊힌 마도 문명의 뼈 (어두운 지층)
  s += `<rect x="14" y="192" width="692" height="140" rx="10" style="fill:#3a3140;stroke:#5f4f6a;stroke-width:2"/>`;
  s += lbl(28, 214, '아래층 — 옛 세계의 잔재 (통째로 잠듦)', { size: 12, bold: true, color: '#d8cbe0', anchor: 'start' });
  // 묻힌 유적들
  s += bx(40, 226, 118, 56, '돌 유적·새김', ['읽을 수 없는 채', '박물관에 선다'], { fill: '#4a4152', stroke: '#7a6a8a', tcolor: '#e8dfe8' });
  s += bx(176, 240, 118, 56, '산속 사당', ['"신전이었을 것"', '으로 오독됨'], { fill: '#4a4152', stroke: '#7a6a8a', tcolor: '#e8dfe8' });
  s += bx(312, 226, 118, 56, '바다 밑 갑문', ['도시가 통째로', '가라앉은 자리'], { fill: '#4a4152', stroke: '#7a6a8a', tcolor: '#e8dfe8' });
  s += bx(448, 244, 118, 56, '기술서·비전', ['고대의 설계도', '— 여전히 선다'], { fill: '#5a3f4a', stroke: '#a13b2e', tcolor: '#f0d8d8' });
  s += bx(584, 228, 108, 56, '잠든 것', ['건드리지', '않는 자리'], { fill: '#4a4152', stroke: '#7a6a8a', tcolor: '#e8dfe8' });
  // 통로 — 기술서가 위로 샌다
  s += arrow(507, 244, 507, 170, { color: '#a13b2e', w: 2.5 });
  s += lbl(507, 300, '통로 = 기술서·비전이 위층으로 샌다 → 그 도시에서 다시 권능이 선다 (§1-1)', { size: 10, color: '#f0c8c8' });
  return {
    match: '세계의 현재',
    html: svg(720, 344, s),
    caption: '두 겹으로 된 지금 — 위층은 우리 지구와 거의 같은 현대 문명, 아래층은 발밑에 통째로 잠든 이름도 잊힌 마도 문명의 뼈. 둘을 잇는 유일한 통로가 기술서·비전이다. 옛 설계도 한 장이 위로 새어 들면 그 도시에서 권능이 다시 선다.',
  };
})();

// ── B. 능력의 두 시대 — 발현의 낙차 (§2) ──
const twoAges = (() => {
  let s = '';
  s += lbl(360, 22, '고대는 뜻만으로 됐고, 현대는 바깥 체계 없이는 아무것도 안 선다 (§2)', { size: 13, bold: true, color: '#3d382e' });
  // 고대
  s += bx(20, 44, 250, 118, '고대 — 마력이 세상의 인프라', [
    '순수 의지로 권능이 선다',
    '강을 잠그고 도시를 들어 올린다',
    '기술서는 "더 잘하게"의 도구일 뿐',
    '경지 = 측정 불가 (반신이 기후가 된다)'], { fill: '#efe6d2' });
  // 금제 벽
  s += `<rect x="300" y="44" width="70" height="118" rx="6" style="fill:#2a2230;stroke:#a13b2e;stroke-width:2.5"/>`;
  s += lbl(335, 92, '금제', { size: 13, bold: true, color: '#e8dfc6' });
  s += lbl(335, 110, '(우리만', { size: 9, color: '#c9a' });
  s += lbl(335, 122, '아는 층)', { size: 9, color: '#c9a' });
  s += lbl(335, 180, '마력 친화도 급락', { size: 9, color: '#a13b2e' });
  // 현대
  s += bx(400, 44, 300, 118, '현대 — 의지만으론 아무것도 안 된다', [
    '마력의 길은 여전히 품고 태어난다',
    '그러나 순수 의지로는 세상에 고정 안 됨',
    '반드시 바깥 체계에 기대야 선다 ↓',
    '능력자↔일반인 경계가 흐리다(관문=익힌 기술)'], { fill: '#dfe6ee', stroke: '#8a95a2' });
  s += arrow(270, 103, 300, 103, { w: 2 });
  s += arrow(370, 103, 400, 103, { w: 2, color: '#a13b2e' });
  // 다섯 길 = 바깥 체계
  const paths = [
    ['언령', '살란의 소리를 정확한 가락으로'],
    ['진(마법진)', '회로를 공간에 그려 박제'],
    ['연공', '몸에 마력의 길을 오래 벼림'],
    ['새김', '무기·부적·살갗에 문양 고정'],
    ['주술', '닮음·닿음의 매개로 인과의 홈'],
  ];
  paths.forEach((p, i) => {
    const x = 14 + i * 141;
    s += `<rect x="${x}" y="196" width="130" height="64" rx="8" style="fill:#faf6ec;stroke:#b3a67f;stroke-width:1.5"/>`;
    s += `<text x="${x + 65}" y="216" text-anchor="middle" style="font-size:11.5px;font-weight:bold;fill:#4a3b2a">${p[0]}</text>`;
    s += `<text x="${x + 65}" y="234" text-anchor="middle" style="font-size:8.6px;fill:#6b6353">${p[1].slice(0, 13)}</text>`;
    s += `<text x="${x + 65}" y="248" text-anchor="middle" style="font-size:8.6px;fill:#6b6353">${p[1].slice(13)}</text>`;
    s += ln(x + 65, 196, 550, 164, { dash: '2 3', w: 1 });
  });
  s += lbl(360, 186, '현대의 "바깥 체계" 다섯 길 — 이게 있어야 낮아진 친화도에서도 마력이 회로에 선다', { size: 10, color: '#4a5460' });
  return {
    match: '능력의 두 시대',
    html: svg(720, 272, s),
    caption: '발현의 낙차 — 고대는 순수 의지로 뭐든 했고, 현대는 금제(마력 친화도 급락)로 의지만으로는 아무것도 못 세운다. 그래서 반드시 바깥 체계(다섯 길)에 기대야 한다. ★"금제"는 우리만 아는 메타 — 현대 인물은 "친화도가 낮아졌다"까지만 안다.',
  };
})();

// ── C. 검은물 세 겹의 진실 (§3) ──
const threeTruths = (() => {
  let s = '';
  s += lbl(360, 22, '검은물(대절멸) — 아래로 팔수록 진실이 뒤집힌다 (§3-1)', { size: 13, bold: true, color: '#3d382e' });
  const layers = [
    ['겉 — 세상 통념', '거의 모두 (옛이야기로)', '"능력자가 너무 많아져 세상이 염증을 느껴 검은물로 청소당했다"', '→ 부분적으로만 맞음 · 능력자 억제·살해의 명분', '#efe8d6', '#b3a67f'],
    ['속 — 극소수 학자', '검은물을 파는 극히 일부', '"검은물이 세상의 마력 친화도를 낮춰 이제 의지로는 능력이 안 선다"', '→ 환경 탓으로 오독 · 금제라는 말은 여기서도 안 나온다', '#e2d8b8', '#8a7a5c'],
    ['참 — 우리·시스템만', '작가 · 플레이 후반 반전', '친화도 하락은 자연현상이 아니라 흑막의 의도적 금제 · 검은물의 진범은 통념과 다르다', '→ 누가·왜·언제 걸었나는 열어 둔다 (단일 흑막 금지)', '#3a3140', '#a13b2e'],
  ];
  layers.forEach((L, i) => {
    const y = 42 + i * 88, dark = i === 2;
    s += `<rect x="14" y="${y}" width="500" height="76" rx="9" style="fill:${L[4]};stroke:${L[5]};stroke-width:2"/>`;
    s += `<text x="30" y="${y + 24}" style="font-size:12.5px;font-weight:bold;fill:${dark ? '#e8dfc6' : '#4a3b2a'}">${L[0]}</text>`;
    s += `<text x="30" y="${y + 44}" style="font-size:10px;fill:${dark ? '#d8cbe0' : '#5f5744'}">${L[2]}</text>`;
    s += `<text x="30" y="${y + 62}" style="font-size:9.5px;fill:${dark ? '#c99' : '#8a5a2a'}">${L[3]}</text>`;
    // 누가 아는가 옆칸
    s += `<rect x="526" y="${y}" width="180" height="76" rx="9" style="fill:#faf6ec;stroke:#cfc4a8;stroke-width:1.5"/>`;
    s += `<text x="616" y="${y + 30}" text-anchor="middle" style="font-size:9px;fill:#6b6353">누가 아는가</text>`;
    s += `<text x="616" y="${y + 50}" text-anchor="middle" style="font-size:10px;font-weight:bold;fill:#4a3b2a">${L[1]}</text>`;
  });
  // 아래로 화살
  s += arrow(255, 118, 255, 130, { w: 2, color: '#a13b2e' });
  s += arrow(255, 206, 255, 218, { w: 2, color: '#a13b2e' });
  return {
    match: '검은물',
    html: svg(720, 316, s),
    caption: '세 겹의 진실 — 세상 거의 모두는 "능력자 과잉의 청소"(겉)로 알고, 극소수 학자만 "친화도가 낮아졌다"(속)까지 파며, 그것이 흑막의 의도적 금제였다(참)는 것은 우리·시스템만 안다. 반전의 재료는 겉과 참의 낙차. 현대 인물은 절대 "금제"를 입에 올리지 않는다.',
  };
})();

// ── D. 전쟁의 꼴 — 두 길 + 이념=표면 고대=뿌리 (§4) ──
const warShape = (() => {
  let s = '';
  s += lbl(360, 22, '왜 대낮에 능력이 안 터지는데 사람이 죽어 나가나 (§4)', { size: 13, bold: true, color: '#3d382e' });
  // 표적
  s += bx(285, 40, 150, 48, '표적', ['세력에 거슬리는 사람 · 자질자'], { fill: '#e7dcc3' });
  // 길 A
  s += arrow(320, 88, 200, 120, { w: 2 });
  s += bx(70, 122, 260, 66, '길 A — 흔적 없는 살해', [
    '능력자 표적을 능력으로 지우되',
    '뒤처리를 완벽히 → 사회엔 실종 한 건 · 살인 사건 자체가 성립 안 함'], { fill: '#dfe6ee', stroke: '#8a95a2' });
  // 길 B
  s += arrow(400, 88, 520, 120, { w: 2, color: '#a13b2e' });
  s += bx(390, 122, 300, 66, '길 B — 평범한 죽음 위장', [
    '능력을 한 톨도 안 씀 · 일반인의 이미 있는 충동',
    '(성실·분노·욕심·선의)을 표적에게 겨눔 → 사고·병·자살·우발범죄'], { fill: '#f0e2e2', stroke: '#a13b2e' });
  // 왜 안 대놓고: 노출=공멸
  s += `<rect x="14" y="204" width="692" height="40" rx="9" style="fill:#2a2230;stroke:#a13b2e;stroke-width:2"/>`;
  s += lbl(360, 220, '왜 안 대놓고 싸우나 — 노출은 공멸이다', { size: 11.5, bold: true, color: '#e8dfc6' });
  s += lbl(360, 236, '독점욕 · 재절멸 공포 · 일반인 방패 · 옛 마녀사냥의 재판을 안 부르려는 암묵 균형 (열린 목록)', { size: 9.5, color: '#d8cbe0' });
  // 이념=표면 / 고대=뿌리
  s += `<rect x="14" y="258" width="692" height="30" rx="8" style="fill:#e7dcc3;stroke:#b3a67f;stroke-width:1.5"/>`;
  s += lbl(120, 277, '표면 — 이념(명분)', { size: 10.5, bold: true, color: '#4a3b2a' });
  s += `<rect x="14" y="294" width="692" height="46" rx="8" style="fill:#4a4152;stroke:#7a6a8a;stroke-width:1.5"/>`;
  s += lbl(120, 312, '뿌리 — 고대의 흔적', { size: 10.5, bold: true, color: '#e8dfe8', anchor: 'middle' });
  ['은원(옛 원한)', '유물(기술서·능력 유물)', '혈통(흘러든 고대 가문)', '지형(옛 유적·잠든 것)'].forEach((t, i) => {
    s += lbl(300 + i * 100, 320, t, { size: 9, color: '#d8cbe0' });
  });
  s += arrow(360, 288, 360, 294, { w: 2, color: '#a13b2e' });
  s += lbl(360, 258, '★모든 사건 뒤에 고대가 깔려야 탄탄해진다 — 이념만 깔면 얄팍하다 (§4-2)', { size: 9.5, color: '#8a5a2a' });
  return {
    match: '전쟁의 꼴',
    html: svg(720, 352, s),
    caption: '전쟁의 두 길과 그 밑바닥 — 살인은 흔적 없는 살해(A)나 평범한 죽음 위장(B)으로 사회에서 사라진다. 대놓고 안 싸우는 건 노출이 공멸이기 때문. 그리고 표면의 이념 뒤에는 반드시 고대의 은원·유물·혈통·지형 중 하나가 깔려 있어야 "왜 하필 이 사람이"가 무서워진다.',
  };
})();

// ── E. 세력 다섯 역할의 판 (§5) ──
const factionRoles = (() => {
  let s = '';
  s += lbl(360, 22, '세력 구도 — 역할만 (★확정 이름 금지 · 소세력 난전) (§5)', { size: 13, bold: true, color: '#3d382e' });
  const roles = [
    ['A. 계승 쪽', ['절멸 이전 가문·유파를', '부분적으로 잇는 세력'], '유산·핏줄·옛 기술', 60, 60],
    ['B. 억제 쪽', ['늘어나는 능력자·기술서를', '지우려는 세력'], '재절멸 공포 / 독점욕', 400, 56],
    ['C. 복원 쪽', ['힘·기술·비전을 더', '많이 쥐려는 세력'], '강해지려는 욕망 / 야심', 68, 176],
    ['D. 규명 쪽', ['세계의 참에 가장', '근접한 소수'], '앎 / 밝히거나 지킴', 260, 234],
    ['E. 신참 쪽', ['절멸이 판을 갈아엎어', '새로 생긴 세력·구도'], '빈자리 / 기회', 452, 176],
  ];
  roles.forEach(r => {
    s += bx(r[3], r[4], 200, 84, r[0], [r[1][0], r[1][1], '동력: ' + r[2]], {});
  });
  // 난전 선(서로 얽힘)
  const centers = roles.map(r => [r[3] + 100, r[4] + 42]);
  for (let i = 0; i < centers.length; i++) for (let j = i + 1; j < centers.length; j++) {
    s += ln(centers[i][0], centers[i][1], centers[j][0], centers[j][1], { dash: '2 5', w: 1, color: '#c9bc9c' });
  }
  // 묶는 것 / 끊는 것
  s += `<rect x="14" y="300" width="340" height="44" rx="8" style="fill:#dfe6ee;stroke:#8a95a2;stroke-width:1.5"/>`;
  s += lbl(184, 318, '묶는 것 — 노출은 공멸이라는 공포', { size: 10.5, bold: true, color: '#3a4757' });
  s += lbl(184, 334, '난전을 그림자 싸움으로 눌러 둔다', { size: 9, color: '#4a5460' });
  s += `<rect x="366" y="300" width="340" height="44" rx="8" style="fill:#f0e2e2;stroke:#a13b2e;stroke-width:1.5"/>`;
  s += lbl(536, 318, '끊는 것 — 옛 원한·풀린 금서·너무 강한 자 하나', { size: 10.5, bold: true, color: '#a13b2e' });
  s += lbl(536, 334, '언제든 균형을 깬다', { size: 9, color: '#8a5a2a' });
  return {
    match: '세력 구도',
    html: svg(720, 356, s),
    caption: '다섯 역할의 판 — 계승 쪽·억제 쪽·복원 쪽·규명 쪽·신참 쪽. ★이건 구조적 역할이지 확정 조직이 아니다(파수/비상/온실/사무실 캐논화 금지). 각 역할 안에 온건↔강경이 갈려, 판은 4~5파전이 아니라 셀 수 없는 소세력의 난전에 가깝다.',
  };
})();

// ── F. 기술서 경제 — 유출→조각→거래→익힘 (§7) ──
const bookEconomy = (() => {
  let s = '';
  s += lbl(360, 20, '종이 한 장이 도시를 바꾼다 — 고대가 비대했던 만큼 물건의 가짓수가 방대하다 (§7)', { size: 12.5, bold: true, color: '#3d382e' });
  // 흐름 1: 아래층 유출
  s += bx(14, 40, 150, 54, '아래층 유출', ['묻혀 있던 고대 설계도가', '어디선가 새어 나온다'], { fill: '#4a4152', stroke: '#a13b2e', tcolor: '#f0d8d8' });
  s += arrow(164, 67, 200, 67, { w: 2, color: '#a13b2e' });
  // 흐름 2: 조각으로 흩어짐
  s += bx(200, 40, 150, 54, '조각으로 흩어짐', ['통권은 드물다 — 낱장·', '반권·베낀 조각으로'], {});
  s += arrow(350, 67, 386, 67, { w: 2 });
  // 흐름 3: 거래 자리
  s += bx(386, 40, 150, 54, '거래의 자리', ['경매·서점 금고·밀매선', '·사람 대 사람(빚·혼약)'], {});
  s += arrow(536, 67, 572, 67, { w: 2 });
  // 흐름 4: 익힘
  s += bx(572, 40, 134, 54, '익힘의 장벽', ['물건≠힘 — 스승·바탕·', '시간이 든다'], { fill: '#dfe6ee', stroke: '#8a95a2' });
  // 값을 정하는 축 (아래 5칸)
  s += lbl(360, 124, '값을 정하는 것들 (하나로 안 닫음) — 여러 축이 겹친다 (§7-1)', { size: 10.5, bold: true, color: '#4a3b2a' });
  const axes = [
    ['어느 길', '언령·진·연공·새김·주술'],
    ['몇 마디(T)', '입문 한 줄 ↔ 큰 주문'],
    ['진본/사본', '획 하나 어긋나면 무너짐'],
    ['온전/조각', '반권은 나머지를 찾아야'],
    ['딸린 위험', '소지=표적(금서·가문 비전)'],
  ];
  axes.forEach((a, i) => {
    const x = 14 + i * 141;
    s += `<rect x="${x}" y="134" width="130" height="52" rx="8" style="fill:#faf6ec;stroke:#b3a67f;stroke-width:1.5"/>`;
    s += `<text x="${x + 65}" y="154" text-anchor="middle" style="font-size:10.5px;font-weight:bold;fill:#4a3b2a">${a[0]}</text>`;
    s += `<text x="${x + 65}" y="172" text-anchor="middle" style="font-size:8.4px;fill:#6b6353">${a[1]}</text>`;
  });
  // 위조·조각 경고 띠
  s += `<rect x="14" y="200" width="692" height="32" rx="8" style="fill:#f0e2e2;stroke:#a13b2e;stroke-width:1.5"/>`;
  s += lbl(360, 220, '진본 아흔아홉에 가짜 하나 — 틀린 사본은 익히는 사람을 헛길로 보내거나 다치게 한다. 감별사의 눈이 곧 값이다 (§7-3)', { size: 9.5, color: '#8a5a2a' });
  return {
    match: '기술서 경제',
    html: svg(720, 244, s),
    caption: '기술서 경제 — 아래층에서 새어 나온 설계도가 조각으로 흩어져 경매·서점·밀매선·사람 대 사람으로 거래된다. 값은 길·마디·진위·온전함·위험이 겹쳐 정해지고, 물건을 쥔 자보다 익혀 낸 자가 귀하다. 반권을 쥔 자를 찾는 일, 위조를 가리는 일이 곧 사건이 된다.',
  };
})();

// ── G. 세력의 결 — 두 축의 사분면 (§8) ──
const factionSplit = (() => {
  let s = '';
  s += lbl(360, 20, '한 역할 안이 두 축으로 갈린다 — 적이 하나가 아니다 (§8-1)', { size: 12.5, bold: true, color: '#3d382e' });
  const cx = 360, cy = 190, R = 132;
  // 축
  s += arrow(cx - R - 18, cy, cx + R + 18, cy, { w: 2 });
  s += arrow(cx, cy + R + 18, cx, cy - R - 18, { w: 2 });
  s += lbl(cx - R - 24, cy + 4, '온건', { size: 11, bold: true, anchor: 'end', color: '#3a4757' });
  s += lbl(cx + R + 24, cy + 4, '강경', { size: 11, bold: true, anchor: 'start', color: '#a13b2e' });
  s += lbl(cx, cy - R - 24, '신념(진심으로 믿어서)', { size: 10.5, bold: true, color: '#4a3b2a' });
  s += lbl(cx, cy + R + 32, '실리(명분을 방패 삼아 이득)', { size: 10.5, bold: true, color: '#4a3b2a' });
  // 사분면 예시(억제 쪽 B 기준)
  const quad = [
    [cx - 96, cy - 78, '온건·신념', '등록·감시로 관리 —', '재절멸을 진심으로 두려워함'],
    [cx + 30, cy - 78, '강경·신념', '"의심스러우면 지운다" —', '능력자 제거 = 인류 구원'],
    [cx - 96, cy + 28, '온건·실리', '때를 기다리며 —', '조용히 기술·비전을 챙김'],
    [cx + 30, cy + 28, '강경·실리', '공포를 방패로 —', '독점을 위해 지운다'],
  ];
  quad.forEach(q => {
    s += `<rect x="${q[0]}" y="${q[1]}" width="126" height="50" rx="7" style="fill:#faf6ec;stroke:#cfc4a8;stroke-width:1.3"/>`;
    s += `<text x="${q[0] + 63}" y="${q[1] + 17}" text-anchor="middle" style="font-size:10px;font-weight:bold;fill:#4a3b2a">${q[2]}</text>`;
    s += `<text x="${q[0] + 63}" y="${q[1] + 32}" text-anchor="middle" style="font-size:8px;fill:#6b6353">${q[3]}</text>`;
    s += `<text x="${q[0] + 63}" y="${q[1] + 43}" text-anchor="middle" style="font-size:8px;fill:#6b6353">${q[4]}</text>`;
  });
  s += lbl(cx, cy + R + 52, '(예시 = 세이르 재단 쪽. 다섯 역할 각각이 이렇게 갈려 소세력 난전이 된다 — §8-2·8-3)', { size: 9, color: '#8a5a2a' });
  return {
    match: '세력의 결',
    html: svg(720, 384, s),
    caption: '세력의 결 — 한 역할 안이 온건↔강경(얼마나 멀리), 신념↔실리(왜 하는가) 두 축으로 갈린다. 그림은 억제 쪽을 예로 든 것: 같은 깃발 아래 네 속내가 다르게 움직인다. 다섯 역할 각각이 이렇게 갈리니 판은 몇 파전이 아니라 셀 수 없는 난전이다.',
  };
})();

// ── H. 담합의 실무 — 유지 비용과 정원 초과 (§9) ──
const noProof = (() => {
  let s = '';
  s += lbl(360, 20, '위층이 아직 모르는 것은 담합이다 — 세계가 아니라 사람이 매일 일해서 유지한다 (§9)', { size: 12.5, bold: true, color: '#3d382e' });
  // 위: 담합이 서는 까닭 (이득)
  s += `<rect x="24" y="34" width="672" height="40" rx="9" style="fill:#e8e0cf;stroke:#a99a78;stroke-width:2"/>`;
  s += lbl(360, 50, '왜 아무도 안 부나 — 드러나면 국가·법·군이 들어오고, 지금 쥔 독점·이동·사적 해결이 그날로 끝난다', { size: 10.5, bold: true, color: '#4a4230' });
  s += lbl(360, 66, '★약해서 못 알리는 게 아니라, 강해서 굳이 안 알린다 (못 알리는 약자로 쓰면 억지톤)', { size: 9.5, color: '#8a5a2a' });
  // 가운데: 유지 비용 다섯 쪽
  const cost = [
    ['죽음', ['검안의를 사서', '병사·사고사로 적게', '유족 없는 사람을 고름']],
    ['기록', ['지우지 않고 묻는다', '헛소리를 열 배로 부어', '괴담 사이에 섞음']],
    ['사람', ['죽이기보다 흔한 건', '먹여 주고 재우기', '입막음 = 생계']],
    ['물건', ['낱장을 후하게 사서', '거둬 태운다', '신고보다 싸다']],
    ['간판', ['재단·상단·의원', '얼굴 하나가 검안·언론·', '수사 셋을 눅인다']],
  ];
  cost.forEach((it, i) => {
    const x = 24 + i * 136;
    s += `<rect x="${x}" y="86" width="126" height="80" rx="8" style="fill:#dfe6ee;stroke:#8a95a2;stroke-width:2"/>`;
    s += `<text x="${x + 63}" y="104" text-anchor="middle" style="font-size:11px;font-weight:bold;fill:#3a4757">${it[0]} 쪽</text>`;
    it[1].forEach((t, j) => { s += `<text x="${x + 63}" y="${121 + j * 14}" text-anchor="middle" style="font-size:8.5px;fill:#4a5460">${t}</text>`; });
  });
  s += lbl(360, 180, '↑ 담합은 공짜가 아니다 — 이 값을 치르는 장면이 곧 현대의 사건이 된다 (§10 트릭 사전 = 이 실무의 사전)', { size: 9.5, color: '#5a5344' });
  // 아래: 정원 초과 → 무너짐
  s += `<rect x="120" y="194" width="480" height="62" rx="10" style="fill:#3a3140;stroke:#8a4a4a;stroke-width:2.5"/>`;
  s += lbl(360, 214, '★담합에는 정원이 있다 — 백 명의 비밀은 지켜지고 만 명의 비밀은 안 지켜진다', { size: 11.5, bold: true, color: '#f0dede' });
  s += lbl(360, 231, '개막이 늘리는 것은 능력자가 아니라 “서명한 적 없는 참가자” · 회수보다 유출이 빨라지는 지점', { size: 9, color: '#d8cbe0' });
  s += lbl(360, 247, '은폐를 서두를수록 은폐가 깨진다 (정교한 병사 → 거친 실종 → 눈에 띔)', { size: 9, color: '#e0b0b0' });
  s += lbl(360, 272, '플레이 시작 = 눈금이 막 흔들리기 시작한 순간 · 어디까지 무너지는가는 결말 자리(열어 둠)', { size: 9.5, color: '#8a5a2a' });
  return {
    match: '담합의 실무',
    html: svg(720, 286, s),
    caption: '위층이 모르는 것은 세계의 성질이 아니라 사람들이 만든 상태다 — 드러나면 다 잃으니 아무도 안 부는 담합. 다만 담합은 사람이 매일 값을 치러야 서고(검안·기록·생계·회수·간판), 참가자가 늘면 무너진다. 개막이 하는 일이 바로 그것이다.',
  };
})();

// ── I. 살인 트릭 — 공식과 위장 종류 (§10) ──
const murderGrid = (() => {
  let s = '';
  s += lbl(360, 20, '살인의 목적은 죽이는 게 아니라 위층이 능력을 모르게 하는 것 (§10)', { size: 12.5, bold: true, color: '#3d382e' });
  // 공식
  s += `<rect x="200" y="34" width="320" height="34" rx="8" style="fill:#e7dcc3;stroke:#b3a67f;stroke-width:1.5"/>`;
  s += lbl(360, 55, '공식 = 빌린 충동 × 겨눔의 매개 (곱이 무한)', { size: 11, bold: true, color: '#4a3b2a' });
  // 두 길
  s += bx(40, 84, 280, 48, '길 A — 능력으로 흔적 없이', ['시신을 안 남기거나 자연스러운 꼴로 → 실종'], { fill: '#dfe6ee', stroke: '#8a95a2' });
  s += bx(400, 84, 280, 48, '길 B — 일반인 충동을 겨눔', ['능력 한 톨 안 씀 → 사고·병·자살·우발범죄'], { fill: '#f0e2e2', stroke: '#a13b2e' });
  // 위장 5종
  const kinds = [
    ['실종', 'A', '유인·급습·위임 — 능력자 표적을 지움'],
    ['사고사', 'B', '배차/매뉴얼·유행 함정·군중 압사'],
    ['병사·자연사', 'B', '만성 독·서류 강등·데이터 조작'],
    ['단독범', 'B', '빌린 칼·자라난 미움·목격자 누명'],
    ['자살·선의', 'B', '피드 편집·틀린 응급처치'],
  ];
  kinds.forEach((k, i) => {
    const x = 14 + i * 141, ab = k[1] === 'A';
    s += `<rect x="${x}" y="146" width="130" height="72" rx="8" style="fill:#faf6ec;stroke:${ab ? '#8a95a2' : '#a13b2e'};stroke-width:1.5"/>`;
    s += `<text x="${x + 65}" y="166" text-anchor="middle" style="font-size:11px;font-weight:bold;fill:#4a3b2a">${k[0]}</text>`;
    s += `<circle cx="${x + 112}" cy="160" r="9" style="fill:${ab ? '#dfe6ee' : '#f0e2e2'};stroke:${ab ? '#8a95a2' : '#a13b2e'};stroke-width:1.3"/>`;
    s += `<text x="${x + 112}" y="164" text-anchor="middle" style="font-size:9px;font-weight:bold;fill:${ab ? '#3a4757' : '#a13b2e'}">${k[1]}</text>`;
    s += `<text x="${x + 65}" y="186" text-anchor="middle" style="font-size:8px;fill:#6b6353">${k[2].slice(0, 15)}</text>`;
    s += `<text x="${x + 65}" y="200" text-anchor="middle" style="font-size:8px;fill:#6b6353">${k[2].slice(15)}</text>`;
  });
  s += lbl(360, 236, '왜 못 잡나 — 초자연 수사도 능력의 흔적을 찾는데 길 B엔 능력이 없다. 답이 아니라 규칙에서 멈춘다 (§10-F)', { size: 9.5, color: '#8a5a2a' });
  return {
    match: '살인 트릭 카탈로그',
    html: svg(720, 250, s),
    caption: '은닉의 사전 — 죽음은 능력으로 흔적 없이 지우거나(길 A), 능력 한 톨 안 쓰고 일반인의 충동을 겨눠(길 B) 사회에서 사라진다. 다섯 위장 종류가 도시 죽음의 바다에 섞여 들고, 서로 연결이 없어 아무도 패턴을 못 본다.',
  };
})();

// ── J. 도시 단면 — 위층/아래층이 겹친 자리 (§11) ──
const cityCrossSection = (() => {
  let s = '';
  s += lbl(360, 20, '한 도시를 자르면 두 겹이 어떻게 포개져 있나 (§11 · 확정 지명 없음)', { size: 12.5, bold: true, color: '#3d382e' });
  // 위층
  s += `<rect x="14" y="36" width="692" height="86" rx="9" style="fill:#dfe6ee;stroke:#8a95a2;stroke-width:2"/>`;
  s += lbl(24, 54, '위층 — 사람들이 겹쳐 사는 도시', { size: 11, bold: true, color: '#3a4757', anchor: 'start' });
  s += bx(30, 62, 150, 50, '같은 아파트·회사', ['익힌 자·안 익힌 자', '뒤섞여 삼(숨긴다)'], { fill: '#eef3f8', stroke: '#8a95a2' });
  s += bx(196, 62, 160, 50, '기술서 도는 자리', ['헌책방 뒷금고·전당포', '·폐건물 지하 경매'], { fill: '#eef3f8', stroke: '#8a95a2' });
  s += bx(372, 62, 150, 50, '세력 간판 뒤', ['재단·연구소·상담소', '·경비회사'], { fill: '#eef3f8', stroke: '#8a95a2' });
  s += bx(538, 62, 156, 50, '죽음의 바다', ['설계된 죽음 몇 건이', '섞여 듦 — 패턴 안 보임'], { fill: '#eef3f8', stroke: '#8a95a2' });
  // 지표
  s += ln(14, 132, 706, 132, { w: 2.5, color: '#5a4f3a' });
  s += lbl(700, 146, '지표 — 굴착·준설이 우연히 건드리는 경계', { size: 8.5, anchor: 'end', color: '#6b6353' });
  // 아래층 새어나오는 자리
  s += `<rect x="14" y="156" width="692" height="104" rx="9" style="fill:#3a3140;stroke:#5f4f6a;stroke-width:2"/>`;
  s += lbl(24, 174, '아래층 — 새어 나오는 자리 (열린 목록)', { size: 11, bold: true, color: '#d8cbe0', anchor: 'start' });
  const under = [
    ['공사장', '읽을 수 없는 새김·회로', '"미상 유적"으로 덮음'],
    ['강 밑·해안', '갑문·성벽·저울대(돌만)', '잠긴 옛 도시의 뼈'],
    ['재개발 구역', '못 드는 돌·조각상 군집', '개발을 막는 "이상한 바위"'],
    ['오래된 사당', '주춧돌 — "신전 터" 오독', '관광지가 되거나 방치'],
  ];
  under.forEach((u, i) => {
    const x = 26 + i * 172;
    s += `<rect x="${x}" y="182" width="160" height="66" rx="7" style="fill:#4a4152;stroke:#7a6a8a;stroke-width:1.5"/>`;
    s += `<text x="${x + 80}" y="202" text-anchor="middle" style="font-size:10.5px;font-weight:bold;fill:#e8dfe8">${u[0]}</text>`;
    s += `<text x="${x + 80}" y="220" text-anchor="middle" style="font-size:8.2px;fill:#c9bcd0">${u[1]}</text>`;
    s += `<text x="${x + 80}" y="234" text-anchor="middle" style="font-size:8.2px;fill:#a898b0">${u[2]}</text>`;
    s += arrow(x + 80, 182, x + 80, 134, { w: 1.3, color: '#a13b2e' });
  });
  s += lbl(360, 278, '★왜 도시인가 — 익힐 사람·기술서 시장·죽음의 바다·발밑 유적이 큰 도시에서 한꺼번에 성립한다 (§11-4)', { size: 9.5, color: '#8a5a2a' });
  return {
    match: '도시 하나의 단면',
    html: svg(720, 292, s),
    caption: '도시 단면 — 위층에는 익힌 자와 안 익힌 사람이 뒤섞인 아파트, 기술서가 도는 헌책방 뒷금고, 세력 간판, 설계된 죽음이 섞인 통계가 있고, 지표 아래로는 공사장·강 밑·재개발 구역에서 옛 세계가 새어 나온다. 둘이 촘촘히 겹친 큰 도시가 현대전의 무대다.',
  };
})();

// ── K. 남은 흔적 — 생존 셈과 네 갈래 (§12) ──
const remnants = (() => {
  let s = '';
  s += lbl(360, 20, '절멸이 기억은 끊었어도 물건·몸짓·노래의 일부는 남았다 (§12)', { size: 12.5, bold: true, color: '#3d382e' });
  // 생존 셈 사다리
  s += lbl(360, 44, '생존 셈 — 만 년 단위에서 무엇이 남나', { size: 10.5, bold: true, color: '#4a3b2a' });
  const surv = [['돌', '#8a7a5c'], ['유리', '#9aa0a6'], ['뼈', '#c9bfa8'], ['쇠', '#a06a4a'], ['글', '#b0a888'], ['말', '#c8bfa0']];
  surv.forEach((sv, i) => {
    const x = 120 + i * 82;
    s += `<rect x="${x}" y="52" width="64" height="30" rx="6" style="fill:${sv[1]};stroke:#5a4f3a;stroke-width:1.3;opacity:${1 - i * 0.1}"/>`;
    s += `<text x="${x + 32}" y="72" text-anchor="middle" style="font-size:11px;font-weight:bold;fill:#2a2418">${sv[0]}</text>`;
    if (i < 5) s += lbl(x + 73, 71, '>', { size: 12, bold: true, color: '#5a4f3a' });
  });
  s += lbl(120, 96, '오래감 · 안 무너짐', { size: 8.5, anchor: 'start', color: '#6b6353' });
  s += lbl(596, 96, '쇠는 녹고 글은 삭는다', { size: 8.5, anchor: 'end', color: '#a13b2e' });
  // 네 갈래
  const branches = [
    ['순수 물성', ['다섯 손가락 고개·유리 언덕', '못 드는 돌·조각상 군집', '글자 0 → 절멸도 못 지움', '"설명 불가능한 층"'], '#f2ecdf', '#8a7a5c'],
    ['옛 도시의 뼈', ['갑문·성벽·이름 벽(돌만)', '운하의 금·쇠길 둑·주춧돌', '"신전"으로 오독됨', '읽을 수 없는 채 박물관에'], '#f2ecdf', '#b3a67f'],
    ['기술서·유물', ['묻힌 설계도가 새어 나옴', '= 현대 능력의 원천·표적', '아티팩트는 옛 위력의 그림자', '판 엎을 물건은 후속작 게이트'], '#5a3f4a', '#a13b2e'],
    ['몸짓·말·노래', ['뜻 잃고 꼴만 남은 관습', '홍수 전승 판본들·왕 신화', '자장가·셈 노래의 잔형', '가리키면 조잡한 역설계'], '#f2ecdf', '#8a7a5c'],
  ];
  branches.forEach((b, i) => {
    const x = 14 + i * 176, dark = b[2] !== '#f2ecdf';
    s += `<rect x="${x}" y="112" width="164" height="104" rx="8" style="fill:${b[2]};stroke:${b[3]};stroke-width:${dark ? 2 : 1.5}"/>`;
    s += `<text x="${x + 82}" y="132" text-anchor="middle" style="font-size:10.5px;font-weight:bold;fill:${dark ? '#f0d8d8' : '#4a3b2a'}">${b[0]}</text>`;
    b[1].forEach((t, j) => { s += `<text x="${x + 82}" y="${150 + j * 15}" text-anchor="middle" style="font-size:8.2px;fill:${dark ? '#e0c8c8' : '#6b6353'}">${t}</text>`; });
  });
  s += lbl(360, 234, '어떻게 살아남았는지는 비워 둔다 — 단일가설로 못박지 않는다', { size: 9, color: '#8a5a2a' });
  return {
    match: '남은 흔적 목록',
    html: svg(720, 246, s),
    caption: '남은 흔적 네 갈래 — 글자 없는 순수 물성이 가장 오래 남고(절멸도 못 지움), 옛 도시의 돌 뼈는 "신전"으로 오독되며, 기술서·유물은 현대 능력의 원천이자 싸움의 표적이 되고, 몸짓·노래는 뜻을 잃고 꼴만 남는다. 생존 셈: 돌>유리>뼈>쇠>글>말.',
  };
})();

// ── L. 다섯 길의 도시 발자국 (§13) ──
const pathFootprints = (() => {
  let s = '';
  s += lbl(360, 20, '각 길은 제 매체가 있어야 서니, 도시에 남기는 발자국이 다르다 (§13)', { size: 12.5, bold: true, color: '#3d382e' });
  // 은밀함 축 (세로 막대로 표시)
  const paths = [
    ['언령', '소리', 2, '즉발', '순간 화력 · 언령 대 언령 수싸움', '#c98a5a'],
    ['진', '그린 회로', 3, '설치', '미리 깐 함정 · 자리 선점', '#8a7ac9'],
    ['연공', '제 몸', 5, '즉발', '근접·도주·잠입 (가장 은밀)', '#5aa06a'],
    ['새김', '문신·부적·무기', 3, '즉발(예비)', '소지품이 곧 힘 · 뺏기면 끝', '#a0885a'],
    ['주술', '닮음·닿음 매개물', 4, '원격', '멀리서 건다 (가장 추적難)', '#7a9ab0'],
  ];
  paths.forEach((p, i) => {
    const x = 14 + i * 141;
    s += `<rect x="${x}" y="40" width="130" height="150" rx="9" style="fill:#faf6ec;stroke:${p[5]};stroke-width:2"/>`;
    s += `<text x="${x + 65}" y="62" text-anchor="middle" style="font-size:13px;font-weight:bold;fill:#4a3b2a">${p[0]}</text>`;
    s += `<text x="${x + 65}" y="80" text-anchor="middle" style="font-size:9px;fill:#6b6353">매체: ${p[1]}</text>`;
    // 은밀함 막대 (5칸)
    s += `<text x="${x + 65}" y="100" text-anchor="middle" style="font-size:8.5px;fill:#6b6353">은밀함</text>`;
    for (let k = 0; k < 5; k++) {
      s += `<rect x="${x + 20 + k * 18}" y="106" width="14" height="10" rx="2" style="fill:${k < p[2] ? p[5] : '#e7dcc3'};stroke:#b3a67f;stroke-width:0.8"/>`;
    }
    s += `<text x="${x + 65}" y="134" text-anchor="middle" style="font-size:9px;font-weight:bold;fill:${p[5]}">${p[3]}</text>`;
    s += `<text x="${x + 65}" y="154" text-anchor="middle" style="font-size:8px;fill:#6b6353">${p[4].slice(0, 12)}</text>`;
    s += `<text x="${x + 65}" y="167" text-anchor="middle" style="font-size:8px;fill:#6b6353">${p[4].slice(12)}</text>`;
    s += `<text x="${x + 65}" y="183" text-anchor="middle" style="font-size:7.5px;fill:#8a5a2a">발자국이 손버릇을 정함</text>`;
  });
  s += lbl(360, 210, '★대놓고 쓰면 노출(§9) → 싸움은 발자국 작은 길(연공·주술)·미리 깐 진·아예 능력 안 쓰는 길 B로 기운다', { size: 9.5, color: '#8a5a2a' });
  return {
    match: '다섯 길의 도시 발자국',
    html: svg(720, 224, s),
    caption: '다섯 길의 도시 발자국 — 언령은 소리가 새서 은밀함이 낮고, 진은 미리 깔아야 하고, 연공은 맨몸이라 가장 은밀하고, 새김은 소지품에 달렸고, 주술은 원격이라 추적이 어렵다. 어느 길에 능한가가 세력의 손버릇과 사건의 꼴을 정한다.',
  };
})();

// ── M. 반전의 층계 — 겉에서 참까지 (§14) ──
const reversalStairs = (() => {
  let s = '';
  s += lbl(360, 20, '한 번에 까지 말고 층층이 — 각 계단의 낙차가 반전이다 (§14)', { size: 12.5, bold: true, color: '#3d382e' });
  const stairs = [
    ['계단 1 — 겉 (도시 미스터리)', '평범한 죽음·실종이 이어진다 · 일반인 시점 · "이 도시 이상하다"', '분위기 = 느와르 수사극', '#dfe6ee', '#8a95a2', '#3a4757'],
    ['계단 2 — 속 (능력의 실재)', '익힌 자들·기술서 경제·세력의 그림자 싸움 · "세상 밑에 다른 판이 돈다"', '분위기 = 어반 판타지 스릴러', '#e7dcc3', '#b3a67f', '#4a3b2a'],
    ['계단 3 — 더 속 (역사의 반전)', '검은물·고대 마도 문명·능력자 제국의 흔적 · "우리는 잊힌 문명의 후예다"', '분위기 = 신화의 무게', '#d9cba6', '#8a7a5c', '#4a3b2a'],
    ['계단 4 — 참 (우리만 · 후반/후속작)', '친화도 하락은 의도적 금제 · 진범은 통념과 다르다 · "누군가 힘을 일부러 잠갔다"', '분위기 = 코즈믹한 배신 · ★현대 인물은 "금제"를 입에 안 올림', '#3a3140', '#5f4f6a', '#e8dfe8'],
  ];
  stairs.forEach((st, i) => {
    const x = 14 + i * 40, y = 44 + i * 62, w = 692 - i * 40, dark = i === 3;
    s += `<rect x="${x}" y="${y}" width="${w}" height="54" rx="9" style="fill:${st[3]};stroke:${st[4]};stroke-width:2"/>`;
    s += `<text x="${x + 16}" y="${y + 20}" style="font-size:11.5px;font-weight:bold;fill:${st[5]}">${st[0]}</text>`;
    s += `<text x="${x + 16}" y="${y + 36}" style="font-size:8.8px;fill:${dark ? '#d8cbe0' : '#5f5744'}">${st[1]}</text>`;
    s += `<text x="${x + 16}" y="${y + 49}" style="font-size:8.5px;fill:${dark ? '#c99' : '#8a5a2a'}">${st[2]}</text>`;
    if (i < 3) s += arrow(x + 20, y + 54, x + 40 + 20, y + 62, { w: 1.5, color: '#a13b2e' });
  });
  s += lbl(360, 316, '★계단 3·4의 무게는 고대가 얼마나 비대했냐에서 나온다 — 잠긴 것이 대단할수록 "누가 잠갔나"가 무섭다 (§14-2)', { size: 9.5, color: '#8a5a2a' });
  return {
    match: '반전의 층계',
    html: svg(720, 330, s),
    caption: '반전의 층계 — 겉(평범한 죽음)에서 속(세력전)으로, 더 속(고대 문명의 후예)으로, 참(누군가 힘을 잠갔다)으로 내려간다. 각 계단이 윗계단의 "당연함"을 뒤집는다. 계단 4의 언어는 우리만 알고, 그 "누가·왜"는 열어 둔다.',
  };
})();

// ── N. 세력의 자금·간판 — 세 겹 조직 (§15) ──
const frontStructure = (() => {
  let s = '';
  s += lbl(360, 20, '그림자에 있으면서 위층에서 돈과 사람을 굴리는 법 (§15)', { size: 12.5, bold: true, color: '#3d382e' });
  // 동심 세 겹
  const cx = 240, cy = 172;
  s += `<circle cx="${cx}" cy="${cy}" r="118" style="fill:#eef3f8;stroke:#8a95a2;stroke-width:2"/>`;
  s += `<circle cx="${cx}" cy="${cy}" r="78" style="fill:#f2ecdf;stroke:#b3a67f;stroke-width:2"/>`;
  s += `<circle cx="${cx}" cy="${cy}" r="40" style="fill:#3a3140;stroke:#5f4f6a;stroke-width:2"/>`;
  s += lbl(cx, cy - 4, '안쪽', { size: 11, bold: true, color: '#e8dfe8' });
  s += lbl(cx, cy + 12, '아는 사람', { size: 8.5, color: '#d8cbe0' });
  s += lbl(cx, cy - 62, '바깥 — 모르는 직원(방패)', { size: 9.5, bold: true, color: '#4a3b2a' });
  s += lbl(cx, cy - 48, '제가 무슨 조직인지 모름 = 무고', { size: 8, color: '#6b6353' });
  s += lbl(cx, cy - 100, '합법 간판 — 위층은 평범한 조직으로 안다', { size: 9.5, bold: true, color: '#3a4757' });
  s += lbl(cx, cy + 96, '부인 가능한 손(길 B의 일반인 도구)', { size: 8.5, color: '#8a5a2a' });
  s += lbl(cx, cy + 108, '잡혀도 세력으로 가는 다리 없음', { size: 8, color: '#8a5a2a' });
  // 자금 3원
  const funds = [
    ['지하 시장', '기술서·유물 거래 — 값 큰 건 빚·혼약·목숨'],
    ['위층 사업', '재단·기업·상담료 — 깨끗한 돈, 세탁 불필요'],
    ['후원 핏줄', '옛 가문을 계승 쪽의 유산 = 군자금'],
  ];
  funds.forEach((f, i) => {
    const y = 56 + i * 68;
    s += `<rect x="430" y="${y}" width="276" height="56" rx="8" style="fill:#faf6ec;stroke:#b3a67f;stroke-width:1.5"/>`;
    s += `<text x="568" y="${y + 22}" text-anchor="middle" style="font-size:11px;font-weight:bold;fill:#4a3b2a">${f[0]}</text>`;
    s += `<text x="568" y="${y + 40}" text-anchor="middle" style="font-size:8.6px;fill:#6b6353">${f[1]}</text>`;
    s += arrow(430, y + 28, 358, 172, { w: 1.2, color: '#8a7a5c' });
  });
  s += lbl(568, 44, '자금은 어디서 (열린 목록)', { size: 9.5, bold: true, color: '#4a3b2a' });
  s += lbl(360, 302, '★역설 — 간판이 평범할수록 안 걸리나, 들추면 무고한 직원·조직이 통째로 판에 휘말린다 (§15-3)', { size: 9.5, color: '#8a5a2a' });
  return {
    match: '세력의 자금과 간판',
    html: svg(720, 316, s),
    caption: '세력의 세 겹 조직 — 안쪽은 아는 사람이 판을 굴리고, 바깥은 아무것도 모르는 위층 직원이 방패가 되며, 더러운 일은 부인 가능한 손이 한다. 자금은 지하 시장·위층 사업·후원 핏줄에서 온다. 어느 역할이 어떤 간판을 쓰는지는 사건을 쓰며 정한다(확정 아님).',
  };
})();

// ── O. 시간축 — 개막 전과 후 (§16) ──
const timeline = (() => {
  let s = '';
  s += lbl(360, 20, '지금의 시끄러움은 오래된 상태가 아니다 — 개막을 경계로 세계가 달라졌다 (§16)', { size: 12, bold: true, color: '#3d382e' });
  // 축
  s += ln(30, 90, 690, 90, { w: 3 });
  s += arrow(690, 90, 700, 90, { w: 3 });
  // 개막 표식
  s += `<circle cx="360" cy="90" r="13" style="fill:#2a2230;stroke:#a13b2e;stroke-width:2.5"/>`;
  s += lbl(360, 94, '개막', { size: 8.5, bold: true, color: '#e8dfc6' });
  // 전
  s += bx(30, 108, 300, 100, '개막 전 — 오랜 침묵 (냉전)', [
    '익힌 자 손에 꼽음 · 저마다 숨어 삶',
    '기술서 극히 드묾 · 위층은 완전히 평온',
    '세력전 = 냉전(노출은 공멸, 물밑만)'], { fill: '#eef3f8', stroke: '#8a95a2' });
  // 방아쇠 (개막 순간)
  s += bx(250, 220, 220, 70, '개막 — 방아쇠 (★어느 쪽이 먼저인지 열림)', [
    '① 자질이 부쩍 눈뜬다(도시 단위)',
    '② 기술서가 한꺼번에 새어 나온다'], { fill: '#f0e2e2', stroke: '#a13b2e' });
  s += ln(360, 103, 360, 220, { dash: '3 3', w: 1.5, color: '#a13b2e' });
  // 후
  s += bx(390, 108, 300, 100, '개막 후 — 지금 (난전)', [
    '익힐 사람↑ 익힐 물건↑ → 곳곳에 권능이 섬',
    '자원 쟁탈·설계된 죽음↑ · 균형 흔들림',
    '강경·신념 쪽에 힘 · 담합이 아직 값을 감당'], { fill: '#e7dcc3', stroke: '#b3a67f' });
  s += lbl(360, 312, '★시계의 자리 — 개막이 가속되면? 노출 임박인가, 재절멸 공포가 현실인가, 전혀 다른 것인가 (열어 둠 §16-4)', { size: 9.5, color: '#8a5a2a' });
  s += lbl(360, 330, '무엇이 개막을 당겼나 = 반전의 씨앗 — 계단을 내려가면 참층의 금제와 닿으나 현대는 거기까지 모른다', { size: 9, color: '#6b6353' });
  return {
    match: '시간축',
    html: svg(720, 344, s),
    caption: '시간축 — 개막 전은 오랜 침묵의 냉전이었고, 개막(자질 눈뜸 + 기술서 유출이 겹침)을 지나 개막 후의 난전이 지금이다. 무엇이 개막을 당겼는지, 어느 쪽이 먼저였는지는 열려 있다 — 이것이 반전의 씨앗이자 현대가 끝내 못 푸는 물음이다.',
  };
})();

// ── P. 일반인 진입 — 겉층의 문 (§17) ──
const civilianEntry = (() => {
  let s = '';
  s += lbl(360, 20, '아직 능력이라는 말은 없다 — 어긋난 작은 것들로 시작한다 (§17)', { size: 12.5, bold: true, color: '#3d382e' });
  // 처음 만나는 것 4
  const seen = [
    ['어긋난 죽음의 연쇄', '하나씩은 평범한데 겹쳐 보면 어긋남'],
    ['변두리의 괴담', '진짜와 헛것이 섞여 아무도 안 믿음'],
    ['설명 안 되는 한 조각', '사진·흔적·메모 — 분류가 안 됨'],
    ['본 것 같은데 아닌 순간', '능력 한 자락 → 스스로 착시로 정리'],
  ];
  seen.forEach((v, i) => {
    const x = 14 + i * 176;
    s += `<rect x="${x}" y="40" width="164" height="60" rx="8" style="fill:#eef3f8;stroke:#8a95a2;stroke-width:1.5"/>`;
    s += `<text x="${x + 82}" y="62" text-anchor="middle" style="font-size:10px;font-weight:bold;fill:#3a4757">${v[0]}</text>`;
    s += `<text x="${x + 82}" y="80" text-anchor="middle" style="font-size:8px;fill:#4a5460">${v[1].slice(0, 16)}</text>`;
    s += `<text x="${x + 82}" y="93" text-anchor="middle" style="font-size:8px;fill:#4a5460">${v[1].slice(16)}</text>`;
    s += arrow(x + 82, 100, 360, 130, { w: 1.2, color: '#8a95a2' });
  });
  // 진입의 문 4
  s += `<rect x="220" y="128" width="280" height="30" rx="8" style="fill:#e7dcc3;stroke:#b3a67f;stroke-width:1.5"/>`;
  s += lbl(360, 148, '진입의 문 — 왜 이 사람이 발을 들이나 (하나로 안 닫음)', { size: 10, bold: true, color: '#4a3b2a' });
  const doors = ['상실 — 제 사람의 납득 안 되는 죽음', '직업 — 조사·검시·기자·보험', '호기심 — 조각을 못 놓는 기질', '우연한 목격 — 못 잊어 확인하러'];
  doors.forEach((d, i) => {
    const x = 14 + (i % 2) * 356, y = 170 + Math.floor(i / 2) * 40;
    s += `<rect x="${x}" y="${y}" width="346" height="32" rx="7" style="fill:#faf6ec;stroke:#cfc4a8;stroke-width:1.3"/>`;
    s += `<text x="${x + 173}" y="${y + 20}" text-anchor="middle" style="font-size:9.5px;fill:#5f5744">${d}</text>`;
  });
  s += `<rect x="120" y="262" width="480" height="34" rx="9" style="fill:#3a3140;stroke:#5f4f6a;stroke-width:2"/>`;
  s += lbl(360, 283, '★계단 1은 안전한 관찰이 아니라 이미 판에 한 발 걸친 위험 — 진입한 순간부터 시계가 돈다 (§17-3)', { size: 9.5, bold: true, color: '#e8dfe8' });
  return {
    match: '일반인 시점의 진입',
    html: svg(720, 308, s),
    caption: '겉층의 문 — 플레이어는 어긋난 죽음의 연쇄, 변두리의 괴담, 분류 안 되는 한 조각, 착시로 정리한 순간을 만난다. 상실·직업·호기심·목격 중 하나로 판에 발을 들이고, 그 순간 이미 위험 안에 있다 — 겉층을 잘 파는 일반인이 사라지는 것이 세계의 방어기제다.',
  };
})();

// ── Q. 세력 간 관계망 — 난전의 선 (§18) ──
const factionWeb = (() => {
  let s = '';
  s += lbl(360, 20, '고정된 동맹·적이 아니라 사안마다 얽힌다 (§18 · 확정 조직 아님)', { size: 12.5, bold: true, color: '#3d382e' });
  // 다섯 노드 배치
  const N = {
    B: [180, 100, '억제 쪽', '지운다'],
    C: [540, 100, '복원 쪽', '긁어모은다'],
    A: [360, 175, '계승 쪽', '유산·기술서·핏줄'],
    D: [180, 260, '규명 쪽', '밝히거나 지킴'],
    E: [540, 260, '신참 쪽', '와일드카드'],
  };
  // 선 먼저
  s += ln(N.B[0], N.B[1], N.C[0], N.C[1], { color: '#a13b2e', w: 2.5 });
  s += lbl(360, 92, '정면 충돌 — 목적이 정반대', { size: 9, bold: true, color: '#a13b2e' });
  ['B', 'C', 'D'].forEach(k => s += arrow(N[k][0], N[k][1] + (k === 'D' ? -20 : 20), N.A[0] + (N[k][0] < 360 ? -30 : N[k][0] > 360 ? 30 : 0), N.A[1], { color: '#8a7a5c', w: 1.5, dash: '4 3' }));
  s += lbl(300, 150, 'A=모두의 자원(표적·판돈)', { size: 8.5, anchor: 'end', color: '#6b6353' });
  s += ln(N.D[0], N.D[1], N.E[0], N.E[1], { color: '#c9bc9c', w: 1.2, dash: '2 4' });
  // 노드 상자
  Object.values(N).forEach(n => {
    const dark = n[2] === '규명 쪽';
    s += `<rect x="${n[0] - 76}" y="${n[1] - 26}" width="152" height="52" rx="9" style="fill:#f2ecdf;stroke:#8a7a5c;stroke-width:2"/>`;
    s += `<text x="${n[0]}" y="${n[1] - 4}" text-anchor="middle" style="font-size:11px;font-weight:bold;fill:#4a3b2a">${n[2]}</text>`;
    s += `<text x="${n[0]}" y="${n[1] + 14}" text-anchor="middle" style="font-size:8.5px;fill:#6b6353">${n[3]}</text>`;
  });
  // 묶는 것 / 끊는 것
  s += `<rect x="14" y="308" width="344" height="46" rx="8" style="fill:#dfe6ee;stroke:#8a95a2;stroke-width:1.5"/>`;
  s += lbl(186, 326, '묶는 것 — 노출·잠든 것 위기 앞엔', { size: 9.5, bold: true, color: '#3a4757' });
  s += lbl(186, 342, '원수도 하룻밤 손잡는다', { size: 9, color: '#4a5460' });
  s += `<rect x="366" y="308" width="340" height="46" rx="8" style="fill:#f0e2e2;stroke:#a13b2e;stroke-width:1.5"/>`;
  s += lbl(536, 326, '끊는 것 — 한 역할 안 강경·신념이 판을 쥘 때', { size: 9.5, bold: true, color: '#a13b2e' });
  s += lbl(536, 342, '(지나친 솎음·독점·폭로 강행)', { size: 9, color: '#8a5a2a' });
  s += lbl(360, 300, '★밝히려는 D(폭로)가 나타나면 적대하던 세력도 잠시 손잡아 막는다 (§18-1)', { size: 9, color: '#8a5a2a' });
  return {
    match: '세력 간 관계망',
    html: svg(720, 366, s),
    caption: '난전의 선 — 억제 쪽과 복원 쪽이 정면으로 부딪히고, 계승 쪽은 모두의 자원(표적·판돈)이며, 폭로 쪽은 모두의 적, 신참 쪽은 와일드카드다. 노출·잠든 것 위기 앞엔 원수도 손잡고, 한 역할 안 강경파가 판을 쥐면 균형이 깨진다. 굵은 선이 종종 한 자원·한 사람으로 수렴한다.',
  };
})();

// ── R. 전체 한눈 — 현대전 배경의 척추 (§19 capstone) ──
const capstone = (() => {
  let s = '';
  s += lbl(360, 22, '현대전 배경 한눈에 — 개막에서 참층까지 (§19)', { size: 13, bold: true, color: '#3d382e' });
  // 위: 개막
  s += bx(240, 40, 240, 44, '개막 (§16)', ['자질 눈뜸 + 기술서 유출이 겹침'], { fill: '#f0e2e2', stroke: '#a13b2e' });
  s += arrow(360, 84, 360, 104, { w: 2, color: '#a13b2e' });
  // 중: 싸움 (기술서 경제 / 세력 난전 / 은닉 살인)
  s += bx(30, 106, 200, 58, '기술서 경제 (§7)', ['종이 한 장이 도시를 바꿈', '조각·위조·익힘의 장벽'], {});
  s += bx(260, 106, 200, 58, '세력 난전 (§5·§18)', ['다섯 역할 A~E', '셀 수 없는 소세력 그물'], {});
  s += bx(490, 106, 200, 58, '은닉 살인 (§10)', ['길 A 흔적없이 / 길 B 일반인', '담합이 값을 치러 감춤 (§9)'], {});
  s += ln(130, 164, 360, 186, { dash: '3 3', w: 1.3 });
  s += ln(360, 164, 360, 186, { dash: '3 3', w: 1.3 });
  s += ln(590, 164, 360, 186, { dash: '3 3', w: 1.3 });
  // 아래: 반전의 층계 (계단으로 내려감)
  s += bx(210, 188, 300, 40, '반전의 층계 (§14)', ['겉 → 속 → 더속 → 참, 층층이 내려감'], { fill: '#e7dcc3' });
  s += arrow(360, 228, 360, 248, { w: 2, color: '#a13b2e' });
  // 바닥: 두 겹 세계 + 금제 (우리만 아는)
  s += `<rect x="30" y="250" width="410" height="70" rx="9" style="fill:#3a3140;stroke:#5f4f6a;stroke-width:2"/>`;
  s += lbl(235, 272, '아래층 = 잊힌 마도 문명 (§1·§12)', { size: 11, bold: true, color: '#d8cbe0' });
  s += lbl(235, 292, '★고대가 비대할수록 반전이 무겁다', { size: 9.5, color: '#c9bcd0' });
  s += lbl(235, 308, '(스케일 안 깎음 — §6)', { size: 8.5, color: '#a898b0' });
  s += `<rect x="460" y="250" width="230" height="70" rx="9" style="fill:#2a2230;stroke:#a13b2e;stroke-width:2.5"/>`;
  s += lbl(575, 272, '금제 (★우리만 아는)', { size: 11, bold: true, color: '#e8dfc6' });
  s += lbl(575, 292, '누군가 힘을 일부러 잠갔다', { size: 9, color: '#c99' });
  s += lbl(575, 308, '누가·왜·언제는 열어 둠', { size: 8.5, color: '#c99' });
  return {
    match: '용어 빠른참조',
    html: svg(720, 332, s),
    caption: '전체 한눈 — 개막(자질 눈뜸+기술서 유출)이 기술서 경제·세력 난전·은닉 살인을 낳고, 그 아래로 반전의 층계가 겉에서 참까지 내려간다. 바닥에는 잊힌 마도 문명과, 우리만 아는 금제가 있다. 이 세계의 무게는 전적으로 아래층(고대)이 얼마나 비대했냐에서 나온다.',
  };
})();

// ── S. 캐논 층위 — 무엇이 이기나 (§20) ──
const canonLayers = (() => {
  let s = '';
  s += lbl(360, 22, '두 문서를 같이 읽을 때 — 충돌하면 위가 이긴다 (§20)', { size: 13, bold: true, color: '#3d382e' });
  // 위계 3층
  s += bx(160, 44, 400, 48, '① 앵커 — 현대전과 T재조정 확정 (2026-07-18)', ['노트북LM 세션 사용자 확정 · 최상위'], { fill: '#e7dcc3', stroke: '#8a5a2a' });
  s += arrow(360, 92, 360, 108, { w: 2 });
  s += bx(160, 110, 400, 48, '② 이 정본 — 현대전 배경 (앵커에 살 붙임)', ['앵커와 충돌하면 앵커가 이김'], { fill: '#f2ecdf', stroke: '#b3a67f' });
  s += arrow(360, 158, 360, 174, { w: 2 });
  s += bx(160, 176, 400, 48, '③ 구식 v3 종합 (2026-06-26) = 참고 틀', ['폐기 아님 · 그러나 위 둘이 갈아엎은 곳은 무효'], { fill: '#eef3f8', stroke: '#8a95a2', dash: '6 4' });
  // 옆: 대체된 기제 용어(쓰지 않음)
  s += `<rect x="14" y="244" width="340" height="86" rx="9" style="fill:#f0e2e2;stroke:#a13b2e;stroke-width:1.5"/>`;
  s += lbl(184, 264, '새 정본에서 안 쓰는 기제 용어', { size: 10.5, bold: true, color: '#a13b2e' });
  ['봉인 · 고립계 · 흔적0%', '능력자 현실무력 · 반신만 예외', '파수/비상/온실/사무실 확정 · 성비 7:3'].forEach((t, i) => {
    s += lbl(184, 284 + i * 15, t, { size: 8.8, color: '#8a5a2a' });
  });
  // 옆: 유지되는 구조 감각
  s += `<rect x="366" y="244" width="340" height="86" rx="9" style="fill:#f2ecdf;stroke:#8a7a5c;stroke-width:1.5"/>`;
  s += lbl(536, 264, '유지하되 새 기제로 갈아 읽는 구조 감각', { size: 10.5, bold: true, color: '#4a3b2a' });
  ['검은물 단 한 번 · 살인 두 길', '노출=공멸 · 고대의 비대함', '잠든 자·유물·가문은 열어 둔 배경'].forEach((t, i) => {
    s += lbl(536, 284 + i * 15, t, { size: 8.8, color: '#6b6353' });
  });
  return {
    match: '구식 v3 종합과의 대조',
    html: svg(720, 342, s),
    caption: '캐논 층위 — 앵커가 최상위, 이 정본이 그 위에 살을 붙이고, 구식 v3는 참고 틀(폐기 아님)이나 위 둘이 갈아엎은 곳은 무효. 봉인·고립계·능력자 현실무력·성비 같은 기제 용어는 새 정본에서 안 쓰고, 검은물·살인 두 길·노출=공멸·고대의 비대함 같은 구조 감각은 새 기제로 갈아 읽는다.',
  };
})();

// ── T. 한 사건이 어떻게 맞물리나 — 수렴 + 연계 (§21) ──
const convergence = (() => {
  let s = '';
  s += lbl(360, 20, '배경이 작동하는 엔진임을 보이는 얼개 (§21 · ★확정 플롯 아님)', { size: 12.5, bold: true, color: '#3d382e' });
  // 가운데: 한 조각(사건의 핵)
  const cx = 360, cy = 168;
  s += `<circle cx="${cx}" cy="${cy}" r="46" style="fill:#5a3f4a;stroke:#a13b2e;stroke-width:2.5"/>`;
  s += lbl(cx, cy - 4, '한 조각', { size: 12, bold: true, color: '#f0d8d8' });
  s += lbl(cx, cy + 14, '(죽은 자가 쥔 것)', { size: 8, color: '#e0c8c8' });
  // 둘러싼 요소들이 조각으로 수렴
  const around = [
    [130, 70, '겉층의 죽음', '평범한 사고로 종결(§10)'],
    [590, 70, '억제 쪽 B', '회수·소각하려'],
    [640, 190, '복원 쪽 C', '나머지 반권을 찾아'],
    [560, 300, '규명 쪽 D', '무엇을 가리키나 읽으려'],
    [130, 300, '고대의 뿌리', '은원·가문·유적(§4-2·§12)'],
    [80, 190, '일반인 시점', '못 놓고 파고듦(§17)'],
  ];
  around.forEach(a => {
    s += `<rect x="${a[0] - 68}" y="${a[1] - 20}" width="136" height="44" rx="8" style="fill:#f2ecdf;stroke:#8a7a5c;stroke-width:1.5"/>`;
    s += `<text x="${a[0]}" y="${a[1] - 2}" text-anchor="middle" style="font-size:10px;font-weight:bold;fill:#4a3b2a">${a[1] ? a[2] : ''}</text>`;
    s += `<text x="${a[0]}" y="${a[1] + 14}" text-anchor="middle" style="font-size:8px;fill:#6b6353">${a[3]}</text>`;
    // 조각으로 향하는 선
    const dx = cx - a[0], dy = cy - a[1], d = Math.hypot(dx, dy);
    s += arrow(a[0] + dx / d * 70, a[1] + dy / d * 22, cx - dx / d * 50, cy - dy / d * 50, { w: 1.3, color: '#a13b2e', dash: '3 3', head: 7 });
  });
  s += lbl(cx, cy + 78, '한 조각으로 굵은 선들이 수렴 (§18-3)', { size: 8.5, color: '#8a5a2a' });
  // 아래: 연계 사슬
  s += lbl(360, 340, '요소들이 서로를 먹여 살린다 (연계의 그물 · 하나를 건드리면 나머지가 다 움직인다)', { size: 10, bold: true, color: '#4a3b2a' });
  const chain = ['개막', '기술서 경제', '세력 난전', '은닉 살인', '증명 부재', '고대 뿌리', '반전 층계'];
  chain.forEach((c, i) => {
    const x = 20 + i * 100;
    s += `<rect x="${x}" y="352" width="86" height="28" rx="6" style="fill:#e7dcc3;stroke:#b3a67f;stroke-width:1.3"/>`;
    s += `<text x="${x + 43}" y="370" text-anchor="middle" style="font-size:8.6px;font-weight:bold;fill:#4a3b2a">${c}</text>`;
    if (i < chain.length - 1) s += arrow(x + 86, 366, x + 100, 366, { w: 1.3, color: '#a13b2e', head: 6 });
  });
  return {
    match: '한 사건이 어떻게 맞물리나',
    html: svg(720, 392, s),
    caption: '통합 예시 — 겉층의 한 죽음이 실은 기술서 조각을 두고 벌어진 일이고, 억제 쪽·복원 쪽·규명 쪽이 그 한 조각으로 수렴하며, 살인은 길 B로 감춰지고, 조각의 뿌리엔 고대가 깔린다. 아래 사슬처럼 요소 하나를 건드리면 나머지가 다 움직인다 — 사건을 끝없이 낳는 엔진. ★확정 플롯이 아니라 작동을 보이는 얼개다.',
  };
})();

// ── U. 숨겨진 능력의 발굴 — 되찾는 루프 (§22) ──
const recovery = (() => {
  let s = '';
  s += lbl(360, 20, '고대의 잃어버린 능력을 되찾는 것 = 게임 진행의 엔진 (§22)', { size: 12.5, bold: true, color: '#3d382e' });
  // 발굴의 원천 5 (왼쪽 세로)
  s += lbl(112, 46, '발굴의 원천 (열린 목록)', { size: 10, bold: true, color: '#4a3b2a' });
  const src = ['유적 — 옛 도시·사당', '조각 해독 — 낱장·반권', '잊힌 가문의 비전', '살아있는 전승 재구성', '잠든 것 곁의 잔재(후속작)'];
  src.forEach((t, i) => {
    const y = 54 + i * 32;
    s += `<rect x="14" y="${y}" width="196" height="26" rx="6" style="fill:#4a4152;stroke:#7a6a8a;stroke-width:1.4"/>`;
    s += `<text x="112" y="${y + 17}" text-anchor="middle" style="font-size:9px;fill:#e8dfe8">${t}</text>`;
    s += arrow(210, y + 13, 250, 158, { w: 1, color: '#8a7a5c', dash: '3 3', head: 6 });
  });
  // 되찾는 과정 파이프라인 (가운데)
  const steps = ['조각', '해독', '익힘', '검증'];
  steps.forEach((t, i) => {
    const x = 250 + i * 78;
    s += `<rect x="${x}" y="144" width="64" height="34" rx="7" style="fill:#f2ecdf;stroke:#b3a67f;stroke-width:1.5"/>`;
    s += `<text x="${x + 32}" y="165" text-anchor="middle" style="font-size:11px;font-weight:bold;fill:#4a3b2a">${t}</text>`;
    if (i < 3) s += arrow(x + 64, 161, x + 78, 161, { w: 1.5, color: '#a13b2e', head: 6 });
  });
  s += lbl(382, 132, '되찾는 과정 — 물건이 곧 힘은 아니다 (각 단계가 관문)', { size: 9, bold: true, color: '#4a3b2a' });
  s += lbl(382, 194, '불완전 복원의 위험 — 틀린 사본·반권만 익히면 헛길·부상', { size: 8.5, color: '#8a5a2a' });
  // 결과 → 되살아난 능력
  s += arrow(562, 161, 610, 161, { w: 1.5, color: '#a13b2e' });
  s += `<rect x="612" y="140" width="94" height="42" rx="8" style="fill:#e7dcc3;stroke:#8a5a2a;stroke-width:2"/>`;
  s += lbl(659, 158, '되살아난', { size: 10, bold: true, color: '#4a3b2a' });
  s += lbl(659, 172, '한 길의 변주', { size: 9, color: '#6b6353' });
  // 아래: 발굴이 곧 하강 + 스케일
  s += `<rect x="14" y="220" width="340" height="70" rx="9" style="fill:#3a3140;stroke:#5f4f6a;stroke-width:2"/>`;
  s += lbl(184, 240, '발굴이 곧 하강 (§14와 한 줄)', { size: 10.5, bold: true, color: '#e8dfe8' });
  s += lbl(184, 258, '잃은 기술을 따라가면 → 그것을 만든 문명', { size: 8.6, color: '#d8cbe0' });
  s += lbl(184, 274, '→ 검은물 → 금제(참층). 능력 복원 = 진실 하강', { size: 8.6, color: '#c9bcd0' });
  s += `<rect x="366" y="220" width="340" height="70" rx="9" style="fill:#f0e2e2;stroke:#a13b2e;stroke-width:1.5"/>`;
  s += lbl(536, 240, '★스케일 = 콘텐츠', { size: 10.5, bold: true, color: '#a13b2e' });
  s += lbl(536, 258, '고대가 비대할수록 발굴할 것이 무진장', { size: 8.6, color: '#8a5a2a' });
  s += lbl(536, 274, '고대를 깎으면 = 게임의 깊이가 깎인다', { size: 8.6, color: '#8a5a2a' });
  return {
    match: '숨겨진 능력의 발굴',
    html: svg(720, 302, s),
    caption: '되찾는 루프 — 유적·조각·잊힌 비전·전승·잔재에서 발굴한 것을 조각→해독→익힘→검증으로 온전히 되찾으면, 잃어버렸던 한 길의 변주가 되살아난다. 발굴이 깊어질수록 고대의 진실(참층)에 가까워지고, 고대가 비대할수록 발굴 콘텐츠가 무진장해진다 — 스케일을 깎으면 게임의 깊이가 깎인다.',
  };
})();

// ── U. 시간이 흐르면 — 주인공 없는 현대의 시계 (§23) ──
const forwardClock = (() => {
  let s = '';
  s += lbl(360, 18, '주인공 없이도 세계는 굴러간다 — 개막이라는 장전된 현재의 시계 (§23)', { size: 12.5, bold: true, color: '#3d382e' });
  s += lbl(360, 36, '전진 동력: ①지식은 안 되감김 ②담합의 정원 ③발굴이 판돈↑ ④세대 교체 ⑤막을 손 부족 — 하나가 움직이면 다 따라 움직인다', { size: 9.5, color: '#7a5a2a' });
  // 국면 흐름 A→B→C
  s += bx(24, 54, 196, 92, '국면 A — 홍수 (개막~수년)', [
    '미숙한 눈뜸이 사고를 쏟음',
    '기술서 값 폭등 · 첫 위조',
    '억누름이 초반엔 먹힘 = 착시'], { fill: '#eef1f4', stroke: '#8a95a2' });
  s += arrow(224, 100, 258, 100, { w: 2.5 });
  s += bx(262, 54, 196, 92, '국면 B — 스며듦 (~십수 년)', [
    '필사가 회수를 앞지름(근본 실패)',
    '물건 흔해지고 사람 귀해짐',
    '첫 큰 되찾기 · 위층의 첫 눈'], { fill: '#e7dcc3', stroke: '#b3a67f' });
  s += arrow(462, 100, 496, 100, { w: 2.5 });
  s += bx(500, 54, 196, 92, '국면 C — 임계 (분기점)', [
    '담합의 정원이 넘침',
    '개별·우연으로 못 덮는 순간',
    '★세계가 여기서 갈린다'], { fill: '#f0e2e2', stroke: '#a13b2e', tcolor: '#7a2a20' });
  // C에서 네 갈래로 부챗살
  const bry = 206, brh = 82, brw = 160;
  const brx = [24, 196, 368, 540];
  const branches = [
    ['㉮ 드러남', ['제도가 능력을 손에 쥠', '등록·단속·무기화·합법시장', '판 전체가 바뀜'], '#dde7ee', '#5a7a95', '#2f4a5f'],
    ['㉯ 재봉인', ['큰 은폐로 밀도를 눌러 내림', '→ 새 냉전 (영구 아님)', '확산 살아 있어 시계 다시 돔'], '#e6e6e2', '#8a8a80', '#4a4a40'],
    ['㉰ 열전', ['압도적 되찾기·노선 폭발', '그림자 싸움 → 공공연한 전쟁', '위층은 피해로 알게 됨'], '#f0dcd6', '#a13b2e', '#7a2a20'],
    ['㉱ 참에 닿음', ['검은물·금제의 참층과 닿음', '싸움의 무대 자체가 거짓', '후반·후속작의 문'], '#e3dce8', '#7a6a8a', '#4a3f5a'],
  ];
  branches.forEach(([t, subs, fill, stroke, tc], i) => {
    s += arrow(598, 148, brx[i] + brw / 2, bry - 2, { dash: '4 3', w: 1.6, color: '#a13b2e' });
    s += bx(brx[i], bry, brw, brh, t, subs, { fill, stroke, tcolor: tc });
  });
  s += lbl(360, 172, '임계의 네 갈래 — 하나로 안 닫음 (도시마다 다를 수도, 한 세계가 여러 갈래를 겹쳐 지날 수도)', { size: 9.5, color: '#8a5a2a' });
  // 세대의 느린 시계 (맨 아래 축)
  s += ln(24, 322, 680, 322, { w: 2.5, color: '#6b6353' });
  s += arrow(680, 322, 696, 322, { w: 2.5, color: '#6b6353' });
  s += lbl(30, 316, '세대의 느린 시계 (수십 년)', { size: 9.5, bold: true, color: '#5f5744', anchor: 'start' });
  s += `<circle cx="150" cy="322" r="6" style="fill:#8a95a2"/>`;
  s += lbl(150, 344, '개막 세대', { size: 9, color: '#5f5744' });
  s += lbl(150, 357, '숨김을 몸으로 배움', { size: 8, color: '#8a8272' });
  s += `<circle cx="420" cy="322" r="6" style="fill:#b3a67f"/>`;
  s += lbl(420, 344, '다음 세대', { size: 9, color: '#5f5744' });
  s += lbl(420, 357, '태어날 때부터 당연', { size: 8, color: '#8a8272' });
  s += lbl(636, 344, '노출(㉮) 쪽으로 기움', { size: 9, color: '#2f4a5f' });
  s += lbl(636, 357, '단 ㉯·㉱가 되돌릴 수 있음', { size: 8, color: '#8a8272' });
  return {
    match: '시간이 흐르면',
    html: svg(720, 372, s),
    caption: '주인공 없는 현대의 시계 — 개막이라는 장전된 현재가 다섯 압력에 밀려 국면 A(홍수)→B(스며듦)→C(임계)로 저절로 굴러간다. 임계에서 세계는 네 갈래(드러남·재봉인·열전·참에 닿음)로 갈리되 하나로 닫지 않는다. 그 아래로 수십 년짜리 세대의 느린 시계가 흘러, 어느 갈래로 갔든 숨김의 규율을 서서히 헐겁게 만든다. 게임의 사건은 이 움직이는 시계 위 한 지점에 놓인다.',
  };
})();


// ── J. 값의 사다리 — 싼 방법일수록 종이에 자국이 남는다 (§10-0) ──
const costLadder = (() => {
  let s = '';
  s += lbl(360, 20, '값의 사다리 — 무엇이 싸고 무엇이 비싼가 (§10-0)', { size: 12.5, bold: true, color: '#3d382e' });
  s += lbl(24, 42, '↓ 값이 오른다 (사람 수 × 시간 × 솜씨)', { size: 9.5, anchor: 'start', color: '#7a6a4a' });
  s += lbl(468, 42, '↓ 종이 자국이 줄어든다', { size: 9.5, anchor: 'end', color: '#7a6a4a' });
  const tiers = [
    ['가장 쌈 — 며칠 · 한둘 · 돈 조금', '서류 한 장(등급 강등) · 빌린 칼 · 게시판에 헛소리 붓기', '자국: 서류 · 전산 기록 · 통화 — 종이에 남는다(아무도 안 볼 뿐)', '#f7f0da', '#c9a54e', '#6b5a24'],
    ['중간 — 몇 주~몇 달 · 서넛', '배차와 매뉴얼 · 만성 독(간병) · 자라난 미움', '자국: 바뀐 배차표 · 약 유통 단계 하나 · 몇 달치 게시글', '#eee6d2', '#b3a67f', '#5f5744'],
    ['비쌈 — 사람 · 솜씨 · 시간 전부', '데이터 살해 · 틀린 응급처치 · 화면에 몰린 사람', '자국 거의 없음. 대신 짤 줄 아는 사람이 도시에 몇 없다', '#e2e6ea', '#8a95a2', '#3a4757'],
    ['다른 종류로 비쌈 (길 A) — 능력으로 지우기', '유인 · 급습 · 위임', '종이 자국 0. 대신 익힌 다른 사람의 눈에 걸린다', '#3a3140', '#5f4f6a', '#e8dfe8'],
  ];
  tiers.forEach((it, i) => {
    const y = 52 + i * 62;
    s += `<rect x="24" y="${y}" width="444" height="54" rx="8" style="fill:${it[3]};stroke:${it[4]};stroke-width:2"/>`;
    s += `<text x="36" y="${y + 18}" style="font-size:11.5px;font-weight:bold;fill:${it[5]}">${it[0]}</text>`;
    s += `<text x="36" y="${y + 34}" style="font-size:9.5px;fill:${it[5]}">${it[1]}</text>`;
    s += `<text x="36" y="${y + 48}" style="font-size:9px;fill:${it[5]};opacity:0.85">${it[2]}</text>`;
  });
  s += bx(492, 58, 204, 62, '위층의 눈', ['서류 · 부검 · 수사', '보험 · 인력 기록'], { fill: '#dfe6ee', stroke: '#8a95a2', tcolor: '#3a4757' });
  s += bx(492, 238, 204, 62, '아래층의 눈', ['익힌 다른 사람', '누가 어디서 무엇을 세웠나'], { fill: '#3a3140', stroke: '#5f4f6a', tcolor: '#e8dfe8' });
  s += arrow(470, 78, 490, 84, { w: 1.6, color: '#8a95a2' });
  s += arrow(470, 140, 490, 106, { w: 1.6, color: '#8a95a2' });
  s += arrow(470, 202, 490, 252, { w: 1.6, color: '#7a6a8a' });
  s += arrow(470, 266, 490, 272, { w: 2, color: '#7a6a8a' });
  s += lbl(594, 152, '한쪽을 피하면', { size: 10, color: '#5f5744' });
  s += lbl(594, 168, '다른 쪽에 가까워진다', { size: 10.5, bold: true, color: '#5f5744' });
  s += lbl(594, 192, '둘 다 피하는 방법은', { size: 9.5, color: '#8a5a2a' });
  s += lbl(594, 207, '아직 아무도 못 찾았다', { size: 9.5, color: '#8a5a2a' });
  s += `<rect x="24" y="314" width="672" height="54" rx="9" style="fill:#f0e2e2;stroke:#8a4a4a;stroke-width:2.5"/>`;
  s += lbl(360, 334, '★개막이 사다리를 기울인다 — 표적이 늘면 비싼 방법을 다 쓸 수가 없다(짤 사람도, 몇 달의 시간도 모자란다)', { size: 10.5, bold: true, color: '#5a2a2a' });
  s += lbl(360, 352, '→ 싼 쪽 비중이 오른다 → 싼 쪽은 종이에 자국을 남긴다 → 부고 · 검안 · 보험 서류를 오래 보는 사람에게 걸린다', { size: 9.5, color: '#6a3a3a' });
  s += lbl(360, 388, '담합은 게을러서 무너지는 게 아니라 바빠서 무너진다.', { size: 12, bold: true, color: '#8a5a2a' });
  return {
    match: '값의 사다리',
    html: svg(720, 400, s),
    caption: '같은 죽음이라도 값이 다르다 — 값은 사람 수 × 시간 × 되돌릴 수 없음 × 남는 자국으로 정해진다. 핵심은 싼 방법일수록 종이에 자국이 남고, 자국이 안 남는 방법일수록 비싸다는 것. 그래서 고르는 일은 위층의 눈과 아래층의 눈 사이에서 고르는 일이 되고, 개막이 표적을 늘리면 싼 쪽으로 기울어 종이 자국이 늘어난다.',
  };
})();

module.exports = [twoLayers, twoAges, threeTruths, warShape, factionRoles, bookEconomy, factionSplit, noProof, costLadder, murderGrid, cityCrossSection, remnants, pathFootprints, reversalStairs, frontStructure, timeline, civilianEntry, factionWeb, capstone, canonLayers, convergence, recovery, forwardClock];
