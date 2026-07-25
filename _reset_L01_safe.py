# -*- coding: utf-8 -*-
# ▶ L01 되돌리기 — 외부 샘플 에셋(Asian_Village) 배제. 엔진 기본 박스 블록아웃으로 재생성.
#   저작권 미확인 팩 사용 금지(사용자 2026-07-21). 다른 22맵과 동일하게 /Engine/BasicShapes/Cube 만 사용.
import unreal

MAP = "/Game/Maps/Rasel/L01_Jangteo_Street"
CUBE = "/Engine/BasicShapes/Cube.Cube"
WALL_H = 400
THICK = 30
W, D = 2400, 1600            # street(M)

_AS = None


def actor_sys():
    global _AS
    if _AS is None:
        _AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return _AS


def spawn_box(cx, cy, cz, sx, sy, sz, tag=""):
    a = actor_sys().spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx, cy, cz))
    if not a:
        return
    a.static_mesh_component.set_static_mesh(unreal.EditorAssetLibrary.load_asset(CUBE))
    a.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    a.set_mobility(unreal.ComponentMobility.STATIC)
    if tag:
        a.set_actor_label(tag)


def build_street():
    # _make_rasel_maps.py 의 street 템플릿과 동일
    spawn_box(0, 0, -THICK / 2, W, D, THICK, "Floor")
    spawn_box(0, D / 2, WALL_H * 1.5, W, THICK, WALL_H * 3, "Bldg_E")
    spawn_box(0, -D / 2, WALL_H * 1.5, W, THICK, WALL_H * 3, "Bldg_W")
    step = W / 5.0
    for i in range(4):
        x = -W / 2 + step * (i + 1)
        side = D / 2 - 300 if i % 2 == 0 else -D / 2 + 300
        spawn_box(x, side, 60, 260, 180, 120, "Prop_Stall_%d" % i)
    actor_sys().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-W / 2 + 300, 0, 120))
    actor_sys().spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, WALL_H * 3),
                                       unreal.Rotator(-50, -35, 0))
    actor_sys().spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, WALL_H * 2))


def main():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        unreal.EditorAssetLibrary.delete_asset(MAP)   # Asian_Village 배치 통째 삭제
    les.new_level(MAP)
    build_street()
    les.save_current_level()
    unreal.log("[ResetL01] 엔진 박스 블록아웃으로 되돌림 완료")


main()
