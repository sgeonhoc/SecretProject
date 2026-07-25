# -*- coding: utf-8 -*-
# ▶ 라셀 플레이 레벨 생성기 — 인물들이 살아가는 장소 23곳의 맵 + 블록아웃
#   정본: C:/Secret_Project/기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md
#
#   하는 일: 레벨마다 빈 레벨을 만들고 → 박스 지오메트리로 블록아웃(바닥·벽·기둥·자재더미) →
#            PlayerStart·조명 배치 → 저장. 동선과 크기를 먼저 세우는 단계(아트 이전).
#   실행: 에디터 Tools → Execute Python Script,  또는 헤드리스
#         UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="C:/Secret_Project/_make_rasel_maps.py" -NullRHI -unattended -nosplash
import unreal

CUBE = "/Engine/BasicShapes/Cube.Cube"
WALL_H = 400          # 벽 높이(uu) — 사람 키 ~180 기준 실내 넉넉히
THICK  = 30           # 벽 두께

# 규모(정본 §0 규모 칸) → 대략 크기(uu)
SIZE = {"S": (900, 700), "M": (2400, 1600), "L": (4200, 3000)}

# (맵경로, 종류, 규모, 사람이 읽는 이름) — 정본 §2 로스터 순서
LEVELS = [
    ("/Game/Maps/Rasel/L01_Jangteo_Street",      "street", "M", "L01 장터 큰길"),
    ("/Game/Maps/Rasel/L02_Antique_Shop",        "room",   "S", "L02 골동상 안"),
    ("/Game/Maps/Rasel/L03_Backalley",           "alley",  "S", "L03 장터 뒷골목"),
    ("/Game/Maps/Rasel/L04_Rooftop_Room",        "room",   "S", "L04 셋집 옥탑 (거점)"),
    ("/Game/Maps/Rasel/L05_Eatery",              "room",   "S", "L05 밥집 두 그릇"),
    ("/Game/Maps/Rasel/L06_Pawnshop_Back",       "room",   "S", "L06 전당포 뒷방"),
    ("/Game/Maps/Rasel/L07_Tram_Jangteo",        "hall",   "S", "L07 전차역 승강장"),
    ("/Game/Maps/Rasel/L06_Academy_Street",      "street", "M", "L08 학당 앞 거리"),
    ("/Game/Maps/Rasel/L09_Academy_Hall",        "hall",   "M", "L09 교실동"),
    ("/Game/Maps/Rasel/L08_Clinic",              "hall",   "M", "L10 의원 병동"),
    ("/Game/Maps/Rasel/L07_Riverbank",           "open",   "M", "L11 학당가 강둑"),
    ("/Game/Maps/Rasel/L14_Selan_Court",         "court",  "M", "L12 셀란 수련원 안뜰"),
    ("/Game/Maps/Rasel/L09_Newport_Site",        "open",   "L", "L13 신항 공사판"),
    ("/Game/Maps/Rasel/L10_Newport_Shaft",       "shaft",  "M", "L14 굴착 갱"),
    ("/Game/Maps/Rasel/L15_Warehouse_Roof",      "open",   "M", "L15 부두 창고 지붕"),
    ("/Game/Maps/Rasel/L16_Tower_Lobby",         "court",  "M", "L16 유리탑 로비"),
    ("/Game/Maps/Rasel/L13_Seir_Sanatorium",          "hall",   "L", "L17 재단 요양원"),
    ("/Game/Maps/Rasel/L18_Council_Hall",        "hall",   "M", "L18 의원 회관 뒷복도"),
    ("/Game/Maps/Rasel/L16_Abandoned_Platform",  "hall",   "M", "L19 폐선 승강장"),
    ("/Game/Maps/Rasel/L20_Sewer_Junction",      "alley",  "L", "L20 지하수로 갈래"),
    ("/Game/Maps/Rasel/L17_Underlayer_Gallery",  "court",  "L", "L21 아래층 회랑"),
    ("/Game/Maps/Rasel/L18_Ruin_Gate",           "open",   "L", "L22 무너진 성문 앞뜰"),
    ("/Game/Maps/Rasel/L19_Ruin_Keep",           "court",  "L", "L23 빈 성 안"),
]


def log(m):
    unreal.log("[Rasel] " + m)
    # 헤드리스에서 LogPython이 안 잡히므로 파일로도 남긴다
    try:
        with open("C:/Secret_Project/Saved/rasel_maps.log", "a", encoding="utf-8") as f:
            f.write(m + "\n")
    except Exception:
        pass


_AS = None
_MESH = None


def actor_sys():
    global _AS
    if _AS is None:
        _AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return _AS


def cube_mesh():
    global _MESH
    if _MESH is None:
        _MESH = unreal.EditorAssetLibrary.load_asset(CUBE)
    return _MESH


def spawn(cls, loc, rot=None):
    return actor_sys().spawn_actor_from_class(cls, loc, rot or unreal.Rotator(0, 0, 0))


def clear_level():
    """맵을 새로 만들지 않고 액터만 비운다 — new_level 반복이 크래시하기 때문."""
    for a in unreal.EditorLevelLibrary.get_all_level_actors():
        try:
            unreal.EditorLevelLibrary.destroy_actor(a)
        except Exception:
            pass


def spawn_box(cx, cy, cz, sx, sy, sz, tag=""):
    """중심(cx,cy,cz)·크기(sx,sy,sz)uu 박스 하나."""
    a = spawn(unreal.StaticMeshActor, unreal.Vector(cx, cy, cz))
    if not a:
        return None
    mesh = cube_mesh()
    a.static_mesh_component.set_static_mesh(mesh)
    a.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    a.set_mobility(unreal.ComponentMobility.STATIC)
    if tag:
        a.set_actor_label(tag)
    return a


def floor_and_walls(w, d, walls=(True, True, True, True)):
    """바닥 + 네 벽(북/남/동/서). walls 로 벽 유무 지정 — 뚫린 쪽이 출입구."""
    spawn_box(0, 0, -THICK / 2, w, d, THICK, "Floor")
    hw, hd = w / 2.0, d / 2.0
    if walls[0]: spawn_box(hw, 0, WALL_H / 2, THICK, d, WALL_H, "Wall_N")
    if walls[1]: spawn_box(-hw, 0, WALL_H / 2, THICK, d, WALL_H, "Wall_S")
    if walls[2]: spawn_box(0, hd, WALL_H / 2, w, THICK, WALL_H, "Wall_E")
    if walls[3]: spawn_box(0, -hd, WALL_H / 2, w, THICK, WALL_H, "Wall_W")


def build(kind, sz):
    w, d = SIZE[sz]

    if kind == "room":            # 방 하나 — 문 한 쪽(서)만 뚫림
        floor_and_walls(w, d, (True, True, True, False))
        spawn_box(w / 2 - 200, -d / 2 + 250, 45, 300, 160, 90, "Prop_Desk")
        spawn_box(-w / 2 + 250, d / 2 - 200, 30, 400, 220, 60, "Prop_Bed")

    elif kind == "street":        # 거리 — 양옆 건물 벽, 양끝 트임. 중간에 좌판 덩어리
        spawn_box(0, 0, -THICK / 2, w, d, THICK, "Floor")
        spawn_box(0, d / 2, WALL_H * 1.5, w, THICK, WALL_H * 3, "Bldg_E")
        spawn_box(0, -d / 2, WALL_H * 1.5, w, THICK, WALL_H * 3, "Bldg_W")
        step = w / 5.0
        for i in range(4):
            x = -w / 2 + step * (i + 1)
            side = d / 2 - 300 if i % 2 == 0 else -d / 2 + 300
            spawn_box(x, side, 60, 260, 180, 120, "Prop_Stall_%d" % i)

    elif kind == "alley":         # 좁은 골목 — 폭 절반, 지그재그 병목
        aw, ad = w, d * 0.45
        spawn_box(0, 0, -THICK / 2, aw, ad, THICK, "Floor")
        spawn_box(0, ad / 2, WALL_H * 1.5, aw, THICK, WALL_H * 3, "Wall_E")
        spawn_box(0, -ad / 2, WALL_H * 1.5, aw, THICK, WALL_H * 3, "Wall_W")
        spawn_box(-aw / 6, ad / 2 - 200, 90, 200, 200, 180, "Choke_1")
        spawn_box(aw / 6, -ad / 2 + 200, 90, 200, 200, 180, "Choke_2")

    elif kind == "hall":          # 복도형 — 가운데 통로, 양옆 방 칸막이
        floor_and_walls(w, d, (True, True, True, True))
        n = 3 if sz == "S" else 5
        step = w / (n + 1.0)
        for i in range(n):
            x = -w / 2 + step * (i + 1)
            spawn_box(x, d / 2 - d * 0.22, WALL_H / 2, THICK, d * 0.44, WALL_H, "Part_E%d" % i)
            spawn_box(x, -d / 2 + d * 0.22, WALL_H / 2, THICK, d * 0.44, WALL_H, "Part_W%d" % i)

    elif kind == "court":         # 넓은 홀·안뜰 — 기둥 격자
        floor_and_walls(w, d, (True, True, True, True))
        for i in (-1, 0, 1):
            for j in (-1, 1):
                spawn_box(i * (w / 3.5), j * (d / 3.5), WALL_H / 2, 140, 140, WALL_H, "Pillar")

    elif kind == "shaft":         # 수직 갱 — 깊은 우물 + 바닥 방
        floor_and_walls(w * 0.6, d * 0.6, (True, True, True, True))
        spawn_box(0, 0, WALL_H * 2, w * 0.6, d * 0.6, THICK, "Ceiling")   # 위가 막힘
        spawn_box(w * 0.2, 0, 60, 200, 200, 120, "Prop_Crate")

    elif kind == "open":          # 트인 야외 — 바닥과 흩어진 덩어리만
        spawn_box(0, 0, -THICK / 2, w, d, THICK, "Ground")
        for i in range(6):
            x = -w / 2 + (w / 7.0) * (i + 1)
            y = (d / 3.0) if i % 2 == 0 else -(d / 3.0)
            spawn_box(x, y, 100, 300, 300, 200, "Block_%d" % i)

    # 플레이어 시작점 — 서쪽 끝
    spawn(unreal.PlayerStart, unreal.Vector(-w / 2 + 300, 0, 120))
    # 조명 — 방향광 + 하늘광 (블록아웃 확인용)
    spawn(unreal.DirectionalLight, unreal.Vector(0, 0, WALL_H * 3), unreal.Rotator(-50, -35, 0))
    spawn(unreal.SkyLight, unreal.Vector(0, 0, WALL_H * 2))


def main():
    # ★ 한 프로세스에서 레벨을 여러 개 만들면 엔진이 죽는다(new_level 반복·액터 전멸 모두 크래시).
    #   그래서 레벨 하나당 프로세스 하나. 어느 레벨인지는 환경변수 RASEL_LEVEL(0-based)로 받는다.
    #   전체 생성: _make_rasel_maps.sh (또는 0..22 루프로 이 스크립트를 반복 실행)
    import os
    idx = os.environ.get("RASEL_LEVEL")
    if idx is None:
        log("RASEL_LEVEL 환경변수가 없다 — 레벨 인덱스(0~%d)를 지정해 실행할 것." % (len(LEVELS) - 1))
        return
    path, kind, sz, name = LEVELS[int(idx)]
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.new_level(path)
    build(kind, sz)
    les.save_current_level()
    log("OK  %s  (%s/%s)  %s" % (name, kind, sz, path))


main()
