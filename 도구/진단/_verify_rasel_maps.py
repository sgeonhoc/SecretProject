# -*- coding: utf-8 -*-
# 라셀 레벨 검수 — 맵을 하나씩 열어 실제 액터 수를 센다. 결과: Saved/rasel_verify.txt
import unreal

OUT = "C:/Secret_Project/Saved/rasel_verify.txt"
MAPS = [
    "L01_Jangteo_Street", "L02_Antique_Shop", "L03_Backalley", "L04_Rooftop_Room",
    "L05_Eatery", "L06_Pawnshop_Back", "L07_Tram_Jangteo", "L08_Academy_Street",
    "L09_Academy_Hall", "L10_Clinic", "L11_Riverbank", "L12_Selan_Court",
    "L13_Newport_Site", "L14_Newport_Shaft", "L15_Warehouse_Roof", "L16_Tower_Lobby",
    "L17_Sanatorium", "L18_Council_Hall", "L19_Abandoned_Platform", "L20_Sewer_Junction",
    "L21_Underlayer_Gallery", "L22_Ruin_Gate", "L23_Ruin_Keep",
]


def w(m):
    with open(OUT, "a", encoding="utf-8") as f:
        f.write(m + "\n")


open(OUT, "w", encoding="utf-8").close()
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
acts = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

for name in MAPS:
    try:
        les.load_level("/Game/Maps/Rasel/" + name)
        all_a = acts.get_all_level_actors()
        mesh = sum(1 for a in all_a if isinstance(a, unreal.StaticMeshActor))
        ps = sum(1 for a in all_a if isinstance(a, unreal.PlayerStart))
        lit = sum(1 for a in all_a if isinstance(a, (unreal.DirectionalLight, unreal.SkyLight)))
        w("%-24s 박스 %3d · PlayerStart %d · 조명 %d" % (name, mesh, ps, lit))
    except Exception as e:
        w("%-24s FAIL %s" % (name, e))
w("끝")
