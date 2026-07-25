# -*- coding: utf-8 -*-
"""L12 부두 창고 (요아의 낮 일터) — 신규.

근거: 기획/02_게임설계/2_레벨/게임_레벨디자인_상세.md L12 · 기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md L12 · 현대 §3-5·§6-1·§22-2.
낮에 짐을 지는 자리. 능력을 쓴 적도 없는데 몸이 되레 가볍다는 것을 여기서 처음 안다.
뒤엔 창고 앞에 낯선 자가 서 있기 시작하는 감시의 자리가 된다.

**진행은 사람이 한다** — 여기 서 있는 감독·인부에게 말을 걸어야 이야기가 움직인다.
장면이 저절로 재생되지 않는다.

공간: 창고 안 1600×800(높은 천장 640·위층 채광창) + 하역문 밖 부두 널.
실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript="C:/Secret_Project/_build_L12_warehouse.py"
"""
import sys, traceback
sys.path.append("C:/Secret_Project")
import unreal
import _rasel_common as C

MAP = "/Game/Maps/Rasel/L12_Warehouse"
HW, HD = 800.0, 400.0        # 창고 안 절반 크기(가로 1600 · 세로 800)
ROW = 320.0                  # 벽 널 한 단 높이
CEIL = 640.0                 # 높은 천장(두 단)

MP = lambda n: "/Game/Rasel/Materials/%s" % n
PLASTER = MP("M_Rasel_Plaster")
PLASTER_IN = MP("M_Rasel_PlasterIn")
STONE = MP("M_Rasel_Stone")
WOOD = MP("M_Rasel_Wood")
DARK = MP("M_Rasel_Dark")
FLOORW = MP("M_Rasel_FloorWood")


def shell():
    """바닥·천장·벽 두 단. 창고라 벽은 회벽, 바닥은 콘크리트(돌)."""
    for gx in (-600.0, -200.0, 200.0, 600.0):
        for gy in (-200.0, 200.0):
            C.place("SM_Rasel_RoomFloor", (gx, gy, 0.0), label="창고바닥", mats=[STONE])
            C.place("SM_Rasel_Ceiling", (gx, gy, CEIL), label="창고천장", mats=[DARK, WOOD])

    # 남쪽(부두 쪽, -Y) — 가운데 두 칸이 하역문, 양끝은 민벽
    for gx in (-600.0, 600.0):
        for z in (0.0, ROW):
            C.place("SM_Rasel_Wall_Plain", (gx, -HD, z), yaw=180.0, label="남벽", mats=[PLASTER])
    for gx in (-200.0, 200.0):
        C.place("SM_Rasel_Wall_Door", (gx, -HD, 0.0), yaw=180.0, label="하역문 칸", mats=[PLASTER])
        C.place("SM_Rasel_Door_Unit", (gx, -HD, 0.0), yaw=180.0, label="하역문")
        # 문 위 두 번째 단은 채광창
        C.place("SM_Rasel_Wall_Win", (gx, -HD, ROW), yaw=180.0, label="남벽 위칸", mats=[PLASTER])
        C.place("SM_Rasel_Win_Small", (gx, -HD - 4.0, ROW + 140.0), yaw=180.0, label="채광창")

    # 북·동·서 — 아래는 민벽, 위 단은 채광창 줄(빛기둥이 떨어지게)
    for gx in (-600.0, -200.0, 200.0, 600.0):
        C.place("SM_Rasel_Wall_Plain", (gx, HD, 0.0), yaw=0.0, label="북벽", mats=[PLASTER])
        C.place("SM_Rasel_Wall_Win", (gx, HD, ROW), yaw=0.0, label="북벽 위칸", mats=[PLASTER])
        C.place("SM_Rasel_Win_Small", (gx, HD + 4.0, ROW + 140.0), yaw=0.0, label="채광창")
    for gy in (-200.0, 200.0):
        for (px, yw, lb) in ((HW, 270.0, "동벽"), (-HW, 90.0, "서벽")):
            for z in (0.0, ROW):
                C.place("SM_Rasel_Wall_Plain", (px, gy, z), yaw=yw, label=lb, mats=[PLASTER])
    C.log("껍데기: 바닥8·천장8·벽 두 단·하역문 2·채광창 6")


def stacks():
    """짐더미 — 목상자·가마니·통을 쌓아 올린다(엄폐도 되고 길도 낸다)."""
    n = 0
    # 동쪽 벽 앞: 목상자 두 줄로 쌓음
    for (bx, by) in ((640.0, 260.0), (640.0, -40.0), (500.0, 300.0)):
        for k, z in enumerate((0.0, 96.0, 192.0)):
            if k == 2 and bx == 500.0:
                break
            C.place("SM_Rasel_Crate", (bx, by, z), yaw=8.0 * k, label="목상자", mats=[WOOD])
            n += 1
    # 북서 구석: 가마니 더미
    for (sx, sy, z) in ((-620.0, 250.0, 0.0), (-540.0, 250.0, 0.0), (-580.0, 250.0, 60.0),
                        (-620.0, 160.0, 0.0), (-540.0, 160.0, 0.0)):
        C.place("SM_Rasel_Sack", (sx, sy, z), yaw=(sx + sy) % 40.0, label="가마니")
        n += 1
    # 통 줄
    for k in range(4):
        C.place("SM_Rasel_Barrel", (-360.0 + k * 90.0, -300.0, 0.0), label="통")
        n += 1
    # 바구니·밧줄·장대
    C.place("SM_Rasel_Basket", (300.0, 300.0, 0.0), label="바구니")
    C.place("SM_Rasel_Basket", (360.0, 260.0, 0.0), yaw=30.0, label="바구니")
    C.place("SM_Rasel_RopeCoil", (120.0, -300.0, 0.0), label="사린 밧줄")
    C.place("SM_Rasel_Poles", (-780.0, -180.0, 0.0), yaw=12.0, label="기대 세운 장대")
    C.place("SM_Rasel_Handcart", (-120.0, 60.0, 0.0), yaw=200.0, label="짐수레")
    n += 5
    C.log("짐더미 %d" % n)


def work_and_office():
    """★삯일 자리(짐 나르는 데)와 감독 자리."""
    # 삯일 자리 — 짐수레 곁에 목상자 몇을 내려놓은 자리
    C.place("SM_Rasel_Crate", (-60.0, -80.0, 0.0), yaw=14.0, label="삯일 자리 상자", mats=[WOOD])
    C.place("SM_Rasel_Crate", (30.0, -120.0, 0.0), yaw=-8.0, label="삯일 자리 상자", mats=[WOOD])
    C.lore((-20.0, -100.0, 60.0), "짐 나르는 자리",
           ["오늘 몫은 저 열 상자. 어제도 같은 수였다.",
            "허리가 안 뻐근하다. 어제는 뻐근했는데.",
            "능력이라는 걸 써 본 적도 없는데, 요새 몸이 되레 가볍다."])

    # 감독 자리 — 카운터와 명부
    C.place("SM_Rasel_Counter", (620.0, -260.0, 0.0), yaw=180.0, label="감독 카운터")
    C.place("SM_Rasel_PaperStack", (620.0, -300.0, 96.0), label="하역 명부")
    C.place("SM_Rasel_Stool", (620.0, -180.0, 0.0), label="걸상")
    C.lore((620.0, -300.0, 110.0), "하역 명부",
           ["배 이름과 짐 수가 날짜별로 적혀 있다.",
            "지난 열흘, 신항에서 실려 온 궤짝이 세 번. 받는 이 칸은 비어 있다."])
    C.log("삯일 자리·감독 자리")


def people():
    """★진행은 이 사람들에게 말을 걸어야 움직인다."""
    C.npc((560.0, -200.0, 0.0), "창고 감독",
          ["왔나. 오늘 몫은 저 열 상자다. 해 지기 전에 끝내라.",
           "요샌 신항 쪽에서 궤짝이 자주 온다. 받는 이 칸이 비어 있는 게 좀 그렇지만, 내 알 바 아니고.",
           "짐 다 지면 삯은 저녁에 준다."],
          yaw=180.0)
    C.npc((-300.0, 120.0, 0.0), "같이 지는 인부",
          ["자네 요새 힘 좋아졌더라. 그 나이엔 원래 그런가?",
           "나는 사흘째 어깨가 안 낫는다. 부럽구먼.",
           "…그러고 보니 자네, 요새 손이 뜨겁다고 하지 않았나?"],
          yaw=250.0)
    C.npc((260.0, 240.0, 0.0), "짐 세는 사람",
          ["세 번째 궤짝인데 무게가 다 다르다. 같은 배에서 내린 건데.",
           "묻지 마라. 나도 세기만 한다."],
          yaw=200.0)
    C.log("사람 셋 — 감독·인부·짐 세는 사람")


def dock_outside():
    """하역문 밖 부두 널 — 여기서 부두 하역장(L11)으로 나간다."""
    for k in range(5):
        C.place("SM_Rasel_RoadTile", (-400.0 + k * 400.0, -640.0, 0.0),
                label="부두 널", mats=[WOOD])
    C.place("SM_Rasel_Bench", (-520.0, -600.0, 0.0), yaw=90.0, label="쉬는 자리")
    C.place("SM_Rasel_Bucket", (420.0, -600.0, 0.0), label="물통")
    C.portal((0.0, -700.0, 60.0), "L11_Dock_Wharf", "→ 부두 하역장")
    # 지붕으로 오르는 사다리 대신 계단참(§22 감시·도주로) — 오를 자리만 세워 둔다
    C.place("SM_Rasel_Steps", (740.0, -430.0, 0.0), yaw=180.0, label="지붕 오르는 계단")
    C.lore((740.0, -470.0, 120.0), "지붕 오르는 계단",
           ["창고 지붕으로 오르는 철 계단. 위에서는 부두가 다 보인다.",
            "누가 오래 서 있었는지, 계단참에 담뱃재가 눌려 있다."])
    C.log("부두 널 5·문 밖 세간·나가는 문 1")


def lighting():
    """낮 — 채광창에서 떨어지는 빛기둥. 창고 안은 반그늘."""
    C.sky_and_exposure((0.0, 0.0), "day", exposure_bias=1.6)
    # 안쪽 매단 등 둘(반그늘을 메운다)
    for (lx, ly) in ((-300.0, 0.0), (400.0, 0.0)):
        C.place("SM_Rasel_HangLantern", (lx, ly, CEIL - 60.0), label="매단 등")
        C.point_light((lx, ly, CEIL - 180.0), 2400.0, (1.0, 0.86, 0.66), 900.0, "매단 등빛")
    C.log("낮 조명 + 매단 등 2")


def main():
    C.begin(MAP, "l12_warehouse")
    shell()
    stacks()
    work_and_office()
    people()
    dock_outside()
    lighting()

    ps = C.A().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, -560.0, 120.0))
    ps.set_actor_label("PlayerStart")

    C.finish_level()


try:
    main()
except Exception:
    unreal.log_error(traceback.format_exc())
    raise
