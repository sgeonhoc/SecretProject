# -*- coding: utf-8 -*-
# ▶ L01 아랫장터 큰길 — 처음부터 끝까지 직접 짓는 레벨 (외부 에셋 0)
#   쓰는 것: 엔진 기본 프리미티브(Cube/Cylinder/Sphere)와 내가 만든 머티리얼뿐.
#   실행: RASEL_LEVEL 필요 없음.
#     UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="C:/Secret_Project/_build_L01_street.py" -RenderOffScreen -unattended -nosplash
#   정본: 기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md  L01 장터 큰길
import unreal

MAP = "/Game/Maps/Rasel/L01_Jangteo_Street"
MATDIR = "/Game/Rasel/Materials"
CUBE = "/Engine/BasicShapes/Cube.Cube"
CYL = "/Engine/BasicShapes/Cylinder.Cylinder"
SPH = "/Engine/BasicShapes/Sphere.Sphere"
LOG = "C:/Secret_Project/Saved/L01_build.log"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
acts = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(m):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write(str(m) + "\n")


open(LOG, "w", encoding="utf-8").close()

# ───────────────────────── 머티리얼 (직접 제작) ─────────────────────────
MATS = {}


def make_mat(name, color, rough=0.85, metal=0.0, emissive=None, emi_power=1.0):
    """단색 PBR 머티리얼 하나. color=(r,g,b) 리니어."""
    path = MATDIR + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)   # 세기 조정분 반영 위해 새로 굽는다
    m = tools.create_asset(name, MATDIR, unreal.Material, unreal.MaterialFactoryNew())
    c = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 0)
    c.set_editor_property("constant", unreal.LinearColor(color[0], color[1], color[2], 1.0))
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)

    r = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 200)
    r.set_editor_property("r", rough)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)

    if metal > 0:
        mt = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 320)
        mt.set_editor_property("r", metal)
        mel.connect_material_property(mt, "", unreal.MaterialProperty.MP_METALLIC)

    if emissive:
        e = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 460)
        e.set_editor_property("constant", unreal.LinearColor(
            emissive[0] * emi_power, emissive[1] * emi_power, emissive[2] * emi_power, 1.0))
        mel.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mel.recompile_material(m)
    unreal.EditorAssetLibrary.save_asset(path)
    MATS[name] = m
    return m


def build_materials():
    # 밤 장터의 팔레트 — 젖은 돌바닥, 낡은 벽돌, 회칠, 나무, 녹슨 철, 차양천, 유리
    make_mat("M_Rasel_Stone",   (0.055, 0.052, 0.048), rough=0.62)   # 젖어 반들한 포석
    make_mat("M_Rasel_Brick",   (0.085, 0.048, 0.038), rough=0.90)
    make_mat("M_Rasel_Plaster", (0.115, 0.105, 0.090), rough=0.92)
    make_mat("M_Rasel_Wood",    (0.070, 0.045, 0.026), rough=0.85)
    make_mat("M_Rasel_Iron",    (0.035, 0.034, 0.033), rough=0.45, metal=0.85)
    make_mat("M_Rasel_Canvas",  (0.120, 0.055, 0.040), rough=0.95)   # 차양천(빛바랜 붉은)
    make_mat("M_Rasel_Dark",    (0.012, 0.012, 0.014), rough=0.95)   # 창 안쪽 어둠
    make_mat("M_Rasel_Puddle",  (0.020, 0.022, 0.026), rough=0.08)   # 물웅덩이
    make_mat("M_Rasel_Roof",    (0.040, 0.038, 0.040), rough=0.80)
    # 발광 — 창문 불빛(따뜻), 등불, 간판
    make_mat("M_Rasel_WinLight", (0.4, 0.3, 0.16), rough=0.4, emissive=(1.0, 0.72, 0.36), emi_power=1.6)
    make_mat("M_Rasel_Lamp",     (0.5, 0.4, 0.2),  rough=0.3, emissive=(1.0, 0.78, 0.45), emi_power=5.0)
    make_mat("M_Rasel_Sign",     (0.2, 0.25, 0.3), rough=0.4, emissive=(0.35, 0.65, 0.85), emi_power=2.5)
    log("머티리얼 %d개 준비" % len(MATS))


# ───────────────────────── 형태 헬퍼 ─────────────────────────
_meshes = {}


def mesh(path):
    if path not in _meshes:
        _meshes[path] = unreal.EditorAssetLibrary.load_asset(path)
    return _meshes[path]


COUNT = {"n": 0}


def box(x, y, z, sx, sy, sz, mat="M_Rasel_Plaster", yaw=0.0, name=None):
    a = acts.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z),
                                    unreal.Rotator(0, yaw, 0))
    a.static_mesh_component.set_static_mesh(mesh(CUBE))
    a.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    a.static_mesh_component.set_material(0, MATS[mat])
    a.set_mobility(unreal.ComponentMobility.STATIC)
    if name:
        a.set_actor_label(name)
    COUNT["n"] += 1
    return a


def cyl(x, y, z, rad, h, mat="M_Rasel_Iron", yaw=0.0, pitch=0.0, name=None):
    a = acts.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z),
                                    unreal.Rotator(pitch, yaw, 0))
    a.static_mesh_component.set_static_mesh(mesh(CYL))
    # 기본 실린더: 반지름 50, 높이 100
    a.set_actor_scale3d(unreal.Vector(rad / 50.0, rad / 50.0, h / 100.0))
    a.static_mesh_component.set_material(0, MATS[mat])
    a.set_mobility(unreal.ComponentMobility.STATIC)
    if name:
        a.set_actor_label(name)
    COUNT["n"] += 1
    return a


def sph(x, y, z, rad, mat="M_Rasel_Lamp", name=None):
    a = acts.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z))
    a.static_mesh_component.set_static_mesh(mesh(SPH))
    a.set_actor_scale3d(unreal.Vector(rad / 50.0, rad / 50.0, rad / 50.0))
    a.static_mesh_component.set_material(0, MATS[mat])
    a.set_mobility(unreal.ComponentMobility.STATIC)
    if name:
        a.set_actor_label(name)
    COUNT["n"] += 1
    return a


def point_light(x, y, z, color, intensity, radius, name=None):
    a = acts.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, z))
    c = a.get_component_by_class(unreal.PointLightComponent)
    c.set_editor_property("intensity", intensity)
    c.set_editor_property("light_color", unreal.Color(color[0], color[1], color[2], 255))
    c.set_editor_property("attenuation_radius", radius)
    c.set_editor_property("source_radius", 12.0)
    c.set_editor_property("cast_shadows", True)
    c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    if name:
        a.set_actor_label(name)
    COUNT["n"] += 1
    return a


# ───────────────────────── 치수 (동선 기준) ─────────────────────────
ST_LEN = 4800        # 큰길 길이 48m
ST_W = 1000          # 걷는 폭 10m — 좌판이 양쪽을 먹으면 실제 6m
FLOOR_H = 340        # 한 층 높이
SIDEWALK = 120       # 인도 턱 높이


def ground():
    """포석 바닥 — 한 장으로 깔지 않고 구획을 나눠 색이 미묘하게 갈리게."""
    seg = 8
    for i in range(seg):
        x = -ST_LEN / 2 + (ST_LEN / seg) * (i + 0.5)
        m = "M_Rasel_Stone" if i % 2 == 0 else "M_Rasel_Roof"
        box(x, 0, -20, ST_LEN / seg - 6, ST_W + 900, 40, m, name="Ground_%d" % i)
    # 인도 턱 (양쪽)
    for s in (1, -1):
        box(0, s * (ST_W / 2 + 60), SIDEWALK / 2 - 20, ST_LEN, 120, SIDEWALK,
            "M_Rasel_Stone", name="Curb_%d" % s)
    # 가운데 배수로 + 쇠살대
    box(0, 0, -12, ST_LEN, 90, 24, "M_Rasel_Dark", name="Drain")
    for i in range(16):
        x = -ST_LEN / 2 + 150 + i * 300
        for k in range(5):
            box(x + k * 14, 0, 2, 8, 84, 8, "M_Rasel_Iron", name="Grate")
    # 물웅덩이 몇
    for (px, py, pr) in [(-1700, 260, 220), (-450, -300, 300), (900, 180, 260),
                         (2100, -240, 200), (1500, 330, 160)]:
        box(px, py, -2, pr, pr * 0.7, 6, "M_Rasel_Puddle", name="Puddle")


def building(x, side, width, floors, mat_wall, shop=True):
    """길 한쪽에 선 건물 하나 — 층·창·처마·간판·1층 가게 입구."""
    y = side * (ST_W / 2 + 60 + 300)       # 벽면이 인도 바로 뒤에
    depth = 600
    h = FLOOR_H * floors
    # 몸통
    box(x, y, h / 2, width, depth, h, mat_wall, name="Bldg")
    # 층마다 돌림띠(가로선) — 이게 있어야 벽이 벽으로 읽힌다
    for f in range(1, floors + 1):
        box(x, y - side * (depth / 2 + 12), FLOOR_H * f, width, 46, 26,
            "M_Rasel_Stone", name="Ledge")
    # 창 — 2층부터. 창틀(밝은 면) + 안쪽 어둠 + 절반은 불이 켜져 있다
    cols = max(2, int(width / 260))
    for f in range(1, floors):
        for c in range(cols):
            wx = x - width / 2 + width * (c + 0.5) / cols
            wz = FLOOR_H * f + FLOOR_H * 0.55
            wy = y - side * (depth / 2 + 8)
            lit = ((c + f + int(x / 100)) % 3 != 0)
            box(wx, wy, wz, 120, 16, 150, "M_Rasel_WinLight" if lit else "M_Rasel_Dark",
                name="Window")
            box(wx, wy - side * 10, wz, 150, 12, 180, "M_Rasel_Wood", name="WinFrame")
            # 창턱
            box(wx, wy - side * 16, wz - 100, 160, 40, 16, "M_Rasel_Stone", name="Sill")
            if lit:
                point_light(wx, wy - side * 120, wz, (255, 196, 120), 90, 320, "WinGlow")
    # 옥상 처마
    box(x, y, h + 20, width + 60, depth + 60, 40, "M_Rasel_Roof", name="Cornice")
    # 1층 가게 — 입구 홈, 차양, 간판
    if shop:
        fy = y - side * (depth / 2 + 4)
        # 어두운 입구 홈
        box(x, fy - side * 30, 130, 220, 70, 260, "M_Rasel_Dark", name="Doorway")
        box(x, fy - side * 60, 130, 40, 20, 250, "M_Rasel_Wood", name="DoorPost")
        # 차양 (앞으로 기울어 나옴)
        box(x, fy - side * 190, 300, width * 0.8, 380, 16, "M_Rasel_Canvas",
            yaw=0, name="Awning")
        for sxp in (-1, 1):
            cyl(x + sxp * width * 0.34, fy - side * 360, 150, 8, 300, "M_Rasel_Iron",
                name="AwningPole")
        # 매달린 간판
        box(x + width * 0.3, fy - side * 120, 250, 20, 200, 90, "M_Rasel_Sign", name="Sign")
        cyl(x + width * 0.3, fy - side * 40, 300, 6, 110, "M_Rasel_Iron", pitch=90, name="SignArm")
        # 창(상점 유리) 양옆
        for sxp in (-1, 1):
            box(x + sxp * width * 0.3, fy, 170, width * 0.24, 14, 220,
                "M_Rasel_WinLight", name="ShopGlass")
            point_light(x + sxp * width * 0.3, fy - side * 150, 170, (255, 186, 110),
                        160, 380, "ShopGlow")


def stalls():
    """좌판 — 길을 실제로 좁히는 것들. 천막·궤짝·바구니."""
    spots = [(-2000, 1), (-1400, -1), (-800, 1), (-200, -1), (400, 1),
             (1000, -1), (1600, 1), (2200, -1)]
    for (x, side) in spots:
        y = side * (ST_W / 2 - 130)
        # 좌판 상판 + 다리
        box(x, y, 90, 300, 200, 20, "M_Rasel_Wood", name="StallTop")
        for dx in (-130, 130):
            for dy in (-80, 80):
                cyl(x + dx, y + dy, 45, 8, 90, "M_Rasel_Wood", name="StallLeg")
        # 천막
        box(x, y, 260, 340, 240, 14, "M_Rasel_Canvas", name="StallCanopy")
        for dx in (-150, 150):
            for dy in (-100, 100):
                cyl(x + dx, y + dy, 175, 6, 250, "M_Rasel_Iron", name="CanopyPole")
        # 물건 — 궤짝과 바구니
        for i in range(3):
            box(x - 90 + i * 90, y, 118, 70, 70, 36, "M_Rasel_Wood", name="Crate")
        cyl(x + 120, y - 60, 120, 34, 40, "M_Rasel_Wood", name="Basket")
        # 좌판 등불
        sph(x, y, 235, 14, "M_Rasel_Lamp", name="StallBulb")
        point_light(x, y, 230, (255, 178, 96), 320, 520, "StallLight")
        # 옆에 쌓인 짐
        box(x + 210, y + (30 * side), 60, 90, 90, 120, "M_Rasel_Wood", name="Bale")


def lamps():
    """가로등 — 빛 웅덩이가 길을 리듬으로 나눈다."""
    for i in range(7):
        x = -ST_LEN / 2 + 400 + i * 700
        side = 1 if i % 2 == 0 else -1
        y = side * (ST_W / 2 + 20)
        cyl(x, y, 210, 12, 420, "M_Rasel_Iron", name="LampPost")
        cyl(x, y - side * 60, 420, 6, 130, "M_Rasel_Iron", pitch=90, name="LampArm")
        box(x, y - side * 118, 410, 46, 46, 60, "M_Rasel_Iron", name="LampHead")
        sph(x, y - side * 118, 385, 20, "M_Rasel_Lamp", name="LampBulb")
        point_light(x, y - side * 118, 380, (255, 190, 118), 1100, 1000, "StreetLight")


def clutter():
    """도시가 사람 사는 곳으로 읽히게 하는 잡동사니."""
    # 벽에 붙은 물받이 관 + 실외기 상자
    for i in range(10):
        x = -ST_LEN / 2 + 260 + i * 480
        side = 1 if i % 2 else -1
        y = side * (ST_W / 2 + 60 + 8)
        cyl(x, y, 500, 11, 1000, "M_Rasel_Iron", name="Downpipe")
        box(x + 120, y, 620, 90, 60, 70, "M_Rasel_Iron", name="WallBox")
    # 길 건너 걸린 줄 + 매달린 등
    for i in range(6):
        x = -ST_LEN / 2 + 500 + i * 760
        box(x, 0, 640, 10, ST_W + 200, 6, "M_Rasel_Iron", name="Wire")
        for k in (-1, 0, 1):
            sph(x, k * 300, 610, 11, "M_Rasel_Lamp", name="WireBulb")
            point_light(x, k * 300, 605, (255, 200, 130), 90, 340, "WireLight")
    # 궤짝 더미·통·버려진 짐
    piles = [(-2400, 380), (-1100, -420), (100, 400), (1300, -400), (2500, 360), (-300, -390)]
    for (x, y) in piles:
        box(x, y, 45, 110, 110, 90, "M_Rasel_Wood", name="Crate")
        box(x + 40, y + 30, 130, 90, 90, 80, "M_Rasel_Wood", name="Crate")
        cyl(x - 110, y, 60, 42, 120, "M_Rasel_Iron", name="Barrel")
    # 길 끝을 막는 구조물(시선 차단) — 저 끝이 어디로 이어지는지 감추기
    box(ST_LEN / 2 + 200, 0, 400, 200, ST_W + 1400, 800, "M_Rasel_Brick", name="EndBlock")
    box(ST_LEN / 2 + 60, 0, 190, 80, 420, 380, "M_Rasel_Dark", name="EndArch")


def sky_and_mood():
    """밤 — 달빛은 약하게, 안개로 깊이를, 색은 등불이 만든다."""
    d = acts.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000),
                                    unreal.Rotator(-38, 145, 0))
    dc = d.get_component_by_class(unreal.DirectionalLightComponent)
    dc.set_editor_property("intensity", 0.10)
    dc.set_editor_property("light_color", unreal.Color(120, 148, 205, 255))
    dc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    d.set_actor_label("Moon")

    s = acts.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 900))
    sc = s.get_component_by_class(unreal.SkyLightComponent)
    sc.set_editor_property("intensity", 0.05)
    sc.set_editor_property("light_color", unreal.Color(90, 110, 160, 255))
    sc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    s.set_actor_label("SkyLight")

    f = acts.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 200))
    fc = f.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property("fog_density", 0.030)
    fc.set_editor_property("fog_height_falloff", 0.35)
    fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.055, 0.065, 0.10, 1.0))
    f.set_actor_label("Fog")

    pp = acts.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 400))
    pp.set_editor_property("unbound", True)
    st = pp.get_editor_property("settings")
    st.set_editor_property("override_auto_exposure_min_brightness", True)
    st.set_editor_property("auto_exposure_min_brightness", 1.0)
    st.set_editor_property("override_auto_exposure_max_brightness", True)
    st.set_editor_property("auto_exposure_max_brightness", 1.0)
    st.set_editor_property("override_bloom_intensity", True)
    st.set_editor_property("bloom_intensity", 0.45)
    st.set_editor_property("override_color_saturation", True)
    st.set_editor_property("color_saturation", unreal.Vector4(0.92, 0.94, 1.05, 1.0))
    st.set_editor_property("override_auto_exposure_bias", True)
    st.set_editor_property("auto_exposure_bias", -1.2)
    pp.set_editor_property("settings", st)
    pp.set_actor_label("PostProcess")

    acts.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-ST_LEN / 2 + 300, 0, 140))


def facade_detail(x, side, width, floors):
    """벽면이 벽면으로만 보이지 않게 — 발코니·덧문·배관·벽 간판·에어컨 상자."""
    y = side * (ST_W / 2 + 60 + 300)
    depth = 600
    face = y - side * (depth / 2 + 8)      # 길을 마주보는 면
    # 2층 발코니 (건물마다 하나씩, 좌우 번갈아)
    bf = 2 if floors >= 3 else 1
    bz = FLOOR_H * bf + 30
    box(x, face - side * 90, bz, width * 0.55, 180, 18, "M_Rasel_Stone", name="BalconyFloor")
    for i in range(7):                     # 난간 살
        bx = x - width * 0.26 + width * 0.52 * i / 6.0
        cyl(bx, face - side * 170, bz + 55, 5, 110, "M_Rasel_Iron", name="Rail")
    box(x, face - side * 170, bz + 112, width * 0.55, 14, 14, "M_Rasel_Iron", name="RailTop")
    for sxp in (-1, 1):                    # 발코니 받침
        box(x + sxp * width * 0.24, face - side * 60, bz - 40, 24, 120, 70,
            "M_Rasel_Stone", name="Corbel")
    # 덧문 — 창 옆에 접어 둔 나무 덧문
    cols = max(2, int(width / 260))
    for f in range(1, floors):
        for c in range(cols):
            wx = x - width / 2 + width * (c + 0.5) / cols
            wz = FLOOR_H * f + FLOOR_H * 0.55
            for sxp in (-1, 1):
                box(wx + sxp * 92, face - side * 14, wz, 46, 16, 170,
                    "M_Rasel_Wood", name="Shutter")
    # 벽에 붙은 간판 (세로형)
    box(x - width * 0.36, face - side * 26, FLOOR_H * 1.35, 70, 30, 260,
        "M_Rasel_Sign", name="WallSign")
    # 실외기·배관 상자
    for f in range(1, floors):
        if (f + int(x / 200)) % 2 == 0:
            box(x + width * 0.4, face - side * 40, FLOOR_H * f + 120, 80, 70, 70,
                "M_Rasel_Iron", name="AcBox")
    # 세로 배관 두 줄
    for sxp in (-1, 1):
        cyl(x + sxp * (width / 2 - 30), face - side * 16, FLOOR_H * floors / 2,
            9, FLOOR_H * floors, "M_Rasel_Iron", name="Pipe")


def roofline(x, side, width, floors):
    """지붕선 — 하늘과 만나는 선이 밋밋하면 도시가 가짜로 보인다."""
    y = side * (ST_W / 2 + 60 + 300)
    h = FLOOR_H * floors
    # 난간벽(파라펫)
    box(x, y - side * 320, h + 90, width, 40, 140, "M_Rasel_Brick", name="Parapet")
    # 굴뚝 두엇
    for sxp in (-1, 1):
        box(x + sxp * width * 0.3, y + side * 100, h + 130, 70, 70, 220,
            "M_Rasel_Brick", name="Chimney")
        box(x + sxp * width * 0.3, y + side * 100, h + 250, 90, 90, 20,
            "M_Rasel_Stone", name="ChimneyCap")
    # 옥상 물탱크 (다리 위에 올린 원통)
    if floors >= 4:
        for lx in (-1, 1):
            for ly in (-1, 1):
                cyl(x + lx * 60, y + ly * 60, h + 90, 8, 180, "M_Rasel_Iron", name="TankLeg")
        cyl(x, y, h + 260, 90, 160, "M_Rasel_Iron", name="WaterTank")
    # 안테나
    cyl(x + width * 0.15, y - side * 120, h + 260, 4, 480, "M_Rasel_Iron", name="Antenna")


def street_life():
    """길에 사람의 자취 — 계단참·벤치·볼라드·수레·널린 빨래."""
    # 가게 앞 계단참(스툽)
    for i, x in enumerate(range(-2100, 2400, 640)):
        side = 1 if i % 2 == 0 else -1
        fy = side * (ST_W / 2 + 60 + 8)
        for st_i in range(3):
            box(x, fy - side * (40 + st_i * 34), 20 + st_i * 22,
                240 - st_i * 30, 34, 22, "M_Rasel_Stone", name="Stoop")
    # 벤치
    for (x, side) in [(-1750, -1), (-250, 1), (1150, -1), (2350, 1)]:
        y = side * (ST_W / 2 - 90)
        box(x, y, 62, 220, 60, 16, "M_Rasel_Wood", name="BenchSeat")
        box(x, y + side * 26, 100, 220, 14, 60, "M_Rasel_Wood", name="BenchBack")
        for dx in (-90, 90):
            box(x + dx, y, 30, 20, 56, 60, "M_Rasel_Iron", name="BenchLeg")
    # 볼라드 (차 못 들어오게)
    for i in range(14):
        x = -ST_LEN / 2 + 250 + i * 340
        for side in (1, -1):
            cyl(x, side * (ST_W / 2 + 10), 55, 14, 110, "M_Rasel_Iron", name="Bollard")
    # 손수레 두 대
    for (x, side) in [(-900, 1), (1750, -1)]:
        y = side * (ST_W / 2 - 200)
        box(x, y, 90, 260, 150, 24, "M_Rasel_Wood", name="CartBed")
        box(x - 120, y, 130, 24, 150, 100, "M_Rasel_Wood", name="CartBack")
        for dy in (-80, 80):
            cyl(x + 60, y + dy, 55, 55, 20, "M_Rasel_Iron", yaw=0, pitch=90, name="CartWheel")
        cyl(x + 160, y, 100, 8, 200, "M_Rasel_Wood", pitch=90, name="CartHandle")
        box(x, y, 140, 120, 110, 80, "M_Rasel_Canvas", name="CartLoad")
    # 위층에 널린 빨래 — 줄 하나에 천 몇 장
    for i in range(4):
        x = -1900 + i * 1300
        z = FLOOR_H * 2 + 120
        box(x, 0, z, 8, ST_W + 700, 5, "M_Rasel_Iron", name="LaundryLine")
        for k in range(6):
            ly = -520 + k * 210
            box(x, ly, z - 70, 12, 130, 140, "M_Rasel_Canvas" if k % 2 else "M_Rasel_Plaster",
                name="Laundry")


def side_alley():
    """옆으로 갈라지는 어두운 골목 — 길이 여기서 끝나지 않는다는 신호(→L03)."""
    ax = 480                      # 골목 입구 위치
    side = -1
    y0 = side * (ST_W / 2 + 60)
    # 골목 벽 두 줄 (길에서 멀어지는 방향)
    for k in range(6):
        yy = y0 + side * (200 + k * 260)
        for sxp in (-1, 1):
            box(ax + sxp * 220, yy, 520, 120, 240, 1040, "M_Rasel_Brick", name="AlleyWall")
    # 골목 바닥
    box(ax, y0 + side * 900, -18, 340, 1600, 36, "M_Rasel_Stone", name="AlleyGround")
    # 골목 안 등 하나 — 이 빛이 사람을 끌어들인다
    box(ax + 180, y0 + side * 700, 300, 40, 40, 50, "M_Rasel_Iron", name="AlleyLampBox")
    sph(ax + 150, y0 + side * 700, 290, 14, "M_Rasel_Lamp", name="AlleyBulb")
    point_light(ax + 140, y0 + side * 700, 285, (255, 176, 96), 420, 700, "AlleyLight")
    # 골목 안 잡동사니
    box(ax - 100, y0 + side * 500, 55, 110, 110, 110, "M_Rasel_Wood", name="AlleyCrate")
    cyl(ax + 120, y0 + side * 1100, 65, 44, 130, "M_Rasel_Iron", name="AlleyBarrel")
    # 골목 끝은 어둠으로 막는다(어디로 이어지는지 감춤)
    box(ax, y0 + side * 1750, 400, 500, 60, 800, "M_Rasel_Dark", name="AlleyEnd")


def main():
    build_materials()
    les.new_level(MAP)
    ground()
    # 길 양쪽 건물 — 폭·층수를 다르게 해서 실루엣이 들쭉날쭉하게
    plan = [(-2100, 700, 4), (-1300, 560, 3), (-650, 640, 5), (100, 720, 3),
            (850, 600, 4), (1500, 680, 3), (2200, 640, 5)]
    for (x, wdt, fl) in plan:
        building(x, 1, wdt, fl, "M_Rasel_Brick")
        facade_detail(x, 1, wdt, fl)
        roofline(x, 1, wdt, fl)
    plan2 = [(-2300, 620, 3), (-1600, 700, 5), (-850, 580, 3), (-100, 660, 4),
             (600, 720, 3), (1350, 600, 5), (2050, 680, 4)]
    for (x, wdt, fl) in plan2:
        building(x, -1, wdt, fl, "M_Rasel_Plaster")
        facade_detail(x, -1, wdt, fl)
        roofline(x, -1, wdt, fl)
    stalls()
    lamps()
    clutter()
    street_life()
    side_alley()
    sky_and_mood()
    les.save_current_level()
    log("L01 완성 — 액터 %d개" % COUNT["n"])


main()
