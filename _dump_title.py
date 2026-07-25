# -*- coding: utf-8 -*-
"""타이틀 맵에 무엇이 서 있나 — 특히 **외부 샘플 에셋**이 쓰이고 있는지 가려낸다.
사용자 규율: 프로젝트 Content 안 서드파티 팩(Asian_Village·Fab·CityPark·Free_Magic 등)은
저작권 미확인이라 게임 화면에 못 쓴다. 타이틀은 게임의 첫 화면이라 여기부터 깨끗해야 한다.
"""
import unreal, json

OURS = ("/Game/Rasel/", "/Game/Maps/", "/Engine/BasicShapes/", "/Engine/EngineMeshes/",
        "/Game/UI/", "/Game/Audio/")
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES.load_level("/Game/Title")

rows = []
for a in A.get_all_level_actors():
    cn = a.get_class().get_name()
    row = {"cls": cn, "label": a.get_actor_label(), "assets": []}
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        if c.static_mesh:
            row["assets"].append(c.static_mesh.get_path_name())
    for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
        sk = c.get_editor_property("skeletal_mesh_asset")
        if sk:
            row["assets"].append(sk.get_path_name())
    try:
        for c in a.get_components_by_class(unreal.NiagaraComponent):
            sys = c.get_editor_property("asset")
            if sys:
                row["assets"].append(sys.get_path_name())
    except Exception:
        pass
    rows.append(row)

for r in rows:
    mark = ""
    for p in r["assets"]:
        if not p.startswith(OURS) and not p.startswith("/Engine/"):
            mark = "  ← 외부?"
    unreal.log("[title] %-28s %-30s %s%s" % (r["cls"], r["label"], ", ".join(r["assets"])[:110], mark))

with open("C:/Secret_Project/Saved/title_actors.json", "w", encoding="utf-8") as f:
    json.dump(rows, f, ensure_ascii=False, indent=1)
unreal.log("[title] 액터 %d" % len(rows))
