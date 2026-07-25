# -*- coding: utf-8 -*-
"""라셀 레벨 연결 훑기 — 맵마다 문(포탈)이 어디로 가는지 실제로 긁어 표로 만든다.

말로 적은 연결표는 금세 실제와 어긋난다. 그래서 **맵에서 직접 읽는다**:
  · 각 맵의 PortalActor → TargetLevelName (+ StoryFlag 조건)
  · 목표 맵이 실제로 있는지(끊긴 문 찾기)
  · 반대편에 돌아오는 문이 있는지(한쪽으로만 난 문 찾기)
산출: C:/Secret_Project/기획/02_게임설계/2_레벨/게임_레벨_연결표.md (사람이 읽는 정본) + Saved/links.log
"""
import unreal, os, json

MAPDIR = "/Game/Maps/Rasel"
OUT_MD = "C:/Secret_Project/기획/02_게임설계/2_레벨/게임_레벨_연결표.md"
lines = []


def log(m):
    lines.append(str(m))
    unreal.log("[links] " + str(m))


EAL = unreal.EditorAssetLibrary
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# 임시 맵(_next·_scratch·_park)과 잠금 우회본은 스캔에서 뺀다 — 정본 아님
maps = [p for p in EAL.list_assets(MAPDIR, recursive=False, include_folder=False)
        if not p.rsplit("/", 1)[1].startswith("_")]
names = sorted(set(p.split(".")[0].rsplit("/", 1)[1] for p in maps))
names = [n for n in names if not n.endswith(("_next", "_scratch", "_park"))]
log("맵 %d개: %s" % (len(names), ", ".join(names)))

# ── 정본 로스터(기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md의 Rasel_* Id) ─────────────────
# 실제 파일이 정본에 있나, 정본이 아직 안 지어졌나를 가른다.
CANON_IDS = [
    ("L01", "Jangteo_Street"), ("L02", "Antique_Shop"), ("L03", "Backalley"),
    ("L04", "Dolgan_Office"), ("L05", "Eatery"), ("L06", "Academy_Street"),
    ("L07", "Riverbank"), ("L08", "Clinic"), ("L09", "Newport_Site"),
    ("L10", "Newport_Shaft"), ("L11", "Dock_Wharf"), ("L12", "Warehouse"),
    ("L13", "Seir_Sanatorium"), ("L14", "Selan_Court"), ("L15", "Nerhan_Estate"),
    ("L16", "Abandoned_Platform"), ("L17", "Underlayer"), ("L18", "Ruin_Gate"),
    ("L19", "Ruin_Keep"), ("L20", "Cargo_Ship"), ("L21", "Din_Safehouse"),
    ("L22", "Yoa_Room"),
]
# 파일명은 접두 없이 저장돼 있다(L01_Jangteo_Street). Id의 뒷부분으로 맞춰 본다.
canon_tails = set(t for (_n, t) in CANON_IDS)

links = {}      # 맵 → [(목표, 라벨, 필요플래그, 금지플래그)]
counts = {}     # 맵 → (부재, 조사, NPC, 잠긴문)
for nmap in names:
    path = "%s/%s" % (MAPDIR, nmap)
    try:
        LES.load_level(path)
    except Exception as e:
        log("  !! 못 엶 %s (%s)" % (nmap, e))
        continue
    out, mesh, lore, npc, gate = [], 0, 0, 0, 0
    for a in A.get_all_level_actors():
        cn = a.get_class().get_name()
        if cn == "PortalActor":
            tgt = str(a.get_editor_property("TargetLevelName"))
            req = str(a.get_editor_property("RequiredFlag"))
            forb = str(a.get_editor_property("ForbiddenFlag"))
            out.append((tgt, a.get_actor_label(),
                        "" if req in ("None", "") else req,
                        "" if forb in ("None", "") else forb))
        elif cn == "LoreNoteActor":
            lore += 1
        elif cn == "ANPCCharacter":
            npc += 1
        elif cn == "LockedGateActor":
            gate += 1
        elif isinstance(a, unreal.StaticMeshActor):
            mesh += 1
    links[nmap] = out
    counts[nmap] = (mesh, lore, npc, gate)
    log("  %-26s 부재%4d · 문%d · 조사%d · NPC%d · 잠긴문%d"
        % (nmap, mesh, len(out), lore, npc, gate))

# ── 끊긴 문 / 한쪽으로만 난 문 ────────────────────────────────────────
dangling, oneway = [], []
for src, outs in links.items():
    for (tgt, lbl, req, forb) in outs:
        if tgt not in names:
            dangling.append((src, tgt, lbl))
            continue
        back = any(t == src for (t, _l, _r, _f) in links.get(tgt, []))
        if not back:
            oneway.append((src, tgt, lbl))

log("")
log("끊긴 문(목표 맵 없음): %d건" % len(dangling))
for (s, t, l) in dangling:
    log("   !! %s → %s  (%s)" % (s, t, l))
log("한쪽으로만 난 문(돌아오는 문 없음): %d건" % len(oneway))
for (s, t, l) in oneway:
    log("   · %s → %s  (%s)" % (s, t, l))

# ── 사람이 읽는 표 ────────────────────────────────────────────────────
md = ["# 🚪 라셀 레벨 연결표 — 어느 문이 어디로",
      "",
      "> **이 파일은 손으로 적지 않는다.** `_scan_links.py`가 실제 맵의 포탈을 긁어 다시 쓴다.",
      "> 말로 적은 연결표는 금세 실제와 어긋나므로, 맵이 곧 정본이다.",
      "> 갱신: `UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=\"C:/Secret_Project/도구/진단/_scan_links.py\"`",
      "",
      "## 지은 맵과 무게", "",
      "| 맵 | 부재 | 문 | 조사 | NPC | 잠긴문 |", "|---|---:|---:|---:|---:|---:|"]
for n in names:
    mesh, lore, npc, gate = counts.get(n, (0, 0, 0, 0))
    md.append("| `%s` | %d | %d | %d | %d | %d |"
              % (n, mesh, len(links.get(n, [])), lore, npc, gate))

md += ["", "## 문이 나가는 곳", ""]
for n in names:
    outs = links.get(n, [])
    if not outs:
        md.append("- **`%s`** — 나가는 문 없음" % n)
        continue
    md.append("- **`%s`**" % n)
    for (tgt, lbl, req, forb) in outs:
        cond = ""
        if req:
            cond += " *(플래그 `%s` 선 뒤에만)*" % req
        if forb:
            cond += " *(플래그 `%s` 서기 전까지만)*" % forb
        mark = "" if tgt in names else "  ⚠️**목표 맵 없음**"
        md.append("  - %s → `%s`%s%s" % (lbl, tgt, cond, mark))

# ── 정본 대조 — 지은 것 / 아직 안 지은 것 / 옛 로스터 잔재 ─────────────
built_tails = set()
for n in names:
    for t in canon_tails:
        if n.endswith(t):
            built_tails.add(t)
md += ["", "## 정본 로스터 대조 (기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md)", ""]
md += ["| # | 정본 Id 꼬리 | 지어짐? |", "|---|---|---|"]
for (num, tail) in CANON_IDS:
    done = "✅" if tail in built_tails else "☐ 아직"
    # 이번 루프에서 조합식으로 새로 지은 것 표시
    made = "  ⟵ 조합식" if tail in ("Jangteo_Street", "Antique_Shop", "Backalley",
                                    "Dolgan_Office", "Eatery") else ""
    md.append("| %s | `%s` | %s%s |" % (num, tail, done, made))

# ★번호 어긋남 — 파일 번호는 07-20 옛 로스터 시절, 정본은 07-22 재편본
mismatch = []
for (num, tail) in CANON_IDS:
    for n in names:
        if n.endswith(tail):
            file_num = n.split("_", 1)[0]
            if file_num != num:
                mismatch.append((tail, num, file_num))
            break
md += ["", "**★번호 어긋남 — 파일 번호(07-20 옛 로스터)와 정본 번호(07-22 재편) 불일치:**"]
if mismatch:
    for (tail, cn, fn) in mismatch:
        md.append("- `%s`: 정본 **%s** ↔ 파일 **%s** (배선 깨짐 방지 위해 개명 보류)" % (tail, cn, fn))
else:
    md.append("- 없음")

leftover = []
for n in names:
    if not any(n.endswith(t) for t in canon_tails):
        leftover.append(n)
md += ["", "**정본에 없는 맵 파일(옛 로스터 잔재 or 상태 변형):**"]
if leftover:
    for n in leftover:
        note = ""
        if n == "L02_Antique_Shop_Burnt":
            note = " — L02 사건 뒤 얼굴(정상, 상태 변형)"
        md.append("- `%s`%s" % (n, note))
else:
    md.append("- 없음")

md += ["", "## 손봐야 할 것", ""]
if dangling:
    md.append("**끊긴 문 — 목표 맵이 아직 없다(%d):**" % len(dangling))
    for (s, t, l) in dangling:
        md.append("- `%s` 의 «%s» → `%s`" % (s, l, t))
else:
    md.append("- 끊긴 문 없음")
md.append("")
if oneway:
    md.append("**한쪽으로만 난 문 — 돌아오는 문이 없다(%d):**" % len(oneway))
    for (s, t, l) in oneway:
        md.append("- `%s` → `%s` (%s)" % (s, t, l))
else:
    md.append("- 한쪽으로만 난 문 없음")

with open(OUT_MD, "w", encoding="utf-8") as f:
    f.write("\n".join(md) + "\n")
log("")
log("표 저장: %s" % OUT_MD)


# ── 검수 대시보드 HTML (사용자가 웹에서 한눈에) ────────────────────────
CANON_TAILS_SET = set(t for (_n, t) in CANON_IDS)
BUILT_MODULAR = {"L01_Jangteo_Street", "L02_Antique_Shop", "L02_Antique_Shop_Burnt",
                 "L03_Backalley", "L04_Dolgan_Office", "L05_Eatery"}
# 조합식 6맵의 좌표(SVG 허브 그래프) — L01 중심
NODE_POS = {
    "L01_Jangteo_Street": (430, 250, "장터 큰길", "거리·기준기"),
    "L02_Antique_Shop": (200, 110, "골동상(낮)", "감정 가게"),
    "L02_Antique_Shop_Burnt": (200, 390, "골동상(밤)", "사건 뒤"),
    "L03_Backalley": (680, 110, "뒷골목", "진·젖은 돌"),
    "L04_Dolgan_Office": (680, 390, "흥신소", "이층·수사"),
    "L05_Eatery": (430, 60, "밥집", "밀담 구석"),
}


def esc(s):
    return (s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def build_html():
    # 조합식 맵 사이 연결선(양쪽 다 조합식 맵일 때만 그린다)
    edges = []
    for src, outs in links.items():
        if src not in NODE_POS:
            continue
        for (tgt, lbl, req, forb) in outs:
            t2 = tgt
            if src == "L01_Jangteo_Street" and tgt == "L02_Antique_Shop" and "탄" in lbl:
                t2 = "L02_Antique_Shop_Burnt"
            if t2 in NODE_POS and (src, t2) not in [(e[0], e[1]) for e in edges]:
                edges.append((src, t2, lbl))

    svg = ['<svg viewBox="0 0 880 470" width="100%" style="max-width:880px">']
    svg.append('<defs><marker id="ar" markerWidth="9" markerHeight="9" refX="7" refY="3" '
               'orient="auto"><path d="M0,0 L7,3 L0,6 Z" fill="#c9a06a"/></marker></defs>')
    for (s, t, lbl) in edges:
        x1, y1 = NODE_POS[s][0], NODE_POS[s][1]
        x2, y2 = NODE_POS[t][0], NODE_POS[t][1]
        svg.append('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="#6b5a3e" '
                   'stroke-width="2" marker-end="url(#ar)"/>' % (x1, y1, x2, y2))
    for nm2, (x, y, ttl, sub) in NODE_POS.items():
        mesh, lore, npc, gate = counts.get(nm2, (0, 0, 0, 0))
        big = (nm2 == "L01_Jangteo_Street")
        r = 62 if big else 52
        fill = "#3a2f22" if big else "#2b241a"
        svg.append('<g>')
        svg.append('<rect x="%d" y="%d" width="%d" height="%d" rx="10" fill="%s" '
                   'stroke="#c9a06a" stroke-width="%d"/>'
                   % (x - r, y - 34, r * 2, 68, fill, 2 if big else 1))
        svg.append('<text x="%d" y="%d" text-anchor="middle" fill="#f0e2c8" '
                   'font-size="14" font-weight="700">%s</text>' % (x, y - 12, esc(ttl)))
        svg.append('<text x="%d" y="%d" text-anchor="middle" fill="#b9a986" '
                   'font-size="10">%s</text>' % (x, y + 3, esc(sub)))
        svg.append('<text x="%d" y="%d" text-anchor="middle" fill="#8f7f5e" '
                   'font-size="9">부재 %d · 문 %d</text>'
                   % (x, y + 19, mesh, len(links.get(nm2, []))))
        svg.append('</g>')
    svg.append('</svg>')

    built = sum(1 for (_n, t) in CANON_IDS if any(n.endswith(t) for n in names))
    total_mesh = sum(counts.get(n, (0,))[0] for n in BUILT_MODULAR)

    rows_canon = []
    for (num, tail) in CANON_IDS:
        done = any(n.endswith(tail) for n in names)
        mod = tail in ("Jangteo_Street", "Antique_Shop", "Backalley",
                       "Dolgan_Office", "Eatery")
        badge = ('<span class="ok">지어짐</span>' if done else '<span class="todo">아직</span>')
        if mod:
            badge = '<span class="mod">조합식 ✓</span>'
        rows_canon.append("<tr><td>%s</td><td><code>%s</code></td><td>%s</td></tr>"
                          % (num, esc(tail), badge))

    rows_built = []
    for n in sorted(BUILT_MODULAR):
        mesh, lore, npc, gate = counts.get(n, (0, 0, 0, 0))
        rows_built.append("<tr><td><code>%s</code></td><td>%d</td><td>%d</td>"
                          "<td>%d</td><td>%d</td></tr>"
                          % (esc(n), mesh, len(links.get(n, [])), lore, npc))

    html = """<!doctype html><html lang="ko"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>라셀 레벨 검수판</title><style>
:root{color-scheme:dark}
body{margin:0;background:#15110c;color:#e8dcc4;font-family:'Malgun Gothic',system-ui,sans-serif;
line-height:1.6}
.wrap{max-width:900px;margin:0 auto;padding:28px 20px 80px}
h1{font-size:24px;margin:0 0 4px;color:#f3e6c8}
h2{font-size:17px;margin:34px 0 10px;color:#d8b87e;border-bottom:1px solid #3a3020;padding-bottom:6px}
.sub{color:#9c8c68;font-size:13px;margin:0 0 20px}
.graph{background:#1c160f;border:1px solid #2f2718;border-radius:12px;padding:16px;text-align:center}
table{width:100%;border-collapse:collapse;font-size:13px;margin-top:6px}
th,td{text-align:left;padding:6px 10px;border-bottom:1px solid #2a2216}
th{color:#c9a06a;font-weight:600}
td code{color:#e8c98a;font-size:12px}
.ok{color:#8fbf7f}.todo{color:#8a7d5e}.mod{color:#e0b060;font-weight:700}
.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px;margin:6px 0}
.card{background:#1c160f;border:1px solid #2f2718;border-radius:10px;padding:12px 14px}
.card .n{font-size:22px;font-weight:800;color:#f0d69a}
.card .l{font-size:12px;color:#9c8c68}
.decide{background:#241a10;border:1px solid #6b4f22;border-radius:12px;padding:16px 18px;margin-top:12px}
.decide b{color:#f0c878}
.warn{color:#c98a5a}
ul{margin:6px 0;padding-left:20px}li{margin:2px 0}
.foot{color:#6f6248;font-size:11px;margin-top:40px;border-top:1px solid #2a2216;padding-top:12px}
</style></head><body><div class="wrap">
<h1>🏮 라셀 레벨 검수판</h1>
<p class="sub">자동 생성 — 실제 맵을 긁어 다시 씀 (<code>_scan_links.py</code>). 맵이 곧 정본.</p>

<div class="cards">
<div class="card"><div class="n">@@NMOD@@</div><div class="l">조합식으로 지은 맵</div></div>
<div class="card"><div class="n">@@BUILT@@ / 22</div><div class="l">정본 로스터 중 존재</div></div>
<div class="card"><div class="n">@@TOTMESH@@</div><div class="l">조합식 6맵 부재 합</div></div>
<div class="card"><div class="n">75</div><div class="l">우리 저작 부재 종수</div></div>
</div>

<h2>조합식으로 지은 핵 — 서로 어떻게 이어지나</h2>
<div class="graph">@@SVG@@</div>
<p class="sub">L01 큰길을 허브로 L02·L03·L05가 걸리고, L03·L04가 실내로 이어진다.
골동상은 사건 전/후 두 얼굴(StoryFlag <code>op_nesa_dead</code>로 갈림).</p>

<h2>조합식 6맵의 무게</h2>
<table><tr><th>맵</th><th>부재</th><th>문</th><th>조사</th><th>NPC</th></tr>
@@ROWS_BUILT@@</table>

<h2>정본 로스터 대조 (기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md)</h2>
<table><tr><th>#</th><th>정본 Id 꼬리</th><th>상태</th></tr>
@@ROWS_CANON@@</table>

<div class="decide">
<b>★결정 대기 — 지도 재편 방향</b><br>
파일 번호가 두 시대(07-20 옛 로스터 / 07-22 정본 재편)로 섞여 어긋나 있습니다.
다음 중 방향을 정해 주세요:
<ul>
<li><b>㉠ 개명</b> — 파일 번호를 정본에 맞춰 일괄 개명 (배선 재작성 동반)</li>
<li><b>㉡ 새로</b> — 옛 잔재 은퇴 후 정본 L06~L22를 조합식으로 새로 (지금 6맵처럼)</li>
</ul>
<span class="warn">옛 로스터 잔재 맵 10개가 아직 살아 있어 번호 어긋남·유령 문의 원인.
파괴적 작업이라 사용자 확인 전까지 손대지 않습니다.</span>
</div>

<h2>검수 시 알아둘 헤드리스 한계</h2>
<ul>
<li>텍스처·실내 톤·반사(웅덩이·유리)는 커맨들릿 촬영에서 안 보임 → <b>에디터에서 직접</b> 봐야 함</li>
<li>지오메트리·배치·조명·실루엣은 스크린샷으로 검증됨 (검사기 뜸/박힘/막음 전부 0)</li>
</ul>

<p class="foot">이 페이지는 @@MAPCOUNT@@개 맵을 스캔해 만들었습니다 · 라셀 현대편</p>
</div></body></html>"""
    for tok, val in (("@@NMOD@@", str(len(BUILT_MODULAR))), ("@@BUILT@@", str(built)),
                     ("@@TOTMESH@@", str(total_mesh)), ("@@SVG@@", "\n".join(svg)),
                     ("@@ROWS_BUILT@@", "\n".join(rows_built)),
                     ("@@ROWS_CANON@@", "\n".join(rows_canon)),
                     ("@@MAPCOUNT@@", str(len(names)))):
        html = html.replace(tok, val)

    with open("C:/Secret_Project/웹/게임_레벨_검수판.html", "w", encoding="utf-8") as f:
        f.write(html)
    log("검수판 HTML 저장: 웹/게임_레벨_검수판.html")


try:
    build_html()
except Exception as e:
    import traceback
    log("HTML 실패\n" + traceback.format_exc())
with open("C:/Secret_Project/Saved/links.log", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
