# Hayakawa 부위별 풀세트 캡처. 16:9 고해상 + 카메라FOV제어 + 레퍼런스포즈 + 텍스처풀로드 + 워밍업.
import unreal, os, time, shutil

OUT = r"C:/Secret_Project/_tripo/hayakawa_caps/full"
SHOT_DIR = r"C:/Secret_Project/Saved/Screenshots/WindowsEditor"
LOG = r"C:/Secret_Project/기획/99_보관/구로그/citoon_capture_log.txt"
_log=[]
def w(s):
    _log.append(str(s))
    try: open(LOG,"w",encoding="utf-8").write("\n".join(_log))
    except Exception: pass
    try: unreal.log("CAP> %s"%s)
    except Exception: pass

os.makedirs(OUT,exist_ok=True); os.makedirs(SHOT_DIR,exist_ok=True)
ues=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
les=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
try: les.load_level('/Game/CiciToonCharacterShaderPak/Levels/Map_Main'); w("loaded")
except Exception as ex: w("load fail %s"%ex)
world=ues.get_editor_world()

target=None
for a in eas.get_all_level_actors():
    has_sk=bool(a.get_components_by_class(unreal.SkeletalMeshComponent))
    if a.get_actor_label()=="SKM_Hayakawa_Main" and target is None: target=a; continue
    if has_sk:
        try: a.set_is_temporarily_hidden_in_editor(True)
        except Exception: pass
comp=target.get_components_by_class(unreal.SkeletalMeshComponent)[0]
try:
    comp.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    comp.set_anim_instance_class(None)
    ad=comp.get_editor_property('animation_data')
    try: ad.set_editor_property('anim_to_play', None)
    except Exception: pass
    comp.set_editor_property('animation_data', ad); w("refpose")
except Exception as ex: w("refpose fail %s"%ex)
for c in ["r.Streaming.FullyLoadUsedTextures 1","r.Streaming.PoolSize 4000","r.ScreenPercentage 100","r.Tonemapper.Sharpen 0.6"]:
    try: unreal.SystemLibrary.execute_console_command(world,c)
    except Exception: pass
try: les.editor_set_game_view(True)
except Exception: pass
cam=eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(0,0,150), unreal.Rotator(0,0,0))
camc=cam.camera_component
try: les.pilot_level_actor(cam); w("piloted")
except Exception as ex: w("pilot fail %s"%ex)

RES_W,RES_H = 1600,900   # 16:9 (세로화각 기준 프레이밍)

# (이름, yaw, look_dx, dist, z_cam, look_z, fov_h)
S=[
 # 전신 턴테이블 (정면=+Y, fov55 d300 level)
 ("body_front",0,0,300,85,85,55),("body_fl45",45,0,300,85,85,55),
 ("body_left90",90,0,300,85,85,55),("body_bl135",135,0,300,85,85,55),
 ("body_back180",180,0,300,85,85,55),("body_br225",225,0,300,85,85,55),
 ("body_right270",270,0,300,85,85,55),("body_fr315",315,0,300,85,85,55),
 # 얼굴 각도
 ("face_front",0,0,134,152,150,24),("face_34L",330,0,134,152,150,24),
 ("face_34R",30,0,134,152,150,24),("face_left",90,0,134,152,150,24),
 ("face_right",270,0,134,152,150,24),
 # 이목구비 클로즈업
 ("eyes",0,0,60,155,155,18),("eyebrow",0,0,55,158,157,14),
 ("nose",0,0,52,148,148,14),("mouth",0,0,52,143,143,15),
 ("ear",90,0,70,152,152,18),
 # 머리
 ("hair_bangs",0,0,90,158,155,22),("hair_back",180,0,110,150,143,30),
 ("hair_side",90,0,110,150,143,30),
 # 상체/의상
 ("collar_bow",0,0,90,132,131,22),("chest_vest",0,0,130,120,116,30),
 ("upper_torso",0,0,165,118,108,38),("shoulder",40,0,120,135,130,26),
 # 팔/손 (T포즈 팔=월드X축, 손은 좌우 끝)
 ("hand_R",0,68,70,135,135,20),("hand_L",0,-68,70,135,135,20),
 ("arm_R",0,40,130,135,133,30),
 # 하체
 ("waist_hem",0,0,110,88,86,24),("skirt_front",0,0,125,58,55,30),
 ("skirt_pleats",0,0,80,55,52,22),
 # 다리/발
 ("legs",0,0,150,28,28,34),("knees",0,0,90,40,40,24),
 ("socks_feet",0,0,85,14,16,22),("shoes",0,0,65,7,9,20),
 # 뒤
 ("back_full",180,0,300,85,85,55),("backpack",180,0,110,120,113,30),
]
w("planned=%d"%len(S))

def setcam(yaw,dx,d,zc,lz,fov):
    target.set_actor_rotation(unreal.Rotator(roll=0.0,pitch=0.0,yaw=float(yaw)),False)
    o,e=target.get_actor_bounds(False); fx=o.x+dx; cy=o.y
    cl=unreal.Vector(fx,cy+d,zc); lk=unreal.Vector(fx,cy,lz)
    rot=unreal.MathLibrary.find_look_at_rotation(cl,lk)
    cam.set_actor_location_and_rotation(cl,rot,False,False)
    try: camc.set_field_of_view(float(fov))
    except Exception: pass
    try: ues.set_level_viewport_camera_info(cl,rot)
    except Exception: pass

ST={"i":0,"phase":"warmup","warm":0,"c":0,"await":0,"pre":set(),"wd":0,"h":None}
def pngs():
    try: return set(f for f in os.listdir(SHOT_DIR) if f.lower().endswith(".png"))
    except Exception: return set()
def tick(dt):
    ST["wd"]+=1
    if ST["wd"]>40000: w("WD"); fin(); return
    if ST["phase"]=="warmup":
        ST["warm"]+=1
        if ST["warm"]==1: setcam(*S[0][1:])
        if ST["warm"]>=420: w("warmup done"); ST["phase"]="set"
        return
    i=ST["i"]
    if i>=len(S): fin(); return
    name,yaw,dx,d,zc,lz,fov=S[i]; ph=ST["phase"]
    if ph=="set":
        try: setcam(yaw,dx,d,zc,lz,fov)
        except Exception as ex: w("setcam %s fail %s"%(name,ex))
        ST["c"]=0; ST["phase"]="settle"
    elif ph=="settle":
        ST["c"]+=1
        if ST["c"]>=14:
            ST["pre"]=pngs()
            try: unreal.AutomationLibrary.take_high_res_screenshot(RES_W,RES_H,name+".png",cam)
            except Exception as ex: w("shot %s fail %s"%(name,ex))
            ST["await"]=0; ST["phase"]="await"
    elif ph=="await":
        ST["await"]+=1
        new=pngs()-ST["pre"]
        if new and ST["await"]>=3:
            fn=sorted(new)[-1]; src=os.path.join(SHOT_DIR,fn); dst=os.path.join(OUT,name+".png")
            try:
                s1=os.path.getsize(src); time.sleep(0.05); s2=os.path.getsize(src)
                if s1==s2 and s1>0:
                    shutil.move(src,dst); w("SHOT %02d %s (%d b)"%(i,name,s2))
                    ST["i"]+=1; ST["phase"]="set"
            except Exception as ex: w("move %s fail %s"%(name,ex))
        elif ST["await"]>600:
            w("SHOT %02d %s TIMEOUT"%(i,name)); ST["i"]+=1; ST["phase"]="set"
def fin():
    try:
        if ST["h"]: unreal.unregister_slate_post_tick_callback(ST["h"])
    except Exception: pass
    try: les.eject_pilot_level_actor()
    except Exception: pass
    w("DONE")
    try: unreal.SystemLibrary.execute_console_command(world,"QUIT_EDITOR")
    except Exception: pass
ST["h"]=unreal.register_slate_post_tick_callback(tick)
w("FULL capturing...")
