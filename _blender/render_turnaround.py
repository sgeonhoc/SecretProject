# Blender headless: hayakawa.fbx 임포트 → 슬롯별 알베도 평면셰이딩 → 단색배경 정사영 4방향 렌더.
#   결과: C:/Secret_Project/_blender/refs/view_{front,back,left,right 후보}.png  (내가 읽어서 front 식별)
import bpy, os, json, math, mathutils
HDIR = "C:/Secret_Project/_blender/hayakawa"
OUT  = "C:/Secret_Project/_blender/refs"
os.makedirs(OUT, exist_ok=True)
LOG = []
def w(s): LOG.append(str(s)); print("RND> " + str(s))
def flush():
    try: open(OUT + "/render_log.txt", "w", encoding="utf-8").write("\n".join(LOG))
    except Exception as e: print("flush", e)

# 클린 씬
try:
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()
except Exception: pass

# FBX 임포트
try:
    bpy.ops.import_scene.fbx(filepath=HDIR + "/hayakawa.fbx")
    w("FBX imported")
except Exception as e:
    w("FBX import fail: %s" % e); flush(); raise SystemExit

# 매핑 로드
mapping = []
try: mapping = json.load(open(HDIR + "/mapping.json", encoding="utf-8"))
except Exception as e: w("mapping load fail: %s" % e)
slot2tex = {}
for m in mapping:
    if m.get("tex"): slot2tex[m["slot"]] = HDIR + "/" + m["tex"]
w("slot2tex: %d" % len(slot2tex))

HIDE = ("outline", "shadow", "lens", "glass")  # 외곽선/그림자/안경렌즈 패스 숨김

def build_into(mat, texname, sname):
    mat.use_nodes = True
    nt = mat.node_tree; nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    transp = nt.nodes.new('ShaderNodeBsdfTransparent')
    low = (sname or "").lower()
    if (not texname) or any(h in low for h in HIDE):
        nt.links.new(transp.outputs['BSDF'], out.inputs['Surface']); return "hidden/none"
    fp = HDIR + "/" + texname
    if not os.path.exists(fp):
        nt.links.new(transp.outputs['BSDF'], out.inputs['Surface']); return "missing"
    img = bpy.data.images.load(fp)
    tex = nt.nodes.new('ShaderNodeTexImage'); tex.image = img
    emis = nt.nodes.new('ShaderNodeEmission')
    mix = nt.nodes.new('ShaderNodeMixShader')
    nt.links.new(tex.outputs['Color'], emis.inputs['Color'])
    nt.links.new(tex.outputs['Alpha'], mix.inputs['Fac'])
    nt.links.new(transp.outputs['BSDF'], mix.inputs[1])
    nt.links.new(emis.outputs['Emission'], mix.inputs[2])
    nt.links.new(mix.outputs['Shader'], out.inputs['Surface'])
    for prop, val in (('blend_method', 'BLEND'), ('surface_render_method', 'BLENDED'), ('show_transparent_back', False)):
        try: setattr(mat, prop, val)
        except Exception: pass
    return os.path.basename(fp)

# ★슬롯 인덱스 순서로 텍스처 매핑(FBX가 UE 슬롯 순서 보존). 슬롯마다 새 머티리얼 생성.
meshobjs = [o for o in bpy.data.objects if o.type == 'MESH']
mesh_obj = max(meshobjs, key=lambda o: len(o.material_slots)) if meshobjs else None
w("main mesh = %s slots=%d" % (mesh_obj.name if mesh_obj else None, len(mesh_obj.material_slots) if mesh_obj else 0))
if mesh_obj:
    for i, mslot in enumerate(mesh_obj.material_slots):
        entry = mapping[i] if i < len(mapping) else {}
        nm = bpy.data.materials.new("celref_%02d" % i)
        r = build_into(nm, entry.get('tex', ''), entry.get('slot', ''))
        mslot.material = nm
        w("  slot[%d] %-22s <- %s" % (i, entry.get('slot', ''), r))

# bbox (월드)
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
mn = mathutils.Vector((1e9, 1e9, 1e9)); mx = mathutils.Vector((-1e9, -1e9, -1e9))
for o in meshes:
    for c in o.bound_box:
        wco = o.matrix_world @ mathutils.Vector(c)
        mn = mathutils.Vector((min(mn.x, wco.x), min(mn.y, wco.y), min(mn.z, wco.z)))
        mx = mathutils.Vector((max(mx.x, wco.x), max(mx.y, wco.y), max(mx.z, wco.z)))
center = (mn + mx) * 0.5
dims = mx - mn
w("center=%s dims=%s" % (center, dims))
height = max(dims.z, 0.1)
radius = max(dims.x, dims.y, dims.z) * 1.5 + 1.0

# 월드 배경 = 연한 회색
world = bpy.data.worlds[0] if bpy.data.worlds else bpy.data.worlds.new("W")
bpy.context.scene.world = world
world.use_nodes = True
try: world.node_tree.nodes['Background'].inputs[0].default_value = (0.82, 0.82, 0.85, 1)
except Exception: pass

# 렌더 설정
sc = bpy.context.scene
for eng in ('BLENDER_EEVEE_NEXT', 'BLENDER_EEVEE'):
    try: sc.render.engine = eng; w("engine=%s" % eng); break
    except Exception: pass
sc.render.resolution_x = 800; sc.render.resolution_y = 1120
sc.render.film_transparent = False
try: sc.view_settings.view_transform = 'Standard'
except Exception: pass

# 카메라(정사영)
cam_data = bpy.data.cameras.new("Cam"); cam_data.type = 'ORTHO'
cam_data.ortho_scale = max(dims.z, dims.x * (sc.render.resolution_y / float(sc.render.resolution_x))) * 1.2
cam = bpy.data.objects.new("Cam", cam_data); bpy.context.collection.objects.link(cam)
sc.camera = cam

views = {
    "view_+Y": mathutils.Vector((center.x, center.y + radius, center.z)),
    "view_-Y": mathutils.Vector((center.x, center.y - radius, center.z)),
    "view_+X": mathutils.Vector((center.x + radius, center.y, center.z)),
    "view_-X": mathutils.Vector((center.x - radius, center.y, center.z)),
}
look = mathutils.Vector((center.x, center.y, center.z))
for name, loc in views.items():
    cam.location = loc
    d = (look - loc); cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = OUT + "/" + name + ".png"
    try:
        bpy.ops.render.render(write_still=True); w("rendered %s" % name)
    except Exception as e:
        w("render %s fail %s" % (name, e))

# 얼굴-우세 정면 포트레이트(머리+어깨) — Tripo 얼굴 테스트용. 정면 = -Y.
try:
    sc.render.resolution_x = 1024; sc.render.resolution_y = 1024
    pz = mx.z - dims.z * 0.12
    cam.location = mathutils.Vector((center.x, center.y - radius, pz))
    dd = mathutils.Vector((center.x, center.y, pz)) - cam.location
    cam.rotation_euler = dd.to_track_quat('-Z', 'Y').to_euler()
    cam.data.ortho_scale = dims.z * 0.46
    sc.render.filepath = OUT + "/portrait_front.png"
    bpy.ops.render.render(write_still=True); w("rendered portrait_front")
except Exception as e:
    w("portrait fail %s" % e)
flush()
w("DONE"); flush()
