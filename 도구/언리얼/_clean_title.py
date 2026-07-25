# -*- coding: utf-8 -*-
"""Title 맵 청소 — 레벨 빌더가 `new_level` 실패 시 시작 화면 맵에 부재를 쏟아 놓은 것을 걷어낸다.
남기는 것(타이틀 화면 원본): NiagaraActor 2 · CameraActor TITLE_CAMERA ·
BP_ANPCCharacter*_C · 접미사 없는 DirectionalLight/SkyLight/PointLight/PlayerStart 각 1.
지우는 것: 라셀 부재 메시 액터 전부 · 우리 배선 액터(Portal/Lore/NPC) ·
SkyAtmosphere/ExponentialHeightFog/PostProcessVolume · 번호 붙은 여벌 빛·PlayerStart.
"""
import unreal, re

KEEP_EXACT = {"DirectionalLight", "SkyLight", "PointLight", "PlayerStart"}
DROP_CLASSES = ("SkyAtmosphere", "ExponentialHeightFog", "PostProcessVolume",
                "PortalActor", "LoreNoteActor", "ANPCCharacter")
lines = []


def log(m):
    lines.append(str(m))
    unreal.log("[clean] " + str(m))


LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES.load_level("/Game/Title")

doomed, kept = [], 0
for a in A.get_all_level_actors():
    cn = a.get_class().get_name()
    lbl = a.get_actor_label()
    drop = False
    if isinstance(a, unreal.StaticMeshActor):
        sm = a.static_mesh_component.static_mesh
        if sm and sm.get_path_name().startswith("/Game/Rasel/Meshes/"):
            drop = True
    if cn in DROP_CLASSES:
        drop = True
    if cn in ("DirectionalLight", "SkyLight", "PointLight", "PlayerStart"):
        # 접미사 없는 원본 하나만 남기고, 번호 붙은 여벌과 우리 라벨은 지운다
        if lbl not in KEEP_EXACT:
            drop = True
    if drop:
        doomed.append(a)
    else:
        kept += 1

log("지울 것 %d개 · 남길 것 %d개" % (len(doomed), kept))
for a in doomed:
    A.destroy_actor(a)

left = {}
for a in A.get_all_level_actors():
    k = a.get_class().get_name()
    left[k] = left.get(k, 0) + 1
log("청소 뒤 남은 것: %s" % left)

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
log("현재 월드: %s" % w.get_name())
log("save_current_level -> %s" % LES.save_current_level())

with open("C:/Secret_Project/Saved/clean_title.log", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
