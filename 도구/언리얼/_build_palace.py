# -*- coding: utf-8 -*-
# ▶ 네르한 궁전 — 전체 부지 뼈대(블록아웃), 궁전 스케일. (2026-07-25 재스케일)
#   사용자: 참고 회랑은 천장~24m·폭~20m·길이 100m+ 통층 대공간. 108m 저택은 어림없음 → 궁전으로 키움.
#   핵심: 잘게 나눈 방 X → 웅장한 통층 대공간. 대회랑(200m×22m×24m)이 주인공. 사람 1.8m 마커로 스케일.
#   실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=_build_palace.py
import unreal, traceback

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
    _ll.append(str(s)); unreal.log("[palace] " + str(s))


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
    mat("ST_Slab",   (0.80, 0.79, 0.76))
    mat("ST_Hall",   (0.34, 0.47, 0.64))
    mat("ST_Col",    (0.86, 0.85, 0.82))   # 기둥·필라스터
    mat("ST_Stair",  (0.58, 0.44, 0.30))
    mat("ST_Court",  (0.48, 0.49, 0.51))
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


T = 60.0        # 궁전 벽 두께
DOORH = 500.0   # 큰 문 5m


def slab(x0, x1, y0, y1, z, m, th=40, name="slab"):
    box((x0 + x1) / 2, (y0 + y1) / 2, z - th / 2, (x1 - x0), (y1 - y0), th, m, name=name)


def marker(x, y, name="사람1.8m"):
    box(x, y, 90, 60, 40, 180, "ST_Mark", name=name)


# ═══ 궁전 좌표(cm) — 궁전 스케일 ═══
GAL_X0, GAL_X1 = -10000.0, 10000.0    # 대회랑 길이 200m
GAL_HW = 1100.0                        # 회랑 폭 22m (반폭 1100)
GAL_H  = 2400.0                        # 회랑 천장 24m
CEN_HW = 3500.0                        # 중앙 파빌리온 반폭/반깊이 (70m)
CEN_H  = 3000.0                        # 중앙 대현관 30m
FC_Y0, FC_Y1 = -12500.0, -3500.0       # 전정(코트) 앞
GD_Y0, GD_Y1 = 3500.0, 18000.0         # 뒤 정원


def grand_gallery():
    """주인공: 200m×22m×24m 통층 대회랑. 정원측(+Y) 큰 창(기둥 사이 개구부), 코트측(-Y) 벽·필라스터."""
    # 바닥 + 천장(격자 대신 통 슬래브 — 뼈대)
    slab(GAL_X0, GAL_X1, -GAL_HW, GAL_HW, 2, "ST_Hall", name="gal_floor")
    slab(GAL_X0, GAL_X1, -GAL_HW, GAL_HW, GAL_H, "ST_Slab", th=120, name="gal_ceil")
    # 코트측 긴 벽(솔리드, 그림 거는 쪽)
    box(0, -GAL_HW, GAL_H / 2, (GAL_X1 - GAL_X0), T, GAL_H, "ST_Wall", name="gal_wall_court")
    # 기둥/필라스터 열 — 양쪽에 일정 간격(참고 이미지의 거대 오더)
    bays = 20
    for i in range(bays + 1):
        gx = GAL_X0 + (GAL_X1 - GAL_X0) * i / bays
        # 코트측 엔게이지드 필라스터
        box(gx, -GAL_HW + 120, GAL_H / 2, 300, 160, GAL_H, "ST_Col", name="pilaster")
        # 정원측 창 피어(기둥) — 사이는 큰 창 개구부(뼈대=열린 틈, 빛 들어옴)
        box(gx, GAL_HW, GAL_H / 2, 360, T + 60, GAL_H, "ST_Col", name="window_pier")
    # 정원측 창 상·하 인방(피어를 잇는 띠) — 개구부가 '창'으로 읽히게
    box(0, GAL_HW, 350, (GAL_X1 - GAL_X0), T + 40, 700, "ST_Wall", name="win_sill")     # 낮은 벽(난간 높이)
    box(0, GAL_HW, GAL_H - 250, (GAL_X1 - GAL_X0), T + 40, 500, "ST_Wall", name="win_lintel")  # 상인방


def central_pavilion():
    """중앙 대현관/계단 홀 — 더 높고(30m) 앞뒤로 돌출."""
    x0, x1 = -CEN_HW, CEN_HW
    y0, y1 = -CEN_HW - 500, CEN_HW + 500
    # 외벽 4면(앞벽에 대문 개구부)
    for (yy, dr) in ((y0, (0, 1400)), (y1, None)):
        if dr:
            dc, dw = dr
            for (a, b) in ((x0, dc - dw / 2), (dc + dw / 2, x1)):
                box((a + b) / 2, yy, CEN_H / 2, (b - a), T, CEN_H, "ST_Wall", name="cen_wall")
            box(dc, yy, (DOORH + CEN_H) / 2, dw, T, CEN_H - DOORH, "ST_Wall", name="cen_lintel")
        else:
            box((x0 + x1) / 2, yy, CEN_H / 2, (x1 - x0), T, CEN_H, "ST_Wall", name="cen_wall")
    for xx in (x0, x1):
        box(xx, (y0 + y1) / 2, CEN_H / 2, T, (y1 - y0), CEN_H, "ST_Wall", name="cen_wall")
    slab(x0, x1, y0, y1, 2, "ST_Slab", name="cen_floor")
    slab(x0, x1, y0, y1, CEN_H, "ST_Slab", th=140, name="cen_ceil")
    # 내부 거대 기둥 4개
    for sx in (-1800, 1800):
        for sy in (-1800, 1800):
            cyl(sx, sy, CEN_H / 2, 220, CEN_H, "ST_Col", name="cen_column")
    # 대계단(뒤쪽, 두 층 규모)
    steps = 18
    for s in range(steps):
        box(0, 1200 + s * 90, s * 55 + 20, 2400, 90, s * 55 + 40, "ST_Stair", name="grand_stair")


def end_pavilion(cx):
    x0, x1 = cx - 1500, cx + 1500
    y0, y1 = -2600, 2600
    for (yy) in (y0, y1):
        box((x0 + x1) / 2, yy, GAL_H * 0.55, (x1 - x0), T, GAL_H * 1.1, "ST_Wall", name="end_wall")
    for xx in (x0, x1):
        box(xx, (y0 + y1) / 2, GAL_H * 0.55, T, (y1 - y0), GAL_H * 1.1, "ST_Wall", name="end_wall")
    box((x0 + x1) / 2, (y0 + y1) / 2, GAL_H * 1.1 + 130, (x1 - x0) + 60, (y1 - y0) + 60, 260, "ST_Roof", name="end_roof")
    slab(x0, x1, y0, y1, 2, "ST_Slab", name="end_floor")


def build():
    mats()
    if EAL.does_asset_exist(MAP):
        EAL.delete_asset(MAP)
        log("헌 맵 삭제")
    log("new_level -> %s" % les.new_level(MAP))

    # 부지 지반(전정~정원 전체)
    slab(-13800, 13800, FC_Y0 - 500, GD_Y1 + 500, 0, "ST_Court", name="Ground")

    grand_gallery()
    central_pavilion()
    end_pavilion(GAL_X0)
    end_pavilion(GAL_X1)

    # ── 전정(코트) — 담 + 대문 + 큰 분수 ──
    for xx in (GAL_X0, GAL_X1):
        box(xx, (FC_Y0 + FC_Y1) / 2, 400, T + 40, (FC_Y1 - FC_Y0), 800, "ST_Court", name="court_wall")
    # 앞담 + 대문 개구부
    for (a, b) in ((GAL_X0, -1600), (1600, GAL_X1)):
        box((a + b) / 2, FC_Y0, 500, (b - a), T + 40, 1000, "ST_Court", name="court_front")
    for gx in (-1600, 1600):
        box(gx, FC_Y0, 700, 240, 240, 1400, "ST_Wall", name="gate_pier")
    # 큰 분수
    fx, fy = 0.0, (FC_Y0 + FC_Y1) / 2 - 1000
    cyl(fx, fy, 40, 1400, 80, "ST_Court", name="fountain_base")
    cyl(fx, fy, 90, 1100, 70, "ST_Water", name="fountain_water")
    cyl(fx, fy, 320, 140, 560, "ST_Court", name="fountain_stem")

    # ── 뒤 정원(대형 파르테르: 산울타리 격자) ──
    gy = GD_Y0 + 1200
    while gy < GD_Y1:
        for gx in range(-9000, 9001, 2400):
            box(gx, gy, 130, 1400, 300, 260, "ST_Garden", name="hedge")
        gy += 2600

    # ── 사람 1.8m 마커 — 회랑 길이 따라 여러 개(스케일이 극적으로 읽히게) ──
    for mx in range(-9000, 9001, 3000):
        marker(mx, 0, "마커_회랑")
    marker(0, FC_Y0 + 600, "마커_대문")
    marker(0, -CEN_HW, "마커_대현관")

    # ── 밝은 대낮 ──
    d = acts.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 6000), unreal.Rotator(-50, -35, 0))
    dc = d.get_component_by_class(unreal.DirectionalLightComponent)
    dc.set_editor_property("intensity", 4.0); dc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    dc.set_editor_property("atmosphere_sun_light", True)
    acts.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    sk = acts.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 3000))
    skc = sk.get_component_by_class(unreal.SkyLightComponent)
    skc.set_editor_property("real_time_capture", True); skc.set_editor_property("intensity", 2.4)
    skc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    pp = acts.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 1200))
    pp.set_editor_property("unbound", True)
    st = pp.get_editor_property("settings")
    st.set_editor_property("override_auto_exposure_method", True)
    st.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    st.set_editor_property("override_auto_exposure_min_brightness", True); st.set_editor_property("auto_exposure_min_brightness", 0.6)
    st.set_editor_property("override_auto_exposure_max_brightness", True); st.set_editor_property("auto_exposure_max_brightness", 1.4)
    pp.set_editor_property("settings", st)

    # PlayerStart — 회랑 한쪽 끝(스케일 체험)
    acts.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(GAL_X0 + 800, 0, 170))
    log("save -> %s · 액터 %d" % (les.save_current_level(), CNT["n"]))


open(LOG, "w", encoding="utf-8").close()
try:
    build()
    log("=== 궁전 뼈대 완료 ===")
except Exception:
    log("실패\n" + traceback.format_exc())
with open(LOG, "w", encoding="utf-8") as f:
    f.write("\n".join(_ll))
