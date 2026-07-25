# -*- coding: utf-8 -*-
"""L02 네사의 골동상 — ★한 자리의 두 얼굴.

근거: 기획/02_게임설계/2_레벨/게임_레벨디자인_상세.md L02 · 기획/01_세계관/3_현대/세계관_현대_이야기.md §3-2·§3-6·§5-1·§10-B
낮: 도렌의 판을 감정하던 가게. 밤(결판의 밤 뒤): 뜯긴 마루와 탄 자국, 주인은 없다.

두 얼굴을 한 맵에서 껐다 켤 방법이 없으므로(메시에는 StoryFlag 축이 없다)
**맵을 둘로 나눈다.** L01의 골동상 문은 이미 `op_nesa_dead`로 갈리게 배선돼 있다 —
사건 전 문은 `L02_Antique_Shop`, 사건 뒤 문은 `L02_Antique_Shop_Burnt`로 간다.

실행: RASEL_STATE=shop|burnt UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=".../_build_L02.py"
"""
import sys, os, traceback
sys.path.append("C:/Secret_Project")
import unreal
import _rasel_common as C

STATE = os.environ.get("RASEL_STATE", "shop")
if STATE not in ("shop", "burnt"):
    STATE = "shop"
MAP = ("/Game/Maps/Rasel/L02_Antique_Shop" if STATE == "shop"
       else "/Game/Maps/Rasel/L02_Antique_Shop_Burnt")

HW = 400.0          # 좌우 절반(가로 800)
FRONT_Y = -200.0    # 앞가게 한가운데
BACK_Y = 200.0      # 뒷방 한가운데
CEIL = 320.0
MP = lambda n: "/Game/Rasel/Materials/%s" % n
PLASTER_IN, FLOOR_WOOD = MP("M_Rasel_PlasterIn"), MP("M_Rasel_FloorWood")


def shell():
    """앞가게와 뒷방 — 가운데 벽에 커튼 문틀 하나. 바닥·천장·벽 전부 낱개 부재."""
    for sx in (-200.0, 200.0):
        for sy in (FRONT_Y, BACK_Y):
            torn = (STATE == "burnt" and sx == 200.0 and sy == BACK_Y)
            C.place("SM_Rasel_FloorTorn" if torn else "SM_Rasel_RoomFloor",
                    (sx, sy, 0.0), label=("★뜯긴 마루" if torn else "바닥_%d_%d" % (sx, sy)),
                    mats=(None if torn else [FLOOR_WOOD]))
            C.place("SM_Rasel_Ceiling", (sx, sy, CEIL), label="천장_%d_%d" % (sx, sy),
                    mats=[PLASTER_IN, MP("M_Rasel_Wood")])

    # 남쪽 = 큰길(L01) 쪽. 앞면이 방 안(+Y)을 보게 yaw=180
    C.place("SM_Rasel_Wall_Door", (-200.0, -400.0, 0.0), yaw=180.0, label="남벽_문칸", mats=[PLASTER_IN])
    C.place("SM_Rasel_Door_Unit", (-200.0, -400.0, 0.0), yaw=180.0, label="앞문(→큰길)")
    C.place("SM_Rasel_Wall_Win", (200.0, -400.0, 0.0), yaw=180.0, label="남벽_창칸", mats=[PLASTER_IN])
    C.place("SM_Rasel_Win_Unit", (200.0, -400.0, 180.0), yaw=180.0, label="앞창")

    # 가운데 벽 — 커튼으로 가린 문틀(문짝은 없다)
    C.place("SM_Rasel_Wall_Door", (-200.0, 0.0, 0.0), yaw=180.0, label="가운데벽_커튼칸", mats=[PLASTER_IN])
    C.place("SM_Rasel_Curtain", (-200.0, 0.0, 0.0), yaw=0.0, label="커튼")
    C.place("SM_Rasel_Wall_Plain", (200.0, 0.0, 0.0), yaw=180.0, label="가운데벽", mats=[PLASTER_IN])

    # 북쪽 = 뒷골목(L03) 쪽 뒷문
    C.place("SM_Rasel_Wall_Plain", (-200.0, 400.0, 0.0), yaw=0.0, label="북벽", mats=[PLASTER_IN])
    C.place("SM_Rasel_Wall_Door", (200.0, 400.0, 0.0), yaw=0.0, label="북벽_뒷문칸", mats=[PLASTER_IN])
    C.place("SM_Rasel_Door_Unit", (200.0, 400.0, 0.0), yaw=0.0, label="뒷문(→뒷골목)")

    # 동·서 — 앞가게·뒷방 각각 한 장씩
    for sy in (FRONT_Y, BACK_Y):
        C.place("SM_Rasel_Wall_Plain", (HW, sy, 0.0), yaw=270.0, label="동벽_%d" % sy, mats=[PLASTER_IN])
        C.place("SM_Rasel_Wall_Plain", (-HW, sy, 0.0), yaw=90.0, label="서벽_%d" % sy, mats=[PLASTER_IN])
    C.log("껍데기: 바닥4(뜯긴 마루 %s)·천장4·벽9·문2·커튼1" %
          ("있음" if STATE == "burnt" else "없음"))


def put(kind, x, y, yaw=0.0, label=None, z=0.0):
    C.place("SM_Rasel_%s" % kind, (x, y, z), yaw=yaw, label=label)


def furnish_shop():
    """낮의 얼굴 — 감정 가게. 물건이 제자리에 있고 융이 깔려 있다."""
    put("AppraisalDesk", 60.0, -300.0, 0.0, "★감정대")
    put("DisplayCase", -300.0, -60.0, 0.0, "진열장1")
    put("DisplayCase", 250.0, -60.0, 0.0, "진열장2")
    put("Stool", 60.0, -170.0, 180.0, "감정대 걸상")
    put("Crate", 330.0, -320.0, 14.0, "궤")
    put("Basket", -350.0, -330.0, 30.0, "광주리")
    # 뒷방 — 아직 성한 창고
    put("Crate", -300.0, 300.0, 8.0, "뒷방 궤1")
    put("Crate", -180.0, 330.0, -12.0, "뒷방 궤2")
    put("Sack", 60.0, 340.0, 20.0, "뒷방 자루")
    put("Barrel", 330.0, 130.0, 0.0, "뒷방 통")
    put("RopeCoil", 150.0, 120.0, 0.0, "밧줄 사리")
    C.log("낮의 얼굴: 감정대·진열장2·걸상·궤3·자루·통·광주리·밧줄")


def furnish_burnt():
    """★밤의 얼굴 — 결판의 밤 뒤. 뜯긴 마루, 태운 궤, 눌어붙은 자국. 주인은 없다."""
    put("AppraisalDesk", 60.0, -300.0, 6.0, "감정대(그대로)")
    put("DisplayCase", -300.0, -60.0, 0.0, "진열장1")
    put("DisplayCase", 250.0, -70.0, 24.0, "진열장2(밀려남)")
    put("Stool", -20.0, -150.0, 62.0, "넘어진 걸상")
    # 뒷방 — 태운 자리와 눌어붙은 자국
    put("ChestBurnt", 320.0, 110.0, 12.0, "★태운 궤")
    C.place("SM_Rasel_BurnPatch", (320.0, 110.0, 1.0), label="바닥 탄 자국")
    C.place("SM_Rasel_BurnPatch", (386.0, 110.0, 150.0), yaw=90.0, pitch=90.0,
            label="벽 탄 자국")
    put("Crate", -300.0, 300.0, 8.0, "뒷방 궤1")
    put("Barrel", 60.0, 250.0, 18.0, "쓰러진 통")
    put("RopeCoil", -80.0, 130.0, 0.0, "밧줄 사리")
    C.log("밤의 얼굴: 뜯긴 마루·태운 궤·탄 자국2·밀려난 진열장·넘어진 걸상")


def wiring():
    C.portal((-200.0, -310.0, 60.0), "L01_Jangteo_Street", "→ 장터 큰길")
    C.portal((200.0, 310.0, 60.0), "L03_Backalley", "→ 장터 뒷골목(뒷문)")

    if STATE == "shop":
        C.lore((60.0, -300.0, 110.0), "감정대",
               ["융을 깐 판. 물건을 여기 올리고 값을 매긴다.",
                "네사가 도렌의 판을 이 위에 놓고 오래 멈췄다.",
                "삭지 않은 판은 보통 물건이 아니다 — 그 말만 하고 값은 안 불렀다."])
        C.lore((250.0, 250.0, 60.0), "뒷방 마루",
               ["널이 한 군데만 색이 다르다. 최근에 들었다 다시 덮은 자리.",
                "밟으면 아래가 빈 소리가 난다."])
        C.npc((60.0, -190.0, 100.0), "골동상 주인",
              ["삭지 않은 물건은 값을 함부로 못 매겨. 먼저 무엇인지부터 알아야지.",
               "안쪽은 창고야. 볼 것 없어."], yaw=0.0)
    else:
        C.lore((250.0, 290.0, 60.0), "뜯긴 마루 널",
               ["널이 통째로 들려 있다. 아래는 비어 있다.",
                "여기 판을 숨겼다. 결판의 밤, 두 손이 같은 널을 뜯었다.",
                "들린 방향이 둘이다 — 한쪽은 급했고 한쪽은 익숙했다."])
        C.lore((320.0, 110.0, 70.0), "탄 자국",
               ["궤 하나를 태운 자리. 벽까지 눌어붙었다.",
                "관은 이것을 새어 든 연기에 의한 사고사로 접수했다."])
        C.lore((60.0, -300.0, 110.0), "감정대(그대로)",
               ["감정대만 손대지 않았다. 융 위에 먼지가 앉지 않았다.",
                "여기 있던 것은 이미 나갔다는 뜻이다."])
    C.log("배선: 포탈2 · 조사%d · NPC%d" % (2 if STATE == "shop" else 3,
                                           1 if STATE == "shop" else 0))


def lighting():
    # ★매단 등마다 종이등 부재를 천장(z=320)에 걸어 조명과 메시를 맞춘다(속 발광은 z≈224).
    if STATE == "shop":
        C.sky_and_exposure((0.0, 0.0), "day", exposure_bias=1.2)
        for (lx, ly, lum, r, lb) in ((60.0, -260.0, 2000.0, 560.0, "감정대 등"),
                                     (0.0, -100.0, 1500.0, 520.0, "앞가게 등"),
                                     (0.0, 250.0, 700.0, 460.0, "뒷방 등")):
            C.place("SM_Rasel_HangLantern", (lx, ly, CEIL), yaw=0.0, label="종이등(%s)" % lb)
            C.point_light((lx, ly, 224.0), lum, (1.0, 0.85, 0.64), r, lb, volumetric=1.2)
    else:
        C.sky_and_exposure((0.0, 0.0), "night", exposure_bias=2.2)
        # 등 하나만 흔들리듯 남았다 — 나머지는 어둠
        C.place("SM_Rasel_HangLantern", (0.0, -140.0, CEIL), yaw=8.0, label="종이등(남은 하나)")
        C.point_light((0.0, -140.0, 224.0), 900.0, (1.0, 0.76, 0.5), 470.0,
                      "남은 등 하나", volumetric=1.8)
        # 뜯긴 마루 아래에서 올라오는 아주 약한 빛(아래층이 비어 있다는 표)
        C.point_light((240.0, 180.0, 20.0), 260.0, (0.5, 0.62, 0.78), 300.0,
                      "마루 밑 빈 곳", volumetric=1.4, shadows=False)
    C.log("빛: %s" % ("낮 가게" if STATE == "shop" else "사건 뒤 밤"))


def build():
    C.begin(MAP, "l02_%s" % STATE)
    shell()
    (furnish_shop if STATE == "shop" else furnish_burnt)()
    wiring()
    lighting()
    C.A().spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-200.0, -300.0, 110.0))
    C.finish_level()


try:
    build()
except Exception:
    C.log("실패\n" + traceback.format_exc())
    C.flush()
