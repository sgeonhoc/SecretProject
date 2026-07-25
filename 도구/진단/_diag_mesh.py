# -*- coding: utf-8 -*-
"""갈라보기 6 — 베이스컬러로 찍어 '기하가 없나 / 빛이 없나'를 못 박는다."""
import math, traceback, unreal

lines = []


def log(m):
    lines.append(str(m))
    unreal.log("[dm6] " + str(m))


def A():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def look_at(eye, tgt):
    d = unreal.Vector(tgt[0] - eye[0], tgt[1] - eye[1], tgt[2] - eye[2])
    return unreal.Rotator(0.0,
                          math.degrees(math.atan2(d.z, math.sqrt(d.x * d.x + d.y * d.y))),
                          math.degrees(math.atan2(d.y, d.x)))


def shot(name, eye, tgt, src, fov=82.0):
    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    rt = unreal.RenderingLibrary.create_render_target2d(
        w, 1280, 720, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    cap = A().spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
    cap.set_actor_location_and_rotation(unreal.Vector(*eye), look_at(eye, tgt), False, False)
    c = cap.get_component_by_class(unreal.SceneCaptureComponent2D)
    c.set_editor_property("capture_source", src)
    c.set_editor_property("texture_target", rt)
    c.set_editor_property("fov_angle", fov)
    c.set_editor_property("capture_every_frame", False)
    c.capture_scene()
    unreal.RenderingLibrary.export_render_target(w, rt, "C:/Secret_Project/Saved/shots",
                                                 "%s.png" % name)
    A().destroy_actor(cap)
    log("  찍음 %s" % name)


try:
    srcs = [n for n in dir(unreal.SceneCaptureSource) if n.startswith("SCS_")]
    log("capture_source 목록: %s" % ", ".join(srcs))

    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(
        "/Game/Maps/Rasel/L01_Jangteo_Street")
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component:
            a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)

    S = unreal.SceneCaptureSource
    shot("DM6_street_base", (-420, 60, 168), (4400, -40, 150), S.SCS_BASE_COLOR)
    shot("DM6_street_normal", (-420, 60, 168), (4400, -40, 150), S.SCS_NORMAL)
    shot("DM6_near_base", (3200, -400, 260), (3200, 700, 300), S.SCS_BASE_COLOR)
    shot("DM6_street_final", (-420, 60, 168), (4400, -40, 150), S.SCS_FINAL_COLOR_LDR)

    # 해가 실제로 어디를 보고 있나
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.DirectionalLight):
            c = a.get_component_by_class(unreal.DirectionalLightComponent)
            log("해 rot=%s intensity=%.2f mob=%s castShadow=%s affectWorld=%s vis=%s" % (
                a.get_actor_rotation(), c.get_editor_property("intensity"),
                c.get_editor_property("mobility"),
                c.get_editor_property("cast_shadows"),
                c.get_editor_property("affects_world"),
                c.get_editor_property("visible")))
except Exception:
    log("실패\n" + traceback.format_exc())

with open("C:/Secret_Project/Saved/diag_mesh.log", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
