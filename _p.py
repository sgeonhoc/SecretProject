import unreal, collections
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level("/Game/Title")
A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cls = collections.Counter(); ours = 0; other_labels = []
for a in A.get_all_level_actors():
    cn = a.get_class().get_name(); cls[cn] += 1
    mine = False
    if isinstance(a, unreal.StaticMeshActor):
        sm = a.static_mesh_component.static_mesh
        if sm and sm.get_path_name().startswith("/Game/Rasel/Meshes/"):
            mine = True
    if mine:
        ours += 1
    elif cn not in ("StaticMeshActor",):
        if len(other_labels) < 60:
            other_labels.append("%s | %s" % (cn, a.get_actor_label()))
out = ["Title 액터 총 %d개 · 우리 라셀 메시 액터 %d개" % (sum(cls.values()), ours), "", "클래스별:"]
for k, v in cls.most_common():
    out.append("  %-34s %d" % (k, v))
out += ["", "라셀 메시가 아닌 액터 라벨(앞 60개):"] + ["  " + s for s in other_labels]
open("C:/Secret_Project/Saved/p.log","w",encoding="utf-8").write("\n".join(out))
