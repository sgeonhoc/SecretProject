# -*- coding: utf-8 -*-
"""L04 돌간의 흥신소 — 조합식으로 다시 짓는다(옛 판은 통짜 시절 산물).

근거: 기획/02_게임설계/2_레벨/게임_레벨디자인_상세.md L04 · 기획/01_세계관/3_현대/세계관_현대_이야기.md §7-1·§12·§15-1·§18
**수사 허브.** 이 도시에서 판 전체를 위에서 내려다보는 유일한 서랍이 여기 있다.
방 하나가 곧 사건판이다. 창이 큰길을 내려다보고, 등불은 창에서 한 뼘 안에 있다(§12-3).

이층 사무소라 바닥을 z=320에 올려 짓는다 — 창밖으로 아랫장터 지붕선이 보인다.
실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=".../_build_L04.py"
"""
import sys, traceback
sys.path.append("C:/Secret_Project/도구/언리얼")
import unreal
import _rasel_common as C

MAP = "/Game/Maps/Rasel/L04_Dolgan_Office"
HW, HD = 400.0, 200.0        # 방 절반(가로 800 · 세로 400)
Z0 = 320.0                   # ★이층 — 바닥이 한 층 위에 있다
CEIL = Z0 + 300.0            # 낮은 천장(정본 "좁고 낮음")


MP = lambda n: "/Game/Rasel/Materials/%s" % n
PLASTER_IN, FLOOR_WOOD = MP("M_Rasel_PlasterIn"), MP("M_Rasel_FloorWood")


def shell():
    """★실내 톤(따뜻한 회벽·닳은 마루)으로 재료 교체 — 좁고 눌린 느와르 사무소."""
    for sx in (-200.0, 200.0):
        C.place("SM_Rasel_RoomFloor", (sx, 0.0, Z0), label="사무소 바닥_%d" % sx,
                mats=[FLOOR_WOOD])
        C.place("SM_Rasel_Ceiling", (sx, 0.0, CEIL), label="천장_%d" % sx,
                mats=[PLASTER_IN, MP("M_Rasel_Wood")])
    # 남쪽 = 큰길 쪽. 창(내려다보는 자리)과 문
    C.place("SM_Rasel_Wall_Win", (-200.0, -HD, Z0), yaw=180.0, label="남벽_창칸",
            mats=[PLASTER_IN])
    C.place("SM_Rasel_Win_Unit", (-200.0, -HD, Z0 + 180.0), yaw=180.0, label="★창(큰길 내려봄)")
    C.place("SM_Rasel_Wall_Door", (200.0, -HD, Z0), yaw=180.0, label="남벽_문칸",
            mats=[PLASTER_IN])
    C.place("SM_Rasel_Door_Unit", (200.0, -HD, Z0), yaw=180.0, label="문(→큰길 계단)")
    # 북·동·서 — 실내 회벽
    for (px, py, yw, lb) in ((-200.0, HD, 0.0, "북벽_L"), (200.0, HD, 0.0, "북벽_R"),
                             (HW, 0.0, 270.0, "동벽"), (-HW, 0.0, 90.0, "서벽")):
        C.place("SM_Rasel_Wall_Plain", (px, py, Z0), yaw=yw, label=lb, mats=[PLASTER_IN])
    C.log("껍데기: 바닥2·천장2·벽6·창1·문1 (실내 톤·이층 z=%d)" % Z0)


def outside():
    """창밖 — 아랫장터 지붕선 실루엣. 이 방이 '내려다보는 자리'임을 눈으로 알린다."""
    for k, (ox, oy, h, w) in enumerate(((-320, -900, 300, 420), (60, -1050, 250, 380),
                                        (400, -880, 340, 360), (-60, -1400, 220, 460))):
        C.place("SM_Rasel_Wall_Plain", (ox, oy, h - 320.0), yaw=0.0,
                label="건너편 집%d" % k, scale=(w / 400.0, 1.0, 1.0))
        C.place("SM_Rasel_Eave", (ox, oy, h), yaw=0.0, label="건너편 처마%d" % k)
    C.log("창밖 지붕선 4채")


def furnish():
    # 책상 — ★창에서 한 뼘 안쪽(§12-3 돌간이 등불을 옮긴 그 자리)
    C.place("SM_Rasel_Desk", (-190.0, -40.0, Z0), yaw=0.0, label="책상")
    C.place("SM_Rasel_Drawer", (-190.0, -40.0, Z0), yaw=0.0, label="★서랍(사건 지도)")
    C.place("SM_Rasel_Chair", (-190.0, 90.0, Z0), yaw=180.0, label="돌간 자리")
    C.place("SM_Rasel_Chair", (-190.0, -170.0, Z0), yaw=0.0, label="손님 의자")
    C.place("SM_Rasel_PaperStack", (-90.0, -20.0, Z0 + 78.0), yaw=12.0, label="책상 위 서류")

    # ★부고란 벽 — 북쪽 벽에 건다
    C.place("SM_Rasel_ObitBoard", (200.0, HD - 22.0, Z0 + 165.0), yaw=0.0, label="★부고란")
    C.place("SM_Rasel_Shelf", (-200.0, HD - 22.0, Z0 + 195.0), yaw=0.0, label="서류 선반")
    for k, sx in enumerate((-310.0, -215.0, -120.0)):
        C.place("SM_Rasel_PaperStack", (sx, HD - 26.0, Z0 + 200.0), yaw=k * 9.0,
                label="선반 서류%d" % k)

    # 뒤 계단(잠김) — 동북 구석
    C.place("SM_Rasel_Steps", (330.0, -60.0, Z0), yaw=180.0, label="뒤 계단")
    C.place("SM_Rasel_Crate", (140.0, -160.0, Z0), yaw=16.0, label="궤")
    C.place("SM_Rasel_Barrel", (40.0, 50.0, Z0), yaw=0.0, label="통")
    C.log("세간: 책상·서랍·의자2·부고란·선반·서류4·계단·궤·통")


def wiring():
    C.portal((200.0, -HD + 90.0, Z0 + 60.0), "L01_Jangteo_Street", "→ 장터 큰길")

    C.lore((-190.0, -110.0, Z0 + 90.0), "★서랍 속 종잇조각",
           ["사고사 셋이 한 종이에 나란히 적혀 있다 — 골동상·되넘김꾼·젊은이.",
            "셋 다 죽기 전 신항에서 나온 낱장을 만졌다.",
            "위에는 물음표 하나. 아래에는 손이 둘 그려져 있다 — 태우는 손, 모으는 손.",
            "이 도시에서 판 전체를 위에서 내려다보는 서랍은 이것 하나다."])
    C.lore((200.0, HD - 40.0, Z0 + 160.0), "부고란",
           ["오려 붙인 부고가 벽을 덮었다. 관은 각각 흔한 사고로 접수했다.",
            "같은 달에 넉 장. 흔한 사고가 한 달에 넉 장이면 흔한 것이 아니다.",
            "아래층에서 죽은 자는 여기 오르지 않는다."])
    C.lore((-200.0, -HD + 50.0, Z0 + 150.0), "창가",
           ["길 건너 처마 밑에 담배꽁초가 두 개. 밤새 두 사람이 서 있었다는 뜻이다.",
            "그래서 등불을 창에서 한 뼘 안으로 옮겼다. 그림자가 창에 안 비치도록."])

    gate = C.A().spawn_actor_from_class(C.cls("LockedGateActor"),
                                        unreal.Vector(330.0, 10.0, Z0 + 60.0))
    C.setp(gate, "GateId", "L04_BackStair")
    gate.set_actor_label("뒤 계단(잠김)")

    C.npc((-60.0, 60.0, Z0 + 100.0), "흥신소 주인",
          ["부고 세 줄이 한 줄에 놓이면, 그건 더는 사고가 아니야.",
           "묻지 않은 게 나를 살렸지.",
           "삯은 선불이야. 나는 오래 하려고."], yaw=200.0)
    C.log("배선: 포탈1 · 조사3 · 잠긴계단1 · NPC1")


def lighting():
    C.sky_and_exposure((0.0, -200.0), "night", exposure_bias=2.1)
    # ★책상 등 — 창에서 한 뼘 안쪽. 이 방의 유일한 주광원(§12-3)
    C.point_light((-150.0, -20.0, Z0 + 150.0), 1600.0, (1.0, 0.74, 0.44), 430.0,
                  "책상 등(창에서 한 뼘 안)", volumetric=1.8, source_radius=10.0)
    # 창으로 새어 드는 큰길 가로등
    C.point_light((-200.0, -HD - 60.0, Z0 + 120.0), 900.0, (1.0, 0.70, 0.40), 560.0,
                  "창으로 새어 든 가로등", volumetric=2.2, shadows=False)
    # 부고란이 아주 흐리게 읽히도록 약한 반사광
    C.point_light((200.0, HD - 90.0, Z0 + 210.0), 260.0, (0.90, 0.86, 0.78), 300.0,
                  "부고란 쪽 약한 빛", volumetric=1.0, shadows=False)
    C.log("빛: 밤 · 책상 등1(주광) + 창 새어듦1 + 부고란 약한 빛1")


def build():
    C.begin(MAP, "l04")
    shell()
    outside()
    furnish()
    wiring()
    lighting()
    C.A().spawn_actor_from_class(unreal.PlayerStart,
                                 unreal.Vector(200.0, -100.0, Z0 + 110.0))
    C.finish_level()


try:
    build()
except Exception:
    C.log("실패\n" + traceback.format_exc())
    C.flush()
