# -*- coding: utf-8 -*-
"""진행표(GameFlow)가 쓸 자리를 맵에서 직접 읽는다.

말로 적은 좌표는 레벨이 바뀌면 곧 어긋난다. 그래서 스테이지의 시작 자리·목표 자리를
정하기 전에 **맵에서 실제로 있는 것**을 긁는다:
  · PlayerStart (태그·위치)
  · PortalActor (어디로 가는 문인지·위치·도착 태그)
  · NPC/조사물/벽보 등 상호작용 액터 (이름·위치)
  · 바닥 윗면 z 와 걸어다닐 수 있는 대략의 범위(액터 바운드 합)
산출: C:/Secret_Project/Saved/stage_spots.json
실행: UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="C:/Secret_Project/_dump_stage_spots.py"
"""
import unreal, json, os

MAPS = [
    "/Game/Maps/Rasel/L22_Yoa_Room",
    "/Game/Maps/Rasel/L01_Jangteo_Street",
    "/Game/Maps/Rasel/L05_Eatery",
    "/Game/Maps/Rasel/L11_Riverbank",
    "/Game/Maps/Rasel/L02_Antique_Shop",
    "/Game/Maps/Rasel/L03_Backalley",
    "/Game/Maps/Rasel/L04_Dolgan_Office",
]

OUT = "C:/Secret_Project/Saved/stage_spots.json"
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def v(a):
    l = a.get_actor_location()
    return [round(l.x, 1), round(l.y, 1), round(l.z, 1)]


def prop(a, name, default=None):
    try:
        return a.get_editor_property(name)
    except Exception:
        return default


result = {}
for m in MAPS:
    if not unreal.EditorAssetLibrary.does_asset_exist(m):
        unreal.log_warning("[spots] 없는 맵: %s" % m)
        continue
    LES.load_level(m)
    actors = AS.get_all_level_actors()
    info = {"starts": [], "portals": [], "npcs": [], "interact": [], "lights": 0,
            "bounds": None, "actor_count": len(actors)}

    lo = [1e9, 1e9, 1e9]
    hi = [-1e9, -1e9, -1e9]
    floor_tops = []

    for a in actors:
        cls = a.get_class().get_name()
        if cls.startswith("PlayerStart"):
            info["starts"].append({"tag": str(prop(a, "player_start_tag", "")), "loc": v(a),
                                   "yaw": round(a.get_actor_rotation().yaw, 1)})
        elif "Portal" in cls:
            info["portals"].append({
                "to": str(prop(a, "target_level_name", "")),
                "entry": str(prop(a, "arrival_entry_tag", "")),
                "label": str(prop(a, "portal_label", "")),
                "loc": v(a), "yaw": round(a.get_actor_rotation().yaw, 1),
            })
        elif "NPC" in cls:
            info["npcs"].append({"name": str(prop(a, "npc_name", "")), "label": a.get_actor_label(), "loc": v(a)})
        elif "LoreNote" in cls or "Harvest" in cls or "Pickup" in cls or "LockedGate" in cls:
            info["interact"].append({"cls": cls, "label": a.get_actor_label(), "loc": v(a)})
        elif "Light" in cls:
            info["lights"] += 1

        # 바운드 합 — 걸어다닐 수 있는 대략의 범위
        try:
            org, ext = a.get_actor_bounds(False)
            for i, (o, e) in enumerate(zip([org.x, org.y, org.z], [ext.x, ext.y, ext.z])):
                lo[i] = min(lo[i], o - e)
                hi[i] = max(hi[i], o + e)
            # 바닥 후보 — 넓고 납작한 것의 윗면
            if ext.x > 150 and ext.y > 150 and ext.z < 120:
                floor_tops.append(round(org.z + ext.z, 1))
        except Exception:
            pass

    if lo[0] < 1e8:
        info["bounds"] = {"min": [round(x, 1) for x in lo], "max": [round(x, 1) for x in hi]}
    if floor_tops:
        info["floor_top_candidates"] = sorted(set(floor_tops))[:8]

    result[m.rsplit("/", 1)[1]] = info
    unreal.log("[spots] %s — 액터 %d · 시작 %d · 문 %d · NPC %d" %
               (m, len(actors), len(info["starts"]), len(info["portals"]), len(info["npcs"])))

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(result, f, ensure_ascii=False, indent=1)
unreal.log("[spots] 기록: %s" % OUT)
