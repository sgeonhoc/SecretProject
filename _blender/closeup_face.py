# Tripo GLB 얼굴 클로즈업 렌더(정면=+X) → 얼굴 퀄 판정용
import bpy, os, mathutils
GLB = "C:/UE_Import_Zone/high_v2_character.glb"
OUT = "C:/Secret_Project/_blender/tripo_eval"
os.makedirs(OUT, exist_ok=True)
try:
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()
except Exception: pass
bpy.ops.import_scene.gltf(filepath=GLB)
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
mn = mathutils.Vector((1e9, 1e9, 1e9)); mx = mathutils.Vector((-1e9, -1e9, -1e9))
for o in meshes:
    for c in o.bound_box:
        wc = o.matrix_world @ mathutils.Vector(c)
        mn = mathutils.Vector((min(mn.x, wc.x), min(mn.y, wc.y), min(mn.z, wc.z)))
        mx = mathutils.Vector((max(mx.x, wc.x), max(mx.y, wc.y), max(mx.z, wc.z)))
center = (mn + mx) * 0.5; dims = mx - mn
head_z = mx.z - dims.z * 0.09
look = mathutils.Vector((center.x, center.y, head_z))

world = bpy.data.worlds[0]; world.use_nodes = True; bpy.context.scene.world = world
world.node_tree.nodes['Background'].inputs[0].default_value = (0.88, 0.88, 0.9, 1)
world.node_tree.nodes['Background'].inputs[1].default_value = 1.1
sc = bpy.context.scene
for eng in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
    try: sc.render.engine = eng; break
    except Exception: pass
sc.render.resolution_x = 800; sc.render.resolution_y = 800
try: sc.view_settings.view_transform = 'Standard'
except Exception: pass
cd = bpy.data.cameras.new("C"); cd.type = 'ORTHO'
cd.ortho_scale = dims.z * 0.26
cam = bpy.data.objects.new("C", cd); bpy.context.collection.objects.link(cam); sc.camera = cam
reach = max(dims.x, dims.y) * 2 + 1
# 정면(+X)과 정면(-Y) 둘 다 클로즈업(어느 쪽이 얼굴인지 확실히)
for nm, loc in {"face_+X": (mx.x + reach, center.y, head_z),
                "face_-Y": (center.x, mn.y - reach, head_z)}.items():
    cam.location = mathutils.Vector(loc)
    d = look - cam.location; cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = OUT + "/" + nm + ".png"
    bpy.ops.render.render(write_still=True)
