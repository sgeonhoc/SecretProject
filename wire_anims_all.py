# -*- coding: utf-8 -*-
# =============================================================================
#  배치2b: 동일 VRoid 리깅 → SKEL_test2 리타겟 애니를 전 캐릭터에 적용
#   1) 각 캐릭터 스켈레톤에 SKEL_test2를 'Compatible Skeleton'으로 추가(비파괴)
#   2) test2 공격/피격 시퀀스를 전 캐릭터 BP에 배선
#   + BGM(battle/field/title) 덕디에서 배선
# =============================================================================
import unreal
EAL = unreal.EditorAssetLibrary
BEL = unreal.BlueprintEditorLibrary
def log(m): unreal.log("[AnimAll] %s" % m)

TARGET_SKEL_PATH = "/Game/TestCharacter/test2/SKEL_test2"
para = "/Game/paragonanimRTGtoCharacters"

# ── 시퀀스 준비 ──
_items = [(p.split('/')[-1].split('.')[0], p.split('.')[0]) for p in EAL.list_assets(para, True, False)]
def find(*subs, ex=()):
    for (n, o) in _items:
        l = n.lower()
        if all(s.lower() in l for s in subs) and not any(e.lower() in l for e in ex):
            return o
    return None
def seqs(specs, ex):
    out = []
    for s in specs:
        o = find(*s, ex=ex)
        if o:
            a = EAL.load_asset(o)
            if a: out.append(a)
    return out
attacks = seqs([("Attack01",), ("Attack_A_Fast",), ("Attack_A_Med",), ("Attack_A_SetA",), ("Attack_A_SetB",)], ("additive", "_ao_", "aim", "msa"))
hits    = seqs([("HitReact_A",), ("HeadHit_1",), ("BigStomachHit",), ("BigSideHit",)], ("additive",))
log("공격 %d / 피격 %d 시퀀스" % (len(attacks), len(hits)))

target_skel = EAL.load_asset(TARGET_SKEL_PATH)

# ── 캐릭터 BP + 스켈레톤 수집 ──
bp_paths = [p.split('.')[0] for p in EAL.list_assets("/Game/BP_Characters", True, False)
            if ("ANPCCharacter" in p or "PlayerCharacter" in p)]

skels = {}   # path -> USkeleton
bp_info = []  # (bpp, bp, cdo, skel)
for bpp in bp_paths:
    bp = EAL.load_asset(bpp)
    if not bp: continue
    try: cdo = unreal.get_default_object(bp.generated_class())
    except Exception: continue
    try:
        comp = cdo.get_editor_property("mesh"); m = comp.get_skeletal_mesh_asset() if comp else None
        sk = m.get_editor_property("skeleton") if m else None
    except Exception: sk = None
    bp_info.append((bpp, bp, cdo, sk))
    if sk: skels[sk.get_path_name()] = sk

log("캐릭터 BP %d개, 고유 스켈레톤 %d개" % (len(bp_info), len(skels)))

# ── 1) Compatible Skeletons 설정: 각 스켈레톤에 test2 추가 ──
compat_done = 0
PROP = None
for cand in ("compatible_skeletons", "CompatibleSkeletons"):
    try:
        _ = target_skel.get_editor_property(cand); PROP = cand; break
    except Exception: pass
log("compatible 프로퍼티: %s" % PROP)
if PROP and target_skel:
    for spath, sk in skels.items():
        if spath.split('.')[0] == TARGET_SKEL_PATH: continue  # 자기 자신 제외
        try:
            cur = list(sk.get_editor_property(PROP))
            paths = [c.get_path_name() if hasattr(c, "get_path_name") else str(c) for c in cur]
            if not any("SKEL_test2" in p for p in paths):
                cur.append(target_skel)
                sk.set_editor_property(PROP, cur)
                EAL.save_asset(sk.get_path_name().split('.')[0], only_if_is_dirty=False)
                compat_done += 1
        except Exception as e:
            log("  ! %s compat 실패: %s" % (spath.split('/')[-1], e))
log("Compatible Skeleton 추가: %d개 스켈레톤" % compat_done)

# ── 2) 전 캐릭터 BP에 시퀀스 배선 ──
applied = 0
for (bpp, bp, cdo, sk) in bp_info:
    try:
        if attacks: cdo.set_editor_property("BasicAttackSequences", attacks)
        if hits:    cdo.set_editor_property("HitReactionSequences", hits)
        BEL.compile_blueprint(bp)
        EAL.save_asset(bpp, only_if_is_dirty=False)
        applied += 1
    except Exception as e:
        log("  ! %s 배선 실패: %s" % (bpp.split('/')[-1], e))
log("애니 배선 캐릭터: %d개" % applied)

# ── 3) BGM 배선(덕디) ──
def dup(src_folder_base, *subs_dst):
    pass
def list_audio(folder):
    return [(p.split('/')[-1].split('.')[0], p.split('.')[0]) for p in EAL.list_assets(folder, True, False)]
def findf(folder, *subs):
    for (n, o) in list_audio(folder):
        if all(s.lower() in n.lower() for s in subs): return o
    return None
COMBAT = "/Game/SFX_ORGANIZED/01_SFX_효과음/04_Combat_Weapon_전투_무기"
AMB = "/Game/SFX_ORGANIZED/02_Ambience_공간음"
MAGIC = "/Game/SFX_ORGANIZED/01_SFX_효과음/02_Magic_Fantasy_마법_판타지"
if not EAL.does_directory_exist("/Game/Audio/BGM"): EAL.make_directory("/Game/Audio/BGM")
bgm_map = {
    "battle": findf(COMBAT, "High_Speed_Sword_Fight") or findf(COMBAT, "Rooftop_Chase_Combat") or findf(COMBAT, "Heavy_Melee"),
    "field":  (list_audio(AMB)[0][1] if list_audio(AMB) else None),
    "title":  findf(MAGIC, "Golden_Title_Shimmer") or findf(MAGIC, "Magical_Aura_Hum"),
}
for k, src in bgm_map.items():
    dst = "/Game/Audio/BGM/BGM_%s" % k
    if not src: log("  BGM_%s 소스 없음" % k); continue
    if EAL.does_asset_exist(dst): EAL.delete_asset(dst)
    if EAL.duplicate_asset(src, dst):
        EAL.save_asset(dst, only_if_is_dirty=False)
        log("  BGM_%s ← %s" % (k, src.split('/')[-1]))
log("==== 배치2b 완료 ====")
