# -*- coding: utf-8 -*-
# =============================================================================
#  VFX 재배선 v2: NS_<속성>을 '타격 이펙트'로 교체
#   문제: 기존 NS_fire 등이 Free_SPELLS의 Aura_*(은은한 오라) 복제라 타격감 0.
#   해결: FXVarietyPack(Cascade, 화려)·Free_Magic(Niagara) 의 임팩트 이펙트로 교체.
#         속성별로 '정지형 폭발/스톰' 위주(턴제 단발 타격에 적합, 화면에 확 보임).
#   ※ SpawnVFX(C++)가 Niagara·Cascade 둘 다 지원하도록 이미 수정됨.
#  실행: UnrealEditor-Cmd <uproject> -ExecutePythonScript=wire_vfx_impact.py -unattended -nopause -nosplash
# =============================================================================
import unreal
EAL = unreal.EditorAssetLibrary
def log(m): unreal.log("[VFX2] %s" % m)

VFX_DST = "/Game/VFX"
if not EAL.does_directory_exist(VFX_DST): EAL.make_directory(VFX_DST)

# 소스 폴더 (Fab 다운로드 3종 중 타격용 2종 + 마법진)
FXV = "/Game/FXVarietyPack/Particles"   # Cascade P_ky_*  (가장 화려)
FM  = "/Game/Free_Magic/VFX_Niagara"    # Niagara NS_Free_Magic_*
FS  = "/Game/Free_Spells/VFX_Niagara"   # 비타격(마법진/오라)만 사용

_cache = {}
def listing(folder):
    if folder not in _cache:
        items = []
        try:
            for p in EAL.list_assets(folder, True, False):
                base = p.split('.')[0]
                items.append((base.split('/')[-1], base))
        except Exception as e:
            log("list 실패 %s: %s" % (folder, e))
        _cache[folder] = items
    return _cache[folder]

def find(folder, *subs):
    for (name, obj) in listing(folder):
        low = name.lower()
        if all(s.lower() in low for s in subs):
            return obj
    return None

def dup(src, dstname):
    dst = "%s/%s" % (VFX_DST, dstname)
    if not src:
        log("  ! 소스없음 → %s 건너뜀" % dstname); return False
    if EAL.does_asset_exist(dst):
        try: EAL.delete_asset(dst)
        except Exception: pass
    ok = False
    try: ok = EAL.duplicate_asset(src, dst)
    except Exception as e: log("  ! dup 실패 %s: %s" % (dstname, e))
    if ok: EAL.save_asset(dst, only_if_is_dirty=False)
    log("  %-14s ← %s" % (dstname, src.split('/')[-1] if src else "?"))
    return bool(ok)

# ── 속성 → 타격 VFX 매핑 ──────────────────────────────────
#  '정지형 스톰/폭발' 우선(제자리 재생 + 속성색 뚜렷). FXVariety 우선, 없으면 Free_Magic 폴백.
vfx = {
    # 불: 화염 폭발/스톰
    "fire":     find(FXV, "fireStorm") or find(FXV, "fireBall") or find(FXV, "explosion") or find(FM, "Attack1"),
    # 얼음/물: 푸른 물 스톰
    "ice":      find(FXV, "aquaStorm") or find(FXV, "waterBallHit") or find(FXV, "waterBall") or find(FM, "Projectile2"),
    # 전격: 번개 스톰/임팩트
    "elec":     find(FXV, "thunderStorm") or find(FXV, "ThunderBallHit") or find(FXV, "thunderBall") or find(FXV, "lightning2"),
    # 질풍: 회오리(plain storm) — "storm"은 aquaStorm 등과도 매칭되므로 'ky_storm'로 고유 지정
    "wind":     find(FXV, "ky_storm") or find(FM, "Slash2") or find(FM, "Area1"),
    # 축복/빛: 금빛 광휘
    "light":    find(FXV, "healAura") or find(FXV, "shootingStar1") or find(FXV, "laser01"),
    # 저주/암흑: 암흑 스톰
    "dark":     find(FXV, "darkStorm") or find(FM, "Attack2"),
    # 물리: 베기/타격 임팩트
    "physical": find(FXV, "hit1") or find(FM, "Slash") or find(FXV, "hit2"),
    # 만능: 대폭발(궁극기 룩)
    "almighty": find(FXV, "explosion") or find(FS, "Explosion") or find(FM, "Attack2"),
}
log("==== 속성 타격 VFX 교체 ====")
for k, src in vfx.items():
    dup(src, "NS_%s" % k)

# ── 시전 마법진(바닥) — 비타격이라 Free_Spells/FX 마법진 사용 ──
cast = find(FXV, "magicCircle1") or find(FM, "Circle1") or find(FS, "Circle")
dup(cast, "NS_cast")

# ── 힐/버프/디버프 오라 — Free_Spells(캐릭터 감싸는 비타격 오라, 사용자 인정 용도) ──
support = {
    "heal":   find(FS, "Buff_Healing") or find(FS, "Aura_Healing") or find(FS, "LevelUp"),
    "buff":   find(FS, "LevelUp") or find(FS, "Buff_Fire") or find(FS, "Buff_Mana"),
    "debuff": find(FS, "Aura_Soul") or find(FS, "Buff_Soul") or find(FM, "Aura"),
    # 현재 턴 유닛 발밑 지면 마커(루프되는 마법진)
    "turnmark": find(FS, "Circle") or find(FM, "Circle1") or find(FXV, "magicCircle1"),
}
log("==== 지원(힐/버프/디버프) 오라 ====")
for k, src in support.items():
    dup(src, "NS_%s" % k)

# ── 시전 효과음(SFX_cast): 범용 마법 시전 휘오시/차징 ──
SFX_DST = "/Game/Audio/SFX"
MAGIC = "/Game/SFX_ORGANIZED/01_SFX_효과음/02_Magic_Fantasy_마법_판타지"
cast_sfx = (find(MAGIC, "Spell_Transition_Whoosh") or find(MAGIC, "Arcane_Vortex")
            or find(MAGIC, "Magical_Aura_Hum") or find(MAGIC, "Whoosh") or find(MAGIC, "Cast"))
if cast_sfx:
    dstp = "%s/SFX_cast" % SFX_DST
    if EAL.does_asset_exist(dstp):
        try: EAL.delete_asset(dstp)
        except Exception: pass
    if EAL.duplicate_asset(cast_sfx, dstp):
        EAL.save_asset(dstp, only_if_is_dirty=False)
        log("  SFX_cast       ← %s" % cast_sfx.split('/')[-1])
else:
    log("  ! SFX_cast 소스 없음")

# ── 검증: 타입 출력 ──
log("==== 검증(에셋 타입) ====")
for k in list(vfx.keys()) + ["cast"]:
    p = "%s/NS_%s" % (VFX_DST, k)
    if EAL.does_asset_exist(p):
        a = EAL.load_asset(p)
        log("  NS_%-10s = %s" % (k, type(a).__name__ if a else "None"))
    else:
        log("  NS_%-10s = (없음)" % k)
log("==== VFX2 완료 ====")
