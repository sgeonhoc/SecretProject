# -*- coding: utf-8 -*-
"""레벨 물리 검사기 — 말이 안 되는 배치를 사람이 보기 전에 잡는다.
  ①떠 있는 것: 바닥에서 떨어져 있는데 벽·지붕 어디에도 안 닿는 부재
  ②박힌 것: 부재끼리 서로 파고든 정도가 심한 짝
  ③막은 것: 문·계단 앞을 가로막은 부재
실행: AUDIT_MAP=/Game/Maps/Rasel/L01_Jangteo_Street UnrealEditor-Cmd ... -ExecutePythonScript=_audit_level.py
"""
import os, unreal

_raw = os.environ.get("AUDIT_MAP", "/Game/Maps/Rasel/L01_Jangteo_Street")
_i = _raw.find("/Game/")
MAP = _raw[_i:] if _i >= 0 else _raw
LOG = "C:/Secret_Project/Saved/audit.log"
lines = []


def log(m):
    lines.append(str(m))
    unreal.log("[audit] " + str(m))


def A():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


# 바닥에 깔리는 것 — 서로 맞닿는 게 정상이라 겹침 검사에서 뺀다
FLOOR = ("RoadTile", "Gutter", "RoadPatch", "AlleyFloor", "Rune",
         "RoomFloor", "FloorTorn", "Ceiling", "BurnPatch", "Puddle", "WheelRuts")
# 벽·지붕에 붙는 것 — 공중에 있어도 되지만 반드시 뭔가에 닿아야 한다
MOUNTED = ("Shutter", "ShopSign", "Downpipe", "Laundry", "Banner", "Chimney",
           "Eave", "Facade")
# 다른 부재 안에 끼워 넣는 것 — 물려 있는 게 정상(서랍은 책상 안에 든다)
NESTED = ("Drawer",)
# 집을 이루는 부재들 — 서로 물려 있는 게 정상(창은 벽 구멍에 끼우고, 기단은 벽을 두른다)
STRUCT = ("Facade", "AlleyWall", "Eave", "Wall_", "Win_", "Door_Unit", "Shop_Unit",
          "Plinth", "StringCourse", "WallRail")


def kind(a):
    sm = a.static_mesh_component.static_mesh
    return sm.get_name().replace("SM_Rasel_", "") if sm else "?"


def isa(k, group):
    return any(g in k for g in group)


def bounds(a):
    o, e = a.get_actor_bounds(False)
    return (o.x - e.x, o.y - e.y, o.z - e.z, o.x + e.x, o.y + e.y, o.z + e.z)


def overlap(b1, b2, pad=0.0):
    dx = min(b1[3], b2[3]) - max(b1[0], b2[0]) + pad
    dy = min(b1[4], b2[4]) - max(b1[1], b2[1]) + pad
    dz = min(b1[5], b2[5]) - max(b1[2], b2[2]) + pad
    if dx <= 0 or dy <= 0 or dz <= 0:
        return 0.0, (0, 0, 0)
    return dx * dy * dz, (dx, dy, dz)


unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
acts = [a for a in A().get_all_level_actors()
        if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh]
log("검사 대상 부재 %d개 — %s" % (len(acts), MAP))

info = [(a, kind(a), bounds(a)) for a in acts]

# ── ① 떠 있는 것 ──────────────────────────────────────────────────────
float_hits = []
for (a, k, b) in info:
    if isa(k, FLOOR):
        continue
    if b[2] <= 30.0:            # 바닥에 발이 닿아 있음
        continue
    touching = False
    for (a2, k2, b2) in info:
        if a2 is a:
            continue
        v, _ = overlap(b, b2, pad=25.0)     # 25cm 안에 뭔가 있으면 닿은 것으로 본다
        if v > 0:
            touching = True
            break
    if not touching:
        float_hits.append((a.get_actor_label(), k, b))
log("")
log("① 허공에 뜬 것: %d건" % len(float_hits))
for (lbl, k, b) in float_hits:
    log("   !! %-22s (%s) 바닥에서 %.0fcm 떠 있고 닿은 데 없음" % (lbl, k, b[2]))

# ── ② 서로 파고든 것 ──────────────────────────────────────────────────
DEEP = 12.0        # 세 축 모두 이만큼 이상 겹치면 '박혔다'로 본다
deep_hits = []
for i in range(len(info)):
    a, k, b = info[i]
    # 집·처마·담은 빼고 본다 — 벽에 붙는 물건이 벽과 겹치는 건 당연하고,
    # 여기서 보려는 건 '세간끼리 서로 파고든 것'이다.
    if isa(k, FLOOR) or isa(k, STRUCT) or isa(k, NESTED):
        continue
    for j in range(i + 1, len(info)):
        a2, k2, b2 = info[j]
        if isa(k2, FLOOR) or isa(k2, STRUCT) or isa(k2, NESTED):
            continue
        v, d = overlap(b, b2)
        if v > 0 and min(d) > DEEP:
            deep_hits.append((a.get_actor_label(), a2.get_actor_label(), d))
log("")
log("② 서로 파고든 짝: %d건 (세 축 모두 %.0fcm 초과)" % (len(deep_hits), DEEP))
for (l1, l2, d) in deep_hits[:40]:
    log("   !! %-20s ↔ %-20s  겹침 %.0f×%.0f×%.0f" % (l1, l2, d[0], d[1], d[2]))

# ── ③ 문 앞을 막은 것 ─────────────────────────────────────────────────
# 포탈(=드나드는 문) 앞 120cm 안에 무릎(40cm) 위로 솟은 부재가 있으면 막은 것
block = []
for p in A().get_all_level_actors():
    if p.get_class().get_name() != "PortalActor":
        continue
    pl = p.get_actor_location()
    zone = (pl.x - 110, pl.y - 110, 40.0, pl.x + 110, pl.y + 110, 200.0)
    for (a, k, b) in info:
        if isa(k, FLOOR) or isa(k, STRUCT):
            continue
        v, d = overlap(zone, b)
        if v > 0 and min(d) > 20.0:
            block.append((p.get_actor_label(), a.get_actor_label(), k))
log("")
log("③ 드나드는 문 앞을 막은 것: %d건" % len(block))
for (pl_, al, k) in block:
    log("   !! %s 앞에 %s (%s)" % (pl_, al, k))

log("")
log("=== 검사 끝 — 뜸 %d · 박힘 %d · 막음 %d ===" %
    (len(float_hits), len(deep_hits), len(block)))
with open(LOG, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
