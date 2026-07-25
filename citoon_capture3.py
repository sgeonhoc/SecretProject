# Hayakawa 캡처 v3 검증(4장): 카메라파일럿(FOV제어) + 레퍼런스포즈 + 텍스처풀로드 + 워밍업.
import unreal, os, math, time, shutil

OUT = r"C:/Secret_Project/_tripo/hayakawa_caps/pass3_test"
SHOT_DIR = r"C:/Secret_Project/Saved/Screenshots/WindowsEditor"
LOG = r"C:/Secret_Project/기획/99_보관/구로그/citoon_capture_log.txt"
_log = []
def w(s):
    _log.append(str(s))
    try: open(LOG, "w", encoding="utf-8").write("\n".join(_log))
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

TARGET = "SKM_Hayakawa_Main"; target=None; hid=0
for a in eas.get_all_level_actors():
    has_sk = bool(a.get_components_by_class(unreal.SkeletalMeshComponent))
    if a.get_actor_label()==TARGET and target is None: target=a; continue
    if has_sk:
        try: a.set_is_temporarily_hidden_in_editor(True); hid+=1
        except Exception: pass
w("target=%s hid=%d" % (target.get_actor_label() if target else None, hid))
comp = target.get_components_by_class(unreal.SkeletalMeshComponent)[0]

# --- 레퍼런스(A) 포즈 강제: 여러 방법 시도 ---
def force_apose():
    res=[]
    try: comp.set_editor_property('force_ref_pose', True); res.append('force_ref_pose_prop=OK')
    except Exception as e: res.append('force_ref_prop_fail:%s'%e)
    try:
        comp.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        ad = comp.get_editor_property('animation_data')
        try: ad.set_editor_property('anim_to_play', None)
        except Exception:
            try: ad.anim_to_play = None
            except Exception: pass
        comp.set_editor_property('animation_data', ad); res.append('animdata_cleared')
    except Exception as e: res.append('animdata_fail:%s'%e)
    try: comp.set_anim_instance_class(None); res.append('animinst_None')
    except Exception as e: res.append('animinst_fail:%s'%e)
    try: comp.init_anim(True); res.append('init_anim')
    except Exception as e: res.append('initanim_fail:%s'%e)
    return res
w("apose: %s" % force_apose())

# --- 화질: 텍스처 풀로드 강제 ---
for c in ["r.Streaming.FullyLoadUsedTextures 1", "r.Streaming.PoolSize 3000",
          "r.MipMapLODBias 0", "r.ScreenPercentage 100"]:
    try: unreal.SystemLibrary.execute_console_command(world, c)
    except Exception: pass
try: les.editor_set_game_view(True); w("game_view ON")
except Exception as ex: w("gv fail %s" % ex)

# --- 카메라 액터 + 파일럿 ---
cam = eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(0,0,150), unreal.Rotator(0,0,0))
camc = cam.camera_component
try:
    les.pilot_level_actor(cam); w("piloted camera")
except Exception as ex:
    try: unreal.EditorLevelLibrary.pilot_level_actor(cam); w("piloted (legacy)")
    except Exception as ex2: w("pilot FAIL %s / %s" % (ex, ex2))

# 샷: (이름, yaw, dist, z_cam, look_z, fov(가로))
SHOTS = [
    ("body_front", 0, 300, 92, 88, 26),
    ("face_front", 0, 100, 152, 152, 14),
    ("eyes",       0, 80, 155, 155, 8),
    ("hands",      0, 150, 92, 92, 18),
]
RES = "HighResShot 1200x1600"

def setcam(yaw, d, zc, lz, fov):
    target.set_actor_rotation(unreal.Rotator(roll=0.0,pitch=0.0,yaw=float(yaw)), False)
    o,e = target.get_actor_bounds(False); cx,cy = o.x,o.y
    cl = unreal.Vector(cx, cy+d, zc); lk = unreal.Vector(cx, cy, lz)
    rot = unreal.MathLibrary.find_look_at_rotation(cl, lk)
    cam.set_actor_location_and_rotation(cl, rot, False, False)
    try: camc.set_field_of_view(float(fov))
    except Exception as ex: w("fov fail %s" % ex)
    # 파일럿 뷰도 갱신되도록 뷰포트 카메라도 맞춰줌
    try: ues.set_level_viewport_camera_info(cl, rot)
    except Exception: pass

ST={"i":0,"phase":"warmup","settle":0,"await":0,"pre":set(),"wd":0,"warm":0,"h":None}
def pngs():
    try: return set(f for f in os.listdir(SHOT_DIR) if f.lower().endswith(".png"))
    except Exception: return set()
def tick(dt):
    ST["wd"]+=1
    if ST["wd"]>20000: w("WD"); fin(); return
    if ST["phase"]=="warmup":
        ST["warm"]+=1
        if ST["warm"]==1:
            setcam(*SHOTS[0][1:])  # 첫 구도로 맞춰 스트리밍 유도
        if ST["warm"]>=420:        # 약 7초+ 텍스처/셰이더 안정 대기
            w("warmup done (%d ticks)"%ST["warm"]); ST["phase"]="set"
        return
    i=ST["i"]
    if i>=len(SHOTS): fin(); return
    name,yaw,d,zc,lz,fov = SHOTS[i]; ph=ST["phase"]
    if ph=="set":
        try: setcam(yaw,d,zc,lz,fov)
        except Exception as ex: w("setcam %s fail %s"%(name,ex))
        ST["settle"]=0; ST["phase"]="settle"
    elif ph=="settle":
        ST["settle"]+=1
        if ST["settle"]>=20:   # 구도 바뀐 뒤 텍스처 안정 대기 넉넉히
            ST["pre"]=pngs()
            try: unreal.SystemLibrary.execute_console_command(world, RES)
            except Exception as ex: w("shot fail %s"%ex)
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
    try: unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
    except Exception: pass
ST["h"]=unreal.register_slate_post_tick_callback(tick)
w("v3 test capturing (warmup first)...")
