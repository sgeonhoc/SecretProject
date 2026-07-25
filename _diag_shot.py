# -*- coding: utf-8 -*-
"""촬영 진단 — ①대조군(L04, 예전 하네스가 찍혔던 레벨)을 내 촬영 함수로 ②L01에서 안개·노출 변수를 하나씩 빼며."""
import os, math, traceback, unreal

OUTDIR = "C:/Secret_Project/Saved/shots"
lines = []


def log(m):
    lines.append(str(m))
    unreal.log("[dshot] " + str(m))


def A():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def look_at(eye, tgt):
    d = unreal.Vector(tgt[0] - eye[0], tgt[1] - eye[1], tgt[2] - eye[2])
    return unreal.Rotator(0.0,
                          math.degrees(math.atan2(d.z, math.sqrt(d.x * d.x + d.y * d.y))),
                          math.degrees(math.atan2(d.y, d.x)))


def capture(name, eye, tgt, fov=75.0):
    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    rt = unreal.RenderingLibrary.create_render_target2d(
        w, 1280, 720, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    cap = A().spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(*eye),
                                     look_at(eye, tgt))
    c = cap.get_component_by_class(unreal.SceneCaptureComponent2D)
    c.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    c.set_editor_property("texture_target", rt)
    c.set_editor_property("fov_angle", fov)
    c.set_editor_property("capture_every_frame", False)
    c.capture_scene()
    unreal.RenderingLibrary.export_render_target(w, rt, OUTDIR, "%s.png" % name)
    A().destroy_actor(cap)
    p = os.path.join(OUTDIR, "%s.png" % name)
    log("  %s -> %d bytes" % (name, os.path.getsize(p) if os.path.exists(p) else -1))


try:
    LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

    # ① 대조군 — 예전에 잘 찍혔던 L04
    LES.load_level("/Game/Maps/Rasel/L04_Dolgan_Office")
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component:
            a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    capture("DIAG_L04_ctrl", (0, -700, 160), (0, 0, 130), 85)

    # ② L01 — 있는 그대로
    LES.load_level("/Game/Maps/Rasel/L01_Jangteo_Street")
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component:
            a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    capture("DIAG_L01_asis", (-420, 60, 168), (4400, -40, 150), 82)

    # ③ 안개 제거
    for a in list(A().get_all_level_actors()):
        if isinstance(a, unreal.ExponentialHeightFog):
            A().destroy_actor(a)
    capture("DIAG_L01_nofog", (-420, 60, 168), (4400, -40, 150), 82)

    # ④ 자동 노출로 교체
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.PostProcessVolume):
            s = a.get_editor_property("settings")
            s.set_editor_property("override_auto_exposure_method", True)
            s.set_editor_property("auto_exposure_method",
                                  unreal.AutoExposureMethod.AEM_HISTOGRAM)
            s.set_editor_property("override_auto_exposure_bias", True)
            s.set_editor_property("auto_exposure_bias", 1.0)
            a.set_editor_property("settings", s)
    capture("DIAG_L01_autoexp", (-420, 60, 168), (4400, -40, 150), 82)

    # ⑤ 아주 가까이 — 파사드 코앞
    capture("DIAG_L01_close", (3100, 300, 200), (3200, 600, 250), 70)

    # ⑥ 위에서 내려다보기
    capture("DIAG_L01_top", (2200, 0, 3000), (2200, 0, 0), 80)

except Exception:
    log("실패\n" + traceback.format_exc())

with open("C:/Secret_Project/Saved/diag_shot.log", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
