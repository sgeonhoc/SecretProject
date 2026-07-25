// 전서 도해 모듈 — 살란 전서의 텍스트 부(Ⅰ부·Ⅲ부)에 끼워 넣는 SVG 도해들.
// 사용자 지시(2026-07-17 21:01): "나 모든페이지들 중에 문자쪽에 그림이랑 같이 하는것들만 잘 읽혀. 다 그렇게해줘."
// 키 = 절 제목에 들어 있는 고유 문자열(빌더가 제목 매칭 후 바로 뒤에 삽입). 화풍·클래스는 도감(Ⅱ부)과 동일.
// 정본 md에는 손대지 않는다 — 도해는 빌드 산출물에만 실린다. 재생성: node _build_살란_전서.js

function box(x, y, w, h, title, subs, opt) {
  opt = opt || {};
  const stroke = opt.stroke || '#b3a67f';
  const dash = opt.dash ? ` stroke-dasharray="${opt.dash}"` : '';
  let s = `<rect x="${x}" y="${y}" width="${w}" height="${h}" rx="8" style="fill:${opt.fill || '#f2ecdf'};stroke:${stroke};stroke-width:2"${dash}/>`;
  s += `<text x="${x + w / 2}" y="${y + 20}" text-anchor="middle" style="font-size:13px;font-weight:bold;fill:${opt.tfill || '#4a3b2a'}">${title}</text>`;
  (subs || []).forEach((t, i) => {
    s += `<text x="${x + w / 2}" y="${y + 38 + i * 15}" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">${t}</text>`;
  });
  return s;
}
function arrow(x1, y1, x2, y2, opt) {
  opt = opt || {};
  const st = opt.dash ? `stroke-dasharray:${opt.dash};` : '';
  const c = opt.color || '#8a7a5c';
  const a = Math.atan2(y2 - y1, x2 - x1);
  const hx1 = x2 - 9 * Math.cos(a - 0.4), hy1 = y2 - 9 * Math.sin(a - 0.4);
  const hx2 = x2 - 9 * Math.cos(a + 0.4), hy2 = y2 - 9 * Math.sin(a + 0.4);
  return `<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" style="stroke:${c};stroke-width:2;${st}"/>` +
    `<path d="M${x2},${y2} L${hx1},${hy1} M${x2},${y2} L${hx2},${hy2}" style="stroke:${c};stroke-width:2;fill:none"/>`;
}
function panel(svgW, svgH, inner, caption) {
  return `<div class="panel" style="text-align:center"><svg width="${svgW}" height="${svgH}" viewBox="0 0 ${svgW} ${svgH}" style="max-width:100%">${inner}</svg>` +
    (caption ? `<div class="caption">${caption}</div>` : '') + `</div>`;
}

// ───────────────────────── Ⅲ부 (회로와 기술) ─────────────────────────

// §2 다섯 길 — 한눈 그림
const fivePaths = (() => {
  let s = '';
  s += `<circle cx="350" cy="50" r="28" style="fill:#e7dcc3;stroke:#8a7a5c;stroke-width:2.5"/>`;
  s += `<text x="350" y="55" text-anchor="middle" style="font-size:14px;font-weight:bold;fill:#4a3b2a">터</text>`;
  s += `<text x="392" y="46" style="font-size:10.5px;fill:#6b6353">다중차원의 마력이 들어오는 어귀</text>`;
  s += `<text x="392" y="60" style="font-size:10.5px;fill:#6b6353">— 늘 열려 있다(마나 통 없음, §4-3)</text>`;
  s += arrow(350, 78, 350, 116);
  s += `<rect x="245" y="118" width="210" height="42" rx="8" style="fill:#e7dcc3;stroke:#8a7a5c;stroke-width:2.5"/>`;
  s += `<text x="350" y="144" text-anchor="middle" style="font-size:13.5px;font-weight:bold;fill:#4a3b2a">회로 — 의지가 긋는 길</text>`;
  const cols = [
    ['몸길 (연공)', ['제2의 신체가 바탕', '평생 미리 낸 산 길', '잠들지 않는다']],
    ['소리길 (언령)', ['목소리가 바탕', '그때그때 흐르는 길', '살란 문장 = 회로']],
    ['그림길 (진·술식)', ['새긴 홈이 바탕', '잠든 길', '부어야 깬다']],
    ['매개길 (주술)', ['닮음·닿음의 인연', '길의 반은 세상 몫', '눈 없이 과녁을 잡음']],
    ['비는 길 (신앙)', ['고대신의 자취', '제가 긋지 않는다', '조련이 안 된다']],
  ];
  cols.forEach((c, i) => {
    const x = 10 + i * 138, cx = x + 64;
    s += arrow(350, 160, cx, 226);
    s += box(x, 228, 128, 96, c[0], c[1], i === 4 ? { stroke: '#a13b2e', dash: '6 4' } : {});
  });
  s += `<text x="350" y="360" text-anchor="middle" style="font-size:11.5px;fill:#5f5744">바탕이 달라도 문법은 하나 — 길의 일곱 값(길·너비·깊이·틈·층·건널목·심)이 다섯 길 모두에 선다(§1-3)</text>`;
  s += `<text x="350" y="380" text-anchor="middle" style="font-size:11.5px;fill:#a13b2e">비는 길만 조련의 법(§1-2) 밖이다 — 빌린 길은 길들일 수도, 넓힐 수도 없다</text>`;
  return panel(700, 395, s,
    '다섯 길 한눈 그림 — 같은 터, 같은 문법, 다섯 바탕. 어느 길로 긋는가가 술자의 갈래(연공·언령·진·주술·신앙)를 정한다.');
})();

// §4-3 그릇의 셈 — 물꼬 그림
const vesselCalc = (() => {
  let s = '';
  // 왼쪽: 가는 물꼬 T1
  s += `<text x="170" y="42" text-anchor="middle" style="font-size:13px;font-weight:bold;fill:#4a3b2a">가는 물꼬 — 낮은 그릇</text>`;
  s += `<path d="M150,58 L164,58 L164,150 L150,150 M190,58 L176,58 L176,150 L190,150" style="stroke:#8a7a5c;stroke-width:2.5;fill:none"/>`;
  s += `<rect x="165" y="58" width="10" height="92" style="fill:#b8c4c9"/>`;
  s += arrow(90, 176, 250, 176);
  s += `<text x="170" y="196" text-anchor="middle" style="font-size:11px;fill:#6b6353">노래 한 곡을 붓는다 (긴 시간)</text>`;
  s += `<rect x="110" y="208" width="120" height="26" style="fill:#e7dcc3;stroke:#b3a67f;stroke-width:2"/>`;
  s += `<text x="170" y="225" text-anchor="middle" style="font-size:11px;fill:#4a3b2a">같은 총량</text>`;
  // 가운데 등식
  s += `<text x="340" y="115" text-anchor="middle" style="font-size:15px;font-weight:bold;fill:#4a3b2a">총량 = 굵기 × 시간</text>`;
  s += `<text x="340" y="136" text-anchor="middle" style="font-size:11px;fill:#6b6353">T사다리 = 굵기의 사다리(§4-3)</text>`;
  // 오른쪽: 굵은 물꼬 T4
  s += `<text x="510" y="42" text-anchor="middle" style="font-size:13px;font-weight:bold;fill:#4a3b2a">굵은 물꼬 — 높은 그릇</text>`;
  s += `<path d="M462,58 L478,58 L478,150 L462,150 M558,58 L542,58 L542,150 L558,150" style="stroke:#8a7a5c;stroke-width:2.5;fill:none"/>`;
  s += `<rect x="479" y="58" width="62" height="92" style="fill:#b8c4c9"/>`;
  s += arrow(470, 176, 550, 176);
  s += `<text x="510" y="196" text-anchor="middle" style="font-size:11px;fill:#6b6353">숨 한 번 (짧은 시간)</text>`;
  s += `<rect x="450" y="208" width="120" height="26" style="fill:#e7dcc3;stroke:#b3a67f;stroke-width:2"/>`;
  s += `<text x="510" y="225" text-anchor="middle" style="font-size:11px;fill:#4a3b2a">같은 총량</text>`;
  s += `<text x="340" y="266" text-anchor="middle" style="font-size:11.5px;fill:#5f5744">이 세계에 마나 통은 없다 — 한계는 담아 둔 양이 아니라 한 호흡에 나를 수 있는 굵기다</text>`;
  s += `<text x="340" y="286" text-anchor="middle" style="font-size:11.5px;fill:#5f5744">그래서 큰 문장은 빠를 수 없고, 빠른 문장은 클 수 없다 — 값은 몸이 아니라 시간과 자리가 문다</text>`;
  return panel(680, 300, s,
    '그릇 = 곳간이 아니라 물꼬. 게임 환산: 마나 게이지 대신 출력 상한(T) + 붓는 시간(영창) + 그동안의 무방비(자리).');
})();

// §4-1 슬롯 — 문장이 곧 조합표
const slotTable = (() => {
  let s = '';
  const slots = [
    ['부름말', ['무엇을', '서고에 이백']],
    ['꼴지음', ['어떤 꼴로', '케-줄 여남은']],
    ['과녁', ['어디에', '모-줄 대여섯']],
    ['시간', ['얼마 동안', '엔·미엔·사란']],
    ['맺음', ['어떻게 끝나나', '-타 · -사']],
  ];
  slots.forEach((c, i) => {
    const x = 15 + i * 132;
    s += box(x, 40, 118, 70, c[0], c[1]);
    if (i < 4) s += `<text x="${x + 125}" y="82" text-anchor="middle" style="font-size:16px;font-weight:bold;fill:#8a7a5c">×</text>`;
  });
  s += `<text x="345" y="142" text-anchor="middle" style="font-size:13px;font-weight:bold;fill:#4a3b2a">= 수십만 갈래(§4-1) — 그리고 같은 문장도 벼림이 다르면 딴 기술(§3-3)</text>`;
  const ex = ['에 카르', '케 렌', '모 이', '(비움 = 숨 그대로)', '카르타'];
  ex.forEach((t, i) => {
    const x = 15 + i * 132;
    const empty = i === 3;
    s += `<rect x="${x}" y="196" width="118" height="34" rx="6" style="fill:${empty ? 'none' : '#e7dcc3'};stroke:${empty ? '#a13b2e' : '#b3a67f'};stroke-width:2${empty ? ';stroke-dasharray:5 4' : ''}"/>`;
    s += `<text x="${x + 59}" y="217" text-anchor="middle" style="font-size:${empty ? 10 : 12}px;fill:${empty ? '#a13b2e' : '#2e2a22'}">${t}</text>`;
    s += arrow(x + 59, 154, x + 59, 192, { dash: '4 4' });
  });
  return panel(690, 250, s,
    '아랫줄 = 본보기 「실금」(§5-0). 슬롯마다 낱말을 갈아 끼우면 딴 기술이 선다 — 문장이 곧 조합표다. 가로(조합) × 세로(벼림·겹 얹기)가 이 체계의 진짜 넓이.');
})();

// §5-19 병서 한 장 — 언령전 다섯 마디
const warManual = (() => {
  let s = '';
  const steps = [
    ['① 눈 끊기', ['안개 이불 · 덮그늘', '낯바람 · 눈부심', '과녁을 지운다(§4-6)']],
    ['② 귀', ['실소리 · 딴 데 소리', '북 · 가라앉힌 심', '듣는 쪽을 잡는다']],
    ['③ 먼저 맺기', ['새긴 세움(빠른 손)', '혀에 얹기(-노 대기)', '먼저 맺는 쪽이 값을 정한다']],
    ['④ 맺힌 다음', ['-타 합 — 세상의 일', '깊은 손 · 딴 이능', '이제 말로는 못 물린다']],
    ['⑤ 예외', ['한 마디 발현 노장', '"물러나는 차례"', '차례 전체를 건너뛴다']],
  ];
  steps.forEach((c, i) => {
    const x = 10 + i * 136;
    s += box(x, 36, 124, 92, c[0], c[1], i === 4 ? { stroke: '#a13b2e' } : {});
    if (i < 4) s += arrow(x + 126, 82, x + 134, 82);
  });
  s += `<text x="78" y="150" text-anchor="middle" style="font-size:10.5px;fill:#a13b2e">매개길은 이 마디를 건너뛴다(§2-4)</text>`;
  return panel(700, 175, s,
    '언령전의 차례(§5-19) — 다섯 마디에 기술첩의 수들이 앉는다. 성을 가질 셈이면 끌을 아껴라(진 재우기 §5-18).');
})();

// ───────────────────────── Ⅰ부 (언어와 문자) ─────────────────────────

// §4 문장 — 영창의 네 자리
const fourSeats = (() => {
  let s = '';
  const seats = [
    ['세움', ['나를 벼린다', '몸과 마음의 줄']],
    ['부름', ['씨를 부른다', '에 + 부름말']],
    ['꼴지음', ['모양·크기·과녁·시간', '케 · 재는말 · 모']],
    ['맺음', ['박는다', '-타 / -사']],
  ];
  seats.forEach((c, i) => {
    const x = 15 + i * 168;
    s += box(x, 36, 152, 70, c[0], c[1]);
    if (i < 3) s += arrow(x + 154, 71, x + 166, 71);
  });
  const ex = ['안 벨 네', '에 자르', '케 카나 · 모 이', '자르타'];
  ex.forEach((t, i) => {
    const x = 15 + i * 168;
    s += `<rect x="${x}" y="140" width="152" height="34" rx="6" style="fill:#e7dcc3;stroke:#b3a67f;stroke-width:2"/>`;
    s += `<text x="${x + 76}" y="161" text-anchor="middle" style="font-size:12px;fill:#2e2a22">${t}</text>`;
    s += arrow(x + 76, 108, x + 76, 136, { dash: '4 4' });
  });
  s += `<text x="345" y="205" text-anchor="middle" style="font-size:11.5px;fill:#5f5744">본보기 — 여문 불의 네 줄(§5-2). 자리의 차례는 안 바뀐다 — 자리가 곧 문법이다.</text>`;
  return panel(690, 225, s,
    '줄이기(§4-5)는 자리를 버리는 것이 아니라 새김·몸이 그 자리를 대신 지는 것이다 — Ⅱ부 ⅩⅣ 겹 얹기 그림 참조.');
})();

// §3-3 맺음말 — -타/-사/-노 갈림
const endingFork = (() => {
  let s = '';
  s += `<rect x="20" y="130" width="230" height="46" rx="8" style="fill:#e7dcc3;stroke:#8a7a5c;stroke-width:2.5"/>`;
  s += `<text x="135" y="150" text-anchor="middle" style="font-size:12.5px;font-weight:bold;fill:#4a3b2a">문장 — 에 두르 · 가르 …</text>`;
  s += `<text x="135" y="167" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">(돌벽 §5-4 — 꼬리만 남았다)</text>`;
  const forks = [
    [20, '두르타 — 세상에 넘긴다', ['맺힌 돌벽은 그냥 돌 — 마력이 더 안 든다', '허묾(-노)도 못 듣는다 — 허물려면 끌과 품'], {}],
    [125, '두르사 — 마력이 붙든다', ['도는 동안만 선다 — 술자가 놓으면 스러진다', '세운 이의 두르노가 듣는다'], { dash: '7 4' }],
    [230, '두르노 — 허문다', ['듣는 것: 도는 것(-사) · 아직 안 맺힌 문장', '같은 이름이라야 — 두르에는 두르노'], { stroke: '#a13b2e' }],
  ];
  forks.forEach(f => {
    s += arrow(250, 153, 330, f[0] + 33);
    s += box(335, f[0], 320, 66, f[1], f[2], f[3]);
  });
  return panel(680, 320, s,
    '맺음말 갈림(§3-3) — 꼬리 하나가 문장의 남은 평생을 정한다. -타는 되돌릴 수 없고, -사는 붙드는 값을 계속 물며, -노는 살란이 아직 쥔 것만 허문다.');
})();

// §1-3 길의 일곱 값 — 회로 단면 흐름도
const sevenValues = (() => {
  let s = '';
  s += `<circle cx="34" cy="165" r="20" style="fill:#e7dcc3;stroke:#8a7a5c;stroke-width:2.5"/>`;
  s += `<text x="34" y="170" text-anchor="middle" style="font-size:12px;font-weight:bold;fill:#4a3b2a">터</text>`;
  const band = (x1, x2, y, h, fill) => `<rect x="${x1}" y="${y - h / 2}" width="${x2 - x1}" height="${h}" style="fill:${fill || '#b8c4c9'};stroke:#8a7a5c;stroke-width:1.5"/>`;
  s += band(56, 170, 165, 14);
  s += band(170, 250, 165, 14, '#7e939b');
  s += band(285, 385, 165, 14);
  // 너비 (양쪽 화살)
  s += arrow(115, 130, 115, 155); s += arrow(115, 200, 115, 175);
  s += `<text x="115" y="122" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">너비 — 얼마나 머무는가</text>`;
  // 길 (띠 그 자체)
  s += `<line x1="88" y1="196" x2="88" y2="176" style="stroke:#8a7a5c;stroke-width:1.5;stroke-dasharray:3 3"/>`;
  s += `<text x="88" y="212" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">길 — 꼴 그 자체</text>`;
  // 깊이
  s += `<text x="210" y="196" text-anchor="middle" style="font-size:10.5px;fill:#4a3b2a">깊이 — 힘준 마디(짙기)</text>`;
  // 틈
  s += `<path d="M250,163 Q267,132 285,163" style="stroke:#8a7a5c;stroke-width:2;fill:none;stroke-dasharray:5 4"/>`;
  s += `<text x="267" y="122" text-anchor="middle" style="font-size:10.5px;fill:#a13b2e">틈 — 숨자리 (없으면 넘친다)</text>`;
  // 건널목 (갈림 마름모)
  s += `<g transform="translate(392,165) rotate(45)"><rect x="-9" y="-9" width="18" height="18" style="fill:#e7dcc3;stroke:#8a7a5c;stroke-width:2"/></g>`;
  s += `<text x="352" y="222" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">건널목 — 갈래를 잇는 자리</text>`;
  s += `<line x1="372" y1="212" x2="390" y2="176" style="stroke:#8a7a5c;stroke-width:1.5;stroke-dasharray:3 3"/>`;
  // 두 갈래 (층)
  s += `<path d="M398,158 C412,132 424,120 445,120 L530,120" style="stroke:#b8c4c9;stroke-width:12;fill:none;stroke-linecap:round"/>`;
  s += `<path d="M398,172 C412,198 424,210 445,210 L530,210" style="stroke:#b8c4c9;stroke-width:12;fill:none;stroke-linecap:round"/>`;
  s += `<text x="470" y="102" text-anchor="middle" style="font-size:10.5px;fill:#4a3b2a">안층 — 안이 바깥을 부린다</text>`;
  s += `<text x="470" y="234" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">바깥층</text>`;
  s += arrow(470, 130, 470, 202, { dash: '4 4', color: '#a13b2e' });
  // 심 (모이는 자리)
  s += `<path d="M530,120 C552,120 562,140 568,152 M530,210 C552,210 562,190 568,178" style="stroke:#b8c4c9;stroke-width:12;fill:none;stroke-linecap:round"/>`;
  s += `<circle cx="580" cy="165" r="17" style="fill:#e7dcc3;stroke:#4a3b2a;stroke-width:2.5"/>`;
  s += `<text x="580" y="170" text-anchor="middle" style="font-size:12px;font-weight:bold;fill:#4a3b2a">심</text>`;
  s += `<text x="580" y="200" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">갈린 길이 모여 맺는 자리</text>`;
  s += arrow(600, 165, 652, 165);
  s += `<text x="656" y="170" style="font-size:13px;font-weight:bold;fill:#4a3b2a">발현</text>`;
  return panel(700, 250, s,
    '길의 일곱 값(§1-3) — 어느 바탕(몸·소리·그림·매개)의 길이든 이 일곱이 길의 전부다. 소리길엔 심이 없다(소리는 한 줄로 흐른다) — 심은 술식과 몸의 것.');
})();

// §1-2 조련의 법 — 세 조항
const tamingLaw = (() => {
  let s = '';
  // ① 오래 길든 길은 너그럽다
  s += `<text x="120" y="36" text-anchor="middle" style="font-size:12.5px;font-weight:bold;fill:#4a3b2a">① 오래 길들수록 너그럽다</text>`;
  s += `<rect x="45" y="58" width="150" height="44" rx="10" style="fill:#7e939b;stroke:#8a7a5c;stroke-width:2"/>`;
  s += `<text x="120" y="118" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">만 년의 자르 — 서툰 입도 받는다</text>`;
  s += `<rect x="45" y="138" width="150" height="8" rx="4" style="fill:#cfd8db;stroke:#b3a67f;stroke-width:1.5"/>`;
  s += `<text x="120" y="164" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">설운 낱말 — 바른 입만 받는다</text>`;
  s += `<text x="120" y="182" text-anchor="middle" style="font-size:10px;fill:#a13b2e">"그래도 말은 해야 알아듣는다"</text>`;
  // ② 잘 가르칠수록 밝다
  s += `<text x="350" y="36" text-anchor="middle" style="font-size:12.5px;font-weight:bold;fill:#4a3b2a">② 잘 가르칠수록 밝다</text>`;
  s += `<path d="M350,62 L320,100 L380,100 Z" style="fill:#e7dcc3;stroke:#8a7a5c;stroke-width:2"/>`;
  s += `<path d="M282,166 L302,166 L302,148 L322,148 L322,130 L342,130 L342,112 L350,104" style="stroke:#4a3b2a;stroke-width:2.5;fill:none"/>`;
  s += `<path d="M420,166 Q450,150 428,136 Q404,124 430,112 Q452,102 408,96 Q380,92 362,100" style="stroke:#8a7a5c;stroke-width:2;fill:none;stroke-dasharray:5 4"/>`;
  s += `<text x="300" y="186" text-anchor="middle" style="font-size:10.5px;fill:#4a3b2a">가르침 — 비전의 첫 획</text>`;
  s += `<text x="428" y="186" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">독학 — 같은 자리에 십 년</text>`;
  // ③ 똑바로 청해야 알아듣는다
  s += `<text x="580" y="36" text-anchor="middle" style="font-size:12.5px;font-weight:bold;fill:#4a3b2a">③ 똑바로 청해야 알아듣는다</text>`;
  s += `<circle cx="510" cy="112" r="13" style="fill:#e7dcc3;stroke:#8a7a5c;stroke-width:2"/>`;
  s += `<text x="510" y="117" text-anchor="middle" style="font-size:10px;fill:#4a3b2a">뜻</text>`;
  s += arrow(525, 104, 640, 76);
  s += `<circle cx="650" cy="74" r="7" style="fill:none;stroke:#4a3b2a;stroke-width:2"/><circle cx="650" cy="74" r="2.5" style="fill:#4a3b2a"/>`;
  s += `<text x="592" y="60" text-anchor="middle" style="font-size:10.5px;fill:#4a3b2a">또렷한 청 → 발현</text>`;
  s += `<path d="M523,122 Q550,136 568,142 Q586,148 598,150" style="stroke:#a13b2e;stroke-width:2;fill:none;stroke-dasharray:7 7"/>`;
  s += `<text x="612" y="158" style="font-size:12px;fill:#a13b2e">…</text>`;
  s += `<text x="586" y="180" text-anchor="middle" style="font-size:10.5px;fill:#a13b2e">흐릿한 청 — 아무것도 안 온다</text>`;
  s += `<text x="350" y="226" text-anchor="middle" style="font-size:11.5px;fill:#5f5744">알아듣는 것은 마력이 아니라 길이다 — 마력 자체는 셈도 귀도 없다(§1-2)</text>`;
  return panel(700, 245, s,
    '조련의 법 세 조항(사용자 확정 캐논) — 이 법은 능력자를 묶는 사슬이 아니라 딛는 계단이다: 체계 없는 천재보다 체계 위의 범재가 멀리 간다.');
})();

// §3-4 배움의 네 마당 — 2×2
const fourYards = (() => {
  let s = '';
  s += box(30, 30, 280, 96, '학당 — 문장에서 시작', ['소리 세 해 · 새김 · 과녁 시험 — 느리고 고르다', '표 = 거둘 줄 안다(세움과 허묾을 한 상에서)']);
  s += box(390, 30, 280, 96, '산문 — 몸에서 시작', ['문장은 몸이 선 다음 — 제일 느리다', '표 = 그 몸길은 늙어도 안 무너진다']);
  s += box(30, 168, 280, 96, '군문 — 쓰임에서 시작', ['문장 열 개를 얕게 — 살아남는 것부터', '표 = 겹과 파훼(마당에서 구른 되받기)']);
  s += box(390, 168, 280, 96, '저자 — 삯에서 시작', ['배움이 아니라 어깨너머 — 좁고 깊다', '표 = 평생 한 문장의 깊이']);
  s += `<rect x="315" y="126" width="70" height="42" rx="8" style="fill:#e7dcc3;stroke:#8a7a5c;stroke-width:2"/>`;
  s += `<text x="350" y="143" text-anchor="middle" style="font-size:10.5px;font-weight:bold;fill:#4a3b2a">같은 기술</text>`;
  s += `<text x="350" y="158" text-anchor="middle" style="font-size:10.5px;font-weight:bold;fill:#4a3b2a">딴 손</text>`;
  s += `<text x="350" y="296" text-anchor="middle" style="font-size:11px;fill:#5f5744">넷이 전부가 아니다 — 사이사이에 사사 · 사본 독학 · 어깨너머 · 가문 전수, 그리고 훔쳐 배움(§3-5)</text>`;
  return panel(700, 315, s,
    '배움의 네 마당(§3-4) — 같은 여문 불이 학당 손에서는 반듯하고, 군문 손에서는 빠르고, 저자 손에서는 아궁이 크기로 정확하다. 게임 축: 조합 × 벼림 × 마당(출신).');
})();

// Ⅰ부 §2-2 여섯 결 — 소리 등고선
const sixGrains = (() => {
  let s = '';
  s += `<line x1="40" y1="160" x2="660" y2="160" style="stroke:#8a7a5c;stroke-width:1.5;stroke-dasharray:6 5"/>`;
  s += `<text x="44" y="250" style="font-size:10.5px;fill:#6b6353">— — 제 가운데(모든 높이의 기준 — 피리 구멍이 아니라 술자의 목이 기준이다)</text>`;
  const ink = 'stroke:#4a3b2a;fill:none;stroke-linecap:round';
  // 높이 (가운데 위로 오른 마디)
  s += `<path d="M70,158 Q85,112 118,118" style="${ink};stroke-width:4"/>`;
  s += `<line x1="94" y1="108" x2="94" y2="78" style="stroke:#8a7a5c;stroke-width:1;stroke-dasharray:3 3"/>`;
  s += `<text x="94" y="66" text-anchor="middle" style="font-size:10.5px;fill:#4a3b2a">높이 — 소리가 앉는 자리</text>`;
  // 길이 (길게 끈 마디)
  s += `<path d="M138,140 L232,140" style="${ink};stroke-width:4"/>`;
  s += `<line x1="185" y1="134" x2="185" y2="104" style="stroke:#8a7a5c;stroke-width:1;stroke-dasharray:3 3"/>`;
  s += `<text x="185" y="94" text-anchor="middle" style="font-size:10.5px;fill:#4a3b2a">길이 — 끄는가 끊는가</text>`;
  // 세기 (힘준 마디 = 굵게)
  s += `<path d="M252,150 L312,146" style="${ink};stroke-width:9"/>`;
  s += `<line x1="282" y1="138" x2="282" y2="72" style="stroke:#8a7a5c;stroke-width:1;stroke-dasharray:3 3"/>`;
  s += `<text x="282" y="62" text-anchor="middle" style="font-size:10.5px;fill:#4a3b2a">세기 — 힘준 마디에 심이 선다</text>`;
  // 꺾임 (끝을 올림)
  s += `<path d="M332,150 Q356,152 372,146 Q384,140 388,124" style="${ink};stroke-width:4"/>`;
  s += `<line x1="388" y1="116" x2="388" y2="104" style="stroke:#8a7a5c;stroke-width:1;stroke-dasharray:3 3"/>`;
  s += `<text x="392" y="94" text-anchor="middle" style="font-size:10.5px;fill:#4a3b2a">꺾임 — 끝을 올리나 내리나</text>`;
  // 이음 (붙임 고리)
  s += `<path d="M388,152 Q402,170 416,152" style="stroke:#8a7a5c;stroke-width:1.7;fill:none"/>`;
  s += `<text x="402" y="196" text-anchor="middle" style="font-size:10.5px;fill:#6b6353">이음 — 붙이는가 떼는가</text>`;
  s += `<path d="M416,150 L492,150" style="${ink};stroke-width:4"/>`;
  // 숨자리 (가라앉는 골)
  s += `<path d="M500,166 Q514,182 528,166" style="stroke:#a13b2e;stroke-width:2;fill:none"/>`;
  s += `<text x="514" y="220" text-anchor="middle" style="font-size:10.5px;fill:#a13b2e">숨자리 — 여기서 가라앉았다 다시 붙는다</text>`;
  s += `<path d="M536,148 L624,144" style="${ink};stroke-width:4"/>`;
  return panel(700, 265, s,
    '여섯 결(§2-2) — 살란이 보는 것은 목소리의 재질이 아니라 소리의 꼴이다. 같은 곡을 다른 목소리로 부르듯, 여섯 결이 맞으면 누구의 입에서든 같은 주문이 선다. 어긋나면: 딴 낱말 · 설익은 맺힘 · 심 어긋남 · 가장자리 일그러짐 · 두 동강 · 붓던 것이 쏟아짐.');
})();

// §6 겹창 — 이어달리기 그림
const choralRelay = (() => {
  let s = '';
  s += `<text x="350" y="22" text-anchor="middle" style="font-size:12.5px;font-weight:bold;fill:#4a3b2a">한 문장, 백 입 — 라 자리에서만 입이 바뀐다</text>`;
  const segs = [
    [14, 136, '첫째 입', ['세움줄부터', '제 토막까지'], {}],
    [190, 136, '둘째 입', ['앞입의 끝에', '이음의 결로 붙는다'], {}],
    [366, 140, '… 아흔아홉째 입', ['들판만 한 꼴을', '나눠 채운다'], { dash: '6 4' }],
    [540, 146, '맺는 입 — 우두머리', ['맺음말은 한 입', '그릇이 곧 천장'], { stroke: '#a13b2e' }],
  ];
  segs.forEach(g => { s += box(g[0], 40, g[1], 64, g[2], g[3], g[4]); });
  [[170, '라'], [346, '라'], [523, '라']].forEach(c => {
    s += `<circle cx="${c[0]}" cy="72" r="12" style="fill:#e7dcc3;stroke:#8a7a5c;stroke-width:2"/>`;
    s += `<text x="${c[0]}" y="76" text-anchor="middle" style="font-size:11px;font-weight:bold;fill:#4a3b2a">${c[1]}</text>`;
    s += `<text x="${c[0]}" y="96" text-anchor="middle" style="font-size:9px;fill:#8a5a2a">숨자리</text>`;
  });
  s += `<text x="350" y="128" text-anchor="middle" style="font-size:10.5px;fill:#5f5744">합창이 아니라 이어달리기 — 앞사람의 마지막 마디와 뒷사람의 첫 마디가 이음의 결로 붙고, 그 사이에 숨자리 하나</text>`;
  s += `<text x="350" y="148" text-anchor="middle" style="font-size:10.5px;fill:#a13b2e">한 입이 제 토막을 어긋내면 문장 전체가 두 동강 — 백 입이 부은 것은 백 입 몫으로 쏟아진다("겹창이 쏟아진 들")</text>`;
  s += box(14, 166, 216, 58, '사흘을 같이 먹고 잔다', ['토막을 받기 전에', '서로의 숨을 익힌다']);
  s += box(242, 166, 216, 58, '앓는 입은 아침에 빠진다', ['숨긴 기침이 백 명을 묻는다', '— 나무라는 우두머리는 없다']);
  s += box(470, 166, 216, 58, '우두머리는 이름을 다 왼다', ['쏟아지면 같이', '묻히는 이름들이라서']);
  return panel(700, 236, s,
    '겹창(§6) — 혼자서 못 채우는 문장을 여럿이 나눠 부른다. 맺음은 나누지 못하니 마지막 맺음말은 반드시 한 입에서 나오고, 문장에 엔(숨 한 번)이 들었으면 재는 숨도 맺는 입의 숨이다 — 맺는 입의 그릇이 겹창의 천장.');
})();

// §6-3 말문 막기 — 네 가지 길
const silencing = (() => {
  let s = '';
  // 가운데 영창 레일 (네 자리 + 숨자리)
  const rail = [[60, '세움줄'], [222, '부름줄'], [384, '꼴지음'], [546, '맺음말']];
  rail.forEach(r => { s += box(r[0], 150, 118, 44, r[1], [], r[1] === '맺음말' ? { stroke: '#a13b2e' } : {}); });
  [200, 362, 524].forEach((x, i) => {
    s += `<circle cx="${x}" cy="172" r="4" style="fill:#8a5a2a"/>`;
    if (i === 0) s += `<text x="${x}" y="208" text-anchor="middle" style="font-size:9px;fill:#8a5a2a">숨자리</text>`;
  });
  // 네 공격로
  s += box(14, 20, 190, 66, '① 소음 — 북·꽹과리', ['제 소리를 못 듣는 술자는', '제 어긋남을 모른다']);
  s += arrow(120, 86, 196, 146);
  s += box(496, 20, 190, 66, '② 숨 흔들기 — 화살·돌부리', ['숨이 흔들리면 숨자리가 무너지고', '붓던 것이 쏟아진다']);
  s += arrow(580, 86, 528, 146);
  s += box(14, 240, 220, 80, '③ 부름줄 엿듣기', ['"에 자르"가 들리면 불이 온다', '감추기 싸움: 부름줄 낮게 깔기 ·', '겹창 토막 · 세움줄 길게 끌기']);
  s += arrow(140, 240, 268, 198);
  s += box(466, 240, 220, 80, '④ 되받기 — -노를 혀에', ['맺히기 전에 치는 것이 싸다', '맺힌 뒤 허물기는 세운 이보다 깊은 그릇', '— 먼저 맺는 쪽이 값을 정한다']);
  s += arrow(560, 240, 596, 198);
  s += `<text x="350" y="348" text-anchor="middle" style="font-size:11px;font-weight:bold;fill:#a13b2e">"북으로 막을 수 있는 술자라면, 애초에 북으로 충분한 술자다" — 한 마디 발현(§4-5)의 노장에게는 북도 화살도 늦다</text>`;
  return panel(700, 362, s,
    '말문 막기(§6-3) — 여섯 결 가운데 하나만 어긋나도 문장이 무너진다는 것은, 어긋내려는 쪽에도 길이 여섯이라는 뜻이다. 언령을 배우지 않은 병사도 이것은 익힌다 — 익혀야 산다.');
})();

// §5 기술첩 한눈 지도
const skillbookMap = (() => {
  let s = '';
  s += `<text x="350" y="20" text-anchor="middle" style="font-size:12.5px;font-weight:bold;fill:#4a3b2a">계열 = 부름말 — 서고에 오른 것이 이백 부름말이니, 여기 적힌 것은 세상의 기술 가운데 한 줌</text>`;
  const fams = [
    ['자르', '불', '화로 곁'], ['시스', '서리', '여울 굳히기'], ['후르', '바람', '뒤바람'], ['마이', '물', '마른 골 물주기'],
    ['켈', '돌·땅', '하루 보루'], ['실란', '빛', '등대 불'], ['무네', '그늘', '곳간 그늘'],
    ['잔', '소리', '실소리'], ['람', '묶음', '비계 묶음'], ['루', '잠·고요', '고요'], ['옴', '무게', '저울눈 무게'],
    ['카르', '깨짐', '실금'], ['오르', '열림', ''], ['니르·코르', '아묾·닫힘', '아묾 · 빗장'],
  ];
  fams.forEach((f, i) => {
    const col = i % 7, row = Math.floor(i / 7);
    const x = 12 + col * 97, y = 32 + row * 72;
    s += `<rect x="${x}" y="${y}" width="90" height="64" rx="7" style="fill:#f2ecdf;stroke:#b3a67f;stroke-width:1.5"/>`;
    s += `<text x="${x + 45}" y="${y + 18}" text-anchor="middle" style="font-size:11.5px;font-weight:bold;fill:#4a3b2a">${f[0]}</text>`;
    s += `<text x="${x + 45}" y="${y + 34}" text-anchor="middle" style="font-size:9.5px;fill:#8a5a2a">${f[1]}</text>`;
    if (f[2]) s += `<text x="${x + 45}" y="${y + 52}" text-anchor="middle" style="font-size:9px;fill:#6b6353">「${f[2]}」</text>`;
  });
  const paths = [
    ['몸길 기술첩', '학당 공용 여덟 (연공)'], ['그림길 기술첩', '이름난 진꼴 (진·술식)'], ['매개길 기술첩', '인연을 쓰는 손 (주술)'],
    ['겹 얹기 기술첩', '두 바탕이 한 발현에'], ['허묾 기술첩', '-노의 손'],
  ];
  paths.forEach((p, i) => {
    const x = 12 + i * 138;
    s += box(x, 188, 130, 52, p[0], [p[1]]);
  });
  s += box(12, 252, 334, 48, '병서 한 장 — 언령전의 차례', ['기술첩이 전장에 서는 배치도(§5-19)']);
  s += box(358, 252, 330, 48, '금기 기술첩 — 금기 5 = 금서 5', ['하지 않기로 한 것들, 그것을 적은 책(§5-20)'], { stroke: '#a13b2e', dash: '6 4' });
  s += `<text x="350" y="322" text-anchor="middle" style="font-size:10.5px;fill:#5f5744">이 목록은 열린 목록이다 — 게임도감의 가문 기술과 겹치는 일은 겹치는 대로 둔다: 같은 일을 딴 길로 하는 것이 이 세계의 정상(§5 머리)</text>`;
  return panel(700, 334, s,
    '기술첩 한눈 지도(§5) — 위 두 줄이 부름말 계열 열넷(각 칸 아래는 그 계열의 이름난 기술 하나), 셋째 줄이 길별 기술첩, 마지막 줄이 병서와 금기. 오르(열림) 계열은 §5-13에 각론이 선다.');
})();

// §5-20 금기 기술첩 — 금기와 금서는 한 몸
const forbiddenBook = (() => {
  let s = '';
  s += `<text x="350" y="20" text-anchor="middle" style="font-size:12px;font-weight:bold;fill:#a13b2e">못 해서 금기가 된 것은 하나도 없다 — 전부 할 수 있는 일들이다</text>`;
  s += `<text x="350" y="40" text-anchor="middle" style="font-size:10.5px;fill:#5f5744">금기첩은 언제나 금서와 한 몸이다 — 하지 말라고 적은 종이가, 하는 법을 적은 종이가 된다</text>`;
  const rows = [
    ['깊은말 세우기', '무엇을 맺는지 아무도 모른다 — 읽지 않고 봉한다', '「아래층 낱장들」', '낱장보다 봉궤 명부가 더 자주 도둑맞는다'],
    ['이름 묶기', '묶는 것은 한 나절, 되돌리는 것은 되짚기 수십 년', '「긁는 차례」', '봤다는 자는 많고 내놓는 자는 없다'],
    ['남의 살 새김', '몸의 임자가 제 살갗의 문장을 못 거둔다', '「바늘의 어두운 장」', '태우는 예식이 곧 안 태운 장의 증거'],
    ['무덤 실', '건 사람이 못 끊는 실 — 드물게 다섯 대륙이 다 금한다', '「식은 실 문답」', '마지막 장은 물음만 남는다'],
    ['진 겹치기', '어느 쪽이 맺는지 셈이 안 선다 — 병서에 못 오른다', '「겹친 진의 병서」', '읽는 손에 따라 경고문이자 교본'],
  ];
  rows.forEach((r, i) => {
    const y = 54 + i * 60;
    s += box(14, y, 330, 52, r[0], [r[1]], { stroke: '#a13b2e' });
    s += box(376, y, 310, 52, r[2], [r[3]], { dash: '4 3' });
    s += `<line x1="344" y1="${y + 26}" x2="376" y2="${y + 26}" style="stroke:#8a7a5c;stroke-width:2;stroke-dasharray:3 3"/>`;
  });
  const seals = [['태우는 나라', '값만 올린다는 것이 저자의 셈'], ['서고에 봉하는 나라', '서고 규율과 한 몸'], ['관이 사들이는 나라', '금기를 관의 독점 기술로 — 제일 미움받는 길']];
  seals.forEach((c, i) => { s += box(14 + i * 230, 364, 218, 50, c[0], [c[1]]); });
  s += `<text x="350" y="438" text-anchor="middle" style="font-size:11px;font-weight:bold;fill:#a13b2e">셋 다 새는 것은 같다 — 베끼는 손은 언제나 지키는 손보다 많다</text>`;
  return panel(700, 448, s,
    '금기 기술첩(§5-20) — 왼쪽이 금기(빨간 테), 오른쪽이 그 금기와 한 몸인 금서(점선 테). 금기의 임자도 까닭도 하나가 아니다: 서고 규율·의방 규약·조합 내규·나라의 법·병가의 금장이 제각기 제 금기를 지키고, 지방이 갈리면 금기도 갈린다. 금서에 적히는 것은 문장이 아니라 차례와 벼림이다.');
})();

module.exports = {
  lang: {
    '문장 — 영창의 네 자리': fourSeats,
    '맺음말 — 문장을 닫는 꼬리': endingFork,
    '2-2. 여섯 결': sixGrains,
    '겹창 — 나눠 부르기': choralRelay,
    '말문 막기': silencing,
  },
  circ: {
    '다섯 길 — 같은 문법': fivePaths,
    '슬롯 — 문장이 곧 조합표': slotTable,
    '그릇의 셈': vesselCalc,
    '병서 한 장': warManual,
    '조련의 법': tamingLaw,
    '길의 일곱 값': sevenValues,
    '배움의 네 마당': fourYards,
    '기술첩 — 정본 기술 목록': skillbookMap,
    '금기 기술첩': forbiddenBook,
  },
};
