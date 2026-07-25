# -*- coding: utf-8 -*-
# ▶ 네르한 궁전 — 전체를 한 번에, 2배 스케일(공간 8배), 끊김 없이 걸어서 이어지는 연속 부지.
#   사용자 2026-07-25: 가로·세로·높이 싹 2배. 공간별로 나눠 만들지 말고 통째로. 포탈 없이 그냥 돌아다니게.
#   정문→전정→대현관→대회랑→양익 살롱들(엔필라드)→정원까지 문 개구부로 전부 연결. 바닥 하나로 연속.
#   실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=_build_palace2.py
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
    _ll.append(str(s)); unreal.log("[palace2] " + str(s))


def mat(name, rgb, rough=0.85, emissive=None):
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
    mat("ST_Wall",   (0.66, 0.63, 0.57))
    mat("ST_Floor",  (0.80, 0.79, 0.76))
    mat("ST_Gal",    (0.34, 0.47, 0.64))   # 대회랑 바닥
    mat("ST_Hall",   (0.30, 0.40, 0.56))   # 대현관 홀 바닥
    mat("ST_Col",    (0.87, 0.86, 0.83))
    mat("ST_Stair",  (0.58, 0.44, 0.30))
    mat("ST_Court",  (0.50, 0.51, 0.53))
    mat("ST_Garden", (0.28, 0.46, 0.26))
    mat("ST_Water",  (0.20, 0.42, 0.62), rough=0.1)
    mat("ST_Roof",   (0.34, 0.30, 0.30))
    mat("ST_Mark",   (0.92, 0.14, 0.10), emissive=(0.6, 0.05, 0.03))


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


T = 80.0          # 벽 두께(궁전, 2배)
DOORH = 1000.0    # 큰 문 10m


def floor(x0, x1, y0, y1, m, z=0, th=40, name="floor"):
    box((x0 + x1) / 2, (y0 + y1) / 2, z - th / 2, (x1 - x0), (y1 - y0), th, m, name=name)


def ceil(x0, x1, y0, y1, z, m, th=120, name="ceil"):
    box((x0 + x1) / 2, (y0 + y1) / 2, z + th / 2, (x1 - x0), (y1 - y0), th, m, name=name)


def wallX(x0, x1, y, z0, z1, m, doors=None, name="wall"):
    """X방향 벽. doors=[(중심x,폭),...] 구간을 문(위 상인방)으로 비운다."""
    doors = sorted(doors or [], key=lambda d: d[0])
    cur = x0
    for (dc, dw) in doors:
        ds, de = dc - dw / 2, dc + dw / 2
        if ds > cur:
            box((cur + ds) / 2, y, (z0 + z1) / 2, (ds - cur), T, (z1 - z0), m, name=name)
        if z1 > z0 + DOORH:
            box(dc, y, (z0 + DOORH + z1) / 2, dw, T, (z1 - (z0 + DOORH)), m, name=name)
        cur = de
    if x1 > cur:
        box((cur + x1) / 2, y, (z0 + z1) / 2, (x1 - cur), T, (z1 - z0), m, name=name)


def wallY(y0, y1, x, z0, z1, m, doors=None, name="wall"):
    doors = sorted(doors or [], key=lambda d: d[0])
    cur = y0
    for (dc, dw) in doors:
        ds, de = dc - dw / 2, dc + dw / 2
        if ds > cur:
            box(x, (cur + ds) / 2, (z0 + z1) / 2, T, (ds - cur), (z1 - z0), m, name=name)
        if z1 > z0 + DOORH:
            box(x, dc, (z0 + DOORH + z1) / 2, T, dw, (z1 - (z0 + DOORH)), m, name=name)
        cur = de
    if y1 > cur:
        box(x, (cur + y1) / 2, (z0 + z1) / 2, T, (y1 - cur), (z1 - z0), m, name=name)


def marker(x, y, name="사람1.8m"):
    box(x, y, 90, 60, 40, 180, "ST_Mark", name=name)


def colrow(x0, x1, y, n, z_top, name="column"):
    for i in range(n + 1):
        cx = x0 + (x1 - x0) * i / n
        cyl(cx, y, z_top / 2, 200, z_top, "ST_Col", name=name)


# ═══════════ 궁전 좌표(cm) — 2배 스케일 ═══════════
# X 대칭(중심 0). 앞(-Y)=정문/전정 → 본관 → 대회랑 → 정원(+Y).
BX0, BX1 = -16000.0, 16000.0     # 본관 폭 320m
Y_FRONT  = -8000.0               # 본관 앞면(전정에 면함)
Y_VH     = -2000.0               # 현관|중앙홀
Y_HG     = 6000.0                # 중앙홀|대회랑
Y_GAR    = 10800.0              # 대회랑|정원(뒷면)
CX0, CX1 = -8000.0, 8000.0       # 중앙 구역
HX0, HX1 = -4000.0, 4000.0       # 중앙홀 코어
H_GAL  = 4800.0                  # 대회랑 천장 48m (2배)
H_HALL = 4800.0                  # 중앙홀 48m
H_WING = 3600.0                  # 양익 살롱 36m
FC_Y0, FC_Y1 = -26000.0, -8000.0 # 전정 (아래로 180m)
GD_Y0, GD_Y1 = 10800.0, 38000.0  # 정원 (뒤로 272m)


def build():
    mats()
    if EAL.does_asset_exist(MAP):
        EAL.delete_asset(MAP)
        log("헌 맵 삭제")
    log("new_level -> %s" % les.new_level(MAP))

    # ── 연속 지반(전정~정원 전체) — 이 위를 끊김 없이 걸어다닌다 ──
    floor(BX0 - 2000, BX1 + 2000, FC_Y0 - 1000, GD_Y1 + 1000, "ST_Court", name="Ground")

    # ═══ 본관 외곽 벽 (앞·좌·우) + 뒷면은 대회랑 정원벽이 담당 ═══
    # 앞면(Y_FRONT): 전정→대현관 큰 문
    wallX(BX0, BX1, Y_FRONT, 0, H_WING, "ST_Wall", doors=[(0, 3000)], name="front")
    # 좌우 외벽
    wallY(Y_FRONT, Y_GAR, BX0, 0, H_WING, "ST_Wall", name="ext_W")
    wallY(Y_FRONT, Y_GAR, BX1, 0, H_WING, "ST_Wall", name="ext_E")

    # ═══ 중앙 축: 대현관(vestibule) → 중앙 대홀 ═══
    # 현관 바닥 + 중앙홀 바닥(강조색)
    floor(HX0, HX1, Y_FRONT, Y_VH, "ST_Floor", name="vest_floor")
    floor(HX0, HX1, Y_VH, Y_HG, "ST_Hall", name="hall_floor")
    ceil(HX0, HX1, Y_FRONT, Y_VH, H_WING, "ST_Floor", name="vest_ceil")
    ceil(HX0, HX1, Y_VH, Y_HG, H_HALL, "ST_Floor", name="hall_ceil")
    # 현관|중앙홀 칸막이(큰 문)
    wallX(HX0, HX1, Y_VH, 0, H_HALL, "ST_Wall", doors=[(0, 2400)], name="p_vh")
    # 중앙홀 좌우 벽(→ 계단홀로 큰 문)
    wallY(Y_VH, Y_HG, HX0, 0, H_HALL, "ST_Wall", doors=[(1500, 1800), (4500, 1800)], name="p_hallW")
    wallY(Y_VH, Y_HG, HX1, 0, H_HALL, "ST_Wall", doors=[(1500, 1800), (4500, 1800)], name="p_hallE")
    # 중앙홀 거대 기둥 + 상부 발코니(대계단으로 오름)
    for sx in (-2600, 2600):
        for sy in (Y_VH + 1400, Y_HG - 1400):
            cyl(sx, sy, H_HALL / 2, 320, H_HALL, "ST_Col", name="hall_col")
    # 대계단(중앙홀 뒤쪽, 상부 발코니 z=2000 으로) — 걸어서 위층으로
    steps = 24
    for s in range(steps):
        box(0, Y_HG - 200 - s * 130, s * 85 + 20, 4400, 130, s * 85 + 40, "ST_Stair", name="grand_stair")
    box(0, Y_HG - 200 - steps * 130 - 400, steps * 85 + 20, 5200, 900, 60, "ST_Floor", name="stair_landing")
    # 상부 발코니(중앙홀 둘레 회랑) — 걸을 수 있는 슬래브 링
    bz = 2100.0
    for (yy) in (Y_VH + 200, Y_HG - 200):
        box(0, yy, bz, HX1 - HX0, 700, 60, "ST_Floor", name="balcony")
    for (xx) in (HX0 + 200, HX1 - 200):
        box(xx, (Y_VH + Y_HG) / 2, bz, 700, Y_HG - Y_VH, 60, "ST_Floor", name="balcony")

    # ═══ 대회랑(hero) — 정원 앞면, 본관 전폭. 400m급이 아니라 전폭 320m × 폭 44m × 천장 48m ═══
    floor(BX0, BX1, Y_HG, Y_GAR, "ST_Gal", name="gal_floor")
    ceil(BX0, BX1, Y_HG, Y_GAR, H_GAL, "ST_Floor", th=160, name="gal_ceil")
    # 중앙홀|대회랑 칸막이(여러 큰 문으로 연결)
    wallX(BX0, BX1, Y_HG, 0, H_GAL, "ST_Wall",
          doors=[(-12000, 2400), (-6000, 2400), (0, 3200), (6000, 2400), (12000, 2400)], name="p_hg")
    # 정원측 뒷벽(창+정원 문) — 큰 창 개구부(피어 사이) + 정원 나가는 문
    gdoors = [(x, 1600) for x in range(-14000, 14001, 4000)]
    wallX(BX0, BX1, Y_GAR, 0, H_GAL, "ST_Wall", doors=gdoors, name="gal_garden")
    # 대회랑 기둥 두 줄(전폭)
    colrow(BX0 + 1500, BX1 - 1500, Y_HG + 1200, 18, H_GAL, name="gal_colN")
    colrow(BX0 + 1500, BX1 - 1500, Y_GAR - 1200, 18, H_GAL, name="gal_colS")

    # ═══ 양익 살롱 엔필라드(좌·우) — 중앙 구역 밖 X[BX0..CX0], [CX1..BX1] ═══
    for (WX0, WX1, side) in ((BX0, CX0, "W"), (CX1, BX1, "E")):
        # 중앙 구역과의 경계벽(큰 문)
        wallY(Y_FRONT, Y_HG, (CX0 if side == "W" else CX1), 0, H_WING, "ST_Wall",
              doors=[(-5000, 1800), (0, 1800), (4000, 1800)], name="wing_inner")
        # 엔필라드: 세로로 방 4칸(가로 칸막이), 문은 축을 맞춰 쭉 연결
        ysteps = [Y_FRONT, -3600, 800, 5200, Y_HG]
        cxm = (WX0 + WX1) / 2
        for yy in ysteps[1:-1]:
            wallX(WX0, WX1, yy, 0, H_WING, "ST_Wall", doors=[(cxm, 1600)], name="wing_part")
        # 살롱 바닥·천장
        floor(WX0, WX1, Y_FRONT, Y_HG, "ST_Floor", name="wing_floor")
        ceil(WX0, WX1, Y_FRONT, Y_HG, H_WING, "ST_Floor", name="wing_ceil")

    # ═══ 전정(cour d'honneur) — 정문·담·큰 분수 ═══
    for xx in (CX0, CX1):
        wallY(FC_Y0, FC_Y1, xx, 0, 1200, "ST_Court", name="court_wall")
    wallX(CX0, CX1, FC_Y0, 0, 1600, "ST_Court", doors=[(0, 3600)], name="court_front")
    for gx in (-1800, 1800):
        box(gx, FC_Y0, 1100, 320, 320, 2200, "ST_Wall", name="gate_pier")
    fx, fy = 0.0, (FC_Y0 + FC_Y1) / 2 - 2000
    cyl(fx, fy, 40, 2400, 80, "ST_Court", name="fount_base")
    cyl(fx, fy, 100, 1900, 80, "ST_Water", name="fount_water")
    cyl(fx, fy, 500, 260, 900, "ST_Court", name="fount_stem")

    # ═══ 정원(대형 파르테르) — 산울타리 격자 + 끝에 정원 파빌리온 ═══
    gy = GD_Y0 + 2400
    while gy < GD_Y1 - 4000:
        for gx in range(-14000, 14001, 4000):
            box(gx, gy, 260, 2400, 500, 520, "ST_Garden", name="hedge")
        gy += 5000
    # 정원 끝 파빌리온(원형 정자)
    pav_cy = GD_Y1 - 3000
    for i in range(12):
        a = 2 * math.pi * i / 12
        cyl(3400 * math.cos(a), pav_cy + 3400 * math.sin(a), 1400, 180, 2800, "ST_Col", name="paviln_col")
    box(0, pav_cy, 2850, 8000, 8000, 200, "ST_Roof", name="paviln_roof")

    # ═══ 사람 1.8m 마커 — 곳곳에 스케일 기준 ═══
    for mx in range(-14000, 14001, 3500):
        marker(mx, (Y_HG + Y_GAR) / 2, "마커_회랑")     # 대회랑
    marker(0, FC_Y0 + 1200, "마커_정문")
    marker(0, (Y_VH + Y_HG) / 2, "마커_중앙홀")
    marker(-12000, 800, "마커_서익")
    marker(12000, 800, "마커_동익")
    marker(0, GD_Y0 + 3000, "마커_정원")

    # ═══ 밝은 대낮 ═══
    d = acts.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 9000), unreal.Rotator(-52, -35, 0))
    dc = d.get_component_by_class(unreal.DirectionalLightComponent)
    dc.set_editor_property("intensity", 4.0); dc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    dc.set_editor_property("atmosphere_sun_light", True)
    acts.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    sk = acts.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 5000))
    skc = sk.get_component_by_class(unreal.SkyLightComponent)
    skc.set_editor_property("real_time_capture", True); skc.set_editor_property("intensity", 2.5)
    skc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    pp = acts.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 2000))
    pp.set_editor_property("unbound", True)
    st = pp.get_editor_property("settings")
    st.set_editor_property("override_auto_exposure_method", True)
    st.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    st.set_editor_property("override_auto_exposure_min_brightness", True); st.set_editor_property("auto_exposure_min_brightness", 0.6)
    st.set_editor_property("override_auto_exposure_max_brightness", True); st.set_editor_property("auto_exposure_max_brightness", 1.4)
    pp.set_editor_property("settings", st)

    acts.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, FC_Y0 + 2500, 190))
    log("save -> %s · 액터 %d" % (les.save_current_level(), CNT["n"]))


open(LOG, "w", encoding="utf-8").close()
try:
    build()
    log("=== 궁전2(2배·통연결) 완료 ===")
except Exception:
    log("실패\n" + traceback.format_exc())
with open(LOG, "w", encoding="utf-8") as f:
    f.write("\n".join(_ll))
