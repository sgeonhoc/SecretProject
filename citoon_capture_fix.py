# 보정 캡처: 본(뼈) 좌표로 얼굴/손 정확 앵커링. 빗나간 8장만 재촬영해 full/ 덮어씀.
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
    comp.set_editor_property('animation_data', ad)
except Exception as ex: w("refpose fail %s"%ex)
for c in ["r.Streaming.FullyLoadUsedTextures 1","r.Streaming.PoolSize 4000","r.ScreenPercentage 100","r.Tonemapper.Sharpen 0.6"]:
    try: unreal.SystemLibrary.execute_console_command(world,c)
    except Exception: pass
try: les.editor_set_game_view(True)
except Exception: pass

# --- 본 좌표 조회 (yaw=0 정면 상태에서) ---
target.set_actor_rotation(unreal.Rotator(roll=0.0,pitch=0.0,yaw=0.0),False)
def find_bone(*subs):
    try: nb=comp.get_num_bones()
    except Exception as ex: w("num_bones fail %s"%ex); return None
    for i in range(nb):
        nm=str(comp.get_bone_name(i)); low=nm.lower()
        if all(s in low for s in subs): return nm
    return None
def bloc(nm):
    try: return comp.get_socket_location(nm)
    except Exception: return None
b_head = find_bone("head") or find_bone("c_head")
b_eyeR = find_bone("eye","_r") or find_bone("eye","r_")
b_eyeL = find_bone("eye","_l") or find_bone("eye","l_")
b_handR= find_bone("hand","_r") or find_bone("hand","r_")
b_handL= find_bone("hand","_l") or find_bone("hand","l_")
w("bones head=%s eyeR=%s eyeL=%s handR=%s handL=%s"%(b_head,b_eyeR,b_eyeL,b_handR,b_handL))
o,e=target.get_actor_bounds(False); cx,cy=o.x,o.y
# 눈 높이
eyeZ=None
for b in (b_eyeR,b_eyeL,b_head):
    if b:
        loc=bloc(b)
        if loc: eyeZ=loc.z; w("eye/head bone %s z=%.1f"%(b,loc.z)); break
if eyeZ is None: eyeZ=143.0; w("eyeZ fallback 143")
hR=bloc(b_handR) if b_handR else None
hL=bloc(b_handL) if b_handL else None
w("handR=%s handL=%s"%(hR,hL))

RES_W,RES_H=1600,900
# 얼굴 클로즈업: 눈Z 기준 오프셋. (이름, look_x, look_z, dist, fov)
FACE=[
 ("eyes",    cx, eyeZ,      58, 16),
 ("eyebrow", cx, eyeZ+3.0,  55, 14),
 ("nose",    cx, eyeZ-4.0,  52, 14),
 ("mouth",   cx, eyeZ-8.5,  50, 15),
 ("ear",     None, eyeZ-1.0, 70, 18),  # 측면(yaw90)
]
SHOTS=[]
for nm,lx,lz,d,fov in FACE:
    SHOTS.append((nm, 90 if nm=="ear" else 0, lx if lx is not None else cx, lz, d, fov))
# 손/팔: 손 뼈 좌표 직접 앵커
if hR: SHOTS.append(("hand_R",0,hR.x,hR.z,42,22, hR.y))
if hL: SHOTS.append(("hand_L",0,hL.x,hL.z,42,22, hL.y))
if hR: SHOTS.append(("arm_R",0,(cx+hR.x)/2.0,hR.z+2,95,30, cy))
w("fix shots=%d"%len(SHOTS))

cam=eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(0,0,150), unreal.Rotator(0,0,0))
camc=cam.camera_component
try: les.pilot_level_actor(cam)
except Exception as ex: w("pilot fail %s"%ex)

def setcam(item):
    name=item[0]; yaw=item[1]; lx=item[2]; lz=item[3]; d=item[4]; fov=item[5]
    ly = item[6] if len(item)>6 else None
    target.set_actor_rotation(unreal.Rotator(roll=0.0,pitch=0.0,yaw=float(yaw)),False)
    o,e=target.get_actor_bounds(False)
    fy = ly if ly is not None else o.y
    cl=unreal.Vector(lx,fy+d,lz); lk=unreal.Vector(lx,fy,lz)
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
        if ST["warm"]==1 and SHOTS: setcam(SHOTS[0])
        if ST["warm"]>=360: w("warmup done"); ST["phase"]="set"
        return
    i=ST["i"]
    if i>=len(SHOTS): fin(); return
    item=SHOTS[i]; name=item[0]; ph=ST["phase"]
    if ph=="set":
        try: setcam(item)
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
                    shutil.move(src,dst); w("FIX %02d %s (%d b)"%(i,name,s2))
                    ST["i"]+=1; ST["phase"]="set"
            except Exception as ex: w("move %s fail %s"%(name,ex))
        elif ST["await"]>600:
            w("FIX %02d %s TIMEOUT"%(i,name)); ST["i"]+=1; ST["phase"]="set"
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
w("FIX capturing...")
