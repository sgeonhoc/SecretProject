# -*- coding: utf-8 -*-
# =============================================================================
#  테스트 전투 아레나 자동 구성 (헤드리스) — v2 (확실한 라이팅 + 24모델 배치)
#  실행:
#    "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
#      "C:\Secret_Project\Secret_Project.uproject" ^
#      -ExecutePythonScript="C:/Secret_Project/setup_test_battle.py" -unattended -nopause -nosplash
#
#  /Game/TestBattle 에:
#   - 무버블 디렉셔널 + 스카이(실시간캡처) + 포인트라이트 다발(라이트빌드 불필요로 밝게)
#   - 바닥(밝은 머티리얼) / PlayerStart / BP_BattleManager
#   - 아군 3(정면 근처, 전투 자동합류) / 적군 그룹 3(약점 다양) / 보스 1(BossLevel150)
#   - 나머지 모든 NPC 모델을 뒤쪽 "쇼케이스 줄"에 배치(모델 다양성 눈으로 확인)
# =============================================================================
import unreal

EAL = unreal.EditorAssetLibrary
BEL = unreal.BlueprintEditorLibrary
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

LEVEL_PATH = "/Game/TestBattle"
GM_BP   = "/Game/BP_SecretProjectGameMode"
PLAYER  = "/Game/BP_Characters/NewFolder/BP_PlayerCharacter"
BM_BP   = "/Game/Battle/Core/BP_BattleManager"
CUBE    = "/Engine/BasicShapes/Cube"
MESH_FB = "/Game/Characters/Mannequins/Meshes/SKM_Manny"  # BP에 메시 없을 때만 폴백

def log(m): unreal.log("[TestBattle] %s" % m)

def gen_class(path):
    if not EAL.does_asset_exist(path):
        log("  ! 에셋 없음: %s" % path); return None
    try: return EAL.load_asset(path).generated_class()
    except Exception as e:
        log("  ! generated_class 실패 %s: %s" % (path, e)); return None

def spawn(cls, loc, rot=None):
    if cls is None: return None
    rot = rot if rot is not None else unreal.Rotator(0, 0, 0)
    try: return EAS.spawn_actor_from_class(cls, unreal.Vector(loc[0], loc[1], loc[2]), rot)
    except Exception as e:
        log("  ! spawn 실패: %s" % e); return None

# ── 1) 새 레벨 ───────────────────────────────────────────────
log("1) 새 레벨: %s" % LEVEL_PATH)
try: LES.new_level(LEVEL_PATH)
except Exception as e: log("  ! new_level: %s" % e)

# 시작맵이 TestBattle이면 헤드리스가 기존 레벨을 로드한 채라 new_level이 안 비울 수 있음 →
# 기존 액터 전부 제거해 깨끗한 상태에서 재구성(중복/거꾸로 옛 NPC 제거).
try:
    old = EAS.get_all_level_actors()
    n = 0
    for a in old:
        try: EAS.destroy_actor(a); n += 1
        except Exception: pass
    log("  기존 액터 정리: %d개 제거" % n)
except Exception as e: log("  ! 기존 액터 정리: %s" % e)

# ── 2) 바닥 ──────────────────────────────────────────────────
cube = EAL.load_asset(CUBE) if EAL.does_asset_exist(CUBE) else None
if cube:
    floor = spawn(unreal.StaticMeshActor, (0, 0, -50))
    if floor:
        try:
            floor.static_mesh_component.set_static_mesh(cube)
            floor.set_actor_scale3d(unreal.Vector(80, 80, 1))  # 80m x 80m
            log("  바닥(80m)")
        except Exception as e: log("  ! 바닥: %s" % e)

# ── 3) 라이팅 (빌드 불필요로 밝게: 무버블 + 실시간 스카이 + 포인트라이트 다발) ──
def set_mobility(comp):
    try: comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    except Exception: pass

# 디렉셔널(태양)
dl = spawn(unreal.DirectionalLight, (0, 0, 1000), unreal.Rotator(0.0, -46.0, -35.0))  # (roll,pitch,yaw)
if dl:
    try:
        c = dl.get_component_by_class(unreal.DirectionalLightComponent)
        set_mobility(c); c.set_intensity(6.0)
        try: c.set_editor_property("atmosphere_sun_light", True)
        except Exception: pass
        log("  디렉셔널 라이트")
    except Exception as e: log("  ! 디렉셔널: %s" % e)
# 스카이 애트모스피어 + 스카이라이트(실시간 캡처)
spawn(unreal.SkyAtmosphere, (0, 0, 0))
sl = spawn(unreal.SkyLight, (0, 0, 600))
if sl:
    try:
        c = sl.get_component_by_class(unreal.SkyLightComponent)
        set_mobility(c)
        try: c.set_editor_property("real_time_capture", True)
        except Exception: pass
        c.set_intensity(1.5)
        try: c.recapture_sky()
        except Exception: pass
        log("  스카이라이트(실시간)")
    except Exception as e: log("  ! 스카이라이트: %s" % e)
spawn(unreal.ExponentialHeightFog, (0, 0, 0))
# 포인트라이트 4개(라이트빌드/캡처 실패해도 무조건 밝게)
for (px, py) in [(0, 0), (1500, 0), (-1200, 1200), (-1200, -1200)]:
    pl = spawn(unreal.PointLight, (px, py, 900))
    if pl:
        try:
            c = pl.get_component_by_class(unreal.PointLightComponent)
            set_mobility(c); c.set_intensity(80000.0); c.set_attenuation_radius(6000.0)
        except Exception: pass
log("  포인트라이트 다발")

# ── 4) PlayerStart + BattleManager ──────────────────────────
spawn(unreal.PlayerStart, (0, 0, 130))
bm = gen_class(BM_BP)
if bm:
    bmi = spawn(bm, (0, 0, 130))
    # ★ 인스턴스에 모던 카드 HUD 직접 배선 (BP CDO는 옛 /Game/Battle/UI/WBP_BattleHUD를 가리킴 → 인스턴스 오버라이드로 교체)
    if bmi:
        h = gen_class("/Game/UI/WBP_BattleHUD")
        d = gen_class("/Game/UI/WBP_DamageNumber")
        try:
            if h: bmi.set_editor_property("BattleHUDClass", h)
            if d: bmi.set_editor_property("DamageNumberClass", d)
        except Exception as ee: log("  ! BM HUD 배선: %s" % ee)
    log("  BattleManager (모던 HUD 인스턴스 배선)")

# ── 5) NPC 헬퍼 ─────────────────────────────────────────────
mesh_fb = EAL.load_asset(MESH_FB) if EAL.does_asset_exist(MESH_FB) else None
COMMON = {"bIsShopkeeper": False, "bCanBeRecruited": False}

def place(bp_path, loc, props, label):
    cls = gen_class(bp_path)
    if cls is None: return None
    npc = spawn(cls, loc, unreal.Rotator(0.0, 0.0, 180.0))  # (roll,pitch,yaw): yaw180 = 플레이어(−X) 바라보게
    if not npc:
        log("  ! 스폰 실패 %s" % label); return None
    # 메시 폴백(BP에 메시 없을 때만 — 모델 다양성 보존)
    try:
        skm = npc.get_editor_property("mesh")
        if skm and mesh_fb and skm.get_skeletal_mesh_asset() is None:
            skm.set_skeletal_mesh_asset(mesh_fb)
            skm.set_relative_location_and_rotation(unreal.Vector(0, 0, -90), unreal.Rotator(0.0, 0.0, -90.0), False, False)
    except Exception: pass
    p = dict(COMMON); p.update(props); p["ActivePhases"] = []
    for k, v in p.items():
        try: npc.set_editor_property(k, v)
        except Exception as e: log("  ! %s.%s: %s" % (label, k, e))
    try: npc.set_actor_label(label)   # 아웃라이너에서 찾기 쉬운 이름
    except Exception: pass
    return npc

def bp(n):  # BP_ANPCCharacter 경로
    return "/Game/BP_Characters/BP_ANPCCharacter%s" % n

# ── 6) 전투 테스트 핵심 배치 ────────────────────────────────
# 아군 3 (플레이어 근처 → 전투 자동합류, 반경 1500u 내)
place(bp(1), (200,  350, 90), {"ArchetypeId":"char_01", "bIsAlly":True,  "bCanEnterCombat":True, "NPCName":"강현"}, "ALLY_1_강현")
place(bp(2), (200, -350, 90), {"ArchetypeId":"char_02", "bIsAlly":True,  "bCanEnterCombat":True, "NPCName":"서연"}, "ALLY_2_서연")
place(bp(3), (350,    0, 90), {"ArchetypeId":"char_03", "bIsAlly":True,  "bCanEnterCombat":True, "NPCName":"은우"}, "ALLY_3_은우")
# 적 그룹 (정면, 약점 다양 → 원모어/바톤/총공격 테스트)
place(bp(4), (750,  -200, 90), {"ArchetypeId":"shadow_lost",  "bIsAlly":False, "bCanEnterCombat":True, "NPCName":"떠도는 그림자(약점:축복)"}, "ENEMY_1_lost_축복약점")
place(bp(5), (850,     0, 90), {"ArchetypeId":"shadow_pyre",  "bIsAlly":False, "bCanEnterCombat":True, "NPCName":"잿불 그림자(약점:빙결)"}, "ENEMY_2_pyre_빙결약점")
place(bp(6), (750,   200, 90), {"ArchetypeId":"shadow_brute", "bIsAlly":False, "bCanEnterCombat":True, "NPCName":"광폭한 그림자(약점:전격)"}, "ENEMY_3_brute_전격약점")
# 보스 (멀리 격리 → 보스 페이즈/분노 테스트). BossLevel 150 = 레이드 스케일.
place(bp(7), (3000, 0, 90), {"ArchetypeId":"shadow_tyrant", "bIsAlly":False, "bCanEnterCombat":True, "BossLevel":150, "NPCName":"폭군 그림자 [보스]"}, "BOSS_tyrant_Lv150")

# ── 7) 모델 쇼케이스 줄 (나머지 모델 전부 — 뒤쪽, 다양성 눈으로 확인. 전투 비참여) ──
# 플레이어 뒤(−X 1200)에 좌우로 늘어놓음. bCanEnterCombat=False → 말만 걸림(전투에 안 끌려옴).
showcase = [str(n) for n in range(8, 24)] + ["capo"]  # BP8~23 + capo
y = -((len(showcase) - 1) * 220) // 2
for idx, n in enumerate(showcase):
    arche = "char_%02d" % (idx + 4) if (idx + 4) <= 28 else ""  # char_04.. 부여(있으면)
    place(bp(n), (-1200, y + idx * 220, 90),
          {"ArchetypeId": arche, "bIsAlly": True, "bCanEnterCombat": False,
           "NPCName": "모델 %s" % n}, "MODEL_%s" % n)
log("  핵심 전투 NPC 7 + 쇼케이스 %d 배치" % len(showcase))

# ── 8) 월드세팅 GameMode + 플레이어 폰 ───────────────────────
try:
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    gm = gen_class(GM_BP)
    if gm: world.get_world_settings().set_editor_property("default_game_mode", gm)
    log("  WorldSettings GameMode=BP_SecretProjectGameMode")
except Exception as e: log("  ! WorldSettings: %s" % e)
try:
    gm_bp = EAL.load_asset(GM_BP); pcls = gen_class(PLAYER)
    cdo = unreal.get_default_object(gm_bp.generated_class())
    if pcls and cdo.get_editor_property("default_pawn_class") != pcls:
        cdo.set_editor_property("default_pawn_class", pcls)
        BEL.compile_blueprint(gm_bp); EAL.save_loaded_asset(gm_bp)
        log("  GameMode.DefaultPawnClass=BP_PlayerCharacter (설정)")
    else: log("  GameMode.DefaultPawnClass 이미 적절")
except Exception as e: log("  ! DefaultPawnClass: %s" % e)

# ── 9) 검증(액터 수) + 저장 ─────────────────────────────────
try:
    actors = EAS.get_all_level_actors()
    log("  레벨 액터 수(저장 전): %d" % len(actors))
except Exception as e: log("  ! 액터 수: %s" % e)
try:
    LES.save_current_level()
    log("9) 저장 완료: %s" % LEVEL_PATH)
except Exception as e:
    log("  ! save_current_level: %s" % e)
    try: EAL.save_asset(LEVEL_PATH); log("  save_asset 폴백 완료")
    except Exception as e2: log("  ! 폴백도 실패: %s" % e2)

log("==== 완료 ====")
