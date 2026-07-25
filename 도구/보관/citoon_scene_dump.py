# headless 덤프(읽기 전용): 씬 라이팅/노출/머티리얼 셰이딩모델 → C:/Secret_Project/기획/99_보관/구로그/citoon_scene_dump.txt
import unreal

OUT = "C:/Secret_Project/기획/99_보관/구로그/citoon_scene_dump.txt"
out = []
def w(s):
    s = str(s); out.append(s)
    try: unreal.log("DUMP> " + s)
    except Exception: pass
def flush():
    try:
        with open(OUT, "w", encoding="utf-8") as f: f.write("\n".join(out))
    except Exception: pass
def gp(o, n):
    try: return o.get_editor_property(n)
    except Exception as e: return "ERR(%s)" % e

EAL = unreal.EditorAssetLibrary
w("================ CITOON SCENE DUMP ================")

# [1] 머티리얼 셰이딩 모델
w("")
w("[1] MATERIALS")
for p in ['/Game/CiciToonCharacterShaderPak/Stage/Materials/M_CiToon_Default',
          '/Game/CiciToonCharacterShaderPak/Stage/Materials/M_Outline']:
    if EAL.does_asset_exist(p):
        m = unreal.load_asset(p)
        w("  %s: shading_model=%s blend_mode=%s two_sided=%s" % (p.split('/')[-1], gp(m,'shading_model'), gp(m,'blend_mode'), gp(m,'two_sided')))
    else:
        w("  %s: NOT FOUND" % p)

# [2] CharacterExposure PP 자산 존재
w("")
w("[2] CharacterExposure PP 자산")
for p in ['/Game/OUTLINE/M_CharacterExposure_PP','/Game/OUTLINE/M_CharacterExposure_PP_Inst',
          '/Game/OUTLINE/M_CharacterExposure_PP_Inst2','/Game/M_CharacterExposure_PP_Inst']:
    w("  %s exists=%s" % (p, EAL.does_asset_exist(p)))

# [3] Showcase 레벨 로드
w("")
w("[3] LOAD Showcase")
loaded = False
try:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/CityPark/Maps/Showcase')
    loaded = True; w("  loaded (LevelEditorSubsystem)")
except Exception as e:
    w("  LevelEditorSubsystem 실패: %s" % e)
if not loaded:
    try:
        unreal.EditorLevelLibrary.load_level('/Game/CityPark/Maps/Showcase'); loaded = True; w("  loaded (EditorLevelLibrary)")
    except Exception as e:
        w("  EditorLevelLibrary 실패: %s" % e)
flush()

# [4] 액터 조사
w("")
w("[4] ACTORS")
def expo(ppset, tag):
    for fld in ['override_auto_exposure_method','auto_exposure_method',
                'override_auto_exposure_bias','auto_exposure_bias',
                'override_auto_exposure_min_brightness','auto_exposure_min_brightness',
                'override_auto_exposure_max_brightness','auto_exposure_max_brightness',
                'override_auto_exposure_apply_physical_camera_exposure']:
        w("      [%s] %s = %s" % (tag, fld, gp(ppset, fld)))
try:
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = list(eas.get_all_level_actors())
    w("  actor count = %d" % len(actors))
    for a in actors:
        try:
            for c in a.get_components_by_class(unreal.DirectionalLightComponent):
                w("  [DirLight] %s intensity=%s cast_dyn=%s contact_len=%s" % (a.get_actor_label(), gp(c,'intensity'), gp(c,'cast_dynamic_shadows'), gp(c,'contact_shadow_length')))
            for c in a.get_components_by_class(unreal.SkyLightComponent):
                w("  [SkyLight] %s intensity=%s real_time_capture=%s lower_hemi_black=%s" % (a.get_actor_label(), gp(c,'intensity'), gp(c,'real_time_capture'), gp(c,'lower_hemisphere_is_black')))
            for c in a.get_components_by_class(unreal.SkyAtmosphereComponent):
                w("  [SkyAtmosphere] %s" % a.get_actor_label())
            for c in a.get_components_by_class(unreal.ExponentialHeightFogComponent):
                w("  [HeightFog] %s" % a.get_actor_label())
            for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
                try: sk = c.get_skeletal_mesh_asset(); nm = sk.get_name() if sk else ''
                except Exception: nm = ''
                if 'VRM' in nm:
                    w("  [VRM] %s cast_shadow=%s affect_indirect=%s affect_df=%s" % (a.get_actor_label(), gp(c,'cast_shadow'), gp(c,'affect_dynamic_indirect_lighting'), gp(c,'affect_distance_field_lighting')))
            if isinstance(a, unreal.PostProcessVolume):
                w("  [PPV] %s unbound=%s priority=%s enabled=%s" % (a.get_actor_label(), gp(a,'unbound'), gp(a,'priority'), gp(a,'enabled')))
                ps = gp(a,'settings')
                if not isinstance(ps, str): expo(ps, a.get_actor_label())
        except Exception as e:
            w("  actor '%s' 예외: %s" % (gp(a,'actor_label'), e))
except Exception as e:
    w("  액터 조사 실패: %s" % e)

w("")
w("================ DONE ================")
flush()

# headless 자동 종료(에디터 안 닫히면 프로젝트 잠김)
try:
    world = None
    try: world = unreal.EditorLevelLibrary.get_editor_world()
    except Exception: pass
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
except Exception:
    pass
