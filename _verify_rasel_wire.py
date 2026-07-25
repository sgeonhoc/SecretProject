# -*- coding: utf-8 -*-
# 라셀 배선 검수 — 맵을 하나씩 열어 배선 액터를 타입별로 세고, 포탈 목표가 실존 맵인지 본다.
#   결과: Saved/rasel_wire_verify.txt
import unreal

OUT = "C:/Secret_Project/Saved/rasel_wire_verify.txt"
MAPS = [
    "L01_Jangteo_Street", "L02_Antique_Shop", "L03_Backalley", "L04_Rooftop_Room",
    "L05_Eatery", "L06_Pawnshop_Back", "L07_Tram_Jangteo", "L08_Academy_Street",
    "L09_Academy_Hall", "L10_Clinic", "L11_Riverbank", "L12_Selan_Court",
    "L13_Newport_Site", "L14_Newport_Shaft", "L15_Warehouse_Roof", "L16_Tower_Lobby",
    "L17_Sanatorium", "L18_Council_Hall", "L19_Abandoned_Platform", "L20_Sewer_Junction",
    "L21_Underlayer_Gallery", "L22_Ruin_Gate", "L23_Ruin_Keep",
]
MAPSET = set(MAPS)

WIRE = ("PortalActor", "FastTravelPointActor", "SavePointActor",
        "LoreNoteActor", "LockedGateActor", "ANPCCharacter")


def w(m):
    with open(OUT, "a", encoding="utf-8") as f:
        f.write(m + "\n")


def class_names(a):
    out = []
    cls = a.get_class()
    while cls is not None:
        out.append(cls.get_name())
        try:
            cls = cls.get_super_class()
        except Exception:
            cls = None
    return out


open(OUT, "w", encoding="utf-8").close()
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
acts = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

tot = {k: 0 for k in WIRE}
dangling = []
mutual_missing = []
# 포탈 그래프: 맵 -> 목표 집합
graph = {}

for name in MAPS:
    les.load_level("/Game/Maps/Rasel/" + name)
    all_a = acts.get_all_level_actors()
    c = {k: 0 for k in WIRE}
    targets = set()
    for a in all_a:
        names = class_names(a)
        for k in WIRE:
            if k in names:
                c[k] += 1
                if k == "PortalActor":
                    try:
                        t = str(a.get_editor_property("TargetLevelName"))
                        targets.add(t)
                        if t not in MAPSET:
                            dangling.append("%s → %s (없는 맵)" % (name, t))
                    except Exception:
                        pass
                break
    graph[name] = targets
    for k in WIRE:
        tot[k] += c[k]
    w("%-24s 포탈%d 전차%d 취침%d 조사%d 문%d NPC%d" %
      (name, c["PortalActor"], c["FastTravelPointActor"], c["SavePointActor"],
       c["LoreNoteActor"], c["LockedGateActor"], c["ANPCCharacter"]))

# 상호연결 점검: A→B 이면 B→A 도 있어야 자연스러움(막다른 편도 출구 경고)
for src, tgts in graph.items():
    for t in tgts:
        if t in graph and src not in graph[t]:
            mutual_missing.append("%s → %s (역방향 없음)" % (src, t))

w("")
w("합계  포탈%d 전차%d 취침%d 조사%d 문%d NPC%d" %
  (tot["PortalActor"], tot["FastTravelPointActor"], tot["SavePointActor"],
   tot["LoreNoteActor"], tot["LockedGateActor"], tot["ANPCCharacter"]))
w("끊긴 출구(없는 맵 가리킴): %d" % len(dangling))
for d in dangling:
    w("  ✗ " + d)
w("편도 출구(역방향 없음): %d" % len(mutual_missing))
for m in mutual_missing:
    w("  △ " + m)
w("끝")
