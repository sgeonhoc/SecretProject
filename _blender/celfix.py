# Blender headless: test2.vrm 임포트 → 얼굴(Face) 노멀 구면화 → FBX 내보내기.
#   얼굴 노멀을 머리 안쪽 중심에서 방사형으로 덮어써 코/눈두덩 그림자 캐치 제거(VRoid 셀 티 1번 제거).
import bpy, mathutils
LOG = "C:/Secret_Project/_blender/celfix_log.txt"; out = []
def w(s): out.append(str(s)); print("CELFIX> " + str(s))
def flush():
    try:
        with open(LOG, "w", encoding="utf-8") as f: f.write("\n".join(out))
    except Exception as e: print("flush", e)

# VRM 애드온 보장 + 임포트
if not hasattr(bpy.ops.import_scene, 'vrm'):
    for mod in ('bl_ext.blender_org.vrm', 'vrm', 'io_scene_vrm'):
        try: bpy.ops.preferences.addon_enable(module=mod)
        except Exception: pass
try:
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()
except Exception: pass
bpy.ops.import_scene.vrm(filepath="C:/Secret_Project/Content/TestCharacter/test2/test2.vrm")
w("Blender %s / imported" % bpy.app.version_string)

face = bpy.data.objects.get('Face')
arm = bpy.data.objects.get('Armature')
if not face:
    w("!! Face 오브젝트 없음"); flush(); raise SystemExit
me = face.data

# 중심: 얼굴 centroid를 머리뼈(J_Bip_C_Head) 쪽으로 0.6 당겨 두개골 안쪽에
centroid = mathutils.Vector((0, 0, 0))
for v in me.vertices: centroid += v.co
centroid /= len(me.vertices)
center = centroid.copy()
hb = arm.data.bones.get('J_Bip_C_Head') if arm else None
if hb:
    head_local = face.matrix_world.inverted() @ (arm.matrix_world @ hb.head_local)
    center = centroid + (head_local - centroid) * 0.6
w("centroid=%s center=%s verts=%d loops=%d" % (centroid, center, len(me.vertices), len(me.loops)))

# 루프별 방사형 노멀
normals = []
for loop in me.loops:
    co = me.vertices[loop.vertex_index].co
    d = co - center
    if d.length < 1e-6:
        d = me.vertices[loop.vertex_index].normal.copy()
    d.normalize()
    normals.append((d.x, d.y, d.z))
try:
    me.normals_split_custom_set(normals)
    me.update()
    w("얼굴 노멀 구면화 적용 OK (%d loops)" % len(normals))
except Exception as e:
    w("!! normals_split_custom_set 실패: %s" % e)
flush()

# FBX 내보내기 (Armature + 모든 MESH)
bpy.ops.object.select_all(action='DESELECT')
for o in bpy.data.objects:
    if o.type in ('MESH', 'ARMATURE'):
        try: o.select_set(True)
        except Exception: pass
if arm:
    bpy.context.view_layer.objects.active = arm
OUT = "C:/Secret_Project/_blender/test2_celfix.fbx"
try:
    bpy.ops.export_scene.fbx(filepath=OUT, use_selection=True,
        object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False,
        bake_anim=False, mesh_smooth_type='OFF', use_mesh_modifiers=True)
    w("FBX 내보내기 OK: %s" % OUT)
except Exception as e:
    w("!! FBX export 실패: %s" % e)
flush()
w("DONE"); flush()
