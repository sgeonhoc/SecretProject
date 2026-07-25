# -*- coding: utf-8 -*-
"""옛 로스터 잔재 맵 은퇴 — 정본이 '폐기'로 명시한 편의 레벨을 `_Retired/`로 옮긴다(삭제 아님).

정본(기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md §5): 폐기 = 전당포 뒷방·전차역 승강장·의원 회관 뒷복도·
유리탑 로비·교실동 빈 강의실·셋집 옥탑. 스토리 근거 없이 도시 편의로 지은 것들.

★안전 규칙:
  · 지금 지도(정본 로스터 22)에 대응하는 맵은 절대 안 건드린다.
  · 다른 맵이 참조하는(문이 걸린) 맵은 옮기지 않고 '참조 남음'으로 보고만 한다
    — 먼저 그 문을 정리해야 하므로.
  · 삭제가 아니라 이동이다. `/Game/Maps/Rasel/_Retired/`.
"""
import unreal

MAPDIR = "/Game/Maps/Rasel"
RETIRE = "/Game/Maps/Rasel/_Retired"
lines = []


def log(m):
    lines.append(str(m))
    unreal.log("[retire] " + str(m))


EAL = unreal.EditorAssetLibrary
reg = unreal.AssetRegistryHelpers.get_asset_registry()

# 정본이 살리는 것(꼬리) — 이 꼬리를 가진 파일은 은퇴 대상에서 뺀다
CANON_TAILS = {
    "Jangteo_Street", "Antique_Shop", "Backalley", "Dolgan_Office", "Eatery",
    "Academy_Street", "Riverbank", "Clinic", "Newport_Site", "Newport_Shaft",
    "Dock_Wharf", "Warehouse", "Seir_Sanatorium", "Selan_Court", "Nerhan_Estate",
    "Abandoned_Platform", "Underlayer", "Ruin_Gate", "Ruin_Keep", "Cargo_Ship",
    "Din_Safehouse", "Yoa_Room",
}
# 상태 변형(정본 아님이지만 살려 둠)
KEEP_EXTRA = {"L02_Antique_Shop_Burnt"}

maps = [p.split(".")[0] for p in EAL.list_assets(MAPDIR, recursive=False, include_folder=False)]
names = sorted(set(p.rsplit("/", 1)[1] for p in maps))
names = [n for n in names if not n.endswith(("_next", "_scratch", "_park"))]

# 어느 맵이 남기고 어느 맵이 은퇴 대상인가
retire, keep = [], []
for n in names:
    tail_ok = any(n.endswith(t) for t in CANON_TAILS)
    if tail_ok or n in KEEP_EXTRA:
        keep.append(n)
    else:
        retire.append(n)

log("전체 %d · 남김 %d · 은퇴 후보 %d" % (len(names), len(keep), len(retire)))
log("은퇴 후보: %s" % ", ".join(retire))

# 은퇴 후보를 다른 (남기는) 맵이 참조하나? — 참조되면 문부터 정리해야 하므로 보류
def refs_to(map_short):
    path = "%s/%s" % (MAPDIR, map_short)
    out = []
    for other in keep:
        op = "%s/%s" % (MAPDIR, other)
        deps = reg.get_referencers(op, unreal.AssetRegistryDependencyOptions()) or []
        # 이름 문자열로도 잡는다(포탈 TargetLevelName은 FName이라 에셋 의존이 아닐 수 있음)
    return out

# 포탈 목표는 에셋 의존이 아니라 이름 문자열이므로, 남기는 맵들의 포탈을 실제로 읽어 확인
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
A = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
referenced = set()
for k in keep:
    try:
        LES.load_level("%s/%s" % (MAPDIR, k))
    except Exception:
        continue
    for a in A.get_all_level_actors():
        if a.get_class().get_name() == "PortalActor":
            tgt = str(a.get_editor_property("TargetLevelName"))
            referenced.add(tgt)

moved, held = [], []
for n in retire:
    if n in referenced:
        held.append(n)
        continue
    src = "%s/%s" % (MAPDIR, n)
    dst = "%s/%s" % (RETIRE, n)
    ok = EAL.rename_asset(src, dst)
    (moved if ok else held).append(n)
    log("  %s %s" % ("→ 은퇴함" if ok else "!! 이동 실패", n))

log("")
log("은퇴 이동 %d · 보류(남기는 맵이 아직 문으로 참조) %d" % (len(moved), len(held)))
for h in held:
    if h in referenced:
        log("   · %s — 아직 다른 맵이 문으로 가리킴, 그 문 먼저 정리 필요" % h)

with open("C:/Secret_Project/Saved/retire.log", "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
