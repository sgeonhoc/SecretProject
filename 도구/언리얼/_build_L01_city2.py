# -*- coding: utf-8 -*-
# ▶ L01 아랫장터 큰길 — 도시스케일 재건 (2026-07-25, 저택 파이프라인 이식)
#   교훈 반영: ①민짜색 금지→우리 절차텍스처(T_Rasel_*)를 월드평면 UV로 물림 ②"칸에서 잘린" 치명 금지→
#   근경건물 뒤에 중경·원경 건물열 + 강 건너 유리탑 스카이라인 + 대기안개로 '도시 안'을 보임 ③도시 스케일
#   (긴 대로 ~280m) ④통일톤(맑고 살짝 쿨한 대낮, 높은 채도). ⑤★new_level 전 delete_asset(저장 안 되는 버그).
#   엔진 프리미티브 + 우리 텍스처만(외부 에셋 0). 실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=_build_L01_city2.py
import unreal, math, random, traceback

MAP    = "/Game/Maps/Rasel/L01_Jangteo_Street"
MATDIR = "/Game/Rasel/Materials"
TEXDIR = "/Game/Rasel/Textures"
LOG    = "C:/Secret_Project/Saved/l01.log"
CUBE = "/Engine/BasicShapes/Cube.Cube"
CYL  = "/Engine/BasicShapes/Cylinder.Cylinder"

EAL  = unreal.EditorAssetLibrary
MEL  = unreal.MaterialEditingLibrary
AT   = unreal.AssetToolsHelpers.get_asset_tools()
les  = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
acts = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MATS, _m, CNT = {}, {}, {"n": 0}
_ll = []


def log(s):
    _ll.append(str(s)); unreal.log("[L01] " + str(s))


def flush():
    with open(LOG, "w", encoding="utf-8") as f:
        f.write("\n".join(_ll))


def tex(name):
    p = "%s/%s" % (TEXDIR, name)
    return EAL.load_asset(p) if EAL.does_asset_exist(p) else None


def _fresh(name):
    p = "%s/%s" % (MATDIR, name)
    if EAL.does_asset_exist(p):
        EAL.delete_asset(p)
    return AT.create_asset(name, MATDIR, unreal.Material, unreal.MaterialFactoryNew())


def _world_uv(mat, proj, tiling_cm):
    wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1300, 700)
    mask = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1080, 700)
    r = proj in ("xy", "xz"); g = proj in ("xy", "yz"); b = proj in ("xz", "yz")
    mask.set_editor_property("r", r); mask.set_editor_property("g", g)
    mask.set_editor_property("b", b); mask.set_editor_property("a", False)
    MEL.connect_material_expressions(wp, "", mask, "")
    div = MEL.create_material_expression(mat, unreal.MaterialExpressionDivide, -900, 700)
    c = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -1080, 830)
    c.set_editor_property("r", float(tiling_cm))
    MEL.connect_material_expressions(mask, "", div, "A")
    MEL.connect_material_expressions(c, "", div, "B")
    return div


def tex_mat(name, base, normal=None, rough=None, proj="xz", tiling_cm=400.0,
            tint=None, rough_lo=0.5, rough_hi=0.95, spec=0.5, metal=0.0, emissive=None):
    mat = _fresh(name)
    uv = _world_uv(mat, proj, tiling_cm)
    bt = tex(base)
    if bt is None:
        log("  !! %s 텍스처 없음 %s" % (name, base)); return None
    bs = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, -200)
    bs.set_editor_property("texture", bt)
    MEL.connect_material_expressions(uv, "", bs, "UVs")
    out, pin = bs, "RGB"
    if tint is not None:
        mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -320, -200)
        cc = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -520, -60)
        cc.set_editor_property("constant", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0))
        MEL.connect_material_expressions(bs, "RGB", mul, "A")
        MEL.connect_material_expressions(cc, "", mul, "B")
        out, pin = mul, ""
    MEL.connect_material_property(out, pin, unreal.MaterialProperty.MP_BASE_COLOR)
    nt = tex(normal) if normal else None
    if nt is not None:
        ns = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, 120)
        ns.set_editor_property("texture", nt)
        ns.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        MEL.connect_material_expressions(uv, "", ns, "UVs")
        MEL.connect_material_property(ns, "RGB", unreal.MaterialProperty.MP_NORMAL)
    rt = tex(rough) if rough else None
    if rt is not None:
        rs = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, 420)
        rs.set_editor_property("texture", rt)
        stt = getattr(unreal.MaterialSamplerType, "SAMPLERTYPE_MASKS", None)
        if stt is not None:
            rs.set_editor_property("sampler_type", stt)
        MEL.connect_material_expressions(uv, "", rs, "UVs")
        lp = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -300, 420)
        a = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -470, 560); a.set_editor_property("r", rough_lo)
        b2 = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -470, 620); b2.set_editor_property("r", rough_hi)
        MEL.connect_material_expressions(a, "", lp, "A")
        MEL.connect_material_expressions(b2, "", lp, "B")
        MEL.connect_material_expressions(rs, "R", lp, "Alpha")
        MEL.connect_material_property(lp, "", unreal.MaterialProperty.MP_ROUGHNESS)
    else:
        rc = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 420); rc.set_editor_property("r", rough_hi)
        MEL.connect_material_property(rc, "", unreal.MaterialProperty.MP_ROUGHNESS)
    sc = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 560); sc.set_editor_property("r", spec)
    MEL.connect_material_property(sc, "", unreal.MaterialProperty.MP_SPECULAR)
    if metal > 0.0:
        mc = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 640); mc.set_editor_property("r", metal)
        MEL.connect_material_property(mc, "", unreal.MaterialProperty.MP_METALLIC)
    if emissive is not None:
        e = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -300, 720)
        e.set_editor_property("constant", unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0))
        MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    try:
        MEL.recompile_material(mat)
    except Exception:
        pass
    EAL.save_asset("%s/%s" % (MATDIR, name), False)
    MATS[name] = mat
    return mat


def flat_mat(name, rgb, rough=0.7, metal=0.0, emissive=None, spec=0.5):
    mat = _fresh(name)
    c = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    c.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200); r.set_editor_property("r", rough)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if metal > 0:
        mm = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 300); mm.set_editor_property("r", metal)
        MEL.connect_material_property(mm, "", unreal.MaterialProperty.MP_METALLIC)
    s = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 360); s.set_editor_property("r", spec)
    MEL.connect_material_property(s, "", unreal.MaterialProperty.MP_SPECULAR)
    if emissive is not None:
        e = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 440)
        e.set_editor_property("constant", unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0))
        MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    try:
        MEL.recompile_material(mat)
    except Exception:
        pass
    EAL.save_asset("%s/%s" % (MATDIR, name), False)
    MATS[name] = mat
    return mat


def build_materials():
    log("[머티리얼]")
    for proj in ("xz", "yz"):
        s = proj[0].upper()
        tex_mat("L01_Plaster_" + s, "T_Rasel_Plaster_C", "T_Rasel_Plaster_N", "T_Rasel_Plaster_R",
                proj=proj, tiling_cm=380, rough_lo=0.6, rough_hi=1.0)
        tex_mat("L01_Brick_" + s, "T_Rasel_OldStone_C", "T_Rasel_OldStone_N", "T_Rasel_OldStone_R",
                proj=proj, tiling_cm=300, rough_lo=0.6, rough_hi=1.0, tint=(1.05, 0.9, 0.82))
    tex_mat("L01_Cobble", "T_Rasel_Cobble_C", "T_Rasel_Cobble_N", "T_Rasel_Cobble_R",
            proj="xy", tiling_cm=320, rough_lo=0.5, rough_hi=0.95)
    tex_mat("L01_Roof", "T_Rasel_Roof_C", "T_Rasel_Roof_N", "T_Rasel_Roof_R",
            proj="xz", tiling_cm=260, rough_lo=0.4, rough_hi=0.9)
    tex_mat("L01_Wood", "T_Rasel_Wood_C", "T_Rasel_Wood_N", "T_Rasel_Wood_R",
            proj="xz", tiling_cm=180, rough_lo=0.5, rough_hi=0.9)
    flat_mat("L01_WinDark", (0.03, 0.05, 0.08), rough=0.15, spec=0.9)
    flat_mat("L01_WinLit", (0.9, 0.72, 0.42), rough=0.3, emissive=(0.9, 0.6, 0.3))
    flat_mat("L01_Trim", (0.86, 0.86, 0.82), rough=0.6)
    flat_mat("L01_Metal", (0.10, 0.11, 0.12), rough=0.4, metal=0.8)
    flat_mat("L01_MidBldg", (0.34, 0.44, 0.60), rough=0.85)
    flat_mat("L01_FarBldg", (0.40, 0.52, 0.72), rough=0.9)
    flat_mat("L01_Tower", (0.30, 0.52, 0.78), rough=0.12, spec=1.0, emissive=(0.10, 0.22, 0.40))
    flat_mat("L01_TowerLit", (0.55, 0.75, 0.95), rough=0.2, emissive=(0.3, 0.55, 0.9))
    flat_mat("L01_River", (0.06, 0.20, 0.34), rough=0.05, spec=1.0)
    flat_mat("L01_Cloth_R", (0.78, 0.16, 0.13), rough=0.85)
    flat_mat("L01_Cloth_T", (0.10, 0.50, 0.55), rough=0.85)
    flat_mat("L01_Cloth_O", (0.92, 0.48, 0.10), rough=0.85)
    log("  머티리얼 %d" % len(MATS))


def mesh(p):
    if p not in _m:
        _m[p] = EAL.load_asset(p)
    return _m[p]


def box(x, y, z, sx, sy, sz, mat, yaw=0.0, pitch=0.0, name=None):
    a = acts.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(pitch, yaw, 0))
    a.static_mesh_component.set_static_mesh(mesh(CUBE))
    a.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    a.static_mesh_component.set_material(0, MATS[mat])
    a.set_mobility(unreal.ComponentMobility.STATIC)
    if name:
        a.set_actor_label(name)
    CNT["n"] += 1
    return a


def cyl(x, y, z, rad, h, mat, name=None):
    a = acts.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z))
    a.static_mesh_component.set_static_mesh(mesh(CYL))
    a.set_actor_scale3d(unreal.Vector(rad / 50.0, rad / 50.0, h / 100.0))
    a.static_mesh_component.set_material(0, MATS[mat])
    a.set_mobility(unreal.ComponentMobility.STATIC)
    if name:
        a.set_actor_label(name)
    CNT["n"] += 1
    return a


def plight(x, y, z, color, inten, rad):
    a = acts.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, z))
    c = a.get_component_by_class(unreal.PointLightComponent)
    c.set_editor_property("intensity", inten)
    c.set_editor_property("light_color", unreal.Color(color[0], color[1], color[2], 255))
    c.set_editor_property("attenuation_radius", rad)
    c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    CNT["n"] += 1
    return a


ROAD_HALF = 900.0
WALK_Y = 1250.0
STORY = 380.0
STREET_X0, STREET_X1 = -14000.0, 14000.0
CLOTHS = ["L01_Cloth_R", "L01_Cloth_T", "L01_Cloth_O"]
WALLS_XZ = ["L01_Plaster_X", "L01_Brick_X"]


def building(cx, sgn, w, depth, stories, rng):
    h = stories * STORY
    fy = sgn * (WALK_Y + depth / 2.0)
    wall = rng.choice(WALLS_XZ)
    box(cx, fy, h / 2.0, w, depth, h, wall, name="bldg")
    box(cx, fy, h + 20, w + 30, depth + 30, 60, "L01_Trim", name="parapet")
    box(cx, fy, h - 30, w + 40, depth + 10, 40, "L01_Trim", name="cornice")
    face = fy - sgn * (depth / 2.0 + 6)
    nwin = max(2, int(w / 360))
    for s in range(stories):
        zc = s * STORY
        if s > 0:
            box(cx, face, zc, w + 20, 20, 24, "L01_Trim", name="stringcourse")
        for k in range(nwin):
            wx = cx - w / 2.0 + w * (k + 0.5) / nwin
            wz = zc + STORY * 0.58
            lit = (s > 0) and rng.random() < 0.28
            box(wx, face - sgn * 8, wz, 150, 16, 190, "L01_WinLit" if lit else "L01_WinDark", name="win")
            box(wx, face, wz, 190, 22, 230, "L01_Trim", name="winframe")
    if rng.random() < 0.7:
        box(cx, fy - sgn * (depth / 2.0 + 120), 300, min(w * 0.8, 420), 260, 16,
            rng.choice(CLOTHS), pitch=sgn * 9, name="awning")
    if rng.random() < 0.5:
        sx = cx - w / 2.0 + w * rng.uniform(0.2, 0.8)
        box(sx, face - sgn * 20, STORY * 1.7, 90, 14, rng.uniform(300, 460),
            rng.choice(CLOTHS), name="banner")


# ── 거리 살림(좌판·소품·등불 줄) — "사람 사는 저잣거리" ──────────────
def stall(x, sgn, rng):
    y = sgn * (ROAD_HALF - 160)
    # 나무 상 + 다리
    box(x, y, 95, 340, 220, 24, "L01_Wood", name="stall")
    for dx in (-150, 150):
        for dy in (-90, 90):
            cyl(x + dx, y + dy, 47, 18, 94, "L01_Wood", name="stall_leg")
    # 차양(도로 쪽으로 기움)
    box(x, y - sgn * 40, 300, 420, 300, 14, rng.choice(CLOTHS), pitch=sgn * 8, name="stall_awn")
    # 좌판 위 물건(궤짝·바구니)
    for _ in range(rng.randint(2, 4)):
        gx = x + rng.uniform(-130, 130); gy = y + rng.uniform(-70, 70)
        s = rng.uniform(60, 95)
        box(gx, gy, 130, s, s, s, "L01_Wood", yaw=rng.uniform(0, 40), name="goods")


def props(x, sgn, rng):
    y = sgn * (ROAD_HALF + 40)
    pick = rng.random()
    if pick < 0.35:
        box(x, y, 55, 90, 90, 90, "L01_Wood", yaw=rng.uniform(0, 40), name="crate")
        box(x + rng.uniform(60, 110), y, 45, 70, 70, 70, "L01_Wood", name="crate")
    elif pick < 0.6:
        cyl(x, y, 60, 55, 120, "L01_Wood", name="barrel")
    elif pick < 0.8:
        cyl(x, y, 35, 42, 70, "L01_Cloth_O", name="pot")
        cyl(x, y, 95, 60, 60, "L01_Cloth_T", name="plant")
    else:
        box(x, y, 40, 90, 70, 70, rng.choice(CLOTHS), yaw=rng.uniform(0, 60), name="sack")


def lantern_string(x0, x1, z, rng):
    box((x0 + x1) / 2.0, 0, z, abs(x1 - x0), 6, 4, "L01_Metal", name="wire")
    n = max(3, int(abs(x1 - x0) / 220))
    for i in range(n):
        lx = x0 + (x1 - x0) * (i + 0.5) / n
        cyl(lx, rng.uniform(-300, 300), z - 45, 24, 40, "L01_WinLit", name="slantern")


def street_life(rng):
    # 좌판 — 양쪽 인도, 성긴 간격
    x = STREET_X0 + 2000
    while x < STREET_X1 - 2000:
        for sgn in (1, -1):
            if rng.random() < 0.8:
                stall(x + rng.uniform(-200, 200), sgn, rng)
        x += rng.uniform(1800, 2800)
    # 건물 밑동 소품
    x = STREET_X0 + 900
    while x < STREET_X1 - 900:
        for sgn in (1, -1):
            if rng.random() < 0.55:
                props(x + rng.uniform(-120, 120), sgn, rng)
        x += rng.uniform(700, 1200)
    # 처마 등불 줄 — 길을 가로질러 위쪽
    x = STREET_X0 + 2500
    while x < STREET_X1 - 2500:
        lantern_string(x, x + rng.uniform(900, 1400), rng.uniform(700, 950), rng)
        x += rng.uniform(2600, 4200)


def build():
    build_materials()
    if EAL.does_asset_exist(MAP):
        EAL.delete_asset(MAP)
        log("[레벨] 헌 맵 삭제")
    log("[레벨] new_level -> %s" % les.new_level(MAP))
    rng = random.Random(101)

    midx = (STREET_X0 + STREET_X1) / 2.0
    L = STREET_X1 - STREET_X0
    box(midx, 0, -20, L + 800, ROAD_HALF * 2, 40, "L01_Cobble", name="Road")
    for sgn in (1, -1):
        box(midx, sgn * (ROAD_HALF + 175), -8, L + 800, 350, 44, "L01_Cobble", name="Sidewalk")

    x = STREET_X0 + 600
    while x < STREET_X1 - 600:
        w = rng.uniform(700, 1300)
        if rng.random() < 0.12:
            box(x + w / 2, WALK_Y + 900, 300, w * 0.5, 1400, 700, "L01_MidBldg", name="alleyback")
            box(x + w / 2, -(WALK_Y + 900), 300, w * 0.5, 1400, 700, "L01_MidBldg", name="alleyback")
            x += w + 500
            continue
        for sgn in (1, -1):
            stories = rng.choice([3, 4, 4, 5, 6])
            building(x + w / 2, sgn, w, rng.uniform(500, 700), stories, rng)
        x += w + rng.uniform(120, 300)

    for sgn in (1, -1):
        x = STREET_X0
        while x < STREET_X1:
            w = rng.uniform(900, 1600); hh = rng.uniform(1800, 3200)
            box(x + w / 2, sgn * (WALK_Y + 1400), hh / 2, w, 700, hh, "L01_MidBldg", name="midbldg")
            x += w + rng.uniform(200, 500)

    for sgn in (1, -1):
        x = STREET_X0 - 3000
        while x < STREET_X1 + 3000:
            w = rng.uniform(1400, 2600); hh = rng.uniform(3000, 6000)
            box(x + w / 2, sgn * (WALK_Y + 4200), hh / 2, w, 900, hh, "L01_FarBldg", name="farbldg")
            x += w + rng.uniform(300, 700)

    def skyline(base_x, base_y, count, seed):
        r = random.Random(seed)
        box(base_x, base_y, -30, 9000, 6000, 20, "L01_River", name="River")
        for i in range(count):
            tx = base_x - 4000 + r.uniform(0, 8000)
            ty = base_y + r.uniform(-2000, 2000)
            tw = r.uniform(600, 1200); th = r.uniform(4000, 11000)
            box(tx, ty, th / 2, tw, tw, th, "L01_Tower", name="Tower")
            for zz in range(3):
                box(tx, ty - tw / 2 - 6, th * (0.3 + 0.2 * zz), tw * 0.8, 12, 120, "L01_TowerLit", name="towerwin")
    skyline(STREET_X1 + 9000, 0, 16, 7)
    skyline(STREET_X0 - 9000, 0, 10, 9)
    skyline(0, WALK_Y + 12000, 14, 11)

    x = STREET_X0 + 1500
    while x < STREET_X1 - 1500:
        for sgn in (1, -1):
            cyl(x, sgn * (WALK_Y - 250), 320, 30, 90, "L01_Metal", name="lamppost")
            cyl(x, sgn * (WALK_Y - 250), 640, 46, 60, "L01_WinLit", name="lamp")
        x += rng.uniform(1600, 2400)

    street_life(rng)   # 좌판·소품·등불 줄 = 사람 사는 저잣거리

    d = acts.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 4000), unreal.Rotator(-52, -25, 0))
    dc = d.get_component_by_class(unreal.DirectionalLightComponent)
    dc.set_editor_property("intensity", 4.2)
    dc.set_editor_property("light_color", unreal.Color(255, 246, 232))
    dc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    dc.set_editor_property("atmosphere_sun_light", True)
    acts.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    sk = acts.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 2500))
    skc = sk.get_component_by_class(unreal.SkyLightComponent)
    skc.set_editor_property("real_time_capture", True)
    skc.set_editor_property("intensity", 3.0)   # 대낮 앰비언트 = 건물 면이 검게 죽지 않게
    skc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    fog = acts.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 200))
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    try:
        fc.set_editor_property("fog_density", 0.0016)
        fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.35, 0.5, 0.75, 1.0))
        fc.set_editor_property("start_distance", 2500.0)
    except Exception as e:
        log("  fog %s" % e)
    pp = acts.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 1000))
    pp.set_editor_property("unbound", True)
    st = pp.get_editor_property("settings")
    st.set_editor_property("override_auto_exposure_method", True)
    st.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    st.set_editor_property("override_auto_exposure_min_brightness", True); st.set_editor_property("auto_exposure_min_brightness", 0.6)
    st.set_editor_property("override_auto_exposure_max_brightness", True); st.set_editor_property("auto_exposure_max_brightness", 1.4)
    st.set_editor_property("override_auto_exposure_bias", True); st.set_editor_property("auto_exposure_bias", 0.3)
    st.set_editor_property("override_bloom_intensity", True); st.set_editor_property("bloom_intensity", 0.3)
    st.set_editor_property("override_color_saturation", True); st.set_editor_property("color_saturation", unreal.Vector4(1.22, 1.22, 1.24, 1.0))
    st.set_editor_property("override_color_contrast", True); st.set_editor_property("color_contrast", unreal.Vector4(1.06, 1.06, 1.08, 1.0))
    pp.set_editor_property("settings", st)

    acts.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(STREET_X0 + 1200, 0, 160))
    log("[레벨] save -> %s · 액터 %d" % (les.save_current_level(), CNT["n"]))


open(LOG, "w", encoding="utf-8").close()
try:
    build()
    log("=== L01 city2 완료 ===")
except Exception:
    log("실패\n" + traceback.format_exc())
flush()
