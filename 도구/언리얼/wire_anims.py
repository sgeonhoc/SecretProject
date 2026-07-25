# -*- coding: utf-8 -*-
# =============================================================================
#  배치2: 리타겟 패러곤 공격/피격 시퀀스를 SKEL_test2 캐릭터 BP에 배선
#   - BasicAttackSequences / HitReactionSequences (AnimSequence 다이내믹 몽타주 재생)
#   - 스킬은 전용 몽타주 없으면 기본공격으로 폴백 → 스킬도 함께 애니메이션
#   - ★ SKEL_test2 메시 캐릭터에만 적용(애니가 그 스켈레톤 전용). 다른 스켈레톤은 건너뜀.
# =============================================================================
import unreal
EAL = unreal.EditorAssetLibrary
BEL = unreal.BlueprintEditorLibrary
def log(m): unreal.log("[WireAnim] %s" % m)

PARA = "/Game/paragonanimRTGtoCharacters"
TARGET_SKEL = "/Game/TestCharacter/test2/SKEL_test2.SKEL_test2"

# 시퀀스 캐시
_items = None
def items():
    global _items
    if _items is None:
        _items = []
        for p in EAL.list_assets(PARA, True, False):
            base = p.split('.')[0]
            _items.append((base.split('/')[-1], base))
    return _items

def find(*subs, exclude=()):
    for (name, obj) in items():
        low = name.lower()
        if all(s.lower() in low for s in subs) and not any(e.lower() in low for e in exclude):
            return obj
    return None

def loadseq(*subs, **kw):
    o = find(*subs, exclude=kw.get("exclude", ()))
    return EAL.load_asset(o) if o else None

# 공격 시퀀스(다양) — additive/aim/MSA 제외
atk_specs = [("Attack01",), ("Attack_A",), ("Attack_A_Fast",), ("Attack_A_Med",), ("Attack_A_SetA",)]
attacks = []
for s in atk_specs:
    seq = loadseq(*s, exclude=("additive", "_ao_", "aim", "msa"))
    if seq: attacks.append(seq); log("공격: %s" % seq.get_name())
# 피격 시퀀스
hit_specs = [("HitReact_A",), ("HeadHit_1",), ("BigStomachHit",), ("BigSideHit",)]
hits = []
for s in hit_specs:
    seq = loadseq(*s, exclude=("additive",))
    if seq: hits.append(seq); log("피격: %s" % seq.get_name())

log("공격 %d / 피격 %d 시퀀스 준비" % (len(attacks), len(hits)))

def mesh_skel_path(cdo):
    try:
        comp = cdo.get_editor_property("mesh")
        m = comp.get_skeletal_mesh_asset() if comp else None
        sk = m.get_editor_property("skeleton") if m else None
        return sk.get_path_name() if sk else None
    except Exception:
        return None

# 모든 캐릭터 BP 순회
bp_paths = []
for p in EAL.list_assets("/Game/BP_Characters", True, False):
    if "ANPCCharacter" in p or "PlayerCharacter" in p:
        bp_paths.append(p.split('.')[0])

applied, skipped = 0, 0
for bpp in bp_paths:
    bp = EAL.load_asset(bpp)
    if not bp: continue
    try: cdo = unreal.get_default_object(bp.generated_class())
    except Exception: continue
    skel = mesh_skel_path(cdo)
    nm = bpp.split('/')[-1]
    if skel != TARGET_SKEL:
        skipped += 1
        continue
    try:
        if attacks: cdo.set_editor_property("BasicAttackSequences", attacks)
        if hits:    cdo.set_editor_property("HitReactionSequences", hits)
        BEL.compile_blueprint(bp)
        EAL.save_asset(bpp, only_if_is_dirty=False)
        # 검증: 리로드 후 개수 확인
        bp2 = EAL.load_asset(bpp)
        cdo2 = unreal.get_default_object(bp2.generated_class())
        n = len(cdo2.get_editor_property("BasicAttackSequences"))
        log("✓ %-26s SKEL_test2 → 공격%d개 배선(검증 %d)" % (nm, len(attacks), n))
        applied += 1
    except Exception as e:
        log("✗ %s 실패: %s" % (nm, e))

log("적용 %d / 건너뜀(다른 스켈) %d" % (applied, skipped))
log("==== 배치2 완료 ====")
