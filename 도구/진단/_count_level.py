# -*- coding: utf-8 -*-
"""레벨 무게 재기 — 액터 수·고유 메시 수·삼각형 합계(성능 눈금)."""
import os, unreal, collections
_raw = os.environ.get("COUNT_MAP", "/Game/Maps/Rasel/L01_Jangteo_Street")
MAP = _raw[_raw.find("/Game/"):] if "/Game/" in _raw else _raw
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
c = collections.Counter(); tri = {}; other = collections.Counter()
for a in A.get_all_level_actors():
    if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh:
        sm = a.static_mesh_component.static_mesh
        c[sm.get_name()] += 1; tri[sm.get_name()] = sm.get_num_triangles(0)
    else:
        other[a.get_class().get_name()] += 1
tot = sum(tri[n] * k for n, k in c.items())
out = ["%s" % MAP,
       "부재 액터 %d개 · 고유 메시 %d종 · 삼각형 합계 %s" %
       (sum(c.values()), len(c), format(tot, ",")),
       "그 밖 액터: %s" % dict(other), ""]
for n, k in c.most_common():
    out.append("  %-26s ×%-4d tri %s" % (n, k, format(tri[n] * k, ",")))
open("C:/Secret_Project/Saved/count.log", "w", encoding="utf-8").write("\n".join(out))
