# Tripo GLB를 새 빈 레벨 + 기본 조명에 배치해서 깨끗하게 본다. (Map_Main 안 씀)
import unreal
AT=unreal.AssetToolsHelpers.get_asset_tools()
EAL=unreal.EditorAssetLibrary
les=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
def w(s):
    try: unreal.log("TGLB> %s"%s)
    except Exception: pass
def mov(a):
    try: a.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    except Exception: pass

GLB="C:/UE_Import_Zone/hayakawa_tripo_test.glb"; DEST="/Game/TripoTest"
t=unreal.AssetImportTask()
t.set_editor_property("filename",GLB); t.set_editor_property("destination_path",DEST)
t.set_editor_property("automated",True); t.set_editor_property("replace_existing",True); t.set_editor_property("save",True)
AT.import_asset_tasks([t])
imported=list(t.get_editor_property("imported_object_paths") or [])
mesh=None; mat=None
for p in list(imported)+list(EAL.list_assets(DEST,recursive=True)):
    try: a=unreal.load_asset(p)
    except Exception: a=None
    if mesh is None and isinstance(a,(unreal.StaticMesh,unreal.SkeletalMesh)): mesh=a; w("mesh=%s"%p)
    if mat is None and isinstance(a,unreal.MaterialInterface) and "tripo" in p.lower(): mat=a; w("mat=%s"%p)
if mat is None:  # 폴백: 아무 머티리얼이나
    for p in list(EAL.list_assets(DEST,recursive=True)):
        a=unreal.load_asset(p)
        if isinstance(a,unreal.MaterialInterface): mat=a; w("mat(fallback)=%s"%p); break

# === 새 빈 레벨 ===
try: les.new_level("/Game/TripoTest/TripoView")
except Exception as e: w("new_level err %s"%e)

# === 기본 조명(새 레벨 디폴트와 동일 구성) ===
dl=eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,400)); mov(dl)
dl.set_actor_rotation(unreal.Rotator(-46.0,-35.0,0.0),False)
try: dl.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(6.0)
except Exception: pass
eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0,0,0))
sky=eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,250)); mov(sky)
try:
    sc=sky.get_component_by_class(unreal.SkyLightComponent)
    sc.set_editor_property('real_time_capture',True); sc.set_intensity(1.0); sc.recapture_sky()
except Exception as e: w("sky err %s"%e)
try: eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0,0,0))
except Exception: pass

# === 메시 배치(원점, 바닥에) ===
act=None
if isinstance(mesh,unreal.StaticMesh):
    act=eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0,0,0))
    try: act.static_mesh_component.set_static_mesh(mesh)
    except Exception as e: w("setmesh err %s"%e)
elif isinstance(mesh,unreal.SkeletalMesh):
    act=eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0,0,0))
    try: act.skeletal_mesh_component.set_skeletal_mesh_asset(mesh)
    except Exception: pass
if act:
    act.set_actor_label("TRIPO_TEST")
    # 임포트된 tripo 머티리얼 명시적으로 입히기(메시 슬롯이 비어 회색으로 뜨는 것 방지)
    if mat:
        try:
            comp=act.static_mesh_component if isinstance(mesh,unreal.StaticMesh) else act.skeletal_mesh_component
            n=comp.get_num_materials() or 1
            for i in range(n): comp.set_material(i, mat)
            w("material applied to %d slot(s)"%n)
        except Exception as ex: w("apply mat err %s"%ex)
    o,e=act.get_actor_bounds(False); h=e.z*2.0
    if h<20.0 and h>0: act.set_actor_scale3d(unreal.Vector(170.0/h,170.0/h,170.0/h))
    o,e=act.get_actor_bounds(False); ctr=o; rad=max(e.x,e.y,e.z)
    camloc=unreal.Vector(ctr.x, ctr.y+rad*3.0+120.0, ctr.z)
    rot=unreal.MathLibrary.find_look_at_rotation(camloc,unreal.Vector(ctr.x,ctr.y,ctr.z))
    try:
        les.editor_set_game_view(True)
        ues.set_level_viewport_camera_info(camloc,rot)
    except Exception as ex: w("cam err %s"%ex)
w("TRIPO VIEW READY")
