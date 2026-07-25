# -*- coding: utf-8 -*-
# ▶ 네르한 저택 — 전체 부지 뼈대(블록아웃). "한 컷(홀 하나)" 아니라 대저택 전체 크기를 먼저.
#   사용자 2026-07-25: 대저택 레벨은 단순 뼈대라도 전체적인 크기를 그대로 뽑아 보여줄 것.
#   구성: 앞 전정(forecourt+문+분수) → 본관(현관홀·대연회장[3층 오픈]·대계단) → 좌우 양익(4층) → 뒤 정원.
#   실제 치수(cm). 사람 크기 마커(1.8m)로 스케일이 읽히게. 엔진 프리미티브 + 단색(뼈대라 텍스처 없음).
#   실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=_build_manor_estate.py
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
    _ll.append(str(s)); unreal.log("[estate] " + str(s))


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
    mat("ST_Wall",   (0.64, 0.61, 0.55))   # 석벽(따뜻한 회)
    mat("ST_Slab",   (0.80, 0.79, 0.76))   # 바닥판(밝은 돌)
    mat("ST_Hall",   (0.32, 0.46, 0.64))   # 대연회장(청 강조)
    mat("ST_Stair",  (0.58, 0.44, 0.30))   # 대계단(목)
    mat("ST_Court",  (0.46, 0.47, 0.49))   # 전정 바닥/담
    mat("ST_Garden", (0.28, 0.46, 0.26))   # 정원 산울타리
    mat("ST_Water",  (0.20, 0.42, 0.62), rough=0.1)  # 분수
    mat("ST_Roof",   (0.34, 0.30, 0.30))   # 양익 지붕
    mat("ST_Mark",   (0.90, 0.14, 0.10), emissive=(0.5, 0.05, 0.03))  # 사람 크기 마커(1.8m)


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


T = 30.0        # 벽 두께
FH = 480.0      # 층 높이 4.8m
DOORH = 300.0   # 문 높이


def slab(x0, x1, y0, y1, z, m, name="slab"):
    box((x0 + x1) / 2, (y0 + y1) / 2, z - 10, (x1 - x0), (y1 - y0), 20, m, name=name)


def wallX(x0, x1, y, z0, z1, m, door=None, name="wall"):
    """X방향 벽(Y 고정). door=(중심x, 폭)이면 그 구간을 문(위에 상인방)으로 비운다."""
    if door:
        dc, dw = door
        for (a, b) in ((x0, dc - dw / 2), (dc + dw / 2, x1)):
            if b > a:
                box((a + b) / 2, y, (z0 + z1) / 2, (b - a), T, (z1 - z0), m, name=name)
        if z1 > z0 + DOORH:      # 문 위 상인방
            box(dc, y, (z0 + DOORH + z1) / 2, dw, T, (z1 - (z0 + DOORH)), m, name=name)
    else:
        box((x0 + x1) / 2, y, (z0 + z1) / 2, (x1 - x0), T, (z1 - z0), m, name=name)


def wallY(y0, y1, x, z0, z1, m, door=None, name="wall"):
    if door:
        dc, dw = door
        for (a, b) in ((y0, dc - dw / 2), (dc + dw / 2, y1)):
            if b > a:
                box(x, (a + b) / 2, (z0 + z1) / 2, T, (b - a), (z1 - z0), m, name=name)
        if z1 > z0 + DOORH:
            box(x, dc, (z0 + DOORH + z1) / 2, T, dw, (z1 - (z0 + DOORH)), m, name=name)
    else:
        box(x, (y0 + y1) / 2, (z0 + z1) / 2, T, (y1 - y0), (z1 - z0), m, name=name)


def marker(x, y, name="사람1.8m"):
    box(x, y, 90, 55, 30, 180, "ST_Mark", name=name)   # 1.8m 사람 크기 기준


# ═══ 부지 좌표(cm) ═══
# 본관: X[-3000..3000], Y[-2600..1800]  (60m x 44m)
# 양익: 서 X[-5400..-3000], 동 X[3000..5400], Y[-2600..2600] (24m x 52m, 4층)
# 전정: X[-3000..3000], Y[-6200..-2600] (60m x 36m)  · 문=앞 Y=-6200
# 정원: X[-3400..3400], Y[1800..4200] (뒤)
MB_X0, MB_X1 = -3000.0, 3000.0
MB_Y0, MB_Y1 = -2600.0, 1800.0
WW_X0, WW_X1 = -5400.0, -3000.0
EW_X0, EW_X1 = 3000.0, 5400.0
WG_Y0, WG_Y1 = -2600.0, 2600.0
FLOORS = 4
TOP = FH * FLOORS          # 1920 = 19.2m
PAR = 220.0                # 파라펫


def exterior_shell(x0, x1, y0, y1, z_top, m, front_door=None):
    """네 벽을 z0..z_top 로 세운다(파라펫 포함은 호출측). front_door=(cx,w) 면 앞벽(y0)에 문."""
    wallX(x0, x1, y0, 0, z_top, m, door=front_door, name="ext")
    wallX(x0, x1, y1, 0, z_top, m, name="ext")
    wallY(y0, y1, x0, 0, z_top, m, name="ext")
    wallY(y0, y1, x1, 0, z_top, m, name="ext")


def build():
    mats()
    if EAL.does_asset_exist(MAP):
        EAL.delete_asset(MAP)
        log("헌 맵 삭제")
    log("new_level -> %s" % les.new_level(MAP))

    # ── 부지 지반(전정~정원 전체) ──
    slab(WW_X0 - 200, EW_X1 + 200, -6400, 4400, 0, "ST_Court", name="Ground")

    # ── 본관 외벽(4층) + 파라펫. 앞벽에 대현관 문 ──
    exterior_shell(MB_X0, MB_X1, MB_Y0, MB_Y1, TOP, "ST_Wall", front_door=(0, 900))
    box((MB_X0 + MB_X1) / 2, MB_Y0, TOP + PAR / 2, (MB_X1 - MB_X0) + 60, T + 20, PAR, "ST_Wall", name="parapet")
    box((MB_X0 + MB_X1) / 2, MB_Y1, TOP + PAR / 2, (MB_X1 - MB_X0) + 60, T + 20, PAR, "ST_Wall", name="parapet")

    # ── 본관 1층 방 나눔(현관홀 / 대연회장 / 좌우 곁방 / 뒤 계단홀) ──
    # 가로 칸막이: 현관홀|연회장 (Y=-1400), 연회장|계단홀 (Y=600)
    HALL_X0, HALL_X1 = -1400.0, 1400.0
    wallX(MB_X0, MB_X1, -1400, 0, FH, "ST_Wall", door=(0, 700), name="p_front")
    wallX(MB_X0, MB_X1, 600, 0, FH, "ST_Wall", door=(0, 700), name="p_rear")
    # 세로 칸막이로 대연회장을 가운데 방으로(좌우에 곁방 복도)
    wallY(-1400, 600, HALL_X0, 0, FH, "ST_Wall", door=(-400, 500), name="p_hallW")
    wallY(-1400, 600, HALL_X1, 0, FH, "ST_Wall", door=(-400, 500), name="p_hallE")
    # 좌우 곁방(식당·서재 등) 세분 — 곁 구역을 2칸씩
    for xx in (MB_X0, HALL_X1):
        wallX(xx, xx + (HALL_X0 - MB_X0), -400, 0, FH, "ST_Wall", door=(xx + 400, 400), name="p_side")
    # ── 대연회장 = 3층 오픈(바닥 강조색, 위층 슬래브 없음) ──
    slab(HALL_X0, HALL_X1, -1400, 600, 2, "ST_Hall", name="HallFloor")

    # ── 본관 위층 슬래브 — 대연회장 오픈 부분만 빼고(가장자리·앞뒤 방은 층이 있다) ──
    for f in range(1, FLOORS):
        z = FH * f
        # 앞(현관홀 위)·뒤(계단홀 위) 는 층 있음
        slab(MB_X0, MB_X1, MB_Y0, -1400, z, "ST_Slab", name="MB_slabFront")
        slab(MB_X0, MB_X1, 600, MB_Y1, z, "ST_Slab", name="MB_slabRear")
        # 연회장 좌우 곁방 위도 층 있음(가운데 홀만 오픈)
        slab(MB_X0, HALL_X0, -1400, 600, z, "ST_Slab", name="MB_slabW")
        slab(HALL_X1, MB_X1, -1400, 600, z, "ST_Slab", name="MB_slabE")
        if f >= 3:   # 3층 위에서 홀 천장을 덮는다
            slab(HALL_X0, HALL_X1, -1400, 600, z, "ST_Slab", name="HallCeil")

    # ── 대계단(뒤 계단홀, 1→2층) ──
    steps = 12
    for s in range(steps):
        box(0, 900 + s * 70, s * 40 + 20, 1000, 70, s * 40 + 40, "ST_Stair", name="stair")
    box(0, 900 + steps * 70 + 200, FH - 20, 1200, 400, 40, "ST_Slab", name="landing")

    # ── 양익(서·동) 4층 + 지붕 + 복도·방 나눔 ──
    for (X0, X1, tag) in ((WW_X0, WW_X1, "W"), (EW_X0, EW_X1, "E")):
        exterior_shell(X0, X1, WG_Y0, WG_Y1, TOP, "ST_Wall")
        # 본관과 잇는 문(안쪽 벽에 개구부는 exterior가 이미 세움 → 안쪽 벽 하나 더해 복도 연결 생략, 뼈대)
        cx = (X0 + X1) / 2
        for f in range(1, FLOORS):     # 층 슬래브
            slab(X0, X1, WG_Y0, WG_Y1, FH * f, "ST_Slab", name="wing_slab")
        # 지붕
        box(cx, 0, TOP + 120, (X1 - X0) + 40, (WG_Y1 - WG_Y0) + 40, 220, "ST_Roof", name="wing_roof")
        # 1층 방 나눔 — 가운데 복도 + 양쪽 방 4칸씩
        wallY(WG_Y0, WG_Y1, cx - 300, 0, FH, "ST_Wall", door=(0, 500), name="wing_corrW")
        wallY(WG_Y0, WG_Y1, cx + 300, 0, FH, "ST_Wall", door=(0, 500), name="wing_corrE")
        for i in range(1, 5):
            yy = WG_Y0 + (WG_Y1 - WG_Y0) * i / 5.0
            wallX(X0, cx - 300, yy, 0, FH, "ST_Wall", door=(( X0 + cx - 300) / 2, 380), name="wing_room")
            wallX(cx + 300, X1, yy, 0, FH, "ST_Wall", door=((cx + 300 + X1) / 2, 380), name="wing_room")

    # ── 앞 전정(forecourt): 담 + 문 + 분수 ──
    FC_Y0, FC_Y1 = -6200.0, -2600.0
    # 담(낮게 3m) — 좌우 + 앞(문 개구부)
    wallY(FC_Y0, FC_Y1, MB_X0, 0, 300, "ST_Court", name="court_wallW")
    wallY(FC_Y0, FC_Y1, MB_X1, 0, 300, "ST_Court", name="court_wallE")
    wallX(MB_X0, MB_X1, FC_Y0, 0, 420, "ST_Court", door=(0, 800), name="court_front")
    # 문 기둥
    for gx in (-400, 400):
        box(gx, FC_Y0, 300, 90, 90, 600, "ST_Wall", name="gate_post")
    # 분수(가운데)
    fx, fy = 0.0, (FC_Y0 + FC_Y1) / 2
    cyl(fx, fy, 40, 500, 80, "ST_Court", name="fountain_base")
    cyl(fx, fy, 90, 380, 60, "ST_Water", name="fountain_water")
    cyl(fx, fy, 220, 60, 300, "ST_Court", name="fountain_stem")

    # ── 뒤 정원(산울타리 몇 줄 + 낮은 단) ──
    for gy in (2200, 3000, 3800):
        for gx in range(-2400, 2401, 800):
            box(gx, gy, 90, 500, 180, 180, "ST_Garden", name="hedge")

    # ── 사람 크기 마커(1.8m) — 문·현관·연회장·양익에서 스케일이 읽히게 ──
    marker(0, FC_Y0 + 300, "마커_문")
    marker(0, MB_Y0 + 300, "마커_현관")
    marker(0, -400, "마커_연회장")
    marker(-4200, 0, "마커_서익")
    marker(4200, 0, "마커_동익")

    # ── 밝은 대낮(뼈대가 또렷이 보이게) ──
    d = acts.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 5000), unreal.Rotator(-50, -35, 0))
    dc = d.get_component_by_class(unreal.DirectionalLightComponent)
    dc.set_editor_property("intensity", 4.0); dc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    dc.set_editor_property("atmosphere_sun_light", True)
    acts.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    sk = acts.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 3000))
    skc = sk.get_component_by_class(unreal.SkyLightComponent)
    skc.set_editor_property("real_time_capture", True); skc.set_editor_property("intensity", 2.4)
    skc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    pp = acts.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 1000))
    pp.set_editor_property("unbound", True)
    st = pp.get_editor_property("settings")
    st.set_editor_property("override_auto_exposure_method", True)
    st.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    st.set_editor_property("override_auto_exposure_min_brightness", True); st.set_editor_property("auto_exposure_min_brightness", 0.6)
    st.set_editor_property("override_auto_exposure_max_brightness", True); st.set_editor_property("auto_exposure_max_brightness", 1.4)
    pp.set_editor_property("settings", st)

    acts.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, FC_Y0 + 400, 160))
    log("save -> %s · 액터 %d" % (les.save_current_level(), CNT["n"]))


open(LOG, "w", encoding="utf-8").close()
try:
    build()
    log("=== 저택 뼈대 완료 ===")
except Exception:
    log("실패\n" + traceback.format_exc())
with open(LOG, "w", encoding="utf-8") as f:
    f.write("\n".join(_ll))
