# test7 버전들(원본/v1/v3/v4/v5)을 각각 다른 텍스처로 입혀 Map_Main에 한 줄로 배치.
# 모든 액터가 같은 SK를 쓰므로, 버전별로 [텍스처 에셋 + MIC(베이스MI 상속, 베이스색 파라미터만 교체)]를 만들어 슬롯별 오버라이드.
import unreal, os
AT  = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
LOG=[]
def w(s):
    LOG.append(str(s))
    try: unreal.log("VAR> %s"%s)
    except Exception: pass
    try: open("C:/Secret_Project/기획/99_보관/구로그/_variants_log.txt","w",encoding="utf-8").write("\n".join(LOG))
    except Exception: pass

MESH_PATH='/Game/TestCharacter/test7/SK_test7'
VERS=[("orig","C:/Secret_Project/_tripo/our_chars/test7_tex"),
      ("v1","C:/Secret_Project/_tripo/our_chars/test7_tex_styled"),
      ("v3","C:/Secret_Project/_tripo/our_chars/test7_tex_styled3"),
      ("v4","C:/Secret_Project/_tripo/our_chars/test7_tex_styled4"),
      ("v5","C:/Secret_Project/_tripo/our_chars/test7_tex_styled5")]
SLOTTEX={3:"T__04",1:"T__02",7:"T__10",8:"T__12",9:"T__13",13:"T__17"}
BASECOLOR=["gltf_tex_diffuse","mtoon_tex_MainTex","mtoon_tex_Shade","mtoon_tex_ShadeTexture"]
VARROOT="/Game/TestCharacter/_variants"

mesh=unreal.load_asset(MESH_PATH)
mats=list(mesh.get_editor_property('materials'))
base_mi={slot:mats[slot].get_editor_property('material_interface') for slot in SLOTTEX}

# 1) 버전별 텍스처 임포트
for ver,folder in VERS:
    texdir="%s/%s_tex"%(VARROOT,ver); tasks=[]
    for slot,tn in SLOTTEX.items():
        f=folder+"/"+tn+".png"
        if not os.path.exists(f): w("MISS %s"%f); continue
        t=unreal.AssetImportTask()
        t.set_editor_property("filename",f); t.set_editor_property("destination_path",texdir)
        t.set_editor_property("destination_name","%s_%s"%(tn,ver))
        t.set_editor_property("replace_existing",True); t.set_editor_property("automated",True); t.set_editor_property("save",True)
        tasks.append(t)
    AT.import_asset_tasks(tasks); w("imported tex for %s"%ver)

# 2) 버전별 MIC 생성(베이스 MI 상속 + 베이스색 파라미터만 교체)
ver_mics={}
for ver,_ in VERS:
    micdir="%s/%s_mat"%(VARROOT,ver); d={}
    for slot,tn in SLOTTEX.items():
        micpath="%s/MIC_%s_%s"%(micdir,tn,ver)
        if EAL.does_asset_exist(micpath):
            mic=unreal.load_asset(micpath)
        else:
            mic=AT.create_asset("MIC_%s_%s"%(tn,ver),micdir,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        mic.set_editor_property("parent",base_mi[slot])
        tex=unreal.load_asset("%s/%s_tex/%s_%s"%(VARROOT,ver,tn,ver))
        if tex:
            for p in BASECOLOR:
                try: MEL.set_material_instance_texture_parameter_value(mic,p,tex)
                except Exception as e: w("param %s/%s/%s err %s"%(ver,tn,p,e))
        EAL.save_asset(micpath,only_if_is_dirty=False); d[slot]=mic
    ver_mics[ver]=d; w("MICs for %s"%ver)

# 3) Map_Main 로드 + 기존 스켈레탈 숨김 + 한 줄 배치
les.load_level('/Game/CiciToonCharacterShaderPak/Levels/Map_Main')
for a in eas.get_all_level_actors():
    if a.get_components_by_class(unreal.SkeletalMeshComponent):
        try: a.set_is_temporarily_hidden_in_editor(True)
        except Exception: pass
SP=62.0; n=len(VERS); x0=-(n-1)*SP/2.0
spawned=[]
for i,(ver,_) in enumerate(VERS):
    loc=unreal.Vector(x0+i*SP, 0.0, 0.0)
    act=eas.spawn_actor_from_class(unreal.SkeletalMeshActor,loc,unreal.Rotator(0,0,0))
    try: act.set_actor_label("test7_%02d_%s"%(i,ver))
    except Exception: pass
    comp=act.skeletal_mesh_component
    try: comp.set_skeletal_mesh_asset(mesh)
    except Exception:
        try: comp.set_skeletal_mesh(mesh)
        except Exception as e: w("setmesh %s err %s"%(ver,e))
    for slot,mic in ver_mics[ver].items():
        try: comp.set_material(slot,mic)
        except Exception as e: w("setmat %s slot%d err %s"%(ver,slot,e))
    spawned.append(act); w("spawned %s @x=%.0f"%(ver,loc.x))

# 4) 카메라: 줄 전체 정면에서 프레이밍
world=ues.get_editor_world()
for c in ["r.Streaming.FullyLoadUsedTextures 1","r.ScreenPercentage 100"]:
    try: unreal.SystemLibrary.execute_console_command(world,c)
    except Exception: pass
try: les.editor_set_game_view(True)
except Exception: pass
camloc=unreal.Vector(0.0, 470.0, 95.0); tgt=unreal.Vector(0.0,0.0,90.0)
rot=unreal.MathLibrary.find_look_at_rotation(camloc,tgt)
try: ues.set_level_viewport_camera_info(camloc,rot)
except Exception as e: w("cam err %s"%e)
w("ROW READY (%d)"%len(spawned))
