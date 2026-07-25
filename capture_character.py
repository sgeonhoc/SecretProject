# 범용 VRoid 캐릭터 파츠 캡처. CONFIG의 MODE로 probe(4장)/full(37장) 전환.
# Map_Main(깨끗한 배경·조명)에 대상 메시를 띄워 실제 뷰포트 렌더로 캡처.
import unreal, os, time, shutil

# ===== CONFIG =====
MESH_PATH = "/Game/TestCharacter/test7/SK_test7"
CHAR      = "test7"
FRONT_YAW = 0          # 정면이 카메라(+Y)를 보게 하는 yaw (프로브로 확인 후 수정)
MODE      = "full"     # "probe" | "full"  (probe로 FRONT_YAW=0 정면 확정됨 → full 37장)
# ==================

OUTBASE = r"C:/Secret_Project/_tripo/our_chars"
OUT = os.path.join(OUTBASE, CHAR + ("_probe" if MODE=="probe" else "")).replace("\\","/")
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
# 백그라운드(포커스 없음)에서도 슬레이트 틱이 멈추지 않게 CPU 스로틀 해제
# — 안 하면 헤드리스/비포커스 실행 시 register_slate_post_tick_callback 가 거의 안 돌아 캡처가 멈춤.
try:
    _ps=unreal.get_default_object(unreal.EditorPerformanceSettings)
    _ps.set_editor_property('bThrottleCPUWhenNotForeground', False)
    try: _ps.set_editor_property('bMonitorEditorPerformance', False)
    except Exception: pass
except Exception as _ex:
    pass
try: les.load_level('/Game/CiciToonCharacterShaderPak/Levels/Map_Main'); w("loaded Map_Main")
except Exception as ex: w("load fail %s"%ex)
world=ues.get_editor_world()

# 기존 스켈레탈 액터 전부 숨김
for a in eas.get_all_level_actors():
    if a.get_components_by_class(unreal.SkeletalMeshComponent):
        try: a.set_is_temporarily_hidden_in_editor(True)
        except Exception: pass

# 대상 메시 스폰
mesh=unreal.load_asset(MESH_PATH)
if not mesh: w("MESH load FAIL %s"%MESH_PATH)
actor=eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(230,0,0), unreal.Rotator(0,0,0))
comp=actor.skeletal_mesh_component
try: comp.set_skeletal_mesh_asset(mesh)
except Exception:
    try: comp.set_skeletal_mesh(mesh)
    except Exception as ex: w("set mesh fail %s"%ex)
target=actor
w("spawned %s"%CHAR)
# 레퍼런스 포즈
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

# 본 조회 (FRONT_YAW 상태에서)
target.set_actor_rotation(unreal.Rotator(roll=0.0,pitch=0.0,yaw=float(FRONT_YAW)),False)
def find_bone(*subs):
    try: nb=comp.get_num_bones()
    except Exception: return None
    for i in range(nb):
        low=str(comp.get_bone_name(i)).lower()
        if all(s in low for s in subs): return str(comp.get_bone_name(i))
    return None
def bloc(nm):
    try: return comp.get_socket_location(nm)
    except Exception: return None
b_head=find_bone("head"); b_eye=find_bone("faceeye") or find_bone("eye"); b_handR=find_bone("hand","_r") or find_bone("hand","r_"); b_handL=find_bone("hand","_l") or find_bone("hand","l_")
o,e=target.get_actor_bounds(False); cx,cy,cz=o.x,o.y,o.z; topz,botz=o.z+e.z,o.z-e.z
# 눈높이: 눈 본(FaceEye) 우선 → head 본 → 비율 폴백. (이 VRoid는 head 본이 입 높이라
# head를 쓰면 얼굴 클로즈업이 아래로 빗나감 → 눈 본을 우선 앵커로.)
eyeZ=None
if b_eye:
    L=bloc(b_eye)
    if L: eyeZ=L.z
if eyeZ is None and b_head:
    L=bloc(b_head)
    if L: eyeZ=L.z
if eyeZ is None: eyeZ=botz+(topz-botz)*0.92
hR=bloc(b_handR) if b_handR else None; hL=bloc(b_handL) if b_handL else None
w("bounds cz=%.1f top=%.1f bot=%.1f | head=%s eye=%s eyeZ=%.1f | handR=%s handL=%s"%(cz,topz,botz,b_head,b_eye,eyeZ,b_handR,b_handL))

RES_W,RES_H=1600,900
# 전신 fit 거리: 16:9 세로화각(fov_h=55 -> 반각tan~0.293)
def body_dist(fov_h):
    import math
    half_v=math.atan(math.tan(math.radians(fov_h/2.0))/ (RES_W/float(RES_H)))
    H=(topz-botz)*1.18
    return (H/2.0)/math.tan(half_v)
BFOV=55; BDZ=(topz+botz)/2.0; BDIST=body_dist(BFOV)

# 샷 구성: (name, yaw, look_x, look_z, dist, fov, [look_y])
def build_full():
    S=[]
    for nm,yoff in [("front",0),("fl45",45),("left90",90),("bl135",135),("back180",180),("br225",225),("right270",270),("fr315",315)]:
        S.append(("body_"+nm, FRONT_YAW+yoff, cx, BDZ, BDIST, BFOV, None))
    # 얼굴 각도
    for nm,yoff,d,fov in [("face_front",0,134,24),("face_34L",-30,134,24),("face_34R",30,134,24),("face_left",90,134,24),("face_right",270,134,24)]:
        S.append((nm, FRONT_YAW+yoff, cx, eyeZ, d, fov, None))
    # 이목구비 (head본 z 기준 오프셋)
    S+= [("eyes",FRONT_YAW,cx,eyeZ,58,16,None),("eyebrow",FRONT_YAW,cx,eyeZ+3,55,14,None),
         ("nose",FRONT_YAW,cx,eyeZ-4,52,14,None),("mouth",FRONT_YAW,cx,eyeZ-8.5,50,15,None),
         ("ear",FRONT_YAW+90,cx,eyeZ-1,70,18,None)]
    # 머리
    S+= [("hair_bangs",FRONT_YAW,cx,eyeZ+8,90,22,None),("hair_back",FRONT_YAW+180,cx,eyeZ,110,30,None),
         ("hair_side",FRONT_YAW+90,cx,eyeZ,110,30,None)]
    # 상체/의상 (비율 기준 z)
    midz=botz+(topz-botz)
    z_chest=botz+(topz-botz)*0.72; z_waist=botz+(topz-botz)*0.55; z_torso=botz+(topz-botz)*0.66
    S+= [("collar_bow",FRONT_YAW,cx,botz+(topz-botz)*0.80,90,22,None),("chest_vest",FRONT_YAW,cx,z_chest,130,30,None),
         ("upper_torso",FRONT_YAW,cx,z_torso,165,38,None),("shoulder",FRONT_YAW+40,cx,botz+(topz-botz)*0.82,120,26,None)]
    # 손/팔 (본 앵커)
    # 손목 본보다 손가락이 바깥(몸 중심 반대)으로 뻗음 → look_x를 바깥으로 밀고 줌아웃해 손 전체 프레임
    _ob=lambda x:(7.0 if x>=cx else -7.0)
    if hR: S.append(("hand_R",FRONT_YAW,hR.x+_ob(hR.x),hR.z,55,22,hR.y))
    if hL: S.append(("hand_L",FRONT_YAW,hL.x+_ob(hL.x),hL.z,55,22,hL.y))
    if hR: S.append(("arm_R",FRONT_YAW,(cx+hR.x)/2.0,hR.z+2,100,30,None))
    # 하체
    S+= [("waist_hem",FRONT_YAW,cx,z_waist,110,24,None),("skirt_front",FRONT_YAW,cx,botz+(topz-botz)*0.36,125,30,None),
         ("skirt_pleats",FRONT_YAW,cx,botz+(topz-botz)*0.33,80,22,None)]
    # 다리/발
    S+= [("legs",FRONT_YAW,cx,botz+(topz-botz)*0.16,150,34,None),("knees",FRONT_YAW,cx,botz+(topz-botz)*0.24,90,24,None),
         ("socks_feet",FRONT_YAW,cx,botz+(topz-botz)*0.06,110,24,None),("shoes",FRONT_YAW,cx,botz+6,115,24,None)]
    # 뒤
    S+= [("back_full",FRONT_YAW+180,cx,BDZ,BDIST,BFOV,None),("backpack",FRONT_YAW+180,cx,z_chest,110,30,None)]
    return S

def build_probe():
    return [("PROBE_front_y%d"%FRONT_YAW, FRONT_YAW, cx, BDZ, BDIST, BFOV, None),
            ("PROBE_back_y%d"%(FRONT_YAW+180), FRONT_YAW+180, cx, BDZ, BDIST, BFOV, None),
            ("PROBE_face_y%d"%FRONT_YAW, FRONT_YAW, cx, eyeZ, 120, 24, None),
            ("PROBE_face_y%d"%(FRONT_YAW+180), FRONT_YAW+180, cx, eyeZ, 120, 24, None)]

SHOTS = build_probe() if MODE=="probe" else build_full()
w("MODE=%s shots=%d"%(MODE,len(SHOTS)))

cam=eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(0,0,150), unreal.Rotator(0,0,0))
camc=cam.camera_component
try: les.pilot_level_actor(cam)
except Exception as ex: w("pilot fail %s"%ex)

def setcam(item):
    name,yaw,lx,lz,d,fov,ly=item
    target.set_actor_rotation(unreal.Rotator(roll=0.0,pitch=0.0,yaw=float(yaw)),False)
    o2,e2=target.get_actor_bounds(False)
    fy = ly if ly is not None else o2.y
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
    if ST["wd"]>50000: w("WD"); fin(); return
    if ST["phase"]=="warmup":
        ST["warm"]+=1
        if ST["warm"]==1 and SHOTS: setcam(SHOTS[0])
        if ST["warm"]>=420: w("warmup done"); ST["phase"]="set"
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
                    shutil.move(src,dst); w("SHOT %02d %s (%d b)"%(i,name,s2)); ST["i"]+=1; ST["phase"]="set"
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
w("%s %s capturing..."%(CHAR,MODE))
