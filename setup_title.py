# -*- coding: utf-8 -*-
# =============================================================================
#  부팅 시작화면(타이틀) 스테이지 레벨 — 게임 켜면 처음 나오는 화면.
#   /Game/Title 을 "흰 공간" 스테이지로 구성:
#     - 흰 바닥 + 흰 백드롭(unlit white) + 밝은 라이팅 → 페르소나풍 무한 흰 공간
#     - 히어로 캐릭터(BP_ANPCCharacter1) + 아이들 애니(루프) — 화면 우측에 서 있음
#     - 뒤쪽 실루엣 캐릭터 2~3(unlit black) → 흰 배경 대비 검은 실루엣(깊이감)
#     - 앰비언트 VFX(발밑 마법진 NS_cast + 아우라) — 임팩트/연출
#     - 프레이밍 카메라(auto-activate Player0) → 히어로가 화면 우측, 좌측은 메뉴 UI 자리
#     - WorldSettings.GameMode = TitleGameMode (→ WBP_MainMenu 자동표시 + title BGM)
#     - WBP_MainMenu.GameplayLevelName = "TestBattle" (새 게임 → 본편)
#   ※ WBP_MainMenu 는 시스루(좌패널만 반투명) 레이아웃 → 이 3D 스테이지가 우측에 비침(build_all_ui.py).
#  실행(에디터 닫은 상태):
#    UnrealEditor-Cmd <uproject> -ExecutePythonScript="setup_title.py" -unattended -nopause -nosplash
# =============================================================================
import unreal
EAL = unreal.EditorAssetLibrary
BEL = unreal.BlueprintEditorLibrary
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
UES = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
ATOOLS = unreal.AssetToolsHelpers.get_asset_tools()
def log(m): unreal.log("[Title] %s" % m)

LEVEL_PATH = "/Game/Title"
HERO_BP    = "/Game/BP_Characters/BP_ANPCCharacter1"
SIL_BPS    = ["/Game/BP_Characters/BP_ANPCCharacter2",
              "/Game/BP_Characters/BP_ANPCCharacter3",
              "/Game/BP_Characters/BP_ANPCCharacter5"]
IDLE_ANIM  = "/Game/ABP/TEST2/BOXING/None_Attack/Idle_UE_Anim"
CUBE       = "/Engine/BasicShapes/Cube"
NS_CIRCLE  = "/Game/VFX/NS_cast"                              # 발밑 마법진(루프)
NS_AURA    = "/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Soul"  # 떠다니는 영혼 입자

def gen_class(path):
    if not EAL.does_asset_exist(path):
        log("  ! 에셋 없음: %s" % path); return None
    try: return EAL.load_asset(path).generated_class()
    except Exception as e: log("  ! generated_class %s: %s" % (path, e)); return None

def spawn(cls, loc, rot=None):
    if cls is None: return None
    rot = rot if rot is not None else unreal.Rotator(0, 0, 0)
    try: return EAS.spawn_actor_from_class(cls, unreal.Vector(*loc), rot)
    except Exception as e: log("  ! spawn: %s" % e); return None

def set_mobility(comp):
    try: comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    except Exception: pass

# ── 0) unlit 머티리얼(흰/검) 생성 ──────────────────────────────
def make_unlit(name, color):
    path = "/Game/Title/%s" % name
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    try:
        mat = ATOOLS.create_asset(name, "/Game/Title", unreal.Material, unreal.MaterialFactoryNew())
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
        node.set_editor_property("constant", color)
        unreal.MaterialEditingLibrary.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        EAL.save_loaded_asset(mat)
        log("  머티리얼 생성: %s" % path)
        return mat
    except Exception as e:
        log("  ! 머티리얼 %s 실패: %s" % (name, e)); return None

# ── 1) 새 레벨 + 기존 액터 정리 ────────────────────────────────
log("1) 새 레벨: %s" % LEVEL_PATH)
try: LES.new_level(LEVEL_PATH)
except Exception as e: log("  ! new_level: %s" % e)
try:
    n = 0
    for a in EAS.get_all_level_actors():
        try: EAS.destroy_actor(a); n += 1
        except Exception: pass
    log("  기존 액터 %d개 정리" % n)
except Exception as e: log("  ! 정리: %s" % e)

M_WHITE = make_unlit("M_TitleWhite", unreal.LinearColor(0.97, 0.97, 0.98, 1.0))
M_BLACK = make_unlit("M_TitleSilhouette", unreal.LinearColor(0.015, 0.015, 0.02, 1.0))

# ── 2) 흰 공간: 바닥 + 백드롭(unlit white) ─────────────────────
cube = EAL.load_asset(CUBE) if EAL.does_asset_exist(CUBE) else None
def white_box(loc, scale, label):
    a = spawn(unreal.StaticMeshActor, loc)
    if a and cube:
        try:
            a.static_mesh_component.set_static_mesh(cube)
            a.set_actor_scale3d(unreal.Vector(*scale))
            if M_WHITE: a.static_mesh_component.set_material(0, M_WHITE)
            a.set_actor_label(label)
        except Exception as e: log("  ! %s: %s" % (label, e))
    return a
white_box((300, 0, -2),    (70, 70, 0.2),  "WhiteFloor")    # 흰 바닥(7000x7000)
white_box((1700, 0, 600),  (0.2, 80, 42),  "WhiteBackdrop") # 뒤쪽 흰 벽(YZ, 8000x4200)
log("  흰 바닥 + 백드롭")

# ── 3) 라이팅(히어로용 — 백드롭/바닥은 unlit이라 무관, 캐릭터만 비춤) ──
dl = spawn(unreal.DirectionalLight, (0, 0, 1000), unreal.Rotator(0.0, -38.0, -60.0))  # (roll,pitch,yaw)
if dl:
    try:
        c = dl.get_component_by_class(unreal.DirectionalLightComponent)
        set_mobility(c); c.set_intensity(4.5)
    except Exception: pass
sl = spawn(unreal.SkyLight, (0, 0, 600))
if sl:
    try:
        c = sl.get_component_by_class(unreal.SkyLightComponent)
        set_mobility(c)
        try: c.set_editor_property("real_time_capture", True)
        except Exception: pass
        c.set_intensity(2.2)
    except Exception: pass
# 정면 보조광(히어로 얼굴 또렷하게)
fp = spawn(unreal.PointLight, (-450, -60, 200))
if fp:
    try:
        c = fp.get_component_by_class(unreal.PointLightComponent)
        set_mobility(c); c.set_intensity(40000.0); c.set_attenuation_radius(2000.0)
    except Exception: pass
log("  라이팅(디렉셔널+스카이+정면보조)")

# ── 4) 캐릭터 헬퍼: 배치 + 아이들 애니(+선택 실루엣 머티리얼) ──
idle = EAL.load_asset(IDLE_ANIM) if EAL.does_asset_exist(IDLE_ANIM) else None
if idle is None: log("  ! 아이들 애니 없음: %s" % IDLE_ANIM)

def set_idle(npc):
    """캐릭터 메시를 단일노드 아이들 루프로(ABP 무관하게 확실히 움직이게)."""
    if idle is None: return
    try:
        skm = npc.get_editor_property("mesh")
        skm.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        try: skm.set_editor_property("anim_to_play", idle)
        except Exception: pass
        try:
            data = skm.get_editor_property("animation_data")
            data.set_editor_property("anim_to_play", idle)
            data.set_editor_property("saved_looping", True)
            data.set_editor_property("saved_playing", True)
            skm.set_editor_property("animation_data", data)
        except Exception: pass
        try: skm.play(True)
        except Exception: pass
    except Exception as e: log("  ! set_idle: %s" % e)

def silhouette(npc):
    try:
        skm = npc.get_editor_property("mesh")
        if M_BLACK:
            for i in range(skm.get_num_materials()):
                skm.set_material(i, M_BLACK)
    except Exception as e: log("  ! silhouette: %s" % e)

def place_char(bp_path, loc, label, props=None, sil=False):
    cls = gen_class(bp_path)
    if cls is None: return None
    npc = spawn(cls, loc, unreal.Rotator(0.0, 0.0, 180.0))  # yaw180 = -X(카메라) 바라봄
    if not npc: return None
    p = {"bCanEnterCombat": False, "bIsShopkeeper": False, "bCanBeRecruited": False, "ActivePhases": []}
    if props: p.update(props)
    for k, v in p.items():
        try: npc.set_editor_property(k, v)
        except Exception: pass
    set_idle(npc)
    if sil: silhouette(npc)
    try: npc.set_actor_label(label)
    except Exception: pass
    return npc

# ── 5) 히어로(화면 우측에 서도록 +Y, 카메라가 좌측으로 프레이밍) + 실루엣 ──
hero = place_char(HERO_BP, (0, 0, 90), "TITLE_HERO_강현", {"NPCName": "강현"})
log("  히어로 배치 + 아이들")
# 뒤쪽(+X, 카메라에서 멂) 실루엣 — 흰 배경에 검은 실루엣
sil_locs = [(420, -300, 90), (520, 280, 90), (760, -40, 90)]
for i, bp_path in enumerate(SIL_BPS):
    if i < len(sil_locs):
        place_char(bp_path, sil_locs[i], "TITLE_SIL_%d" % (i + 1), sil=True)
log("  실루엣 %d 배치" % len(SIL_BPS))

# ── 6) 앰비언트 VFX(발밑 마법진 + 영혼 아우라) ─────────────────
NiagaraActor = getattr(unreal, "NiagaraActor", None)
def place_ns(asset_path, loc, label):
    if NiagaraActor is None: return
    if not EAL.does_asset_exist(asset_path):
        log("  ! VFX 없음: %s" % asset_path); return
    a = spawn(NiagaraActor, loc)
    if not a: return
    try:
        comp = a.get_component_by_class(unreal.NiagaraComponent)
        comp.set_asset(EAL.load_asset(asset_path))
        try: comp.set_editor_property("auto_activate", True)
        except Exception: pass
        a.set_actor_label(label)
    except Exception as e: log("  ! VFX %s: %s" % (label, e))
place_ns(NS_CIRCLE, (0, 0, 6),   "TITLE_VFX_circle")   # 히어로 발밑 마법진
place_ns(NS_AURA,   (0, 0, 110), "TITLE_VFX_aura")     # 히어로 주변 영혼 입자
log("  앰비언트 VFX")

# ── 7) 프레이밍 카메라(히어로를 화면 우측에) + auto-activate ───
cam_loc = (-560, 150, 150)
look_at = (40, -90, 118)   # 히어로(Y=0)보다 왼쪽을 바라봐 → 히어로가 화면 우측에 잡힘
try:
    cam_rot = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*cam_loc), unreal.Vector(*look_at))
except Exception:
    cam_rot = unreal.Rotator(0, 0, 0)
cam = spawn(unreal.CameraActor, cam_loc, cam_rot)
if cam:
    try: cam.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.PLAYER0)
    except Exception as e: log("  ! cam auto-activate: %s" % e)
    try:
        cc = cam.camera_component
        cc.set_field_of_view(46.0)
    except Exception: pass
    try: cam.set_actor_label("TITLE_CAMERA")
    except Exception: pass
log("  카메라(auto-activate, 히어로 우측 프레이밍)")

# 기본 폰이 화면에 안 잡히게 PlayerStart는 카메라 뒤로
spawn(unreal.PlayerStart, (-1600, 0, 90))

# ── 8) WorldSettings.GameMode = TitleGameMode ──────────────────
try:
    ws = UES.get_editor_world().get_world_settings()
    tgm = getattr(unreal, "TitleGameMode", None)
    if tgm:
        ws.set_editor_property("default_game_mode", tgm)
        log("  GameModeOverride = TitleGameMode")
    else:
        log("  ! unreal.TitleGameMode 없음 — C++ 빌드 확인")
except Exception as e:
    log("  ! WorldSettings GameMode: %s" % e)

# ── 9) WBP_MainMenu.GameplayLevelName = TestBattle ─────────────
MM = "/Game/UI/WBP_MainMenu"
try:
    if EAL.does_asset_exist(MM):
        mm = EAL.load_asset(MM)
        cdo = unreal.get_default_object(mm.generated_class())
        cdo.set_editor_property("GameplayLevelName", "TestBattle")
        BEL.compile_blueprint(mm)
        EAL.save_asset(MM, only_if_is_dirty=False)
        log("  WBP_MainMenu.GameplayLevelName = TestBattle")
    else:
        log("  ! WBP_MainMenu 없음 — build_all_ui 먼저")
except Exception as e:
    log("  ! GameplayLevelName: %s" % e)

# ── 10) 검증 + 저장 ────────────────────────────────────────────
try:
    actors = EAS.get_all_level_actors()
    log("  레벨 액터 수(저장 전): %d" % len(actors))
except Exception as e: log("  ! 액터 수: %s" % e)
try:
    LES.save_current_level()
    log("  레벨 저장 완료")
except Exception as e:
    log("  ! save: %s" % e)
    try: EAL.save_asset(LEVEL_PATH); log("  save_asset 폴백")
    except Exception as e2: log("  ! 폴백도 실패: %s" % e2)
log("==== 타이틀 스테이지 구성 완료 ====")
