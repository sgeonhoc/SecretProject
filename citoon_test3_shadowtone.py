# 테스트3: 셀 그림자 톤을 밝게+부드럽게. 검은 얼룩이 ShadowColor(툰 그림자면) 때문인지 확인 + 완화.
#   공유 머티리얼(MI_Cici_*)을 고치므로 배치/스폰 캐릭터 전부에 동시 적용.
# 실행:  py "C:/Secret_Project/citoon_test3_shadowtone.py"
#   ※ PIE 정지 상태에서 실행 → 다시 Play.  값은 아래 CONFIG만 고쳐 반복.

# ----- CONFIG -----
SHADOW_COLOR  = (0.66, 0.63, 0.70)   # 밝힐수록 그림자 옅어짐. 너무 밝으면 밋밋해짐.
SHADOW_SMOOTH = 0.18                 # 키울수록 경계 부드러움(검은 얼룩 덜 하드). 0.05=날카로움.
SHADOW_OFFSET = 0.50                 # 그림자 면적(0~1).
# ------------------

import unreal
mel = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
SK  = '/Game/TestCharacter/MAXIMO/mixamo/SK_VRM'
DST = '/Game/TestCharacter/CiciVRoid'

log = []
def w(s):
    s = str(s); log.append(s); unreal.log(s)

if not EAL.does_asset_exist(SK):
    w("!! SK_VRM 못 읽음 — 플레이모드일 수 있음. PIE 정지 후 다시.")
else:
    sk = unreal.load_asset(SK)
    mats = list(sk.get_editor_property('materials'))
    done = 0
    for i, sm in enumerate(mats):
        slot = str(sm.get_editor_property('material_slot_name'))
        if 'face' in slot.lower():
            continue
        mi = sm.get_editor_property('material_interface')
        if not mi or DST not in mi.get_path_name():
            continue
        p = mi.get_path_name()
        try:
            mel.set_material_instance_vector_parameter_value(mi, 'ShadowColor', unreal.LinearColor(SHADOW_COLOR[0], SHADOW_COLOR[1], SHADOW_COLOR[2], 1.0))
            mel.set_material_instance_scalar_parameter_value(mi, 'ShadowSmooth', float(SHADOW_SMOOTH))
            mel.set_material_instance_scalar_parameter_value(mi, 'ShadowOffset', float(SHADOW_OFFSET))
            mel.update_material_instance(mi)
            EAL.save_asset(p.split('.')[0])
            done += 1
            w("[%d] %-38s <- ShadowColor=%s Smooth=%s" % (i, slot, SHADOW_COLOR, SHADOW_SMOOTH))
        except Exception as e:
            w("[%d] %s 실패: %s" % (i, slot, e))
    w("==== 테스트3 완료: %d개 슬롯. PIE 다시 Play해서 검은 얼룩 보세요." % done)

try:
    with open("C:/Secret_Project/기획/99_보관/구로그/citoon_test3_log.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(log))
except Exception as e:
    unreal.log_error("로그 쓰기 실패: " + str(e))
