# -*- coding: utf-8 -*-
"""아무 레벨이나 찍는 검수 카메라 — 경계를 재서 자동으로 자리를 잡는다.
★함정 셋을 모두 피한다: ①스폰 인자의 위치가 무시되므로 액터·성분 양쪽에 변환을 박는다
②레벨을 열기만 하면 부재의 렌더 자원이 안 올라오므로 메시마다 더미를 하나씩 놓아 깨운다
③메시를 Movable로 돌려 라이트맵 없이도 렌더되게 한다.
실행: SHOT_MAP=/Game/Maps/Rasel/L03_Backalley SHOT_TAG=L03 ... -ExecutePythonScript=_shot_any.py
"""
import os, math, traceback, unreal

_raw = os.environ.get("SHOT_MAP", "/Game/Maps/Rasel/L03_Backalley")
MAP = _raw[_raw.find("/Game/"):] if "/Game/" in _raw else _raw
TAG = os.environ.get("SHOT_TAG", "shot")
OUTDIR = "C:/Secret_Project/Saved/shots"
W, H = 1280, 720
EYE_Z = 165.0


def log(m):
    unreal.log("[shotany] " + str(m))
    with open("C:/Secret_Project/Saved/shot.log", "a", encoding="utf-8") as f:
        f.write(str(m) + "\n")


def A():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def look_at(eye, tgt):
    d = unreal.Vector(tgt[0] - eye[0], tgt[1] - eye[1], tgt[2] - eye[2])
    return unreal.Rotator(0.0,
                          math.degrees(math.atan2(d.z, math.sqrt(d.x * d.x + d.y * d.y))),
                          math.degrees(math.atan2(d.y, d.x)))


def capture(name, eye, tgt, fov=80.0):
    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    rt = unreal.RenderingLibrary.create_render_target2d(
        w, W, H, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    cap = A().spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
    rot = look_at(eye, tgt)
    cap.set_actor_location_and_rotation(unreal.Vector(*eye), rot, False, False)
    c = cap.get_component_by_class(unreal.SceneCaptureComponent2D)
    c.set_world_location_and_rotation(unreal.Vector(*eye), rot, False, False)
    c.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    c.set_editor_property("texture_target", rt)
    c.set_editor_property("fov_angle", fov)
    c.set_editor_property("capture_every_frame", False)
    c.capture_scene()
    unreal.RenderingLibrary.export_render_target(w, rt, OUTDIR, "%s_%s.png" % (TAG, name))
    A().destroy_actor(cap)
    log("  · %s_%s.png" % (TAG, name))


def run():
    if not os.path.isdir(OUTDIR):
        os.makedirs(OUTDIR)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
    log("레벨 로드: %s" % MAP)

    used, lo, hi = {}, [1e9] * 3, [-1e9] * 3
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh:
            sm = a.static_mesh_component.static_mesh
            used[sm.get_path_name()] = sm
            a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
            o, e = a.get_actor_bounds(False)
            lo = [min(lo[0], o.x - e.x), min(lo[1], o.y - e.y), min(lo[2], o.z - e.z)]
            hi = [max(hi[0], o.x + e.x), max(hi[1], o.y + e.y), max(hi[2], o.z + e.z)]
    for k, (p, sm) in enumerate(used.items()):
        d = A().spawn_actor_from_object(sm, unreal.Vector(0, 0, -9000 - k * 60),
                                        unreal.Rotator(0, 0, 0))
        if d:
            d.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
            d.set_actor_label("_깨우기_%d" % k)
    log("부재 깨우기 %d종 · 경계 %s ~ %s" % (len(used), [int(v) for v in lo],
                                          [int(v) for v in hi]))

    cx, cy = (lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2
    r = max(hi[0] - lo[0], hi[1] - lo[1])
    capture("iso", (cx - r * 0.9, cy - r * 0.9, r * 0.85), (cx, cy, 150.0), 62)

    # ★눈높이 컷은 반드시 '바닥 판 위'에서 찍는다 — 경계 상자 끝에서 찍으면
    #   ㄱ자 같은 레벨에선 카메라가 담 속에 박힌다.
    floors = []
    for a in A().get_all_level_actors():
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        sm = a.static_mesh_component.static_mesh
        if sm and any(t in sm.get_name() for t in ("RoadTile", "AlleyFloor", "Floor")):
            l = a.get_actor_location()
            if l.z > -1000:
                # ★바닥의 실제 높이까지 적는다 — 이층 방(바닥 z=320)을
                #   1층 눈높이에서 찍으면 아무것도 안 나온다.
                floors.append((l.x, l.y, l.z))
    if len(floors) >= 2:
        floors.sort()
        picks = [floors[0], floors[len(floors) // 2], floors[-1]]
        for i in range(len(picks)):
            ex, ey, ez = picks[i]
            tx, ty, tz = picks[(i + 1) % len(picks)]
            capture("walk%d" % i, (ex, ey, ez + EYE_Z), (tx, ty, tz + 140.0), 84)
        # 가운데 바닥에서 네 방향 훑기
        mx, my, mz = picks[1]
        for nm_, (dx, dy) in (("N", (0, 1)), ("E", (1, 0)), ("S", (0, -1)), ("W", (-1, 0))):
            capture("look_%s" % nm_, (mx, my, mz + EYE_Z),
                    (mx + dx * 1200.0, my + dy * 1200.0, mz + 130.0), 86)
        log("바닥 판 %d개에서 눈높이 컷" % len(floors))
    else:
        capture("mid", (cx, cy, EYE_Z), (cx, cy + 1000.0, 140.0), 88)
    log("=== 촬영 완료 ===")


open("C:/Secret_Project/Saved/shot.log", "w", encoding="utf-8").close()
try:
    run()
except Exception:
    log("!! 예외:\n" + traceback.format_exc())
