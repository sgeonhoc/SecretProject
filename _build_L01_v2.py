# -*- coding: utf-8 -*-
"""L01 장터 큰길 — 우리 부재 키트로 다시 짓는다(기준기 레벨).

규칙: /Engine/BasicShapes 조차 안 쓴다. 전부 우리가 깎은 `/Game/Rasel/Meshes` 부재 +
우리가 구운 `/Game/Rasel/Textures` 를 물린 `/Game/Rasel/Materials`.
근거: 기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md §2 L01 · 기획/02_게임설계/2_레벨/게임_레벨디자인_상세.md 결 A · 기획/01_세계관/3_현대/세계관_현대_이야기.md §1·§3-2·§8-1

실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=".../_build_L01_v2.py"
"""
import unreal, json, os, traceback

MAP = "/Game/Maps/Rasel/L01_Jangteo_Street"
MESHDIR = "/Game/Rasel/Meshes"
MANIFEST = "C:/Secret_Project/_ArtSource/kit_manifest.json"
LOG = "C:/Secret_Project/Saved/l01_v2.log"

TILE = 400.0
COLS = 12
FAC_Y = 600.0          # 파사드 앞면이 서는 자리
FT_HALF = 15.0         # 벽 두께의 절반(부재 기준 30)
ALLEY_COL = 5          # 우측에 뚫린 골목(→L03)

lines = []
_AS = None
_MATS = {}
KIT = {}


def log(m):
    lines.append(str(m))
    unreal.log("[L01v2] " + str(m))


def flush():
    with open(LOG, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


def A():
    global _AS
    if _AS is None:
        _AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return _AS


def xc(i):
    return TILE * i


def mat(path):
    if path not in _MATS:
        _MATS[path] = unreal.EditorAssetLibrary.load_asset(path)
    return _MATS[path]


_MESHES = {}


def mesh(name):
    if name not in _MESHES:
        _MESHES[name] = unreal.EditorAssetLibrary.load_asset("%s/%s" % (MESHDIR, name))
    return _MESHES[name]


def place(name, loc, yaw=0.0, pitch=0.0, roll=0.0, label=None, scale=None):
    """부재 하나를 놓고, 명세대로 머티리얼을 건다."""
    sm = mesh(name)
    if sm is None:
        log("  !! 부재 없음 %s" % name)
        return None
    a = A().spawn_actor_from_object(sm, unreal.Vector(*loc),
                                    unreal.Rotator(roll, pitch, yaw))
    if a is None:
        return None
    comp = a.static_mesh_component
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    if scale:
        a.set_actor_scale3d(unreal.Vector(*scale))
    for i, mp in enumerate(KIT.get(name, [])):
        m = mat(mp)
        if m:
            comp.set_material(i, m)
    a.set_actor_label(label or name)
    return a


# ── 바닥에 놓는 것들의 자리 예약 ──────────────────────────────────────
# ★눈대중으로 흩으면 계속 부딪힌다. 놓을 때마다 발자국(반지름)을 적어 두고,
#   다음 것은 빈자리에만 놓는다. 문 앞은 미리 통째로 잡아 둬서 아무것도 못 놓게 한다.
_TAKEN = []
FOOT = {"Stall_A": 130, "Stall_B": 130, "Crate": 48, "Barrel": 36, "Sack": 32,
        "Basket": 34, "RopeCoil": 38, "Bucket": 26, "Stool": 32, "Pot": 36,
        "Poles": 34, "Handcart": 125, "Lantern": 28, "Well": 115, "Bench": 100,
        "NoticeBoard": 95}


def ground_free(x, y, r, pad=16.0):
    for (tx, ty, tr) in _TAKEN:
        dx, dy = x - tx, y - ty
        if dx * dx + dy * dy < (r + tr + pad) ** 2:
            return False
    return True


def reserve(x, y, r):
    _TAKEN.append((x, y, r))


def reserve_doorways():
    """드나드는 문 앞은 사람이 지나가야 한다 — 반지름 150을 통째로 잡아 둔다."""
    for i in DOORS_L:
        reserve(xc(i) - 105.0, FAC_Y - 150.0, 150.0)
    for i in DOORS_R:
        reserve(xc(i) + 105.0, -FAC_Y + 150.0, 150.0)
    reserve(xc(ALLEY_COL), -FAC_Y - 60.0, 160.0)        # 골목 어귀
    log("문 앞·골목 어귀 %d자리 잡아 둠" % len(_TAKEN))


def put_ground(kind, x, y, yaw=0.0, label=None):
    """빈자리면 놓고 발자국을 적는다. 자리가 없으면 건너뛴다."""
    r = FOOT.get(kind, 40)
    if not ground_free(x, y, r):
        return False
    place("SM_Rasel_%s" % kind, (x, y, 0.0), yaw=yaw, label=label)
    reserve(x, y, r)
    return True


# ── 큰길 바닥 ─────────────────────────────────────────────────────────
def build_ground():
    n = 0
    for i in range(COLS):
        for row in (-400.0, 0.0, 400.0):
            place("SM_Rasel_RoadTile", (xc(i), row, 0.0), label="Road_%d_%d" % (i, int(row)))
            n += 1
        place("SM_Rasel_Gutter", (xc(i), 560.0, 0.0), label="Gutter_L%d" % i)
        place("SM_Rasel_Gutter", (xc(i), -560.0, 0.0), label="Gutter_R%d" % i)
        n += 2
    # 포장이 깨져 아래 옛 돌이 드러난 자리 (§7-3)
    place("SM_Rasel_RoadPatch", (xc(9), -170.0, 1.0), label="옛 돌 드러난 자리")
    # ★비 온 뒤 저녁 — 갓길 도랑 근처와 포장 팬 자리에 물웅덩이(등불을 되비침).
    #   걷는 한가운데(y 0 근처)는 비우고 갓길 쪽에 앉힌다.
    for (px, py, kind) in ((xc(2), 300.0, "Puddle_M"), (xc(5) + 120.0, -320.0, "Puddle_S"),
                           (xc(7), 340.0, "Puddle_L"), (xc(9), -170.0, "Puddle_S"),
                           (xc(10) + 80.0, 300.0, "Puddle_M"), (xc(3) - 60.0, -300.0, "Puddle_S")):
        place("SM_Rasel_%s" % kind, (px, py, 2.0), label="물웅덩이_%s" % kind)
    # 바큇자국 — 수레가 다니는 길 가운데 두 줄, 몇 칸 걸러
    for i in (1, 4, 7, 10):
        place("SM_Rasel_WheelRuts", (xc(i), 0.0, 1.0), label="바큇자국_%d" % i)
    log("바닥 %d칸 + 웅덩이6 + 바큇자국4" % n)


# ── 파사드 + 처마 ─────────────────────────────────────────────────────
# 문이 걸린 칸은 반드시 Home(문 있는 얼굴)으로 둔다
LEFT = ["Home", "Shop", "Home", "Low", "Shop", "Blank", "Tall", "Home",
        "Home", "Shop", "Low", "Tall"]
RIGHT = ["Shop", "Tall", "Blank", "Home", "Shop", None, "Home", "Low",
         "Blank", "Home", "Tall", "Home"]
MOD_HW = 200.0         # 칸 폭의 절반
FH_OF = {"Shop": 640.0, "Home": 640.0, "Blank": 640.0, "Tall": 960.0, "Low": 320.0}


def jitter(i, side, k=0):
    """자리를 흩는 결정론적 값(-0.5~0.5).
    ★예전 판은 k에 큰 수를 곱해 더하기만 해서 16비트로 자르면 k=0과 k=1이 거의 같은 값이 나왔다
      — 그래서 세간 두 개가 한 자리에 겹쳐 떨어졌다. 이제 비트를 제대로 섞는다."""
    h = (i * 0x9E3779B1) ^ (0 if side == "L" else 0x85EBCA6B) ^ (k * 0xC2B2AE35)
    h &= 0xFFFFFFFF
    h ^= (h >> 15)
    h = (h * 0x2545F491) & 0xFFFFFFFF
    h ^= (h >> 13)
    return ((h & 0xFFFF) / 65535.0) - 0.5

DOORS_L = {2: ("L04_Dolgan_Office", "돌간의 흥신소"), 8: ("L02_Antique_Shop", "네사의 골동상")}
DOORS_R = {3: ("L05_Eatery", "조용한 밥집")}


# ── 집 조리법 — 층마다 [벽 널, (창·문 부재, 상대x, 상대z), ...] ─────────
# ★사용자 지시(2026-07-24): 통짜 파사드 금지. 낱개 부재를 쌓아 집을 만든다.
STOREY = 320.0
RECIPE = {
    "Home": [("Wall_DoorWin", [("Door_Unit", -105.0, 0.0), ("Win_Small", 105.0, 190.0)]),
             ("Wall_Win2", [("Win_Tall", -95.0, 180.0), ("Win_Unit_Lit", 95.0, 180.0)])],
    "Shop": [("Wall_Shop", [("Shop_Unit", 0.0, 0.0)]),
             ("Wall_Win", [("Win_Unit", 0.0, 180.0)])],
    "Blank": [("Wall_Plain", []),
              ("Wall_Win", [("Win_Unit", 0.0, 180.0)])],
    "Tall": [("Wall_DoorWin", [("Door_Unit", -105.0, 0.0), ("Win_Small", 105.0, 190.0)]),
             ("Wall_Win2", [("Win_Tall", -95.0, 180.0), ("Win_Tall", 95.0, 180.0)]),
             ("Wall_Win", [("Win_Unit", 0.0, 180.0)])],
    "Low": [("Wall_DoorWin", [("Door_Unit", -105.0, 0.0), ("Win_Small", 105.0, 190.0)])],
}
# 아래층에 뚫린 자리 — 기단을 여기엔 두르지 않는다(문·가게 목을 막으므로)
GROUND_GAP = {"Home": [(-105.0, 130.0)], "Tall": [(-105.0, 130.0)],
              "Low": [(-105.0, 130.0)], "Shop": [(0.0, 250.0)], "Blank": []}


def _house(i, side, v):
    """집 한 채를 낱개 부재로 쌓는다 — 벽 널·창·문·기단·띠·처마·덧붙임 전부 따로."""
    s = 1.0 if side == "L" else -1.0
    yaw = 0.0 if side == "L" else 180.0
    y = (FAC_Y + jitter(i, side) * 26.0) * s      # 벽이 조금씩 앞뒤로 어긋남
    face = y - s * (FT_HALF + 6.0)                 # 벽 바깥면 바로 앞
    x = xc(i)
    tag = "%s%d" % (side, i)

    storeys = RECIPE[v]
    h = STOREY * len(storeys)
    for k, (panel, units) in enumerate(storeys):
        z = STOREY * k
        place("SM_Rasel_%s" % panel, (x, y, z), yaw=yaw,
              label="벽_%s_%d층" % (tag, k + 1))
        for (unit, ux, uz) in units:
            place("SM_Rasel_%s" % unit, (x + ux * (1.0 if side == "L" else -1.0),
                                         y, z + uz), yaw=yaw,
                  label="%s_%s_%d층" % (unit.replace("SM_Rasel_", ""), tag, k + 1))
        if k > 0:
            place("SM_Rasel_StringCourse", (x, y, z - 4.0), yaw=yaw,
                  label="층띠_%s_%d" % (tag, k))

    # 기단 — 아래층에 뚫린 자리를 피해 토막으로 두른다
    gaps = sorted(GROUND_GAP[v])
    spans, cur = [], -MOD_HW
    for (gx, gw) in gaps:
        lo, hi = gx - gw / 2 - 8.0, gx + gw / 2 + 8.0
        if lo > cur:
            spans.append((cur, lo))
        cur = max(cur, hi)
    if cur < MOD_HW:
        spans.append((cur, MOD_HW))
    for n, (a, b) in enumerate(spans):
        if b - a < 24.0:
            continue
        place("SM_Rasel_Plinth", (x + (a + b) / 2 * (1.0 if side == "L" else -1.0), y, 0.0),
              yaw=yaw, label="기단_%s_%d" % (tag, n),
              scale=((b - a) / (MOD_HW * 2), 1.0, 1.0))

    # 벽보 거는 살 — ★뚫린 자리가 없는 민벽에만 (문을 가로지르던 결함 수정)
    if v == "Blank":
        place("SM_Rasel_WallRail", (x, face, 190.0), yaw=yaw, label="벽보살_%s" % tag)

    place("SM_Rasel_Eave", (x, y, h), yaw=yaw, label="처마_%s" % tag)

    r = jitter(i, side, 1)
    mx = 1.0 if side == "L" else -1.0      # ★오른쪽 줄은 좌우가 뒤집힌다. 덧붙임도 같이 뒤집어야
    #   문·창 자리와 어긋나지 않는다(항아리가 문 댓돌에 박히던 원인).

    # 홈통 — 집과 집이 만나는 이음매를 타고 내려온다.
    # ★기단(앞으로 8)·층띠(앞으로 6)보다 더 앞에 세워야 서로 안 파고든다.
    if r > -0.15:
        place("SM_Rasel_Downpipe", (x + 200.0 * mx, face - s * 10.0, 0.0), yaw=yaw,
              label="홈통_%s" % tag, scale=(1.0, 1.0, h / 600.0))
    # 굴뚝 — ★지붕마루에 앉힌다(전에는 벽 뒤 허공에 떠 반쯤 걸쳐 보였음)
    if r < 0.1:
        place("SM_Rasel_Chimney", (x + r * 90.0, y, h - 12.0),
              yaw=yaw, label="굴뚝_%s" % tag)
    # 덧문 — ★창 한 짝짜리 위층에만 단다.
    #   창이 둘인 집(Home·Tall)은 창턱까지 폭 146이라 덧문 놓을 자리가 안 나온다.
    if v in ("Shop", "Blank"):
        for sx, ry in ((-106.0, -28.0), (106.0, 28.0)):
            place("SM_Rasel_Shutter", (x + sx * mx, face, STOREY + 180.0),
                  yaw=yaw + ry, label="덧문_%s" % tag)
    # 간판 — 가게 목 위로 내민다
    if v == "Shop":
        place("SM_Rasel_ShopSign", (x + 150.0 * mx, face, 300.0), yaw=yaw,
              label="간판_%s" % tag)
        # ★차양 — 가게 목(z 0~240) 위 z=262에 앞으로 드리운다(통행 높이 위라 안 막음).
        #   집마다 붉은/푸른 천을 번갈아 걸어 리듬을 준다.
        awn = "A" if (i % 2 == 0) else "B"
        place("SM_Rasel_Awning_%s" % awn, (x, face, 262.0), yaw=yaw,
              label="차양_%s" % tag)
    # 문간 항아리 — 문(x=-105) 반대쪽 구석에
    if v in ("Home", "Low", "Tall"):
        put_ground("Pot", x + 128.0 * mx, face - s * 24.0, yaw, "항아리_%s" % tag)
    # 기대 세운 장대 — 뚫린 자리가 없는 민벽에만
    if v == "Blank" and r > 0.0:
        put_ground("Poles", x - 140.0 * mx, face - s * 16.0, yaw, "장대_%s" % tag)
    # 빨랫줄 — 층 사이(창턱 407, 덧문 425~575 아래)로
    if v in ("Home", "Tall") and jitter(i, side, 2) > 0.05:
        place("SM_Rasel_Laundry", (x, face - s * 60.0, 355.0), yaw=yaw,
              label="빨랫줄_%s" % tag)
    return 1


def build_facades():
    n = 0
    for i in range(COLS):
        if LEFT[i]:
            n += _house(i, "L", LEFT[i])
        if RIGHT[i]:
            n += _house(i, "R", RIGHT[i])
    # 우측 5번 칸 — 골목 어귀(→L03). 담이 안쪽으로 물러나며 좁아진다
    ax = xc(ALLEY_COL)
    for k in range(3):
        y = -FAC_Y - 200.0 - k * 400.0
        place("SM_Rasel_AlleyWall", (ax - 170.0, y, 0.0), yaw=90.0, label="AlleyWall_L%d" % k)
        place("SM_Rasel_AlleyWall", (ax + 170.0, y, 0.0), yaw=90.0, label="AlleyWall_R%d" % k)
    place("SM_Rasel_AlleyWall", (ax, -FAC_Y - 1500.0, 0.0), yaw=0.0, label="AlleyWall_End")
    # ★처마 등롱 — 가로등 없는 칸(1·4·7·10 제외)의 처마 밑에 매단다. 밤 거리를 채운다.
    #   처마 앞(면에서 s*80)·높이 z=560. 매다는 줄 원점이 위라 등이 아래로 드리운다.
    lc = 0
    for i in (2, 5, 8, 11):
        for side, row in (("L", LEFT), ("R", RIGHT)):
            if not row[i]:            # ★집(처마)이 없는 칸(골목 어귀 등)엔 매달 데가 없다
                continue
            s = 1.0 if side == "L" else -1.0
            place("SM_Rasel_HangLantern", (xc(i), (FAC_Y - 70.0) * s, 560.0),
                  yaw=0.0 if side == "L" else 180.0, label="처마 등롱_%s%d" % (side, i))
            lc += 1
    log("파사드 %d채 + 골목 담 7 + 처마 등롱 %d" % (n, lc))


# ── 좌판·세간 ─────────────────────────────────────────────────────────
STALLS = [(1, "L", "A"), (4, "R", "B"), (7, "L", "B"), (10, "R", "A"), (6, "R", "A")]


def build_props():
    for col, side, kind in STALLS:
        y = 330.0 if side == "L" else -330.0
        yaw = 0.0 if side == "L" else 180.0
        put_ground("Stall_%s" % kind, xc(col), y, yaw, "좌판_%s%d" % (side, col))
        put_ground("Crate", xc(col) + 156.0, y + (60 if side == "L" else -60),
                   18.0, "Crate_%d" % col)
        put_ground("Barrel", xc(col) - 130.0, y + (40 if side == "L" else -40),
                   0.0, "Barrel_%d" % col)

    for col in (0, 3, 6, 9, 11):
        put_ground("Lantern", xc(col) + 200.0, 470.0, 0.0, "가로등_L%d" % col)
        put_ground("Lantern", xc(col) + 200.0, -470.0, 0.0, "가로등_R%d" % col)

    put_ground("Well", xc(6), 0.0, 0.0, "우물")
    put_ground("Bench", xc(6) - 330.0, 120.0, 90.0, "벤치1")
    put_ground("Bench", xc(6) + 330.0, -120.0, 90.0, "벤치2")
    put_ground("NoticeBoard", xc(5), FAC_Y - 60.0, 0.0, "벽보판")
    # 현수막은 줄이 양쪽 벽에 걸려야 하므로 ★양쪽 다 640 이상인 칸에만 친다
    # (낮은 집 430 칸에 치면 지붕 위 허공에 매단 꼴이 된다).
    for col in (4, 9):
        place("SM_Rasel_Banner", (xc(col), 0.0, 470.0), yaw=90.0, label="현수막_%d" % col)
    # ※문 앞 계단은 놓지 않는다 — 파사드 메시에 댓돌(문턱 돌)이 이미 붙어 있고,
    #   그 위에 4단 계단을 또 놓으면 68cm 턱이 생겨 문을 막는다.
    log("좌판 %d · 가로등 10 · 우물·벤치2·벽보판·현수막2" % len(STALLS))


# ── 빛 — 해 진 직후 저잣거리 ───────────────────────────────────────────
# ── 하루 세 때 ────────────────────────────────────────────────────────
# 정본(기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md L01): "낮: 빽빽·상점 열림 / 밤: 셔터 반쯤·골목 입구 열림"
# 값 = (해 pitch, 해 yaw, 해 lux, 해 색, 하늘 세기, 안개 짙기, 안개 색, 가로등 lumen, 노출 보정)
PHASES = {
    # 낮 — 좁은 길이라 해를 낮게 두면 한쪽이 통째로 새까매진다. 해를 머리 위로 올리고
    #      하늘빛을 크게 넣어 그늘에서도 벽이 읽히게 한다.
    "day":     (-68.0, -20.0, 7.0, (1.0, 0.97, 0.92), 5.5, 0.005,
                (0.46, 0.50, 0.56), 0.0, -0.2),
    "evening": (-34.0, -62.0, 6.5, (1.0, 0.80, 0.56), 2.6, 0.008,
                (0.30, 0.25, 0.22), 2600.0, 0.6),
    "night":   (-14.0, 118.0, 0.35, (0.55, 0.62, 0.85), 0.9, 0.013,
                (0.10, 0.12, 0.18), 3200.0, 1.4),
}
PHASE = os.environ.get("RASEL_PHASE", "evening")
if PHASE not in PHASES:
    PHASE = "evening"


def build_clutter():
    """저잣거리 세간 — 사람이 산다는 표를 길 가장자리에 흩는다.
    한가운데(걷는 길)는 비워 두고, 벽 쪽 380~540 띠에만 놓는다."""
    KINDS = ["Sack", "Basket", "RopeCoil", "Bucket", "Stool", "Crate", "Barrel"]
    n = 0
    for i in range(COLS):
        for side in ("L", "R"):
            s = 1.0 if side == "L" else -1.0
            for k in range(2):
                r = jitter(i, side, 20 + k)
                if r < -0.12:                       # 다 채우면 지저분하다 — 듬성듬성
                    continue
                kind = KINDS[int(abs(r * 1000)) % len(KINDS)]
                dx = (jitter(i, side, 30 + k) * 300.0)
                dy = 400.0 + abs(jitter(i, side, 40 + k)) * 130.0
                x = xc(i) + dx
                if abs((x % TILE) - 200.0) < 40.0:  # 이음매의 홈통 자리는 피한다
                    x += 60.0
                # 자리가 막히면 몇 번 옆으로 옮겨 본다 — 그래야 듬성해지지 않는다
                yaw0 = jitter(i, side, 50 + k) * 300.0
                for (ox, oy) in ((0, 0), (95, 0), (-95, 0), (0, 70), (0, -70),
                                 (165, 45), (-165, -45)):
                    if put_ground(kind, x + ox, (dy + oy) * s, yaw0,
                                  "세간_%s%d_%d_%s" % (side, i, k, kind)):
                        n += 1
                        break
    # 손수레 한 대 — 실루엣이 커서 거리 중간쯤에 세워 둔다
    put_ground("Handcart", xc(8) + 60.0, 430.0, -14.0, "손수레")
    put_ground("Bucket", xc(6) - 150.0, 95.0, 20.0, "우물가 물통1")
    put_ground("Bucket", xc(6) + 128.0, -88.0, -35.0, "우물가 물통2")
    log("세간 %d(빈자리에만) + 손수레 + 물통" % n)


def build_lighting():
    cx = xc(COLS // 2)
    (p_pitch, p_yaw, p_lux, p_col, p_sky, p_fog, p_fogcol,
     p_lamp, p_exp) = PHASES[PHASE]
    dl = A().spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(cx, 0, 1600))
    # 해가 큰길을 비껴 훑도록 — 한쪽 파사드는 받고 반대쪽은 그늘, 바닥엔 긴 그림자
    dl.set_actor_rotation(unreal.Rotator(0.0, p_pitch, p_yaw), False)
    lc = dl.get_component_by_class(unreal.DirectionalLightComponent)
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_intensity(p_lux)
    lc.set_light_color(unreal.LinearColor(*p_col))
    lc.set_editor_property("atmosphere_sun_light", True)
    dl.set_actor_label("해 (%s)" % PHASE)

    # 보조광 — 해 반대쪽에서 약하게 채운다. 좁은 길이라 그늘진 벽이 통째로 새까매지는 것을
    # 막는다. 하늘빛(SkyLight 실시간 캡처)은 커맨들릿 촬영에서 안 잡히므로 이쪽이 더 믿을 만하다.
    if p_lux > 1.0:
        fl = A().spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(cx, 0, 1500))
        fl.set_actor_rotation(unreal.Rotator(0.0, -32.0, p_yaw + 180.0), False)
        flc = fl.get_component_by_class(unreal.DirectionalLightComponent)
        flc.set_mobility(unreal.ComponentMobility.MOVABLE)
        flc.set_intensity(p_lux * 0.30)
        flc.set_light_color(unreal.LinearColor(0.72, 0.80, 1.0))
        flc.set_editor_property("cast_shadows", False)
        flc.set_editor_property("atmosphere_sun_light", False)
        fl.set_actor_label("보조광 (%s)" % PHASE)

    A().spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(cx, 0, 0))
    fog = A().spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(cx, 0, 100))
    try:
        fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        fc.set_editor_property("fog_density", p_fog)
        fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(*p_fogcol))
        fc.set_editor_property("fog_height_falloff", 0.22)
    except Exception as e:
        log("  ! fog %s" % e)
    try:
        # 부피 안개 — 가로등 불이 공기 중에 빛줄기로 남는다(저녁 저잣거리 분위기의 핵심)
        for nm_ in ("volumetric_fog", "enable_volumetric_fog", "b_enable_volumetric_fog"):
            try:
                fc.set_editor_property(nm_, True)
                log("  · 부피 안개 켬(%s)" % nm_)
                break
            except Exception:
                continue
        fc.set_editor_property("volumetric_fog_extinction_scale", 1.4)
        fc.set_editor_property("volumetric_fog_distance", 6000.0)
        fc.set_editor_property("volumetric_fog_albedo", unreal.Color(210, 196, 180, 255))
    except Exception as e:
        log("  ! fog %s" % e)
    sky = A().spawn_actor_from_class(unreal.SkyLight, unreal.Vector(cx, 0, 900))
    try:
        skc = sky.get_component_by_class(unreal.SkyLightComponent)
        skc.set_mobility(unreal.ComponentMobility.MOVABLE)
        skc.set_editor_property("real_time_capture", True)
        skc.set_editor_property("intensity", p_sky)   # 그늘진 쪽이 새까맣지 않게 채우는 빛
    except Exception as e:
        log("  ! sky %s" % e)

    ppv = A().spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(cx, 0, 400))
    ppv.set_editor_property("unbound", True)
    s = ppv.get_editor_property("settings")
    # 옛 블록아웃은 단색 머티리얼이라 수동 노출(bias 9~10)을 박아 뒀지만,
    # 텍스처가 붙은 지금은 히스토그램 자동 노출이 맞다(게임에서도 이 값으로 돈다).
    s.set_editor_property("override_auto_exposure_method", True)
    s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    s.set_editor_property("override_auto_exposure_bias", True)
    s.set_editor_property("auto_exposure_bias", p_exp)
    s.set_editor_property("override_auto_exposure_min_brightness", True)
    s.set_editor_property("auto_exposure_min_brightness", 0.05)
    s.set_editor_property("override_auto_exposure_max_brightness", True)
    s.set_editor_property("auto_exposure_max_brightness", 2.2)
    try:
        s.set_editor_property("override_bloom_intensity", True)
        s.set_editor_property("bloom_intensity", 0.55)
    except Exception:
        pass
    ppv.set_editor_property("settings", s)
    ppv.set_actor_label("노출·색")

    # 가로등 불 — 부재의 등피 자리에 맞춰 z=372
    n = 0
    for col in (0, 3, 6, 9, 11):
        for y in (470.0, -470.0):
            pl = A().spawn_actor_from_class(unreal.PointLight,
                                            unreal.Vector(xc(col) + 200.0, y, 372.0))
            c = pl.get_component_by_class(unreal.PointLightComponent)
            c.set_mobility(unreal.ComponentMobility.MOVABLE)
            c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
            c.set_intensity(p_lamp)
            c.set_attenuation_radius(880.0)
            c.set_light_color(unreal.LinearColor(1.0, 0.71, 0.4))
            c.set_editor_property("cast_shadows", True)
            c.set_editor_property("source_radius", 6.0)          # 그림자 가장자리가 부드럽게
            c.set_editor_property("volumetric_scattering_intensity", 2.6)
            pl.set_actor_label("가로등불 %d %s" % (col, "L" if y > 0 else "R"))
            n += 1
    # 사람 사는 창에서 새는 불 — Home 파사드 위층 왼쪽 창
    for i, v in enumerate(LEFT):
        if v == "Home":
            pl = A().spawn_actor_from_class(unreal.PointLight,
                                            unreal.Vector(xc(i) + 95.0, FAC_Y - 60.0, 500.0))
            c = pl.get_component_by_class(unreal.PointLightComponent)
            c.set_mobility(unreal.ComponentMobility.MOVABLE)
            c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
            c.set_intensity(900.0)
            c.set_attenuation_radius(420.0)
            c.set_light_color(unreal.LinearColor(1.0, 0.78, 0.5))
            pl.set_actor_label("창불 L%d" % i)
            n += 1
    # 처마 등롱의 불 — 낮엔 약하게, 저녁·밤엔 따뜻하게(등롱 속 z≈460)
    if p_lamp > 0.0:
        for i in (2, 5, 8, 11):
            for side, row in (("L", LEFT), ("R", RIGHT)):
                if not row[i]:
                    continue
                s = 1.0 if side == "L" else -1.0
                pl = A().spawn_actor_from_class(unreal.PointLight,
                                                unreal.Vector(xc(i), (FAC_Y - 70.0) * s, 460.0))
                c = pl.get_component_by_class(unreal.PointLightComponent)
                c.set_mobility(unreal.ComponentMobility.MOVABLE)
                c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
                c.set_intensity(p_lamp * 0.5)
                c.set_attenuation_radius(520.0)
                c.set_light_color(unreal.LinearColor(1.0, 0.72, 0.42))
                c.set_editor_property("volumetric_scattering_intensity", 2.2)
                pl.set_actor_label("등롱불_%s%d" % (side, i))
                n += 1
    log("빛: 해·하늘·안개 + 점광 %d" % n)


# ── 배선 — 나가는 곳·짚을 것·사는 사람 (StoryFlag 축 포함) ──────────────
def cls(name):
    c = getattr(unreal, name, None)
    return c if c is not None else unreal.load_class(None, "/Script/Secret_Project." + name)


def setp(a, k, v):
    try:
        a.set_editor_property(k, v)
    except Exception as e:
        log("  ! set %s = %s" % (k, e))


def wire():
    Portal, Lore, NPC = cls("PortalActor"), cls("LoreNoteActor"), cls("ANPCCharacter")

    def portal(loc, tgt, label, req=None, forb=None):
        a = A().spawn_actor_from_class(Portal, unreal.Vector(*loc))
        setp(a, "TargetLevelName", tgt)
        if req:
            setp(a, "RequiredFlag", req)
        if forb:
            setp(a, "ForbiddenFlag", forb)
        a.set_actor_label(label)

    for i, (tgt, nm) in DOORS_L.items():
        if tgt == "L02_Antique_Shop":
            # 사건 전엔 여는 문, 셋째날 소문 뒤엔 탄 뒷방 쪽으로 갈린다(§10-B)
            portal((xc(i), FAC_Y - 150.0, 60.0), tgt, "→ %s (사건 전)" % nm,
                   forb="op_nesa_dead")
            # 사건 뒤엔 같은 문이 '탄 뒷방' 맵으로 간다(두 얼굴을 맵 둘로 나눔)
            portal((xc(i), FAC_Y - 150.0, 60.0), "L02_Antique_Shop_Burnt",
                   "→ %s (탄 뒤)" % nm, req="op_nesa_dead")
        else:
            portal((xc(i), FAC_Y - 150.0, 60.0), tgt, "→ %s" % nm)
    for i, (tgt, nm) in DOORS_R.items():
        portal((xc(i), -FAC_Y + 150.0, 60.0), tgt, "→ %s" % nm)
    portal((xc(ALLEY_COL), -FAC_Y - 120.0, 60.0), "L03_Backalley", "→ 장터 뒷골목")

    def lore(loc, title, body, req=None, forb=None, yaw=0.0):
        a = A().spawn_actor_from_class(Lore, unreal.Vector(*loc),
                                       unreal.Rotator(0.0, 0.0, yaw))
        setp(a, "Title", title)
        setp(a, "Lines", body)
        if req:
            setp(a, "RequiredFlag", req)
        if forb:
            setp(a, "ForbiddenFlag", forb)
        a.set_actor_label("조사: " + title)

    bx, by = xc(5), FAC_Y - 130.0
    lore((bx, by, 150.0), "벽보판",
         ["삯일과 소문이 같은 판에 붙는다.",
          "신항 굴착에서 지하 열여덟 자 아래 옛 석축이 나왔다는 말.",
          "사람 구하는 쪽지가 석 장 겹쳐 붙어 있다."], req="op_day1")
    lore((bx - 60.0, by, 150.0), "벽보판 — 탄 가게",
         ["아랫장터 골동상에 불이 났다는 쪽지.",
          "관은 새어 든 연기에 의한 사고사로 접수했다고 적혀 있다."], req="op_day3")
    lore((bx + 60.0, by, 150.0), "벽보판 — 겹치는 부고",
         ["같은 달에 부고가 넉 장. 넉 장 다 사고사.",
          "그림을 만진 자들이라는 말이 아래에 연필로 덧적혀 있다."], req="op_day3")
    lore((xc(9), -170.0, 30.0), "포장이 깨진 자리",
         ["벽돌 포장이 한 뼘 깨져 그 아래가 드러났다.",
          "밑에 깔린 것은 이 도시가 깔아 둔 돌이 아니다. 이음매가 너무 곱다."])
    lore((xc(6) + 120.0, 0.0, 90.0), "우물",
         ["구시가에서 아직 물이 나오는 우물.",
          "두레박 줄이 새것이다. 아직 쓰는 사람이 있다."])

    DayEnum = getattr(unreal, "DayPhase", None)
    day = getattr(DayEnum, "DAY", None) if DayEnum else None
    night = getattr(DayEnum, "NIGHT", None) if DayEnum else None
    specs = [
        (xc(1), 260.0, 180.0, "잡화 좌판", ["오늘 물건은 좋아. 골라 봐.",
                                        "밤엔 셔터 내리니까 낮에 와."], day, "convenience"),
        (xc(4), -260.0, 0.0, "약초 좌판", ["의원 것보다 싸. 효험은 봐야 알고."], day, "convenience"),
        (xc(7), 260.0, 180.0, "골동 중개", ["안쪽에 진짜가 있어. 감정도 해 주고.",
                                        "삭지 않은 물건은 값을 함부로 못 매겨."], day, None),
        (xc(10), -260.0, 0.0, "뱃말 뜨내기", ["뱃말 섞어 쓰는 자들이 요즘 부쩍 늘었어.",
                                          "신항 쪽에서 왔다더군."], night, None),
    ]
    m = 0
    for x, y, yaw, role, dl_, phase, shop in specs:
        a = A().spawn_actor_from_class(NPC, unreal.Vector(x, y, 100.0),
                                       unreal.Rotator(0.0, 0.0, yaw))
        setp(a, "NPCName", role)
        setp(a, "DialogueLines", dl_)
        setp(a, "bCanEnterCombat", False)
        if shop:
            setp(a, "bIsShopkeeper", True)
            setp(a, "ShopKind", shop)
            setp(a, "bClosedAtNight", True)
        if phase is not None:
            setp(a, "ActivePhases", [phase])
        a.set_actor_label("NPC " + role)
        m += 1
    log("배선: 포탈5 · 조사 5 · NPC %d" % m)


def build():
    global KIT
    with open(MANIFEST, "r", encoding="utf-8") as f:
        KIT = json.load(f)
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    # ★★반드시 지켜야 할 가드 — `new_level`이 조용히 실패하면 이어지는 스폰이 전부
    #   '지금 열려 있는 맵'(시작 화면 Title)에 쏟아진다. 실제로 Title이 1,848 액터까지
    #   불어난 사고가 있었다(2026-07-24). 그래서 ①먼저 딴 레벨로 빠져나가 대상 맵을
    #   내린 뒤 지우고 ②새로 만들고 ③월드 이름이 맞는지 확인해서 아니면 즉시 멈춘다.
    # 대상 맵 위에 바로 new_level 하는 것은 이미 있는 에셋이라 실패한다.
    # → 빈 임시 레벨에 지어 놓고 마지막에 대상 경로로 저장(다른 이름으로 저장)한다.
    # ★맵 다루기 — 여기서 한 번 크게 데였다(2026-07-24). 정리해 두면:
    #   · `new_level(경로)`는 그 경로에 에셋이 있으면 **실패하고, 조용히 옛 월드에 머문다**
    #     → 이어지는 스폰이 전부 시작 화면 맵(Title)에 쏟아져 1,848 액터까지 불어났다.
    #   · `save_map(월드, 다른경로)`도 `rename_asset`도 이 판에선 안 먹는다.
    #   → 그래서 **대상 맵을 열어서 비우고 다시 짓는다.** 열린 월드 이름을 반드시 확인한다.
    # ★에디터가 그 맵을 열어 두면 .umap 파일이 잠겨서 헤드리스 저장이 전부 실패한다.
    #   (사용자가 검수하러 에디터를 켜 두는 일이 잦다.) 잠겨 있으면 옆 이름으로 짓고,
    #   나중에 풀렸을 때 갈아 끼운다. 그래야 루프가 멈추지 않는다.
    global MAP
    disk = ("C:/Secret_Project/Content" + MAP[len("/Game"):] + ".umap")
    locked = False
    if os.path.exists(disk):
        try:
            with open(disk, "r+b"):
                pass
        except Exception:
            locked = True
    if locked:
        MAP = MAP + "_next"
        log("★대상 맵이 에디터에 열려 잠김 → 대신 %s 에 짓는다(나중에 갈아 끼움)" % MAP)

    want = MAP.rsplit("/", 1)[1]
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        les.load_level(MAP)
    else:
        les.new_level(MAP)
    got = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_name()
    if got != want:
        raise RuntimeError(
            "맵 열기 실패 — 열린 월드가 '%s'라서 여기에 스폰하면 그 맵을 망친다. 중단." % got)
    old = list(A().get_all_level_actors())
    for a in old:
        A().destroy_actor(a)
    log("%s 열어서 옛 액터 %d개 비움" % (got, len(old)))
    reserve_doorways()
    build_ground()
    build_facades()
    build_props()
    build_clutter()
    build_lighting()
    A().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-260.0, 0.0, 120.0))
    wire()
    # ★저장 검증 — save_current_level()이 조용히 실패해 옛 맵이 그대로 남는 사고가 있었다.
    EAL = unreal.EditorAssetLibrary
    n = 0
    for a in A().get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor):
            n += 1
    log("현재 월드: %s · 부재 액터 %d개" % (
        unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        .get_editor_world().get_name(), n))

    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    ok = les.save_current_level()
    log("저장(save_current_level) -> %s" % ok)
    if not ok:
        try:
            ok = unreal.EditorLoadingAndSavingUtils.save_map(w, MAP)
            log("저장(save_map) -> %s" % ok)
        except Exception as e:
            log("  ! save_map %s" % e)
    if not ok:
        try:
            ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
            log("저장(save_dirty_packages) -> %s" % ok)
        except Exception as e:
            log("  ! save_dirty_packages %s" % e)
    if not ok:
        raise RuntimeError("맵 저장 실패 — 디스크에 안 쓰였다. 중단.")
    for junk in ("/Game/Maps/Rasel/_scratch", "/Game/Maps/Rasel/_park"):
        if EAL.does_asset_exist(junk):
            EAL.delete_asset(junk)
    log("=== L01 v2 저장 완료 · 부재 액터 %d개 ===" % n)


try:
    build()
except Exception:
    log("실패\n" + traceback.format_exc())
flush()
