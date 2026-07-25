# -*- coding: utf-8 -*-
"""라셀 부재 키트 — 우리가 저작하는 스태틱메시.
GeometryScript로 형태를 깎고(불리언으로 창·문 구멍), 모서리를 접고(베벨),
UV를 상자투영으로 깔고, 노멀/탄젠트를 계산해 /Game/Rasel/Meshes 에 굽는다.
외부 에셋 참조 0 — 엔진 기하 연산만 쓴다.

부재는 결 A(아랫장터) 기준이지만 거리·실내 어디서나 재사용되는 것들이다.
실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=".../_rasel_kit.py"
산출: 메시 에셋 + C:/Secret_Project/_ArtSource/kit_manifest.json (부재→머티리얼 배열)
"""
import unreal, json, math, traceback

MESHDIR = "/Game/Rasel/Meshes"
MATDIR = "/Game/Rasel/Materials"
MANIFEST = "C:/Secret_Project/_ArtSource/kit_manifest.json"
LOG = "C:/Secret_Project/Saved/rasel_kit.log"

lines = []
manifest = {}


def log(m):
    lines.append(str(m))
    unreal.log("KIT: " + str(m))


P = unreal.GeometryScript_Primitives
B = unreal.GeometryScript_MeshBooleans
M = unreal.GeometryScript_MeshModeling
N = unreal.GeometryScript_Normals
U = unreal.GeometryScript_UVs
Q = unreal.GeometryScript_MeshQueries
NA = unreal.GeometryScript_NewAssetUtils
EAL = unreal.EditorAssetLibrary

CENTER = getattr(unreal.GeometryScriptPrimitiveOriginMode, "CENTER", None)
BASE = getattr(unreal.GeometryScriptPrimitiveOriginMode, "BASE", None)


def nm():
    return unreal.new_object(unreal.DynamicMesh)


def popt(mid=0):
    o = unreal.GeometryScriptPrimitiveOptions()
    o.material_id = mid
    return o


def xf(loc=(0, 0, 0), rot=(0, 0, 0), scale=(1, 1, 1)):
    t = unreal.Transform()
    t.translation = unreal.Vector(*loc)
    t.rotation = unreal.Rotator(rot[0], rot[1], rot[2]).quaternion()
    t.scale3d = unreal.Vector(*scale)
    return t


def box(mesh, mid, center, size, rot=(0, 0, 0)):
    """가운데를 잡고 놓는 상자. rot=(roll,pitch,yaw)"""
    P.append_box(mesh, popt(mid), xf(center, rot), size[0], size[1], size[2],
                 0, 0, 0, CENTER)
    return mesh


def cylinder(mesh, mid, base_center, radius, height, rot=(0, 0, 0), steps=20):
    P.append_cylinder(mesh, popt(mid), xf(base_center, rot), radius, height,
                      steps, 0, True, BASE)
    return mesh


def cut(mesh, center, size, rot=(0, 0, 0)):
    """구멍 파기 — 도구 상자를 만들어 빼낸다."""
    tool = nm()
    box(tool, 0, center, size, rot)
    opts = unreal.GeometryScriptMeshBooleanOptions()
    opts.fill_holes = True
    B.apply_mesh_boolean(mesh, unreal.Transform(), tool, unreal.Transform(),
                         unreal.GeometryScriptBooleanOperation.SUBTRACT, opts)
    return mesh


def bevel(mesh, dist=1.6, subdiv=0):
    """모서리 접기. ★subdiv=0(한 번 깎기)이면 게임 거리에선 둥근 것과 구별이 안 되면서
    삼각형이 절반 아래로 떨어진다 — 볼륨 큰 게임이라 여기서 아끼는 편이 낫다."""
    try:
        o = unreal.GeometryScriptMeshBevelOptions()
        o.bevel_distance = dist
        o.subdivisions = subdiv
        o.infer_material_id = True
        M.apply_mesh_polygroup_bevel(mesh, o)
    except Exception as e:
        log("    (베벨 건너뜀: %s)" % e)
    return mesh


def finish(mesh, name, mats, uv_scale=200.0, bevel_dist=1.6, collision=True):
    """UV·노멀·탄젠트를 채우고 스태틱메시로 굽는다."""
    if bevel_dist > 0:
        bevel(mesh, bevel_dist)

    sel = unreal.GeometryScriptMeshSelection()
    U.set_mesh_u_vs_from_box_projection(
        mesh, 0, xf((0, 0, 0), (0, 0, 0), (uv_scale, uv_scale, uv_scale)), sel, 2)

    co = unreal.GeometryScriptCalculateNormalsOptions()
    co.angle_weighted = True
    co.area_weighted = True
    N.recompute_normals(mesh, co)
    N.compute_tangents(mesh, unreal.GeometryScriptTangentsOptions())

    path = "%s/%s" % (MESHDIR, name)
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    opts = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
    opts.enable_collision = collision
    opts.enable_recompute_normals = False
    opts.enable_recompute_tangents = False
    sm, outcome = NA.create_new_static_mesh_asset_from_mesh(mesh, path, opts)

    # 머티리얼 슬롯을 에셋에 직접 박아 둔다(실패해도 배치 때 성분으로 다시 건다)
    try:
        slots = []
        for i, mp in enumerate(mats):
            mat = EAL.load_asset(mp)
            s = unreal.StaticMaterial()
            s.set_editor_property("material_interface", mat)
            s.set_editor_property("material_slot_name", "Slot%d" % i)
            slots.append(s)
        sm.set_editor_property("static_materials", slots)
    except Exception as e:
        log("    (슬롯 직박기 실패, 배치 때 걸음: %s)" % e)
    EAL.save_asset(path, False)

    tris = Q.get_num_triangle_i_ds(mesh)
    manifest[name] = mats
    log("  · %-26s tri=%-6d mats=%d" % (name, tris, len(mats)))
    return sm


MP = lambda n: "%s/%s" % (MATDIR, n)
PLASTER, WOOD, ROOF = MP("M_Rasel_Plaster"), MP("M_Rasel_Wood"), MP("M_Rasel_Roof")
STONE, COBBLE, OLD = MP("M_Rasel_Stone"), MP("M_Rasel_Cobble"), MP("M_Rasel_OldStone")
IRON, GLASS, DARK = MP("M_Rasel_Iron"), MP("M_Rasel_Glass"), MP("M_Rasel_Dark")
CLOTHA, CLOTHB = MP("M_Rasel_ClothA"), MP("M_Rasel_ClothB")
PAPER, LANTERN, WINLIGHT = MP("M_Rasel_Paper"), MP("M_Rasel_Lantern"), MP("M_Rasel_WinLight")


# ══ 파사드 — 아랫장터 두 층 집. 폭 400 · 두께 30 · 높이 640 ═══════════
# 재료 순서: 0 회벽 · 1 나무 · 2 돌(기단·창턱) · 3 유리 · 4 창 안 불빛
FACADE_MATS = [PLASTER, WOOD, STONE, GLASS, WINLIGHT]
FW, FT, FH = 400.0, 30.0, 640.0


def _window(mesh, cx, cz, w, h, lit=False, sill=True):
    """창 하나 — 구멍을 파고 문설주·창턱·유리를 앉힌다."""
    cut(mesh, (cx, 0, cz), (w, FT * 3, h))
    fr = 13.0
    d = FT + 6.0
    box(mesh, 1, (cx, 0, cz + h / 2 - fr / 2), (w, d, fr))       # 윗틀
    box(mesh, 1, (cx, 0, cz - h / 2 + fr / 2), (w, d, fr))       # 아랫틀
    box(mesh, 1, (cx - w / 2 + fr / 2, 0, cz), (fr, d, h))       # 왼설주
    box(mesh, 1, (cx + w / 2 - fr / 2, 0, cz), (fr, d, h))       # 오른설주
    box(mesh, 1, (cx, 0, cz), (7.0, FT - 4, h - fr * 2))         # 가운데 살
    box(mesh, 4 if lit else 3, (cx, 4.0, cz), (w - fr * 2, 4.0, h - fr * 2))
    if sill:
        box(mesh, 2, (cx, -FT / 2 - 7.0, cz - h / 2 - 5.0), (w + 26, 30.0, 11.0))


def _door(mesh, cx, w, h):
    """길로 난 문 — 구멍·문설주·문짝·댓돌."""
    cut(mesh, (cx, 0, h / 2), (w, FT * 3, h))
    fr = 15.0
    d = FT + 8.0
    box(mesh, 1, (cx, 0, h - fr / 2), (w, d, fr))
    box(mesh, 1, (cx - w / 2 + fr / 2, 0, h / 2), (fr, d, h))
    box(mesh, 1, (cx + w / 2 - fr / 2, 0, h / 2), (fr, d, h))
    box(mesh, 1, (cx, 5.0, (h - fr) / 2), (w - fr * 2, 6.0, h - fr))   # 문짝
    box(mesh, 1, (cx, 5.0, (h - fr) / 2), (w - fr * 2 - 24, 9.0, h - fr - 40))
    box(mesh, 2, (cx, -FT / 2 - 22.0, 7.0), (w + 40, 60.0, 14.0))      # 댓돌


def _facade_shell(mesh):
    box(mesh, 0, (0, 0, FH / 2), (FW, FT, FH))
    box(mesh, 2, (0, 0, 34.0), (FW, FT + 16, 68.0))          # 기단
    box(mesh, 1, (0, 0, 316.0), (FW, FT + 12, 16.0))         # 층 사이 띠


# ══ 조합식 파사드 부재 ═══════════════════════════════════════════════
# ★사용자 지시(2026-07-24): 통짜 파사드 대신 낱개 부재를 조합한다.
#   에디터에서 벽 널 하나, 창 하나, 문 하나를 따로 골라 지우거나 옮길 수 있어야 한다.
#   한 칸 = 폭 400 · 한 층 = 높이 320 · 벽 두께 30 · 바깥면은 -Y 쪽.
MOD_W, MOD_H, MOD_T = 400.0, 320.0, 30.0
WIN_W, WIN_H, WIN_Z = 160.0, 175.0, 180.0      # 창 구멍(칸 안에서의 자리)
DOOR_W, DOOR_H = 130.0, 245.0                  # 문 구멍(바닥에서 올라옴)
SHOP_W, SHOP_H = 250.0, 240.0                  # 가게 목


def _panel(name, holes):
    """벽 널 한 장 — holes = [(중심x, 중심z, 폭, 높이), ...] 만큼 구멍이 뚫린다."""
    m = nm()
    box(m, 0, (0, 0, MOD_H / 2), (MOD_W, MOD_T, MOD_H))
    for (hx, hz, hw, hh) in holes:
        cut(m, (hx, 0, hz), (hw, MOD_T * 3, hh))
    return finish(m, name, [PLASTER])


def panels():
    _panel("SM_Rasel_Wall_Plain", [])
    _panel("SM_Rasel_Wall_Win", [(0.0, WIN_Z, WIN_W, WIN_H)])
    _panel("SM_Rasel_Wall_Win2", [(-95.0, WIN_Z, 120.0, 165.0),
                                  (95.0, WIN_Z, 120.0, 165.0)])
    _panel("SM_Rasel_Wall_Door", [(0.0, DOOR_H / 2, DOOR_W, DOOR_H)])
    _panel("SM_Rasel_Wall_DoorWin", [(-105.0, DOOR_H / 2, DOOR_W, DOOR_H),
                                     (105.0, 190.0, 120.0, 130.0)])
    _panel("SM_Rasel_Wall_Shop", [(0.0, SHOP_H / 2, SHOP_W, SHOP_H)])


def win_unit(name="SM_Rasel_Win_Unit", w=WIN_W, h=WIN_H, lit=False):
    """창 한 짝 — 문설주·가운데 살·유리·창턱. 원점은 구멍 한가운데."""
    m = nm()
    mats = [WOOD, STONE, WINLIGHT if lit else GLASS]
    fr, d = 13.0, MOD_T + 6.0
    box(m, 0, (0, 0, h / 2 - fr / 2), (w, d, fr))
    box(m, 0, (0, 0, -h / 2 + fr / 2), (w, d, fr))
    box(m, 0, (-w / 2 + fr / 2, 0, 0), (fr, d, h))
    box(m, 0, (w / 2 - fr / 2, 0, 0), (fr, d, h))
    box(m, 0, (0, 0, 0), (7.0, MOD_T - 4, h - fr * 2))
    box(m, 2, (0, 4.0, 0), (w - fr * 2, 4.0, h - fr * 2))
    box(m, 1, (0, -MOD_T / 2 - 7.0, -h / 2 - 5.0), (w + 26, 30.0, 11.0))
    return finish(m, name, mats, bevel_dist=0.9)


def door_unit():
    """문 한 짝 — 문설주·문짝·댓돌. 원점은 구멍 바닥 한가운데(z=0)."""
    m = nm()
    mats = [WOOD, STONE]
    w, h, fr, d = DOOR_W, DOOR_H, 15.0, MOD_T + 8.0
    box(m, 0, (0, 0, h - fr / 2), (w, d, fr))
    box(m, 0, (-w / 2 + fr / 2, 0, h / 2), (fr, d, h))
    box(m, 0, (w / 2 - fr / 2, 0, h / 2), (fr, d, h))
    box(m, 0, (0, 5.0, (h - fr) / 2), (w - fr * 2, 6.0, h - fr))
    box(m, 0, (0, 5.0, (h - fr) / 2), (w - fr * 2 - 24, 9.0, h - fr - 40))
    box(m, 1, (0, -MOD_T / 2 - 22.0, 7.0), (w + 40, 60.0, 14.0))
    return finish(m, "SM_Rasel_Door_Unit", mats, bevel_dist=0.9)


def shop_unit():
    """가게 목 — 인방·문설주·문턱 판·안쪽 어둠."""
    m = nm()
    mats = [WOOD, DARK]
    w, h, fr = SHOP_W, SHOP_H, 16.0
    d = MOD_T + 8.0
    box(m, 0, (0, 0, h - fr / 2), (w, d, fr))
    box(m, 0, (-w / 2 + fr / 2, 0, h / 2), (fr, d, h))
    box(m, 0, (w / 2 - fr / 2, 0, h / 2), (fr, d, h))
    box(m, 0, (0, 0, 8.0), (w, MOD_T + 10, 16.0))
    box(m, 1, (0, 6.0, h / 2), (w - fr * 2, 5.0, h - fr))
    return finish(m, "SM_Rasel_Shop_Unit", mats, bevel_dist=0.9)


def trims():
    """기단·층 사이 띠·벽보 거는 살 — 각각 따로 놓는다."""
    m = nm()
    box(m, 0, (0, 0, 34.0), (MOD_W, MOD_T + 16, 68.0))
    finish(m, "SM_Rasel_Plinth", [STONE], bevel_dist=1.2)

    m = nm()
    box(m, 0, (0, 0, 0), (MOD_W, MOD_T + 12, 16.0))
    finish(m, "SM_Rasel_StringCourse", [WOOD], bevel_dist=1.0)

    m = nm()
    box(m, 0, (0, 0, 0), (300.0, 8.0, 8.0))
    finish(m, "SM_Rasel_WallRail", [WOOD], bevel_dist=0.6)


def facade_shop():
    """아래는 열린 가게 목, 위는 살창 하나."""
    m = nm()
    _facade_shell(m)
    cut(m, (0, 0, 175.0), (250.0, FT * 3, 215.0))
    fr = 16.0
    box(m, 1, (0, 0, 175 + 215 / 2 - fr / 2), (250.0, FT + 8, fr))
    box(m, 1, (-125 + fr / 2, 0, 175.0), (fr, FT + 8, 215.0))
    box(m, 1, (125 - fr / 2, 0, 175.0), (fr, FT + 8, 215.0))
    box(m, 1, (0, 0, 74.0), (250.0, FT + 10, 12.0))          # 가게 문턱 판
    box(m, 3, (0, 6.0, 175.0), (250 - fr * 2, 5.0, 215 - fr))
    _window(m, 0, 470.0, 160.0, 175.0, lit=False)
    return finish(m, "SM_Rasel_Facade_Shop", FACADE_MATS)


def facade_home():
    """아래는 문과 쪽창, 위는 창 둘."""
    m = nm()
    _facade_shell(m)
    _door(m, -105.0, 125.0, 245.0)
    _window(m, 105.0, 190.0, 120.0, 130.0)
    _window(m, -95.0, 470.0, 120.0, 165.0, lit=True)
    _window(m, 95.0, 470.0, 120.0, 165.0)
    return finish(m, "SM_Rasel_Facade_Home", FACADE_MATS)


def facade_tall():
    """세 층까지 올린 집 — 지붕선을 끊어 거리가 한 덩어리로 안 보이게 한다."""
    m = nm()
    TH = 940.0
    box(m, 0, (0, 0, TH / 2), (FW, FT, TH))
    box(m, 2, (0, 0, 34.0), (FW, FT + 16, 68.0))
    box(m, 1, (0, 0, 316.0), (FW, FT + 12, 16.0))
    box(m, 1, (0, 0, 632.0), (FW, FT + 12, 14.0))
    _door(m, -110.0, 125.0, 245.0)
    _window(m, 110.0, 190.0, 120.0, 130.0)
    _window(m, -95.0, 470.0, 120.0, 165.0, lit=True)
    _window(m, 95.0, 470.0, 120.0, 165.0)
    _window(m, 0.0, 790.0, 140.0, 150.0)              # 다락 창
    return finish(m, "SM_Rasel_Facade_Tall", FACADE_MATS)


def facade_low():
    """한 층 반짜리 낮은 집 — 사이사이 끼워 넣으면 처마 높이가 들쭉날쭉해진다."""
    m = nm()
    LH = 430.0
    box(m, 0, (0, 0, LH / 2), (FW, FT, LH))
    box(m, 2, (0, 0, 30.0), (FW, FT + 16, 60.0))
    _door(m, -100.0, 120.0, 240.0)
    _window(m, 105.0, 300.0, 130.0, 120.0)
    box(m, 1, (0, -FT / 2 - 4.0, 180.0), (280.0, 8.0, 8.0))
    return finish(m, "SM_Rasel_Facade_Low", FACADE_MATS)


def facade_blank():
    """벽만 선 자리 — 위층 창 하나에 아래는 벽보 붙는 민벽."""
    m = nm()
    _facade_shell(m)
    _window(m, 0.0, 470.0, 130.0, 150.0)
    box(m, 1, (0, -FT / 2 - 4.0, 190.0), (300.0, 8.0, 8.0))   # 벽보 거는 살
    return finish(m, "SM_Rasel_Facade_Blank", FACADE_MATS)


# ══ 처마 — 파사드 위에 얹는다 ══════════════════════════════════════════
def eave():
    """서까래가 내밀고 그 위로 기와가 흐른다."""
    m = nm()
    mats = [ROOF, WOOD]
    box(m, 1, (0, -20.0, 8.0), (FW + 10, 150.0, 16.0))            # 도리
    for i in range(4):                                            # 서까래 6→4(무게 절감)
        x = -FW / 2 + 52 + i * (FW - 104) / 3.0
        box(m, 1, (x, -55.0, 20.0), (14.0, 145.0, 14.0))          # 서까래
    box(m, 0, (0, -58.0, 40.0), (FW + 30, 190.0, 14.0), rot=(-16.0, 0, 0))
    box(m, 0, (0, 30.0, 56.0), (FW + 30, 34.0, 22.0))             # 마루 기와
    box(m, 0, (0, -148.0, 12.0), (FW + 30, 26.0, 20.0))           # 처마 끝 막새
    return finish(m, "SM_Rasel_Eave", mats, bevel_dist=1.2)


# ══ 좌판 ══════════════════════════════════════════════════════════════
def stall(name, cloth):
    m = nm()
    mats = [WOOD, cloth, DARK]
    box(m, 0, (0, 0, 88.0), (170.0, 92.0, 9.0))                   # 상판
    for sx in (-76, 76):
        for sy in (-38, 38):
            box(m, 0, (sx, sy, 44.0), (9.0, 9.0, 88.0))           # 다리
    box(m, 0, (0, -46.0, 66.0), (170.0, 7.0, 26.0))               # 앞막이
    for sx in (-80, 80):
        box(m, 0, (sx, 40.0, 100.0), (8.0, 8.0, 200.0))           # 차양 기둥
    box(m, 1, (0, -18.0, 196.0), (186.0, 132.0, 5.0), rot=(-13.0, 0, 0))
    box(m, 1, (0, -80.0, 172.0), (186.0, 5.0, 34.0))              # 차양 앞단
    box(m, 2, (-40, 6.0, 100.0), (52.0, 42.0, 24.0))              # 물건 궤
    box(m, 2, (36, 6.0, 100.0), (44.0, 38.0, 24.0))
    return finish(m, name, mats, bevel_dist=1.1)


# ══ 가로등 ════════════════════════════════════════════════════════════
def lantern():
    m = nm()
    mats = [STONE, IRON, LANTERN]
    cylinder(m, 0, (0, 0, 0), 19.0, 20.0)                         # 주춧돌
    cylinder(m, 1, (0, 0, 18.0), 7.5, 322.0, steps=14)            # 기둥
    box(m, 1, (0, 0, 344.0), (24.0, 24.0, 10.0))                  # 받침
    box(m, 2, (0, 0, 372.0), (30.0, 30.0, 46.0))                  # 등피
    box(m, 1, (0, 0, 398.0), (38.0, 38.0, 9.0))                   # 갓
    box(m, 1, (0, 0, 406.0), (14.0, 14.0, 12.0))
    return finish(m, "SM_Rasel_Lantern", mats, bevel_dist=1.0)


# ══ 우물 ══════════════════════════════════════════════════════════════
def well():
    m = nm()
    mats = [STONE, WOOD, DARK]
    prof = [unreal.Vector2D(0, 0), unreal.Vector2D(22, 0),
            unreal.Vector2D(22, 74), unreal.Vector2D(0, 74)]
    P.append_revolve_polygon(m, popt(0), xf((0, 0, 0)), prof,
                             unreal.GeometryScriptRevolveOptions(), 86.0, 24)
    box(m, 2, (0, 0, 6.0), (176.0, 176.0, 8.0))                   # 물낯
    for sx in (-96, 96):
        box(m, 1, (sx, 0, 100.0), (12.0, 12.0, 200.0))            # 기둥
    box(m, 1, (0, 0, 196.0), (216.0, 13.0, 13.0))                 # 도르래 대
    box(m, 1, (0, 0, 176.0), (26.0, 26.0, 26.0))
    return finish(m, "SM_Rasel_Well", mats, bevel_dist=1.2)


# ══ 자잘한 것 ═════════════════════════════════════════════════════════
def crate():
    m = nm()
    mats = [WOOD, IRON]
    box(m, 0, (0, 0, 27.0), (62.0, 62.0, 54.0))
    for z in (10.0, 44.0):
        box(m, 1, (0, 0, z), (64.0, 64.0, 5.0))
    return finish(m, "SM_Rasel_Crate", mats, bevel_dist=1.4)


def barrel():
    m = nm()
    mats = [WOOD, IRON]
    cylinder(m, 0, (0, 0, 0), 27.0, 74.0, steps=16)
    for z in (7.0, 34.0, 62.0):
        cylinder(m, 1, (0, 0, z), 28.5, 5.0, steps=16)
    return finish(m, "SM_Rasel_Barrel", mats, bevel_dist=0.9)


def bench():
    m = nm()
    mats = [WOOD, STONE]
    box(m, 0, (0, 0, 44.0), (190.0, 44.0, 9.0))
    box(m, 0, (0, 18.0, 62.0), (190.0, 8.0, 28.0))
    for sx in (-72, 72):
        box(m, 1, (sx, 0, 20.0), (18.0, 40.0, 40.0))
    return finish(m, "SM_Rasel_Bench", mats, bevel_dist=1.2)


def noticeboard():
    """벽보판 — 소문이 붙는 자리(§8-1). 조사 지점 하나가 여기 걸린다."""
    m = nm()
    mats = [WOOD, PAPER]
    box(m, 0, (0, 0, 118.0), (172.0, 12.0, 122.0))
    box(m, 0, (0, 0, 182.0), (188.0, 22.0, 14.0))                 # 비 가림 처마
    for sx in (-80, 80):
        box(m, 0, (sx, 0, 30.0), (14.0, 14.0, 60.0))              # 다리
    for (px, pz, pw, ph) in ((-46, 140, 62, 78), (34, 148, 54, 62), (10, 84, 74, 44)):
        box(m, 1, (px, -7.0, pz), (pw, 2.0, ph))                  # 붙은 낱장들
    return finish(m, "SM_Rasel_NoticeBoard", mats, bevel_dist=0.8)


def banner():
    """길 건너로 친 현수막 — ★줄이 양쪽 벽에 실제로 박혀야 한다.
    길 폭 1200 + 벽 속으로 30씩 = 1260. 천은 가운데에만 걸린다."""
    m = nm()
    mats = [CLOTHB, WOOD, IRON]
    L = 1260.0
    box(m, 1, (0, 0, 56.0), (L, 9.0, 9.0))                    # 건너지른 줄
    for sx in (-1, 1):
        box(m, 2, (sx * (L / 2 - 16.0), 0, 56.0), (34.0, 16.0, 16.0))   # 벽에 박은 고리
    box(m, 0, (0, 0, 0), (430.0, 4.0, 104.0))                 # 천
    box(m, 1, (0, 0, 52.0), (446.0, 7.0, 7.0))                # 천 윗대
    box(m, 1, (0, 0, -52.0), (446.0, 7.0, 7.0))               # 천 아랫대(펄럭임 잡는 것)
    return finish(m, "SM_Rasel_Banner", mats, bevel_dist=0.6)


def road_tile():
    """큰길 한 칸 — 400×400. 가운데가 아주 조금 솟아 비가 갓길로 흐른다."""
    m = nm()
    mats = [COBBLE]
    box(m, 0, (0, 0, -10.0), (400.0, 400.0, 20.0))
    return finish(m, "SM_Rasel_RoadTile", mats, bevel_dist=0.0)


def road_patch():
    """포장이 깨져 아래 옛 돌이 드러난 자리(§7-3)."""
    m = nm()
    mats = [OLD]
    cylinder(m, 0, (0, 0, -6.0), 155.0, 7.0, steps=18)
    return finish(m, "SM_Rasel_RoadPatch", mats, bevel_dist=0.0)


def gutter():
    """갓길 물도랑 — 큰길 양옆으로 흐른다."""
    m = nm()
    mats = [STONE, DARK]
    box(m, 0, (0, 0, -8.0), (400.0, 46.0, 18.0))
    box(m, 1, (0, 0, -6.0), (400.0, 22.0, 8.0))
    return finish(m, "SM_Rasel_Gutter", mats, bevel_dist=0.0)


def alley_wall():
    """골목 쪽 민벽 — 아래는 회 떨어져 옛 돌이 드러남."""
    m = nm()
    mats = [PLASTER, OLD]
    box(m, 0, (0, 0, FH / 2), (400.0, 26.0, FH))
    box(m, 1, (0, 0, 46.0), (400.0, 30.0, 92.0))
    return finish(m, "SM_Rasel_AlleyWall", mats)


def shutter():
    """창 옆에 젖혀 둔 덧문 한 짝 — 벽면에 그림자를 만들어 평면을 깬다."""
    m = nm()
    mats = [WOOD, IRON]
    box(m, 0, (0, 0, 0), (58.0, 9.0, 150.0))
    for z in (-52.0, 0.0, 52.0):
        box(m, 0, (0, -6.0, z), (52.0, 4.0, 13.0))       # 살
    for z in (-58.0, 58.0):
        box(m, 1, (-26.0, 0, z), (10.0, 14.0, 6.0))      # 돌쩌귀
    return finish(m, "SM_Rasel_Shutter", mats, bevel_dist=0.7)


def shopsign():
    """길로 내민 간판 — 걸이대에 매달려 거리 위쪽 실루엣을 만든다."""
    m = nm()
    mats = [IRON, WOOD, PAPER]
    box(m, 0, (0, -34.0, 96.0), (7.0, 76.0, 7.0))        # 걸이대
    box(m, 0, (0, -6.0, 74.0), (7.0, 7.0, 50.0))         # 버팀
    box(m, 0, (0, -62.0, 78.0), (5.0, 5.0, 30.0))        # 사슬
    box(m, 1, (0, -62.0, 34.0), (10.0, 96.0, 62.0))      # 판
    box(m, 2, (0, -62.0, 34.0), (2.0, 78.0, 44.0))       # 글씨 자리
    return finish(m, "SM_Rasel_ShopSign", mats, bevel_dist=0.7)


def downpipe():
    """벽을 타고 내려오는 홈통 — 세로선 하나가 벽면을 나눈다."""
    m = nm()
    mats = [IRON]
    cylinder(m, 0, (0, 0, 0), 9.0, 600.0, steps=8)
    for z in (110.0, 430.0):                             # 죔쇠 3→2
        cylinder(m, 0, (0, 0, z), 12.0, 10.0, steps=8)
    box(m, 0, (0, -14.0, 604.0), (26.0, 34.0, 16.0))     # 물받이 아가리
    return finish(m, "SM_Rasel_Downpipe", mats, bevel_dist=0.0)


def laundry():
    """벽에서 내밀어 친 빨랫줄 — ★줄 양 끝이 벽에 박은 꺾쇠에 걸려야 한다.
    부재의 +Y가 벽 쪽이다(벽에서 62 떨어져 줄이 걸린다)."""
    m = nm()
    mats = [IRON, CLOTHA, CLOTHB]
    for sx in (-1, 1):
        box(m, 0, (sx * 178.0, 31.0, 0.0), (7.0, 62.0, 7.0))     # 벽으로 뻗은 꺾쇠
        box(m, 0, (sx * 178.0, 60.0, -9.0), (11.0, 9.0, 26.0))   # 벽에 박은 판
    box(m, 0, (0, 0, 0), (360.0, 3.0, 3.0))                      # 줄
    hangs = [(-130, 74, 96, 1), (-40, 62, 80, 2), (50, 80, 104, 1), (140, 58, 72, 2)]
    for (x, w, h, mid) in hangs:
        box(m, mid, (x, 2.0, -h / 2.0), (w, 2.5, h))
    return finish(m, "SM_Rasel_Laundry", mats, bevel_dist=0.0)


def chimney():
    m = nm()
    mats = [OLD, ROOF]
    box(m, 0, (0, 0, 90.0), (56.0, 56.0, 180.0))
    box(m, 1, (0, 0, 186.0), (72.0, 72.0, 14.0))
    box(m, 0, (0, 0, 202.0), (24.0, 24.0, 22.0))
    return finish(m, "SM_Rasel_Chimney", mats, bevel_dist=1.2)


def pot():
    """물항아리 — 문간마다 하나씩 놓인다."""
    m = nm()
    mats = [OLD, DARK]
    prof = [unreal.Vector2D(-16, 0), unreal.Vector2D(2, 4), unreal.Vector2D(14, 30),
            unreal.Vector2D(8, 58), unreal.Vector2D(-2, 66), unreal.Vector2D(-6, 62),
            unreal.Vector2D(2, 54), unreal.Vector2D(8, 30), unreal.Vector2D(-4, 6)]
    P.append_revolve_polygon(m, popt(0), xf((0, 0, 0)), prof,
                             unreal.GeometryScriptRevolveOptions(), 18.0, 20)
    box(m, 1, (0, 0, 64.0), (34.0, 34.0, 4.0))
    return finish(m, "SM_Rasel_Pot", mats, bevel_dist=0.0)


def poles():
    """벽에 기대 세워 둔 장대 묶음."""
    m = nm()
    mats = [WOOD]
    for i in range(5):
        box(m, 0, (-16 + i * 8.0, i * 3.0, 108.0), (7.0, 7.0, 216.0),
            rot=(0, 0, 0) if i % 2 else (0, 4.0, 0))
    return finish(m, "SM_Rasel_Poles", mats, bevel_dist=0.6)


# ══ 저잣거리 세간 — 사람이 산다는 표. 낱개로 놓아 하나씩 지울 수 있게 ══
def sack():
    """곡식 자루 — 아래가 퍼지고 위를 묶었다."""
    m = nm()
    mats = [CLOTHA, IRON]
    prof = [unreal.Vector2D(-13, 0), unreal.Vector2D(4, 3), unreal.Vector2D(11, 26),
            unreal.Vector2D(4, 52), unreal.Vector2D(-2, 60), unreal.Vector2D(-9, 56)]
    P.append_revolve_polygon(m, popt(0), xf((0, 0, 0)), prof,
                             unreal.GeometryScriptRevolveOptions(), 15.0, 12)
    box(m, 1, (0, 0, 56.0), (14.0, 14.0, 7.0))                 # 묶은 목
    return finish(m, "SM_Rasel_Sack", mats, bevel_dist=0.0)


def basket():
    """광주리 — 낮고 넓게 벌어진 것."""
    m = nm()
    mats = [WOOD]
    prof = [unreal.Vector2D(-6, 0), unreal.Vector2D(0, 2), unreal.Vector2D(9, 30),
            unreal.Vector2D(6, 31), unreal.Vector2D(-3, 4)]
    P.append_revolve_polygon(m, popt(0), xf((0, 0, 0)), prof,
                             unreal.GeometryScriptRevolveOptions(), 26.0, 14)
    return finish(m, "SM_Rasel_Basket", mats, bevel_dist=0.0)


def ropecoil():
    """사려 놓은 밧줄."""
    m = nm()
    mats = [WOOD]
    # 고리 세 겹 — 작은 네모 단면을 반지름 r로 돌려 만든다(회전체는 이미 검증된 길).
    prof = [unreal.Vector2D(-4, -4), unreal.Vector2D(4, -4),
            unreal.Vector2D(4, 4), unreal.Vector2D(-4, 4)]
    for k, (r, z) in enumerate(((30.0, 5.0), (26.0, 12.0), (21.0, 19.0))):
        P.append_revolve_polygon(m, popt(0), xf((k * 2.0, 0, z)), prof,
                                 unreal.GeometryScriptRevolveOptions(), r, 12)
    return finish(m, "SM_Rasel_RopeCoil", mats, bevel_dist=0.0)


def bucket():
    m = nm()
    mats = [WOOD, IRON]
    prof = [unreal.Vector2D(-2, 0), unreal.Vector2D(2, 0), unreal.Vector2D(5, 34),
            unreal.Vector2D(1, 34)]
    P.append_revolve_polygon(m, popt(0), xf((0, 0, 0)), prof,
                             unreal.GeometryScriptRevolveOptions(), 17.0, 12)
    box(m, 1, (0, 0, 30.0), (44.0, 44.0, 4.0))
    box(m, 1, (0, 0, 46.0), (4.0, 42.0, 4.0))                  # 손잡이
    return finish(m, "SM_Rasel_Bucket", mats, bevel_dist=0.0)


def stool():
    """걸상 — 문간이나 좌판 옆에."""
    m = nm()
    mats = [WOOD]
    box(m, 0, (0, 0, 44.0), (46.0, 40.0, 7.0))
    for sx in (-17, 17):
        for sy in (-14, 14):
            box(m, 0, (sx, sy, 22.0), (6.0, 6.0, 44.0))
    box(m, 0, (0, 0, 20.0), (40.0, 5.0, 5.0))
    return finish(m, "SM_Rasel_Stool", mats, bevel_dist=0.6)


def handcart():
    """손수레 — 실루엣이 커서 거리에 하나만 놓아도 눈에 붙는다."""
    m = nm()
    mats = [WOOD, IRON]
    box(m, 0, (0, 0, 46.0), (150.0, 80.0, 8.0))                # 바닥판
    box(m, 0, (0, -42.0, 60.0), (150.0, 6.0, 28.0))            # 옆널
    box(m, 0, (0, 42.0, 60.0), (150.0, 6.0, 28.0))
    box(m, 0, (-79.0, 0, 60.0), (6.0, 80.0, 28.0))
    for sy in (-46, 46):                                       # 채
        box(m, 0, (96.0, sy, 44.0), (110.0, 7.0, 7.0))
    for sy in (-44, 44):                                       # 바퀴
        cylinder(m, 1, (10.0, sy, 34.0), 34.0, 8.0, rot=(90.0, 0, 0), steps=12)
    box(m, 0, (150.0, 0, 20.0), (7.0, 7.0, 46.0))              # 받침다리
    return finish(m, "SM_Rasel_Handcart", mats, bevel_dist=0.6)


# ══ 뒷골목(L03)용 — 진의 새김·맨홀·골목 바닥 ═══════════════════════════
RUNE, RUNE_F = MP("M_Rasel_Rune"), MP("M_Rasel_RuneFaint")


def rune_parts():
    """바닥에 그은 진 — 낱개 선·고리·매듭으로 나눠 두면 무늬를 자유로이 짤 수 있고,
    사용자가 한 획씩 지울 수도 있다(조합식 원칙)."""
    m = nm()
    box(m, 0, (0, 0, 1.0), (200.0, 7.0, 2.0))
    finish(m, "SM_Rasel_RuneLine", [RUNE], bevel_dist=0.0, collision=False)

    m = nm()
    box(m, 0, (0, 0, 1.0), (200.0, 7.0, 2.0))
    finish(m, "SM_Rasel_RuneLine_Faint", [RUNE_F], bevel_dist=0.0, collision=False)

    m = nm()
    prof = [unreal.Vector2D(-4, 0), unreal.Vector2D(4, 0),
            unreal.Vector2D(4, 2), unreal.Vector2D(-4, 2)]
    P.append_revolve_polygon(m, popt(0), xf((0, 0, 0)), prof,
                             unreal.GeometryScriptRevolveOptions(), 92.0, 28)
    finish(m, "SM_Rasel_RuneRing", [RUNE], bevel_dist=0.0, collision=False)

    m = nm()
    cylinder(m, 0, (0, 0, 0), 15.0, 2.5, steps=12)
    finish(m, "SM_Rasel_RuneNode", [RUNE], bevel_dist=0.0, collision=False)


def manhole():
    """환기구 맨홀 — 아래층(폐선 승강장)으로 내려가는 구멍의 뚜껑."""
    m = nm()
    mats = [DARK, IRON, WOOD]
    cylinder(m, 0, (0, 0, -6.0), 52.0, 8.0, steps=16)          # 구멍 테
    cylinder(m, 1, (0, 0, 0.0), 46.0, 5.0, steps=16)           # 쇠 뚜껑
    for k in range(3):
        box(m, 2, (0, -28.0 + k * 28.0, 4.0), (78.0, 16.0, 4.0))   # 널판 세 장
    box(m, 1, (34.0, 0, 4.0), (14.0, 30.0, 5.0))               # 들어 올리는 손잡이
    return finish(m, "SM_Rasel_Manhole", mats, bevel_dist=0.0)


def alley_floor():
    """골목 바닥 — 젖은 돌. 큰길 포장과 달리 판이 크고 이음매가 굵다."""
    m = nm()
    box(m, 0, (0, 0, -10.0), (400.0, 400.0, 20.0))
    return finish(m, "SM_Rasel_AlleyFloor", [STONE], bevel_dist=0.0)


# ══ 실내(L05 밥집 등)용 — 방을 짜는 부재 ═══════════════════════════════
def room_parts():
    """바닥·천장 — 벽 널과 같은 400 칸에 맞춰 이어 붙인다."""
    m = nm()
    box(m, 0, (0, 0, -8.0), (400.0, 400.0, 16.0))
    finish(m, "SM_Rasel_RoomFloor", [WOOD], bevel_dist=0.0)

    m = nm()
    box(m, 0, (0, 0, 7.0), (400.0, 400.0, 14.0))
    for k in range(2):                                    # 서까래 두 줄이 드러난 천장
        box(m, 1, (0, -100.0 + k * 200.0, -6.0), (400.0, 22.0, 14.0))
    finish(m, "SM_Rasel_Ceiling", [PLASTER, WOOD], bevel_dist=0.0)


def counter():
    """밥집 카운터 — 앞널·상판·아래 선반."""
    m = nm()
    mats = [WOOD, STONE]
    box(m, 0, (0, 0, 96.0), (500.0, 62.0, 8.0))           # 상판
    box(m, 0, (0, -28.0, 48.0), (500.0, 7.0, 96.0))       # 앞널
    for sx in (-230, 0, 230):
        box(m, 0, (sx, 20.0, 48.0), (10.0, 50.0, 96.0))   # 다리
    box(m, 0, (0, 14.0, 40.0), (480.0, 44.0, 6.0))        # 아래 선반
    box(m, 1, (0, 0, 101.0), (500.0, 62.0, 3.0))          # 돌 얹은 윗면
    return finish(m, "SM_Rasel_Counter", mats, bevel_dist=0.8)


def hearth():
    """★화덕 — 이 방의 주광원. 아궁이 안이 스스로 빛난다."""
    m = nm()
    mats = [STONE, LANTERN, IRON]
    box(m, 0, (0, 0, 54.0), (190.0, 150.0, 108.0))        # 돌 몸
    cut(m, (0, -50.0, 44.0), (110.0, 130.0, 76.0))        # 아궁이 구멍
    box(m, 1, (0, 24.0, 40.0), (104.0, 60.0, 66.0))       # 안쪽 불빛
    box(m, 2, (0, -46.0, 96.0), (150.0, 26.0, 8.0))       # 걸침쇠
    box(m, 0, (0, 40.0, 124.0), (150.0, 70.0, 32.0))      # 굴뚝 목
    return finish(m, "SM_Rasel_Hearth", mats, bevel_dist=1.0)


def table():
    """밥집 상 — 낮고 네모난 것."""
    m = nm()
    mats = [WOOD]
    box(m, 0, (0, 0, 72.0), (160.0, 96.0, 8.0))
    for sx in (-66, 66):
        for sy in (-34, 34):
            box(m, 0, (sx, sy, 36.0), (8.0, 8.0, 72.0))
    box(m, 0, (0, 0, 26.0), (150.0, 6.0, 6.0))
    return finish(m, "SM_Rasel_Table", mats, bevel_dist=0.7)


def partition():
    """칸막이 — 구석 밀담 자리를 가린다. 살창처럼 위가 트여 있다."""
    m = nm()
    mats = [WOOD]
    box(m, 0, (0, 0, 8.0), (240.0, 18.0, 16.0))           # 밑틀
    box(m, 0, (0, 0, 156.0), (240.0, 18.0, 14.0))         # 윗틀
    for sx in (-112, 112):
        box(m, 0, (sx, 0, 82.0), (16.0, 18.0, 164.0))     # 기둥
    box(m, 0, (0, 0, 60.0), (222.0, 10.0, 88.0))          # 아래 막힌 판
    for k in range(5):                                    # 위쪽 살
        box(m, 0, (-88.0 + k * 44.0, 0, 128.0), (9.0, 10.0, 44.0))
    return finish(m, "SM_Rasel_Partition", mats, bevel_dist=0.7)


def shelf():
    """벽 선반 — 그릇을 얹는다."""
    m = nm()
    mats = [WOOD, IRON]
    box(m, 0, (0, 0, 0), (300.0, 34.0, 8.0))
    for sx in (-120, 0, 120):
        box(m, 1, (sx, 12.0, -12.0), (7.0, 26.0, 22.0))   # 받침쇠
    return finish(m, "SM_Rasel_Shelf", mats, bevel_dist=0.5)


def bowl():
    m = nm()
    mats = [STONE]
    prof = [unreal.Vector2D(-3, 0), unreal.Vector2D(1, 1), unreal.Vector2D(6, 13),
            unreal.Vector2D(3, 14)]
    P.append_revolve_polygon(m, popt(0), xf((0, 0, 0)), prof,
                             unreal.GeometryScriptRevolveOptions(), 11.0, 12)
    return finish(m, "SM_Rasel_Bowl", mats, bevel_dist=0.0, collision=False)


# ══ 골동상(L02)용 — 두 얼굴을 위한 부재 ═══════════════════════════════
ASH, FELT = MP("M_Rasel_Ash"), MP("M_Rasel_Felt")


def display_case():
    """진열장 — 유리 너머로 물건이 보인다. 낮의 가게 얼굴."""
    m = nm()
    mats = [WOOD, GLASS, STONE]
    box(m, 0, (0, 0, 6.0), (200.0, 62.0, 12.0))                # 굽
    for sx in (-96, 96):
        box(m, 0, (sx, 0, 66.0), (8.0, 62.0, 120.0))           # 기둥
    box(m, 0, (0, 0, 130.0), (200.0, 62.0, 10.0))              # 갓
    box(m, 0, (0, 28.0, 66.0), (200.0, 6.0, 108.0))            # 뒤판
    box(m, 1, (0, -28.0, 66.0), (184.0, 4.0, 108.0))           # 앞 유리
    box(m, 0, (0, 0, 68.0), (184.0, 56.0, 5.0))                # 가운데 선반
    for k, sx in enumerate((-60.0, 0.0, 62.0)):                # 얹힌 물건
        box(m, 2, (sx, 0, 84.0), (30.0, 26.0, 28.0))
    return finish(m, "SM_Rasel_DisplayCase", mats, bevel_dist=0.8)


def appraisal_desk():
    """감정대 — 융을 깐 판. 네사가 도렌의 판을 여기 놓고 오래 멈췄다(§3-2)."""
    m = nm()
    mats = [WOOD, FELT, IRON]
    box(m, 0, (0, 0, 86.0), (220.0, 110.0, 10.0))
    box(m, 1, (0, 0, 92.0), (190.0, 84.0, 3.0))                # 융
    for sx in (-96, 96):
        for sy in (-44, 44):
            box(m, 0, (sx, sy, 43.0), (10.0, 10.0, 86.0))
    box(m, 0, (0, 40.0, 52.0), (206.0, 26.0, 6.0))             # 아래 칸
    box(m, 2, (86.0, -30.0, 100.0), (16.0, 16.0, 18.0))        # 확대경 받침
    return finish(m, "SM_Rasel_AppraisalDesk", mats, bevel_dist=0.8)


def floor_torn():
    """★뜯긴 마루 — 밤의 얼굴. 널이 들려 아래 어둠이 보인다(§5-1 숨긴 자리)."""
    m = nm()
    mats = [WOOD, DARK]
    box(m, 0, (0, 0, -8.0), (400.0, 400.0, 16.0))
    cut(m, (40.0, -20.0, -4.0), (200.0, 150.0, 40.0))          # 뜯어낸 구멍
    box(m, 1, (40.0, -20.0, -22.0), (196.0, 146.0, 20.0))      # 아래 어둠
    box(m, 0, (-96.0, -20.0, 14.0), (34.0, 150.0, 12.0), rot=(0, -22.0, 0))
    box(m, 0, (150.0, 6.0, 10.0), (150.0, 30.0, 10.0), rot=(0, 9.0, 0))
    box(m, 0, (120.0, -96.0, 8.0), (120.0, 26.0, 9.0), rot=(0, -6.0, 0))
    return finish(m, "SM_Rasel_FloorTorn", mats, bevel_dist=0.0)


def burn_patch():
    """탄 자국 — 벽이나 바닥에 눌어붙은 검은 자리. 납작해서 어디에나 붙인다."""
    m = nm()
    box(m, 0, (0, 0, 0.6), (260.0, 200.0, 1.2))
    return finish(m, "SM_Rasel_BurnPatch", [ASH], bevel_dist=0.0, collision=False)


def chest_burnt():
    """태운 궤 — 뒷방에서 무언가를 태운 자리."""
    m = nm()
    mats = [ASH, IRON]
    box(m, 0, (0, 0, 24.0), (96.0, 70.0, 48.0))
    box(m, 0, (-10.0, 6.0, 52.0), (70.0, 52.0, 10.0), rot=(0, 12.0, 0))   # 무너진 뚜껑
    box(m, 1, (0, 0, 14.0), (100.0, 74.0, 5.0))
    return finish(m, "SM_Rasel_ChestBurnt", mats, bevel_dist=0.6)


def curtain():
    """커튼 — 앞가게와 뒷방을 가른다. 문설주 개구(180)에 걸린다."""
    m = nm()
    mats = [CLOTHB, WOOD]
    box(m, 1, (0, 0, 236.0), (200.0, 12.0, 10.0))              # 봉
    box(m, 0, (-52.0, 0, 118.0), (86.0, 5.0, 226.0))           # 왼 폭
    box(m, 0, (58.0, 0, 118.0), (74.0, 5.0, 226.0), rot=(0, 0, 4.0))
    return finish(m, "SM_Rasel_Curtain", mats, bevel_dist=0.0)


# ══ 흥신소(L04)용 — 수사 허브의 세간 ═══════════════════════════════════
def desk():
    """책상 — 서랍은 따로 놓는다(그 서랍이 사건의 심장이라 낱개여야 한다)."""
    m = nm()
    mats = [WOOD, DARK]
    box(m, 0, (0, 0, 74.0), (230.0, 120.0, 8.0))               # 상판
    for sx in (-100, 100):
        box(m, 0, (sx, 0, 36.0), (12.0, 110.0, 72.0))          # 옆판
    box(m, 0, (0, 52.0, 36.0), (206.0, 8.0, 72.0))             # 뒤판
    box(m, 1, (0, -6.0, 44.0), (176.0, 96.0, 44.0))            # 서랍 들어갈 빈칸(어둠)
    box(m, 0, (0, 0, 12.0), (206.0, 100.0, 6.0))               # 아래 받침
    return finish(m, "SM_Rasel_Desk", mats, bevel_dist=0.8)


def drawer():
    """★서랍 — 반쯤 열려 종잇조각이 보인다. 이 방의 심장(§7-1)."""
    m = nm()
    mats = [WOOD, DARK, PAPER]
    box(m, 0, (0, -34.0, 44.0), (172.0, 92.0, 40.0))           # 서랍 몸
    box(m, 1, (0, -34.0, 50.0), (156.0, 78.0, 30.0))           # 안쪽 어둠
    box(m, 0, (0, -80.0, 44.0), (176.0, 8.0, 44.0))            # 앞널
    box(m, 0, (0, -85.0, 44.0), (44.0, 6.0, 8.0))              # 손잡이
    for k, (px, py, pw) in enumerate(((-40, -46, 54), (14, -30, 46), (52, -52, 38))):
        box(m, 2, (px, py, 62.0), (pw, 34.0, 2.0),
            rot=(0, 0, -18.0 + k * 16.0))                      # 흩어진 종잇조각
    return finish(m, "SM_Rasel_Drawer", mats, bevel_dist=0.5)


def obit_board():
    """부고란 벽 — 오려 붙인 부고가 다닥다닥. 벽에 건다(§9·§20)."""
    m = nm()
    mats = [WOOD, PAPER, IRON]
    box(m, 0, (0, 0, 0), (360.0, 10.0, 240.0))                 # 판
    for k, (px, pz, pw, ph, rz) in enumerate((
            (-118, 74, 68, 52, -4.0), (-34, 88, 56, 44, 6.0), (44, 70, 72, 58, -9.0),
            (124, 84, 52, 46, 3.0), (-108, -8, 60, 50, 7.0), (-20, 2, 66, 54, -5.0),
            (62, -14, 58, 48, 11.0), (132, 4, 54, 44, -7.0),
            (-70, -92, 74, 56, 2.0), (26, -86, 62, 50, -12.0), (118, -96, 56, 46, 8.0))):
        box(m, 1, (px, -6.0, pz), (pw, 2.0, ph), rot=(0, 0, rz))
        box(m, 2, (px, -8.0, pz + ph / 2 - 6.0), (5.0, 5.0, 5.0))   # 꽂은 핀
    return finish(m, "SM_Rasel_ObitBoard", mats, bevel_dist=0.0)


def chair():
    """등받이 있는 의자 — 손님 자리."""
    m = nm()
    mats = [WOOD]
    box(m, 0, (0, 0, 44.0), (52.0, 48.0, 7.0))
    for sx in (-20, 20):
        for sy in (-18, 18):
            box(m, 0, (sx, sy, 22.0), (6.0, 6.0, 44.0))
    box(m, 0, (0, 21.0, 76.0), (52.0, 6.0, 58.0))              # 등판
    box(m, 0, (0, 0, 18.0), (46.0, 5.0, 5.0))
    return finish(m, "SM_Rasel_Chair", mats, bevel_dist=0.6)


def paper_stack():
    """쌓인 서류 뭉치."""
    m = nm()
    mats = [PAPER]
    for k in range(5):
        box(m, 0, (k * 2.0 - 4.0, k * 1.5 - 3.0, 3.0 + k * 5.0), (74.0, 54.0, 5.0),
            rot=(0, 0, -8.0 + k * 4.0))
    return finish(m, "SM_Rasel_PaperStack", mats, bevel_dist=0.0)


# ══ 길바닥 마모·물웅덩이 ═══════════════════════════════════════════════
PUDDLE, WETSTONE = MP("M_Rasel_Puddle"), MP("M_Rasel_WetStone")


def puddle(name, dia):
    """물웅덩이 — 아주 얕게 파인 자리에 고인 물. 둘레는 젖은 돌로 짙다.
    바닥보다 살짝(2) 낮게 앉혀 웅덩이가 바닥에 파묻힌 것처럼 보이게 놓는다."""
    m = nm()
    mats = [PUDDLE, WETSTONE]
    r = dia / 2.0
    # 젖은 돌 갓 — 물보다 조금 넓게
    prof = [unreal.Vector2D(0, 0), unreal.Vector2D(r + 18, 0),
            unreal.Vector2D(r + 14, 3.0), unreal.Vector2D(r, 3.5)]
    P.append_revolve_polygon(m, popt(1), xf((0, 0, 0)), prof,
                             unreal.GeometryScriptRevolveOptions(), 0.0, 20)
    # 물낯 — 갓 안쪽에 아주 얕게
    cylinder(m, 0, (0, 0, 0.5), r, 2.0, steps=20)
    return finish(m, name, mats, bevel_dist=0.0, collision=False)


def puddles():
    puddle("SM_Rasel_Puddle_S", 120.0)
    puddle("SM_Rasel_Puddle_M", 220.0)
    puddle("SM_Rasel_Puddle_L", 340.0)


def wheel_ruts():
    """바큇자국 — 수레가 오래 다닌 길에 팬 두 줄. 바닥에 얕게 눌러 앉힌다."""
    m = nm()
    mats = [WETSTONE]
    for sy in (-70.0, 70.0):
        box(m, 0, (0, sy, -1.0), (400.0, 26.0, 3.0))
    return finish(m, "SM_Rasel_WheelRuts", mats, bevel_dist=0.0, collision=False)


# ══ 처마 등롱 — 밤 거리에 매다는 종이등 ═══════════════════════════════
def hanging_lantern():
    """종이등 — 처마·간판대에 매단다. 팔각 갓 + 발광 속. 밤 거리 실루엣의 핵."""
    m = nm()
    mats = [WOOD, LANTERN, IRON]
    box(m, 2, (0, 0, 0), (5.0, 5.0, 44.0))                    # 매다는 줄
    box(m, 0, (0, 0, -30.0), (34.0, 34.0, 8.0))               # 윗 갓
    # 종이 몸 — 팔각 근사(회전체 8단)
    prof = [unreal.Vector2D(0, 0), unreal.Vector2D(20, 4),
            unreal.Vector2D(22, 30), unreal.Vector2D(16, 58), unreal.Vector2D(0, 62)]
    P.append_revolve_polygon(m, popt(1), xf((0, 0, -96.0)), prof,
                             unreal.GeometryScriptRevolveOptions(), 0.0, 8)
    box(m, 0, (0, 0, -98.0), (30.0, 30.0, 7.0))               # 아래 갓
    box(m, 2, (0, 0, -108.0), (4.0, 4.0, 14.0))               # 술
    return finish(m, "SM_Rasel_HangLantern", mats, bevel_dist=0.5)


# ══ 가게 차양 — 목 위로 내민 천 지붕 ═══════════════════════════════════
def shop_awning(name="SM_Rasel_Awning", cloth=None):
    """가게 목 위로 비스듬히 내민 천 차양 + 버팀대. 벽에 붙어 앞(-Y)으로 드리운다.
    원점은 벽면(z=0 바닥 기준으로 배치 때 올린다)."""
    m = nm()
    mats = [WOOD, cloth or CLOTHA, IRON]
    box(m, 0, (0, 8.0, 0.0), (280.0, 16.0, 12.0))            # 벽에 붙는 대
    # 천 — 벽에서 앞으로 내리막(앞이 낮다)
    box(m, 1, (0, -70.0, -20.0), (280.0, 160.0, 5.0), rot=(18.0, 0, 0))
    box(m, 1, (0, -148.0, -46.0), (280.0, 6.0, 30.0))        # 앞단 늘어짐
    for sx in (-120.0, 120.0):                               # 버팀 막대
        box(m, 2, (sx, -70.0, -40.0), (5.0, 150.0, 5.0), rot=(18.0, 0, 0))
    return finish(m, name, mats, bevel_dist=0.6)


def shop_awnings():
    shop_awning("SM_Rasel_Awning_A", CLOTHA)
    shop_awning("SM_Rasel_Awning_B", CLOTHB)


def stair_step():
    m = nm()
    mats = [STONE]
    P.append_linear_stairs(m, popt(0), xf((0, 0, 0)), 220.0, 17.0, 32.0, 4, False)
    return finish(m, "SM_Rasel_Steps", mats, bevel_dist=1.0)


def build_all():
    log("=== 라셀 부재 키트 저작 ===")
    # 조합식 부재(새 방식) — 벽 널·창·문·가게 목·기단·띠·살을 따로
    panels()
    win_unit()
    win_unit("SM_Rasel_Win_Unit_Lit", lit=True)
    win_unit("SM_Rasel_Win_Small", w=120.0, h=130.0)
    win_unit("SM_Rasel_Win_Tall", w=120.0, h=165.0)
    door_unit()
    shop_unit()
    trims()
    eave()
    shutter()
    shopsign()
    downpipe()
    laundry()
    chimney()
    pot()
    poles()
    stall("SM_Rasel_Stall_A", CLOTHA)
    stall("SM_Rasel_Stall_B", CLOTHB)
    lantern()
    well()
    crate()
    barrel()
    sack()
    basket()
    ropecoil()
    bucket()
    stool()
    handcart()
    bench()
    noticeboard()
    banner()
    road_tile()
    road_patch()
    gutter()
    alley_wall()
    rune_parts()
    puddles()
    shop_awnings()
    hanging_lantern()
    wheel_ruts()
    room_parts()
    display_case()
    desk()
    drawer()
    obit_board()
    chair()
    paper_stack()
    appraisal_desk()
    floor_torn()
    burn_patch()
    chest_burnt()
    curtain()
    counter()
    hearth()
    table()
    partition()
    shelf()
    bowl()
    manhole()
    alley_floor()
    stair_step()
    with open(MANIFEST, "w", encoding="utf-8") as f:
        json.dump(manifest, f, ensure_ascii=False, indent=1)
    log("=== 부재 %d종 · 명세 %s ===" % (len(manifest), MANIFEST))


if __name__ == "__main__":
    try:
        build_all()
    except Exception:
        log("실패\n" + traceback.format_exc())
    with open(LOG, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
