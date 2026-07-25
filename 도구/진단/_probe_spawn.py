# -*- coding: utf-8 -*-
# 액터가 __ExternalActors__/Untitled 로 새는 원인 추적 (World Partition / OFPA 의심)
import unreal

OUT = "C:/Secret_Project/Saved/probe_spawn.txt"


def step(m):
    with open(OUT, "a", encoding="utf-8") as f:
        f.write(str(m) + "\n")


open(OUT, "w", encoding="utf-8").close()

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
acts = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

les.new_level("/Game/Maps/_probe2")
w = ues.get_editor_world()
step("world name after new_level: %s" % w.get_name())
step("world path: %s" % w.get_path_name())
try:
    step("is_partitioned: %s" % (w.get_world_partition() is not None))
except Exception as e:
    step("get_world_partition err: %s" % e)

a = acts.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
a.static_mesh_component.set_static_mesh(
    unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube"))
step("actor package: %s" % a.get_package().get_path_name())
step("actor is_package_external: %s" % a.is_package_external())

les.save_current_level()
step("saved")
saved = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
step("save_dirty_packages: %s" % saved)
