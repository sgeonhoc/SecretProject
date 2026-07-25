# -*- coding: utf-8 -*-
# ▶ L01 장터 큰길 — 100% 우리 것으로. 엔진 기본 프리미티브(Cube/Cylinder, UE 배포 라이선스 안전) +
#   우리가 직접 저작한 머티리얼(/Game/Rasel/Materials)로 구성. 외부 샘플 팩 0.
#   (사용자 2026-07-21: 아트는 우리가 전부 직접 만든다. 지금은 프리미티브 블록아웃+우리 색,
#    나중에 우리가 만든 실제 메시로 교체.)
import math
import unreal

MAP = "/Game/Maps/Rasel/L01_Jangteo_Street"
MATDIR = "/Game/Rasel/Materials"
CUBE = "/Engine/BasicShapes/Cube.Cube"
CYL = "/Engine/BasicShapes/Cylinder.Cylinder"

COLS = 12
TILE = 400
X0 = 200
FAC_Y = 700
STORY = 400
_AS = None
_MATS = {}


def xc(i):
    return X0 + TILE * i


DOOR_LEFT = {2: "L04_Rooftop_Room", 8: "L02_Antique_Shop"}
DOOR_RIGHT = {3: "L05_Eatery", 9: "L06_Pawnshop_Back"}
ALLEY_COL = 5
TENT_COLS = [1, 4, 7, 10]


def log(m):
    unreal.log("[L01ours] " + m)
    try:
        with open("C:/Secret_Project/Saved/l01_ours.log", "a", encoding="utf-8") as f:
            f.write(m + "\n")
    except Exception:
        pass


def actor_sys():
    global _AS
    if _AS is None:
        _AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return _AS


# ── 우리 머티리얼 저작 ────────────────────────────────────────
def make_material(name, rgb, emissive=None):
    path = MATDIR + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    at = unreal.AssetToolsHelpers.get_asset_tools()
    mat = at.create_asset(name, MATDIR, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary
    col = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    col.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
    rough.set_editor_property("r", 0.85)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if emissive:
        em = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 400)
        em.set_editor_property("constant", unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0))
        mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    return mat


def build_materials():
    if not unreal.EditorAssetLibrary.does_directory_exist(MATDIR):
        unreal.EditorAssetLibrary.make_directory(MATDIR)
    spec = {
        "M_Rasel_Stone":   (0.30, 0.28, 0.25),   # 포장 돌
        "M_Rasel_Plaster": (0.78, 0.70, 0.54),   # 벽 회벽
        "M_Rasel_Wood":    (0.36, 0.23, 0.13),   # 나무(문틀·좌판·벤치)
        "M_Rasel_Roof":    (0.16, 0.40, 0.36),   # 기와(청록)
        "M_Rasel_ClothA":  (0.62, 0.20, 0.16),   # 천막/현수막(붉은)
        "M_Rasel_ClothB":  (0.24, 0.34, 0.52),   # 천막(푸른)
        "M_Rasel_Dark":    (0.09, 0.09, 0.11),   # 창·문안·골목 어둠
    }
    for n, c in spec.items():
        _MATS[n] = make_material(n, c)
    _MATS["M_Rasel_Lantern"] = make_material("M_Rasel_Lantern", (1.0, 0.65, 0.3),
                                             emissive=(3.0, 1.6, 0.6))
    log("머티리얼 %d 저작" % len(_MATS))


# ── 프리미티브 배치 ──────────────────────────────────────────
def box(cx, cy, cz, sx, sy, sz, mat, yaw=0.0, pitch=0.0, tag=""):
    a = actor_sys().spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(cx, cy, cz),
        unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
    if not a:
        return None
    smc = a.static_mesh_component
    smc.set_static_mesh(unreal.EditorAssetLibrary.load_asset(CUBE))
    a.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    if mat in _MATS:
        smc.set_material(0, _MATS[mat])
    a.set_mobility(unreal.ComponentMobility.STATIC)
    if tag:
        a.set_actor_label(tag)
    return a


def cyl(cx, cy, cz, dia, h, mat, tag=""):
    a = actor_sys().spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx, cy, cz))
    if not a:
        return None
    smc = a.static_mesh_component
    smc.set_static_mesh(unreal.EditorAssetLibrary.load_asset(CYL))
    a.set_actor_scale3d(unreal.Vector(dia / 100.0, dia / 100.0, h / 100.0))
    if mat in _MATS:
        smc.set_material(0, _MATS[mat])
    a.set_mobility(unreal.ComponentMobility.STATIC)
    if tag:
        a.set_actor_label(tag)
    return a


# ── 거리 ────────────────────────────────────────────────────
def build_ground():
    box(xc(6) - 200, 0, -15, TILE * COLS + 400, 1500, 30, "M_Rasel_Stone", tag="Ground")


def build_facade(side_y, doors, alley_col, tag):
    for i in range(COLS):
        if i == alley_col:
            box(xc(i), side_y, 40, TILE - 10, 60, 80, "M_Rasel_Dark", tag="AlleyMouth")
            continue
        cx = xc(i)
        # 벽 2층
        box(cx, side_y, STORY, TILE - 8, 60, STORY * 2, "M_Rasel_Plaster", tag="%s_wall_%d" % (tag, i))
        # 창(1·2층) — 벽면에 어두운 인셋
        wy = side_y - 35 if side_y > 0 else side_y + 35
        box(cx, wy, 250, 150, 14, 150, "M_Rasel_Dark", tag="%s_win1_%d" % (tag, i))
        box(cx, wy, 620, 150, 14, 150, "M_Rasel_Dark", tag="%s_win2_%d" % (tag, i))
        if i in doors:
            # 문(어두운 입구) + 나무 문틀
            box(cx, wy, 140, 170, 16, 280, "M_Rasel_Dark", tag="%s_door_%d" % (tag, i))
            box(cx, wy, 300, 210, 26, 40, "M_Rasel_Wood", tag="Doorlintel")
            box(cx - 100, wy, 140, 26, 26, 300, "M_Rasel_Wood", tag="Doorpost")
            box(cx + 100, wy, 140, 26, 26, 300, "M_Rasel_Wood", tag="Doorpost")
    # 지붕: 벽 위에 평평한 청록 처마(거리 쪽으로 살짝 내밈). 칼날 슬래브 대신 반듯한 지붕선.
    ov = side_y - 120 if side_y > 0 else side_y + 120     # 처마가 거리 쪽으로 조금 나옴
    box(xc(6) - 200, ov, STORY * 2 + 25, TILE * COLS + 200, 360, 50,
        "M_Rasel_Roof", tag="%s_roof" % tag)
    # 처마 밑 나무 도리
    box(xc(6) - 200, side_y - 30 if side_y > 0 else side_y + 30, STORY * 2 - 30,
        TILE * COLS + 100, 40, 40, "M_Rasel_Wood", tag="%s_eave" % tag)


def build_stalls():
    for k, i in enumerate(TENT_COLS):
        cy = 340 if k % 2 == 0 else -340
        cx = xc(i)
        cloth = "M_Rasel_ClothA" if k % 2 == 0 else "M_Rasel_ClothB"
        # 좌판 상 + 다리 4 + 차양
        box(cx, cy, 90, 320, 200, 24, "M_Rasel_Wood", tag="Stall_top_%d" % i)
        for dx in (-140, 140):
            for dy in (-80, 80):
                cyl(cx + dx, cy + dy, 45, 22, 90, "M_Rasel_Wood", tag="Stall_leg")
        box(cx, cy, 250, 360, 240, 16, cloth, pitch=8, tag="Stall_awn_%d" % i)
        # 물건 상자
        box(cx - 120, cy * 0.5, 35, 120, 90, 70, "M_Rasel_Wood", tag="Box")
        box(cx + 110, cy * 0.5, 30, 100, 80, 60, "M_Rasel_Wood", tag="Box")


def build_props():
    # 가로등: 나무 기둥 + 발광 등 + 점광
    n = 0
    for i in range(1, COLS, 2):
        for sy in (430, -430):
            cyl(xc(i), sy, 190, 26, 380, "M_Rasel_Wood", tag="Lamppost")
            box(xc(i), sy, 400, 70, 70, 90, "M_Rasel_Lantern", tag="Lantern")
            pl = actor_sys().spawn_actor_from_class(unreal.PointLight, unreal.Vector(xc(i), sy, 400))
            if pl:
                try:
                    c = pl.get_component_by_class(unreal.PointLightComponent)
                    c.set_intensity(2400.0)
                    c.set_light_color(unreal.LinearColor(1.0, 0.72, 0.42))
                    c.set_attenuation_radius(820.0)
                except Exception:
                    pass
            n += 1
    # 현수막(파사드에 세로 천)
    for i in (1, 6, 10):
        box(xc(i), FAC_Y - 70, 640, 120, 16, 300, "M_Rasel_ClothA", tag="Banner")
        box(xc(i), -FAC_Y + 70, 640, 120, 16, 300, "M_Rasel_ClothB", tag="Banner")
    # 중앙 우물(랜드마크) — 좌측 니치
    cyl(xc(6), 470, 60, 220, 120, "M_Rasel_Stone", tag="Well")
    cyl(xc(6), 470, 130, 170, 40, "M_Rasel_Dark", tag="Well_water")
    # 벤치
    for (bx, by) in ((xc(3), 470), (xc(8), -470)):
        box(bx, by, 45, 220, 60, 20, "M_Rasel_Wood", tag="Bench")
        for dx in (-90, 90):
            cyl(bx + dx, by, 22, 20, 44, "M_Rasel_Wood", tag="Bench_leg")
    log("좌판·가로등·현수막·우물·벤치 (점광 %d)" % n)


def build_lighting():
    dl = actor_sys().spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(xc(6), 0, 1400))
    if dl:
        try:
            dl.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-46.0, yaw=-35.0), False)
            lc = dl.get_component_by_class(unreal.DirectionalLightComponent)
            lc.set_intensity(3.5)
            lc.set_light_color(unreal.LinearColor(1.0, 0.96, 0.88))
            lc.set_editor_property("atmosphere_sun_light", True)
        except Exception as e:
            log("  ! DL %s" % e)
    actor_sys().spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(xc(6), 0, 0))
    actor_sys().spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(xc(6), 0, 200))
    sky = actor_sys().spawn_actor_from_class(unreal.SkyLight, unreal.Vector(xc(6), 0, 800))
    if sky:
        try:
            sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property("real_time_capture", True)
        except Exception:
            pass
    ppv = actor_sys().spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(xc(6), 0, 400))
    if ppv:
        try:
            ppv.set_editor_property("unbound", True)
            s = ppv.get_editor_property("settings")
            s.set_editor_property("override_auto_exposure_method", True)
            s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
            s.set_editor_property("override_auto_exposure_bias", True)
            s.set_editor_property("auto_exposure_bias", 9.5)
            ppv.set_editor_property("settings", s)
        except Exception as e:
            log("  ! PPV %s" % e)


# ── 배선(우리 C++ 액터) ─────────────────────────────────────
def resolve(name):
    c = getattr(unreal, name, None)
    return c if c is not None else unreal.load_class(None, "/Script/Secret_Project." + name)


def setp(a, k, v):
    try:
        a.set_editor_property(k, v)
    except Exception as e:
        log("  ! set %s %s" % (k, e))


def wire():
    Portal = resolve("PortalActor")

    def portal(cx, cy, tgt, label):
        a = actor_sys().spawn_actor_from_class(Portal, unreal.Vector(cx, cy, 60))
        if a:
            setp(a, "TargetLevelName", tgt)
            a.set_actor_label(label)

    for i, tgt in DOOR_LEFT.items():
        portal(xc(i), FAC_Y - 130, tgt, "→ " + tgt)
    for i, tgt in DOOR_RIGHT.items():
        portal(xc(i), -FAC_Y + 130, tgt, "→ " + tgt)
    portal(xc(ALLEY_COL), -FAC_Y + 40, "L03_Backalley", "→ L03_Backalley")
    portal(xc(COLS - 1) + 260, 0, "L07_Tram_Jangteo", "→ L07_Tram_Jangteo (전차역)")

    Lore = resolve("LoreNoteActor")
    ln = actor_sys().spawn_actor_from_class(Lore, unreal.Vector(xc(5), FAC_Y - 90, 180))
    if ln:
        setp(ln, "Title", "벽보판")
        setp(ln, "Lines", ["도시 소문과 의뢰가 붙는 판.",
                           "신항 공사장에서 밤마다 등불이 오르내린다는 말.",
                           "의원에 실려 온 사람이 늘었다는 말."])
        ln.set_actor_label("조사: 벽보판")
    box(xc(5), FAC_Y - 70, 180, 220, 16, 200, "M_Rasel_Wood", tag="Board")

    NPC = resolve("ANPCCharacter")
    DayEnum = getattr(unreal, "DayPhase", None)
    day = getattr(DayEnum, "DAY", None) if DayEnum else None
    night = getattr(DayEnum, "NIGHT", None) if DayEnum else None
    specs = [
        (xc(1), 340, "좌판 상인", ["오늘 물건은 좋아. 골라 봐.", "밤엔 셔터 내리니까 낮에 와."], day, "convenience"),
        (xc(4), -340, "골동상 호객꾼", ["안쪽에 진짜가 있어. 감정도 해 주고."], day, None),
        (xc(7), 340, "약초 좌판", ["의원 것보다 싸. 효험은 봐야 알고."], day, None),
        (xc(10), -340, "뜨내기", ["뱃말 섞어 쓰는 자들이 요즘 부쩍 늘었어."], night, None),
    ]
    m = 0
    for cx, cy, role, lines, phase, shop in specs:
        a = actor_sys().spawn_actor_from_class(NPC, unreal.Vector(cx, cy * 0.45, 120),
                                               unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0 if cy > 0 else 0.0))
        if a:
            setp(a, "NPCName", role)
            setp(a, "DialogueLines", lines)
            setp(a, "bCanEnterCombat", False)
            if shop:
                setp(a, "bIsShopkeeper", True)
                setp(a, "ShopKind", shop)
                setp(a, "bClosedAtNight", True)
            if phase is not None:
                setp(a, "ActivePhases", [phase])
            a.set_actor_label("NPC " + role)
            m += 1
    log("배선: 포탈6·벽보판·NPC%d" % m)


def build():
    build_materials()
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        unreal.EditorAssetLibrary.delete_asset(MAP)
    les.new_level(MAP)
    build_ground()
    build_facade(FAC_Y, DOOR_LEFT, ALLEY_COL if False else -1, "L")   # 좌측: 골목 없음
    build_facade(-FAC_Y, DOOR_RIGHT, ALLEY_COL, "R")                  # 우측: 골목 뚫림
    build_stalls()
    build_props()
    build_lighting()
    actor_sys().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-200, 0, 120))
    wire()
    les.save_current_level()
    log("=== L01(우리 것) 저장 완료 ===")


open("C:/Secret_Project/Saved/l01_ours.log", "w", encoding="utf-8").close()
build()
