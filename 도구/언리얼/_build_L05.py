# -*- coding: utf-8 -*-
"""L05 조용한 밥집 — 실내를 조합식으로 짜는 첫 사례.

근거: 기획/02_게임설계/2_레벨/게임_레벨디자인_상세.md L05 · 기획/01_세계관/3_현대/세계관_현대_이야기.md §8-2
**위협 없이 위협적인 방.** 셀란의 사람이 도렌을 구석 자리에 앉히고 "밤에 지하를
잠깐 보게 해 달라"고 청했다 — 위협을 안 해서 더 무서웠던 자리.

방은 벽 널 400칸에 맞춰 800×400×320. 큰길(L01) 쪽이 남쪽(-Y)이라 거기 문과 창이 난다.
실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=".../_build_L05.py"
"""
import sys, traceback
sys.path.append("C:/Secret_Project/도구/언리얼")
import unreal
import _rasel_common as C

MAP = "/Game/Maps/Rasel/L05_Eatery"
HW, HD = 400.0, 200.0        # 방 절반 크기(가로 800 · 세로 400)
CEIL = 320.0


MP = lambda n: "/Game/Rasel/Materials/%s" % n
PLASTER_IN = MP("M_Rasel_PlasterIn")
FLOOR_WOOD = MP("M_Rasel_FloorWood")


def shell():
    """바닥·천장·네 벽 — 전부 낱개 부재. ★실내 톤(따뜻한 회벽·닳은 마루)으로 재료 교체."""
    for sx in (-200.0, 200.0):
        C.place("SM_Rasel_RoomFloor", (sx, 0.0, 0.0), label="방바닥_%d" % sx,
                mats=[FLOOR_WOOD])
        C.place("SM_Rasel_Ceiling", (sx, 0.0, CEIL), label="천장_%d" % sx,
                mats=[PLASTER_IN, MP("M_Rasel_Wood")])
    # 남쪽(큰길 쪽) — 문 한 칸, 창 한 칸. 앞면이 방 안(+Y)을 보도록 yaw=180
    C.place("SM_Rasel_Wall_Door", (-200.0, -HD, 0.0), yaw=180.0, label="남벽_문칸",
            mats=[PLASTER_IN])
    C.place("SM_Rasel_Door_Unit", (-200.0, -HD, 0.0), yaw=180.0, label="문(→큰길)")
    C.place("SM_Rasel_Wall_Win", (200.0, -HD, 0.0), yaw=180.0, label="남벽_창칸",
            mats=[PLASTER_IN])
    C.place("SM_Rasel_Win_Unit", (200.0, -HD, 180.0), yaw=180.0, label="창")
    # 북쪽·동·서 민벽 — 실내 회벽
    for (px, py, yw, lb) in ((-200.0, HD, 0.0, "북벽_L"), (200.0, HD, 0.0, "북벽_R"),
                             (HW, 0.0, 270.0, "동벽"), (-HW, 0.0, 90.0, "서벽")):
        C.place("SM_Rasel_Wall_Plain", (px, py, 0.0), yaw=yw, label=lb, mats=[PLASTER_IN])
    C.log("껍데기: 바닥2·천장2·벽6·문1·창1 (실내 톤)")


def put(kind, x, y, yaw=0.0, label=None, foot=None):
    """실내는 손으로 자리를 정한다 — 좁은 방에서는 원 예약이 너무 거칠어
    상과 걸상까지 밀어낸다. 대신 놓은 자리를 적어 두고 검사기로 확인한다."""
    C.place("SM_Rasel_%s" % kind, (x, y, 0.0), yaw=yaw, label=label)
    C.reserve(x, y, foot if foot is not None else C.FOOT.get(kind, 40))


def furnish():
    # 북쪽 주방 줄 — 카운터와 화덕
    C.place("SM_Rasel_Counter", (130.0, 150.0, 0.0), yaw=0.0, label="카운터")
    C.reserve_line(-110.0, 150.0, 370.0, 150.0, 60.0, 4)
    put("Hearth", -250.0, 140.0, 0.0, "화덕", foot=100.0)
    C.place("SM_Rasel_Shelf", (130.0, HD - 22.0, 205.0), yaw=0.0, label="선반")
    for k, sx in enumerate((30.0, 80.0, 130.0, 180.0)):
        C.place("SM_Rasel_Bowl", (sx, HD - 26.0, 209.0), label="그릇%d" % k)

    # 손님 상 — 정본대로 "몇 안 되는 상". 문 앞 통로(x<-90)는 비운다.
    for k, (tx, ty) in enumerate(((0.0, -70.0),)):
        put("Table", tx, ty, 0.0, "상%d" % (k + 1))
        put("Stool", tx, ty + 110.0, 180.0, "걸상%d-1" % (k + 1))
        put("Stool", tx, ty - 110.0, 0.0, "걸상%d-2" % (k + 1))
        for j in range(2):
            C.place("SM_Rasel_Bowl", (tx - 40.0 + j * 80.0, ty + 8.0, 77.0),
                    label="상%d_그릇%d" % (k + 1, j))

    # ★구석 밀담상 — 칸막이로 가린 자리(§8-2)
    put("Table", 305.0, -105.0, 0.0, "★구석 밀담상")
    put("Stool", 305.0, -2.0, 180.0, "밀담 걸상(안쪽)")
    put("Stool", 305.0, -208.0, 0.0, "밀담 걸상(바깥)")
    C.place("SM_Rasel_Partition", (185.0, -105.0, 0.0), yaw=90.0, label="칸막이")
    C.reserve_line(185.0, -215.0, 185.0, 5.0, 45.0, 3)

    # 주방 뒤 세간
    put("Sack", -360.0, 30.0, 20.0, "쌀자루")
    put("Barrel", -360.0, -160.0, 0.0, "물통")
    put("Basket", -360.0, -70.0, 30.0, "광주리")
    C.log("세간: 카운터·화덕·선반·그릇5·상3·걸상6·칸막이·자루/통/광주리")


def wiring():
    C.portal((-200.0, -HD + 90.0, 60.0), "L01_Jangteo_Street", "→ 장터 큰길")
    C.lore((305.0, -105.0, 90.0), "구석 자리",
           ["칸막이에 가려 등이 잘 안 드는 자리. 이 집에서 제일 조용하다.",
            "셀란의 사람이 도렌을 여기 앉혔다. 목소리를 높이지 않았고, 값도 후하게 쳤다.",
            "위협하지 않았다. 그래서 더 무서웠다고 도렌은 나중에 말했다."])
    C.lore((130.0, 96.0, 110.0), "카운터",
           ["국이 끓는 냄새. 저녁이면 늘 같은 얼굴들이 같은 자리에 앉는다.",
            "요즘 신항 쪽 얘기를 묻는 낯선 손님이 부쩍 늘었다."])
    D = getattr(getattr(unreal, "DayPhase", None), "DAY", None)
    N = getattr(getattr(unreal, "DayPhase", None), "NIGHT", None)
    C.npc((130.0, 210.0, 100.0), "밥집 주인 내외",
          ["앉아. 오늘 국은 괜찮아.",
           "요즘 신항 쪽 얘기 물어보는 사람이 부쩍 늘었어."], yaw=270.0)
    C.npc((0.0, -180.0, 100.0), "저녁 단골",
          ["여기 이 자리가 내 자리야. 십 년째.",
           "저 구석은 앉지 마. 저긴 값을 치르는 자리야."], yaw=90.0, phase=N)
    C.log("배선: 포탈1 · 조사2 · NPC2")


def lighting():
    phase = C.day_phase("evening")
    C.sky_and_exposure((0.0, 0.0), phase, exposure_bias=1.6, volumetric=True)
    # 화덕 — 이 방의 주광원(주황)
    C.point_light((-250.0, 100.0, 90.0), 3000.0, (1.0, 0.62, 0.28), 620.0,
                  "화덕불", volumetric=2.6, source_radius=26.0)
    # 매단 등 둘 — 손님 상 위. ★부재(종이등)를 실제로 매달아 조명과 메시를 맞춘다.
    #   등롱 원점은 매다는 줄 위라 천장(z=320)에 걸고, 속 발광은 z≈224에 온다.
    for k, (lx, ly) in enumerate(((0.0, -70.0), (-250.0, 40.0))):
        C.place("SM_Rasel_HangLantern", (lx, ly, CEIL), yaw=k * 40.0,
                label="종이등%d" % (k + 1))
        C.point_light((lx, ly, 224.0), 1600.0, (1.0, 0.80, 0.55), 520.0,
                      "매단 등%d" % (k + 1), volumetric=1.4)
    # ★구석은 일부러 어둡게 — 얼굴이 반쯤 그늘에 든다(밀담 톤)
    C.point_light((305.0, -105.0, 262.0), 420.0, (0.86, 0.80, 0.72), 320.0,
                  "구석 약한 등", volumetric=1.0, shadows=True)
    C.log("빛: %s · 화덕1 + 매단등2 + 구석 약한등1" % phase)


def build():
    C.begin(MAP, "l05")
    shell()
    furnish()
    wiring()
    lighting()
    C.A().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-200.0, -110.0, 110.0))
    C.finish_level()


try:
    build()
except Exception:
    C.log("실패\n" + traceback.format_exc())
    C.flush()
