# UE 헤드리스: Hanekawa 메시 FBX + 슬롯별 알베도 텍스처(PNG) 추출 → Blender 렌더 입력용.
import unreal, os, json
OUT = "C:/Secret_Project/_blender/hayakawa"
try: os.makedirs(OUT, exist_ok=True)
except Exception: pass
log = []
def w(s): s = str(s); log.append(s)
def flush():
    try: open(OUT + "/export_log.txt", "w", encoding="utf-8").write("\n".join(log))
    except Exception: pass

SKM = '/Game/CiciToonCharacterShaderPak/Character/Hayakawa/SKM_Hayakawa_Main'
mesh = unreal.load_asset(SKM)
w("mesh = %s" % (mesh.get_name() if mesh else "NONE"))

# 1) FBX export
def export_asset(obj, filename, exporter=None):
    t = unreal.AssetExportTask()
    t.set_editor_property('object', obj)
    t.set_editor_property('filename', filename)
    t.set_editor_property('automated', True)
    t.set_editor_property('replace_identical', True)
    t.set_editor_property('prompt', False)
    if exporter is not None:
        t.set_editor_property('exporter', exporter)
    try:
        return unreal.Exporter.run_asset_export_task(t)
    except Exception as e:
        w("  export err %s: %s" % (filename, e)); return False

if mesh:
    opt = None
    try: opt = unreal.FbxExportOption()
    except Exception: pass
    t = unreal.AssetExportTask()
    t.set_editor_property('object', mesh)
    t.set_editor_property('filename', OUT + "/hayakawa.fbx")
    t.set_editor_property('automated', True)
    t.set_editor_property('replace_identical', True)
    t.set_editor_property('prompt', False)
    if opt: t.set_editor_property('options', opt)
    try:
        ok = unreal.Exporter.run_asset_export_task(t); w("FBX export ok=%s" % ok)
    except Exception as e:
        w("FBX export ERR: %s" % e)

    # 2) 슬롯별 BaseTex(알베도) export
    mel = unreal.MaterialEditingLibrary
    mats = list(mesh.get_editor_property('materials'))
    mapping = []
    for i, sm in enumerate(mats):
        slot = str(sm.get_editor_property('material_slot_name'))
        mi = sm.get_editor_property('material_interface')
        tex = None
        if mi:
            for pn in ['BaseTex', 'gltf_tex_diffuse', 'MainTex', 'Diffuse', 'BaseColor']:
                try:
                    tt = mel.get_material_instance_texture_parameter_value(mi, pn)
                    if tt: tex = tt; break
                except Exception: pass
        texfile = ''
        if tex:
            safe = ''.join(c for c in slot if c.isalnum())[:18]
            texfile = OUT + "/tex_%02d_%s.png" % (i, safe)
            okx = export_asset(tex, texfile)
            w("  [%d] %-30s tex=%s exported=%s" % (i, slot, tex.get_name(), okx))
        else:
            w("  [%d] %-30s (no albedo param)" % (i, slot))
        mapping.append({'index': i, 'slot': slot,
                        'material': mi.get_name() if mi else '',
                        'tex': os.path.basename(texfile) if texfile and os.path.exists(texfile) else ''})
    try: open(OUT + "/mapping.json", "w", encoding="utf-8").write(json.dumps(mapping, ensure_ascii=False, indent=1))
    except Exception as e: w("mapping write err %s" % e)
    w("DONE. files:")
    try:
        for f in os.listdir(OUT): w("  " + f)
    except Exception: pass
flush()

try:
    world = unreal.EditorLevelLibrary.get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
except Exception: pass
