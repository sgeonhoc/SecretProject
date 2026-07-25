# -*- coding: utf-8 -*-
u"""라셀 도시 맵 검수 — 폴더별 액터 수와 높이 범위를 센다."""
import unreal, collections

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/Maps/Rasel/Rasel_City")
A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = A.get_all_level_actors()

cnt = collections.Counter()
zlo = {}
zhi = {}
for a in acts:
    f = str(a.get_folder_path()) or "(폴더 없음)"
    cnt[f] += 1
    z = a.get_actor_location().z / 100.0
    zlo[f] = min(zlo.get(f, 1e9), z)
    zhi[f] = max(zhi.get(f, -1e9), z)

unreal.log("[audit] 총 액터 %d" % len(acts))
for f in sorted(cnt):
    unreal.log("[audit] %-34s %5d   z %8.1f ~ %8.1f m" % (f, cnt[f], zlo[f], zhi[f]))

# 세계 범위
xs = [a.get_actor_location().x / 100.0 for a in acts]
ys = [a.get_actor_location().y / 100.0 for a in acts]
unreal.log("[audit] 세계 범위  X %.0f ~ %.0f m (%.1f km)  ·  Y %.0f ~ %.0f m (%.1f km)"
           % (min(xs), max(xs), (max(xs) - min(xs)) / 1000.0,
              min(ys), max(ys), (max(ys) - min(ys)) / 1000.0))
