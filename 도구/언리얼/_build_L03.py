# -*- coding: utf-8 -*-
"""L03 장터 뒷골목 — 조합식으로 짓는다(키트 재사용 검증판).

근거: 기획/02_게임설계/2_레벨/게임_레벨디자인_상세.md L03 · 기획/01_세계관/3_현대/세계관_현대_이야기.md §5-1·§13
핵심 = **로한의 진(陣)**. 하루 먼저 그은 자가 골목을 닫는다. 급히 그은 한 귀퉁이만
이가 덜 맞물렸고, 아이린이 거기를 짚어 환기구로 몸을 던졌다.

평면(ㄱ자): 큰길 어귀(x=0)에서 x축으로 1400 → 꺾여 -Y로 900. 폭 300, 담 높이 640.
실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=".../_build_L03.py"
"""
import sys, math, traceback
sys.path.append("C:/Secret_Project/도구/언리얼")
import unreal
import _rasel_common as C

MAP = "/Game/Maps/Rasel/L03_Backalley"
W = 300.0            # 골목 폭
LEG_A = 1400.0       # 어귀에서 꺾임까지
LEG_B = 900.0        # 꺾인 뒤 안쪽 끝까지
BEND_X = 1200.0      # 꺾임 중심
TILE = 400.0


def floor_and_walls():
    """바닥 판과 담벼락 — 담은 400폭 부재를 이어 붙이고, 위층 높이(640)로 하늘을 띠로 만든다."""
    n = 0
    # ── 첫 다리(어귀→꺾임): x 0~1400, y -150~150
    for i in range(4):
        x = 200.0 + i * TILE
        C.place("SM_Rasel_AlleyFloor", (x, 0.0, 0.0), label="골목바닥_A%d" % i)
        C.place("SM_Rasel_AlleyWall", (x, W / 2 + 13.0, 0.0), yaw=0.0,
                label="담_북_A%d" % i)
        C.place("SM_Rasel_AlleyWall", (x, -W / 2 - 13.0, 0.0), yaw=180.0,
                label="담_남_A%d" % i)
        n += 3
    # ── 두 번째 다리(꺾임→안쪽 끝): x 1050~1350, y -150~-1050
    for j in range(3):
        y = -350.0 - j * TILE
        C.place("SM_Rasel_AlleyFloor", (BEND_X, y, 0.0), label="골목바닥_B%d" % j)
        C.place("SM_Rasel_AlleyWall", (BEND_X - W / 2 - 13.0, y, 0.0), yaw=90.0,
                label="담_서_B%d" % j)
        C.place("SM_Rasel_AlleyWall", (BEND_X + W / 2 + 13.0, y, 0.0), yaw=270.0,
                label="담_동_B%d" % j)
        n += 3
    # 꺾임 바깥 모서리를 막는 담 + 안쪽 막다른 끝
    C.place("SM_Rasel_AlleyWall", (1400.0 + 13.0, 0.0, 0.0), yaw=270.0, label="담_꺾임끝")
    C.place("SM_Rasel_AlleyWall", (BEND_X, -1250.0 - 13.0, 0.0), yaw=180.0, label="담_막다른끝")
    n += 2
    C.log("바닥·담 %d장" % n)


def the_ward():
    """★진의 새김 — 어귀를 가로질러 그은 선. 이것이 골목을 닫는다.
    가운데 고리 하나 + 가로지르는 획 여럿 + 매듭점. 낱개 부재라 한 획씩 지울 수 있다."""
    x0 = 320.0
    C.place("SM_Rasel_RuneRing", (x0, 0.0, 0.5), label="진_고리")
    # 골목을 가로막는 획 — 폭 300을 200짜리 선 두 개로 잇는다
    for k, dy in enumerate((-75.0, 75.0)):
        C.place("SM_Rasel_RuneLine", (x0, dy, 0.5), yaw=90.0, label="진_가로획%d" % k)
    # 고리에서 담으로 뻗는 획 넷
    for k, ang in enumerate((35.0, 145.0, 215.0, 325.0)):
        a = math.radians(ang)
        C.place("SM_Rasel_RuneLine", (x0 + math.cos(a) * 150.0,
                                      math.sin(a) * 105.0, 0.5),
                yaw=ang, label="진_뻗은획%d" % k)
    for k, dy in enumerate((-120.0, 0.0, 120.0)):
        C.place("SM_Rasel_RuneNode", (x0 + 190.0, dy, 0.6), label="진_매듭%d" % k)
    # 어귀 쪽 촘촘한 겹획 — "하루 먼저 그은" 쪽이라 선이 배다
    for k in range(3):
        C.place("SM_Rasel_RuneLine", (x0 - 60.0 - k * 46.0, 0.0, 0.5), yaw=90.0,
                label="진_겹획%d" % k)
    C.log("진의 새김 13획(어귀)")


def the_gap():
    """★덜 맞물린 귀퉁이 — 급히 그은 자리. 선이 흐리고 한 군데가 끊겨 있다.
    꺾임 바깥 모서리(아이린이 냄새로 짚은 자리)."""
    gx, gy = BEND_X + 60.0, -230.0
    for k, (dx, dy, yw) in enumerate(((0, 0, 0.0), (-150, -40, 62.0), (120, -70, 118.0))):
        C.place("SM_Rasel_RuneLine_Faint", (gx + dx, gy + dy, 0.5), yaw=yw,
                label="덜맞물린획%d" % k)
    C.place("SM_Rasel_RuneNode", (gx - 20.0, gy - 120.0, 0.6), label="끊긴 매듭")
    C.log("덜 맞물린 귀퉁이 4획")


def props_and_wiring():
    # ★젖은 골목 — 물웅덩이가 등불·진의 빛을 되비친다(길바닥 마모·물웅덩이).
    #   걷는 한가운데를 피해 벽 쪽·꺾임 안쪽에 얕게 앉힌다.
    for (px, py, kind) in ((520.0, -60.0, "Puddle_M"), (980.0, 70.0, "Puddle_S"),
                           (BEND_X + 40.0, -430.0, "Puddle_L"),
                           (BEND_X - 60.0, -760.0, "Puddle_S"),
                           (320.0, 60.0, "Puddle_S")):
        C.place("SM_Rasel_%s" % kind, (px, py, 1.0), label="물웅덩이_%s" % kind)
    # 바큇자국 — 어귀에서 안쪽으로 두 줄
    for k in range(3):
        C.place("SM_Rasel_WheelRuts", (300.0 + k * 400.0, -20.0, 0.5),
                label="바큇자국%d" % k)

    # 골목이라 세간은 적게 — 벽에 기댄 것 몇, 버려진 것 몇
    C.put_ground("Crate", 620.0, 95.0, 12.0, "궤")
    C.put_ground("Barrel", 900.0, -100.0, 0.0, "통")
    C.put_ground("Poles", 1080.0, 96.0, 0.0, "장대")
    C.put_ground("RopeCoil", BEND_X - 90.0, -560.0, 0.0, "밧줄 사리")
    C.put_ground("Basket", BEND_X + 88.0, -880.0, 40.0, "광주리")

    # 환기구 맨홀 — 아래층(폐선 승강장)으로
    C.put_ground("Manhole", BEND_X, -1080.0, 0.0, "환기구 맨홀")
    C.portal((BEND_X, -1080.0, 40.0), "L19_Abandoned_Platform", "→ 폐선 승강장(환기구)")
    C.portal((-60.0, 0.0, 60.0), "L01_Jangteo_Street", "→ 장터 큰길")

    C.lore((320.0, 0.0, 40.0), "담벼락의 진",
           ["바닥에 그은 선이 골목을 가로질러 닫혀 있다.",
            "하루 먼저 그은 자가 골목을 닫는다. 자리와 시간을 먼저 차지한 쪽이 이긴다.",
            "급히 그은 자리는 이가 안 맞는다 — 그 자리만 찾으면 된다."])
    C.lore((BEND_X + 40.0, -230.0, 40.0), "이가 덜 맞물린 귀퉁이",
           ["여기만 선이 끊겼다. 획의 폭이 앞의 것과 다르다.",
            "급히 그은 손이다. 아이린은 이 자리를 냄새로 짚었다."])
    C.lore((BEND_X, -1080.0, 40.0), "환기구 맨홀",
           ["쇠 뚜껑 아래로 바람이 올라온다. 아래는 폐선 승강장.",
            "뚜껑 가장자리에 최근에 긁힌 자국이 있다."])

    gate = C.A().spawn_actor_from_class(C.cls("LockedGateActor"),
                                        unreal.Vector(BEND_X, -1230.0, 60.0))
    C.setp(gate, "GateId", "L03_BackDoor")
    gate.set_actor_label("잠긴 뒷문 (다른 길: 맨홀)")

    NIGHT = getattr(getattr(unreal, "DayPhase", None), "NIGHT", None)
    C.npc((760.0, -70.0, 100.0), "밤 중개인",
          ["낮엔 여기 아무것도 없어. 밤에만 서는 거래가 있지.",
           "바닥 조심해. 저 선은 어제 그은 게 아니야."],
          yaw=180.0, phase=NIGHT)
    C.log("배선: 포탈2 · 조사3 · 잠긴문1 · NPC1")


def lighting():
    phase = C.day_phase("night")          # 골목은 밤이 본판
    C.sky_and_exposure((BEND_X * 0.5, -300.0), phase, exposure_bias=2.0)
    # 진의 새김이 바닥에서 올라오는 청백 주광원
    C.point_light((320.0, 0.0, 55.0), 2900.0, (0.36, 0.72, 1.0), 760.0,
                  "진의 빛(어귀)", volumetric=3.4, source_radius=40.0)
    C.point_light((BEND_X + 40.0, -230.0, 45.0), 900.0, (0.34, 0.68, 0.95), 520.0,
                  "진의 빛(덜 맞물린 자리)", volumetric=2.0, source_radius=30.0)
    # 큰길에서 새어 드는 가로등 주황빛 — 청백과 대비를 만든다
    C.point_light((-120.0, 40.0, 330.0), 3000.0, (1.0, 0.70, 0.38), 950.0,
                  "큰길 가로등 새어듦", volumetric=2.6)
    # 막다른 끝 문 위 등 하나
    C.point_light((BEND_X, -1190.0, 250.0), 1300.0, (1.0, 0.78, 0.5), 560.0,
                  "뒷문 등", volumetric=1.6)
    C.log("빛: %s + 진의 빛2 + 새어듦1 + 뒷문등1" % phase)


def build():
    C.begin(MAP, "l03")
    floor_and_walls()
    the_ward()
    the_gap()
    props_and_wiring()
    lighting()
    C.A().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(60.0, 0.0, 120.0))
    C.finish_level()


try:
    build()
except Exception:
    C.log("실패\n" + traceback.format_exc())
    C.flush()
