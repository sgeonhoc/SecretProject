# 깨끗한 SK_test2(=플레이어 캐릭터의 비-Mixamo 버전)를 Showcase에 세워 A/B 비교.
#   SK_test2는 test2 머티리얼(이미 셀튜닝됨)을 그대로 쓰므로 '깨끗한 메시 + 셀MToon' 결과를 바로 보여줌.
import unreal
LOG = "C:/Secret_Project/기획/99_보관/구로그/citoon_place_log.txt"; log = []
def w(s):
    s = str(s); log.append(s)
def flush():
    try: open(LOG, "w", encoding="utf-8").write("\n".join(log))
    except Exception: pass

world = None
try:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/CityPark/Maps/Showcase')
    world = unreal.EditorLevelLibrary.get_editor_world()
    w("loaded Showcase")
except Exception as e:
    w("load fail %s" % e)

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# 기준 위치 = 배치된 SK_VRM 액터(플레이어 메시) 옆
base = None
for a in eas.get_all_level_actors():
    for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
        try: sk = c.get_skeletal_mesh_asset(); nm = sk.get_name() if sk else ''
        except Exception: nm = ''
        if nm == 'SK_VRM' and a.get_actor_label() == 'SK_VRM':
            base = a.get_actor_location(); w("기준 SK_VRM loc=%s" % base)
            break
    if base: break
if base is None:
    base = unreal.Vector(0, 0, 100); w("SK_VRM 못 찾음 — 기본 위치 사용")

spawn_loc = unreal.Vector(base.x, base.y + 150.0, base.z)
sk2 = unreal.load_asset('/Game/TestCharacter/test2/SK_test2')
if not sk2:
    w("!! SK_test2 로드 실패"); flush()
else:
    actor = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, spawn_loc, unreal.Rotator(0, 0, 0))
    comp = actor.get_components_by_class(unreal.SkeletalMeshComponent)
    comp = comp[0] if comp else None
    if comp:
        ok = False
        try: comp.set_skeletal_mesh_asset(sk2); ok = True
        except Exception:
            try: comp.set_editor_property('skeletal_mesh_asset', sk2); ok = True
            except Exception:
                try: comp.set_editor_property('skeletal_mesh', sk2); ok = True
                except Exception as e: w("mesh 세팅 실패: %s" % e)
        # NPC와 동일 조건: 노출PP 혜택 받게
        try:
            comp.set_editor_property('render_custom_depth', True)
            comp.set_editor_property('custom_depth_stencil_value', 1)
        except Exception: pass
        try: actor.set_actor_label('CLEAN_test2_COMPARE')
        except Exception: pass
        w("배치 완료: SK_test2 @ %s (mesh_set=%s)" % (spawn_loc, ok))
    else:
        w("스폰 액터에 SkeletalMeshComponent 없음")
    try:
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
        w("레벨 저장됨")
    except Exception as e:
        w("레벨 저장 실패: %s" % e)
    flush()

try:
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
except Exception: pass
