# celfix 메시 슬롯에 셀튜닝 test2 머티리얼 재매핑(슬롯명이 __Instance_ 형식이라 'MI_'+슬롯명).
import unreal
EAL = unreal.EditorAssetLibrary
log = []
def w(s): log.append(str(s))
SKP = '/Game/TestCharacter/CelFix/test2_celfix'
ORIG = '/Game/TestCharacter/test2/'
sk = unreal.load_asset(SKP)
mats = list(sk.get_editor_property('materials'))
newmats = []
for sm in mats:
    slot = str(sm.get_editor_property('material_slot_name'))
    cands = [ORIG + 'MI_' + slot,
             ORIG + 'MI_' + slot.replace('__Instance_', '').replace('(Instance)', '').strip() + '__Instance_']
    mi = None
    for c in cands:
        if EAL.does_asset_exist(c): mi = unreal.load_asset(c); break
    ns = unreal.SkeletalMaterial()
    ns.set_editor_property('material_interface', mi if mi else sm.get_editor_property('material_interface'))
    ns.set_editor_property('material_slot_name', sm.get_editor_property('material_slot_name'))
    newmats.append(ns)
    w("slot %s -> %s" % (slot, (mi.get_name() if mi else 'KEEP')))
sk.set_editor_property('materials', newmats)
EAL.save_asset(SKP)
w("DONE")
try: open("C:/Secret_Project/기획/99_보관/구로그/citoon_celfix_mats_log.txt", "w", encoding="utf-8").write("\n".join(log))
except Exception: pass
try:
    world = unreal.EditorLevelLibrary.get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
except Exception: pass
