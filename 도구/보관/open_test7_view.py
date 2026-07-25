# test7(교체본)을 Map_Main에 띄우고 에디터를 열어둠(종료 안 함). 사용자가 직접 돌려보게.
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
try: les.load_level('/Game/CiciToonCharacterShaderPak/Levels/Map_Main')
except Exception as e: unreal.log("load fail %s" % e)
# 기존 스켈레탈 액터 숨김(하네카와 복제본 등)
for a in eas.get_all_level_actors():
    if a.get_components_by_class(unreal.SkeletalMeshComponent):
        try: a.set_is_temporarily_hidden_in_editor(True)
        except Exception: pass
mesh = unreal.load_asset('/Game/TestCharacter/test7/SK_test7')
actor = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(230, 0, 0), unreal.Rotator(0, 0, 0))
try: actor.set_actor_label('TEST7_SWAPPED')
except Exception: pass
comp = actor.skeletal_mesh_component
try: comp.set_skeletal_mesh_asset(mesh)
except Exception:
    try: comp.set_skeletal_mesh(mesh)
    except Exception as e: unreal.log("set mesh fail %s" % e)
# 텍스처 풀로드 + 카메라 프레이밍(앞에서 약간 위)
w = ues.get_editor_world()
for c in ["r.Streaming.FullyLoadUsedTextures 1", "r.ScreenPercentage 100"]:
    try: unreal.SystemLibrary.execute_console_command(w, c)
    except Exception: pass
try:
    les.editor_set_game_view(True)
    ues.set_level_viewport_camera_info(unreal.Vector(230.0, 320.0, 120.0),
                                       unreal.Rotator(0.0, -90.0, -8.0))
except Exception as e: unreal.log("cam fail %s" % e)
unreal.log("TEST7 VIEW READY")
# QUIT 안 함 — 에디터 열린 상태 유지
