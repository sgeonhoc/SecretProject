# -*- coding: utf-8 -*-
# ▶ L01 아랫장터 큰길 — 도시 스케일 재건 (2026-07-24 아트 방향)
#   근거: 라셀=백만 항구도시(세계관_현대_이야기 §1). 이전 48m "칸"이 세상에서 잘려 보이던 문제 교정.
#   원칙([[feedback-level-art-stylized-world]]): ①레벨 뒤에 '세상'(강 건너 업무지구 유리탑 스카이라인·
#   멀어지는 건물·대기 안개) — 하늘로 뚝 끊기는 절단면 금지 ②스타일라이즈드 통일 톤(황혼)·강한 색 대비
#   ③도시 스케일(긴 대로+수직감). 엔진 프리미티브 + 자작 머티리얼만(외부 에셋 0).
#   실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=_build_L01_cityscale.py
import math, unreal

MAP   = "/Game/Maps/Rasel/L01_Jangteo_Street"
MATDIR= "/Game/Rasel/Materials"
CUBE  = "/Engine/BasicShapes/Cube.Cube"
CYL   = "/Engine/BasicShapes/Cylinder.Cylinder"
PLANE = "/Engine/BasicShapes/Plane.Plane"

# ── 거리 치수 (도시 스케일) ─────────────────────────────
STREET_LEN = 24000.0   # 큰길 길이 240m (동서 +X)
HALF_W     = 750.0     # 차도 반폭(걷는 폭 ~15m)
WALK_Y     = 1050.0    # 근경 파사드가 서는 Y(양쪽)
STORY      = 340.0

_AS = None
_MATS = {}

def log(m):
    unreal.log("[L01cs] " + str(m))
    with open("C:/Secret_Project/Saved/l01_cityscale.log", "a", encoding="utf-8") as f:
        f.write(str(m) + "\n")

def A():
    global _AS
    if _AS is None:
        _AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return _AS

# ── 자작 머티리얼(있으면 재사용) ─────────────────────────
def mkmat(name, rgb, emissive=None, rough=0.9):
    path = MATDIR + "/" + name
    # 색을 새로 잡을 때마다 갱신 — 기존이 있으면 지우고 다시 만든다(안 그러면 옛 색이 남는다).
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    at = unreal.AssetToolsHelpers.get_asset_tools()
    mat = at.create_asset(name, MATDIR, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary
    col = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    col.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
    r.set_editor_property("r", rough)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if emissive:
        em = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 400)
        em.set_editor_property("constant", unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0))
        mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    return mat

def build_materials():
    # ★고채도·고대비 팔레트(P3식 쨍한 색감). 따뜻한 저지대 vs 쨍한 파랑 하늘/강/유리탑 = 강한 보색 대비.
    spec = {
        "M_L01_Cobble":   ((0.42, 0.40, 0.42), None, 0.9),
        "M_L01_Walk":     ((0.56, 0.53, 0.52), None, 0.9),
        "M_L01_PlasterA": ((0.95, 0.76, 0.42), None, 0.85),   # 쨍한 크림
        "M_L01_PlasterB": ((0.92, 0.40, 0.24), None, 0.85),   # 선명한 테라코타
        "M_L01_PlasterC": ((0.90, 0.83, 0.58), None, 0.85),
        "M_L01_Wood":     ((0.42, 0.24, 0.12), None, 0.8),
        "M_L01_Roof":     ((0.06, 0.52, 0.46), None, 0.75),   # 쨍한 청록 기와
        "M_L01_WinDark":  ((0.05, 0.08, 0.14), None, 0.5),
        "M_L01_ClothR":   ((0.90, 0.14, 0.12), None, 0.85),   # 선명한 빨강
        "M_L01_ClothT":   ((0.06, 0.56, 0.62), None, 0.85),   # 선명한 청록
        "M_L01_ClothO":   ((1.00, 0.52, 0.06), None, 0.85),   # 선명한 주황
        "M_L01_ClothY":   ((0.96, 0.80, 0.10), None, 0.85),   # 노랑 차양/천
        "M_L01_ClothG":   ((0.36, 0.62, 0.22), None, 0.85),   # 초록 천
        # 벽 색 다양화(단조로움 깨기)
        "M_L01_PlasterD": ((0.62, 0.70, 0.74), None, 0.85),   # 회청
        "M_L01_PlasterE": ((0.86, 0.56, 0.52), None, 0.85),   # 분홍기와톤
        "M_L01_PlasterF": ((0.55, 0.62, 0.42), None, 0.85),   # 올리브
        # 소품
        "M_L01_Plant":    ((0.20, 0.46, 0.18), None, 0.8),    # 화분 잎
        "M_L01_Sack":     ((0.72, 0.62, 0.40), None, 0.9),    # 자루
        "M_L01_Metal":    ((0.30, 0.32, 0.34), None, 0.5),    # 함석·통
        "M_L01_Sign":     ((0.14, 0.12, 0.12), None, 0.7),    # 간판 판
        # 원경(물러나되 색은 살아 있게 — 안개로 뭉개지 않는다)
        "M_L01_MidBldg":  ((0.40, 0.54, 0.72), None, 0.9),
        "M_L01_FarBldg":  ((0.32, 0.52, 0.78), None, 0.92),
        # 강 건너 업무지구 유리탑(쨍한 청색 유리)
        "M_L01_Tower":    ((0.20, 0.46, 0.80), None, 0.3),
        "M_L01_TowerB":   ((0.28, 0.58, 0.90), None, 0.25),
        "M_L01_River":    ((0.06, 0.34, 0.66), None, 0.2),     # 쨍한 파란 강
    }
    for n,(c,e,r) in spec.items():
        _MATS[n] = mkmat(n, c, e, r)
    # 발광(창불·등롱·탑 창띠) — 밝고 선명하게
    _MATS["M_L01_WinLit"]  = mkmat("M_L01_WinLit",  (1.0,0.78,0.42), emissive=(3.2,1.9,0.7), rough=0.4)
    _MATS["M_L01_Lantern"] = mkmat("M_L01_Lantern", (1.0,0.66,0.30), emissive=(5.0,2.4,0.9), rough=0.4)
    _MATS["M_L01_TowerLit"]= mkmat("M_L01_TowerLit",(0.5,0.8,1.0), emissive=(0.6,1.3,2.2), rough=0.3)
    log("머티리얼 %d" % len(_MATS))

# ── 프리미티브 ───────────────────────────────────────────
def box(cx, cy, cz, sx, sy, sz, mat, yaw=0.0, pitch=0.0, tag=""):
    a = A().spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx,cy,cz),
                                   unreal.Rotator(0.0, pitch, yaw))
    if not a: return None
    smc = a.static_mesh_component
    smc.set_static_mesh(unreal.EditorAssetLibrary.load_asset(CUBE))
    a.set_actor_scale3d(unreal.Vector(sx/100.0, sy/100.0, sz/100.0))
    if mat in _MATS: smc.set_material(0, _MATS[mat])
    a.set_mobility(unreal.ComponentMobility.STATIC)
    if tag: a.set_actor_label(tag)
    return a

def cyl(cx, cy, cz, dia, h, mat, tag=""):
    a = A().spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx,cy,cz))
    if not a: return None
    smc = a.static_mesh_component
    smc.set_static_mesh(unreal.EditorAssetLibrary.load_asset(CYL))
    a.set_actor_scale3d(unreal.Vector(dia/100.0, dia/100.0, h/100.0))
    if mat in _MATS: smc.set_material(0, _MATS[mat])
    a.set_mobility(unreal.ComponentMobility.STATIC)
    if tag: a.set_actor_label(tag)
    return a

import random as _rnd
WALLPOOL = ["M_L01_PlasterA","M_L01_PlasterB","M_L01_PlasterC","M_L01_PlasterD",
            "M_L01_PlasterE","M_L01_PlasterF"]
CLOTHPOOL = ["M_L01_ClothR","M_L01_ClothT","M_L01_ClothO","M_L01_ClothY","M_L01_ClothG"]

# ── 근경 건물 한 채(모양·창·옥상·상점을 매번 다르게) ─────
def building(cx, cy, w, depth, stories, wall, face_sign=-1, rng=None):
    r = rng or _rnd
    h = stories * STORY
    box(cx, cy, h/2.0, w, depth, h, wall, tag="bldg")
    # 지붕 + 지붕 잡동사니(굴뚝·물탱크·난간) — 스카이라인을 톱니처럼
    box(cx, cy + face_sign*40, h + 25, w+40, depth+120, 50, "M_L01_Roof", tag="roof")
    if r.random() < 0.55:
        box(cx + r.uniform(-w*0.3,w*0.3), cy, h+120, 90, 90, r.uniform(140,260), "M_L01_Metal", tag="chimney")
    if r.random() < 0.4:
        box(cx + r.uniform(-w*0.25,w*0.25), cy, h+90, 200, 200, 130, "M_L01_Wood", tag="watertank")
    fy = cy + face_sign*(depth/2.0 + 8)
    # 위층 창 — 층·칸마다 불규칙(단조로움 깨기). 가끔 발코니.
    nwin = max(2, int(w/380))
    for s in range(1, stories):   # 1층은 상점이라 위층부터
        wz = STORY*s + STORY*0.55
        skip = r.random() < 0.12
        if skip: continue
        for k in range(nwin):
            if r.random() < 0.15: continue
            wx = cx - w/2.0 + w*(k+0.5)/nwin
            lit = r.random() < 0.35
            wh = r.choice([140,150,170])
            box(wx, fy, wz, r.choice([130,150,170]), 16, wh,
                "M_L01_WinLit" if lit else "M_L01_WinDark", tag="win")
        if r.random() < 0.18:   # 발코니
            bx = cx - w/2.0 + w*r.uniform(0.2,0.8)
            box(bx, fy + face_sign*55, wz-70, 220, 90, 20, "M_L01_Wood", tag="balcony")
            box(bx, fy + face_sign*95, wz-30, 220, 14, 70, "M_L01_Metal", tag="balcrail")
    # 벽에 걸린 세로 간판/현수막
    if r.random() < 0.5:
        sx = cx - w/2.0 + w*r.uniform(0.15,0.85)
        box(sx, fy + face_sign*30, STORY*1.8, 70, 12, r.uniform(260,420),
            r.choice(CLOTHPOOL), tag="banner")

def shopfront(cx, cy, w, face_sign, r):
    """1층 상점 — 차양 + 열린 가게 어둠 + 진열 소품. 거리를 '장사하는 곳'으로."""
    fy = cy + face_sign*(250)
    kind = r.random()
    # 차양(거리로 나온 천)
    box(cx, fy, 240, min(w*0.8,360), 260, 14, r.choice(CLOTHPOOL), pitch=face_sign*10, tag="shopawn")
    # 열린 가게 안(어둠)
    box(cx, cy + face_sign*(200), 130, min(w*0.7,300), 30, 240, "M_L01_WinDark", tag="shopdark")
    # 진열대 + 물건
    tx = cx; ty = cy + face_sign*300
    box(tx, ty, 70, min(w*0.6,280), 90, 18, "M_L01_Wood", tag="counter")
    goods = r.randint(2,4)
    for gi in range(goods):
        gx = tx - w*0.22 + w*0.44*gi/max(1,goods-1)
        pick = r.random()
        if pick < 0.4:   basket(gx, ty, 92)
        elif pick < 0.7: crate(gx, ty, 90)
        else:            pot(gx, ty, 88)

def door_slot(cx, cy, face_sign, wtag="door"):
    fy = cy + face_sign*(60)
    box(cx, fy, 150, 180, 20, 300, "M_L01_WinDark", tag=wtag)
    box(cx-105, fy, 150, 26, 26, 320, "M_L01_Wood", tag="post")
    box(cx+105, fy, 150, 26, 26, 320, "M_L01_Wood", tag="post")
    box(cx, fy, 315, 230, 30, 44, "M_L01_Wood", tag="lintel")

# ── 소품 프리미티브(작게, 장사·생활 흔적) ────────────────
def crate(cx, cy, cz, s=None):
    s = s or _rnd.uniform(70,110)
    box(cx, cy, cz, s, s, s, "M_L01_Wood", yaw=_rnd.uniform(0,40), tag="crate")
def basket(cx, cy, cz):
    d = _rnd.uniform(70,95); cyl(cx, cy, cz, d, d*0.9, "M_L01_Sack", tag="basket")
def barrel(cx, cy, cz):
    cyl(cx, cy, cz, 80, 120, "M_L01_Wood", tag="barrel")
def pot(cx, cy, cz):
    cyl(cx, cy, cz, 60, 70, "M_L01_ClothO", tag="pot")
    cyl(cx, cy, cz+70, 90, 90, "M_L01_Plant", tag="plant")
def sack(cx, cy, cz):
    box(cx, cy, cz, 90, 70, 70, "M_L01_Sack", yaw=_rnd.uniform(0,60), tag="sack")

def stall(cx, cy, cloth, r):
    box(cx, cy, 95, 360, 240, 24, "M_L01_Wood", tag="stall")
    for dx in (-160,160):
        for dy in (-100,100):
            cyl(cx+dx, cy+dy, 47, 22, 94, "M_L01_Wood", tag="leg")
    box(cx, cy, 275, 420, 300, 14, cloth, pitch=6, tag="awn")
    # 좌판 위 물건
    for gi in range(r.randint(2,4)):
        gx = cx + r.uniform(-140,140); gy = cy + r.uniform(-70,70)
        (basket if r.random()<0.5 else crate)(gx, gy, 130)
    if r.random() < 0.5: barrel(cx + r.uniform(-220,220), cy + r.uniform(-140,140)*-1, 60)

def lantern_string(x0, x1, y, z, n):
    """처마 등불 줄 — 두 점 사이에 등불을 줄줄이. 저잣거리 밤 canopy."""
    box((x0+x1)/2.0, y, z, abs(x1-x0), 6, 4, "M_L01_Wood", tag="wire")
    for i in range(n):
        lx = x0 + (x1-x0)*(i+0.5)/n
        cyl(lx, y, z-40, 26, 42, "M_L01_Lantern", tag="slantern")

def laundry_line(x0, x1, y, z):
    box((x0+x1)/2.0, y, z, abs(x1-x0), 5, 4, "M_L01_Metal", tag="clothesline")
    for i in range(int(abs(x1-x0)/220)):
        lx = x0 + (x1-x0)*(i+0.6)/max(1,int(abs(x1-x0)/220))
        box(lx, y, z-90, 90, 8, _rnd.uniform(90,160), _rnd.choice(CLOTHPOOL), tag="laundry")

def build():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
    # 옛 액터 비우기
    for a in list(A().get_all_level_actors()):
        try: A().destroy_actor(a)
        except Exception: pass
    build_materials()

    X0, X1 = 0.0, STREET_LEN
    midx = STREET_LEN/2.0

    # ── 차도 + 인도 ──
    box(midx, 0, -20, STREET_LEN+800, HALF_W*2, 40, "M_L01_Cobble", tag="Road")
    for sgn in (1,-1):
        box(midx, sgn*(HALF_W+180), -8, STREET_LEN+800, 360, 40, "M_L01_Walk", tag="Sidewalk")
    # 바닥 결: 얼룩 포석 패치·물웅덩이·맨홀(텅 빈 회색 깨기)
    grng = _rnd.Random(31)
    for _ in range(70):
        gx = grng.uniform(300, STREET_LEN-300); gy = grng.uniform(-HALF_W+120, HALF_W-120)
        t = grng.random()
        if t < 0.5:   box(gx, gy, -14, grng.uniform(200,520), grng.uniform(200,520), 12, "M_L01_Walk", yaw=grng.uniform(0,90), tag="patch")
        elif t < 0.8: box(gx, gy, -12, grng.uniform(160,340), grng.uniform(120,240), 6, "M_L01_River", yaw=grng.uniform(0,90), tag="puddle")
        else:         cyl(gx, gy, -10, 90, 8, "M_L01_Metal", tag="manhole")

    # ── 근경 건물: 양쪽에 폭이 다른 건물을 늘어세우되, 일정 간격 교차로(골목)로 끊는다 ──
    # (교차로 = 하늘이 아니라 '물러나는 골목'이 보이게 → 세상이 이어짐)
    cross_at = [3, 8, 13]   # 이 블록 자리에 교차로
    blocks = 17
    bw = STREET_LEN / blocks
    doors = {2:"L04", 5:"L02", 11:"L05", 14:"L06"}
    rng = _rnd.Random(11)
    for sgn in (1,-1):
        for b in range(blocks):
            # 한 블록 안에 폭이 다른 건물 1~2채(정면선이 들쭉날쭉 → 단조로움 깨기)
            if b in cross_at:
                cx = X0 + bw*(b+0.5)
                box(cx, sgn*(WALK_Y+600), 220, bw*0.5, 900, 440, "M_L01_WinDark", tag="AlleyMouth")
                box(cx, sgn*(WALK_Y+1500), 700, bw*0.42, 200, 1400, "M_L01_MidBldg", tag="AlleyInner")
                continue
            nsub = rng.choice([1,2])
            x_at = X0 + bw*b
            for si in range(nsub):
                w = bw/nsub - rng.uniform(40,120)
                cx = x_at + bw*(si+0.5)/nsub
                stories = rng.choice([3,3,4,4,5,6])       # 높이 리듬
                setback = rng.uniform(0, 140)             # 정면선 들쭉날쭉
                wall = rng.choice(WALLPOOL)
                building(cx, sgn*(WALK_Y+setback), w, rng.uniform(420,560),
                         stories, wall, face_sign=-sgn, rng=rng)
                shopfront(cx, sgn*(WALK_Y+setback), w, -sgn, rng)
            if b in doors:
                door_slot(X0 + bw*(b+0.5), sgn*WALK_Y, -sgn)

    # ── 중경 건물(한 겹 뒤, 더 높고 차갑게) — 근경 지붕 너머로 도시가 이어짐 ──
    for sgn in (1,-1):
        for b in range(blocks+1):
            cx = X0 + bw*b
            h = (5 + (b%4))*STORY
            box(cx, sgn*(WALK_Y+1300), h/2.0, bw*0.8, 700, h, "M_L01_MidBldg", tag="MidBldg")
    # ── 원경 건물(실루엣, 안개에 녹음) ──
    for sgn in (1,-1):
        for b in range(9):
            cx = X0 + STREET_LEN*b/8.0
            h = (7 + (b%5))*STORY
            box(cx, sgn*(WALK_Y+3200), h/2.0, 2200, 1200, h, "M_L01_FarBldg", tag="FarBldg")

    # ── 강 + 강 건너 업무지구 유리탑 스카이라인(북쪽 +Y 멀리) ──
    # 캐논: 강이 도시 북쪽을 흐르고, 강 건너에 유리로 두른 업무지구.
    box(midx, 6400, -30, STREET_LEN+6000, 2600, 20, "M_L01_River", tag="River")
    import random
    random.seed(7)
    tx = -2000.0
    while tx < STREET_LEN+2000:
        w = random.uniform(700, 1500)
        d = random.uniform(700, 1400)
        h = random.uniform(4000, 9000)
        ty = 8200 + random.uniform(0, 3200)
        mat = "M_L01_Tower" if random.random() < 0.6 else "M_L01_TowerB"
        box(tx, ty, h/2.0, w, d, h, mat, tag="Tower")
        # 창불 띠 몇 줄(발광) — 스카이라인이 '켜진 도시'로
        for s in range(3, int(h/700)):
            if random.random() < 0.5:
                box(tx, ty - d/2.0 - 6, s*700, w*0.8, 12, 90, "M_L01_TowerLit", tag="towerwin")
        tx += random.uniform(1300, 2200)

    # ── 저잣거리 살림: 가운데 좌판 두 줄 + 처마 등불/빨래 canopy + 흩어진 물건 ──
    prng = _rnd.Random(23)
    # 중앙 좌판 두 줄(길을 따라) — 텅 빈 한복판을 장으로
    for row_y in (330, -330):
        x = 1400.0
        while x < STREET_LEN-1400:
            if abs((x % (STREET_LEN/17.0))) > 60:   # 교차로 근처는 비움
                stall(x, row_y, prng.choice(CLOTHPOOL), prng)
            x += prng.uniform(900, 1400)
    # 건물 밑동에 기댄 물건(궤짝·통·자루·화분) — 걷는 길 가장자리
    for sgn in (1,-1):
        x = 700.0
        while x < STREET_LEN-700:
            by = sgn*(WALK_Y-260) + prng.uniform(-40,40)
            pick = prng.random()
            if pick < 0.3:   crate(x, by, 55); crate(x+prng.uniform(60,110), by, 55)
            elif pick < 0.55: barrel(x, by, 60)
            elif pick < 0.75: sack(x, by, 40); sack(x+80, by-40, 40)
            elif pick < 0.9:  pot(x, by, 40)
            else:             basket(x, by, 48)
            x += prng.uniform(500, 900)
    # 처마 등불 줄 + 빨래 — 길 위를 가로/세로로 (사람 사는 canopy)
    for b in range(2, blocks-1):
        cx0 = X0 + bw*b; cx1 = X0 + bw*(b+1)
        if prng.random() < 0.55:
            lantern_string(cx0, cx1, prng.uniform(-200,200), prng.uniform(560,700), prng.randint(4,6))
        if prng.random() < 0.4:
            # 두 건물 사이 빨래(길을 가로질러 위쪽)
            laundry_line(cx0+120, cx1-120, prng.choice([WALK_Y-200,-(WALK_Y-200)]), prng.uniform(650,820))
    # 초점 소품 — 우물·화로·수레
    cyl(1800, 0, 60, 300, 120, "M_L01_Metal", tag="Well")
    box(midx, -180, 70, 240, 200, 90, "M_L01_Wood", tag="Handcart"); cyl(midx-140,-180,50,70,30,"M_L01_Metal")
    cyl(STREET_LEN-2600, 200, 60, 260, 110, "M_L01_Wood", tag="Trough")

    # ── 조명: ★쨍한 대낮(강한 색감·높은 대비) — 밝은 태양 + 맑은 파란 하늘 + 안개 최소 ──
    sun = A().spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(midx,0,3000),
                                     unreal.Rotator(0.0, -42.0, 30.0))
    if sun:
        c = sun.get_component_by_class(unreal.DirectionalLightComponent)
        c.set_editor_property("intensity", 7.0)
        c.set_editor_property("light_color", unreal.Color(255, 246, 232))
        c.set_editor_property("atmosphere_sun_light", True)
    A().spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(midx,0,0))
    sky = A().spawn_actor_from_class(unreal.SkyLight, unreal.Vector(midx,0,1500))
    if sky:
        sc = sky.get_component_by_class(unreal.SkyLightComponent)
        sc.set_editor_property("real_time_capture", True)
        sc.set_editor_property("intensity", 1.4)
    # 안개는 아주 옅게(원경 타워가 살짝 물러나기만 — 색을 뭉개지 않는다). 색도 쨍한 파랑.
    fog = A().spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(midx,0,60))
    if fog:
        fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        fc.set_editor_property("fog_density", 0.0018)
        fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.25,0.5,0.9,1.0))
        fc.set_editor_property("fog_height_falloff", 0.25)
        fc.set_editor_property("start_distance", 3000.0)
    # 노출 수동 + 살짝 채도/대비 부스트로 그래픽하게
    ppv = A().spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(midx,0,500))
    if ppv:
        ppv.set_editor_property("unbound", True)
        s = ppv.settings
        s.set_editor_property("override_auto_exposure_method", True)
        s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
        s.set_editor_property("override_auto_exposure_bias", True)
        s.set_editor_property("auto_exposure_bias", 9.5)
        s.set_editor_property("override_color_saturation", True)
        s.set_editor_property("color_saturation", unreal.Vector4(1.22, 1.22, 1.22, 1.0))
        s.set_editor_property("override_color_contrast", True)
        s.set_editor_property("color_contrast", unreal.Vector4(1.08, 1.08, 1.08, 1.0))
        ppv.settings = s

    # ── 플레이어 시작 ──
    ps = A().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(2200, 0, 130))
    ps.set_actor_label("PlayerStart")

    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    log("저장 완료 · 액터 %d" % len(A().get_all_level_actors()))

build()
