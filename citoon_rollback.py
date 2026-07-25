# SK_VRM 머티리얼을 VRoid 원본(test2)으로 완전 복구
# 슬롯 이름 = 원본 MI 이름 규칙(MI_<slot>__Instance_)이라 그대로 되돌림. 외곽선 슬롯은 유지.
# 실행: exec(open(r"C:/Secret_Project/citoon_rollback.py").read())

import unreal
EAL = unreal.EditorAssetLibrary

SK = '/Game/TestCharacter/MAXIMO/mixamo/SK_VRM'
ORIG_DIR = '/Game/TestCharacter/test2/'

sk = unreal.load_asset(SK)
mats = list(sk.get_editor_property('materials'))
restored = 0
for i, sm in enumerate(mats):
    slot = str(sm.get_editor_property('material_slot_name'))
    clean = slot.replace('(Instance)', '').strip()
    if clean == 'M_Outline' or clean == '':
        continue  # 외곽선 슬롯 유지
    orig_path = ORIG_DIR + 'MI_' + clean + '__Instance_'
    if EAL.does_asset_exist(orig_path):
        orig = unreal.load_asset(orig_path)
        ns = unreal.SkeletalMaterial()
        ns.set_editor_property('material_interface', orig)
        ns.set_editor_property('material_slot_name', sm.get_editor_property('material_slot_name'))
        mats[i] = ns
        restored += 1
        unreal.log("[%d] restored: %s" % (i, orig_path.split('/')[-1]))
    else:
        unreal.log_warning("[%d] orig NOT FOUND, left as-is: %s" % (i, orig_path))

sk.set_editor_property('materials', mats)
EAL.save_asset(SK)
unreal.log("==== ROLLBACK DONE. restored slots: %d / %d" % (restored, len(mats)))
