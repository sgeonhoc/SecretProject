# -*- coding: utf-8 -*-
"""L21 딘의 은신처 (흩어진 자의 빈집) — 신규.

근거: 기획/02_게임설계/2_레벨/게임_레벨디자인_상세.md L21 · 기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md L21 · 현대 §6-2·§22-3.
조직도 스승도 없이 서로를 옮겨 숨기는 빈집. 세간은 남의 것이고, 사람은 며칠마다 바뀐다.

**진행은 사람이 한다** — 딘과 옮겨 온 자들에게 말을 걸어야 이야기가 움직인다.
갓 눈뜬 자들은 `RequiredFlag`로 회차마다 다르게 서게 할 수 있다(지금은 셋 다 세워 둔다).

공간: 방 하나(1200×800) + 부엌 구석 + 뒷길.
실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript="C:/Secret_Project/_build_L21_safehouse.py"
"""
import sys, traceback
sys.path.append("C:/Secret_Project/도구/언리얼")
import unreal
import _rasel_common as C

MAP = "/Game/Maps/Rasel/L21_Din_Safehouse"
HW, HD = 600.0, 400.0
CEIL = 320.0

MP = lambda n: "/Game/Rasel/Materials/%s" % n
PLASTER_IN = MP("M_Rasel_PlasterIn")
FLOORW = MP("M_Rasel_FloorWood")
WOOD = MP("M_Rasel_Wood")
CLOTH_A = MP("M_Rasel_ClothA")
CLOTH_B = MP("M_Rasel_ClothB")


def shell():
    for gx in (-400.0, 0.0, 400.0):
        for gy in (-200.0, 200.0):
            C.place("SM_Rasel_RoomFloor", (gx, gy, 0.0), label="방바닥", mats=[FLOORW])
            C.place("SM_Rasel_Ceiling", (gx, gy, CEIL), label="천장", mats=[PLASTER_IN, WOOD])
    # 남쪽 — 들어오는 문 한 칸, 창 두 칸(덧문 내려 반쯤 가림)
    C.place("SM_Rasel_Wall_Door", (0.0, -HD, 0.0), yaw=180.0, label="남벽_문칸", mats=[PLASTER_IN])
    C.place("SM_Rasel_Door_Unit", (0.0, -HD, 0.0), yaw=180.0, label="드는 문")
    for gx in (-400.0, 400.0):
        C.place("SM_Rasel_Wall_Win", (gx, -HD, 0.0), yaw=180.0, label="남벽_창칸", mats=[PLASTER_IN])
        C.place("SM_Rasel_Win_Small", (gx, -HD - 4.0, 180.0), yaw=180.0, label="창")
        C.place("SM_Rasel_Shutter", (gx - 60.0, -HD - 10.0, 200.0), yaw=180.0, label="내린 덧문")
    # 북쪽 — 뒷길 문 한 칸
    C.place("SM_Rasel_Wall_Door", (-400.0, HD, 0.0), yaw=0.0, label="북벽_뒷문칸", mats=[PLASTER_IN])
    C.place("SM_Rasel_Door_Unit", (-400.0, HD, 0.0), yaw=0.0, label="뒷길 문")
    for gx in (0.0, 400.0):
        C.place("SM_Rasel_Wall_Plain", (gx, HD, 0.0), yaw=0.0, label="북벽", mats=[PLASTER_IN])
    for gy in (-200.0, 200.0):
        C.place("SM_Rasel_Wall_Plain", (HW, gy, 0.0), yaw=270.0, label="동벽", mats=[PLASTER_IN])
        C.place("SM_Rasel_Wall_Plain", (-HW, gy, 0.0), yaw=90.0, label="서벽", mats=[PLASTER_IN])
    C.log("껍데기: 바닥6·천장6·벽8·문2·창2")


def furnish():
    """남의 집 세간 + 옮겨 온 자들이 들고 온 것. 살림이 아니라 야영에 가깝다."""
    # 가운데 상 하나와 걸상 몇 — 앉을 데가 모자란다
    C.place("SM_Rasel_Table", (0.0, 40.0, 0.0), label="상")
    C.reserve_line(-120.0, 40.0, 120.0, 40.0, 60.0, 3)
    for (sx, sy, yw) in ((-160.0, 20.0, 90.0), (160.0, 20.0, 270.0), (0.0, 170.0, 180.0)):
        C.place("SM_Rasel_Stool", (sx, sy, 0.0), yaw=yw, label="걸상")
    # 잠자리 — 가마니와 천을 깔았다
    for (bx, by) in ((-430.0, -180.0), (-430.0, 60.0), (430.0, -180.0)):
        C.place("SM_Rasel_Sack", (bx, by, 0.0), label="잠자리 가마니")
        C.place("SM_Rasel_Curtain", (bx, by + 60.0, 0.0), yaw=90.0, label="가린 천",
                mats=[CLOTH_A if bx < 0 else CLOTH_B])
    # 부엌 구석 — 화덕과 그릇
    C.place("SM_Rasel_Hearth", (430.0, 250.0, 0.0), yaw=180.0, label="화덕")
    C.place("SM_Rasel_Pot", (330.0, 250.0, 0.0), label="솥")
    C.place("SM_Rasel_Bowl", (60.0, 60.0, 78.0), label="그릇")
    C.place("SM_Rasel_Bowl", (-60.0, 20.0, 78.0), label="그릇")
    C.place("SM_Rasel_Bucket", (500.0, 120.0, 0.0), label="물통")
    # 짐 — 언제든 뜰 수 있게 싸 둔 것
    C.place("SM_Rasel_Crate", (-520.0, 280.0, 0.0), yaw=20.0, label="싸 둔 짐")
    C.place("SM_Rasel_Basket", (-450.0, 300.0, 0.0), label="바구니")
    C.place("SM_Rasel_PaperStack", (0.0, -20.0, 78.0), label="상 위 종이")
    C.log("세간: 상·걸상3·잠자리3·부엌·싸 둔 짐")


def things():
    C.lore((0.0, -20.0, 96.0), "상 위 종이",
           ["잿마당에서 베낀 글자 몇 줄. 손글씨가 서로 다르다 — 여럿이 돌려 적었다.",
            "귀퉁이에 자리 이름이 적혔다가 그어져 있다. 두 번 쓴 자리는 없다."])
    C.lore((-400.0, 360.0, 120.0), "뒷길 문",
           ["빗장이 안에서만 걸린다. 문턱에 흙이 밟혀 있다 — 오늘도 누가 나갔다."])
    C.lore((-430.0, -180.0, 60.0), "빈 잠자리",
           ["가마니가 눌린 자국이 아직 남아 있다. 어제까지 누가 여기 있었다.",
            "짐은 없다. 짐이 없다는 건 급하게 옮겼다는 뜻이다."])
    C.portal((-400.0, 470.0, 60.0), "L01_Jangteo_Street", "뒷길 → 장터")
    C.portal((0.0, -470.0, 60.0), "L01_Jangteo_Street", "드는 문 → 장터")
    C.log("조사 3 · 문 2")


def people():
    C.npc((-120.0, 150.0, 0.0), "딘",
          ["왔구나. 신은 벗어 두고, 문 쪽엔 앉지 마.",
           "규율은 하나야 — 뭉치지 않는다. 여기 있는 사람 이름도 다 알 필요 없어.",
           "잿마당은 끊었나? 글 올린 자리가 곧 주소고, 주소는 요양원으로 간다.",
           "쉬어. 밤에 옮길 수도 있으니 짐은 싸 둔 채로."],
          yaw=200.0)
    C.npc((300.0, -120.0, 0.0), "갓 눈뜬 사내",
          ["나는 사흘 됐다. 손에서 물이 맺힌다. 그게 다야.",
           "여기 오기 전엔 병인 줄 알았지. 의원에 갔더니 열이라더라.",
           "…딘이 그러던데, 의원 기록은 위층이 긁어 간다더군."],
          yaw=140.0)
    C.npc((420.0, 180.0, 0.0), "이름 안 대는 여자",
          ["묻지 마라. 나도 안 묻는다.",
           "그게 여기 규칙이야. 서로 모를수록 서로 안전하다."],
          yaw=210.0)
    C.log("사람 셋 — 딘·갓 눈뜬 사내·이름 안 대는 여자")


def lighting():
    C.sky_and_exposure((0.0, 0.0), "night", exposure_bias=1.9)
    # 등불 하나만. 나머지는 어둠 — 숨어 있는 집이라 밝힐 수 없다.
    C.place("SM_Rasel_Lantern", (0.0, 120.0, 78.0), label="상 위 등")
    C.point_light((0.0, 120.0, 150.0), 1500.0, (1.0, 0.74, 0.42), 620.0, "등불")
    C.point_light((430.0, 250.0, 120.0), 700.0, (1.0, 0.58, 0.28), 420.0, "화덕불")
    C.log("밤 조명 — 등불 하나·화덕불")


def main():
    C.begin(MAP, "l21_safehouse")
    shell()
    furnish()
    things()
    people()
    lighting()
    ps = C.A().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, -320.0, 120.0))
    ps.set_actor_label("PlayerStart")
    C.finish_level()


try:
    main()
except Exception:
    unreal.log_error(traceback.format_exc())
    raise
