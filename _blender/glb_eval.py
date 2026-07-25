# Tripo 결과 GLB를 임포트해 4방향 렌더(얼굴 포함 평가용) → C:/Secret_Project/_blender/tripo_eval/
import bpy, os, mathutils
GLB = "C:/UE_Import_Zone/face_test_portrait.glb"
OUT = "C:/Secret_Project/_blender/tripo_eval2"
os.makedirs(OUT, exist_ok=True)
LOG = []
def w(s): LOG.append(str(s)); print("EVAL> " + str(s))
def flush():
    try: open(OUT + "/eval_log.txt", "w", encoding="utf-8").write("\n".join(LOG))
    except Exception: pass

try:
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()
except Exception: pass
try:
    bpy.ops.import_scene.gltf(filepath=GLB); w("GLB imported")
except Exception as e:
    w("import fail %s" % e); flush(); raise SystemExit

meshes = [o for o in bpy.data.objects if o.type == 'MESH']
mn = mathutils.Vector((1e9, 1e9, 1e9)); mx = mathutils.Vector((-1e9, -1e9, -1e9))
for o in meshes:
    for c in o.bound_box:
        wc = o.matrix_world @ mathutils.Vector(c)
        mn = mathutils.Vector((min(mn.x, wc.x), min(mn.y, wc.y), min(mn.z, wc.z)))
        mx = mathutils.Vector((max(mx.x, wc.x), max(mx.y, wc.y), max(mx.z, wc.z)))
center = (mn + mx) * 0.5; dims = mx - mn
w("center=%s dims=%s" % (center, dims))

world = bpy.data.worlds[0] if bpy.data.worlds else bpy.data.worlds.new("W")
bpy.context.scene.world = world; world.use_nodes = True
try:
    world.node_tree.nodes['Background'].inputs[0].default_value = (0.85, 0.85, 0.88, 1)
    world.node_tree.nodes['Background'].inputs[1].default_value = 1.1
except Exception: pass

sc = bpy.context.scene
for eng in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
    try: sc.render.engine = eng; w("engine=%s" % eng); break
    except Exception: pass
sc.render.resolution_x = 800; sc.render.resolution_y = 1120
sc.render.film_transparent = False
try: sc.view_settings.view_transform = 'Standard'
except Exception: pass

cd = bpy.data.cameras.new("C"); cd.type = 'ORTHO'
cd.ortho_scale = max(dims.z, dims.x * (1120 / 800.0)) * 1.2
cam = bpy.data.objects.new("C", cd); bpy.context.collection.objects.link(cam); sc.camera = cam
radius = max(dims.x, dims.y, dims.z) * 1.5 + 1.0
look = center.copy()
views = {"glb_+Y": (center.x, center.y + radius, center.z),
         "glb_-Y": (center.x, center.y - radius, center.z),
         "glb_+X": (center.x + radius, center.y, center.z),
         "glb_-X": (center.x - radius, center.y, center.z)}
for name, loc in views.items():
    cam.location = mathutils.Vector(loc)
    d = look - cam.location; cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = OUT + "/" + name + ".png"
    try: bpy.ops.render.render(write_still=True); w("rendered %s" % name)
    except Exception as e: w("render %s fail %s" % (name, e))
flush()
