# -*- coding: utf-8 -*-
"""L01 진단 — 액터가 있는데 안 보이는 이유 찾기."""
import unreal, traceback

MAP = "/Game/Maps/Rasel/L01_Jangteo_Street"
lines = []


def log(m):
    lines.append(str(m))
    unreal.log("[diag] " + str(m))


def A():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


try:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
    counts = {}
    sample = []
    lo = [1e9] * 3
    hi = [-1e9] * 3
    for a in A().get_all_level_actors():
        cn = a.get_class().get_name()
        counts[cn] = counts.get(cn, 0) + 1
        if isinstance(a, unreal.StaticMeshActor):
            o, e = a.get_actor_bounds(False)
            lo = [min(lo[0], o.x - e.x), min(lo[1], o.y - e.y), min(lo[2], o.z - e.z)]
            hi = [max(hi[0], o.x + e.x), max(hi[1], o.y + e.y), max(hi[2], o.z + e.z)]
            if len(sample) < 6:
                smc = a.static_mesh_component
                sm = smc.static_mesh
                sample.append("%s | mesh=%s | loc=(%.0f,%.0f,%.0f) ext=(%.0f,%.0f,%.0f) | vis=%s | mats=%d" % (
                    a.get_actor_label(), sm.get_name() if sm else "None",
                    o.x, o.y, o.z, e.x, e.y, e.z,
                    smc.is_visible(), smc.get_num_materials()))
    log("클래스별: %s" % counts)
    log("전체 경계 lo=%s hi=%s" % ([int(v) for v in lo], [int(v) for v in hi]))
    for s in sample:
        log("  " + s)

    # 메시 에셋 자체 검사
    for nm in ("SM_Rasel_Facade_Home", "SM_Rasel_RoadTile", "SM_Rasel_Lantern"):
        p = "/Game/Rasel/Meshes/" + nm
        sm = unreal.EditorAssetLibrary.load_asset(p)
        if sm is None:
            log("  !! 없음 %s" % p)
            continue
        b = sm.get_bounds()
        log("  에셋 %s: bounds box_ext=(%.0f,%.0f,%.0f) tris=%s nanite=%s" % (
            nm, b.box_extent.x, b.box_extent.y, b.box_extent.z,
            sm.get_num_triangles(0) if hasattr(sm, "get_num_triangles") else "?",
            sm.get_editor_property("nanite_settings").enabled))

    # 조명 실제 값
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.DirectionalLight):
            c = a.get_component_by_class(unreal.DirectionalLightComponent)
            log("해: intensity=%.2f rot=%s mobility=%s" % (
                c.get_editor_property("intensity"), a.get_actor_rotation(),
                c.get_editor_property("mobility")))
        if isinstance(a, unreal.ExponentialHeightFog):
            c = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
            log("안개: density=%.4f falloff=%.3f" % (
                c.get_editor_property("fog_density"),
                c.get_editor_property("fog_height_falloff")))
        if isinstance(a, unreal.PostProcessVolume):
            s = a.get_editor_property("settings")
            log("PPV: bias=%.2f method=%s unbound=%s" % (
                s.get_editor_property("auto_exposure_bias"),
                s.get_editor_property("auto_exposure_method"),
                a.get_editor_property("unbound")))
except Exception:
    log("실패\n" + traceback.format_exc())

with open("C:/Secret_Project/Saved/diag_l01.log", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
