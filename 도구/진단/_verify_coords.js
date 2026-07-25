/**
 * 라셀 좌표 검사기 겸 자리표 생성기
 *
 *   node 도구/진단/_verify_coords.js            → 검사만 (모순 목록 출력)
 *   node 도구/진단/_verify_coords.js --write     → 검사 + 01_지형과_자리표.md 다시 씀
 *
 * 원본은 하나다 — `도구/_rasel_plan.json`. 문서는 그걸 비추는 거울이라 손으로 고치지 않는다.
 * 규약은 `기획/02_게임설계/2_레벨/좌표/00_좌표계_규약.md`.
 *
 * ★프로젝트 루트에서 실행할 것 (cwd = C:/Secret_Project).
 */
const fs = require('fs'), path = require('path');

const ROOT = process.cwd().replace(/\\/g, '/');
const PLAN = path.join(ROOT, '도구/_rasel_plan.json');
const DOCDIR = path.join(ROOT, '기획/02_게임설계/2_레벨/좌표');
const MAPDIR = path.join(ROOT, 'Content/Maps/Rasel');
const WRITE = process.argv.includes('--write');

const p = JSON.parse(fs.readFileSync(PLAN, 'utf8'));

// ── 00_좌표계_규약 §3 의 수직 켜. 여기 없는 z는 모순이다.
const Z_LAYERS = [12, 6, 0, -5.5, -8, -12, -14, -16, -18, -22, -26];
// ── §6 문 연결 한계 (m)
const LIMIT = { indoor: 80, alley: 250, sameDistrict: 600, vertical: 120 };

const issues = [];
const add = (sev, where, what) => issues.push({ sev, where, what });

// ══ 1. 구역 사각형 정합 ═════════════════════════════════════════════
// rect = [x1, y1, x2, y2]
const dist = {};
for (const d of p.districts) {
  const [x1, y1, x2, y2] = d.rect;
  if (x1 >= x2 || y1 >= y2) add('오류', `구역 ${d.id}`, `사각형이 뒤집혔다 [${d.rect}]`);
  dist[d.id] = { ...d, x1, y1, x2, y2 };
}
// 구역끼리 겹치나
const ids = Object.keys(dist);
for (let i = 0; i < ids.length; i++) for (let j = i + 1; j < ids.length; j++) {
  const a = dist[ids[i]], b = dist[ids[j]];
  const ox = Math.min(a.x2, b.x2) - Math.max(a.x1, b.x1);
  const oy = Math.min(a.y2, b.y2) - Math.max(a.y1, b.y1);
  // 경계가 맞닿아 수십 m 겹치는 건 정상(구역은 칼로 자른 듯 나뉘지 않는다). 실제로 파고든 것만 잡는다.
  if (ox > 0 && oy > 0) {
    const deep = Math.min(ox, oy);
    add(deep > 50 ? '경고' : '알림', `구역 ${a.id}×${b.id}`,
      `${Math.round(ox)}×${Math.round(oy)} m 겹친다${deep > 50 ? '' : ' (경계 접촉 — 정상 범위)'}`);
  }
}

// ══ 2. 레벨 자리 ════════════════════════════════════════════════════
const inRect = (l, d) => l.x >= d.x1 && l.x <= d.x2 && l.y >= d.y1 && l.y <= d.y2;
const seen = new Set();
for (const l of p.levels) {
  if (seen.has(l.id)) add('오류', l.id, '레벨 번호가 겹친다');
  seen.add(l.id);

  // z가 정해진 켜인가
  if (!Z_LAYERS.includes(l.z))
    add('오류', l.id, `z=${l.z} 는 규약 §3의 켜가 아니다 (허용: ${Z_LAYERS.join(', ')})`);

  // 구역 안에 있나
  const d = dist[l.g];
  if (!d) {
    if (!['S', 'E', 'X'].includes(l.g))
      add('오류', l.id, `구역 코드 '${l.g}' 가 districts 에 없다`);
    else
      add('알림', l.id, `구역 코드 '${l.g}' 는 구역이 아니다 (S=바다 · E=아래층 · X=미확정)`);
  } else if (!inRect(l, d)) {
    add('오류', l.id, `구역 ${l.g} 사각형 밖이다 — 자리(${l.x}, ${l.y}) vs [${d.x1}~${d.x2}, ${d.y1}~${d.y2}]`);
  }
}

// 구역에 레벨이 하나도 없나
for (const d of p.districts)
  if (!p.levels.some(l => l.g === d.id))
    add('알림', `구역 ${d.id}`, `${d.name} — 레벨 자리가 하나도 없다`);

// ══ 3. 수직 통로 (지상 ↔ 아래층) ════════════════════════════════════
const D2 = (a, b) => Math.round(Math.hypot(a.x - b.x, a.y - b.y));
for (const u of (p.under || [])) {
  if (!u.pts || u.pts.length < 2) continue;
  // 이름에 '→' 가 있는 것만 레벨을 잇는 통로다. 나머지(끊긴 지선·옛 지하수로)는 그냥 지형이라 검사 대상이 아니다.
  if (!u.name.includes('→')) continue;
  const [sx, sy] = u.pts[0], [ex, ey] = u.pts[u.pts.length - 1];
  const near = (x, y) => p.levels
    .map(l => ({ l, d: Math.round(Math.hypot(l.x - x, l.y - y)) }))
    .sort((a, b) => a.d - b.d)[0];
  const a = near(sx, sy), b = near(ex, ey);
  if (a.d > LIMIT.vertical)
    add('경고', `아래층 「${u.name}」`, `시작점(${sx}, ${sy})에서 가장 가까운 레벨이 ${a.l.id} — ${a.d} m (한계 ${LIMIT.vertical})`);
  if (b.d > LIMIT.vertical)
    add('경고', `아래층 「${u.name}」`, `끝점(${ex}, ${ey})에서 가장 가까운 레벨이 ${b.l.id} — ${b.d} m (한계 ${LIMIT.vertical})`);
  if (!Z_LAYERS.includes(u.z))
    add('오류', `아래층 「${u.name}」`, `z=${u.z} 는 규약 §3의 켜가 아니다`);
}

// ══ 4. note 에 적힌 연결이 좌표로 성립하나 ══════════════════════════
// note 안의 "↓L16" "↑L03" 같은 표시를 읽는다
for (const l of p.levels) {
  const m = [...String(l.note || '').matchAll(/[↑↓→]\s*(L\d{2})/g)].map(x => x[1]);
  for (const tid of m) {
    const t = p.levels.find(v => v.id === tid);
    if (!t) { add('오류', l.id, `note 가 없는 레벨 ${tid} 를 가리킨다`); continue; }
    const d = D2(l, t);
    // 두 레벨을 실제로 잇는 아래층 통로가 있으면 멀어도 정상이다 — 문이 아니라 굴이니까.
    const tunnel = (p.under || []).some(u => {
      if (!u.pts || u.pts.length < 2) return false;
      const a = u.pts[0], b = u.pts[u.pts.length - 1];
      const near = (pt, lv) => Math.hypot(pt[0] - lv.x, pt[1] - lv.y) <= LIMIT.vertical;
      return (near(a, l) && near(b, t)) || (near(a, t) && near(b, l));
    });
    if (tunnel) { add('알림', `${l.id}→${tid}`, `${d} m 떨어졌지만 아래층 통로로 이어진다 — 정상`); continue; }

    if (Math.abs(l.z - t.z) > 0.1) {           // 수직 연결
      if (d > LIMIT.vertical)
        add('경고', `${l.id}→${tid}`, `수직 통로인데 수평으로 ${d} m 떨어졌다 (한계 ${LIMIT.vertical})`);
    } else if (d > LIMIT.sameDistrict) {
      add('경고', `${l.id}→${tid}`, `문으로 잇기엔 ${d} m 로 멀다 (한계 ${LIMIT.sameDistrict})`);
    }
  }
}

// ══ 5. 도면 ↔ 실제 .umap ════════════════════════════════════════════
let maps = [];
try { maps = fs.readdirSync(MAPDIR).filter(f => f.endsWith('.umap')).map(f => f.replace('.umap', '')); }
catch { add('알림', 'Content/Maps/Rasel', '맵 폴더를 못 읽었다'); }

const byId = {};
for (const m of maps) { const id = m.slice(0, 3); if (/^L\d\d$/.test(id)) (byId[id] ||= []).push(m); }
for (const l of p.levels) {
  const u = byId[l.id] || [];
  if (u.length === 0) add('오류', l.id, `도면에 있는데 맵이 없다 — 「${l.n}」`);
  else if (u.length > 1) add('경고', l.id, `한 번호에 맵이 ${u.length}개 — ${u.join(' / ')}`);
}
const planIds = new Set(p.levels.map(l => l.id));
for (const id of Object.keys(byId))
  if (!planIds.has(id)) add('오류', id, `맵은 있는데 도면에 자리가 없다 — ${byId[id].join(' / ')}`);

// ══ 출력 ════════════════════════════════════════════════════════════
const order = { '오류': 0, '경고': 1, '알림': 2 };
issues.sort((a, b) => order[a.sev] - order[b.sev] || a.where.localeCompare(b.where));
const n = s => issues.filter(i => i.sev === s).length;

console.log('=== 라셀 좌표 검사 ===');
console.log(`구역 ${p.districts.length} · 레벨 ${p.levels.length} · 맵 ${maps.length}`);
console.log(`오류 ${n('오류')} · 경고 ${n('경고')} · 알림 ${n('알림')}\n`);
for (const i of issues) console.log(`[${i.sev}] ${i.where} — ${i.what}`);

// ══ 자리표 문서 생성 ════════════════════════════════════════════════
if (WRITE) {
  const L = [];
  const f = v => (v >= 0 ? ' ' : '') + v;
  L.push('# 🗺️ 라셀 지형과 자리표 — 실제 좌표',
    '', '> **★이 문서는 손으로 고치지 않는다.** `도구/_rasel_plan.json`이 원본이고, 이 문서는 거울이다.',
    '> 갱신: `node 도구/진단/_verify_coords.js --write`',
    '>', '> 단위 m · 원점 = 장터 큰길 한복판 · **+X 동(내륙) / −X 서(바다) / +Y 북** — 규약은 `00_좌표계_규약.md`',
    `>`, `> 생성 ${new Date().toISOString().slice(0, 10)}`, '', '---', '');

  L.push('## 1. 구역 여덟', '', '| 코드 | 이름 | 조직 | X 범위 | Y 범위 | 크기 |', '|---|---|---|---:|---:|---:|');
  for (const d of p.districts) {
    const w = Math.round(d.rect[2] - d.rect[0]), h = Math.round(d.rect[3] - d.rect[1]);
    L.push(`| **${d.id}** | ${d.name} | ${d.kind} | ${f(d.rect[0])} ~ ${f(d.rect[2])} | ${f(d.rect[1])} ~ ${f(d.rect[3])} | ${w} × ${h} |`);
  }

  L.push('', '## 2. 레벨 자리 스물둘', '',
    '| 번호 | 이름 | X | Y | Z | 구역 | 비고 |', '|---|---|---:|---:|---:|:--:|---|');
  for (const l of p.levels.slice().sort((a, b) => a.id.localeCompare(b.id)))
    L.push(`| **${l.id}** | ${l.n} | ${f(l.x)} | ${f(l.y)} | ${f(l.z)} | ${l.g} | ${l.note || ''} |`);

  L.push('', '## 3. 아래층 통로', '', '| 이름 | z | 폭 | 시작 → 끝 |', '|---|---:|---:|---|');
  for (const u of (p.under || [])) {
    const a = u.pts[0], b = u.pts[u.pts.length - 1];
    L.push(`| ${u.name} | ${u.z} | ${u.w} | (${Math.round(a[0])}, ${Math.round(a[1])}) → (${Math.round(b[0])}, ${Math.round(b[1])}) |`);
  }
  if (p.hall) L.push(`| ${p.hall.name} | ${p.hall.z} | — | 중심 (${p.hall.x}, ${p.hall.y}) · ${p.hall.w} × ${p.hall.h} |`);
  if (p.platform) L.push(`| ${p.platform.name} | ${p.platform.z} | — | 중심 (${p.platform.x}, ${p.platform.y}) · ${p.platform.w} × ${p.platform.h} |`);

  L.push('', '## 4. 다리 다섯', '', '| 이름 | 남단 | 북단 | 길이 |', '|---|---|---|---:|');
  for (const b of p.bridges) {
    const [a, c] = [b.pts[0], b.pts[b.pts.length - 1]];
    L.push(`| ${b.name} | (${a[0]}, ${a[1]}) | (${c[0]}, ${c[1]}) | ${Math.round(Math.hypot(c[0] - a[0], c[1] - a[1]))} m |`);
  }

  L.push('', '## 5. 지하철 역 여덟', '', '| 역 | X | Y | 어느 구역 |', '|---|---:|---:|---|');
  for (const s of p.stations) {
    const d = p.districts.find(d => s.x >= d.rect[0] && s.x <= d.rect[2] && s.y >= d.rect[1] && s.y <= d.rect[3]);
    L.push(`| ${s.n} | ${f(s.x)} | ${f(s.y)} | ${d ? d.id + ' ' + d.name : '— (구역 밖)'} |`);
  }

  L.push('', '## 6. 광장·공터 다섯', '', '| 이름 | 종류 | 중심 | 반경 |', '|---|---|---|---:|');
  for (const v of (p.voids || []))
    L.push(`| ${v.name} | ${v.kind} | (${v.x}, ${v.y}) | ${v.r} m |`);

  L.push('', '## 7. 물길', '', '| 이름 | 시작 | 끝 |', '|---|---|---|');
  for (const c of (p.channels || [])) {
    const a = c.pts[0], b = c.pts[c.pts.length - 1];
    L.push(`| ${c.name} | (${Math.round(a[0])}, ${Math.round(a[1])}) | (${Math.round(b[0])}, ${Math.round(b[1])}) |`);
  }
  if (p.dry) {
    const a = p.dry.pts[0], b = p.dry.pts[p.dry.pts.length - 1];
    L.push(`| ${p.dry.name} (마른 골) | (${Math.round(a[0])}, ${Math.round(a[1])}) | (${Math.round(b[0])}, ${Math.round(b[1])}) |`);
  }

  L.push('', '---', '', `## 8. 마지막 검사 결과`, '',
    `오류 ${n('오류')} · 경고 ${n('경고')} · 알림 ${n('알림')} — 자세한 건 \`02_모순대장과_재번호.md\``, '');

  fs.mkdirSync(DOCDIR, { recursive: true });
  fs.writeFileSync(path.join(DOCDIR, '01_지형과_자리표.md'), L.join('\n'), 'utf8');
  console.log('\n✔ 01_지형과_자리표.md 다시 씀');
}

process.exitCode = n('오류') > 0 ? 1 : 0;
