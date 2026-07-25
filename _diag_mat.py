# -*- coding: utf-8 -*-
"""머티리얼 진단 — 레벨의 성분에 실제로 무엇이 걸려 있나, 그 머티리얼은 성한가."""
import unreal, traceback

lines = []


def log(m):
    lines.append(str(m))
    unreal.log("[dmat] " + str(m))


try:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(
        "/Game/Maps/Rasel/L01_Jangteo_Street")
    A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    seen = 0
    for a in A.get_all_level_actors():
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        lbl = a.get_actor_label()
        if lbl not in ("Facade_L8_Home", "Road_8_0", "가로등_L6"):
            continue
        c = a.static_mesh_component
        mats = [c.get_material(i) for i in range(c.get_num_materials())]
        log("%s: %s" % (lbl, [m.get_name() if m else None for m in mats]))
        for m in mats:
            if m is None:
                continue
            log("   %s  class=%s  path=%s" % (m.get_name(), m.get_class().get_name(),
                                              m.get_path_name()))
        seen += 1
    log("검사 %d개" % seen)

    # 머티리얼 에셋 자체
    for nm in ("M_Rasel_Plaster", "M_Rasel_Cobble", "M_Rasel_Wood"):
        p = "/Game/Rasel/Materials/" + nm
        mat = unreal.EditorAssetLibrary.load_asset(p)
        if mat is None:
            log("!! 없음 %s" % p)
            continue
        n = unreal.MaterialEditingLibrary.get_num_material_expressions(mat)
        log("%s: 노드 %d개  blend=%s  shading=%s  twoSided=%s" % (
            nm, n,
            mat.get_editor_property("blend_mode"),
            mat.get_editor_property("shading_model"),
            mat.get_editor_property("two_sided")))
except Exception:
    log("실패\n" + traceback.format_exc())

with open("C:/Secret_Project/Saved/diag_mat.log", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
