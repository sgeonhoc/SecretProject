# -*- coding: utf-8 -*-
# =============================================================================
#  배치1: 덕디 SFX + Niagara VFX를 게임 컨벤션 이름으로 복제 → 전투 자동 재생
#   - /Game/Audio/SFX/SFX_<속성>     (fire/ice/elec/wind/light/dark/physical/almighty)
#   - /Game/Audio/SFX/SFX_<플레어>   (weak/critical/.../victory/defeat)  ← BattleManager.Flair
#   - /Game/VFX/NS_<속성>            ← 스킬 SkillVFX(속성명) 자동 스폰
#  실행: UnrealEditor-Cmd <uproject> -ExecutePythonScript=wire_audio_vfx.py -unattended -nopause -nosplash
# =============================================================================
import unreal
EAL = unreal.EditorAssetLibrary
def log(m): unreal.log("[WireAV] %s" % m)

SFX_DST = "/Game/Audio/SFX"
VFX_DST = "/Game/VFX"
for d in (SFX_DST, VFX_DST, "/Game/Audio", "/Game/Audio/BGM"):
    if not EAL.does_directory_exist(d): EAL.make_directory(d)

# 폴더 캐시: {folder: [(name, objpath), ...]}
_cache = {}
def listing(folder):
    if folder not in _cache:
        items = []
        try:
            for p in EAL.list_assets(folder, True, False):  # recursive, no folders
                base = p.split('.')[0]
                name = base.split('/')[-1]
                items.append((name, base))
        except Exception as e:
            log("list 실패 %s: %s" % (folder, e))
        _cache[folder] = items
    return _cache[folder]

def find(folder, *subs):
    """folder 하위에서 모든 sub 포함하는 첫 에셋 objpath."""
    for (name, obj) in listing(folder):
        low = name.lower()
        if all(s.lower() in low for s in subs):
            return obj
    return None

def dup(src, dst):
    if not src:
        log("  ! 소스 없음 → %s 건너뜀" % dst); return False
    if EAL.does_asset_exist(dst):
        try: EAL.delete_asset(dst)
        except Exception: pass
    ok = False
    try: ok = EAL.duplicate_asset(src, dst)
    except Exception as e: log("  ! dup 실패 %s: %s" % (dst, e))
    if ok: EAL.save_asset(dst, only_if_is_dirty=False)
    log("  %s ← %s" % (dst.split('/')[-1], src.split('/')[-1] if src else "?"))
    return bool(ok)

# ── SFX 소스 폴더 ──
B = "/Game/SFX_ORGANIZED/01_SFX_효과음"
MAGIC = B + "/02_Magic_Fantasy_마법_판타지"
COMBAT = B + "/04_Combat_Weapon_전투_무기"
UI = B + "/01_UI_System_UI_시스템"
CINE = B + "/08_Cinematic_Transition_시네마틱_전환"

# 속성 SFX (스킬 SkillSFX=속성명 → SFX_<속성> 자동재생)
sfx_attr = {
    "fire":     find(MAGIC, "Dragon_Fire_Burst") or find(MAGIC, "Fire"),
    "ice":      find(MAGIC, "Crystal_Crown") or find(MAGIC, "Crystal"),
    "elec":     find(COMBAT, "Lightning_Sword_Spark") or find(COMBAT, "Electrical"),
    "wind":     find(MAGIC, "Spell_Transition_Whoosh") or find(COMBAT, "Swoosh"),
    "light":    find(MAGIC, "Divine_Smite") or find(MAGIC, "Holy"),
    "dark":     find(MAGIC, "Dark_Magic_Transformation") or find(MAGIC, "Dark"),
    "physical": find(COMBAT, "Epic_Weapon_Clash") or find(COMBAT, "Sword_Impact") or find(COMBAT, "Clash"),
    "almighty": find(MAGIC, "Vortex_Energy_Blast") or find(MAGIC, "Vortex"),
}
# 연출(Flair) SFX
sfx_flair = {
    "weak":      find(COMBAT, "Fiery_Sword") or find(MAGIC, "Divine_Smite"),
    "critical":  find(COMBAT, "Epic_Weapon_Clash") or find(COMBAT, "Clash"),
    "technical": find(MAGIC, "Vortex_Energy_Blast") or find(MAGIC, "Arcane_Vortex"),
    "onemore":   find(UI, "Clean_Ui_Swipe") or find(UI, "Swipe"),
    "baton":     find(CINE, "Bright_Flash") or find(CINE, "Flash"),
    "allout":    find(CINE, "Bright_Magical_Flash") or find(MAGIC, "Vortex_Energy_Blast"),
    "miss":      find(COMBAT, "Stylized_Sword_Swoosh") or find(COMBAT, "Swoosh"),
    "repel":     find(COMBAT, "Sword_Crumbling") or find(COMBAT, "Armor_Settling"),
    "absorb":    find(MAGIC, "Magical_Aura_Hum") or find(MAGIC, "Aura_Hum"),
    "counter":   find(COMBAT, "Epic_Weapon_Clash") or find(COMBAT, "Clash"),
    "ailment":   find(MAGIC, "Dark_Magic_Transformation") or find(MAGIC, "Dark"),
    "endure":    find(COMBAT, "Armor_Settling") or find(COMBAT, "Armor"),
    "instakill": find(CINE, "Body_Thrown_Impact") or find(CINE, "Impact"),
    "ambush":    find(COMBAT, "Medieval_battle_cry") or find(COMBAT, "battle_cry"),
    "ambushed":  find(MAGIC, "Dragon_Pained_Roar") or find(MAGIC, "Pained"),
    "victory":   find(COMBAT, "War_Title_Impact") or find(COMBAT, "Medieval_battle_cry"),
    "defeat":    find(MAGIC, "Dragon_Pained_Roar") or find(MAGIC, "Low_Growl"),
}
# 전투 BGM 후보(긴장 드론 루프)
bgm = { "battle": find(COMBAT, "Pre_Battle_Tense_Drone") or find(COMBAT, "Tense") }

log("==== SFX 복제 ====")
for k, src in sfx_attr.items():  dup(src, "%s/SFX_%s" % (SFX_DST, k))
for k, src in sfx_flair.items(): dup(src, "%s/SFX_%s" % (SFX_DST, k))
for k, src in bgm.items():
    if src: dup(src, "/Game/Audio/BGM/BGM_%s" % k)

# ── VFX(Niagara) 속성 매핑 ──
FS = "/Game/Free_Spells/VFX_Niagara"
FM = "/Game/Free_Magic/VFX_Niagara"
vfx_attr = {
    "fire":     find(FS, "Aura_Fire") or find(FM, "Attack1"),
    "ice":      find(FS, "Aura_Mana") or find(FM, "Projectile2"),
    "elec":     find(FS, "Aura_Lightning"),
    "wind":     find(FS, "Aura_Air"),
    "light":    find(FS, "Aura_Healing"),
    "dark":     find(FS, "Aura_Soul"),
    "physical": find(FM, "Slash") or find(FM, "Attack1"),
    "almighty": find(FS, "Explosion") or find(FM, "Attack2"),
}
log("==== VFX 복제 ====")
for k, src in vfx_attr.items(): dup(src, "%s/NS_%s" % (VFX_DST, k))

log("==== 배치1 완료 ====")
