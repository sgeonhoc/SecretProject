# CiciToon 셀셰이더를 VRoid(SK_VRM)에 1차 적용
# - 피부/머리/옷 슬롯만 CiciToon으로 (눈/눈썹/입/외곽선은 VRoid 원본 유지)
# - MI_Kawa_* 복제(스위치 셋업 상속) + BaseTex를 VRoid diffuse로 교체
# - 원본 MI는 안 건드림. 새 MI는 /Game/TestCharacter/CiciVRoid/. 슬롯 연결만 변경.
# 실행: Cmd(Python)  exec(open(r"C:/Secret_Project/citoon_apply.py").read())
#       또는 Cmd      py "C:/Secret_Project/citoon_apply.py"

import unreal

mel = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary

MASTER_DIR = '/Game/CiciToonCharacterShaderPak/Character/Hayakawa/Materials/'
DST = '/Game/TestCharacter/CiciVRoid'
SK_PATH = '/Game/TestCharacter/MAXIMO/mixamo/SK_VRM'

log = []
def w(s):
    log.append(str(s)); unreal.log(str(s))

def parent_for(slot):
    s = slot.lower()
    if 'body' in s and 'skin' in s: return 'MI_Kawa_Body'
    if 'face' in s and 'skin' in s: return 'MI_Kawa_Face'
    if 'hair' in s: return 'MI_Kawa_Hair'
    if 'cloth' in s: return 'MI_Kawa_Cloth'
    return None  # eye / brow / eyeline / mouth / outline -> VRoid 원본 유지

if not EAL.does_directory_exist(DST):
    EAL.make_directory(DST)

sk = unreal.load_asset(SK_PATH)
mats = list(sk.get_editor_property('materials'))
w("slot count: %d" % len(mats))
changed = 0

for i, sm in enumerate(mats):
    slot = str(sm.get_editor_property('material_slot_name'))
    pname = parent_for(slot)
    if not pname:
        w("[%d] %-40s -> skip (VRoid 원본 유지)" % (i, slot)); continue
    parent_path = MASTER_DIR + pname
    if not EAL.does_asset_exist(parent_path):
        w("[%d] parent NOT FOUND: %s" % (i, parent_path)); continue
    src_mi = sm.get_editor_property('material_interface')
    diffuse = None
    try:
        diffuse = mel.get_material_instance_texture_parameter_value(src_mi, 'gltf_tex_diffuse')
    except Exception as e:
        w("    [%d] diffuse read err: %s" % (i, e))
    safe = ''.join(c for c in slot if c.isalnum())[:24]
    new_path = "%s/MI_Cici_%02d_%s" % (DST, i, safe)
    try:
        if EAL.does_asset_exist(new_path): EAL.delete_asset(new_path)
        new_mi = EAL.duplicate_asset(parent_path, new_path)
        if not new_mi:
            w("[%d] duplicate FAILED" % i); continue
        if diffuse:
            mel.set_material_instance_texture_parameter_value(new_mi, 'BaseTex', diffuse)
            mel.set_material_instance_texture_parameter_value(new_mi, 'SSSTex', diffuse)
        EAL.save_asset(new_path)
        ns = unreal.SkeletalMaterial()
        ns.set_editor_property('material_interface', new_mi)
        ns.set_editor_property('material_slot_name', sm.get_editor_property('material_slot_name'))
        mats[i] = ns
        changed += 1
        w("[%d] %-40s -> %s  (parent=%s, diffuse=%s)" % (i, slot, new_path.split('/')[-1], pname, diffuse.get_name() if diffuse else "None"))
    except Exception as e:
        w("[%d] ERROR: %s" % (i, e))

sk.set_editor_property('materials', mats)
EAL.save_asset(SK_PATH)
w("DONE. changed slots: %d / %d" % (changed, len(mats)))

try:
    with open("C:/Secret_Project/기획/99_보관/구로그/citoon_apply_log.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(log))
    unreal.log("==== WROTE: C:/Secret_Project/기획/99_보관/구로그/citoon_apply_log.txt")
except Exception as e:
    unreal.log_error("write failed: " + str(e))
