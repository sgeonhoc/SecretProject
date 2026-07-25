# -*- coding: utf-8 -*-
u"""라셀 도시 뼈대 → 언리얼  (_build_rasel_city_ue.py)

★원칙
  · 라셀은 **끊기지 않은 하나의 공간**이다. 맵을 구역별로 쪼개 짓지 않는다.
    맵 하나(/Game/Maps/Rasel/Rasel_City)에 도시 전체를 얹는다. 스트리밍은 나중 일이고,
    그때도 자리 좌표는 안 바뀐다.
  · **뼈대만.** 매스(덩어리)·가로·물길·다리·자리표까지. 창틀·간판·소품은 아직 없다.
  · 원본은 하나 — `_build_rasel_plan.js`가 뱉은 `_rasel_plan.json`.
    웹 도면(세계관_라셀_도시뼈대.html)과 이 레벨이 **같은 데이터**에서 나온다.
  · 에셋 규율: `/Engine/BasicShapes` + 우리가 저작한 `/Game/Rasel/*` 뿐. 외부 팩 0.
  · 낱개 액터로 놓는다(통짜 금지) — 아웃라이너 폴더로 갈라 두어 골라 지우고 고칠 수 있게.

단위: 도면 1 m = UE 100 유닛. 원점(0,0) = 장터 큰길 한복판. +X 동(내륙·상류) / +Y 북.

실행:
  UnrealEditor-Cmd.exe C:/Secret_Project/Secret_Project.uproject ^
    -ExecutePythonScript="C:/Secret_Project/_build_rasel_city_ue.py" -unattended -nosplash
"""
import unreal, json, math, os, sys

sys.path.append("C:/Secret_Project")
import _rasel_common as C

M = 100.0                                    # 1 m
MAP = "/Game/Maps/Rasel/Rasel_City"
MATDIR = "/Game/Rasel/Materials/Skeleton"
PLAN = "C:/Secret_Project/_rasel_plan.json"

EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
CUBE = EAL.load_asset("/Engine/BasicShapes/Cube")
CYL = EAL.load_asset("/Engine/BasicShapes/Cylinder")

_log = []


def log(m):
    _log.append(str(m))
    unreal.log("[rasel-city] " + str(m))


# ── 결정적 난수 (건물 높이를 흩되 매번 같게) ─────────────────────────────
_s = [20260725]


def rnd():
    _s[0] = (_s[0] * 1664525 + 1013904223) % 4294967296
    return _s[0] / 4294967296.0


# ── 머티리얼 (우리 저작·단색) ────────────────────────────────────────────
MATS = {}


def flat(name, rgb, rough=0.75, metal=0.0, spec=0.4, emissive=None):
    path = "%s/%s" % (MATDIR, name)
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    m = AT.create_asset(name, MATDIR, unreal.Material, unreal.MaterialFactoryNew())
    c = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 0)
    c.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 200)
    r.set_editor_property("r", rough)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if metal > 0:
        mm = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 300)
        mm.set_editor_property("r", metal)
        MEL.connect_material_property(mm, "", unreal.MaterialProperty.MP_METALLIC)
    sp = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 360)
    sp.set_editor_property("r", spec)
    MEL.connect_material_property(sp, "", unreal.MaterialProperty.MP_SPECULAR)
    if emissive:
        e = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 440)
        e.set_editor_property("constant", unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0))
        MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    try:
        MEL.recompile_material(m)
    except Exception:
        pass
    EAL.save_asset(path, False)
    MATS[name] = m
    return m


def build_materials():
    # 구역이 색으로 갈려야 뼈대가 읽힌다. 단색이되 결끼리 확실히 다르게.
    flat("M_Sk_Land", (0.072, 0.068, 0.044))          # 개발 안 된 삼각주 땅
    flat("M_Sk_Water", (0.018, 0.052, 0.072), rough=0.09, spec=0.95)
    flat("M_Sk_Sea", (0.012, 0.038, 0.056), rough=0.07, spec=0.95)
    flat("M_Sk_Dry", (0.190, 0.152, 0.082))           # 마른 골
    flat("M_Sk_Road", (0.115, 0.106, 0.080))          # 큰 가로
    flat("M_Sk_Lane", (0.088, 0.080, 0.058))          # 골목
    flat("M_Sk_Bridge", (0.420, 0.290, 0.130))
    flat("M_Sk_Old", (0.300, 0.235, 0.145))           # 구시가 — 따뜻한 흙빛
    flat("M_Sk_Grid", (0.330, 0.315, 0.255))          # 격자 시가 — 밝은 돌빛
    flat("M_Sk_Super", (0.150, 0.245, 0.320), rough=0.22, metal=0.3, spec=0.85)  # 유리탑
    flat("M_Sk_Indus", (0.335, 0.190, 0.120))         # 신항 — 녹빛
    flat("M_Sk_Quay", (0.150, 0.235, 0.235))          # 항만 — 찬 청록
    flat("M_Sk_Ruin", (0.215, 0.195, 0.155))          # 폐허 — 바랜 회갈
    flat("M_Sk_Square", (0.245, 0.195, 0.120))        # 광장
    flat("M_Sk_Park", (0.090, 0.175, 0.070))          # 공원
    flat("M_Sk_Mark", (0.90, 0.70, 0.25), emissive=(0.55, 0.38, 0.10))   # 자리표
    flat("M_Sk_MarkU", (0.55, 0.40, 0.80), emissive=(0.28, 0.16, 0.45))  # 아래층 자리표
    flat("M_Sk_Station", (0.65, 0.45, 0.30), emissive=(0.30, 0.18, 0.10))
    flat("M_Sk_Tunnel", (0.105, 0.098, 0.092))        # 지하철 터널
    flat("M_Sk_Under", (0.140, 0.120, 0.135))         # 아래층 회랑 (옛 켜)
    flat("M_Sk_UnderWater", (0.030, 0.058, 0.062), rough=0.12, spec=0.9)  # 물 찬 승강장
    flat("M_Sk_Wall", (0.200, 0.185, 0.155))          # 옛 도읍 성벽
    flat("M_Sk_Plot", (0.520, 0.400, 0.150), emissive=(0.18, 0.13, 0.03))  # 레벨 대지
    log("머티리얼 %d종" % len(MATS))


# ── 놓기 ────────────────────────────────────────────────────────────────
A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
_n = [0]


def box(cx, cy, cz, sx, sy, sz, yaw, matname, label, folder):
    """cx..cz = 중심(m) · sx..sz = 크기(m) · yaw = 도"""
    a = A.spawn_actor_from_object(CUBE, unreal.Vector(cx * M, cy * M, cz * M),
                                  unreal.Rotator(0.0, 0.0, yaw))
    if a is None:
        return None
    a.set_actor_scale3d(unreal.Vector(sx * M / 100.0, sy * M / 100.0, sz * M / 100.0))
    mt = MATS.get(matname)
    if mt:
        a.static_mesh_component.set_material(0, mt)
    a.set_actor_label(label)
    a.set_folder_path(folder)
    _n[0] += 1
    return a


def pillar(cx, cy, r, h, matname, label, folder, z0=0.0):
    a = A.spawn_actor_from_object(CYL, unreal.Vector(cx * M, cy * M, (z0 + h / 2.0) * M))
    if a is None:
        return None
    a.set_actor_scale3d(unreal.Vector(r * 2 * M / 100.0, r * 2 * M / 100.0, h * M / 100.0))
    mt = MATS.get(matname)
    if mt:
        a.static_mesh_component.set_material(0, mt)
    a.set_actor_label(label)
    a.set_folder_path(folder)
    _n[0] += 1
    return a


def ribbon(pts, w0, w1, z, thick, matname, prefix, folder, step=3):
    """중심선을 따라 띠를 깐다 (물길·가로·마른 골)."""
    k = 0
    i = 0
    n = len(pts)
    while i < n - 1:
        j = min(i + step, n - 1)
        x1, y1 = pts[i]
        x2, y2 = pts[j]
        dx, dy = x2 - x1, y2 - y1
        L = math.hypot(dx, dy)
        if L < 0.5:
            i = j
            continue
        t = (i + j) / 2.0 / max(1.0, n - 1.0)
        w = w0 + (w1 - w0) * t
        box((x1 + x2) / 2.0, (y1 + y2) / 2.0, z,
            L + w * 0.35, w, thick,
            math.degrees(math.atan2(dy, dx)), matname,
            "%s_%02d" % (prefix, k), folder)
        k += 1
        i = j
    return k


def quad_box(poly, h, matname, label, folder):
    """네 점 필지 → 방향 있는 매스."""
    (x0, y0), (x1, y1), (x2, y2), (x3, y3) = poly[0], poly[1], poly[2], poly[3]
    ex, ey = x1 - x0, y1 - y0
    fx, fy = x3 - x0, y3 - y0
    la = (math.hypot(ex, ey) + math.hypot(x2 - x3, y2 - y3)) / 2.0
    lb = (math.hypot(fx, fy) + math.hypot(x2 - x1, y2 - y1)) / 2.0
    cx = (x0 + x1 + x2 + x3) / 4.0
    cy = (y0 + y1 + y2 + y3) / 4.0
    if la < 3 or lb < 3:
        return None
    return box(cx, cy, h / 2.0, la, lb, h,
               math.degrees(math.atan2(ey, ex)), matname, label, folder)


def text(cx, cy, cz, s, size=260.0, color=(0.95, 0.85, 0.62), label=None):
    a = A.spawn_actor_from_class(unreal.TextRenderActor,
                                 unreal.Vector(cx * M, cy * M, cz * M),
                                 unreal.Rotator(0.0, 90.0, 0.0))
    c = a.text_render
    c.set_editor_property("text", unreal.Text(s))
    c.set_editor_property("world_size", size)
    c.set_editor_property("text_render_color", unreal.Color(
        int(color[0] * 255), int(color[1] * 255), int(color[2] * 255), 255))
    c.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)
    a.set_actor_label(label or ("글 " + s))
    a.set_folder_path("라셀/자리표/이름")
    _n[0] += 1
    return a


# ── 본 작업 ─────────────────────────────────────────────────────────────
def main():
    with open(PLAN, "r", encoding="utf-8") as f:
        P = json.load(f)
    log("도면 읽음 — 블록 %d · 가로 %d · 물길 %d · 자리 %d"
        % (len(P["blocks"]), len(P["streets"]), len(P["channels"]), len(P["levels"])))

    C.begin(MAP, "rasel_city")
    build_materials()

    # 1) 땅 — 큰 판 하나가 아니라 타일로 깔아 골라 고칠 수 있게
    TX0, TX1, TY0, TY1 = -7600, 7600, -5200, 5200
    tw, th = 1900, 1300
    nx = int((TX1 - TX0) / tw)
    ny = int((TY1 - TY0) / th)
    for i in range(nx):
        for j in range(ny):
            box(TX0 + tw * (i + 0.5), TY0 + th * (j + 0.5), -0.5, tw, th, 1.0,
                0.0, "M_Sk_Land", "땅_%02d_%02d" % (i, j), "라셀/땅")
    log("땅 타일 %d" % (nx * ny))

    # 2) 바다 — 해안선을 따라 서쪽으로
    co = P["coast"]
    for i in range(0, len(co) - 1, 2):
        x1, y1 = co[i]
        x2, y2 = co[min(i + 2, len(co) - 1)]
        ymid = (y1 + y2) / 2.0
        hy = abs(y2 - y1) + 40
        xmid = (x1 + x2) / 2.0
        box((xmid + TX0 - 400) / 2.0, ymid, -0.2, abs(xmid - (TX0 - 400)), hy, 1.4,
            0.0, "M_Sk_Sea", "바다_%02d" % i, "라셀/물길/바다")

    # 3) 물길
    for ch in P["channels"]:
        k = ribbon(ch["pts"], ch["w0"], ch["w1"], -0.2, 1.6, "M_Sk_Water",
                   ch["name"], "라셀/물길/" + ch["name"], step=3)
        log("  %s — 띠 %d" % (ch["name"], k))
    d = P["dry"]
    ribbon(d["pts"], d["w0"], d["w1"], 0.05, 0.6, "M_Sk_Dry", "마른골",
           "라셀/물길/마른 골", step=3)

    # 4) 가로 — 큰 길과 골목
    for st in P["streets"]:
        nm = st["name"] or ("길_" + st["cls"])
        ribbon(st["pts"], st["w"], st["w"], 0.16, 0.35, "M_Sk_Road", nm,
               "라셀/가로/" + nm, step=4)
    for i, ln in enumerate(P["lanes"]):
        ribbon(ln["pts"], ln["w"], ln["w"], 0.14, 0.30, "M_Sk_Lane",
               "골목%02d" % i, "라셀/가로/골목", step=3)
    log("가로까지 액터 %d" % _n[0])

    # 5) 광장·공원
    for v in P["voids"]:
        mt = "M_Sk_Park" if v["kind"] == "park" else "M_Sk_Square"
        pillar(v["x"], v["y"], v["r"] * 0.92, 0.25, mt, v["name"], "라셀/광장", z0=0.05)

    # 6) 블록 매스 — 구역 조직마다 높이 결이 다르다
    HGT = {"organic": (9, 18), "grid": (13, 26), "super": (40, 130),
           "industrial": (7, 17), "quay": (9, 16), "ruin": (2.5, 7.5)}
    MT = {"organic": "M_Sk_Old", "grid": "M_Sk_Grid", "super": "M_Sk_Super",
          "industrial": "M_Sk_Indus", "quay": "M_Sk_Quay", "ruin": "M_Sk_Ruin"}
    FOLD = {"organic": "라셀/블록/구시가", "grid": "라셀/블록/격자 시가",
            "super": "라셀/블록/위층 유리탑", "industrial": "라셀/블록/신항",
            "quay": "라셀/블록/부두", "ruin": "라셀/블록/폐허"}
    cnt = {}
    for i, b in enumerate(P["blocks"]):
        k = b["k"]
        lo, hi = HGT.get(k, (10, 20))
        cnt[k] = cnt.get(k, 0) + 1
        if k == "super":
            # 위층 업무지구는 초대형 필지를 통째로 채우지 않는다 —
            # 넓은 대지 위에 탑 두엇이 서고 나머지는 앞마당이다(그래야 스카이라인이 선다).
            (x0, y0), (x1, y1), _, (x3, y3) = b["p"][0], b["p"][1], b["p"][2], b["p"][3]
            ex, ey = x1 - x0, y1 - y0
            fx, fy = x3 - x0, y3 - y0
            la = math.hypot(ex, ey)
            lb = math.hypot(fx, fy)
            ux, uy = ex / la, ey / la
            vx, vy = fx / lb, fy / lb
            cx = (x0 + x1 + b["p"][2][0] + x3) / 4.0
            cy = (y0 + y1 + b["p"][2][1] + y3) / 4.0
            yaw = math.degrees(math.atan2(ey, ex))
            for t in range(2):
                du = (t - 0.5) * la * 0.40 + (rnd() - 0.5) * la * 0.10
                dv = (rnd() - 0.5) * lb * 0.24
                th = 55.0 + rnd() * 115.0
                box(cx + ux * du + vx * dv, cy + uy * du + vy * dv, th / 2.0,
                    la * (0.22 + rnd() * 0.12), lb * (0.34 + rnd() * 0.18), th,
                    yaw, "M_Sk_Super", "tower_%04d_%d" % (cnt[k], t), FOLD[k])
            # 대지(포디움) — 낮게 깔아 필지를 보이게
            quad_box(b["p"], 5.0, "M_Sk_Super", "podium_%04d" % cnt[k], FOLD[k])
            continue
        # ★블록 하나를 통짜 슬래브로 세우지 않는다 — 긴 쪽을 필지로 갈라
        #   제각각 높이로 세워야 도시의 결(들쭉날쭉한 처마선)이 산다.
        (x0, y0), (x1, y1), (x2, y2), (x3, y3) = b["p"]
        ex, ey = x1 - x0, y1 - y0
        la = math.hypot(ex, ey)
        lb = math.hypot(x3 - x0, y3 - y0)
        npar = max(1, min(6, int(round(la / 34.0))))
        base = lo + rnd() * (hi - lo)
        if npar <= 1:
            quad_box(b["p"], base, MT.get(k, "M_Sk_Grid"),
                     "%s_%04d" % (k, cnt[k]), FOLD.get(k, "라셀/블록"))
        else:
            for t in range(npar):
                a0 = t / float(npar)
                a1 = (t + 1) / float(npar)
                q = [[x0 + (x1 - x0) * a0, y0 + (y1 - y0) * a0],
                     [x0 + (x1 - x0) * a1, y0 + (y1 - y0) * a1],
                     [x3 + (x2 - x3) * a1, y3 + (y2 - y3) * a1],
                     [x3 + (x2 - x3) * a0, y3 + (y2 - y3) * a0]]
                q = [[q[i][0] * 0.97 + (sum(p[0] for p in q) / 4.0) * 0.03,
                      q[i][1] * 0.97 + (sum(p[1] for p in q) / 4.0) * 0.03] for i in range(4)]
                h = base * (0.72 + rnd() * 0.62)
                quad_box(q, h, MT.get(k, "M_Sk_Grid"),
                         "%s_%04d_%d" % (k, cnt[k], t), FOLD.get(k, "라셀/블록"))
    log("블록 매스 — " + " · ".join("%s %d" % (a, b) for a, b in sorted(cnt.items())))

    # 7) 다리
    for br in P["bridges"]:
        (x1, y1), (x2, y2) = br["pts"]
        L = math.hypot(x2 - x1, y2 - y1)
        box((x1 + x2) / 2.0, (y1 + y2) / 2.0, 8.0, L, 30.0, 1.6,
            math.degrees(math.atan2(y2 - y1, x2 - x1)), "M_Sk_Bridge",
            br["name"], "라셀/다리")
        for t in (0.0, 1.0):
            box(x1 + (x2 - x1) * t, y1 + (y2 - y1) * t, 4.0, 34.0, 34.0, 8.0,
                0.0, "M_Sk_Bridge", br["name"] + "_교대", "라셀/다리")

    # 7-2) 부두 손가락 잔교 — 항만은 이게 있어야 항만으로 읽힌다
    for i in range(5):
        y = 560 - i * 260
        box(-4470, y - 20, 1.2, 560, 34, 2.4, 4.0, "M_Sk_Quay",
            "잔교_%d" % (i + 1), "라셀/블록/부두")
    box(-4180, 60, 1.4, 40, 1450, 2.8, 0.0, "M_Sk_Quay", "안벽", "라셀/블록/부두")

    # 7-3) 구역 이름표 — 하늘에 띄워 어디가 어딘지 바로 보이게
    DNAME = {"A": (-280, 480, "아랫장터 구시가"), "B": (1650, 620, "학당가·의원권"),
             "W": (-2300, 1000, "서편 셋집 구역"), "Q": (-3750, 900, "부두 — 바다 목"),
             "D": (1300, 3150, "강 건너 유리탑 (위층)"), "C": (-600, -2250, "신항 재개발지"),
             "E2": (3000, 500, "동편 새 동네"), "F": (6250, 3550, "옛 사르간 도읍 폐허")}
    for k, (x, y, nm) in DNAME.items():
        text(x, y, 150.0, nm, 3000.0, (0.98, 0.88, 0.60), "구역 " + nm)

    # 7-4) 옛 사르간 도읍 — 성벽과 성문과 빈 성 (마른 골 끝의 랜드마크)
    lm = P["landmarks"]
    ribbon(lm["wall"], 26, 26, 5.0, 10.0, "M_Sk_Wall", "성벽",
           "라셀/블록/폐허", step=2)
    g = lm["gate"]
    for sgn in (-1, 1):
        box(g["x"] + sgn * 44, g["y"] + sgn * 18, 9.0, 34, 34, 18.0, 0.0,
            "M_Sk_Wall", "성문 망루%d" % (sgn + 2), "라셀/블록/폐허")
    kp = lm["keep"]
    box(kp["x"], kp["y"], 11.0, kp["w"], kp["h"], 22.0, 12.0, "M_Sk_Wall",
        "빈 성 본채", "라셀/블록/폐허")
    for dx, dy in ((-0.42, -0.42), (0.42, -0.42), (-0.42, 0.42), (0.42, 0.42)):
        box(kp["x"] + kp["w"] * dx, kp["y"] + kp["h"] * dy, 17.0, 30, 30, 34.0, 12.0,
            "M_Sk_Wall", "빈 성 탑", "라셀/블록/폐허")

    # 7-5) 지하철 — 열두 갈래 중 여섯을 실제로 지하에 깐다
    for m in P["metro"]:
        ribbon(m["pts"], 13, 13, -26.0, 7.0, "M_Sk_Tunnel",
               "지하철%d호선" % (m["i"] + 1), "라셀/지하철/터널", step=5)

    # 7-6) ★아래층 — 지표 아래도 같은 공간이다
    for u in P["under"]:
        ribbon(u["pts"], u["w"], u["w"], u["z"], 6.0, "M_Sk_Under",
               u["name"], "라셀/아래층/" + u["name"], step=3)
    pf = P["platform"]
    box(pf["x"], pf["y"], pf["z"], pf["w"], pf["h"], 5.0, -4.0, "M_Sk_Under",
        "폐선 승강장 바닥", "라셀/아래층/폐선 승강장")
    box(pf["x"], pf["y"], pf["z"] + 1.6, pf["w"] - 14, pf["h"] - 8, 0.8, -4.0,
        "M_Sk_UnderWater", "물 찬 선로", "라셀/아래층/폐선 승강장")
    for t in range(6):
        box(pf["x"] - pf["w"] / 2 + 14 + t * 20, pf["y"], pf["z"] + 4.5, 3, 3, 6.0, 0.0,
            "M_Sk_Under", "승강장 기둥%d" % t, "라셀/아래층/폐선 승강장")

    # 묻힌 도시 — 회랑이 격자로 도는 도시 규모의 방
    H = P["hall"]
    nx2 = int(H["w"] / H["cell"][0])
    ny2 = int(H["h"] / H["cell"][1])
    for i in range(nx2 + 1):
        x = H["x"] - H["w"] / 2.0 + i * H["cell"][0]
        box(x, H["y"], H["z"], 12, H["h"], 7.0, 0.0, "M_Sk_Under",
            "회랑 세로%02d" % i, "라셀/아래층/묻힌 도시")
    for j in range(ny2 + 1):
        y = H["y"] - H["h"] / 2.0 + j * H["cell"][1]
        box(H["x"], y, H["z"], H["w"], 12, 7.0, 0.0, "M_Sk_Under",
            "회랑 가로%02d" % j, "라셀/아래층/묻힌 도시")
    for i in range(nx2):
        for j in range(ny2):
            box(H["x"] - H["w"] / 2.0 + (i + 0.5) * H["cell"][0],
                H["y"] - H["h"] / 2.0 + (j + 0.5) * H["cell"][1],
                H["z"] + 5.0, 5, 5, 10.0, 0.0, "M_Sk_Under",
                "무너진 기둥 %d-%d" % (i, j), "라셀/아래층/묻힌 도시")

    # 7-7) 레벨 대지 — 어느 자리에 무엇이 앉는지 바닥에 표시
    PLOT = {"L01": (280, 26), "L02": (26, 20), "L03": (60, 12), "L04": (24, 18),
            "L05": (22, 16), "L22": (18, 14), "L06": (220, 24), "L07": (180, 40),
            "L08": (70, 50), "L11": (240, 120), "L12": (110, 70), "L09": (300, 180),
            "L13": (150, 110), "L14": (120, 90), "L15": (140, 100),
            "L18": (120, 90), "L19": (190, 150), "L20": (90, 22), "L10": (60, 60),
            "L16": (130, 26), "L17": (720, 440), "L21": (24, 18)}
    for L in P["levels"]:
        w, h = PLOT.get(L["id"], (30, 24))
        if L["z"] < 0:
            continue
        box(L["x"], L["y"], L["z"] + 0.35, w, h, 0.5, 0.0, "M_Sk_Plot",
            "대지 %s %s" % (L["id"], L["n"]), "라셀/자리표/대지")

    # 8) 지하철역 자리표
    for s in P["stations"]:
        pillar(s["x"], s["y"], 7.0, 9.0, "M_Sk_Station", "역 " + s["n"], "라셀/지하철역")
        text(s["x"], s["y"], 12.0, s["n"], 200.0, (0.78, 0.60, 0.40))

    # 9) 레벨 자리표 — 어디서든 보이는 기둥 + 이름
    GC = {"E": "M_Sk_MarkU"}
    for L in P["levels"]:
        if not isinstance(L.get("x"), (int, float)):
            continue
        under = L["z"] < 0
        mt = "M_Sk_MarkU" if under else "M_Sk_Mark"
        h = 30.0 if under else 46.0
        pillar(L["x"], L["y"], 2.0, h, mt, "%s %s" % (L["id"], L["n"]),
               "라셀/자리표/" + ("아래층" if under else "지상"), z0=L["z"])
        text(L["x"], L["y"], L["z"] + h + 6.0, "%s %s" % (L["id"], L["n"]),
             300.0, (0.72, 0.55, 0.85) if under else (0.95, 0.85, 0.62))
    log("자리표 %d" % len(P["levels"]))

    # 10) 조명 — 뼈대는 "읽히는 것"이 먼저다.
    #     해 하나만 둔다(보조광 없음 — 방향광 둘이면 경고가 뜨고 그림자가 흐려진다).
    #     자동노출을 끄고 고정 노출로 못박아, 큰 하늘 때문에 도시가 하얗게 날아가지 않게.
    dl = A.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 30000))
    dl.set_actor_rotation(unreal.Rotator(0.0, -52.0, -35.0), False)
    lc = dl.get_component_by_class(unreal.DirectionalLightComponent)
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_intensity(4.0)
    lc.set_light_color(unreal.LinearColor(1.0, 0.96, 0.90))
    lc.set_editor_property("atmosphere_sun_light", True)
    lc.set_editor_property("dynamic_shadow_distance_movable_light", 400000.0)
    dl.set_actor_label("해")
    dl.set_folder_path("라셀/하늘")

    sa = A.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    sa.set_folder_path("라셀/하늘")
    sky = A.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 60000))
    skc = sky.get_component_by_class(unreal.SkyLightComponent)
    skc.set_mobility(unreal.ComponentMobility.MOVABLE)
    skc.set_editor_property("real_time_capture", True)
    skc.set_editor_property("intensity", 1.15)
    sky.set_actor_label("하늘빛")
    sky.set_folder_path("라셀/하늘")

    fog = A.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 200))
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property("fog_density", 0.0016)
    fc.set_editor_property("fog_height_falloff", 0.06)
    fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.30, 0.38, 0.46))
    fog.set_actor_label("대기 안개")
    fog.set_folder_path("라셀/하늘")

    ppv = A.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 400))
    ppv.set_editor_property("unbound", True)
    s = ppv.get_editor_property("settings")
    s.set_editor_property("override_auto_exposure_method", True)
    # ★노출: 하늘이 화면의 절반인 부감에서도 도시가 안 날아가게 히스토그램 + 클램프.
    #   (Manual은 고도마다 어긋난다 — 뼈대는 부감과 거리 눈높이를 둘 다 봐야 한다.)
    s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    s.set_editor_property("override_auto_exposure_bias", True)
    s.set_editor_property("auto_exposure_bias", -0.7)
    s.set_editor_property("override_auto_exposure_min_brightness", True)
    s.set_editor_property("auto_exposure_min_brightness", 0.35)
    s.set_editor_property("override_auto_exposure_max_brightness", True)
    s.set_editor_property("auto_exposure_max_brightness", 4.5)
    s.set_editor_property("override_color_saturation", True)
    s.set_editor_property("color_saturation", unreal.Vector4(1.18, 1.18, 1.18, 1.0))
    ppv.set_editor_property("settings", s)
    ppv.set_actor_label("노출·색")
    ppv.set_folder_path("라셀/하늘")

    ps = A.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, 0.0, 200.0))
    ps.set_actor_label("시작 — 장터 큰길")
    ps.set_folder_path("라셀/하늘")

    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    ok = les.save_current_level()
    log("저장 -> %s · 총 액터 %d" % (ok, _n[0]))
    with open("C:/Secret_Project/Saved/rasel_city_build.log", "w", encoding="utf-8") as f:
        f.write("\n".join(_log))
    if not ok:
        raise RuntimeError("맵 저장 실패")


main()
