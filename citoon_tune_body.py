# ============================================================================
#  CiciToon 그림자 튜닝 — 몸통(Body SKIN) 슬롯 1개만. "값 보며 확정" 루프용.
# ----------------------------------------------------------------------------
#  왜 필요?  1차 적용(citoon_apply.py)은 BaseTex/SSSTex 텍스처만 바꿨고,
#            정작 그림자를 만드는 스위치/색/오프셋은 안 건드려서 음영이 약했음.
#            여기서 그걸 직접 잡는다.
#
#  사용법:  ① 아래 CONFIG 숫자만 고친다  ② 에디터 Cmd(Python)에서 실행:
#              exec(open(r"C:/Secret_Project/citoon_tune_body.py").read())
#           ③ 뷰포트에서 몸통 그림자 확인 → 약하면 SHADOW_COLOR 더 어둡게 /
#              테두리 흐리면 SHADOW_SMOOTH 낮게 / 그림자 면적은 SHADOW_OFFSET
#              → 다시 실행. 또렷해질 때까지 이 파일만 반복 수정.
#           ④ 확정되면 citoon_copy_to_rest.py 실행 → 머리·옷에 같은 값 복제.
#
#  ※ 몸통 슬롯 1개만 건드린다. 머리/옷/얼굴은 안 만짐(복제는 4번에서).
# ============================================================================

# ----- CONFIG (이 숫자들만 만지면 됨) ---------------------------------------
USE_SSS_TEX   = False          # ★핵심: 끄면 SSSTex(밝은텍스처) 대신 아래 ShadowColor로 그림자. 켜면 그림자 약함.
SHADOW_COLOR  = (0.45, 0.42, 0.50)   # 그림자 색(선형). 더 어둡게=음영 진해짐. 살색이면 (0.55,0.40,0.40)처럼 따뜻하게도.
SHADOW_OFFSET = 0.50           # 그림자 경계 위치(0~1). 클수록 그림자 면적↑.
SHADOW_SMOOTH = 0.05           # 경계 부드러움. 작을수록 또렷한 셀(0.02~0.08 추천).
USE_RAMP      = False          # 램프(다단계 음영) 사용. 일단 2톤으로 보려면 False.
RAMP          = 0.50           # UseRamp=True일 때만 의미. 램프 위치/강도.
# ----------------------------------------------------------------------------

import unreal

mel  = unreal.MaterialEditingLibrary
EAL  = unreal.EditorAssetLibrary
SK_PATH = '/Game/TestCharacter/MAXIMO/mixamo/SK_VRM'
DST     = '/Game/TestCharacter/CiciVRoid'

log = []
def w(s):
    s = str(s); log.append(s); unreal.log(s)

sk = unreal.load_asset(SK_PATH)
if not sk:
    w("!! SK_VRM 로드 실패: %s" % SK_PATH); raise SystemExit
mats = list(sk.get_editor_property('materials'))

# --- 몸통(body+skin) 슬롯 찾기 ---
body_mi = None; body_idx = -1; body_slot = None
for i, sm in enumerate(mats):
    slot = str(sm.get_editor_property('material_slot_name')).lower()
    if 'body' in slot and 'skin' in slot:
        body_idx  = i
        body_slot = str(sm.get_editor_property('material_slot_name'))
        body_mi   = sm.get_editor_property('material_interface')
        break

if not body_mi:
    w("!! 몸통(body+skin) 슬롯을 못 찾음. 슬롯명 확인 필요."); raise SystemExit

mi_path = body_mi.get_path_name()
w("몸통 슬롯 [%d] %s" % (body_idx, body_slot))
w("  -> 현재 MI: %s" % mi_path)

# --- Cici MI가 안 걸려 있으면(=apply 안됐거나 롤백됨) 중단하고 안내 ---
if DST not in mi_path:
    w("")
    w("!! 이 슬롯에 CiciToon MI가 안 걸려 있음(현재=VRoid 원본).")
    w("!! 먼저 citoon_apply.py 를 실행해서 Cici를 입힌 뒤 다시 이 스크립트를 돌리세요.")
    raise SystemExit

# --- BEFORE 값 기록 ---
def get_scalar(name):
    try: return mel.get_material_instance_scalar_parameter_value(body_mi, name)
    except Exception as e: return "ERR(%s)" % e
def get_switch(name):
    try: return mel.get_material_instance_static_switch_parameter_value(body_mi, name)
    except Exception as e: return "ERR(%s)" % e
def get_vec(name):
    try:
        c = mel.get_material_instance_vector_parameter_value(body_mi, name)
        return "(%.3f, %.3f, %.3f)" % (c.r, c.g, c.b)   # ★문자열로 반환(튜플 %s 포맷 버그 회피)
    except Exception as e: return "ERR(%s)" % e

w("---- BEFORE ----")
w("  Use SSS Tex = %s" % get_switch('Use SSS Tex'))
w("  UseOMR      = %s" % get_switch('UseOMR'))
w("  UseSDF      = %s" % get_switch('UseSDF'))
w("  UseRamp     = %s" % get_switch('UseRamp'))
w("  ShadowColor = %s" % get_vec('ShadowColor'))
w("  ShadowOffset= %s" % get_scalar('ShadowOffset'))
w("  ShadowSmooth= %s" % get_scalar('ShadowSmooth'))
w("  Ramp        = %s" % get_scalar('Ramp'))

# --- SET ---
def set_scalar(name, v):
    try: mel.set_material_instance_scalar_parameter_value(body_mi, name, float(v)); w("  set scalar %s = %s" % (name, v))
    except Exception as e: w("  !! set scalar %s 실패: %s" % (name, e))
def set_vec(name, rgb):
    try:
        mel.set_material_instance_vector_parameter_value(body_mi, name, unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        w("  set vector %s = %s" % (name, rgb))
    except Exception as e: w("  !! set vector %s 실패: %s" % (name, e))
def set_switch(name, b):
    try: mel.set_material_instance_static_switch_parameter_value(body_mi, name, bool(b)); w("  set switch %s = %s" % (name, b))
    except Exception as e: w("  !! set switch %s 실패(에디터에서 수동 체크 필요): %s" % (name, e))

w("---- APPLY ----")
set_switch('Use SSS Tex', USE_SSS_TEX)
set_switch('UseOMR',      True)    # ★몸통 스킨은 OMR(AO) 켜야 함 — 끄면 그림자가 바닥까지 떨어져 새까매짐(실측 확인)
set_switch('UseSDF',      False)   # ★SDF는 얼굴 전용. 몸/머리/옷엔 끔
set_switch('UseRamp',     USE_RAMP)
set_vec('ShadowColor',    SHADOW_COLOR)
set_scalar('ShadowOffset', SHADOW_OFFSET)
set_scalar('ShadowSmooth', SHADOW_SMOOTH)
set_scalar('Ramp',         RAMP)

try: mel.update_material_instance(body_mi)
except Exception as e: w("  update_material_instance 경고: %s" % e)
EAL.save_asset(mi_path.split('.')[0])
w("==== 저장 완료. 뷰포트에서 몸통 그림자 확인하세요.")
w("     약하면 SHADOW_COLOR 더 어둡게, 테두리 흐리면 SHADOW_SMOOTH 낮게, 면적은 SHADOW_OFFSET.")

try:
    with open("C:/Secret_Project/기획/99_보관/구로그/citoon_tune_log.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(log))
except Exception as e:
    unreal.log_error("로그 쓰기 실패: " + str(e))
