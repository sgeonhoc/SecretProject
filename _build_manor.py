# -*- coding: utf-8 -*-
# ▶ 네르한 가 대전당 (L15 저택) — 텍스처 + 통일 광 분위기 재건 (2026-07-25)
#   교훈: 민짜 큐브+평평한 색 = 장난감. 이번엔 우리가 구운 절차 텍스처(대리석 결·헤링본 마루·
#   다마스크 벽지·붉은 카펫·금장)를 물리고, 큰 면은 월드평면 UV로 늘어남을 막는다.
#   통일 색감(사용자 2026-07-25): 재료 베이스색은 다양하되, 차가운 새벽/달빛 한 줄기가 홀 전체를
#   씻어 하나의 분위기로 묶는다. 아치창으로 쏟아지는 볼류메트릭 신광 + 따뜻한 샹들리에(웜 악센트).
#   엄청 크고·밝고·탁 트임 유지(복도=도로 수준). 엔진 프리미티브 + 우리 텍스처만(외부 에셋 0).
#   실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=_build_manor.py
import unreal, math, os, traceback

MAP    = "/Game/Maps/Rasel/L15_Nerhan_Manor"
MATDIR = "/Game/Rasel/Materials"
TEXDIR = "/Game/Rasel/Textures"
SRC    = "C:/Secret_Project/_ArtSource/Textures"
LOG    = "C:/Secret_Project/Saved/manor.log"
CUBE = "/Engine/BasicShapes/Cube.Cube"
CYL  = "/Engine/BasicShapes/Cylinder.Cylinder"
SPH  = "/Engine/BasicShapes/Sphere.Sphere"

EAL  = unreal.EditorAssetLibrary
MEL  = unreal.MaterialEditingLibrary
AT   = unreal.AssetToolsHelpers.get_asset_tools()
les  = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
acts = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MATS, _m, CNT = {}, {}, {"n": 0}
_loglines = []


def log(s):
    _loglines.append(str(s))
    unreal.log("[manor] " + str(s))


def flush():
    with open(LOG, "w", encoding="utf-8") as f:
        f.write("\n".join(_loglines))


# ── 텍스처 반입 ────────────────────────────────────────────────────────
MANOR_TEX = [
    ("T_Manor_MarbleW_C", "C"), ("T_Manor_MarbleW_N", "N"), ("T_Manor_MarbleW_R", "R"),
    ("T_Manor_MarbleD_C", "C"), ("T_Manor_MarbleD_N", "N"), ("T_Manor_MarbleD_R", "R"),
    ("T_Manor_Parquet_C", "C"), ("T_Manor_Parquet_N", "N"), ("T_Manor_Parquet_R", "R"),
    ("T_Manor_Damask_C", "C"), ("T_Manor_Damask_N", "N"), ("T_Manor_Damask_R", "R"),
    ("T_Manor_Carpet_C", "C"), ("T_Manor_Carpet_N", "N"), ("T_Manor_Carpet_R", "R"),
    ("T_Manor_Gilt_C", "C"), ("T_Manor_Gilt_N", "N"), ("T_Manor_Gilt_R", "R"),
]


def import_tex():
    try:
        unreal.SystemLibrary.execute_console_command(None, "Editor.AsyncAssetCompilation 0")
    except Exception:
        pass
    tasks = []
    for name, kind in MANOR_TEX:
        p = os.path.join(SRC, name + ".png")
        if not os.path.exists(p):
            log("  !! 원본 없음 %s" % p); continue
        t = unreal.AssetImportTask()
        t.filename = p; t.destination_path = TEXDIR; t.destination_name = name
        t.automated = True; t.replace_existing = True; t.save = True
        tasks.append(t)
    AT.import_asset_tasks(tasks)
    ok = 0
    for name, kind in MANOR_TEX:
        path = "%s/%s" % (TEXDIR, name)
        if not EAL.does_asset_exist(path):
            log("  !! 반입 실패 %s" % name); continue
        tx = EAL.load_asset(path)
        if kind == "N":
            tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tx.set_editor_property("srgb", False)
        elif kind == "R":
            tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tx.set_editor_property("srgb", False)
        else:
            tx.set_editor_property("srgb", True)
        tx.set_editor_property("never_stream", True)
        EAL.save_asset(path, False); ok += 1
    log("[텍스처] %d/%d 반입" % (ok, len(MANOR_TEX)))


def tex(name):
    path = "%s/%s" % (TEXDIR, name)
    return EAL.load_asset(path) if EAL.does_asset_exist(path) else None


# ── 머티리얼 저작 ──────────────────────────────────────────────────────
def _fresh(name):
    path = "%s/%s" % (MATDIR, name)
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    return AT.create_asset(name, MATDIR, unreal.Material, unreal.MaterialFactoryNew())


def _world_uv(mat, proj, tiling_cm):
    """월드평면 UV — 큰 면에서도 텍스처가 늘어나지 않게 월드 좌표로 타일링.
    proj: 'xy'(바닥·천장) / 'xz'(±Y향 벽) / 'yz'(±X향 벽). tiling_cm = 한 반복의 크기(cm)."""
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


def tex_mat(name, base, normal=None, rough=None, proj="xy", tiling_cm=300.0,
            tint=None, rough_lo=0.3, rough_hi=0.9, spec=0.5, metal=0.0, emissive=None):
    """색·결·거칠기 텍스처를 물린 머티리얼(월드평면 UV)."""
    mat = _fresh(name)
    uv = _world_uv(mat, proj, tiling_cm)

    bt = tex(base)
    if bt is None:
        log("  !! %s 색 텍스처 없음 %s" % (name, base)); return None
    bs = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, -200)
    bs.set_editor_property("texture", bt)
    MEL.connect_material_expressions(uv, "", bs, "UVs")
    out, pin = bs, "RGB"
    if tint is not None:
        mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -320, -200)
        c = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -520, -60)
        c.set_editor_property("constant", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0))
        MEL.connect_material_expressions(bs, "RGB", mul, "A")
        MEL.connect_material_expressions(c, "", mul, "B")
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
        st = getattr(unreal.MaterialSamplerType, "SAMPLERTYPE_MASKS", None) \
            or getattr(unreal.MaterialSamplerType, "SAMPLERTYPE_LINEAR_GRAYSCALE", None)
        if st is not None:
            rs.set_editor_property("sampler_type", st)
        MEL.connect_material_expressions(uv, "", rs, "UVs")
        lerp = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -300, 420)
        a = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -470, 560); a.set_editor_property("r", rough_lo)
        b2 = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -470, 620); b2.set_editor_property("r", rough_hi)
        MEL.connect_material_expressions(a, "", lerp, "A")
        MEL.connect_material_expressions(b2, "", lerp, "B")
        MEL.connect_material_expressions(rs, "R", lerp, "Alpha")
        MEL.connect_material_property(lerp, "", unreal.MaterialProperty.MP_ROUGHNESS)
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
        e.set_editor_property("constant", unreal.LinearColor(*emissive, 1.0) if len(emissive) == 3 else unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0))
        MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    try:
        MEL.recompile_material(mat)
    except Exception:
        pass
    EAL.save_asset("%s/%s" % (MATDIR, name), False)
    MATS[name] = mat
    log("  · %s" % name)
    return mat


def flat_mat(name, rgb, rough=0.6, metal=0.0, emissive=None, spec=0.5):
    mat = _fresh(name)
    c = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    c.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200); r.set_editor_property("r", rough)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if metal > 0:
        m = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 300); m.set_editor_property("r", metal)
        MEL.connect_material_property(m, "", unreal.MaterialProperty.MP_METALLIC)
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
    log("  · %s (단색)" % name)
    return mat


def build_materials():
    log("[머티리얼]")
    # 대리석 — 바닥/천장은 xy, 벽/기둥은 벽투영. 광택 강(spec 높고 rough 낮음).
    tex_mat("MN_MarbleW", "T_Manor_MarbleW_C", "T_Manor_MarbleW_N", "T_Manor_MarbleW_R",
            proj="xy", tiling_cm=360.0, rough_lo=0.12, rough_hi=0.35, spec=0.8)
    tex_mat("MN_MarbleW_W", "T_Manor_MarbleW_C", "T_Manor_MarbleW_N", "T_Manor_MarbleW_R",
            proj="xz", tiling_cm=360.0, rough_lo=0.12, rough_hi=0.35, spec=0.8)
    tex_mat("MN_MarbleD", "T_Manor_MarbleD_C", "T_Manor_MarbleD_N", "T_Manor_MarbleD_R",
            proj="xy", tiling_cm=360.0, rough_lo=0.10, rough_hi=0.30, spec=0.85)
    # 마루(바닥 xy)
    tex_mat("MN_Parquet", "T_Manor_Parquet_C", "T_Manor_Parquet_N", "T_Manor_Parquet_R",
            proj="xy", tiling_cm=440.0, rough_lo=0.30, rough_hi=0.62, spec=0.5)
    # 다마스크 벽지 — 두 벽 방향
    tex_mat("MN_Damask_Y", "T_Manor_Damask_C", "T_Manor_Damask_N", "T_Manor_Damask_R",
            proj="xz", tiling_cm=520.0, rough_lo=0.55, rough_hi=0.85, spec=0.4)
    tex_mat("MN_Damask_X", "T_Manor_Damask_C", "T_Manor_Damask_N", "T_Manor_Damask_R",
            proj="yz", tiling_cm=520.0, rough_lo=0.55, rough_hi=0.85, spec=0.4)
    # 카펫(바닥)
    tex_mat("MN_Carpet", "T_Manor_Carpet_C", "T_Manor_Carpet_N", "T_Manor_Carpet_R",
            proj="xy", tiling_cm=320.0, rough_lo=0.85, rough_hi=0.98, spec=0.25)
    # 금장 — 결 있는 금속. 여러 방향에 쓰이므로 xz 기본(몰딩·틀은 대개 벽/수평띠).
    tex_mat("MN_Gilt", "T_Manor_Gilt_C", "T_Manor_Gilt_N", "T_Manor_Gilt_R",
            proj="xz", tiling_cm=180.0, rough_lo=0.22, rough_hi=0.45, spec=0.9, metal=0.85)
    tex_mat("MN_Gilt_F", "T_Manor_Gilt_C", "T_Manor_Gilt_N", "T_Manor_Gilt_R",
            proj="xy", tiling_cm=180.0, rough_lo=0.22, rough_hi=0.45, spec=0.9, metal=0.85)
    # 마호가니 가구 — 거리 나무 텍스처를 짙게 물들여
    tex_mat("MN_Wood", "T_Rasel_Wood_C", "T_Rasel_Wood_N", "T_Rasel_Wood_R",
            proj="xz", tiling_cm=200.0, tint=(0.55, 0.34, 0.24), rough_lo=0.3, rough_hi=0.6, spec=0.6)
    # 결 없는 것
    flat_mat("MN_Teal", (0.06, 0.30, 0.34), rough=0.7, spec=0.4)        # 청록 융단 의자
    flat_mat("MN_Paint", (0.16, 0.11, 0.10), rough=0.55)                 # 액자 유화(짙은 웜톤)
    # 창유리 — 밝은 빛이 쏟아지는 창(쿨). 발광으로 창이 하얗게 빛남.
    flat_mat("MN_WinGlow", (0.62, 0.74, 0.92), rough=0.1, emissive=(0.18, 0.25, 0.36))
    flat_mat("MN_Glass", (0.02, 0.03, 0.05), rough=0.04, spec=1.0)
    # 등불 — 따뜻한 발광(쿨 바탕의 웜 악센트)
    flat_mat("MN_Chandelier", (1.0, 0.82, 0.48), rough=0.3, emissive=(2.2, 1.4, 0.7))
    flat_mat("MN_Sconce", (1.0, 0.80, 0.45), rough=0.3, emissive=(1.7, 1.05, 0.52))


# ── 스폰 헬퍼 ──────────────────────────────────────────────────────────
def mesh(p):
    if p not in _m:
        _m[p] = EAL.load_asset(p)
    return _m[p]


def box(x, y, z, sx, sy, sz, mat, yaw=0.0, name=None):
    a = acts.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0, yaw, 0))
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


def plight(x, y, z, color, inten, rad, name=None):
    a = acts.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, z))
    c = a.get_component_by_class(unreal.PointLightComponent)
    c.set_editor_property("intensity", inten)
    c.set_editor_property("light_color", unreal.Color(color[0], color[1], color[2], 255))
    c.set_editor_property("attenuation_radius", rad)
    c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    if name:
        a.set_actor_label(name)
    CNT["n"] += 1
    return a


# ── 치수: 엄청 크게 ── 홀 = 96m(길이) x 36m(폭) x 20m(높이)
HW = 4800.0   # 길이(±X) 절반 = 96m
HD = 1800.0   # 폭(±Y) 절반 = 36m
CH = 2000.0   # 천장 높이 20m


def column(x, y, h):
    cyl(x, y, 20, 190, 40, "MN_MarbleD", name="Plinth")       # 짙은 대리석 주춧
    cyl(x, y, h / 2 + 40, 150, h - 80, "MN_MarbleW_W", name="Column")
    box(x, y, 30, 420, 420, 60, "MN_MarbleD", name="Base")
    box(x, y, h - 50, 380, 380, 100, "MN_Gilt", name="Capital")  # 금장 주두
    box(x, y, h + 30, 300, 300, 60, "MN_MarbleW", name="Abacus")


def coffered_ceiling():
    box(0, 0, CH + 60, HW * 2 + 400, HD * 2 + 400, 120, "MN_MarbleW", name="CeilSlab")
    nx, ny = 16, 6
    for i in range(nx):
        for j in range(ny):
            cx = -HW + HW * 2 * (i + 0.5) / nx
            cy = -HD + HD * 2 * (j + 0.5) / ny
            box(cx, cy, CH - 10, HW * 2 / nx - 70, HD * 2 / ny - 70, 40, "MN_Gilt_F", name="Coffer")
            box(cx, cy, CH - 45, HW * 2 / nx - 190, HD * 2 / ny - 190, 30, "MN_MarbleW", name="CofferIn")


def chandelier(x, y):
    cyl(x, y, CH - 300, 12, 300, "MN_Gilt", name="ChainRod")
    cyl(x, y, CH - 380, 130, 90, "MN_Gilt", name="ChTier")
    box(x, y, CH - 380, 340, 340, 30, "MN_Gilt", name="ChRing")
    for k in range(12):
        a = 2 * math.pi * k / 12
        bx = x + 260 * math.cos(a); by = y + 260 * math.sin(a)
        cyl(bx, by, CH - 420, 22, 80, "MN_Chandelier", name="Candle")
    plight(x, y, CH - 430, (255, 216, 156), 2000, 2400, name="ChLight")


def build():
    import_tex()
    build_materials()
    # ★MAP 이 이미 있으면 new_level(MAP) 이 덮지 못하고 Untitled 에서 작업→저장이 MAP 에 안 간다.
    #   헌 레벨을 지우고 새로 만들어 확실히 이 경로에 저장되게 한다.
    if EAL.does_asset_exist(MAP):
        EAL.delete_asset(MAP)
        log("[레벨] 헌 %s 삭제" % MAP)
    ok = les.new_level(MAP)
    log("[레벨] new_level(%s) -> %s · 스폰 시작" % (MAP, ok))

    # ── 바닥 — 대리석 체커(큰 타일) + 가운데 카펫 러너 ──
    tile = 480
    nx = int(HW * 2 / tile); ny = int(HD * 2 / tile)
    for i in range(nx):
        for j in range(ny):
            cx = -HW + tile * (i + 0.5); cy = -HD + tile * (j + 0.5)
            m = "MN_MarbleW" if (i + j) % 2 == 0 else "MN_MarbleD"
            box(cx, cy, -20, tile - 6, tile - 6, 40, m, name="Floor")
    # 가운데 붉은 카펫 러너(복도=도로 길이로 길게)
    box(0, 0, 3, HW * 2 - 1200, 1100, 22, "MN_Carpet", name="CarpetRunner")
    box(0, 0, 2, HW * 2 - 1000, 1260, 18, "MN_Gilt_F", name="CarpetTrim")

    # ── 벽 4면(다마스크) + 징두리(마호가니) + 몰딩 띠(금장) ──
    # 긴 벽(±Y향, xz 다마스크)
    for py in (-HD, HD):
        box(0, py, CH / 2, HW * 2, 120, CH, "MN_Damask_Y", name="WallLong")
        box(0, py + (40 if py < 0 else -40), 260, HW * 2, 50, 500, "MN_Wood", name="Wainscot")
        box(0, py + (40 if py < 0 else -40), 560, HW * 2, 40, 60, "MN_Gilt", name="ChairRail")
        box(0, py + (40 if py < 0 else -40), CH - 120, HW * 2, 45, 90, "MN_Gilt", name="Cornice")
    # 짧은 벽(±X향, yz 다마스크)
    for px in (-HW, HW):
        box(px, 0, CH / 2, 120, HD * 2, CH, "MN_Damask_X", name="WallShort")
        box(px + (40 if px < 0 else -40), 0, CH - 120, 45, HD * 2, 90, "MN_Gilt", name="CorniceS")

    # ── 기둥 두 줄(대칭 콜로네이드) ──
    col_xs = [-3800, -2850, -1900, -950, 0, 950, 1900, 2850, 3800]
    for x in col_xs:
        for y in (-1150, 1150):
            column(x, y, CH)

    # ── 아치 창(긴 벽) — 빛이 쏟아짐 ──
    win_xs = [-4200, -3350, -2500, -1650, -800, 800, 1650, 2500, 3350, 4200]
    for x in win_xs:
        for py in (-HD, HD):
            wy = py + (52 if py < 0 else -52)
            box(x, wy, 1000, 560, 26, 1400, "MN_WinGlow", name="WinGlass")
            box(x, wy, 300, 640, 46, 60, "MN_Gilt", name="WinSill")
            box(x, wy - (10 if py < 0 else -10), 1000, 60, 40, 1500, "MN_Gilt", name="WinMull")
            box(x, wy, 1740, 660, 46, 120, "MN_Gilt", name="WinArch")

    # ── 벽 그림 + 촛대(기둥 사이) ──
    for x in [-3350, -2500, -1650, -800, 800, 1650, 2500, 3350]:
        for py in (-HD, HD):
            wy = py + (58 if py < 0 else -58)
            box(x, wy, 1150, 400, 22, 560, "MN_Gilt", name="Frame")
            box(x, wy, 1150, 330, 14, 490, "MN_Paint", name="Painting")
    for x in win_xs:
        for py in (-HD, HD):
            wy = py + (66 if py < 0 else -66)
            cyl(x - 420, wy, 1300, 14, 70, "MN_Sconce", name="Sconce")
            plight(x - 420, py * 0.9, 1300, (255, 202, 142), 2600, 950)

    # ── 대계단 — 안쪽 끝(+X) 앞, 넓게 올라 갤러리로 ──
    steps = 16
    sx0 = HW - 2200
    for s in range(steps):
        box(sx0 + s * 110, 0, s * 90 + 30, 110, 2000, 60, "MN_MarbleW", name="Step")
        box(sx0 + s * 110, 0, s * 90, 110, 2100, s * 90 + 40, "MN_MarbleD", name="StepSide")
        box(sx0 + s * 110, 0, s * 90 + 65, 96, 1400, 12, "MN_Carpet", name="StepRunner")
    box(sx0 + steps * 110 + 500, 0, steps * 90 + 40, 1100, 2200, 60, "MN_MarbleW", name="Landing")
    for sy in (-1000, 1000):
        for s in range(0, steps, 1):
            cyl(sx0 + s * 110, sy, s * 90 + 160, 15, 180, "MN_Gilt", name="Baluster")
        box(sx0 + steps * 55, sy, steps * 90 + 160, steps * 110, 26, 26, "MN_Gilt", name="Handrail")

    # ── 격자 천장 + 샹들리에 ──
    coffered_ceiling()
    for cx in (-3200, -1600, 0, 1600, 3200):
        chandelier(cx, 0)

    # ── 가구(최소 — 탁 트임 유지): 긴 탁자 + 소파 ──
    box(-1400, 0, 110, 1800, 300, 50, "MN_Wood", name="Table")
    box(-1400, 0, 140, 1700, 220, 12, "MN_Gilt_F", name="TableTop")
    for sy in (-560, 560):
        box(-1400, sy, 90, 1300, 200, 130, "MN_Teal", name="Sofa")
        box(-1400, sy + (80 if sy < 0 else -80), 230, 1300, 60, 200, "MN_Teal", name="SofaBack")

    # ── 통일 광 분위기: 차가운 새벽/달빛이 홀 전체를 씻는다 ──
    # 방향광(창으로 쏟아지는 쿨 태양) + 볼류메트릭 신광
    d = acts.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 3000), unreal.Rotator(-34, 74, 0))
    dc = d.get_component_by_class(unreal.DirectionalLightComponent)
    dc.set_editor_property("intensity", 0.7)   # 은은한 방향 악센트(그늘/햇빛 대비 완화)
    dc.set_editor_property("light_color", unreal.Color(206, 224, 255))  # 쿨 화이트
    dc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    try:
        dc.set_editor_property("volumetric_scattering_intensity", 2.2)
    except Exception:
        pass
    acts.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    # 스카이라이트(쿨·앰비언트 = 밝고 탁 트임, 그러나 과하지 않게)
    sk = acts.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1400))
    skc = sk.get_component_by_class(unreal.SkyLightComponent)
    skc.set_editor_property("real_time_capture", True)
    skc.set_editor_property("intensity", 1.9)   # 균등 앰비언트가 주광 — 홀 전체를 고르게 밝힘
    skc.set_editor_property("light_color", unreal.Color(190, 210, 255))
    skc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    # 볼류메트릭 안개 — 창 신광이 뜨게(옅게)
    fog = acts.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 200))
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    try:
        fc.set_editor_property("fog_density", 0.0009)
        fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.10, 0.18, 0.34, 1.0))
        fc.set_editor_property("volumetric_fog", True)
        fc.set_editor_property("volumetric_fog_scattering_distribution", 0.2)
        fc.set_editor_property("volumetric_fog_extinction_scale", 0.6)
    except Exception as e:
        log("  fog prop 일부 실패 %s" % e)
    # (겹치는 필광·창광 제거 — 방향광+스카이 균등광으로 핫스팟을 없앤다. 등불만 웜 악센트.)

    # ── 포스트: 노출·블룸·통일 컬러그레이딩(쿨하게 씻고 그림자 파랗게) ──
    pp = acts.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 800))
    pp.set_editor_property("unbound", True)
    st = pp.get_editor_property("settings")
    # 자동노출(히스토그램)에 맡기되 범위를 좁혀 어두운 씬을 과하게 밝히지 않게. -game 이 균형 잡는다.
    st.set_editor_property("override_auto_exposure_method", True)
    st.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    st.set_editor_property("override_auto_exposure_bias", True)
    st.set_editor_property("auto_exposure_bias", -0.2)
    st.set_editor_property("override_auto_exposure_min_brightness", True)
    st.set_editor_property("auto_exposure_min_brightness", 0.4)
    st.set_editor_property("override_auto_exposure_max_brightness", True)
    st.set_editor_property("auto_exposure_max_brightness", 2.0)
    st.set_editor_property("override_bloom_intensity", True)
    st.set_editor_property("bloom_intensity", 0.28)
    st.set_editor_property("override_color_saturation", True)
    st.set_editor_property("color_saturation", unreal.Vector4(1.16, 1.16, 1.18, 1.0))
    st.set_editor_property("override_color_contrast", True)
    st.set_editor_property("color_contrast", unreal.Vector4(1.05, 1.05, 1.07, 1.0))
    # 통일: 그림자에 파란기, 하이라이트에 살짝 온기(쿨-웜 대비) — 컬러그레이딩 게인/오프셋
    st.set_editor_property("override_color_gain_shadows", True)
    st.set_editor_property("color_gain_shadows", unreal.Vector4(0.86, 0.96, 1.18, 1.0))
    st.set_editor_property("override_color_gain_highlights", True)
    st.set_editor_property("color_gain_highlights", unreal.Vector4(1.06, 1.02, 0.96, 1.0))
    st.set_editor_property("override_white_temp", True)
    st.set_editor_property("white_temp", 7200.0)   # 살짝 쿨
    # ── 환경 툰 외곽선(있으면 부착) — 셀 캐릭터와 톤 통일 ──
    #   ★기본 off: 캐릭터 없는 환경 단독 샷에선 화면테두리 헤이즈만 더한다. 캐릭터 배치 후 켜고 튜닝할 것.
    ATTACH_TOON = False
    toon = EAL.load_asset("%s/M_PP_ToonOutline" % MATDIR) if (ATTACH_TOON and EAL.does_asset_exist("%s/M_PP_ToonOutline" % MATDIR)) else None
    if toon:
        try:
            wb = unreal.WeightedBlendable()
            wb.set_editor_property("weight", 1.0)
            wb.set_editor_property("object", toon)
            wbs = unreal.WeightedBlendables()
            wbs.set_editor_property("array", [wb])
            st.set_editor_property("weighted_blendables", wbs)
            log("[포스트] 툰 외곽선 부착")
        except Exception as e:
            log("  툰 부착 실패 %s" % e)
    pp.set_editor_property("settings", st)

    acts.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-HW + 900, 0, 150))
    saved = les.save_current_level()
    log("[레벨] save_current_level -> %s" % saved)
    log("=== 저택 완료 · 액터 %d ===" % CNT["n"])


try:
    build()
except Exception:
    log("실패\n" + traceback.format_exc())
flush()
