# -*- coding: utf-8 -*-
# ▶ L01 아랫장터 큰길 — 도시 스케일 + 건축 디테일 + 세상 뒤 배경 + 쨍한 색
#   교훈(2026-07-25): 민짜 큐브 = 장난감. 건물마다 돌림띠·창틀·창턱·발코니·덧문·배관·지붕선(파라펫·굴뚝·
#   물탱크·안테나)이 있어야 벽이 벽으로, 도시가 도시로 읽힌다(_build_L01_street.py의 교훈).
#   여기에 ①도시 스케일(240m) ②강 건너 유리탑 스카이라인(세상 뒤) ③P3식 쨍한 대낮 색을 합친다.
#   엔진 프리미티브 + 자작 머티리얼만. 실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=_build_L01_final.py
import unreal, random

MAP    = "/Game/Maps/Rasel/L01_Jangteo_Street"
MATDIR = "/Game/Rasel/Materials"
CUBE = "/Engine/BasicShapes/Cube.Cube"
CYL  = "/Engine/BasicShapes/Cylinder.Cylinder"
SPH  = "/Engine/BasicShapes/Sphere.Sphere"

les  = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
acts = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mel  = unreal.MaterialEditingLibrary
tools= unreal.AssetToolsHelpers.get_asset_tools()
MATS = {}
_meshes = {}
CNT = {"n":0}

def log(m):
    unreal.log("[L01f] "+str(m))
    with open("C:/Secret_Project/Saved/l01_final.log","a",encoding="utf-8") as f: f.write(str(m)+"\n")

def make_mat(name, color, rough=0.85, metal=0.0, emissive=None, emi=1.0):
    path = MATDIR+"/"+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    m = tools.create_asset(name, MATDIR, unreal.Material, unreal.MaterialFactoryNew())
    c = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 0)
    c.set_editor_property("constant", unreal.LinearColor(color[0],color[1],color[2],1.0))
    mel.connect_material_property(c,"",unreal.MaterialProperty.MP_BASE_COLOR)
    r = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 200)
    r.set_editor_property("r", rough); mel.connect_material_property(r,"",unreal.MaterialProperty.MP_ROUGHNESS)
    if metal>0:
        mt = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 320)
        mt.set_editor_property("r", metal); mel.connect_material_property(mt,"",unreal.MaterialProperty.MP_METALLIC)
    if emissive:
        e = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 460)
        e.set_editor_property("constant", unreal.LinearColor(emissive[0]*emi,emissive[1]*emi,emissive[2]*emi,1.0))
        mel.connect_material_property(e,"",unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m); unreal.EditorAssetLibrary.save_asset(path); MATS[name]=m; return m

def build_materials():
    # ★쨍한 대낮 팔레트 — 채도 높고 밝게. 벽색을 여러 개(단조로움 깨기) + 디테일용(틀·턱·철).
    make_mat("M_Rasel_Cobble", (0.34,0.33,0.35), rough=0.9)
    make_mat("M_Rasel_Walk",   (0.55,0.52,0.50), rough=0.9)
    make_mat("M_Rasel_WallA",  (0.95,0.76,0.42))   # 크림
    make_mat("M_Rasel_WallB",  (0.90,0.40,0.26))   # 테라코타
    make_mat("M_Rasel_WallC",  (0.55,0.62,0.42))   # 올리브
    make_mat("M_Rasel_WallD",  (0.60,0.68,0.74))   # 회청
    make_mat("M_Rasel_WallE",  (0.88,0.58,0.54))   # 분홍
    make_mat("M_Rasel_WallF",  (0.80,0.74,0.58))   # 베이지
    make_mat("M_Rasel_Trim",   (0.96,0.93,0.86))   # 밝은 돌림띠/창틀
    make_mat("M_Rasel_Wood",   (0.42,0.24,0.12))
    make_mat("M_Rasel_Iron",   (0.22,0.23,0.25), rough=0.5, metal=0.7)
    make_mat("M_Rasel_Roof",   (0.06,0.52,0.46))   # 쨍한 청록 기와
    make_mat("M_Rasel_Dark",   (0.05,0.07,0.11))   # 창 안 어둠
    make_mat("M_Rasel_Puddle", (0.10,0.34,0.62), rough=0.08)
    make_mat("M_Rasel_CanvasR",(0.90,0.14,0.12))
    make_mat("M_Rasel_CanvasT",(0.06,0.56,0.62))
    make_mat("M_Rasel_CanvasO",(1.00,0.52,0.06))
    make_mat("M_Rasel_CanvasY",(0.96,0.80,0.10))
    make_mat("M_Rasel_CanvasG",(0.36,0.62,0.22))
    make_mat("M_Rasel_Plant",  (0.20,0.48,0.18))
    make_mat("M_Rasel_Sack",   (0.74,0.64,0.42))
    make_mat("M_Rasel_MidB",   (0.42,0.56,0.74))   # 중경(원경 물러남·색 살아있게)
    make_mat("M_Rasel_FarB",   (0.34,0.54,0.80))
    make_mat("M_Rasel_Tower",  (0.22,0.48,0.82), rough=0.3)   # 유리탑
    make_mat("M_Rasel_TowerB", (0.30,0.60,0.92), rough=0.25)
    make_mat("M_Rasel_River",  (0.06,0.36,0.70), rough=0.15)
    make_mat("M_Rasel_WinLight",(0.9,0.72,0.42), rough=0.4, emissive=(1.0,0.72,0.36), emi=2.0)
    make_mat("M_Rasel_Lamp",   (0.5,0.4,0.2), rough=0.3, emissive=(1.0,0.78,0.45), emi=5.0)
    make_mat("M_Rasel_Sign",   (0.12,0.12,0.14), rough=0.5)
    log("머티리얼 %d" % len(MATS))

WALLS  = ["M_Rasel_WallA","M_Rasel_WallB","M_Rasel_WallC","M_Rasel_WallD","M_Rasel_WallE","M_Rasel_WallF"]
CANVAS = ["M_Rasel_CanvasR","M_Rasel_CanvasT","M_Rasel_CanvasO","M_Rasel_CanvasY","M_Rasel_CanvasG"]

def mesh(p):
    if p not in _meshes: _meshes[p]=unreal.EditorAssetLibrary.load_asset(p)
    return _meshes[p]
def box(x,y,z,sx,sy,sz,mat="M_Rasel_WallF",yaw=0.0,name=None):
    a=acts.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x,y,z),unreal.Rotator(0,yaw,0))
    a.static_mesh_component.set_static_mesh(mesh(CUBE)); a.set_actor_scale3d(unreal.Vector(sx/100.0,sy/100.0,sz/100.0))
    a.static_mesh_component.set_material(0,MATS[mat]); a.set_mobility(unreal.ComponentMobility.STATIC)
    if name:a.set_actor_label(name)
    CNT["n"]+=1; return a
def cyl(x,y,z,rad,h,mat="M_Rasel_Iron",yaw=0.0,pitch=0.0,name=None):
    a=acts.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x,y,z),unreal.Rotator(pitch,yaw,0))
    a.static_mesh_component.set_static_mesh(mesh(CYL)); a.set_actor_scale3d(unreal.Vector(rad/50.0,rad/50.0,h/100.0))
    a.static_mesh_component.set_material(0,MATS[mat]); a.set_mobility(unreal.ComponentMobility.STATIC)
    if name:a.set_actor_label(name)
    CNT["n"]+=1; return a
def sph(x,y,z,rad,mat="M_Rasel_Lamp",name=None):
    a=acts.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x,y,z))
    a.static_mesh_component.set_static_mesh(mesh(SPH)); a.set_actor_scale3d(unreal.Vector(rad/50.0,rad/50.0,rad/50.0))
    a.static_mesh_component.set_material(0,MATS[mat]); a.set_mobility(unreal.ComponentMobility.STATIC)
    if name:a.set_actor_label(name)
    CNT["n"]+=1; return a

# ── 치수 ──
ST_LEN=24000; ST_W=1500; FLOOR_H=340; WALK_Y=None
FACE = lambda side, depth: side*(ST_W/2+60+300) - side*(depth/2+8)   # 길 마주보는 면 y
WALLY= lambda side: side*(ST_W/2+60+300)

def building(x, side, width, floors, wall):
    y=WALLY(side); depth=600; h=FLOOR_H*floors
    box(x,y,h/2,width,depth,h,wall,name="Bldg")
    # 층 돌림띠 — 벽이 벽으로 읽히는 핵심
    for f in range(1,floors+1):
        box(x, y-side*(depth/2+12), FLOOR_H*f, width, 46, 26, "M_Rasel_Trim", name="Ledge")
    # 창 — 틀+창턱+안쪽 어둠, 일부 불
    cols=max(2,int(width/260)); face=y-side*(depth/2+8)
    for f in range(1,floors):
        for c in range(cols):
            wx=x-width/2+width*(c+0.5)/cols; wz=FLOOR_H*f+FLOOR_H*0.55
            lit=((c+f+int(x/100))%4==0)
            box(wx,face,wz,120,16,150,"M_Rasel_WinLight" if lit else "M_Rasel_Dark",name="Win")
            box(wx,face-side*8,wz,150,12,180,"M_Rasel_Trim",name="WinFrame")
            box(wx,face-side*14,wz-100,160,40,16,"M_Rasel_Trim",name="Sill")
    box(x,y,h+20,width+60,depth+60,40,"M_Rasel_Roof",name="Cornice")

def shopfront(x, side, width, wall):
    y=WALLY(side); depth=600; fy=y-side*(depth/2+4)
    box(x,fy-side*30,130,240,70,260,"M_Rasel_Dark",name="Doorway")
    box(x,fy-side*62,130,44,22,250,"M_Rasel_Wood",name="DoorPost")
    can=random.choice(CANVAS)
    box(x,fy-side*200,300,width*0.8,400,16,can,name="Awning")
    for s in (-1,1): cyl(x+s*width*0.34,fy-side*380,150,8,300,"M_Rasel_Iron",name="AwnPole")
    box(x+width*0.30,fy-side*120,250,20,200,90,"M_Rasel_Sign",name="Sign")
    # 진열 물건
    for s in (-1,1):
        box(x+s*width*0.30,fy,170,width*0.22,14,220,"M_Rasel_WinLight",name="ShopGlass")
    tx=x; ty=fy-side*320
    for gi in range(random.randint(2,4)):
        gx=tx-width*0.22+width*0.44*gi/3
        pick=random.random()
        if pick<0.4: box(gx,ty,60,80,80,90,"M_Rasel_Wood",yaw=random.uniform(0,40),name="Crate")
        elif pick<0.7: cyl(gx,ty,60,40,80,"M_Rasel_Sack",name="Basket")
        else: cyl(gx,ty,55,40,60,"M_Rasel_CanvasO",name="Pot"); cyl(gx,ty,110,55,55,"M_Rasel_Plant",name="Plant")

def facade_detail(x, side, width, floors):
    y=WALLY(side); depth=600; face=y-side*(depth/2+8)
    # 발코니(층 하나)
    bf=2 if floors>=3 else 1; bz=FLOOR_H*bf+30
    if random.random()<0.7:
        box(x,face-side*90,bz,width*0.55,180,18,"M_Rasel_Trim",name="Balcony")
        for i in range(7):
            bx=x-width*0.26+width*0.52*i/6.0; cyl(bx,face-side*170,bz+55,5,110,"M_Rasel_Iron",name="Rail")
        box(x,face-side*170,bz+112,width*0.55,14,14,"M_Rasel_Iron",name="RailTop")
    # 덧문
    cols=max(2,int(width/260))
    for f in range(1,floors):
        for c in range(cols):
            wx=x-width/2+width*(c+0.5)/cols; wz=FLOOR_H*f+FLOOR_H*0.55
            for s in (-1,1): box(wx+s*92,face-side*14,wz,44,16,170,"M_Rasel_Wood",name="Shutter")
    # 벽 세로 간판
    if random.random()<0.6:
        box(x-width*0.36,face-side*26,FLOOR_H*1.35,70,30,260,random.choice(CANVAS),name="WallSign")
    # 실외기·배관
    for f in range(1,floors):
        if (f+int(x/200))%2==0: box(x+width*0.4,face-side*40,FLOOR_H*f+120,80,70,70,"M_Rasel_Iron",name="AcBox")
    for s in (-1,1): cyl(x+s*(width/2-30),face-side*16,FLOOR_H*floors/2,9,FLOOR_H*floors,"M_Rasel_Iron",name="Pipe")

def roofline(x, side, width, floors):
    y=WALLY(side); h=FLOOR_H*floors
    box(x,y-side*320,h+90,width,40,140,random.choice(WALLS),name="Parapet")
    for s in (-1,1):
        box(x+s*width*0.3,y+side*100,h+130,70,70,220,"M_Rasel_Wood",name="Chimney")
        box(x+s*width*0.3,y+side*100,h+250,90,90,20,"M_Rasel_Trim",name="ChimCap")
    if floors>=4:
        for lx in (-1,1):
            for ly in (-1,1): cyl(x+lx*60,y+ly*60,h+90,8,180,"M_Rasel_Iron",name="TankLeg")
        cyl(x,y,h+260,90,160,"M_Rasel_Iron",name="Tank")
    cyl(x+width*0.15,y-side*120,h+260,4,480,"M_Rasel_Iron",name="Antenna")

def ground():
    seg=40
    for i in range(seg):
        x=-ST_LEN/2+(ST_LEN/seg)*(i+0.5)
        m="M_Rasel_Cobble" if i%2==0 else "M_Rasel_Walk"
        box(x,0,-20,ST_LEN/seg-6,ST_W+900,40,m,name="Ground")
    for s in (1,-1):
        box(0,s*(ST_W/2+60),40,ST_LEN,120,120,"M_Rasel_Cobble",name="Curb")
    box(0,0,-12,ST_LEN,90,24,"M_Rasel_Dark",name="Drain")
    r=random.Random(5)
    for _ in range(60):
        px=r.uniform(-ST_LEN/2+300,ST_LEN/2-300); py=r.uniform(-ST_W/2+120,ST_W/2-120)
        box(px,py,-2,r.uniform(150,340),r.uniform(120,240),6,"M_Rasel_Puddle",name="Puddle")

def stalls_and_life():
    r=random.Random(9)
    # 좌판 두 줄(길 따라)
    x=-ST_LEN/2+1200
    while x<ST_LEN/2-1200:
        side=1 if r.random()<0.5 else -1; y=side*(ST_W/2-160)
        box(x,y,90,300,200,20,"M_Rasel_Wood",name="StallTop")
        for dx in (-130,130):
            for dy in (-80,80): cyl(x+dx,y+dy,45,8,90,"M_Rasel_Wood",name="StallLeg")
        box(x,y,260,340,240,14,r.choice(CANVAS),name="Canopy")
        for dx in (-150,150):
            for dy in (-100,100): cyl(x+dx,y+dy,175,6,250,"M_Rasel_Iron",name="Pole")
        for i in range(3): box(x-90+i*90,y,118,70,70,36,"M_Rasel_Wood",name="Crate")
        cyl(x+120,y-60,120,34,40,"M_Rasel_Sack",name="Basket")
        x+=r.uniform(900,1500)
    # 가로등
    for i in range(int(ST_LEN/900)):
        x=-ST_LEN/2+500+i*900; side=1 if i%2==0 else -1; y=side*(ST_W/2+20)
        cyl(x,y,210,12,420,"M_Rasel_Iron",name="LampPost")
        cyl(x,y-side*60,420,6,130,"M_Rasel_Iron",pitch=90,name="LampArm")
        sph(x,y-side*118,385,20,"M_Rasel_Lamp",name="Bulb")
    # 벤치·볼라드·궤짝더미
    for i in range(int(ST_LEN/700)):
        x=-ST_LEN/2+400+i*700
        for side in (1,-1): cyl(x,side*(ST_W/2+10),55,14,110,"M_Rasel_Iron",name="Bollard")
    r2=random.Random(17)
    x=-ST_LEN/2+900
    while x<ST_LEN/2-700:
        side=1 if r2.random()<0.5 else -1; y=side*(ST_W/2-90)
        pick=r2.random()
        if pick<0.4:
            box(x,y,62,220,60,16,"M_Rasel_Wood",name="Bench"); box(x,y+side*26,100,220,14,60,"M_Rasel_Wood",name="BenchBack")
        elif pick<0.7:
            box(x,y,45,110,110,90,"M_Rasel_Wood",name="Crate"); cyl(x-110,y,60,42,120,"M_Rasel_Iron",name="Barrel")
        else:
            box(x,y,60,90,70,70,"M_Rasel_Sack",name="Sack")
        x+=r2.uniform(500,900)
    # 널린 빨래(위층)
    for i in range(int(ST_LEN/1300)):
        x=-ST_LEN/2+900+i*1300; z=FLOOR_H*2+120
        box(x,0,z,8,ST_W+700,5,"M_Rasel_Iron",name="Line")
        for k in range(6):
            box(x,-520+k*210,z-70,12,130,r2.uniform(90,150),r2.choice(CANVAS),name="Laundry")

def side_alleys():
    r=random.Random(3)
    for ax in (-8000,-1500,5500,11000):
        side=1 if r.random()<0.5 else -1; y0=side*(ST_W/2+60)
        for k in range(6):
            yy=y0+side*(200+k*280)
            for s in (-1,1): box(ax+s*260,yy,700,140,280,1400,random.choice(WALLS),name="AlleyWall")
        box(ax,y0+side*1000,-18,420,2000,36,"M_Rasel_Cobble",name="AlleyGround")
        box(ax,y0+side*2100,600,600,60,1200,"M_Rasel_Dark",name="AlleyEnd")

def backdrop():
    # 중경·원경 건물(근경 지붕 너머 도시가 이어짐)
    r=random.Random(7)
    for side in (1,-1):
        x=-ST_LEN/2
        while x<ST_LEN/2:
            h=(5+r.randint(0,4))*FLOOR_H
            box(x,side*(ST_W/2+1500),h/2,r.uniform(700,1200),700,h,"M_Rasel_MidB",name="MidB")
            x+=r.uniform(800,1300)
    for side in (1,-1):
        for i in range(10):
            x=-ST_LEN/2+ST_LEN*i/9.0; h=(7+r.randint(0,5))*FLOOR_H
            box(x,side*(ST_W/2+3600),h/2,2200,1300,h,"M_Rasel_FarB",name="FarB")
    # 강 + 강 건너 유리탑 스카이라인(북쪽)
    box(0,7000,-30,ST_LEN+6000,2800,20,"M_Rasel_River",name="River")
    tx=-ST_LEN/2-2000
    while tx<ST_LEN/2+2000:
        w=r.uniform(700,1500); d=r.uniform(700,1400); h=r.uniform(4500,10000)
        ty=9000+r.uniform(0,3500)
        m="M_Rasel_Tower" if r.random()<0.6 else "M_Rasel_TowerB"
        box(tx,ty,h/2,w,d,h,m,name="Tower")
        for s in range(3,int(h/800)):
            if r.random()<0.5: box(tx,ty-d/2-6,s*800,w*0.8,12,120,"M_Rasel_WinLight",name="TowerWin")
        tx+=r.uniform(1300,2200)

def lighting():
    d=acts.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,3000),unreal.Rotator(-45,35,0))
    dc=d.get_component_by_class(unreal.DirectionalLightComponent)
    dc.set_editor_property("intensity",6.0); dc.set_editor_property("light_color",unreal.Color(255,247,235))
    dc.set_editor_property("atmosphere_sun_light",True); dc.set_editor_property("mobility",unreal.ComponentMobility.MOVABLE)
    acts.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector(0,0,0))
    s=acts.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1500))
    sc=s.get_component_by_class(unreal.SkyLightComponent)
    sc.set_editor_property("real_time_capture",True); sc.set_editor_property("intensity",1.3)
    sc.set_editor_property("mobility",unreal.ComponentMobility.MOVABLE)
    f=acts.spawn_actor_from_class(unreal.ExponentialHeightFog,unreal.Vector(0,0,60))
    fc=f.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property("fog_density",0.0016); fc.set_editor_property("start_distance",4000.0)
    fc.set_editor_property("fog_inscattering_luminance",unreal.LinearColor(0.25,0.5,0.9,1.0))
    pp=acts.spawn_actor_from_class(unreal.PostProcessVolume,unreal.Vector(0,0,500)); pp.set_editor_property("unbound",True)
    st=pp.get_editor_property("settings")
    st.set_editor_property("override_auto_exposure_method",True)
    st.set_editor_property("auto_exposure_method",unreal.AutoExposureMethod.AEM_MANUAL)
    st.set_editor_property("override_auto_exposure_bias",True); st.set_editor_property("auto_exposure_bias",9.5)
    st.set_editor_property("override_bloom_intensity",True); st.set_editor_property("bloom_intensity",0.35)
    st.set_editor_property("override_color_saturation",True); st.set_editor_property("color_saturation",unreal.Vector4(1.2,1.2,1.2,1.0))
    st.set_editor_property("override_color_contrast",True); st.set_editor_property("color_contrast",unreal.Vector4(1.06,1.06,1.06,1.0))
    pp.set_editor_property("settings",st)
    acts.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(-ST_LEN/2+900,0,150))

def main():
    build_materials()
    les.load_level(MAP)
    for a in list(acts.get_all_level_actors()):
        try: acts.destroy_actor(a)
        except Exception: pass
    ground()
    rng=random.Random(11)
    # 근경 건물 — 길 양쪽, 폭·층수·색·정면선 들쭉날쭉 (교차로 자리는 비움)
    cross=[-9000,-1600,5400,10900]
    for side in (1,-1):
        x=-ST_LEN/2+700
        while x<ST_LEN/2-700:
            if any(abs(x-cx)<900 for cx in cross): x+=900; continue
            wdt=rng.uniform(560,900); fl=rng.choice([3,3,4,4,5,6])
            random.seed(int(x)+ (0 if side>0 else 7))
            wall=rng.choice(WALLS)
            building(x,side,wdt,fl,wall); shopfront(x,side,wdt,wall)
            facade_detail(x,side,wdt,fl); roofline(x,side,wdt,fl)
            x+=wdt+rng.uniform(120,320)
    stalls_and_life(); side_alleys(); backdrop(); lighting()
    les.save_current_level()
    log("L01 완성 — 액터 %d" % CNT["n"])

main()
