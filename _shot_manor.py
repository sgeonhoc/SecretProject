# -*- coding: utf-8 -*-
"""네르한 저택 검수 촬영 — 그랜드홀을 제대로 보여주는 고정 히어로 앵글.
_shot_any 의 함정 회피(부재 깨우기 + Movable) 를 그대로 쓰되, 큰 홀 스케일에 맞춘 카메라.
실행: -ExecutePythonScript=_shot_manor.py
"""
import os, math, traceback, unreal

MAP = "/Game/Maps/Rasel/L15_Nerhan_Manor"
TAG = os.environ.get("SHOT_TAG", "manor")
OUTDIR = "C:/Secret_Project/Saved/shots"
W, H = 1600, 900


def log(m):
    unreal.log("[shotmanor] " + str(m))
    with open("C:/Secret_Project/Saved/shot.log", "a", encoding="utf-8") as f:
        f.write(str(m) + "\n")


def A():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def look_at(eye, tgt):
    d = unreal.Vector(tgt[0] - eye[0], tgt[1] - eye[1], tgt[2] - eye[2])
    return unreal.Rotator(0.0,
                          math.degrees(math.atan2(d.z, math.sqrt(d.x * d.x + d.y * d.y))),
                          math.degrees(math.atan2(d.y, d.x)))


EXPO = float(os.environ.get("EXPO", "1.0"))   # 월드 PostProcessVolume 에 박는 수동노출 보정(EV)
_PPV = {"a": None}


def find_world_ppv():
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.PostProcessVolume):
            _PPV["a"] = a
            return a
    return None


def set_world_exposure(expo):
    a = _PPV["a"]
    if a is None:
        return
    st = a.get_editor_property("settings")
    st.set_editor_property("override_auto_exposure_method", True)
    st.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
    st.set_editor_property("override_auto_exposure_bias", True)
    st.set_editor_property("auto_exposure_bias", expo)
    a.set_editor_property("settings", st)


def capture(name, eye, tgt, fov=75.0, expo=None):
    if expo is None:
        expo = EXPO
    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    rt = unreal.RenderingLibrary.create_render_target2d(w, W, H, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    cap = A().spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
    rot = look_at(eye, tgt)
    cap.set_actor_location_and_rotation(unreal.Vector(*eye), rot, False, False)
    c = cap.get_component_by_class(unreal.SceneCaptureComponent2D)
    c.set_world_location_and_rotation(unreal.Vector(*eye), rot, False, False)
    c.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    c.set_editor_property("texture_target", rt)
    c.set_editor_property("fov_angle", fov)
    c.set_editor_property("capture_every_frame", False)
    set_world_exposure(expo)
    # ★단일 capture_scene 은 라이팅/노출이 수렴 전이라 프레임이 깨진다 — 여러 번 돌려 수렴시킨다.
    for _ in range(16):
        c.capture_scene()
    unreal.RenderingLibrary.export_render_target(w, rt, OUTDIR, "%s_%s.png" % (TAG, name))
    A().destroy_actor(cap)
    log("  · %s_%s.png" % (TAG, name))


def run():
    if not os.path.isdir(OUTDIR):
        os.makedirs(OUTDIR)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
    log("레벨 로드: %s" % MAP)
    log("월드 PP 볼륨: %s" % ("찾음" if find_world_ppv() else "없음"))
    # 자동노출/눈적응을 끄고 고정 노출로 — 헤드리스 단일캡처의 노출 요동을 없앤다
    for cmd in ("r.EyeAdaptationQuality 0", "r.DefaultFeature.AutoExposure 0",
                "r.Lumen.DiffuseIndirect.Allow 1", "r.SkylightIntensity 1"):
        try:
            unreal.SystemLibrary.execute_console_command(None, cmd)
        except Exception:
            pass
    used = {}
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh:
            sm = a.static_mesh_component.static_mesh
            used[sm.get_path_name()] = sm
            a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    for k, (p, sm) in enumerate(used.items()):
        d = A().spawn_actor_from_object(sm, unreal.Vector(0, 0, -9000 - k * 60), unreal.Rotator(0, 0, 0))
        if d:
            d.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
            d.set_actor_label("_깨우기_%d" % k)
    log("부재 깨우기 %d종" % len(used))

    # ── 노출 스윕(길이34 앵글로 한 방에 맞는 값 찾기) ──
    if os.environ.get("SWEEP", "0") == "1":
        for e in (-10, -7, -4, 0):
            capture("sweep%+d" % e, (-4200, -1250, 1150), (2200, 300, 560), 80, expo=e)
        log("스윕 완료")
        return

    # ── 히어로 앵글(홀: X -4800..5410, 바닥 0, 천장 2000) ──
    capture("length",   (-4300, 0, 300),      (3200, 0, 700),    72)   # 콜로네이드 길이로
    capture("length34", (-4200, -1250, 1150), (2200, 300, 560),  80)   # 3/4 부감(천장·기둥·바닥)
    capture("corner",   (-3600, -1400, 460),  (2600, 900, 520),  82)   # 코너에서 대각
    capture("stair",    (700, 0, 360),        (3600, 0, 1050),   74)   # 대계단 정면
    capture("window",   (-1400, 950, 320),    (-1400, -1650, 820), 84) # 창측 신광
    capture("carpet",   (-4300, 0, 220),      (2000, 0, 260),    70)   # 바닥 카펫 러너 결
    log("=== 촬영 완료 ===")


open("C:/Secret_Project/Saved/shot.log", "w", encoding="utf-8").close()
try:
    run()
except Exception:
    log("!! 예외:\n" + traceback.format_exc())
