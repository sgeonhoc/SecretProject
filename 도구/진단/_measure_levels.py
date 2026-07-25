# -*- coding: utf-8 -*-
"""라셀 레벨들의 실제 크기(바운딩 박스)와 액터 수를 한 에디터 세션에서 전부 잰다.
페르소나 스케일 판정용 — 추측 말고 실측.
실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=_measure_levels.py
결과: Saved/level_sizes.txt
"""
import unreal, os

EAL = unreal.EditorAssetLibrary
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ES  = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

OUT = "C:/Secret_Project/Saved/level_sizes.txt"
lines = []
def log(s):
    print(s); lines.append(s)

# Rasel 폴더의 모든 맵
root = "/Game/Maps/Rasel"
assets = EAL.list_assets(root, recursive=False)
maps = sorted(set(a.split(".")[0] for a in assets if EAL.does_asset_exist(a.split(".")[0]) and
                  unreal.EditorAssetLibrary.find_asset_data(a.split(".")[0]).asset_class_path.asset_name == "World"))

log("=== 라셀 레벨 실측 (footprint = 가로 x 세로 미터, 액터수) ===")
log("%-28s %10s %10s %8s %8s" % ("level", "가로m", "세로m", "높이m", "액터"))
for m in maps:
    try:
        LES.load_level(m)
    except Exception as e:
        log("%-28s  LOAD FAIL %s" % (m.split('/')[-1], e)); continue
    actors = ES.get_all_level_actors()
    mn = unreal.Vector(1e9, 1e9, 1e9)
    mx = unreal.Vector(-1e9, -1e9, -1e9)
    n = 0
    for a in actors:
        # 바닥/벽 등 렌더 액터의 바운즈만
        try:
            origin, ext = a.get_actor_bounds(only_colliding_components=False)
        except Exception:
            continue
        if ext.x <= 0 and ext.y <= 0 and ext.z <= 0:
            continue
        n += 1
        mn.x = min(mn.x, origin.x - ext.x); mx.x = max(mx.x, origin.x + ext.x)
        mn.y = min(mn.y, origin.y - ext.y); mx.y = max(mx.y, origin.y + ext.y)
        mn.z = min(mn.z, origin.z - ext.z); mx.z = max(mx.z, origin.z + ext.z)
    if n == 0:
        log("%-28s   (빈 레벨)" % m.split('/')[-1]); continue
    gx = (mx.x - mn.x) / 100.0
    gy = (mx.y - mn.y) / 100.0
    gz = (mx.z - mn.z) / 100.0
    log("%-28s %10.1f %10.1f %8.1f %8d" % (m.split('/')[-1], gx, gy, gz, len(actors)))

with open(OUT, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
log("\n-> " + OUT)
