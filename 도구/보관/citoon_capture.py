# Hayakawa 뷰포트 캡처 (실제 셀 렌더 그대로). 풀 에디터에서 -ExecCmds="py ..." 로 구동.
# 슬레이트 틱 상태머신: 카메라 이동 -> FOV -> HighResShot -> 새 파일 이동/리네임 -> 다음.
import unreal, os, math, time, shutil

OUT = r"C:/Secret_Project/_tripo/hayakawa_caps/pass1"
SHOT_DIR = r"C:/Secret_Project/Saved/Screenshots/WindowsEditor"
LOG = r"C:/Secret_Project/기획/99_보관/구로그/citoon_capture_log.txt"
_log = []
def w(s):
    _log.append(str(s))
    try: open(LOG, "w", encoding="utf-8").write("\n".join(_log))
    except Exception: pass
    try: unreal.log("CAP> %s" % s)
    except Exception: pass

os.makedirs(OUT, exist_ok=True)
os.makedirs(SHOT_DIR, exist_ok=True)

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# 맵을 스크립트 안에서 직접 로드 (커맨드라인 경로 문제 회피)
try:
    les.load_level('/Game/CiciToonCharacterShaderPak/Levels/Map_Main')
    w("loaded Map_Main")
except Exception as ex:
    w("load_level fail: %s" % ex)
world = unreal.EditorLevelLibrary.get_editor_world()

# 1) 타깃만 남기고 다른 스켈레탈 액터 숨김
TARGET = "SKM_Hayakawa_Main"
target_actor = None
hid = 0
for a in eas.get_all_level_actors():
    lbl = a.get_actor_label()
    has_sk = bool(a.get_components_by_class(unreal.SkeletalMeshComponent))
    if lbl == TARGET and target_actor is None:
        target_actor = a; continue
    if has_sk:
        try: a.set_is_temporarily_hidden_in_editor(True); hid += 1
        except Exception as e: w("hide fail %s: %s" % (lbl, e))
w("target=%s hidden_others=%d" % (target_actor.get_actor_label() if target_actor else None, hid))

o, e = target_actor.get_actor_bounds(False)
CX, CY = o.x, o.y
TOPZ, BOTZ = o.z + e.z, o.z - e.z
w("center=(%.1f,%.1f) topZ=%.1f botZ=%.1f" % (CX, CY, TOPZ, BOTZ))

# 2) 게임뷰(그리드/아이콘 숨김)
try: les.editor_set_game_view(True); w("game_view ON")
except Exception as ex: w("game_view fail %s" % ex)

# 3) 샷 정의: (이름, 카메라위치, 바라볼지점, FOV)
def cam_at(angle_deg, dist, z, look_z, fov, name):
    rad = math.radians(angle_deg)
    cl = unreal.Vector(CX + dist*math.cos(rad), CY + dist*math.sin(rad), z)
    lk = unreal.Vector(CX, CY, look_z)
    rot = unreal.MathLibrary.find_look_at_rotation(cl, lk)
    return (name, cl, rot, fov)

SHOTS = []
# 전신 8각도 (월드 각도 라벨)
for ang in [0, 45, 90, 135, 180, 225, 270, 315]:
    SHOTS.append(cam_at(ang, 300.0, 95.0, 85.0, 30.0, "body_%03d" % ang))
# 얼굴 4방향 (어디가 정면인지 식별용)
for ang in [0, 90, 180, 270]:
    SHOTS.append(cam_at(ang, 80.0, 152.0, 152.0, 20.0, "face_%03d" % ang))

w("planned shots=%d" % len(SHOTS))

RESCMD = "HighResShot 1200x1600"

# 4) 상태머신
ST = {"i": 0, "phase": "set", "settle": 0, "await": 0, "pre": set(), "watchdog": 0, "handle": None}

def list_pngs():
    try: return set(f for f in os.listdir(SHOT_DIR) if f.lower().endswith(".png"))
    except Exception: return set()

def tick(dt):
    ST["watchdog"] += 1
    if ST["watchdog"] > 6000:   # 안전장치(약 100초+): 무한루프 방지
        w("WATCHDOG quit"); finish(); return
    i = ST["i"]
    if i >= len(SHOTS):
        finish(); return
    name, cl, rot, fov = SHOTS[i]
    ph = ST["phase"]
    if ph == "set":
        try:
            ues.set_level_viewport_camera_info(cl, rot)
            unreal.SystemLibrary.execute_console_command(world, "FOV %.1f" % fov)
        except Exception as ex: w("setcam fail %s: %s" % (name, ex))
        ST["settle"] = 0; ST["phase"] = "settle"
    elif ph == "settle":
        ST["settle"] += 1
        if ST["settle"] >= 5:   # 카메라 반영 위해 몇 프레임 대기
            ST["pre"] = list_pngs()
            try: unreal.SystemLibrary.execute_console_command(world, RESCMD)
            except Exception as ex: w("shot cmd fail %s: %s" % (name, ex))
            ST["await"] = 0; ST["phase"] = "await"
    elif ph == "await":
        ST["await"] += 1
        new = list_pngs() - ST["pre"]
        if new and ST["await"] >= 2:
            fn = sorted(new)[-1]
            src = os.path.join(SHOT_DIR, fn)
            dst = os.path.join(OUT, name + ".png")
            try:
                # 파일 쓰기 완료 대기(사이즈 안정)
                s1 = os.path.getsize(src); time.sleep(0.05); s2 = os.path.getsize(src)
                if s1 == s2 and s1 > 0:
                    shutil.move(src, dst); w("SHOT %02d %s -> %s (%d bytes)" % (i, name, dst, s2))
                    ST["i"] += 1; ST["phase"] = "set"
            except Exception as ex:
                w("move fail %s: %s" % (name, ex))
        elif ST["await"] > 400:   # 한 샷 타임아웃(~7초)
            w("SHOT %02d %s TIMEOUT (no file)" % (i, name)); ST["i"] += 1; ST["phase"] = "set"

def finish():
    try:
        if ST["handle"]: unreal.unregister_slate_post_tick_callback(ST["handle"])
    except Exception: pass
    w("DONE. quitting editor.")
    try: unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
    except Exception: pass

ST["handle"] = unreal.register_slate_post_tick_callback(tick)
w("registered tick callback; capturing...")
