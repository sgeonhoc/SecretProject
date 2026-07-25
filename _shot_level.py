# -*- coding: utf-8 -*-
# ▶ 헤드리스 레벨 스크린샷 하네스 — SceneCapture2D → 렌더타깃 → PNG.
#   레벨 진입 후 액터 경계로 자동 프레이밍, 3각도(상공/눈높이/반대) 캡처.
#   불빛 안 구운 블록아웃도 보이게 임시 Movable 태양광+스카이라이트 추가(저장 안 함).
#   실행: SHOT_MAP=/Game/Maps/Rasel/L04_Dolgan_Office SHOT_OUT=L04  UnrealEditor-Cmd ... -ExecutePythonScript=_shot_level.py -RenderOffScreen
import os
import math
import traceback
import unreal

# MSYS(Git Bash)가 /Game/... 을 C:/Program Files/Git/Game/... 으로 변환해버려도 복원.
_raw = os.environ.get("SHOT_MAP", "/Game/Maps/Rasel/L01_Jangteo_Street")
_i = _raw.find("/Game/")
MAP = _raw[_i:] if _i >= 0 else _raw
OUT = os.environ.get("SHOT_OUT", "shot")
OUTDIR = "C:/Secret_Project/Saved/shots"
W, H = 1280, 720

def log(m):
    unreal.log("[shot] " + m)
    with open("C:/Secret_Project/Saved/shot.log", "a", encoding="utf-8") as f:
        f.write(m + "\n")

def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

def actor_sys():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def load_map():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level(MAP)
    log("레벨 로드: " + MAP)

def scene_bounds():
    lo = unreal.Vector(1e9, 1e9, 1e9)
    hi = unreal.Vector(-1e9, -1e9, -1e9)
    n = 0
    for a in actor_sys().get_all_level_actors():
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        origin, ext = a.get_actor_bounds(False)
        for s in (-1, 1):
            p = unreal.Vector(origin.x + s * ext.x, origin.y + s * ext.y, origin.z + s * ext.z)
            lo = unreal.Vector(min(lo.x, p.x), min(lo.y, p.y), min(lo.z, p.z))
            hi = unreal.Vector(max(hi.x, p.x), max(hi.y, p.y), max(hi.z, p.z))
        n += 1
    center = unreal.Vector((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, (lo.z + hi.z) / 2)
    ext = unreal.Vector((hi.x - lo.x) / 2, (hi.y - lo.y) / 2, (hi.z - lo.z) / 2)
    log("경계: 액터 %d, center(%.0f,%.0f,%.0f) ext(%.0f,%.0f,%.0f)" %
        (n, center.x, center.y, center.z, ext.x, ext.y, ext.z))
    return center, ext

def add_capture_lights():
    # 임시 Movable 태양광 + 스카이 — 불빛 안 구운 씬도 보이게(저장 안 하므로 원본 조명 안 건드림)
    dl = actor_sys().spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 3000))
    if dl:
        dl.set_actor_rotation(unreal.Rotator(0, -50, -40), False)
        c = dl.get_component_by_class(unreal.DirectionalLightComponent)
        c.set_mobility(unreal.ComponentMobility.MOVABLE)
        c.set_intensity(3.0)
    sky = actor_sys().spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1500))
    if sky:
        sc = sky.get_component_by_class(unreal.SkyLightComponent)
        sc.set_mobility(unreal.ComponentMobility.MOVABLE)
        sc.set_editor_property("real_time_capture", True)
        sc.set_intensity(0.3)
    # ★레벨 자체의 점광/스포트를 Movable로 돌려 라이트맵 없이도 실내가 켜지게(원본 저장 안 함).
    flipped = 0
    for a in actor_sys().get_all_level_actors():
        lc = None
        if isinstance(a, unreal.PointLight):
            lc = a.get_component_by_class(unreal.PointLightComponent)
        elif isinstance(a, unreal.SpotLight):
            lc = a.get_component_by_class(unreal.SpotLightComponent)
        if lc:
            lc.set_mobility(unreal.ComponentMobility.MOVABLE)
            flipped += 1
    log("레벨 점광 Movable 전환: %d" % flipped)
    # ★스태틱 메시도 Movable로 — 라이트맵 안 구운 프리뷰의 체커 아티팩트 제거(원본 저장 안 함).
    meshed = 0
    for a in actor_sys().get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor):
            smc = a.static_mesh_component
            if smc:
                smc.set_mobility(unreal.ComponentMobility.MOVABLE)
                meshed += 1
    log("메시 Movable 전환: %d" % meshed)
    # ★검수용: 레벨의 어두운 무드 PPV(수동 노출)를 자동노출로 덮어써 어떤 레벨도 보이게(원본 저장 안 함).
    ppvs = 0
    for a in actor_sys().get_all_level_actors():
        if isinstance(a, unreal.PostProcessVolume):
            s = a.get_editor_property("settings")
            s.set_editor_property("override_auto_exposure_method", True)
            s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
            s.set_editor_property("override_auto_exposure_bias", True)
            s.set_editor_property("auto_exposure_bias", 1.0)
            s.set_editor_property("override_auto_exposure_min_brightness", True)
            s.set_editor_property("auto_exposure_min_brightness", 0.05)
            s.set_editor_property("override_auto_exposure_max_brightness", True)
            s.set_editor_property("auto_exposure_max_brightness", 2.0)
            a.set_editor_property("settings", s)
            ppvs += 1
    log("PPV 자동노출 오버라이드: %d" % ppvs)
    return dl, sky

def capture(center, ext, name, pose, fov=75.0):
    world_ref = world()
    rt = unreal.RenderingLibrary.create_render_target2d(world_ref, W, H, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    cap = actor_sys().spawn_actor_from_class(unreal.SceneCapture2D, pose[0], pose[1])
    comp = cap.get_component_by_class(unreal.SceneCaptureComponent2D)
    comp.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    comp.set_editor_property("texture_target", rt)
    comp.set_editor_property("fov_angle", fov)
    comp.set_editor_property("capture_every_frame", False)
    comp.capture_scene()
    fn = "%s_%s.png" % (OUT, name)
    unreal.RenderingLibrary.export_render_target(world_ref, rt, OUTDIR, fn)
    actor_sys().destroy_actor(cap)
    log("캡처 저장: %s/%s" % (OUTDIR, fn))

def look_at(eye, target):
    d = unreal.Vector(target.x - eye.x, target.y - eye.y, target.z - eye.z)
    yaw = math.degrees(math.atan2(d.y, d.x))
    pitch = math.degrees(math.atan2(d.z, math.sqrt(d.x * d.x + d.y * d.y)))
    return unreal.Rotator(0.0, pitch, yaw)

def run():
    if not os.path.isdir(OUTDIR):
        os.makedirs(OUTDIR)
    load_map()
    add_capture_lights()
    center, ext = scene_bounds()
    r = max(ext.x, ext.y, ext.z) + 200
    # 눈높이: 우리 빌더는 바닥을 z≈0에 둔다. 바운즈에 외부 배경 박스가 껴 min이 왜곡되므로 고정 눈높이 사용.
    eye_z = 155.0

    # 외부 상공 3/4 (전체 배치 파악용)
    iso_eye = unreal.Vector(center.x - r * 1.4, center.y - r * 1.4, center.z + r * 1.4)
    capture(center, ext, "iso", (iso_eye, look_at(iso_eye, center)))

    # ★실내 눈높이 4방향 스윕 (닫힌 방 내부 확인 — FOV 넓게)
    look = unreal.Vector(center.x, center.y, eye_z)
    for name, yaw in (("N", 0.0), ("E", 90.0), ("S", 180.0), ("W", 270.0)):
        eye = unreal.Vector(center.x, center.y, eye_z)
        rot = unreal.Rotator(0.0, -6.0, yaw)
        # 넓은 실내용: 방 안 중앙에서 각 방향
        capture(center, ext, "in_" + name, (eye, rot), 95.0)
    log("=== 완료 ===")

open("C:/Secret_Project/Saved/shot.log", "w", encoding="utf-8").close()
try:
    run()
except Exception:
    log("!! 예외:\n" + traceback.format_exc())
