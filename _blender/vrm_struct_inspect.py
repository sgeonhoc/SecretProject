# Blender headless: test2.vrm 임포트 + 구조 덤프 → C:/Secret_Project/_blender/inspect_log.txt
import bpy
LOG = "C:/Secret_Project/_blender/inspect_log.txt"; out = []
def w(s): out.append(str(s)); print("INSPECT> " + str(s))
def flush():
    try:
        with open(LOG, "w", encoding="utf-8") as f: f.write("\n".join(out))
    except Exception as e: print("flush err", e)

w("Blender " + bpy.app.version_string)
w("has import_scene.vrm: %s" % hasattr(bpy.ops.import_scene, 'vrm'))
if not hasattr(bpy.ops.import_scene, 'vrm'):
    for mod in ('bl_ext.blender_org.vrm', 'vrm', 'io_scene_vrm'):
        try: bpy.ops.preferences.addon_enable(module=mod); w("enabled " + mod)
        except Exception as e: w("enable %s fail %s" % (mod, e))
w("has import_scene.vrm now: %s" % hasattr(bpy.ops.import_scene, 'vrm'))

# 기본 오브젝트 제거
try:
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()
except Exception as e: w("clear fail %s" % e)

VRM = "C:/Secret_Project/Content/TestCharacter/test2/test2.vrm"
imported = False
try:
    bpy.ops.import_scene.vrm(filepath=VRM); imported = True; w("VRM import OK")
except Exception as e:
    w("VRM import fail: %s" % e)
if not imported:
    try:
        bpy.ops.import_scene.gltf(filepath="C:/Secret_Project/_blender/test2.glb"); imported = True; w("glTF import OK")
    except Exception as e: w("glTF import fail: %s" % e)

w("---- OBJECTS ----")
for o in bpy.data.objects:
    w("OBJ '%s' type=%s parent=%s" % (o.name, o.type, o.parent.name if o.parent else None))
    if o.type == 'MESH':
        me = o.data
        slots = [(s.material.name if s.material else '(none)') for s in o.material_slots]
        w("   verts=%d polys=%d uv_layers=%s" % (len(me.vertices), len(me.polygons), [u.name for u in me.uv_layers]))
        w("   material_slots=%s" % slots)
        # 노멀/오토스무스 상태
        w("   has_custom_normals=%s auto_smooth(attr?)=%s" % (getattr(me, 'has_custom_normals', '?'), hasattr(me, 'use_auto_smooth')))
    if o.type == 'ARMATURE':
        w("   bones=%d (예: %s)" % (len(o.data.bones), [b.name for b in o.data.bones[:6]]))
flush()
w("DONE")
flush()
