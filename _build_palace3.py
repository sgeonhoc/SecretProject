# -*- coding: utf-8 -*-
# ▶ 네르한 궁전 — 실제 바로크 궁전 표준 평면 복제(건축적으로 말 되게). (2026-07-25 v3)
#   사용자: "저택 구조 그대로 배껴서" — 임의 배치 폐기. 대칭 정본 평면:
#   전정(U자·대문·분수) → 대기실·현관 → 중앙홀+대계단(★진짜 2층으로 연결) → 국가실 엔필라드(문축 일직선)
#   → 좌우 대회랑(통층). ★2층 실제 존재·도달 / 천장 완전히 닫힘 / 모든 방 문 일직선.
#   층높이 FH=1800(18m) 2층, 통층 대공간 DH=3600(36m). 실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=_build_palace3.py
import unreal, math, traceback

MAP    = "/Game/Maps/Rasel/L15_Nerhan_Estate"
MATDIR = "/Game/Rasel/Materials"
CUBE = "/Engine/BasicShapes/Cube.Cube"
CYL  = "/Engine/BasicShapes/Cylinder.Cylinder"
LOG  = "C:/Secret_Project/Saved/estate.log"
EAL  = unreal.EditorAssetLibrary
MEL  = unreal.MaterialEditingLibrary
AT   = unreal.AssetToolsHelpers.get_asset_tools()
les  = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
acts = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MATS, _m, CNT = {}, {}, {"n": 0}
_ll = []


def log(s):
    _ll.append(str(s)); unreal.log("[palace3] " + str(s))


def matx(name, rgb, rough=0.85, emissive=None):
    p = MATDIR + "/" + name
    if EAL.does_asset_exist(p):
        EAL.delete_asset(p)
    m = AT.create_asset(name, MATDIR, unreal.Material, unreal.MaterialFactoryNew())
    c = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 0)
    c.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 200); r.set_editor_property("r", rough)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if emissive:
        e = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 400)
        e.set_editor_property("constant", unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0))
        MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    try:
        MEL.recompile_material(m)
    except Exception:
        pass
    EAL.save_asset(p, False); MATS[name] = m; return m


def mats():
    matx("ST_Wall",   (0.66, 0.63, 0.57))
    matx("ST_Floor",  (0.80, 0.79, 0.76))
    matx("ST_Gal",    (0.34, 0.47, 0.64))
    matx("ST_Hall",   (0.30, 0.40, 0.56))
    matx("ST_Col",    (0.87, 0.86, 0.83))
    matx("ST_Stair",  (0.58, 0.44, 0.30))
    matx("ST_Court",  (0.50, 0.51, 0.53))
    matx("ST_Garden", (0.28, 0.46, 0.26))
    matx("ST_Water",  (0.20, 0.42, 0.62), rough=0.1)
    matx("ST_Roof",   (0.34, 0.30, 0.30))
    matx("ST_Mark",   (0.92, 0.14, 0.10), emissive=(0.6, 0.05, 0.03))


def mesh(p):
    if p not in _m:
        _m[p] = EAL.load_asset(p)
    return _m[p]


def box(x, y, z, sx, sy, sz, m, name=None):
    a = acts.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z))
    a.static_mesh_component.set_static_mesh(mesh(CUBE))
    a.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    a.static_mesh_component.set_material(0, MATS[m]); a.set_mobility(unreal.ComponentMobility.STATIC)
    if name:
        a.set_actor_label(name)
    CNT["n"] += 1
    return a


def cyl(x, y, z, rad, h, m, name=None):
    a = acts.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z))
    a.static_mesh_component.set_static_mesh(mesh(CYL))
    a.set_actor_scale3d(unreal.Vector(rad / 50.0, rad / 50.0, h / 100.0))
    a.static_mesh_component.set_material(0, MATS[m]); a.set_mobility(unreal.ComponentMobility.STATIC)
    if name:
        a.set_actor_label(name)
    CNT["n"] += 1
    return a


def plight(x, y, z, inten, rad, col=(235, 240, 255)):
    a = acts.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, z))
    c = a.get_component_by_class(unreal.PointLightComponent)
    c.set_editor_property("intensity", inten); c.set_editor_property("attenuation_radius", rad)
    c.set_editor_property("light_color", unreal.Color(col[0], col[1], col[2], 255))
    c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    CNT["n"] += 1
    return a


T = 80.0
FH = 1800.0     # 층 높이 18m
DH = 3600.0     # 통층 대공간 36m


def floor(x0, x1, y0, y1, z, m, th=40, name="floor"):
    box((x0 + x1) / 2, (y0 + y1) / 2, z - th / 2, (x1 - x0), (y1 - y0), th, m, name=name)


def slab_up(x0, x1, y0, y1, z, m, th=60, name="slab"):
    box((x0 + x1) / 2, (y0 + y1) / 2, z + th / 2, (x1 - x0), (y1 - y0), th, m, name=name)


def wallX(x0, x1, y, z0, z1, m, doors=None, doorh=1400, name="wall"):
    doors = sorted(doors or [], key=lambda d: d[0])
    cur = x0
    for (dc, dw) in doors:
        ds, de = dc - dw / 2, dc + dw / 2
        if ds > cur:
            box((cur + ds) / 2, y, (z0 + z1) / 2, (ds - cur), T, (z1 - z0), m, name=name)
        if z1 > z0 + doorh:
            box(dc, y, (z0 + doorh + z1) / 2, dw, T, (z1 - (z0 + doorh)), m, name=name)
        cur = de
    if x1 > cur:
        box((cur + x1) / 2, y, (z0 + z1) / 2, (x1 - cur), T, (z1 - z0), m, name=name)


def wallY(y0, y1, x, z0, z1, m, doors=None, doorh=1400, name="wall"):
    doors = sorted(doors or [], key=lambda d: d[0])
    cur = y0
    for (dc, dw) in doors:
        ds, de = dc - dw / 2, dc + dw / 2
        if ds > cur:
            box(x, (cur + ds) / 2, (z0 + z1) / 2, T, (ds - cur), (z1 - z0), m, name=name)
        if z1 > z0 + doorh:
            box(x, dc, (z0 + doorh + z1) / 2, T, dw, (z1 - (z0 + doorh)), m, name=name)
        cur = de
    if y1 > cur:
        box(x, (cur + y1) / 2, (z0 + z1) / 2, T, (y1 - cur), (z1 - z0), m, name=name)


def marker(x, y, z=0, name="사람1.8m"):
    box(x, y, z + 90, 60, 40, 180, "ST_Mark", name=name)


def colrow_x(x0, x1, y, n, z0, z1, name="col"):
    for i in range(n + 1):
        cx = x0 + (x1 - x0) * i / n
        cyl(cx, y, (z0 + z1) / 2, 200, z1 - z0, "ST_Col", name=name)


def colrow_y(y0, y1, x, n, z0, z1, name="col"):
    for i in range(n + 1):
        cy = y0 + (y1 - y0) * i / n
        cyl(x, cy, (z0 + z1) / 2, 200, z1 - z0, "ST_Col", name=name)


# ═══ 정본 평면 좌표(cm) — 대칭 ═══
BX0, BX1 = -17000.0, 17000.0     # 건물 전폭 340m
GAL_W = 3000.0                    # 대회랑 폭 30m
GXW0, GXW1 = BX0, BX0 + GAL_W     # 서회랑 X[-17000,-14000]
GXE0, GXE1 = BX1 - GAL_W, BX1     # 동회랑 X[14000,17000]
CBX0, CBX1 = GXW1, GXE0           # 중앙 블록 X[-14000,14000]
Y_CFRONT = -6000.0                # 건물 앞면(코트)
Y_A_C    = -3400.0                # 대기실|중앙band
Y_C_E    = 2200.0                 # 중앙band|엔필라드
Y_GARDEN = 6000.0                 # 건물 뒷면(정원)
VX0, VX1 = -5000.0, 5000.0        # 현관/중앙홀 코어 폭
FC_Y0, FC_Y1 = -22000.0, -6000.0  # 전정
GD_Y0, GD_Y1 = 6000.0, 32000.0    # 정원
ENF_BND = [-14000.0, -8400.0, -2800.0, 2800.0, 8400.0, 14000.0]  # 엔필라드 5칸
ENF_AXIS_DOOR = (4100.0, 1400.0)  # 엔필라드 관통 문(문축 일직선)


def build():
    mats()
    if EAL.does_asset_exist(MAP):
        EAL.delete_asset(MAP)
        log("헌 맵 삭제")
    log("new_level -> %s" % les.new_level(MAP))

    # ── 연속 지반(건물보다 50cm 낮춤 = 건물이 기단 위에 앉음, 바닥 겹침 z-fighting 방지) ──
    floor(BX0 - 2500, BX1 + 2500, FC_Y0 - 1500, GD_Y1 + 1500, -50, "ST_Court", name="Ground")

    # ══════════ 건물 바닥 — 겹치지 않게 분할(중앙홀 둘레로 조각) ══════════
    # 중앙 블록: 중앙홀만 강조색, 나머지는 4조각으로 홀을 둘러 깐다(겹침 0)
    floor(CBX0, CBX1, Y_CFRONT, Y_A_C, 0, "ST_Floor", name="B_front")
    floor(CBX0, CBX1, Y_C_E, Y_GARDEN, 0, "ST_Floor", name="B_enf")
    floor(CBX0, VX0, Y_A_C, Y_C_E, 0, "ST_Floor", name="B_midW")
    floor(VX1, CBX1, Y_A_C, Y_C_E, 0, "ST_Floor", name="B_midE")
    floor(VX0, VX1, Y_A_C, Y_C_E, 0, "ST_Hall", name="hall_floor")     # 중앙홀 강조
    floor(GXW0, GXW1, Y_CFRONT, Y_GARDEN, 0, "ST_Gal", name="galW_floor")
    floor(GXE0, GXE1, Y_CFRONT, Y_GARDEN, 0, "ST_Gal", name="galE_floor")
    # 지붕(전체 36m에서 닫음)
    slab_up(BX0, BX1, Y_CFRONT, Y_GARDEN, DH, "ST_Roof", th=140, name="Roof")

    # ══════════ 외벽(창 개구부로 채광) ══════════
    # 앞면(코트): 현관 대문 + 대기실 큰 창
    win_front = [(0, 2600)] + [(x, 1600) for x in (-11000, -8000, 8000, 11000)]
    wallX(BX0, BX1, Y_CFRONT, 0, DH, "ST_Wall", doors=win_front, doorh=2200, name="ext_front")
    # 뒷면(정원): 엔필라드 큰 창 + 정원 문
    win_gard = [(x, 1800) for x in range(-13000, 13001, 3200)]
    wallX(BX0, BX1, Y_GARDEN, 0, DH, "ST_Wall", doors=win_gard, doorh=2400, name="ext_garden")
    # 좌우 외벽(회랑 창 — 참고 이미지처럼 큰 창 줄)
    win_side = [(y, 1600) for y in range(-4800, 4801, 2000)]
    wallY(Y_CFRONT, Y_GARDEN, BX0, 0, DH, "ST_Wall", doors=win_side, doorh=2400, name="ext_W")
    wallY(Y_CFRONT, Y_GARDEN, BX1, 0, DH, "ST_Wall", doors=win_side, doorh=2400, name="ext_E")

    # ══════════ 대회랑(서·동) — 통층 36m, 기둥 두 줄 ══════════
    for (GX0, GX1, tag) in ((GXW0, GXW1, "W"), (GXE0, GXE1, "E")):
        # 회랑 천장(통층)·안쪽 벽(중앙블록으로 큰 문 여러 개)
        inner_x = GX1 if tag == "W" else GX0
        wallY(Y_CFRONT, Y_GARDEN, inner_x, 0, DH, "ST_Wall",
              doors=[(-4700, 1600), (-800, 1600), (4100, 1600)], doorh=2200, name="gal_inner")
        # 기둥 두 줄
        gxm = (GX0 + GX1) / 2
        colrow_y(Y_CFRONT + 1200, Y_GARDEN - 1200, gxm - 900, 8, 0, DH, name="gal_col")
        colrow_y(Y_CFRONT + 1200, Y_GARDEN - 1200, gxm + 900, 8, 0, DH, name="gal_col")

    # ══════════ 중앙 축: 현관 → 중앙홀(통층) ══════════
    # 현관(통층) — 대기실과 좌우 문
    wallY(Y_CFRONT, Y_A_C, VX0, 0, DH, "ST_Wall", doors=[(-4700, 1400)], doorh=1400, name="vest_W")
    wallY(Y_CFRONT, Y_A_C, VX1, 0, DH, "ST_Wall", doors=[(-4700, 1400)], doorh=1400, name="vest_E")
    # 현관|중앙홀 칸막이(대문)
    wallX(VX0, VX1, Y_A_C, 0, DH, "ST_Wall", doors=[(0, 2200)], doorh=2200, name="p_vest_hall")
    # 중앙홀 좌우 벽(→ 곁 살롱 큰 문)
    wallY(Y_A_C, Y_C_E, VX0, 0, DH, "ST_Wall", doors=[(-1600, 1600), (1400, 1600)], doorh=2000, name="hall_W")
    wallY(Y_A_C, Y_C_E, VX1, 0, DH, "ST_Wall", doors=[(-1600, 1600), (1400, 1600)], doorh=2000, name="hall_E")
    # 중앙홀|엔필라드 대살롱(대문)
    wallX(VX0, VX1, Y_C_E, 0, DH, "ST_Wall", doors=[(0, 2200)], doorh=2200, name="p_hall_enf")
    # 중앙홀 거대 기둥 4
    for sx in (-3200, 3200):
        for sy in (Y_A_C + 1500, Y_C_E - 1500):
            cyl(sx, sy, DH / 2, 300, DH, "ST_Col", name="hall_col")

    # ══════════ 대계단 — 중앙홀 뒤쪽, ★진짜 2층(z=FH)으로 ══════════
    steps = 20
    run = 130.0
    for s in range(steps):
        z = s * (FH / steps)
        box(0, Y_C_E - 300 - s * run, z + (FH / steps) / 2, 4000, run + 10, (FH / steps), "ST_Stair", name="grand_stair")
    land_y = Y_C_E - 300 - steps * run - 500
    box(0, land_y, FH, 4600, 900, 60, "ST_Floor", name="stair_landing")

    # ══════════ 대기실(현관 좌우, 2층) ══════════
    for (RX0, RX1) in ((CBX0, VX0), (VX1, CBX1)):
        slab_up(RX0, RX1, Y_CFRONT, Y_A_C, FH, "ST_Floor", name="ante_2f")
        # 대기실|곁살롱 (Y_A_C) 문
        wallX(RX0, RX1, Y_A_C, 0, FH, "ST_Wall", doors=[((RX0 + RX1) / 2, 1400)], name="ante_back_1")
        wallX(RX0, RX1, Y_A_C, FH, DH, "ST_Wall", doors=[((RX0 + RX1) / 2, 1400)], name="ante_back_2")

    # ══════════ 곁 살롱(중앙홀 좌우, 2층) ══════════
    for (RX0, RX1) in ((CBX0, VX0), (VX1, CBX1)):
        slab_up(RX0, RX1, Y_A_C, Y_C_E, FH, "ST_Floor", name="salon_2f")
        # 곁살롱|엔필라드 (Y_C_E) 문
        wallX(RX0, RX1, Y_C_E, 0, FH, "ST_Wall", doors=[((RX0 + RX1) / 2, 1400)], name="salon_back_1")
        wallX(RX0, RX1, Y_C_E, FH, DH, "ST_Wall", doors=[((RX0 + RX1) / 2, 1400)], name="salon_back_2")

    # ══════════ 국가실 엔필라드(정원 앞면, 5칸, 문축 일직선, 2층) ══════════
    slab_up(CBX0, CBX1, Y_C_E, Y_GARDEN, FH, "ST_Floor", name="enf_2f")
    for bx in ENF_BND[1:-1]:   # 내부 칸막이 4개
        wallY(Y_C_E, Y_GARDEN, bx, 0, FH, "ST_Wall", doors=[ENF_AXIS_DOOR], name="enf_part_1")
        wallY(Y_C_E, Y_GARDEN, bx, FH, DH, "ST_Wall", doors=[ENF_AXIS_DOOR], name="enf_part_2")
    # 엔필라드 양 끝 ↔ 회랑 문(축 맞춤)
    for bx in (CBX0, CBX1):
        wallY(Y_C_E, Y_GARDEN, bx, 0, DH, "ST_Wall", doors=[ENF_AXIS_DOOR, (Y_C_E + 900, 1400)], doorh=1600, name="enf_end")

    # ══════════ 2층: 상부 발코니(통층 공간 둘레 — 걸을 수 있게) + 난간 ══════════
    for yy in (Y_A_C, Y_C_E):
        box(0, yy, FH, VX1 - VX0, 700, 60, "ST_Floor", name="hall_balcony")
    for xx in (VX0, VX1):
        box(xx, (Y_A_C + Y_C_E) / 2, FH, 700, Y_C_E - Y_A_C, 60, "ST_Floor", name="hall_balcony")

    # ══════════ 전정(cour d'honneur) — U자 담·대문·분수 ══════════
    for xx in (CBX0 + 3000, CBX1 - 3000):   # 코트 담(본관 앞 U)
        wallY(FC_Y0, Y_CFRONT, xx, 0, 1400, "ST_Court", name="court_wall")
    wallX(CBX0 + 3000, CBX1 - 3000, FC_Y0, 0, 1800, "ST_Court", doors=[(0, 4000)], doorh=1600, name="court_front")
    for gx in (-2000, 2000):
        box(gx, FC_Y0, 1300, 360, 360, 2600, "ST_Wall", name="gate_pier")
    fx, fy = 0.0, (FC_Y0 + Y_CFRONT) / 2 - 1500
    cyl(fx, fy, 40, 2200, 80, "ST_Court", name="fount_base")
    cyl(fx, fy, 100, 1700, 80, "ST_Water", name="fount_water")
    cyl(fx, fy, 460, 240, 820, "ST_Court", name="fount_stem")

    # ══════════ 정원(파르테르 + 정자) ══════════
    gy = GD_Y0 + 2600
    while gy < GD_Y1 - 4000:
        for gx in range(-13000, 13001, 3400):
            box(gx, gy, 240, 2000, 460, 480, "ST_Garden", name="hedge")
        gy += 4600
    pav_cy = GD_Y1 - 3200
    for i in range(12):
        a = 2 * math.pi * i / 12
        cyl(3000 * math.cos(a), pav_cy + 3000 * math.sin(a), 1300, 170, 2600, "ST_Col", name="pav_col")
    box(0, pav_cy, 2680, 7200, 7200, 180, "ST_Roof", name="pav_roof")

    # ══════════ 사람 1.8m 마커(스케일) ══════════
    marker(0, FC_Y0 + 1400, name="마커_정문")
    marker(0, (Y_CFRONT + Y_A_C) / 2, name="마커_현관")
    marker(0, (Y_A_C + Y_C_E) / 2, name="마커_중앙홀")
    for bx in ((ENF_BND[i] + ENF_BND[i + 1]) / 2 for i in range(5)):
        marker(bx, 4100, name="마커_엔필라드")
    marker((GXW0 + GXW1) / 2, 0, name="마커_서회랑")
    marker((GXE0 + GXE1) / 2, 0, name="마커_동회랑")
    marker(0, (Y_A_C + Y_C_E) / 2, z=FH, name="마커_2층")
    marker(0, GD_Y0 + 3000, name="마커_정원")

    # ══════════ 조명(대낮·실내까지) ══════════
    d = acts.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 9000), unreal.Rotator(-55, -30, 0))
    dc = d.get_component_by_class(unreal.DirectionalLightComponent)
    dc.set_editor_property("intensity", 4.0); dc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    dc.set_editor_property("atmosphere_sun_light", True)
    acts.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    sk = acts.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 5000))
    skc = sk.get_component_by_class(unreal.SkyLightComponent)
    skc.set_editor_property("real_time_capture", True); skc.set_editor_property("intensity", 2.5)
    skc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    # 실내 채움광(지붕 덮여 어두운 깊은 방들)
    for (lx, ly) in [(0, -1600), (0, 700), (-9000, 700), (9000, 700), (0, 4100),
                     (-11000, 4100), (11000, 4100), (GXW0 + 1500, 0), (GXE1 - 1500, 0)]:
        plight(lx, ly, DH - 700, 4000, 6000)
    pp = acts.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 2000))
    pp.set_editor_property("unbound", True)
    st = pp.get_editor_property("settings")
    st.set_editor_property("override_auto_exposure_method", True)
    st.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    st.set_editor_property("override_auto_exposure_min_brightness", True); st.set_editor_property("auto_exposure_min_brightness", 0.5)
    st.set_editor_property("override_auto_exposure_max_brightness", True); st.set_editor_property("auto_exposure_max_brightness", 1.5)
    pp.set_editor_property("settings", st)

    acts.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, FC_Y0 + 2500, 190))
    log("save -> %s · 액터 %d" % (les.save_current_level(), CNT["n"]))


open(LOG, "w", encoding="utf-8").close()
try:
    build()
    log("=== 궁전3(정본평면) 완료 ===")
except Exception:
    log("실패\n" + traceback.format_exc())
with open(LOG, "w", encoding="utf-8") as f:
    f.write("\n".join(_ll))
