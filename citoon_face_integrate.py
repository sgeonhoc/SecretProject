# 얼굴 일체화: 얼굴 SKIN 슬롯에 CiciToon 셀 적용 (단 ★SDF 끔 = 예전 코분홍/입얼룩 원인 제거).
#   몸통과 동일한 그림자값을 읽어와 통일. 눈/눈썹/입/외곽선은 VRoid 원본 유지.
# 실행:  py "C:/Secret_Project/citoon_face_integrate.py"   (PIE 정지 상태)
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

    # 1) 몸통 MI에서 그림자값 읽기(통일용 단일 진실원)
    body = None
    for sm in mats:
        s = str(sm.get_editor_property('material_slot_name')).lower()
        if 'body' in s and 'skin' in s:
            body = sm.get_editor_property('material_interface'); break
    def gv(n, d):
        try: return mel.get_material_instance_vector_parameter_value(body, n)
        except Exception: return d
    def gs(n, d):
        try: return mel.get_material_instance_scalar_parameter_value(body, n)
        except Exception: return d
    def gsw(n, d):
        try:
            v = mel.get_material_instance_static_switch_parameter_value(body, n)
            return v[0] if isinstance(v, (tuple, list)) else v
        except Exception: return d
    sc   = gv('ShadowColor', unreal.LinearColor(0.66, 0.63, 0.70, 1.0))
    ssm  = gs('ShadowSmooth', 0.18)
    sof  = gs('ShadowOffset', 0.5)
    usss = gsw('Use SSS Tex', False)
    uramp= gsw('UseRamp', False)
    w("몸통값: ShadowColor=(%.2f,%.2f,%.2f) Smooth=%s Offset=%s UseSSS=%s" % (sc.r, sc.g, sc.b, ssm, sof, usss))

    # 2) 얼굴 Cici MI 찾기(없으면 MI_Kawa_Face 복제)
    face_mi_path = None
    try:
        for a in EAL.list_assets(DST, False, False):
            if 'Face' in a:
                face_mi_path = a.split('.')[0]; break
    except Exception as e:
        w("list_assets 예외: %s" % e)
    if not face_mi_path:
        src = '/Game/CiciToonCharacterShaderPak/Character/Hayakawa/Materials/MI_Kawa_Face'
        face_mi_path = DST + '/MI_Cici_Face'
        EAL.duplicate_asset(src, face_mi_path)
        w("얼굴 MI 신규복제: " + face_mi_path)
    face_mi = unreal.load_asset(face_mi_path)
    w("얼굴 MI = %s" % face_mi_path)

    # 3) 레시피 적용 (SDF 끔이 핵심)
    def ssw(n, v):
        try: mel.set_material_instance_static_switch_parameter_value(face_mi, n, bool(v)); w("  switch %s = %s" % (n, bool(v)))
        except Exception as e: w("  switch %s 실패: %s" % (n, e))
    ssw('UseSDF', False)        # ★핵심: 얼굴 깨짐 원인 제거
    ssw('UseOMR', False)        # 얼굴 OMR은 흰맵(무효)이라 꺼도 동일
    ssw('Use SSS Tex', usss)
    ssw('UseRamp', uramp)
    try:
        mel.set_material_instance_vector_parameter_value(face_mi, 'ShadowColor', sc)
        mel.set_material_instance_scalar_parameter_value(face_mi, 'ShadowSmooth', float(ssm))
        mel.set_material_instance_scalar_parameter_value(face_mi, 'ShadowOffset', float(sof))
        mel.update_material_instance(face_mi)
    except Exception as e:
        w("  param 실패: %s" % e)
    EAL.save_asset(face_mi_path)

    # 4) 얼굴 SKIN 슬롯에만 할당 (눈/눈썹/입/외곽선은 안 건드림)
    assigned = 0
    for i, sm in enumerate(mats):
        s = str(sm.get_editor_property('material_slot_name'))
        if 'face' in s.lower() and 'skin' in s.lower():
            ns = unreal.SkeletalMaterial()
            ns.set_editor_property('material_interface', face_mi)
            ns.set_editor_property('material_slot_name', sm.get_editor_property('material_slot_name'))
            mats[i] = ns; assigned += 1
            w("[%d] 얼굴 SKIN <- %s" % (i, face_mi_path.split('/')[-1]))
    sk.set_editor_property('materials', mats)
    EAL.save_asset(SK)
    w("==== 얼굴 일체화 완료(슬롯 %d개). SDF 끔. 눈/입/눈썹 원본 유지. PIE Play로 확인." % assigned)

try:
    with open("C:/Secret_Project/기획/99_보관/구로그/citoon_face_log.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(log))
except Exception as e:
    unreal.log_error("로그 쓰기 실패: " + str(e))
