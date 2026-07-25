# celfix FBX(얼굴 노멀 구면화본)를 UE에 임포트(노멀 보존) + 셀튜닝 test2 머티리얼 적용 + Showcase에 배치(비교용).
import unreal
LOG = "C:/Secret_Project/기획/99_보관/구로그/citoon_celfix_log.txt"; log = []
def w(s):
    s = str(s); log.append(s)
def flush():
    try: open(LOG, "w", encoding="utf-8").write("\n".join(log))
    except Exception: pass

ATH = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
DEST = "/Game/TestCharacter/CelFix"
FBX = "C:/Secret_Project/_blender/test2_celfix.fbx"
ORIG = "/Game/TestCharacter/test2/"

# 1) FBX 임포트 (스켈레탈, ★노멀 보존)
opts = unreal.FbxImportUI()
opts.set_editor_property('import_mesh', True)
opts.set_editor_property('import_as_skeletal', True)
opts.set_editor_property('import_materials', False)
opts.set_editor_property('import_textures', False)
opts.set_editor_property('import_animations', False)
opts.set_editor_property('create_physics_asset', False)
opts.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_SKELETAL_MESH)
try:
    smid = opts.get_editor_property('skeletal_mesh_import_data')
    smid.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    smid.set_editor_property('normal_generation_method', unreal.FBXNormalGenerationMethod.MIKK_T_SPACE)
    opts.set_editor_property('skeletal_mesh_import_data', smid)
    w("normal_import_method=IMPORT_NORMALS 설정")
except Exception as e:
    w("normal import 옵션 설정 실패: %s" % e)

task = unreal.AssetImportTask()
task.set_editor_property('filename', FBX)
task.set_editor_property('destination_path', DEST)
task.set_editor_property('automated', True)
task.set_editor_property('save', True)
task.set_editor_property('replace_existing', True)
task.set_editor_property('options', opts)
try:
    ATH.import_asset_tasks([task])
    w("import_asset_tasks 호출됨. imported=%s" % list(task.get_editor_property('imported_object_paths')))
except Exception as e:
    w("임포트 실패: %s" % e)

# 2) 임포트된 SkeletalMesh 찾기
sk = None
for p in EAL.list_assets(DEST, True, False):
    a = unreal.load_asset(p.split('.')[0])
    if isinstance(a, unreal.SkeletalMesh):
        sk = a; w("SkeletalMesh = %s" % p); break
if not sk:
    w("!! 임포트된 SkeletalMesh 못 찾음"); flush()
else:
    # 3) 셀튜닝 test2 머티리얼을 슬롯명 매칭으로 적용
    mats = list(sk.get_editor_property('materials'))
    newmats = []
    for sm in mats:
        slot = str(sm.get_editor_property('material_slot_name'))
        clean = slot.replace('(Instance)', '').strip()
        op = ORIG + 'MI_' + clean + '__Instance_'
        mi = unreal.load_asset(op) if EAL.does_asset_exist(op) else None
        ns = unreal.SkeletalMaterial()
        ns.set_editor_property('material_interface', mi if mi else sm.get_editor_property('material_interface'))
        ns.set_editor_property('material_slot_name', sm.get_editor_property('material_slot_name'))
        newmats.append(ns)
        w("  slot %s -> %s" % (slot, (op.split('/')[-1] if mi else 'KEEP')))
    sk.set_editor_property('materials', newmats)
    EAL.save_asset(sk.get_path_name().split('.')[0])

    # 4) Showcase에 배치 (CLEAN_test2_COMPARE 옆)
    try:
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/CityPark/Maps/Showcase')
        eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        base = None
        for a in eas.get_all_level_actors():
            if a.get_actor_label() == 'SK_VRM':
                base = a.get_actor_location(); break
        if base is None: base = unreal.Vector(3560, 5080, -450)
        loc = unreal.Vector(base.x, base.y + 320.0, base.z)
        actor = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, loc, unreal.Rotator(0, 0, 0))
        comp = actor.get_components_by_class(unreal.SkeletalMeshComponent)[0]
        try: comp.set_skeletal_mesh_asset(sk)
        except Exception:
            try: comp.set_editor_property('skeletal_mesh_asset', sk)
            except Exception: comp.set_editor_property('skeletal_mesh', sk)
        comp.set_editor_property('render_custom_depth', True)
        comp.set_editor_property('custom_depth_stencil_value', 1)
        try: actor.set_actor_label('CELFIX_test2_FACE')
        except Exception: pass
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
        w("배치 완료 CELFIX_test2_FACE @ %s, 레벨 저장" % loc)
    except Exception as e:
        w("배치 실패: %s" % e)
    flush()

try:
    world = None
    try: world = unreal.EditorLevelLibrary.get_editor_world()
    except Exception: pass
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
except Exception: pass
