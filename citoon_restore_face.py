# 얼굴(Face SKIN) 슬롯만 VRoid 원본으로 복구. 몸/머리/옷의 Cici 적용은 그대로 유지.
# 실행: exec(open(r"C:/Secret_Project/citoon_restore_face.py").read())

import unreal
EAL = unreal.EditorAssetLibrary

SK = '/Game/TestCharacter/MAXIMO/mixamo/SK_VRM'
ORIG_DIR = '/Game/TestCharacter/test2/'

sk = unreal.load_asset(SK)
mats = list(sk.get_editor_property('materials'))
done = 0
for i, sm in enumerate(mats):
    slot = str(sm.get_editor_property('material_slot_name'))
    s = slot.lower()
    if 'face' in s and 'skin' in s:   # 얼굴 피부 슬롯만
        clean = slot.replace('(Instance)', '').strip()
        orig_path = ORIG_DIR + 'MI_' + clean + '__Instance_'
        orig = unreal.load_asset(orig_path)
        if orig:
            ns = unreal.SkeletalMaterial()
            ns.set_editor_property('material_interface', orig)
            ns.set_editor_property('material_slot_name', sm.get_editor_property('material_slot_name'))
            mats[i] = ns
            done += 1
            unreal.log("[%d] FACE restored -> %s" % (i, orig_path.split('/')[-1]))
        else:
            unreal.log_warning("orig not found: " + orig_path)

sk.set_editor_property('materials', mats)
EAL.save_asset(SK)
unreal.log("==== FACE restore done: %d slot(s). 몸/머리/옷 Cici는 유지됨." % done)
