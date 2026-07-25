# Hayakawa 캡처 v4 검증: take_high_res_screenshot(정확한 세로해상도+카메라FOV) + 파일럿 + 풀로드 + 워밍업.
import unreal, os, math, time, shutil

OUT = r"C:/Secret_Project/_tripo/hayakawa_caps/pass4_test"
SHOT_DIR = r"C:/Secret_Project/Saved/Screenshots/WindowsEditor"
LOG = r"C:/Secret_Project/기획/99_보관/구로그/citoon_capture_log.txt"
_log = []
def w(s):
    _log.append(str(s))
    try: open(LOG,"w",encoding="utf-8").write("\n".join(_log))
    except Exception: pass
    try: unreal.log("CAP> %s" % s)
    except Exception: pass

os.makedirs(OUT, exist_ok=True); os.makedirs(SHOT_DIR, exist_ok=True)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
try: les.load_level('/Game/CiciToonCharacterShaderPak/Levels/Map_Main'); w("loaded")
except Exception as ex: w("load fail %s" % ex)
world = ues.get_editor_world()

target=None; hid=0
for a in eas.get_all_level_actors():
    has_sk = bool(a.get_components_by_class(unreal.SkeletalMeshComponent))
    if a.get_actor_label()=="SKM_Hayakawa_Main" and target is None: target=a; continue
    if has_sk:
        try: a.set_is_temporarily_hidden_in_editor(True); hid+=1
        except Exception: pass
comp = target.get_components_by_class(unreal.SkeletalMeshComponent)[0]
# 레퍼런스 포즈(T) — animdata 비우면 바인드포즈
try:
    comp.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    comp.set_anim_instance_class(None)
    ad = comp.get_editor_property('animation_data')
    try: ad.set_editor_property('anim_to_play', None)
    except Exception: pass
    comp.set_editor_property('animation_data', ad)
    w("refpose set")
except Exception as ex: w("refpose fail %s" % ex)

for c in ["r.Streaming.FullyLoadUsedTextures 1","r.Streaming.PoolSize 3000","r.ScreenPercentage 100","r.Tonemapper.Sharpen 0.5"]:
    try: unreal.SystemLibrary.execute_console_command(world, c)
    except Exception: pass
try: les.editor_set_game_view(True)
except Exception: pass

cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(0,0,150), unreal.Rotator(0,0,0))
camc = cam.camera_component
try: les.pilot_level_actor(cam); w("piloted")
except Exception as ex: w("pilot fail %s" % ex)

RES_W, RES_H = 1000, 1400   # 세로 비율 고정
ASPECT = RES_W/float(RES_H)

# (이름, yaw, dist, z_cam, look_z, fov_h)
SHOTS = [
    ("body_front", 0, 290, 92, 88, 28),
    ("face_front", 0, 90, 152, 150, 14),
    ("eyes",       0, 48, 156, 156, 12),
    ("hand_R",     0, 110, 95, 95, 16),
]

def setcam(yaw,d,zc,lz,fov):
    target.set_actor_rotation(unreal.Rotator(roll=0.0,pitch=0.0,yaw=float(yaw)), False)
    o,e = target.get_actor_bounds(False); cx,cy=o.x,o.y
    cl=unreal.Vector(cx,cy+d,zc); lk=unreal.Vector(cx,cy,lz)
    rot=unreal.MathLibrary.find_look_at_rotation(cl,lk)
    cam.set_actor_location_and_rotation(cl,rot,False,False)
    try: camc.set_field_of_view(float(fov))
    except Exception: pass
    try: ues.set_level_viewport_camera_info(cl,rot)
    except Exception: pass

ST={"i":0,"phase":"warmup","warm":0,"settle":0,"await":0,"pre":set(),"wd":0,"h":None}
def pngs():
    try: return set(f for f in os.listdir(SHOT_DIR) if f.lower().endswith(".png"))
    except Exception: return set()
def tick(dt):
    ST["wd"]+=1
    if ST["wd"]>30000: w("WD"); fin(); return
    if ST["phase"]=="warmup":
        ST["warm"]+=1
        if ST["warm"]==1: setcam(*SHOTS[0][1:])
        if ST["warm"]>=480: w("warmup done"); ST["phase"]="set"
        return
    i=ST["i"]
    if i>=len(SHOTS): fin(); return
    name,yaw,d,zc,lz,fov=SHOTS[i]; ph=ST["phase"]
    if ph=="set":
        try: setcam(yaw,d,zc,lz,fov)
        except Exception as ex: w("setcam %s fail %s"%(name,ex))
        ST["settle"]=0; ST["phase"]="settle"
    elif ph=="settle":
        ST["settle"]+=1
        if ST["settle"]>=24:
            ST["pre"]=pngs()
            try:
                ok = unreal.AutomationLibrary.take_high_res_screenshot(RES_W, RES_H, name+".png", cam)
                w("  shot req %s ok=%s"%(name, ok))
            except Exception as ex: w("shot fail %s: %s"%(name,ex))
            ST["await"]=0; ST["phase"]="await"
    elif ph=="await":
        ST["await"]+=1
        new=pngs()-ST["pre"]
        if new and ST["await"]>=2:
            fn=sorted(new)[-1]; src=os.path.join(SHOT_DIR,fn); dst=os.path.join(OUT,name+".png")
            try:
                s1=os.path.getsize(src); time.sleep(0.05); s2=os.path.getsize(src)
                if s1==s2 and s1>0:
                    shutil.move(src,dst); w("SHOT %02d %s (%d b)"%(i,name,s2))
                    ST["i"]+=1; ST["phase"]="set"
            except Exception as ex: w("move fail %s %s"%(name,ex))
        elif ST["await"]>500:
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
w("v4 test (take_high_res_screenshot)...")
