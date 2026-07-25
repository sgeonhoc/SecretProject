# -*- coding: utf-8 -*-
# =============================================================================
#  종합 오디오 배선: 덕디 사운드 라이브러리 전체를 게임 이벤트에 최적 매핑.
#   - 전투: 속성8 + 연출(Flair)17 + 시전/힐/버프/디버프 + battlestart
#   - 메뉴 UI: 클릭/열기/닫기/스와이프/에러/확정/타이핑  ← NEW
#   - BGM: battle/field/title  ← 짧은 스팅어 대신 '제대로 루프되는 앰비언스'로 교체
#         (시작 시 외계전투 앰비언스/반짝임이 루프되던 "이상한 소리" 해결)
#   파일은 /Game/Audio/SFX/SFX_<name> · /Game/Audio/BGM/BGM_<name> 로 복제.
#   GameAudioSubsystem.PlaySFX("name") / PlayBGM("name")가 이 경로를 자동 로드.
#  실행: UnrealEditor-Cmd <uproject> -ExecutePythonScript=wire_audio_full.py -unattended -nopause -nosplash -nullrhi
# =============================================================================
import unreal
EAL = unreal.EditorAssetLibrary
def log(m): unreal.log("[Audio] %s" % m)

SFX_DST = "/Game/Audio/SFX"
BGM_DST = "/Game/Audio/BGM"
for d in ("/Game/Audio", SFX_DST, BGM_DST):
    if not EAL.does_directory_exist(d): EAL.make_directory(d)

B = "/Game/SFX_ORGANIZED/01_SFX_효과음"
UI    = B + "/01_UI_System_UI_시스템"
MAGIC = B + "/02_Magic_Fantasy_마법_판타지"
SCIFI = B + "/03_SciFi_Digital_SF_디지털"
COMBAT= B + "/04_Combat_Weapon_전투_무기"
CINE  = B + "/08_Cinematic_Transition_시네마틱_전환"
AMB   = "/Game/SFX_ORGANIZED/02_Ambience_공간음"

_cache = {}
def listing(folder):
    if folder not in _cache:
        items = []
        try:
            for p in EAL.list_assets(folder, True, False):
                base = p.split('.')[0]; items.append((base.split('/')[-1], base))
        except Exception as e: log("list 실패 %s: %s" % (folder, e))
        _cache[folder] = items
    return _cache[folder]

def find(folder, *subs):
    for (name, obj) in listing(folder):
        low = name.lower()
        if all(s.lower() in low for s in subs): return obj
    return None

def dup(dst_dir, dst_name, src):
    dst = "%s/%s" % (dst_dir, dst_name)
    if not src: log("  ! 소스없음 → %s 건너뜀" % dst_name); return False
    if EAL.does_asset_exist(dst):
        try: EAL.delete_asset(dst)
        except Exception: pass
    ok = False
    try: ok = EAL.duplicate_asset(src, dst)
    except Exception as e: log("  ! dup 실패 %s: %s" % (dst_name, e))
    if ok: EAL.save_asset(dst, only_if_is_dirty=False)
    log("  %-13s ← %s" % (dst_name, src.split('/')[-1] if src else "?"))
    return bool(ok)

# ── 속성 공격 SFX (스킬 SkillSFX=속성명 → 자동재생) ──
sfx = {
    "fire":     find(MAGIC, "Dragon_Fire_Burst"),
    "ice":      find(MAGIC, "Crystal_Crown_Forming"),
    "elec":     find(COMBAT, "Lightning_Sword_Spark"),
    "wind":     find(COMBAT, "Stylized_Sword_Swoosh"),
    "light":    find(MAGIC, "Divine_Smite"),
    "dark":     find(MAGIC, "Dark_Magic_Transformation"),
    "physical": find(COMBAT, "Epic_Weapon_Clash"),
    "almighty": find(MAGIC, "Vortex_Energy_Blast"),
    # ── 연출(Flair) ──
    "weak":      find(COMBAT, "Fiery_Sword_Impacts"),
    "critical":  find(COMBAT, "Heavy_Hammer_Swing_Impact"),
    "technical": find(MAGIC, "Arcane_Vortex_Spell"),
    "onemore":   find(UI, "Ui_Element_Pop"),
    "baton":     find(CINE, "Fast_Transition_Whoosh_1") or find(CINE, "Fast_Transition_Whoosh"),
    "allout":    find(COMBAT, "War_Title_Impact"),
    "miss":      find(COMBAT, "Knife_Slice_Whoosh"),
    "repel":     find(COMBAT, "Crystalline_Armor_Cracking"),
    "absorb":    find(MAGIC, "Magical_Aura_Hum"),
    "counter":   find(COMBAT, "Epic_Weapon_Clash"),
    "ailment":   find(MAGIC, "Dark_Magic_Transformation"),
    "endure":    find(COMBAT, "Armor_Settling"),
    "instakill": find(CINE, "Body_Thrown_Impact"),
    "ambush":    find(COMBAT, "Female_Warrior_Battle_Cry"),
    "ambushed":  find(MAGIC, "Dragon_Pained_Roar"),
    "victory":   find(CINE, "Final_Reveal_Flourish") or find(CINE, "Group_Reveal_Flourish"),
    "defeat":    find(MAGIC, "Dragon_S_Low_Growl") or find(MAGIC, "Low_Growl"),
    # ── 시전/지원 ──
    "cast":      find(MAGIC, "Spell_Transition_Whoosh"),
    "heal":      find(MAGIC, "Subtle_Magical_Shimmer"),
    "buff":      find(MAGIC, "Holy_Shield_Aura"),
    "debuff":    find(MAGIC, "Dark_Magic_Transformation"),
    "battlestart": find(COMBAT, "War_Title_Impact"),
    # ── 메뉴 UI (NEW) ──
    "ui_click":   find(UI, "Button_Click"),
    "ui_open":    find(UI, "Ui_Element_Pop") or find(UI, "Phone_Ui_Pop"),
    "ui_close":   find(UI, "Ui_Cards_Burst_Collapse"),
    "ui_swipe":   find(UI, "Clean_Ui_Swipe"),
    "ui_error":   find(UI, "Error_Page_Appearance"),
    "ui_confirm": find(UI, "Voice_Button_Chime"),
    "ui_type":    find(UI, "Subtitle_Text_Typing") or find(UI, "Text_Typing_Sequence"),
}
log("==== SFX 복제 (%d개) ====" % len(sfx))
for k, src in sfx.items(): dup(SFX_DST, "SFX_%s" % k, src)

# ── BGM: 짧은 스팅어 금지 → 제대로 루프되는 앰비언스 ──
bgm = {
    "battle": find(AMB, "Intense_Battlefield_Chaos") or find(AMB, "Large_Scale_Battle"),
    "field":  find(AMB, "Peaceful_Village_Morning") or find(AMB, "Pastoral_Landscape"),
    "title":  find(AMB, "Clean_Airy_Ambience") or find(AMB, "Ethereal_Drone"),
}
log("==== BGM 복제 (루프 앰비언스) ====")
for k, src in bgm.items(): dup(BGM_DST, "BGM_%s" % k, src)

# ── 검증: 누락 카운트 ──
miss = [k for k, v in list(sfx.items()) + [("BGM_"+k, v) for k, v in bgm.items()] if not v]
log("==== 완료. 누락=%d %s ====" % (len(miss), miss if miss else ""))
