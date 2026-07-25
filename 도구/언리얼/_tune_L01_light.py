# -*- coding: utf-8 -*-
"""L01 빛 고르기 — 같은 자리에서 프리셋을 바꿔 가며 찍어 눈으로 고른다.
(레벨은 저장하지 않는다. 고른 값만 빌더에 옮겨 적는다.)"""
import math, os, traceback, unreal

OUTDIR = "C:/Secret_Project/Saved/shots"
lines = []


def log(m):
    lines.append(str(m))
    unreal.log("[tune] " + str(m))


def A():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def look_at(eye, tgt):
    d = unreal.Vector(tgt[0] - eye[0], tgt[1] - eye[1], tgt[2] - eye[2])
    return unreal.Rotator(0.0,
                          math.degrees(math.atan2(d.z, math.sqrt(d.x * d.x + d.y * d.y))),
                          math.degrees(math.atan2(d.y, d.x)))


def shot(name, eye, tgt, fov=80.0):
    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    rt = unreal.RenderingLibrary.create_render_target2d(
        w, 1280, 720, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    cap = A().spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
    cap.set_actor_location_and_rotation(unreal.Vector(*eye), look_at(eye, tgt), False, False)
    c = cap.get_component_by_class(unreal.SceneCaptureComponent2D)
    c.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    c.set_editor_property("texture_target", rt)
    c.set_editor_property("fov_angle", fov)
    c.set_editor_property("capture_every_frame", False)
    c.capture_scene()
    unreal.RenderingLibrary.export_render_target(w, rt, OUTDIR, "%s.png" % name)
    A().destroy_actor(cap)


def apply(pitch, yaw, sun, sky_i, fog_d, bias, sun_col=(1.0, 0.86, 0.68)):
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.DirectionalLight):
            a.set_actor_rotation(unreal.Rotator(0.0, pitch, yaw), False)
            c = a.get_component_by_class(unreal.DirectionalLightComponent)
            c.set_mobility(unreal.ComponentMobility.MOVABLE)
            c.set_intensity(sun)
            c.set_light_color(unreal.LinearColor(*sun_col))
        elif isinstance(a, unreal.SkyLight):
            c = a.get_component_by_class(unreal.SkyLightComponent)
            c.set_mobility(unreal.ComponentMobility.MOVABLE)
            c.set_editor_property("real_time_capture", True)
            c.set_editor_property("intensity", sky_i)
            try:
                c.recapture_sky()
            except Exception:
                pass
        elif isinstance(a, unreal.ExponentialHeightFog):
            c = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
            c.set_editor_property("fog_density", fog_d)
        elif isinstance(a, unreal.PostProcessVolume):
            s = a.get_editor_property("settings")
            s.set_editor_property("override_auto_exposure_method", True)
            s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
            s.set_editor_property("override_auto_exposure_bias", True)
            s.set_editor_property("auto_exposure_bias", bias)
            a.set_editor_property("settings", s)


# (이름, 해 pitch, 해 yaw, 해 lux, 하늘, 안개, 노출)
PRESETS = [
    ("P1", -30.0, 15.0, 5.0, 2.0, 0.010, 9.0),
    ("P2", -22.0, -25.0, 9.0, 3.0, 0.007, 8.4),
    ("P3", -45.0, 35.0, 12.0, 3.0, 0.005, 7.8),
    ("P4", -12.0, 8.0, 3.0, 1.6, 0.012, 10.2),
]

try:
    if not os.path.isdir(OUTDIR):
        os.makedirs(OUTDIR)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(
        "/Game/Maps/Rasel/L01_Jangteo_Street")
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component:
            a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    for (nm, p, y, s, sk, fg, bi) in PRESETS:
        apply(p, y, s, sk, fg, bi)
        shot("TUNE_%s_street" % nm, (-420, 60, 168), (4400, -40, 150), 82)
        shot("TUNE_%s_facade" % nm, (3200 - 180, 40, 190), (3200, 600, 330), 70)
        log("찍음 %s (해 %.0f lux p%.0f y%.0f · 하늘 %.1f · 안개 %.3f · 노출 %.1f)"
            % (nm, s, p, y, sk, fg, bi))
except Exception:
    log("실패\n" + traceback.format_exc())

with open("C:/Secret_Project/Saved/tune.log", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
