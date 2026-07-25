import unreal
mesh = unreal.load_asset('/Game/TestCharacter/test7/SK_test7')
mats = list(mesh.get_editor_property('materials'))
out = []
for i, sm in enumerate(mats):
    slot = sm.get_editor_property('material_slot_name')
    mi = sm.get_editor_property('material_interface')
    out.append("[%d] slot=%s  mat=%s" % (i, slot, mi.get_name() if mi else None))
    if mi and isinstance(mi, unreal.MaterialInstance):
        try:
            tpv = mi.get_editor_property('texture_parameter_values')
            for p in tpv:
                info = p.get_editor_property('parameter_info')
                nm = info.get_editor_property('name')
                tex = p.get_editor_property('parameter_value')
                out.append("      texparam '%s' = %s" % (nm, tex.get_name() if tex else None))
        except Exception as e:
            out.append("      (tpv err %s)" % e)
open('C:/Secret_Project/기획/99_보관/구로그/_mat_inspect.txt', 'w', encoding='utf-8').write("\n".join(out))
unreal.log("MAT INSPECT DONE")
