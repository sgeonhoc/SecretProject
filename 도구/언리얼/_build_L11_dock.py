# -*- coding: utf-8 -*-
"""L11 부두 하역장 — 신규.

근거: 기획/02_게임설계/2_레벨/게임_레벨디자인_상세.md L11 · 기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md L11 · 현대 §16·§19.
부두는 위층이라 아무도 능력을 안 쓴다. 여기서 벌어지는 것은 짐수레를 두고 미는
**흔한 하역 다툼** — 사람 수와 삯의 셈이다.

**진행은 사람이 한다** — 하역꾼·선원·손이 고운 사내에게 말을 걸어야 이야기가 움직인다.

공간: 부두 널 3200×2000 · 컨테이너 열(엄폐) · 크레인 · 계류된 화물선(트랩 → L20) · 밤바다.
실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript="C:/Secret_Project/_build_L11_dock.py"
"""
import sys, traceback
sys.path.append("C:/Secret_Project/도구/언리얼")
import unreal
import _rasel_common as C

MAP = "/Game/Maps/Rasel/L11_Dock_Wharf"

MP = lambda n: "/Game/Rasel/Materials/%s" % n
WOOD = MP("M_Rasel_Wood")
STONE = MP("M_Rasel_Stone")
DARK = MP("M_Rasel_Dark")
IRON = MP("M_Rasel_Iron")
RIVER = MP("M_Rasel_River")
CLOTH_A = MP("M_Rasel_ClothA")
CLOTH_B = MP("M_Rasel_ClothB")


def deck():
    """부두 널 — 안쪽은 콘크리트, 물가 쪽 두 줄은 나무 널."""
    n = 0
    for ix in range(8):           # x: 0 .. 2800
        x = ix * 400.0
        for iy in range(5):       # y: -800 .. 800
            y = -800.0 + iy * 400.0
            mat = WOOD if y >= 400.0 else STONE
            C.place("SM_Rasel_RoadTile", (x, y, 0.0), label="부두 바닥", mats=[mat])
            n += 1
    # 물가 계선주 대신 통을 줄지어 — 밧줄이 걸린 자리
    for ix in range(6):
        C.place("SM_Rasel_Barrel", (200.0 + ix * 480.0, 760.0, 0.0), label="계선 통")
        C.place("SM_Rasel_RopeCoil", (280.0 + ix * 480.0, 700.0, 0.0), label="사린 밧줄")
        n += 2
    C.log("부두 바닥 %d" % n)


def sea_and_ship():
    """밤바다와 계류된 화물선. 배 위(L20)로는 트랩으로 오른다."""
    # 바다 — 널 판을 크게 늘려 깔고 강물 재료를 입힌다(밤엔 남색으로 가라앉는다)
    for ix in range(4):
        C.place("SM_Rasel_RoadTile", (ix * 1200.0, 2000.0, -20.0), label="밤바다",
                scale=(3.0, 3.0, 1.0), mats=[RIVER])
    # 선체 — 어두운 덩치 하나. 갑판 난간은 벽 난간을 얹어 표시한다.
    C.place("SM_Rasel_Crate", (1400.0, 1300.0, 0.0), label="화물선 선체",
            scale=(9.0, 3.4, 3.2), mats=[DARK])
    for k in range(7):
        C.place("SM_Rasel_WallRail", (500.0 + k * 300.0, 1120.0, 330.0), yaw=0.0,
                label="갑판 난간", mats=[IRON])
    # 트랩(건널 판) — 배로 오르는 자리
    C.place("SM_Rasel_Steps", (1400.0, 980.0, 0.0), yaw=0.0, label="배로 오르는 트랩")
    C.portal((1400.0, 1060.0, 80.0), "L20_Cargo_Ship", "→ 화물선")
    C.log("바다 4 · 선체 1 · 난간 7 · 트랩 1")


def containers_and_crane():
    """컨테이너 열 — 엄폐가 되고, 사이가 어두워 실랑이가 여기서 난다."""
    n = 0
    rows = ((600.0, -520.0, CLOTH_A), (600.0, -160.0, DARK),
            (1500.0, -520.0, CLOTH_B), (1500.0, -160.0, CLOTH_A),
            (2300.0, -520.0, DARK), (2300.0, -160.0, CLOTH_B))
    for (cx, cy, mat) in rows:
        C.place("SM_Rasel_Crate", (cx, cy, 0.0), label="컨테이너",
                scale=(4.2, 2.2, 2.4), mats=[mat])
        n += 1
    # 위에 하나씩 더 얹은 줄(그늘이 깊어진다)
    for (cx, cy, mat) in ((600.0, -520.0, DARK), (2300.0, -160.0, CLOTH_A)):
        C.place("SM_Rasel_Crate", (cx, cy, 230.0), label="얹은 컨테이너",
                scale=(4.2, 2.2, 2.4), mats=[mat])
        n += 1
    # 크레인 — 기둥 둘과 가로보 하나
    for px in (2650.0, 2650.0):
        pass
    C.place("SM_Rasel_Poles", (2620.0, 300.0, 0.0), label="크레인 기둥", scale=(1.6, 1.6, 6.0), mats=[IRON])
    C.place("SM_Rasel_Poles", (2620.0, 700.0, 0.0), label="크레인 기둥", scale=(1.6, 1.6, 6.0), mats=[IRON])
    C.place("SM_Rasel_WallRail", (2620.0, 500.0, 700.0), yaw=90.0, label="크레인 가로보",
            scale=(4.0, 1.0, 1.6), mats=[IRON])
    C.log("컨테이너 %d · 크레인 3" % n)


def things():
    """★짐수레 — 이 부두의 싸움은 여기서 붙는다. 능력이 아니라 사람 수로."""
    C.place("SM_Rasel_Handcart", (1150.0, -60.0, 0.0), yaw=20.0, label="★짐수레")
    C.place("SM_Rasel_Crate", (1150.0, -60.0, 96.0), label="짐수레 위 궤", mats=[DARK])
    C.lore((1150.0, -60.0, 150.0), "짐수레",
           ["궤 하나가 실려 있다. 겉에 상호도 이름도 없다.",
            "손잡이가 양쪽으로 나 있다 — 미는 쪽과 당기는 쪽이 다를 수 있게.",
            "여기선 아무도 손에 불을 세우지 않는다. 세우는 순간 온 도시가 눈을 뜬다."])

    C.place("SM_Rasel_Counter", (2500.0, -700.0, 0.0), yaw=90.0, label="하역 사무 탁자")
    C.place("SM_Rasel_PaperStack", (2500.0, -740.0, 96.0), label="하역 명부")
    C.lore((2500.0, -740.0, 120.0), "하역 명부",
           ["배 이름·짐 수·시각이 줄줄이 적혀 있다.",
            "오늘 밤 칸만 필체가 다르다. 받는 이 자리에 이름 대신 표식 하나."])

    C.place("SM_Rasel_Bench", (300.0, -740.0, 0.0), label="쉬는 자리")
    C.place("SM_Rasel_Sack", (1800.0, -700.0, 0.0), label="가마니")
    C.place("SM_Rasel_Sack", (1880.0, -720.0, 0.0), yaw=30.0, label="가마니")
    C.place("SM_Rasel_Basket", (700.0, -740.0, 0.0), label="바구니")

    # 나가는 문 — 창고(L12)와 도시 안쪽
    C.portal((-120.0, -200.0, 80.0), "L12_Warehouse", "→ 부두 창고")
    C.portal((-120.0, 400.0, 80.0), "L09_Newport_Site", "→ 신항 공사판")
    C.log("짐수레·명부·세간 · 나가는 문 2")


def people():
    C.npc((1000.0, -160.0, 0.0), "하역꾼 조장",
          ["이 시각에 부두에 있을 사람은 아닌데.",
           "저 궤? 우리 짐이 아니야. 우린 밀라니까 미는 거고, 삯은 저쪽에서 나온다.",
           "여기선 손에 뭐 세우지 마라. 그러면 다들 곤란해진다. 위층이 눈을 뜨거든."],
          yaw=180.0)
    C.npc((1700.0, 500.0, 0.0), "선원",
          ["배는 물때 맞춰 나간다. 궤가 실리면 바로.",
           "바다 위에선 자네들이 하는 그런 게 하나도 안 선다더군. 나야 잘 모르지만.",
           "짐이 뭔지는 안 묻는다. 묻는 사람은 다음 배를 못 탄다."],
          yaw=250.0)
    C.npc((1300.0, 300.0, 0.0), "손이 고운 사내",
          ["…나는 여기 사람 아니오. 짐 세는 걸 도우러 왔소.",
           "손? 원래 이렇소. 굳은살은 사람마다 다르지 않소."],
          yaw=200.0)
    C.npc((2400.0, -300.0, 0.0), "밤에 오르는 자",
          ["삯이 두 배라기에 왔소. 조장은 모르는 일이오.",
           "짐 하나 미는 데 사람이 왜 이리 많은가 싶긴 하더군."],
          yaw=160.0)
    C.log("사람 넷 — 조장·선원·손 고운 사내·밤에 오르는 자")


def lighting():
    C.sky_and_exposure((1400.0, 0.0), "night", exposure_bias=1.7)
    # 부두 조명등 — 찬 흰빛 넷. 컨테이너 사이는 일부러 비운다(그늘이 깊게).
    for (lx, ly) in ((300.0, -820.0), (1400.0, -820.0), (2500.0, -820.0), (1400.0, 820.0)):
        C.place("SM_Rasel_Lantern", (lx, ly, 0.0), label="부두 조명등", scale=(1.0, 1.0, 2.2))
        C.point_light((lx, ly, 420.0), 5200.0, (0.86, 0.92, 1.0), 1500.0, "부두 조명")
    C.log("밤 조명 — 조명등 4(찬 흰)")


def main():
    C.begin(MAP, "l11_dock")
    deck()
    sea_and_ship()
    containers_and_crane()
    things()
    people()
    lighting()
    ps = C.A().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, -200.0, 120.0))
    ps.set_actor_label("PlayerStart")
    C.finish_level()


try:
    main()
except Exception:
    unreal.log_error(traceback.format_exc())
    raise
