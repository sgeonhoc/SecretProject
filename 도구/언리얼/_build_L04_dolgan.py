# -*- coding: utf-8 -*-
# ▶ L04 돌간의 흥신소 (아랫장터 이층) — 스토리 §7-1·§12·§15-1·§18 수사 허브.
#   100% 우리 것: /Engine/BasicShapes(Cube/Cylinder) + /Game/Rasel/Materials(우리 저작). 외부 팩 0.
#   설계 정본: 기획/02_게임설계/2_레벨/게임_레벨디자인_상세.md · L04.
import unreal

MAP = "/Game/Maps/Rasel/L04_Dolgan_Office"
MATDIR = "/Game/Rasel/Materials"
CUBE = "/Engine/BasicShapes/Cube.Cube"
CYL = "/Engine/BasicShapes/Cylinder.Cylinder"

# 방 치수 (좁고 낮은 이층 사무소)
HX = 300     # X 반길이 → 길이 600
HY = 260     # Y 반폭   → 폭 520
H = 300      # 천장 높이
T = 25       # 벽 두께
_AS = None
_MATS = {}


def log(m):
    unreal.log("[L04] " + m)
    try:
        with open("C:/Secret_Project/Saved/l04_dolgan.log", "a", encoding="utf-8") as f:
            f.write(m + "\n")
    except Exception:
        pass


def actor_sys():
    global _AS
    if _AS is None:
        _AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return _AS


# ── 우리 머티리얼 ────────────────────────────────────────────
def make_material(name, rgb, rough=0.85, emissive=None):
    path = MATDIR + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    at = unreal.AssetToolsHelpers.get_asset_tools()
    mat = at.create_asset(name, MATDIR, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary
    col = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    col.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
    r.set_editor_property("r", rough)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
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
        "M_Rasel_Plaster": (0.60, 0.55, 0.42),   # 누렇게 바랜 회벽(이층 사무소)
        "M_Rasel_Wood":    (0.34, 0.22, 0.13),
        "M_Rasel_Dark":    (0.07, 0.07, 0.09),
        "M_Rasel_Stone":   (0.28, 0.26, 0.24),
        "M_Rasel_Paper":   (0.82, 0.76, 0.58),   # ★신규: 부고란 누런 종이
        "M_Rasel_Roof":    (0.16, 0.40, 0.36),   # 창밖 지붕 실루엣
    }
    for n, c in spec.items():
        _MATS[n] = make_material(n, c)
    # 책상 등(발광 주황) — 이미 L01에서 저작됐으면 재사용
    _MATS["M_Rasel_Lantern"] = make_material("M_Rasel_Lantern", (1.0, 0.65, 0.3),
                                             emissive=(3.0, 1.6, 0.6))
    log("머티리얼 %d (신규 Paper 포함)" % len(_MATS))


# ── 프리미티브 ──────────────────────────────────────────────
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


def point_light(cx, cy, cz, lm, color, radius):
    pl = actor_sys().spawn_actor_from_class(unreal.PointLight, unreal.Vector(cx, cy, cz))
    if pl:
        try:
            c = pl.get_component_by_class(unreal.PointLightComponent)
            c.set_intensity(lm)
            c.set_light_color(unreal.LinearColor(color[0], color[1], color[2]))
            c.set_attenuation_radius(radius)
        except Exception:
            pass
    return pl


# ── 벽(개구부 포함) ─────────────────────────────────────────
def wall_x(y, openings, tag):
    """X 축을 따라 선 벽(두께 T, 높이 H). openings=[(x0,x1,z0,z1)] 만큼 구멍."""
    xs = -HX - T
    xe = HX + T
    ops = sorted(openings, key=lambda o: o[0])
    cur = xs
    for (x0, x1, z0, z1) in ops:
        if x0 > cur:  # 개구부 앞 꽉 찬 기둥
            box((cur + x0) / 2, y, H / 2, x0 - cur, T, H, "M_Rasel_Plaster", tag=tag + "_seg")
        if z1 < H:    # 상인방
            box((x0 + x1) / 2, y, (z1 + H) / 2, x1 - x0, T, H - z1, "M_Rasel_Plaster", tag=tag + "_lintel")
        if z0 > 0:    # 하방(문턱/창틀 아래)
            box((x0 + x1) / 2, y, z0 / 2, x1 - x0, T, z0, "M_Rasel_Plaster", tag=tag + "_sill")
        cur = x1
    if cur < xe:
        box((cur + xe) / 2, y, H / 2, xe - cur, T, H, "M_Rasel_Plaster", tag=tag + "_seg")


def wall_y(x, tag):
    box(x, 0, H / 2, T, 2 * HY + 2 * T, H, "M_Rasel_Plaster", tag=tag)


# ── 배선(우리 C++ 액터) ─────────────────────────────────────
def resolve(name):
    c = getattr(unreal, name, None)
    return c if c is not None else unreal.load_class(None, "/Script/Secret_Project." + name)


def setp(a, k, v):
    try:
        a.set_editor_property(k, v)
    except Exception as e:
        log("  ! set %s %s" % (k, e))


def lore(cx, cy, cz, title, lines, label):
    a = actor_sys().spawn_actor_from_class(resolve("LoreNoteActor"), unreal.Vector(cx, cy, cz))
    if a:
        setp(a, "Title", title)
        setp(a, "Lines", lines)
        a.set_actor_label("조사: " + label)
    return a


def build():
    build_materials()
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        unreal.EditorAssetLibrary.delete_asset(MAP)
    les.new_level(MAP)

    # 바닥·천장(낮은 이층)
    box(0, 0, -12, 2 * HX + 40, 2 * HY + 40, 24, "M_Rasel_Wood", tag="Floor")
    box(0, 0, H + 12, 2 * HX + 2 * T, 2 * HY + 2 * T, 24, "M_Rasel_Plaster", tag="Ceiling")

    # 앞벽(Y-) = 큰길 쪽: 문(좌) + 창(우)
    door = (-230, -80, 0, 230)     # 문 개구
    window = (60, 320, 110, 250)   # 창 개구(큰길 내려봄)
    wall_x(-HY, [door, window], "front")
    # 뒷벽(Y+) = 부고란 + 뒤 계단문(잠김)
    stair = (150, 300, 0, 220)
    wall_x(HY, [stair], "back")
    # 옆벽
    wall_y(-HX, "left")
    wall_y(HX, "right")

    # 문틀·창틀(나무)
    box(-155, -HY, 240, 190, T + 8, 30, "M_Rasel_Wood", tag="DoorLintel")
    box(190, -HY, 250, 280, T + 8, 26, "M_Rasel_Wood", tag="WinFrameTop")
    box(190, -HY, 110, 280, T + 8, 22, "M_Rasel_Wood", tag="WinSill")
    box(190, -HY - 6, 180, 260, 10, 130, "M_Rasel_Dark", tag="WinGlass")  # 어두운 유리

    # 창밖 큰길 지붕 실루엣(원경)
    for (ox, oz) in ((120, 60), (320, 30), (-40, 90)):
        box(190 + ox * 0.3, -HY - 260, oz, 220, 120, 140, "M_Rasel_Roof", tag="OutRoof")
    box(190, -HY - 400, -60, 900, 200, 200, "M_Rasel_Dark", tag="OutNight")

    # ★책상 + 서랍(사건의 심장) — 창에서 한 뼘 안쪽(§12-3)
    dx, dy = 40, 40
    box(dx, dy, 78, 300, 160, 20, "M_Rasel_Wood", tag="Desk")
    for lx in (-120, 120):
        for ly in (-60, 60):
            cyl(dx + lx, dy + ly, 39, 18, 78, "M_Rasel_Wood", tag="DeskLeg")
    box(dx - 90, dy, 55, 100, 150, 40, "M_Rasel_Wood", tag="Drawer")      # 서랍 몸
    box(dx - 90, dy - 78, 55, 96, 8, 36, "M_Rasel_Dark", tag="DrawerFace")  # 서랍 앞(어둠=열린 안)
    # 돌간 의자 + 손님 의자
    box(dx, dy + 130, 45, 90, 80, 12, "M_Rasel_Wood", tag="Chair")
    box(dx, dy + 168, 78, 90, 12, 60, "M_Rasel_Wood", tag="ChairBack")
    box(dx, dy - 150, 42, 80, 70, 12, "M_Rasel_Wood", tag="GuestChair")

    # 부고란 벽(뒷벽 안쪽에 누런 종이 다닥다닥)
    for i, gx in enumerate((-180, -60, 60)):
        box(gx, HY - 8, 190 + (i % 2) * 20, 90, 8, 120, "M_Rasel_Paper", tag="Obit_%d" % i)
    box(-60, HY - 6, 190, 300, 4, 8, "M_Rasel_Wood", tag="ObitRail")

    # ── 조명 (밤 기본) ──
    # 책상 등: 유일한 주광, 창에서 한 뼘 안쪽
    box(dx + 40, dy, 96, 40, 40, 8, "M_Rasel_Lantern", tag="DeskLamp")
    point_light(dx + 40, dy, 120, 1500.0, (1.0, 0.72, 0.42), 420.0)
    # 창으로 새어드는 큰길 가로등 주황
    point_light(190, -HY + 60, 190, 700.0, (1.0, 0.7, 0.4), 380.0)
    # 부고란 흐린 반사광
    point_light(-60, HY - 90, 210, 300.0, (0.9, 0.85, 0.7), 260.0)

    # SkyLight 약하게(밤 실내 앰비언트) + Fog + PPV
    sky = actor_sys().spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 200))
    if sky:
        try:
            sc = sky.get_component_by_class(unreal.SkyLightComponent)
            sc.set_editor_property("real_time_capture", False)
            sc.set_intensity(0.6)
        except Exception:
            pass
    actor_sys().spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 100))
    ppv = actor_sys().spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 150))
    if ppv:
        try:
            ppv.set_editor_property("unbound", True)
            s = ppv.get_editor_property("settings")
            s.set_editor_property("override_auto_exposure_method", True)
            s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
            s.set_editor_property("override_auto_exposure_bias", True)
            s.set_editor_property("auto_exposure_bias", 10.5)
            ppv.set_editor_property("settings", s)
        except Exception as e:
            log("  ! PPV %s" % e)

    # ── 배선 ──
    # 문 → L01
    a = actor_sys().spawn_actor_from_class(resolve("PortalActor"), unreal.Vector(-155, -HY + 90, 60))
    if a:
        setp(a, "TargetLevelName", "L01_Jangteo_Street")
        a.set_actor_label("→ L01_Jangteo_Street")
    # 뒤 계단(잠김)
    g = actor_sys().spawn_actor_from_class(resolve("LockedGateActor"), unreal.Vector(225, HY - 40, 110))
    if g:
        setp(g, "GateId", "dolgan_backstair")
        setp(g, "RequiredItemId", "")
        setp(g, "bConsumeKey", False)
        g.set_actor_label("잠긴 문 뒤계단")

    lore(dx - 90, dy - 60, 70,
         "서랍 속 종잇조각",
         ["아랫장터 사고사 셋 — 골동상·되넘김꾼·셋방 젊은이.",
          "셋 다 죽기 전 며칠 사이 신항 낱장 그림을 손에 쥐었다.",
          "위에 물음표 하나. 아래에 손 둘 — 태우는 손, 모으는 손.",
          "도시에서 이 셋을 한 줄에 놓고 본 눈은 이 서랍 하나뿐이다."],
         "서랍 속 종잇조각")
    lore(-60, HY - 40, 200,
         "부고란",
         ["관은 셋을 각각 흔한 사고로 접수했다 — 연기·계단·앓음.",
          "증명할 문법이 없다. 부고 세 줄과 주운 소문뿐.",
          "아래층에서 죽은 자는 여기 어느 줄에도 오르지 않는다."],
         "부고란")
    lore(190, -HY + 90, 170,
         "창가",
         ["길 건너 밤새 서 있던 두 사람이 남긴 담배꽁초 둘.",
          "등불을 창에서 한 뼘 안으로 옮겼다.",
          "누가 지켜보는지는 알아도, 왜 아직 살아 있는지는 몰랐다."],
         "창가")

    # 상주: 흥신소 주인(부고·진위 역할)
    npc = actor_sys().spawn_actor_from_class(resolve("ANPCCharacter"),
                                             unreal.Vector(dx, dy + 130, 120))
    if npc:
        setp(npc, "NPCName", "흥신소 주인")
        setp(npc, "DialogueLines",
             ["부고 세 줄이 한 줄에 놓이면, 그건 더는 사고가 아니야.",
              "묻지 않은 게 나를 살렸지. 자넨 뭘 묻고 싶나."])
        setp(npc, "bCanEnterCombat", False)
        npc.set_actor_label("NPC 흥신소 주인")

    # 플레이어 시작(문 안쪽)
    actor_sys().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-155, -HY + 130, 100))

    les.save_current_level()
    log("=== L04 돌간의 흥신소 저장 완료 ===")


open("C:/Secret_Project/Saved/l04_dolgan.log", "w", encoding="utf-8").close()
build()
