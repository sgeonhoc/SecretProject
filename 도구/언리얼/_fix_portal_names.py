"""
재번호 뒤 맵 안의 문(PortalActor) 목적지 문자열을 새 이름으로 고친다.

  UnrealEditor-Cmd.exe C:/Secret_Project/Secret_Project.uproject ^
    -ExecutePythonScript="C:/Secret_Project/도구/언리얼/_fix_portal_names.py" -nullrhi -unattended -nosplash

★왜 따로 필요한가 (2026-07-25)
  맵 이름은 복제+삭제로 바꿨는데(_renumber_levels.py), 문이 목적지를 가리키는 방식이
  에셋 참조가 아니라 **문자열**(TargetLevelName)이다. 문자열은 리다이렉터로도 안 따라오고
  에셋 개명으로도 안 바뀐다. 그래서 맵을 하나씩 열어 손으로 고친다.
"""
import unreal

EAL = unreal.EditorAssetLibrary
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
DIR = "/Game/Maps/Rasel"

RENAMES = {
    "L08_Academy_Street":     "L06_Academy_Street",
    "L11_Riverbank":          "L07_Riverbank",
    "L10_Clinic":             "L08_Clinic",
    "L13_Newport_Site":       "L09_Newport_Site",
    "L14_Newport_Shaft":      "L10_Newport_Shaft",
    "L17_Sanatorium":         "L13_Seir_Sanatorium",
    "L12_Selan_Court":        "L14_Selan_Court",
    "L19_Abandoned_Platform": "L16_Abandoned_Platform",
    "L21_Underlayer_Gallery": "L17_Underlayer_Gallery",
    "L22_Ruin_Gate":          "L18_Ruin_Gate",
    "L23_Ruin_Keep":          "L19_Ruin_Keep",
}

# 목적지를 문자열로 들고 있는 프로퍼티들
PROPS = ["TargetLevelName", "GoalLevel", "LevelName", "DestinationLevel"]

LOG = []


def log(s):
    LOG.append(s)
    unreal.log(s)


def fix_current_level(map_name):
    n = 0
    for a in AS.get_all_level_actors():
        for prop in PROPS:
            try:
                v = a.get_editor_property(prop)
            except Exception:
                continue
            if not v:
                continue
            s = str(v)
            for old, new in RENAMES.items():
                if old in s:
                    a.set_editor_property(prop, s.replace(old, new))
                    log("    %s . %s : %s -> %s" % (a.get_actor_label(), prop, s, s.replace(old, new)))
                    n += 1
                    break
    return n


def main():
    log("=== 문 목적지 문자열 교정 ===")
    maps = [p for p in EAL.list_assets(DIR, recursive=False, include_folder=False)
            if EAL.does_asset_exist(p.split(".")[0])]
    total = 0
    touched = 0
    for p in sorted(set(m.split(".")[0] for m in maps)):
        name = p.rsplit("/", 1)[-1]
        if not LES.load_level(p):
            log("  ! 열기 실패: %s" % name)
            continue
        n = fix_current_level(name)
        if n:
            LES.save_current_level()
            log("  O %s — %d곳" % (name, n))
            total += n
            touched += 1
    log("\n고친 맵 %d개 · 문자열 %d곳" % (touched, total))
    with open("C:/Secret_Project/Saved/fix_portals.log", "w", encoding="utf-8") as f:
        f.write("\n".join(LOG))


main()
