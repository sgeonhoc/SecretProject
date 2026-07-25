# -*- coding: utf-8 -*-
"""L01 검수 촬영 — 레벨이 가진 제 빛 그대로 찍는다.
(공용 하네스와 달리 임시 태양광·자동노출 덮어쓰기를 하지 않는다. 우리 조명값을 봐야 하므로.)
실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=".../_shot_L01.py" -RenderOffScreen
"""
import os, math, traceback, unreal

_raw = os.environ.get("SHOT_MAP", "/Game/Maps/Rasel/L01_Jangteo_Street")
_i = _raw.find("/Game/")
MAP = _raw[_i:] if _i >= 0 else _raw
OUTDIR = "C:/Secret_Project/Saved/shots"
TAG = os.environ.get("SHOT_TAG", "L01v2")
W, H = 1280, 720
TILE = 400.0


def log(m):
    unreal.log("[shotL01] " + str(m))
    with open("C:/Secret_Project/Saved/shot.log", "a", encoding="utf-8") as f:
        f.write(str(m) + "\n")


def A():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def xc(i):
    return TILE * i


def look_at(eye, tgt):
    d = unreal.Vector(tgt[0] - eye[0], tgt[1] - eye[1], tgt[2] - eye[2])
    yaw = math.degrees(math.atan2(d.y, d.x))
    pitch = math.degrees(math.atan2(d.z, math.sqrt(d.x * d.x + d.y * d.y)))
    return unreal.Rotator(0.0, pitch, yaw)


def capture(name, eye, tgt, fov=75.0):
    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    rt = unreal.RenderingLibrary.create_render_target2d(
        w, W, H, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    cap = A().spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(*eye),
                                     look_at(eye, tgt))
    # ★★함정 2단: ①spawn 인자의 위치가 무시돼 늘 같은 자리에 선다
    #   ②액터를 옮겨도 캡처 성분의 변환은 안 따라온다(에디터가 안 돌아 갱신이 안 됨).
    #   → 액터와 성분 둘 다에 직접 박아야 카메라가 실제로 움직인다.
    rot = look_at(eye, tgt)
    cap.set_actor_location_and_rotation(unreal.Vector(*eye), rot, False, False)
    c = cap.get_component_by_class(unreal.SceneCaptureComponent2D)
    c.set_world_location_and_rotation(unreal.Vector(*eye), rot, False, False)
    got = c.get_world_location()
    if abs(got.x - eye[0]) > 1 or abs(got.y - eye[1]) > 1 or abs(got.z - eye[2]) > 1:
        log("  !! 카메라 자리 안 맞음: 요청(%.0f,%.0f,%.0f) 실제(%.0f,%.0f,%.0f)"
            % (eye[0], eye[1], eye[2], got.x, got.y, got.z))
    c.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    c.set_editor_property("texture_target", rt)
    c.set_editor_property("fov_angle", fov)
    c.set_editor_property("capture_every_frame", False)
    c.capture_scene()
    c.capture_scene()          # 한 번 더 — 시간에 따라 수렴하는 조명 안정화
    fn = "%s_%s.png" % (TAG, name)
    unreal.RenderingLibrary.export_render_target(w, rt, OUTDIR, fn)
    A().destroy_actor(cap)
    log("  · %s" % fn)


SHOTS = [
    # 이름          눈                              보는 곳                       화각
    ("street",   (xc(0) - 420, 60, 168),         (xc(11), -40, 150),           82),
    ("street_b", (xc(11) + 420, -60, 168),       (xc(0), 40, 150),             82),
    ("facade",   (xc(8) - 180, 40, 190),         (xc(8), 600, 330),            70),
    ("stall",    (xc(4) - 330, -40, 150),        (xc(4), -330, 120),           64),
    ("alley",    (xc(5), -420, 160),             (xc(5), -1800, 130),          78),
    ("well",     (xc(6) - 430, 330, 205),        (xc(6), 0, 80),               68),
    ("board",    (xc(5) - 180, 210, 180),        (xc(5), 560, 160),            58),
    ("iso",      (xc(2) - 1600, -2100, 2000),    (xc(7), 0, 200),              62),
]


def run():
    if not os.path.isdir(OUTDIR):
        os.makedirs(OUTDIR)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
    log("레벨 로드: %s" % MAP)
    n = 0
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component:
            a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
            n += 1
    log("메시 Movable 확인: %d" % n)
    # ★레벨을 열기만 하면 성분들이 렌더 씬에 안 올라온다(에디터가 안 도니까).
    #   성분을 내렸다 다시 올려 강제로 등록시킨다. 이걸 빼면 온 화면이 하늘만 나온다.
    re = 0
    # ★레벨이 참조하는 것만으론 부재의 렌더 자원이 안 올라온다(커맨들릿에서만 생기는 일).
    #   쓰이는 부재를 전부 명시적으로 불러오고 액터를 하나 놓아 씬을 갱신시킨다.
    used = {}
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh:
            used[a.static_mesh_component.static_mesh.get_path_name()] = \
                a.static_mesh_component.static_mesh
    k = 0
    for p, m in used.items():
        d = A().spawn_actor_from_object(m, unreal.Vector(0, 0, -9000 - k * 50),
                                        unreal.Rotator(0, 0, 0))
        if d:
            d.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
            d.set_actor_label("_깨우기_%d" % k)
            k += 1
    log("부재 깨우기: %d종" % k)
    # ★텍스처가 체커 자리표시로 나오는 것 막기 — 커맨들릿에선 밉 스트리밍이 안 돈다
    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    for cmd in ("r.Streaming.FullyLoadUsedTextures 1",
                "r.Streaming.Boost 10000",
                "r.Streaming.PoolSize 0",
                "r.Streaming.UseFixedPoolSize 0"):
        try:
            unreal.SystemLibrary.execute_console_command(w, cmd)
        except Exception as e:
            log("  ! %s (%s)" % (cmd, e))
    for name, eye, tgt, fov in SHOTS:
        capture(name, eye, tgt, fov)
    log("=== 촬영 완료 ===")


open("C:/Secret_Project/Saved/shot.log", "w", encoding="utf-8").close()
try:
    run()
except Exception:
    log("!! 예외:\n" + traceback.format_exc())
