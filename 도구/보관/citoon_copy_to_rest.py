# ============================================================================
#  CiciToon 그림자값 일괄복제 — 몸통에서 확정한 값을 머리/옷에 그대로 복사.
# ----------------------------------------------------------------------------
#  citoon_tune_body.py 로 몸통 그림자를 또렷하게 확정한 뒤 실행.
#  몸통 MI에서 그림자 관련 파라미터를 읽어(=단일 진실원) SK_VRM의 다른
#  Cici 슬롯(머리/옷)에 똑같이 써넣는다. 값을 다시 입력할 필요 없음.
#
#  자동 제외: 얼굴(VRoid 원본으로 복구돼 있어 /CiciVRoid/ 아래가 아님),
#            눈/눈썹/외곽선(원래 변환 안 함) → 전부 알아서 건너뜀.
#
#  실행:  exec(open(r"C:/Secret_Project/citoon_copy_to_rest.py").read())
# ============================================================================

import unreal

mel  = unreal.MaterialEditingLibrary
EAL  = unreal.EditorAssetLibrary
SK_PATH = '/Game/TestCharacter/MAXIMO/mixamo/SK_VRM'
DST     = '/Game/TestCharacter/CiciVRoid'

SCL_KEYS = ['ShadowOffset', 'ShadowSmooth', 'Ramp']
VEC_KEYS = ['ShadowColor']
SW_KEYS  = ['Use SSS Tex', 'UseRamp', 'UseSDF']   # ★UseOMR은 슬롯마다 적정값 달라 복제 제외(몸=켜야정상 / 머리=꺼야할수도)

log = []
def w(s):
    s = str(s); log.append(s); unreal.log(s)

sk = unreal.load_asset(SK_PATH)
mats = list(sk.get_editor_property('materials'))

# --- 몸통 MI(소스) 찾기 ---
src = None; src_idx = -1
for i, sm in enumerate(mats):
    slot = str(sm.get_editor_property('material_slot_name')).lower()
    if 'body' in slot and 'skin' in slot:
        src = sm.get_editor_property('material_interface'); src_idx = i; break
if not src or DST not in src.get_path_name():
    w("!! 몸통 Cici MI를 못 찾음. 먼저 citoon_tune_body.py로 몸통을 확정하세요."); raise SystemExit

# --- 소스 값 읽기 ---
vals_scl = {}; vals_vec = {}; vals_sw = {}
for k in SCL_KEYS:
    try: vals_scl[k] = mel.get_material_instance_scalar_parameter_value(src, k)
    except Exception as e: w("  소스 스칼라 %s 읽기 실패: %s" % (k, e))
for k in VEC_KEYS:
    try: vals_vec[k] = mel.get_material_instance_vector_parameter_value(src, k)
    except Exception as e: w("  소스 벡터 %s 읽기 실패: %s" % (k, e))
for k in SW_KEYS:
    try:
        v = mel.get_material_instance_static_switch_parameter_value(src, k)
        if isinstance(v, (tuple, list)): v = v[0]   # 일부 UE 버전은 (값, found) 튜플 반환
        vals_sw[k] = bool(v)
    except Exception as e: w("  소스 스위치 %s 읽기 실패: %s" % (k, e))

w("소스(몸통[%d]) 값: scl=%s sw=%s ShadowColor=(%.2f,%.2f,%.2f)" % (
    src_idx,
    {k: round(v,3) for k,v in vals_scl.items()},
    vals_sw,
    vals_vec.get('ShadowColor').r if 'ShadowColor' in vals_vec else 0,
    vals_vec.get('ShadowColor').g if 'ShadowColor' in vals_vec else 0,
    vals_vec.get('ShadowColor').b if 'ShadowColor' in vals_vec else 0))

# --- 대상(머리/옷 = /CiciVRoid/ 아래, 몸통 제외)에 복사 ---
copied = 0
for i, sm in enumerate(mats):
    if i == src_idx: continue
    mi = sm.get_editor_property('material_interface')
    if not mi: continue
    slot = str(sm.get_editor_property('material_slot_name'))
    if 'face' in slot.lower():          # ★얼굴은 절대 안 건드림(별도 과제)
        w("  [%d] %-38s -> skip(얼굴=별도과제)" % (i, slot)); continue
    p = mi.get_path_name()
    if DST not in p: continue          # 눈/눈썹/외곽선 등 원본 자동 제외
    for k, v in vals_scl.items():
        try: mel.set_material_instance_scalar_parameter_value(mi, k, v)
        except Exception as e: w("  [%d] 스칼라 %s 실패: %s" % (i, k, e))
    for k, v in vals_vec.items():
        try: mel.set_material_instance_vector_parameter_value(mi, k, v)
        except Exception as e: w("  [%d] 벡터 %s 실패: %s" % (i, k, e))
    for k, v in vals_sw.items():
        try: mel.set_material_instance_static_switch_parameter_value(mi, k, v)
        except Exception as e: w("  [%d] 스위치 %s 실패: %s" % (i, k, e))
    try: mel.update_material_instance(mi)
    except Exception: pass
    EAL.save_asset(p.split('.')[0])
    copied += 1
    w("  [%d] %-38s <- 복제 (%s)" % (i, slot, p.split('/')[-1]))

w("==== 일괄복제 완료: %d개 슬롯(머리/옷). 얼굴은 VRoid 원본 유지." % copied)

try:
    with open("C:/Secret_Project/기획/99_보관/구로그/citoon_copy_log.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(log))
except Exception as e:
    unreal.log_error("로그 쓰기 실패: " + str(e))
