/* ============================================================
   라셀 도시 뼈대 생성기  —  _build_rasel_plan.js
   ------------------------------------------------------------
   ★원칙: 라셀은 "레벨 여러 개"가 아니라 **끊기지 않은 하나의 공간**이다.
     세계 좌표 = 미터. 원점(0,0) = 장터 큰길 한복판. +X 동(내륙·상류) / +Y 북.
     레벨은 그 공간 안의 *자리*일 뿐, 공간을 자르는 단위가 아니다.
   ★도시 구조는 현실 항구도시를 따른다:
     구시가 = 불규칙 유기 가로망(작은 블록·굽은 길·광장)
     신시가 = 격자 + 대각 가로수길 + 공원
     업무지구 = 초대형 블록 + 광폭 대로 + 수변 산책로
     항만    = 손가락 잔교 + 창고 열 + 철도 야드
     공업    = 성긴 대형 필지
   실행: node _build_rasel_plan.js   →  세계관_라셀_도시뼈대.html
   ============================================================ */
const fs = require('fs');

/* ---------- 결정적 난수 ---------- */
let _s = 20260725;
const rnd = () => { _s = (_s * 1664525 + 1013904223) % 4294967296; return _s / 4294967296; };
const rr = (a, b) => a + (b - a) * rnd();
const reseed = v => { _s = v; };

/* ---------- 기하 도우미 ---------- */
const lerp = (a, b, t) => a + (b - a) * t;
function catmull(pts, per = 12) {          // 굽은 길·물길을 매끄럽게
  const P = [pts[0], ...pts, pts[pts.length - 1]], out = [];
  for (let i = 1; i < P.length - 2; i++) {
    for (let j = 0; j < per; j++) {
      const t = j / per, t2 = t * t, t3 = t2 * t;
      const f = k => 0.5 * ((2 * P[i][k]) + (-P[i - 1][k] + P[i + 1][k]) * t +
        (2 * P[i - 1][k] - 5 * P[i][k] + 4 * P[i + 1][k] - P[i + 2][k]) * t2 +
        (-P[i - 1][k] + 3 * P[i][k] - 3 * P[i + 1][k] + P[i + 2][k]) * t3);
      out.push([f(0), f(1)]);
    }
  }
  out.push(pts[pts.length - 1]);
  return out;
}
function ribbon(center, w0, w1) {           // 중심선 → 폭 있는 띠(물길·대로)
  const n = center.length, L = [], R = [];
  for (let i = 0; i < n; i++) {
    const p = center[i], a = center[Math.max(0, i - 1)], b = center[Math.min(n - 1, i + 1)];
    let dx = b[0] - a[0], dy = b[1] - a[1];
    const d = Math.hypot(dx, dy) || 1; dx /= d; dy /= d;
    const w = lerp(w0, w1, i / (n - 1)) / 2;
    L.push([p[0] - dy * w, p[1] + dx * w]);
    R.push([p[0] + dy * w, p[1] - dx * w]);
  }
  return L.concat(R.reverse());
}
function distToPolyline(x, y, pts) {
  let best = 1e9;
  for (let i = 0; i < pts.length - 1; i++) {
    const [x1, y1] = pts[i], [x2, y2] = pts[i + 1];
    const dx = x2 - x1, dy = y2 - y1, L2 = dx * dx + dy * dy || 1;
    let t = ((x - x1) * dx + (y - y1) * dy) / L2; t = Math.max(0, Math.min(1, t));
    const d = Math.hypot(x - (x1 + t * dx), y - (y1 + t * dy));
    if (d < best) best = d;
  }
  return best;
}
const inPoly = (x, y, poly) => {
  let c = false;
  for (let i = 0, j = poly.length - 1; i < poly.length; j = i++) {
    const [xi, yi] = poly[i], [xj, yj] = poly[j];
    if ((yi > y) !== (yj > y) && x < (xj - xi) * (y - yi) / (yj - yi) + xi) c = !c;
  }
  return c;
};
const centroid = p => {
  let x = 0, y = 0; p.forEach(q => { x += q[0]; y += q[1]; });
  return [x / p.length, y / p.length];
};
const inset = (p, k) => { const c = centroid(p); return p.map(q => [lerp(c[0], q[0], k), lerp(c[1], q[1], k)]); };

/* ============================================================
   1. 물 — 사일란 바다 + 삼각주 세 물길 + 마른 골
   ============================================================ */
const COAST = catmull([[-5100, 4600], [-4850, 3200], [-5250, 2000], [-4780, 700],
[-5050, -600], [-4700, -1900], [-5150, -3200], [-4900, -4600]], 14);

const RIVER_MAIN = catmull([[7200, 700], [5900, 780], [4900, 640], [4200, 520]], 14);
const CH1 = catmull([[4200, 520], [3200, 900], [2100, 1180], [900, 1290],
[-400, 1360], [-1900, 1560], [-3300, 1850], [-4600, 2150], [-5600, 2400]], 16);
const CH2 = catmull([[4200, 520], [3300, 60], [2200, -430], [900, -760],
[-600, -960], [-2100, -1120], [-3600, -1220], [-5000, -1280]], 16);
const CH3 = catmull([[4050, 440], [3200, -520], [2100, -1420], [800, -2020],
[-800, -2380], [-2600, -2560], [-4200, -2680], [-5300, -2760]], 16);
const DRY = catmull([[4350, 700], [4900, 1500], [5400, 2200], [5900, 2750], [6300, 3050]], 14);

const WATERS = [
  { poly: ribbon(RIVER_MAIN, 560, 520), name: null },
  { poly: ribbon(CH1, 420, 780), name: '첫째 물길' },
  { poly: ribbon(CH2, 300, 460), name: '둘째 물길' },
  { poly: ribbon(CH3, 260, 420), name: '셋째 물길' },
];
const SEA = [[-9000, 5000], ...COAST, [-9000, -5000]];

/* ============================================================
   2. 큰 가로 — 실제 도시처럼 굽고, 갈라지고, 광장에서 모인다
   ============================================================ */
const ST = {
  // 구시가의 등뼈. 옛 물가를 따라 생긴 길이라 곧지 않다.
  jangteo: {
    cls: 'main', w: 26, name: '장터 큰길',
    pts: catmull([[-1500, -180], [-900, -60], [-420, 40], [0, 0], [430, -70], [900, -40], [1400, 60]], 10)
  },
  // 큰길이 서로 이어져 부두까지 — 도시의 동맥
  budu: {
    cls: 'arterial', w: 34, name: '부두길',
    pts: catmull([[-1500, -180], [-2100, -260], [-2800, -220], [-3500, -120], [-4200, 40]], 10)
  },
  // 첫째 물길 남안 강변대로
  gangbyeon: {
    cls: 'arterial', w: 32, name: '강변대로',
    pts: catmull([[1900, 830], [900, 900], [-300, 960], [-1600, 1120], [-2900, 1380], [-3900, 1620]], 12)
  },
  // 남쪽 간선 (둘째 물길 북안)
  namdaero: {
    cls: 'arterial', w: 30, name: '남대로',
    pts: catmull([[2100, -180], [1100, -430], [-100, -600], [-1400, -720], [-2700, -800], [-3800, -860]], 12)
  },
  // 구시가 ↔ 강변 남북 길 셋
  n1: { cls: 'sec', w: 20, name: null, pts: catmull([[-880, -80], [-940, 320], [-1000, 780], [-1050, 1110]], 8) },
  n2: { cls: 'sec', w: 22, name: null, pts: catmull([[-30, 10], [40, 400], [30, 780], [-10, 960]], 8) },
  n3: { cls: 'sec', w: 20, name: null, pts: catmull([[880, -30], [930, 340], [900, 700], [880, 890]], 8) },
  s1: { cls: 'sec', w: 20, name: null, pts: catmull([[-100, -10], [-140, -300], [-110, -560], [-90, -700]], 8) },
  s2: { cls: 'sec', w: 18, name: null, pts: catmull([[700, -50], [760, -320], [800, -560]], 8) },
  // 학당가 대각 가로수길 (신시가 특유)
  hakdang: {
    cls: 'sec', w: 24, name: '학당 가로수길',
    pts: catmull([[1100, -420], [1450, -60], [1750, 330], [1980, 720]], 10)
  },
  // 위층 섬 광폭 대로 + 수변 산책로
  uicheung: { cls: 'arterial', w: 44, name: '유리탑 대로', pts: catmull([[-300, 2500], [600, 2560], [1500, 2600], [2400, 2620], [3000, 2600]], 12) },
  promenade: { cls: 'sec', w: 20, name: '수변 산책로', pts: catmull([[-350, 2020], [600, 2060], [1600, 2100], [2600, 2130]], 12) },
  // 신항 공업 간선
  sinhang: { cls: 'sec', w: 26, name: '신항 진입로', pts: catmull([[600, -1500], [-200, -1780], [-1100, -1900], [-1900, -1930]], 10) },
  // 부두 안벽 도로
  quay: { cls: 'sec', w: 22, name: '안벽로', pts: catmull([[-3100, 560], [-3600, 480], [-4100, 300], [-4350, 0], [-4200, -400], [-3700, -560]], 12) },
};
const STREETS = Object.values(ST);

/* ============================================================
   3. 다리 — 물길을 건너는 자리
   ============================================================ */
const BRIDGES = [
  { name: '북다리', pts: [[-1030, 1080], [-1120, 1760]] },
  { name: '가운데다리', pts: [[-10, 940], [-60, 1720]] },
  { name: '동다리', pts: [[880, 870], [900, 1620]] },
  { name: '남다리', pts: [[-120, -580], [-160, -1300]] },
  { name: '신항다리', pts: [[700, -800], [640, -1480]] },
];

/* ============================================================
   4. 구역 — 각기 다른 도시 조직
   ============================================================ */
const DISTRICTS = [
  { id: 'A', name: '아랫장터 (구시가)', kind: 'organic', rect: [-1600, -700, 1000, 950], cell: [78, 66], jit: 0.42, rot: -0.10, fill: '#2a2318' },
  { id: 'B', name: '학당가·의원권', kind: 'grid', rect: [980, -700, 2350, 900], cell: [128, 104], jit: 0.10, rot: 0.16, fill: '#242a19' },
  { id: 'W', name: '서편 셋집 구역', kind: 'grid', rect: [-3000, -820, -1560, 1200], cell: [112, 96], jit: 0.16, rot: -0.05, fill: '#262018' },
  { id: 'Q', name: '부두 (바다 목)', kind: 'quay', rect: [-4500, -700, -3000, 800], cell: [190, 120], jit: 0.06, rot: 0.02, fill: '#1e2429' },
  { id: 'D', name: '강 건너 유리탑 (위층 섬)', kind: 'super', rect: [-500, 1950, 3050, 3350], cell: [330, 250], jit: 0.10, rot: 0.03, fill: '#1d242a' },
  { id: 'C', name: '신항 재개발지', kind: 'industrial', rect: [-2100, -2400, 1100, -1450], cell: [280, 190], jit: 0.12, rot: -0.04, fill: '#2a1e19' },
  { id: 'E2', name: '동편 새 동네', kind: 'grid', rect: [2350, -600, 3600, 800], cell: [150, 120], jit: 0.12, rot: 0.10, fill: '#242519' },
  { id: 'F', name: '옛 사르간 도읍 폐허', kind: 'ruin', rect: [5600, 2700, 6900, 3700], cell: [120, 100], jit: 0.34, rot: 0.10, fill: '#221d15' },
];

/* 광장·공원 — 블록을 비워 만든다 (현실 도시의 결절점). 정원이 아니라 제각각인 다각형 */
function blob(x, y, r, n, wob, seed) {
  reseed(seed); const p = [];
  for (let i = 0; i < n; i++) {
    const a = i / n * Math.PI * 2, rad = r * (1 - wob / 2 + rnd() * wob);
    p.push([x + Math.cos(a) * rad, y + Math.sin(a) * rad * 0.82]);
  }
  return p;
}
const VOIDS = [
  { x: -430, y: 60, r: 165, kind: 'square', name: '장터 마당', poly: blob(-430, 60, 165, 11, .55, 11) },
  { x: 1520, y: 210, r: 180, kind: 'park', name: '학당 앞뜰', poly: blob(1520, 210, 180, 10, .38, 22) },
  { x: 1180, y: 2560, r: 210, kind: 'park', name: '유리탑 광장', poly: blob(1180, 2560, 210, 9, .28, 33) },
  { x: -3450, y: 120, r: 140, kind: 'yard', name: '철도 야드', poly: blob(-3450, 120, 140, 8, .22, 44) },
  { x: 240, y: -190, r: 92, kind: 'square', name: '샛마당', poly: blob(240, -190, 92, 9, .5, 55) },
];

/* 골목 — 구시가를 유기적으로 만드는 잔가지. 마당과 큰길에서 갈라진다 */
const LANES = [
  catmull([[-430, 60], [-560, 250], [-620, 470], [-560, 700]], 8),
  catmull([[-430, 60], [-330, 300], [-180, 520], [-120, 760]], 8),
  catmull([[-430, 60], [-520, -140], [-600, -380], [-560, -600]], 8),
  catmull([[-430, 60], [-240, 170], [-30, 210], [200, 196], [430, 240], [640, 320]], 10),
  catmull([[86, 196], [330, 250], [560, 300], [780, 420]], 8),
  catmull([[240, -190], [120, -380], [60, -600]], 8),
  catmull([[240, -190], [470, -250], [700, -300], [880, -420]], 8),
  catmull([[-900, -60], [-820, 220], [-880, 480], [-960, 700]], 8),
  catmull([[430, -70], [520, 120], [560, 380], [500, 640]], 8),
  catmull([[-1180, -120], [-1240, 180], [-1180, 520]], 8),
].map(pts => ({ pts, w: 11 }));

/* 변화 있는 간격의 축 — 균일 격자를 깨는 핵심 */
function axis(a, b, cell, varr) {
  const out = [a - cell]; let x = a - cell;
  while (x < b + cell) { x += cell * (1 - varr / 2 + rnd() * varr); out.push(x); }
  return out;
}
function blocksFor(d) {
  reseed(9000 + d.id.charCodeAt(0) * 137 + d.name.length);
  const [x0, y0, x1, y1] = d.rect;
  const cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;
  const varr = d.kind === 'organic' ? 0.62 : d.kind === 'ruin' ? 0.7 : d.kind === 'super' ? 0.3 : 0.24;
  const U = axis(x0, x1, d.cell[0], varr), V = axis(y0, y1, d.cell[1], varr);
  const co = Math.cos(d.rot), si = Math.sin(d.rot);
  const P = [];
  for (let i = 0; i < U.length; i++) {
    P[i] = [];
    for (let j = 0; j < V.length; j++) {
      const jx = (rnd() - 0.5) * d.cell[0] * d.jit, jy = (rnd() - 0.5) * d.cell[1] * d.jit;
      const lx = U[i] + jx - cx, ly = V[j] + jy - cy;
      P[i][j] = [cx + lx * co - ly * si, cy + lx * si + ly * co];
    }
  }
  const lanes = d.kind === 'organic' ? LANES : [];
  const out = [];
  for (let i = 0; i < U.length - 1; i++) for (let j = 0; j < V.length - 1; j++) {
    const q = [P[i][j], P[i + 1][j], P[i + 1][j + 1], P[i][j + 1]];
    const c = centroid(q);
    if (c[0] < x0 || c[0] > x1 || c[1] < y0 || c[1] > y1) continue;
    if (WATERS.some(w => inPoly(c[0], c[1], w.poly))) continue;
    if (inPoly(c[0], c[1], SEA)) continue;
    if (inPoly(c[0], c[1], ribbon(DRY, 240, 190))) continue;
    let onRoad = false;
    for (const s of STREETS) if (distToPolyline(c[0], c[1], s.pts) < s.w * 0.9 + 14) { onRoad = true; break; }
    if (!onRoad) for (const l of lanes) if (distToPolyline(c[0], c[1], l.pts) < l.w + 12) { onRoad = true; break; }
    if (onRoad) continue;
    if (VOIDS.some(V2 => inPoly(c[0], c[1], V2.poly))) continue;
    // 시가지 가장자리는 자로 자른 듯 끊기지 않는다 — 바깥으로 갈수록 성글어지다 삼각주로 흩어진다
    const fade = d.kind === 'organic' ? 220 : d.kind === 'ruin' ? 150 : 300;
    const edge = Math.min(c[0] - x0, x1 - c[0], c[1] - y0, y1 - c[1]);
    if (edge < fade && rnd() > 0.25 + 0.75 * (edge / fade)) continue;
    const gap = d.kind === 'organic' ? 0.78 : d.kind === 'super' ? 0.90 : d.kind === 'ruin' ? 0.6 : 0.86;
    out.push({ poly: inset(q, gap), kind: d.kind, c });
  }
  return out;
}
const BLOCKS = [];
DISTRICTS.forEach(d => blocksFor(d).forEach(b => BLOCKS.push(b)));

/* ============================================================
   4-2. 아래층 — 지표 아래도 같은 공간이다 (z는 m, 음수)
   ------------------------------------------------------------
   ★삼각주에 얕고 어지럽게 묻힌 옛 켜. 내려가는 입구는 둘(L03 환기구·L10 갱)이고,
     마른 골 밑이 옛 도읍으로 가는 천연 통로다.
   ============================================================ */
const UNDER = [
  { name: '끊긴 지하철 지선', z: -8, w: 16, pts: catmull([[-1100, 340], [-400, 300], [150, 268], [900, 210], [1600, 120]], 8) },
  { name: '옛 지하수로', z: -12, w: 11, pts: catmull([[-1500, -240], [-700, 60], [150, 268], [900, 420], [1700, 540]], 8) },
  { name: '승강장 → 아래층', z: -16, w: 13, pts: catmull([[150, 268], [300, -220], [480, -700], [620, -1080]], 8) },
  { name: '갱 → 아래층', z: -14, w: 11, pts: catmull([[-320, -1880], [-40, -1520], [360, -1250], [620, -1080]], 8) },
  { name: '마른 골 밑 통로', z: -18, w: 17, pts: catmull([[620, -1080], [1700, -520], [2900, 320], [4200, 1250], [5200, 2150], [6080, 3020]], 10) },
];
// 묻힌 도시 — 회랑이 격자로 도는 도시 규모의 방
const HALL = { name: '묻힌 도시 회랑', x: 620, y: -1080, z: -22, w: 720, h: 440, cell: [92, 74] };
// 폐선 승강장 — 물이 차 있는 방
const PLATFORM = { name: '폐선 승강장', x: 150, y: 268, z: -8, w: 130, h: 26 };

/* ============================================================
   4-3. 랜드마크 — 옛 사르간 도읍의 뼈
   ============================================================ */
const LANDMARKS = {
  wall: catmull([[5700, 2740], [6100, 2680], [6560, 2820], [6820, 3160], [6700, 3560],
  [6280, 3700], [5860, 3560], [5680, 3180], [5700, 2740]], 10),
  gate: { x: 6080, y: 3020, w: 120, name: '무너진 성문' },
  keep: { x: 6560, y: 3220, w: 190, h: 150, name: '빈 성' },
};

/* ============================================================
   5. 지하철 — 열두 갈래 중 지도에 그리는 여섯
   ============================================================ */
const METRO = [
  { c: '#c9704a', pts: catmull([[-4200, 100], [-2600, -60], [-1200, -120], [0, -90], [1400, -20], [2900, 120]], 10) },
  { c: '#4a8ac9', pts: catmull([[-1150, 1740], [-700, 1180], [-300, 400], [-80, -420], [200, -1500], [700, -2200]], 10) },
  { c: '#9a7ac0', pts: catmull([[-3600, 700], [-2200, 500], [-900, 260], [500, 320], [1700, 700], [2500, 1300], [2600, 2400]], 10) },
  { c: '#6aa36a', pts: catmull([[900, -2300], [700, -1300], [820, -300], [980, 700], [1000, 1700], [900, 2560]], 10) },
  { c: '#c0a04a', pts: catmull([[-3000, -800], [-1700, -640], [-400, -520], [900, -420], [2300, -280], [3400, -100]], 10) },
  { c: '#b05a7a', pts: catmull([[-400, 2540], [700, 2500], [1800, 2560], [2900, 2620]], 10) },
];
const STATIONS = [
  { x: -110, y: -100, n: '아랫장터역' }, { x: 1380, y: 60, n: '학당가역' },
  { x: -3480, y: 60, n: '부두역' }, { x: 640, y: -1980, n: '신항역' },
  { x: 1150, y: 2540, n: '유리탑역' }, { x: -1080, y: 1160, n: '북다리역' },
  { x: -2380, y: -30, n: '서편역' }, { x: 2450, y: 1180, n: '동강역' },
];

/* ============================================================
   6. 레벨 — 끊기지 않은 이 공간 안의 *자리* (세계 좌표 m)
   ============================================================ */
const LEVELS = [
  { id: 'L01', n: '장터 큰길', x: 0, y: 0, z: 0, g: 'A', note: '동서 280 m 굽은 대로 · 허브' },
  { id: 'L02', n: '네사의 골동상', x: 118, y: 84, z: 0, g: 'A', note: '앞문 큰길 / 뒷문 뒷골목 관통' },
  { id: 'L03', n: '장터 뒷골목', x: 86, y: 196, z: 0, g: 'A', note: '북측 블록 뒤 · 환기구 ↓L16' },
  { id: 'L04', n: '돌간의 흥신소', x: -146, y: 92, z: 6, g: 'A', note: '이층 · 창 남향' },
  { id: 'L05', n: '조용한 밥집', x: -98, y: -78, z: 0, g: 'A', note: '문 북향' },
  { id: 'L22', n: '요아의 셋방·옥상', x: 252, y: 96, z: 12, g: 'A', note: '창 서향 → 부두·바다' },
  { id: 'L06', n: '학당가 거리', x: 1360, y: 190, z: 0, g: 'B', note: '격자 + 대각 가로수길' },
  { id: 'L07', n: '학당 뒤 강둑', x: 1560, y: 880, z: 0, g: 'B', note: '첫째 물길 남안' },
  { id: 'L08', n: '의원 병동', x: 1430, y: -300, z: 0, g: 'B', note: '학당가 남측' },
  { id: 'L11', n: '부두 하역장', x: -3620, y: 190, z: 0, g: 'Q', note: '손가락 잔교 · 현역 항' },
  { id: 'L12', n: '부두 창고', x: -3320, y: -280, z: 0, g: 'Q', note: '요아의 낮 일터 · 지붕 감시' },
  { id: 'L20', n: '화물선', x: -5750, y: 340, z: 0, g: 'S', note: '사일란 바다 — 능력 무효' },
  { id: 'L09', n: '신항 공사판', x: -320, y: -1880, z: 0, g: 'C', note: '옛 부두를 갈아엎는 자리' },
  { id: 'L10', n: '굴착 갱', x: -320, y: -1880, z: -5.5, g: 'C', note: '지하 열여덟 자 · 돌리는 벽 →L17' },
  { id: 'L13', n: '세이르 재단', x: 260, y: 2620, z: 0, g: 'D', note: '요양원 · 지하 처치실' },
  { id: 'L14', n: '셀란 수련원', x: 1300, y: 2700, z: 0, g: 'D', note: '안뜰 + 안채' },
  { id: 'L15', n: '네르한 저택', x: 2320, y: 2740, z: 0, g: 'D', note: '다락 궤 → 부두로' },
  { id: 'L16', n: '폐선 승강장', x: 150, y: 268, z: -8, g: 'E', note: '환기구 ↑L03 · 터널 셋' },
  { id: 'L17', n: '아래층 묻힌 도시', x: 620, y: -1080, z: -22, g: 'E', note: '도시 규모 회랑 · 마른 골 밑으로' },
  { id: 'L18', n: '무너진 성문 앞뜰', x: 6080, y: 3020, z: 0, g: 'F', note: '마른 골 끝 · 아래층이 여기로 오름' },
  { id: 'L19', n: '빈 성 안', x: 6560, y: 3220, z: 0, g: 'F', note: '주인 없이 도는 고대 물건' },
  { id: 'L21', n: '딘의 은신처', x: -2280, y: 640, z: 0, g: 'X', note: '◇ 사건 따라 옮김 — 자리 미확정(임시 표시)' },
];
const GC = { A: '#f0d69a', B: '#a8c46a', Q: '#7ab0c8', D: '#8fb4c2', C: '#d08a6a', E: '#b48ad8', F: '#c0a870', S: '#6aa0b0', X: '#8a8a8a' };

/* ============================================================
   7. 렌더러 — 같은 공간을 다른 창(window)으로 본다. 공간은 안 끊는다.
   ============================================================ */
function plan(opt) {
  const { cx, cy, spanX, W, H, showBlocks = true, showMetro = true, minLevelSpan = 0, title } = opt;
  const S = W / spanX, spanY = H / S;
  const px = x => (x - cx) * S + W / 2, py = y => (cy - y) * S + H / 2;
  const pp = pts => pts.map(p => `${px(p[0]).toFixed(1)},${py(p[1]).toFixed(1)}`).join(' ');
  const vis = (x, y, m = 400) => x > cx - spanX / 2 - m && x < cx + spanX / 2 + m && y > cy - spanY / 2 - m && y < cy + spanY / 2 + m;
  let s = '';
  // ★뭍이 바탕이다 — 물이 그 위를 판다. 그래야 섬이 섬으로 읽힌다.
  s += `<rect x="0" y="0" width="${W}" height="${H}" fill="#241f16"/>`;
  // ※구역 경계는 색칠하지 않는다 — 현실 도시엔 구역을 두르는 네모가 없다.
  //   구역의 성격은 블록 조직(크기·각도·필지 색)으로만 드러난다.
  // 광장·공원 (제각각인 다각형)
  VOIDS.forEach(v => {
    const col = v.kind === 'park' ? '#2c3a22' : v.kind === 'yard' ? '#241f18' : '#3a2f1e';
    s += `<polygon points="${pp(v.poly)}" fill="${col}" stroke="#4a3f2c" stroke-width=".8"/>`;
  });
  // 블록
  if (showBlocks) {
    const col = { organic: '#4a3f2c', grid: '#443f2b', super: '#31414a', quay: '#33403f', industrial: '#4a3428', ruin: '#3a3226' };
    BLOCKS.forEach(b => {
      if (!vis(b.c[0], b.c[1])) return;
      s += `<polygon points="${pp(b.poly)}" fill="${col[b.kind] || '#443c2a'}" stroke="#1a160f" stroke-width=".6"/>`;
    });
  }
  // 골목 (구시가 잔가지) — 큰 가로보다 먼저 깔아 아래로 간다
  LANES.forEach(l => {
    s += `<polyline points="${pp(l.pts)}" fill="none" stroke="#4e452f" stroke-width="${Math.max(0.6, l.w * S).toFixed(1)}" stroke-linecap="round" stroke-linejoin="round"/>`;
  });
  // 길
  STREETS.forEach(st => {
    const w = Math.max(0.7, st.w * S);
    const c = st.cls === 'arterial' ? '#6b6047' : st.cls === 'main' ? '#7d6f4f' : '#57503a';
    s += `<polyline points="${pp(st.pts)}" fill="none" stroke="${c}" stroke-width="${w.toFixed(1)}" stroke-linecap="round" stroke-linejoin="round"/>`;
  });
  // 지하철
  if (showMetro) METRO.forEach(m => {
    s += `<polyline points="${pp(m.pts)}" fill="none" stroke="${m.c}" stroke-width="${Math.max(1, 2.2)}" stroke-dasharray="7 5" opacity=".55"/>`;
  });
  // 물
  WATERS.forEach(w => { s += `<polygon points="${pp(w.poly)}" fill="#1b3a44" stroke="#2d5a68" stroke-width="1"/>`; });
  s += `<polygon points="${pp(SEA)}" fill="#16303a" stroke="#2d5a68" stroke-width="1.2"/>`;
  // 마른 골 (물 없음)
  s += `<polygon points="${pp(ribbon(DRY, 240, 190))}" fill="#2e2717" stroke="#5a4a32" stroke-width="1" stroke-dasharray="9 6"/>`;
  // 다리
  BRIDGES.forEach(b => {
    s += `<line x1="${px(b.pts[0][0]).toFixed(1)}" y1="${py(b.pts[0][1]).toFixed(1)}" x2="${px(b.pts[1][0]).toFixed(1)}" y2="${py(b.pts[1][1]).toFixed(1)}" stroke="#c9a06a" stroke-width="${Math.max(2, 30 * S).toFixed(1)}" stroke-linecap="round"/>`;
  });
  // 잔교 (부두)
  if (cx < -1500 || spanX > 8000) {
    for (let i = 0; i < 5; i++) {
      const y = 560 - i * 260;
      s += `<line x1="${px(-4200).toFixed(1)}" y1="${py(y).toFixed(1)}" x2="${px(-4750).toFixed(1)}" y2="${py(y - 40).toFixed(1)}" stroke="#4a5a5f" stroke-width="${Math.max(1.5, 40 * S).toFixed(1)}" stroke-linecap="round"/>`;
    }
  }
  // 역
  if (showMetro) STATIONS.forEach(t => {
    if (!vis(t.x, t.y, 0)) return;
    s += `<circle cx="${px(t.x).toFixed(1)}" cy="${py(t.y).toFixed(1)}" r="${Math.max(2.5, 26 * S).toFixed(1)}" fill="#12100b" stroke="#c9a06a" stroke-width="1.6"/>`;
    if (spanX < 9000) s += `<text x="${px(t.x).toFixed(1)}" y="${(py(t.y) - 10).toFixed(1)}" fill="#a3946e" font-size="9.5" text-anchor="middle">${t.n}</text>`;
  });
  // 레벨 핀
  LEVELS.forEach(L => {
    if (!vis(L.x, L.y, 0)) return;
    if (spanX > minLevelSpan && false) return;
    const X = px(L.x), Y = py(L.y), c = GC[L.g] || '#e0b060';
    s += `<circle cx="${X.toFixed(1)}" cy="${Y.toFixed(1)}" r="${L.z < 0 ? 4.5 : 5.5}" fill="${L.z < 0 ? '#12100b' : c}" stroke="${c}" stroke-width="2"${L.z < 0 ? ' stroke-dasharray="2.5 2"' : ''}/>`;
  });
  return { svg: s, px, py, S, spanY };
}
const esc = t => String(t).replace(/&/g, '&amp;').replace(/</g, '&lt;');
function labels(o, arr) {                        // 겹침 줄인 라벨
  let s = '';
  arr.forEach(a => {
    const X = o.px(a.x), Y = o.py(a.y);
    s += `<text x="${(X + (a.dx || 0)).toFixed(1)}" y="${(Y + (a.dy || -10)).toFixed(1)}" fill="${a.c || '#e8dcc4'}" font-size="${a.s || 11}" text-anchor="${a.a || 'middle'}"${a.b ? ' font-weight="700"' : ''}>${esc(a.t)}</text>`;
    if (a.t2) s += `<text x="${(X + (a.dx || 0)).toFixed(1)}" y="${(Y + (a.dy || -10) + (a.s || 11) + 2).toFixed(1)}" fill="${a.c2 || '#9f8f68'}" font-size="${(a.s || 11) - 2}" text-anchor="${a.a || 'middle'}">${esc(a.t2)}</text>`;
  });
  return s;
}
function scalebar(o, W, H, meters, label) {
  const w = meters * o.S, x = W - w - 40, y = H - 38;
  return `<line x1="${x}" y1="${y}" x2="${x + w}" y2="${y}" stroke="#c9a06a" stroke-width="2"/>
<line x1="${x}" y1="${y - 5}" x2="${x}" y2="${y + 5}" stroke="#c9a06a" stroke-width="2"/>
<line x1="${x + w}" y1="${y - 5}" x2="${x + w}" y2="${y + 5}" stroke="#c9a06a" stroke-width="2"/>
<text x="${x + w / 2}" y="${y - 9}" fill="#c9a06a" font-size="10.5" text-anchor="middle">${label}</text>`;
}
const compass = (x, y) => `<g transform="translate(${x},${y})"><circle r="26" fill="#161009" stroke="#5a4a30"/>
<path d="M0,-20 L5,0 L0,-5 L-5,0 Z" fill="#c9a06a"/><path d="M0,20 L5,0 L0,5 L-5,0 Z" fill="#4a3f2a"/>
<text x="0" y="-10" fill="#f0d69a" font-size="10" text-anchor="middle">N</text>
<text x="-17" y="4" fill="#a08a5a" font-size="9" text-anchor="middle">W</text>
<text x="17" y="4" fill="#a08a5a" font-size="9" text-anchor="middle">E</text></g>`;

/* ---------- 컷 A : 라셀 전도 ---------- */
const A = plan({ cx: 400, cy: 300, spanX: 14400, W: 1400, H: 940 });
let svgA = A.svg;
svgA += labels(A, [
  { x: -5900, y: 1200, t: '사 일 란   바 다', s: 17, b: 1, c: '#5f8a96' },
  { x: -5900, y: 900, t: '◀ 서 · 화물선이 드는 쪽', s: 11, c: '#4a7a86' },
  { x: 6400, y: 500, t: '강 본류 ▶ 동 · 상류(루담)', s: 12, c: '#5f8a96' },
  { x: -2400, y: 1700, t: '첫째 물길', s: 12.5, c: '#7fb0c0' },
  { x: -2700, y: -1180, t: '둘째 물길', s: 12.5, c: '#7fb0c0' },
  { x: -3300, y: -2700, t: '셋째 물길 — 옛 부두 자리', s: 12.5, c: '#7fb0c0' },
  { x: 5400, y: 2150, t: '마른 골 (죽은 물길)', s: 11.5, c: '#a08a5a' },
  { x: -280, y: 480, t: '[A] 아랫장터 구시가', s: 13, b: 1, c: '#f0d69a', dy: -4 },
  { x: 1650, y: 620, t: '[B] 학당가·의원권', s: 12.5, b: 1, c: '#a8c46a' },
  { x: -2300, y: 1000, t: '서편 셋집 구역', s: 11.5, c: '#c9b98a' },
  { x: -3750, y: 900, t: '부두 — 바다 목', s: 12.5, b: 1, c: '#7ab0c8' },
  { x: 1300, y: 3200, t: '[D] 강 건너 유리탑 — 위층 섬', s: 13, b: 1, c: '#8fb4c2' },
  { x: -600, y: -2250, t: '[C] 신항 재개발지', s: 12.5, b: 1, c: '#d08a6a' },
  { x: 3000, y: 500, t: '동편 새 동네', s: 11.5, c: '#c9b98a' },
  { x: 6250, y: 3560, t: '[F] 옛 사르간 도읍 폐허', s: 12, b: 1, c: '#c0a870' },
  { x: 900, y: -560, t: '남대로', s: 10, c: '#8a7f66' },
  { x: -2000, y: 1250, t: '강변대로', s: 10, c: '#8a7f66' },
  { x: -2500, y: -180, t: '부두길', s: 10, c: '#8a7f66' },
]);
LEVELS.forEach(L => {
  if (['L01', 'L09', 'L11', 'L13', 'L15', 'L18', 'L20', 'L07'].includes(L.id))
    svgA += labels(A, [{ x: L.x, y: L.y, t: L.id, s: 9.5, c: GC[L.g], dy: 14 }]);
});
svgA += compass(1330, 80) + scalebar(A, 1400, 940, 2000, '2 km');

/* ---------- 컷 B : 도심 섬 ---------- */
const B = plan({ cx: 300, cy: 400, spanX: 5000, W: 1400, H: 900 });
let svgB = B.svg;
svgB += labels(B, [
  { x: -1500, y: 1900, t: '첫째 물길 — 건너면 위층 섬', s: 13, c: '#7fb0c0' },
  { x: -1500, y: -1150, t: '둘째 물길', s: 12, c: '#7fb0c0' },
  { x: -430, y: 60, t: '장터 마당', s: 11.5, b: 1, c: '#e0b060', dy: 4 },
  { x: 1520, y: 210, t: '학당 앞뜰', s: 11, c: '#a8c46a', dy: 4 },
  { x: -1050, y: 1500, t: '북다리', s: 10.5, c: '#c9a06a' },
  { x: -30, y: 1420, t: '가운데다리', s: 10.5, c: '#c9a06a' },
  { x: 890, y: 1350, t: '동다리', s: 10.5, c: '#c9a06a' },
  { x: -140, y: -1000, t: '남다리', s: 10.5, c: '#c9a06a' },
  { x: -2100, y: 1250, t: '강변대로', s: 11, c: '#8a7f66' },
  { x: -2050, y: -790, t: '남대로', s: 11, c: '#8a7f66' },
  { x: -1750, y: -230, t: '부두길 ◀ 부두로', s: 11, c: '#8a7f66' },
  { x: 1800, y: 480, t: '학당 가로수길', s: 10.5, c: '#8a9a68' },
  { x: -2350, y: 700, t: '서편 셋집 구역', s: 12, b: 1, c: '#c9b98a' },
  { x: -1000, y: -1900, t: '[C] 신항 →', s: 11.5, c: '#d08a6a' },
]);
LEVELS.filter(L => Math.abs(L.x - 300) < 2600 && Math.abs(L.y - 400) < 1700).forEach(L => {
  svgB += labels(B, [{ x: L.x, y: L.y, t: `${L.id} ${L.n}`, s: 10.5, c: GC[L.g], dy: -11, b: 1 }]);
});
svgB += compass(1330, 80) + scalebar(B, 1400, 900, 500, '500 m');

/* ---------- 컷 C : 아랫장터 구시가 ---------- */
const C = plan({ cx: -60, cy: 60, spanX: 1500, W: 1400, H: 900 });
let svgC = C.svg;
svgC += labels(C, [
  { x: -430, y: 60, t: '장터 마당', s: 13, b: 1, c: '#e0b060', dy: 5, t2: '좌판이 서고 소문이 갈라지는 결절점' },
  { x: 0, y: -60, t: '장터 큰길', s: 12, c: '#c9b98a', dy: 26 },
  { x: 86, y: 196, t: '', s: 1 },
]);
LEVELS.filter(L => Math.abs(L.x + 60) < 780 && Math.abs(L.y - 60) < 480).forEach(L => {
  svgC += labels(C, [{ x: L.x, y: L.y, t: `${L.id} ${L.n}`, s: 11.5, b: 1, c: GC[L.g], dy: -13, t2: L.note, c2: '#9f8f68' }]);
});
svgC += compass(1330, 80) + scalebar(C, 1400, 900, 100, '100 m');

/* ---------- HTML ---------- */
const rows = LEVELS.map(L => `<tr><td><code>${L.id}</code></td><td>${L.n}</td><td class="num">${L.x.toLocaleString()}</td><td class="num">${L.y.toLocaleString()}</td><td class="num">${L.z}</td><td class="num">${(L.x * 100).toLocaleString()}, ${(L.y * 100).toLocaleString()}</td><td>${L.note}</td></tr>`).join('\n');

const html = `<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>라셀 도시 뼈대 — 끊기지 않은 하나의 공간</title>
<style>
:root{color-scheme:dark}*{box-sizing:border-box}
body{margin:0;background:#0e0c08;color:#e8dcc4;font-family:'Malgun Gothic',system-ui,sans-serif;line-height:1.65}
.wrap{max-width:1460px;margin:0 auto;padding:30px 18px 90px}
h1{font-size:27px;margin:0 0 6px;color:#f3e6c8}
.sub{color:#9c8c68;font-size:13.5px;margin:0 0 22px}
h2{font-size:19px;margin:44px 0 4px;color:#e0b060;border-bottom:1px solid #3a3020;padding-bottom:7px}
h2 .z{font-size:12px;color:#7d7050;font-weight:400;margin-left:8px}
.cut{background:#15110b;border:1px solid #2f2718;border-radius:14px;padding:12px 12px 6px;margin-top:12px;overflow-x:auto}
svg{display:block;width:100%;height:auto;border-radius:8px}
.cap{font-size:12.5px;color:#a3946e;padding:10px 6px 8px;border-top:1px solid #241d13;margin-top:8px}
.cap b{color:#e0b060}
table{width:100%;border-collapse:collapse;font-size:12.5px;margin:10px 0}
th,td{text-align:left;padding:6px 9px;border-bottom:1px solid #2a2216}
th{color:#c9a06a;background:#181309}
td.num{text-align:right;font-variant-numeric:tabular-nums;color:#e8c98a}
code{color:#e8c98a}
.note{background:#241a10;border:1px solid #6b4f22;border-radius:12px;padding:14px 16px;margin:16px 0;font-size:13px}
.note b{color:#f0c878}
ul{margin:8px 0;padding-left:20px}li{margin:4px 0;font-size:13.5px}
.legend{display:flex;flex-wrap:wrap;gap:10px 16px;margin:10px 4px;font-size:11.5px;color:#a3946e}
.lg{display:inline-flex;align-items:center;gap:6px}
.sw{width:12px;height:12px;border-radius:3px;display:inline-block}
.foot{color:#6f6248;font-size:11.5px;margin-top:46px;border-top:1px solid #2a2216;padding-top:12px}
</style>
<div class="wrap">
<h1>🏙 라셀 도시 뼈대 — 끊기지 않은 하나의 공간</h1>
<p class="sub">라셀은 레벨 여러 개가 아니라 <b>연속된 한 공간</b>이다. 아래 세 컷은 그 공간을 자른 것이 아니라 <b>같은 좌표계를 다른 거리에서 본 것</b>이다 — 원점(0,0)은 장터 큰길 한복판, +X 동(내륙·상류), +Y 북, 단위 m. 레벨은 이 공간을 나누는 단위가 아니라 <b>공간 안의 자리</b>다.</p>

<h2>컷 A — 라셀 전도 <span class="z">/ 14.4 km × 9.7 km · 삼각주 전체</span></h2>
<div class="cut"><svg viewBox="0 0 1400 940" font-family="Malgun Gothic,sans-serif">${svgA}</svg>
<div class="cap"><b>도시 조직이 구역마다 다르다.</b> 구시가는 작고 제각각인 필지에 굽은 길이 얽히고, 학당가는 격자에 대각 가로수길이 지나며, 위층 섬은 초대형 블록에 광폭 대로, 신항은 성긴 대형 필지, 부두는 손가락 잔교와 창고 열. <b>직선 한 줄이 아니라 가로망이다</b> — 큰길·부두길·강변대로·남대로가 서로 다른 각도로 만나고, 다리 다섯이 물길을 건넌다.</div>
<div class="legend">
<span class="lg"><span class="sw" style="background:#4a3f2c"></span> 구시가 필지</span>
<span class="lg"><span class="sw" style="background:#443f2b"></span> 격자 시가</span>
<span class="lg"><span class="sw" style="background:#31414a"></span> 초대형 블록(위층)</span>
<span class="lg"><span class="sw" style="background:#4a3428"></span> 공업·재개발</span>
<span class="lg"><span class="sw" style="background:#33403f"></span> 항만</span>
<span class="lg"><span class="sw" style="background:#1b3a44"></span> 물길</span>
<span class="lg"><span class="sw" style="background:#2e2717"></span> 마른 골</span>
<span class="lg"><span class="sw" style="background:#c9a06a"></span> 다리</span>
<span class="lg">┄ 지하철 (열두 갈래 중 여섯)</span>
</div>
</div>

<h2>컷 B — 도심 섬 <span class="z">/ 5 km × 3.2 km · 같은 공간, 더 가까이</span></h2>
<div class="cut"><svg viewBox="0 0 1400 900" font-family="Malgun Gothic,sans-serif">${svgB}</svg>
<div class="cap"><b>구시가와 신시가가 붙어 있는 결이 보인다.</b> 왼쪽(서)은 물가에 먼저 선 아랫장터 — 길이 굽고 블록이 잘다. 오른쪽(동)은 나중에 뻗은 학당가 — 격자에 대각선이 하나 지난다. 둘을 <b>남대로</b>와 <b>강변대로</b>가 위아래로 묶고, 서쪽으로 <b>부두길</b>이 항까지 뻗는다.</div></div>

<h2>컷 C — 아랫장터 구시가 <span class="z">/ 1.5 km × 0.96 km · 레벨이 실제로 앉은 자리</span></h2>
<div class="cut"><svg viewBox="0 0 1400 900" font-family="Malgun Gothic,sans-serif">${svgC}</svg>
<div class="cap"><b>큰길은 자로 그은 직선이 아니다.</b> 옛 물가를 따라 생긴 길이라 완만하게 굽고, 서쪽에서 <b>장터 마당</b>(광장)에 걸렸다가 다시 동으로 빠진다. 골목은 그 큰길에서 갈라져 블록 뒤로 돌아 들어가고, 골동상은 큰길과 뒷골목을 앞뒤로 관통한다.</div></div>

<h2>레벨 자리표 — 하나의 좌표계 위</h2>
<table>
<tr><th>Id</th><th>이름</th><th class="num">X (m)</th><th class="num">Y (m)</th><th class="num">Z (m)</th><th class="num">UE 좌표 (cm)</th><th>메모</th></tr>
${rows}
</table>

<h2>언리얼에 선 라셀 <span class="z">/ 같은 좌표·같은 데이터로 지은 실제 레벨</span></h2>
<p class="sub" style="margin:6px 0 12px">맵 <code>/Game/Maps/Rasel/Rasel_City</code> — <b>도시 전체가 맵 하나</b>다(구역별로 쪼개지 않았다). 액터 3,447개(도시 13.8 × 9.1 km).
빌더 <code>_build_rasel_city_ue.py</code>가 위 도면과 <b>같은 <code>_rasel_plan.json</code></b>을 읽어 짓는다. 에셋은 <code>/Engine/BasicShapes</code> + 우리가 저작한 단색 머티리얼 18종뿐.</p>
<div class="cut"><img src="_shots/ue_top.png" alt="라셀 부감" style="width:100%;display:block;border-radius:8px">
<div class="cap"><b>부감 (고도 4.3 km).</b> 구역이 조직과 색으로 갈린다 — 가운데 따뜻한 흙빛이 구시가(잔 블록·굽은 길), 그 양옆 밝은 회백이 격자 시가, 위쪽 파랑이 강 건너 유리탑, 아래 붉은빛이 신항, 오른쪽(서) 청록이 부두. 노란 막대가 다리 다섯, 원형 빈터가 장터 마당.</div></div>
<div class="cut"><img src="_shots/ue_skyline.png" alt="구시가 위에서 본 스카이라인" style="width:100%;display:block;border-radius:8px">
<div class="cap"><b>★세상 뒤가 있다.</b> 구시가의 들쭉날쭉한 처마선 너머로 물길이 지나고, 그 건너에 유리탑 스카이라인이 지평선을 채운다. "칸에서 잘린 세상"이 아니라 도시 <b>안</b>이다. 블록을 통짜로 세우지 않고 긴 쪽을 필지로 갈라 높이를 흩었다.</div></div>
<div class="cut"><img src="_shots/ue_oblique.png" alt="라셀 사선 부감" style="width:100%;display:block;border-radius:8px">
<div class="cap"><b>사선 부감.</b> 삼각주가 실제로 도시를 가르고, 물길 건너 유리탑 지구가 따로 선다.</div></div>
<div class="cut"><img src="_shots/ue_pano.png" alt="물길 건너에서 본 라셀" style="width:100%;display:block;border-radius:8px">
<div class="cap"><b>물길 건너에서.</b> 둘째 물길 남쪽 들판에서 도심 섬을 본 것 — 낮은 시가지 위로 유리탑이 솟고, 다리가 물을 건넌다. 왼쪽 아래 보랏빛 기둥이 아래층 자리표(L17).</div></div>
<div class="cut"><img src="_shots/ue_ruin.png" alt="옛 사르간 도읍 폐허" style="width:100%;display:block;border-radius:8px">
<div class="cap"><b>마른 골 끝 — 옛 사르간 도읍.</b> 말라붙은 옛 물길(누런 띠)이 성벽 안으로 곧장 들어간다. 강이 도읍을 버리고 옮겨 앉았다는 것이 지형으로 보인다. 노란 대지가 L18 무너진 성문 앞뜰.</div></div>
<div class="cut"><img src="_shots/ue_roof.png" alt="구시가 지붕 높이" style="width:100%;display:block;border-radius:8px">
<div class="cap"><b>지붕 높이에서.</b> 노란 기둥이 레벨 자리표 — 어디에 무엇이 앉는지 공간 안에서 바로 보인다.</div></div>

<h3>아래층 — 지표 아래도 같은 공간이다</h3>
<p style="font-size:13.5px;color:#a3946e;margin:6px 0 4px">화면으로는 못 찍히므로(땅에 덮여 있다) 맵을 직접 세어 확인했다 — <code>_audit_rasel_city.py</code>.</p>
<table>
<tr><th style="width:200px">아래층 갈래</th><th style="width:90px">깊이</th><th>무엇</th></tr>
<tr><td>끊긴 지하철 지선</td><td class="num">−8 m</td><td>삼십 년 전 끊긴 지선. L03 뒷골목 환기구가 여기로 내려온다</td></tr>
<tr><td>폐선 승강장 (L16)</td><td class="num">−8 m</td><td>물이 찬 방 130 × 26 m · 기둥 여섯 · 터널 셋이 각각 다른 데로</td></tr>
<tr><td>옛 지하수로</td><td class="num">−12 m</td><td>라셀이 세워지기 전부터 있던 물길. 누가 언제 팠는지 아무도 모른다</td></tr>
<tr><td>갱 → 아래층</td><td class="num">−14 m</td><td>L10 굴착 갱의 돌리는 벽이 뚫은 길</td></tr>
<tr><td>승강장 → 아래층</td><td class="num">−16 m</td><td>아이린이 세 시간 걸어 오른 그 길</td></tr>
<tr><td><b>마른 골 밑 통로</b></td><td class="num">−18 m</td><td><b>죽은 물길 밑을 따라 옛 도읍까지 5.5 km.</b> 물이 다니던 자리라 뚫려 있고, 물이 빠졌으니 걸어간다</td></tr>
<tr><td><b>묻힌 도시 (L17)</b></td><td class="num">−22 m</td><td>회랑이 격자로 도는 720 × 440 m의 방. 무너진 기둥 서른다섯</td></tr>
<tr><td>지하철 터널 여섯</td><td class="num">−26 m</td><td>열두 갈래 중 여섯. 아래층 위를 가로지른다 — 새로 뚫을 때마다 도시가 발밑을 건드리는 까닭</td></tr>
</table>

<h3>아웃라이너 구성 (골라 지우고 고칠 수 있게)</h3>
<table>
<tr><th style="width:210px">폴더</th><th>무엇</th></tr>
<tr><td><code>라셀/땅</code></td><td>삼각주 지반 타일 64</td></tr>
<tr><td><code>라셀/물길/…</code></td><td>바다 · 강 본류 · 첫째/둘째/셋째 물길 · 마른 골</td></tr>
<tr><td><code>라셀/가로/…</code></td><td>장터 큰길 · 부두길 · 강변대로 · 남대로 · 학당 가로수길 · 유리탑 대로 · 안벽로 · 골목</td></tr>
<tr><td><code>라셀/블록/…</code></td><td>구시가 537블록 · 격자 시가 305 · 위층 유리탑 32(탑+포디움) · 신항 21 · 부두 63+잔교 5 · 폐허 98 — 블록마다 필지로 갈라 세움</td></tr>
<tr><td><code>라셀/다리</code></td><td>북·가운데·동·남·신항 다리</td></tr>
<tr><td><code>라셀/광장</code></td><td>장터 마당 · 샛마당 · 학당 앞뜰 · 유리탑 광장 · 철도 야드</td></tr>
<tr><td><code>라셀/자리표</code></td><td>레벨 22곳 기둥(지상=금빛 / 아래층=보랏빛) + 이름 · 지하철역 8</td></tr>
<tr><td><code>라셀/하늘</code></td><td>해 · 하늘빛 · 대기 안개 · 노출 · 시작 지점</td></tr>
</table>

<div class="note">
<b>★이 뼈대의 쓰는 법</b>
<ul>
<li><b>공간을 자르지 않는다.</b> 라셀 전체가 한 좌표계 위에 있고, 레벨은 그 안의 자리다. UE에서도 <b>맵을 나눠 짓지 말고</b> 하나의 월드에 이 좌표로 얹은 뒤, 필요해지면 그때 스트리밍 단위를 나눈다(자리 좌표는 안 바뀐다).</li>
<li><b>1 m = 100 UE 유닛.</b> 표의 마지막 칸이 그대로 액터 좌표다.</li>
<li>지금 지어 둔 L01 대로(자체 좌표 X ±14000 유닛 = 280 m)는 이 뼈대의 (0,0) 자리에 그대로 얹힌다.</li>
<li>가로망·물길·다리·지하철 노선은 전부 <code>_build_rasel_plan.js</code>의 데이터다. 고치고 다시 돌리면 세 컷이 함께 갱신된다.</li>
</ul>
</div>

<p class="foot">생성 = <code>node _build_rasel_plan.js</code> · 지리 근거 = <code>기획/01_세계관/3_현대/세계관_현대_지리와_나라_정본.md</code> · 좌표 정본 = <code>기획/02_게임설계/2_레벨/게임_라셀_배치정본.md</code> · 레벨 로스터 = <code>기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md</code></p>
</div>
`;
fs.writeFileSync('C:/Secret_Project/세계관_라셀_도시뼈대.html', html, 'utf8');

/* ============================================================
   8. ★UE로 넘기는 데이터 — 웹 도면과 언리얼이 같은 원본에서 나온다
   ============================================================ */
const r1 = p => p.map(q => [+q[0].toFixed(1), +q[1].toFixed(1)]);
const data = {
  meta: { unit: 'm', ue_per_m: 100, origin: '장터 큰길 한복판', axis: '+X 동(내륙·상류) / +Y 북' },
  blocks: BLOCKS.map(b => ({ p: r1(b.poly), k: b.kind })),
  streets: STREETS.map(s => ({ pts: r1(s.pts), w: s.w, cls: s.cls, name: s.name })),
  lanes: LANES.map(l => ({ pts: r1(l.pts), w: l.w })),
  channels: [
    { name: '강 본류', pts: r1(RIVER_MAIN), w0: 560, w1: 520 },
    { name: '첫째 물길', pts: r1(CH1), w0: 420, w1: 780 },
    { name: '둘째 물길', pts: r1(CH2), w0: 300, w1: 460 },
    { name: '셋째 물길', pts: r1(CH3), w0: 260, w1: 420 },
  ],
  dry: { name: '마른 골', pts: r1(DRY), w0: 240, w1: 190 },
  coast: r1(COAST),
  bridges: BRIDGES.map(b => ({ name: b.name, pts: r1(b.pts) })),
  metro: METRO.map((m, i) => ({ i: i, pts: r1(m.pts) })),
  under: UNDER.map(u => ({ name: u.name, z: u.z, w: u.w, pts: r1(u.pts) })),
  hall: HALL,
  platform: PLATFORM,
  landmarks: { wall: r1(LANDMARKS.wall), gate: LANDMARKS.gate, keep: LANDMARKS.keep },
  stations: STATIONS,
  voids: VOIDS.map(v => ({ name: v.name, kind: v.kind, x: v.x, y: v.y, r: v.r, poly: r1(v.poly) })),
  districts: DISTRICTS.map(d => ({ id: d.id, name: d.name, kind: d.kind, rect: d.rect })),
  levels: LEVELS,
};
fs.writeFileSync('C:/Secret_Project/_rasel_plan.json', JSON.stringify(data), 'utf8');
console.log('blocks =', BLOCKS.length, '· levels =', LEVELS.length,
  '→ 세계관_라셀_도시뼈대.html + _rasel_plan.json');


