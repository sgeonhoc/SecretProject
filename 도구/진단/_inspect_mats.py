# -*- coding: utf-8 -*-
# L04 액터별 머티리얼 슬롯0 실제 할당 확인 — 체커보드가 우리 머티리얼인지 null 폴백인지 판별.
import unreal
MAP = "/Game/Maps/Rasel/L04_Dolgan_Office"
def log(m):
    unreal.log("[insp] " + m)
    with open("C:/Secret_Project/Saved/insp.log", "a", encoding="utf-8") as f:
        f.write(m + "\n")
open("C:/Secret_Project/Saved/insp.log", "w", encoding="utf-8").close()
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level(MAP)
asys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
from collections import Counter
c = Counter()
n = 0
for a in asys.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    smc = a.static_mesh_component
    mat = smc.get_material(0)
    matname = mat.get_name() if mat else "<NULL>"
    c[matname] += 1
    if n < 12:
        log("%s -> mat0=%s" % (a.get_actor_label(), matname))
    n += 1
log("--- 머티리얼별 액터 수 ---")
for k, v in c.most_common():
    log("  %s : %d" % (k, v))
log("총 StaticMeshActor %d" % n)
