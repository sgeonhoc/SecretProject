# Hayakawa 캡처 Pass2: 턴테이블(카메라 고정+캐릭터 회전) + 레퍼런스 포즈 + 부위별 정밀 프레이밍.
# 정면=+Y(회색 배경판 쪽). 풀 에디터 -ExecCmds="py ..." 로 구동.
import unreal, os, math, time, shutil

OUT = r"C:/Secret_Project/_tripo/hayakawa_caps/pass2"
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

try: les.load_level('/Game/CiciToonCharacterShaderPak/Levels/Map_Main'); w("loaded Map_Main")
except Exception as ex: w("load fail %s" % ex)
world = ues.get_editor_world()

TARGET = "SKM_Hayakawa_Main"
target = None; hid = 0
for a in eas.get_all_level_actors():
    has_sk = bool(a.get_components_by_class(unreal.SkeletalMeshComponent))
    if a.get_actor_label() == TARGET and target is None: target = a; continue
    if has_sk:
        try: a.set_is_temporarily_hidden_in_editor(True); hid += 1
        except Exception: pass
w("target=%s hidden=%d" % (target.get_actor_label() if target else None, hid))

comp = target.get_components_by_class(unreal.SkeletalMeshComponent)[0]
# 레퍼런스(바인드=A) 포즈 강제 → 손 내림
try:
    comp.set_force_ref_pose(True); w("force_ref_pose OK")
except Exception as ex:
    w("force_ref_pose fail: %s -> try clear anim" % ex)
    try:
        comp.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        comp.set_anim_instance_class(None); w("cleared anim")
    except Exception as ex2: w("anim clear fail: %s" % ex2)

try: les.editor_set_game_view(True); w("game_view ON")
except Exception as ex: w("game_view fail %s" % ex)

PIVOT = target.get_actor_location()   # 발밑 피벗 (회전 중심 XY)

# 샷: (이름, 캐릭터yaw, dist, z_cam, look_z, fov)
SHOTS = [
    ("body_front",   0,   300, 95, 90, 30),
    ("body_fl45",    45,  300, 95, 90, 30),
    ("body_left90",  90,  300, 95, 90, 30),
    ("body_bl135",   135, 300, 95, 90, 30),
    ("body_back180", 180, 300, 95, 90, 30),
    ("body_br225",   225, 300, 95, 90, 30),
    ("body_right270",270, 300, 95, 90, 30),
    ("body_fr315",   315, 300, 95, 90, 30),
    ("face_front",   0,   70, 153, 153, 20),
    ("face_34",      330, 70, 153, 153, 20),
    ("eyes",         0,   52, 156, 156, 12),
    ("mouth",        0,   48, 146, 146, 12),
    ("hair_back",    180, 78, 150, 150, 22),
    ("hair_side",    90,  80, 150, 150, 22),
    ("upper_torso",  0,   150, 120, 120, 26),
    ("arms_hands",   20,  180, 100, 100, 30),
    ("lower_skirt",  0,   150, 55, 55, 28),
    ("legs_feet",    0,   160, 18, 25, 30),
]
w("planned=%d" % len(SHOTS))
RES = "HighResShot 1200x1600"

def setcam(yaw, d, zc, lz, fov):
    # 캐릭터 회전(Rotator 인자순서 roll,pitch,yaw — yaw는 키워드로 안전하게)
    target.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(yaw)), False)
    o, e = target.get_actor_bounds(False)   # 회전 후 새 중심
    cx, cy = o.x, o.y
    cl = unreal.Vector(cx, cy + d, zc)       # 정면=+Y 고정
    lk = unreal.Vector(cx, cy, lz)
    rot = unreal.MathLibrary.find_look_at_rotation(cl, lk)
    ues.set_level_viewport_camera_info(cl, rot)
    unreal.SystemLibrary.execute_console_command(world, "FOV %.1f" % fov)

ST = {"i":0,"phase":"set","settle":0,"await":0,"pre":set(),"wd":0,"h":None}
def pngs():
    try: return set(f for f in os.listdir(SHOT_DIR) if f.lower().endswith(".png"))
    except Exception: return set()

def tick(dt):
    ST["wd"] += 1
    if ST["wd"] > 8000: w("WATCHDOG"); fin(); return
    i = ST["i"]
    if i >= len(SHOTS): fin(); return
    name, yaw, d, zc, lz, fov = SHOTS[i]
    ph = ST["phase"]
    if ph == "set":
        try: setcam(yaw, d, zc, lz, fov)
        except Exception as ex: w("setcam %s fail %s" % (name, ex))
        ST["settle"]=0; ST["phase"]="settle"
    elif ph == "settle":
        ST["settle"] += 1
        if ST["settle"] >= 6:
            ST["pre"]=pngs()
            try: unreal.SystemLibrary.execute_console_command(world, RES)
            except Exception as ex: w("shot fail %s" % ex)
            ST["await"]=0; ST["phase"]="await"
    elif ph == "await":
        ST["await"] += 1
        new = pngs() - ST["pre"]
        if new and ST["await"] >= 2:
            fn = sorted(new)[-1]; src = os.path.join(SHOT_DIR, fn); dst = os.path.join(OUT, name + ".png")
            try:
                s1=os.path.getsize(src); time.sleep(0.05); s2=os.path.getsize(src)
                if s1==s2 and s1>0:
                    shutil.move(src, dst); w("SHOT %02d %s (%d b)" % (i, name, s2))
                    ST["i"]+=1; ST["phase"]="set"
            except Exception as ex: w("move fail %s %s" % (name, ex))
        elif ST["await"] > 400:
            w("SHOT %02d %s TIMEOUT" % (i, name)); ST["i"]+=1; ST["phase"]="set"

def fin():
    try:
        if ST["h"]: unreal.unregister_slate_post_tick_callback(ST["h"])
    except Exception: pass
    w("DONE");
    try: unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
    except Exception: pass

ST["h"] = unreal.register_slate_post_tick_callback(tick)
w("capturing pass2...")
