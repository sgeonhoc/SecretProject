# -*- coding: utf-8 -*-
# ▶ L22 요아의 셋방 + 옥상 — ★게임 시작점. 스토리 §3-5·§6-1·§6-3·§24-3.
#   좁은 셋방(실내) + 사다리 없이 문 하나로 이어진 옥상(개방). 옥상에서 첫 한 뼘 불이 선다.
#   100% 우리 것: /Engine/BasicShapes + /Game/Rasel/Materials. 외부 팩 0.
#   설계 정본: 기획/02_게임설계/2_레벨/게임_레벨디자인_상세.md · L22. StoryFlag 배선(op_has_leaf/op_symptom).
import unreal

MAP = "/Game/Maps/Rasel/L22_Yoa_Room"
MATDIR = "/Game/Rasel/Materials"
CUBE = "/Engine/BasicShapes/Cube.Cube"
CYL = "/Engine/BasicShapes/Cylinder.Cylinder"
T = 25
_AS = None
_MATS = {}


def log(m):
    unreal.log("[L22] " + m)
    try:
        with open("C:/Secret_Project/Saved/l22_yoa.log", "a", encoding="utf-8") as f:
            f.write(m + "\n")
    except Exception:
        pass


def actor_sys():
    global _AS
    if _AS is None:
        _AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return _AS


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
        "M_Rasel_Plaster": (0.62, 0.56, 0.44),
        "M_Rasel_Wood":    (0.34, 0.22, 0.13),
        "M_Rasel_Dark":    (0.07, 0.07, 0.09),
        "M_Rasel_Stone":   (0.30, 0.29, 0.27),   # 옥상 방수 콘크리트
        "M_Rasel_Roof":    (0.16, 0.40, 0.36),   # 원경 장터 지붕
        "M_Rasel_ClothA":  (0.55, 0.22, 0.18),   # 침구
        "M_Rasel_River":   (0.09, 0.17, 0.20),   # ★신규: 강 조망(어두운 청록)
    }
    for n, c in spec.items():
        _MATS[n] = make_material(n, c)
    _MATS["M_Rasel_Lantern"] = make_material("M_Rasel_Lantern", (1.0, 0.65, 0.3), emissive=(3.0, 1.6, 0.6))
    _MATS["M_Rasel_Flame"] = make_material("M_Rasel_Flame", (1.0, 0.5, 0.15), emissive=(6.0, 2.2, 0.4))  # 한 뼘 불
    log("머티리얼 %d (신규 River·Flame)" % len(_MATS))


def box(cx, cy, cz, sx, sy, sz, mat, yaw=0.0, pitch=0.0, tag=""):
    a = actor_sys().spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx, cy, cz),
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


def resolve(name):
    c = getattr(unreal, name, None)
    return c if c is not None else unreal.load_class(None, "/Script/Secret_Project." + name)


def setp(a, k, v):
    try:
        a.set_editor_property(k, v)
    except Exception as e:
        log("  ! set %s %s" % (k, e))


def lore(cx, cy, cz, title, lines, label, req="", forb=""):
    a = actor_sys().spawn_actor_from_class(resolve("LoreNoteActor"), unreal.Vector(cx, cy, cz))
    if a:
        setp(a, "Title", title)
        setp(a, "Lines", lines)
        if req:
            setp(a, "RequiredFlag", req)
        if forb:
            setp(a, "ForbiddenFlag", forb)
        a.set_actor_label("조사: " + label)
    return a


# ── 방 좌표: 방 x[-460,-20] y[-210,210] z[0,300], 옥상 x[-20,680] y[-300,300] 개방 ──
RX0, RX1, RY0, RY1, RH = -460, -20, -210, 210, 300


def wall_seg(x0, x1, y0, y1, z0, z1, mat, tag):
    box((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2,
        abs(x1 - x0) or T, abs(y1 - y0) or T, abs(z1 - z0) or T, mat, tag=tag)


def build_room():
    # 바닥·천장
    box((RX0 + RX1) / 2, 0, -12, (RX1 - RX0) + 40, (RY1 - RY0) + 40, 24, "M_Rasel_Wood", tag="Floor")
    box((RX0 + RX1) / 2, 0, RH + 12, (RX1 - RX0) + 2 * T, (RY1 - RY0) + 2 * T, 24, "M_Rasel_Plaster", tag="Ceiling")
    # 서벽(x=RX0) 솔리드
    wall_seg(RX0 - T, RX0, RY0 - T, RY1 + T, 0, RH, "M_Rasel_Plaster", "wall_W")
    # 남벽(y=RY0) 솔리드
    wall_seg(RX0, RX1, RY0 - T, RY0, 0, RH, "M_Rasel_Plaster", "wall_S")
    # 북벽(y=RY1): 창 개구(강 조망) — 좌우 기둥 + 상하 인방
    win_x0, win_x1, win_z0, win_z1 = -320, -160, 120, 240
    wall_seg(RX0, win_x0, RY1, RY1 + T, 0, RH, "M_Rasel_Plaster", "wall_N_l")
    wall_seg(win_x1, RX1, RY1, RY1 + T, 0, RH, "M_Rasel_Plaster", "wall_N_r")
    wall_seg(win_x0, win_x1, RY1, RY1 + T, 0, win_z0, "M_Rasel_Plaster", "wall_N_sill")
    wall_seg(win_x0, win_x1, RY1, RY1 + T, win_z1, RH, "M_Rasel_Plaster", "wall_N_lintel")
    box((win_x0 + win_x1) / 2, RY1 + 6, (win_z0 + win_z1) / 2, win_x1 - win_x0, 10, win_z1 - win_z0, "M_Rasel_Dark", tag="WinGlass")
    box((win_x0 + win_x1) / 2, RY1, win_z0, win_x1 - win_x0 + 30, T + 8, 20, "M_Rasel_Wood", tag="WinSill")
    # 동벽(x=RX1): 문 개구(→옥상)
    door_y0, door_y1, door_z = -60, 90, 240
    wall_seg(RX1, RX1 + T, RY0, door_y0, 0, RH, "M_Rasel_Plaster", "wall_E_s")
    wall_seg(RX1, RX1 + T, door_y1, RY1, 0, RH, "M_Rasel_Plaster", "wall_E_n")
    wall_seg(RX1, RX1 + T, door_y0, door_y1, door_z, RH, "M_Rasel_Plaster", "wall_E_lintel")
    box(RX1, (door_y0 + door_y1) / 2, door_z, T + 8, door_y1 - door_y0 + 20, 26, "M_Rasel_Wood", tag="DoorLintel")
    # 침상(SavePoint 자리)
    box(RX0 + 150, RY1 - 130, 45, 260, 150, 30, "M_Rasel_Wood", tag="Bed")
    box(RX0 + 150, RY1 - 130, 62, 250, 140, 16, "M_Rasel_ClothA", tag="Bedding")
    box(RX0 + 60, RY1 - 130, 78, 70, 120, 20, "M_Rasel_ClothA", tag="Pillow")
    # 책상
    box(RX0 + 120, RY0 + 90, 78, 200, 110, 18, "M_Rasel_Wood", tag="Desk")
    for lx in (-80, 80):
        cyl(RX0 + 120 + lx, RY0 + 90, 39, 16, 78, "M_Rasel_Wood", tag="DeskLeg")
    box(RX0 + 120, RY0 + 90, 100, 40, 40, 8, "M_Rasel_Lantern", tag="DeskLamp")
    point_light(RX0 + 120, RY0 + 90, 130, 1200.0, (1.0, 0.72, 0.42), 380.0)


def build_deck():
    DX0, DX1, DY0, DY1 = -20, 680, -300, 300
    # 옥상 바닥(방수 콘크리트)
    box((DX0 + DX1) / 2, 0, -12, (DX1 - DX0), (DY1 - DY0), 24, "M_Rasel_Stone", tag="DeckFloor")
    # 난간(북·동·남 가장자리, 낮음 90) — 서쪽은 방 벽
    rail = "M_Rasel_Wood"
    box((DX0 + DX1) / 2, DY1, 55, DX1 - DX0, 20, 90, rail, tag="Rail_N")
    box((DX0 + DX1) / 2, DY0, 55, DX1 - DX0, 20, 90, rail, tag="Rail_S")
    box(DX1, 0, 55, 20, DY1 - DY0, 90, rail, tag="Rail_E")
    # 물탱크(북동 구석)
    cyl(DX1 - 120, DY1 - 120, 110, 160, 220, "M_Rasel_Dark", tag="WaterTank")
    # 빨랫줄 기둥
    for px in (120, 420):
        cyl(px, DY0 + 120, 120, 16, 240, "M_Rasel_Wood", tag="LinePost")
    # ★불 시험 자리(북쪽 난간 앞, 도시 조망) — 낮은 턱
    box(300, DY1 - 90, 35, 200, 120, 40, "M_Rasel_Stone", tag="FlameLedge")
    # 계단실(남서 구석) — 내려감(→L01)
    box(DX0 + 90, DY0 + 90, 130, 150, 150, 260, "M_Rasel_Plaster", tag="StairHut")
    box(DX0 + 90, DY0 + 165, 120, 130, 10, 220, "M_Rasel_Dark", tag="StairDoor")


def build_view():
    # 원경: 장터 지붕선(북쪽 난간 너머), 그 뒤 강 띠
    for (ox, oz, w) in ((120, 40, 260), (360, 20, 300), (560, 60, 240), (-120, 30, 220)):
        box(ox, 620, oz, w, 160, 180, "M_Rasel_Roof", tag="FarRoof")
    box(300, 640, -40, 1400, 100, 160, "M_Rasel_Dark", tag="FarBlock")
    box(300, 900, -80, 2000, 400, 20, "M_Rasel_River", tag="RiverView")  # 강 띠(낮게)


def build_lighting():
    # 밤: 달빛 약 + 도시 글로우 + 책상등(방)
    dl = actor_sys().spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(300, 0, 1500))
    if dl:
        try:
            dl.set_actor_rotation(unreal.Rotator(0.0, -48.0, 120.0), False)
            c = dl.get_component_by_class(unreal.DirectionalLightComponent)
            c.set_intensity(1.6)
            c.set_light_color(unreal.LinearColor(0.7, 0.78, 1.0))  # 달빛 청백
            c.set_editor_property("atmosphere_sun_light", True)
        except Exception:
            pass
    actor_sys().spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(300, 0, 0))
    actor_sys().spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(300, 0, 100))
    sky = actor_sys().spawn_actor_from_class(unreal.SkyLight, unreal.Vector(300, 0, 700))
    if sky:
        try:
            sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property("real_time_capture", True)
        except Exception:
            pass
    # 도시 글로우(옥상 아래에서 올라오는 주황)
    point_light(300, 500, 40, 1500.0, (1.0, 0.7, 0.4), 900.0)
    ppv = actor_sys().spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(300, 0, 200))
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


def wire():
    # 침상 = 취침·저장(거점)
    sp = actor_sys().spawn_actor_from_class(resolve("SavePointActor"), unreal.Vector(RX0 + 150, RY1 - 130, 90))
    if sp:
        setp(sp, "bRestoreOnSave", True)
        setp(sp, "bAdvanceDayOnRest", True)
        sp.set_actor_label("침상 (취침·저장)")
    # 계단문 → L01
    p = actor_sys().spawn_actor_from_class(resolve("PortalActor"), unreal.Vector(50, -210, 60))
    if p:
        setp(p, "TargetLevelName", "L01_Jangteo_Street")
        p.set_actor_label("→ L01_Jangteo_Street (계단↓)")
    # 책상 조사(항상)
    lore(RX0 + 120, RY0 + 130, 100, "책상",
         ["외운 글자를 되짚는다. 아직 손끝만 뜨겁다.",
          "물그릇의 물이, 쥐면 미지근해진다."],
         "책상")
    # ★옥상 불 시험 자리 — 낱장을 얻은 뒤(op_has_leaf)에만
    lore(300, 190, 90, "옥상 끝",
         ["낱장의 글자를 더듬더듬 소리 내 읽어 본다.",
          "손끝의 뜨거움이 손바닥 위에서 한 뼘 불꽃으로 선다.",
          "처음으로, 앓던 것이 할 수 있는 것으로 바뀐 밤."],
         "옥상 끝 (첫 발현)", req="op_has_leaf")
    box(300, 210, 70, 60, 30, 60, "M_Rasel_Flame", yaw=0, tag="FirstFlameFX")  # 한 뼘 불(발광 표지)
    # 문 앞 명함 — 증상 이벤트 뒤(op_symptom)
    lore(RX0 + 30, -100, 120, "문 앞 명함",
         ["문틈에 명함 한 장이 끼워져 있다.",
          "'힘든 시기를 겪는 분들께 무료 상담' — 재단의 이름.",
          "누가 다녀갔는지는, 아직 모른다."],
         "문 앞 명함", req="op_symptom")
    # 상주: 아래층 셋집 주인(첫날 방세)
    npc = actor_sys().spawn_actor_from_class(resolve("ANPCCharacter"), unreal.Vector(80, -240, 100),
                                             unreal.Rotator(0, 0, 90))
    if npc:
        setp(npc, "NPCName", "셋집 주인")
        setp(npc, "DialogueLines",
             ["방세가 밀렸어. 이레 안에 못 내면 방을 빼야 해.",
              "젊을 때 부지런히 벌어. 딴생각 말고."])
        setp(npc, "bCanEnterCombat", False)
        DayEnum = getattr(unreal, "DayPhase", None)
        if DayEnum is not None:
            phs = [p for p in (getattr(DayEnum, "MORNING", None), getattr(DayEnum, "DAY", None)) if p is not None]
            if phs:
                setp(npc, "ActivePhases", phs)
        npc.set_actor_label("NPC 셋집 주인")


def build():
    build_materials()
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        unreal.EditorAssetLibrary.delete_asset(MAP)
    les.new_level(MAP)
    build_room()
    build_deck()
    build_view()
    build_lighting()
    wire()
    actor_sys().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(RX0 + 200, 0, 100))
    les.save_current_level()
    log("=== L22 요아의 셋방 저장 완료 ===")


open("C:/Secret_Project/Saved/l22_yoa.log", "w", encoding="utf-8").close()
build()
