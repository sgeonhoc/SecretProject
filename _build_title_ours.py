# -*- coding: utf-8 -*-
"""시작 화면(Title) 다시 짓기 — **우리 것만으로**.

왜: 지금 Title 맵은 외부 샘플 팩(Asian_Village 벽·지붕·등롱·장막, Free_Spells 나이아가라)으로
    차 있다. 사용자 상설 규율 — "이 안에있는 에셋들 전부다 외부에서 샘플로 가져온거라
    저작권 확인 안 돼서 쓰면 안 된다" (2026-07-21). 게임을 켜면 맨 처음 보이는 화면이라
    여기부터 우리 것이어야 한다.

무엇을: 라셀 아랫장터의 **밤 한 귀퉁이**. 우리 키트(/Game/Rasel/Meshes)와 우리 머티리얼만.
    젖은 포석 · 가게 파사드 둘 · 불 켜진 창 · 처마 등롱 · 벽보판 · 좌판 하나 · 물웅덩이.
    시작 화면 메뉴(WBP_MainMenu)가 그 위에 얹힌다.

실행: UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="C:/Secret_Project/_build_title_ours.py"
"""
import sys
sys.path.append("C:/Secret_Project")
import unreal
import _rasel_common as C

TILE = 400.0
FAC_Y = 600.0          # 파사드 앞면이 서는 자리(L01과 같은 규격)
STOREY = 320.0


def xc(i):
    return TILE * i


def build():
    # ★옛 Title 맵을 비우고 다시 짓지 않는다 — 액터 174개를 한 번에 지우면 엔진이 죽는다
    #   (2026-07-24 실제 크래시: 전멸 직후 EXCEPTION_ACCESS_VIOLATION).
    #   대신 **빈 맵을 새로 만들어** 거기에 짓고, 시작 맵 설정을 그쪽으로 돌린다.
    C.begin("/Game/Maps/Title_Rasel", "title_ours")

    # ── 바닥: 젖은 포석 넉 줄 × 다섯 칸 ─────────────────────────
    for i in range(5):
        for row in (-400.0, 0.0, 400.0):
            C.place("SM_Rasel_RoadTile", (xc(i), row, 0.0), label="Road_%d_%d" % (i, int(row)))
        C.place("SM_Rasel_Gutter", (xc(i), 560.0, 0.0), label="Gutter_L%d" % i)
        C.place("SM_Rasel_Gutter", (xc(i), -560.0, 0.0), label="Gutter_R%d" % i)

    # 비 온 뒤 — 등불을 되비치는 웅덩이(시작 화면의 빛을 두 배로 쓴다)
    for (px, py, kind) in ((xc(1), 300.0, "Puddle_M"), (xc(2) + 140.0, -300.0, "Puddle_L"),
                           (xc(3) - 60.0, 260.0, "Puddle_S")):
        C.place("SM_Rasel_%s" % kind, (px, py, 1.0), label="웅덩이")

    # ── 왼쪽 줄: 가게 둘 + 살림집 하나 ──────────────────────────
    for (i, kind, lit) in ((1, "Shop", True), (2, "Home", False), (3, "Shop", True)):
        x = xc(i)
        C.place("SM_Rasel_Facade_%s" % kind, (x, FAC_Y, 0.0), yaw=0.0, label="파사드_L%d" % i)
        C.place("SM_Rasel_Win_Unit_Lit" if lit else "SM_Rasel_Win_Unit",
                (x, FAC_Y - 6.0, STOREY + 40.0), yaw=0.0, label="창_L%d" % i)
        C.place("SM_Rasel_Eave", (x, FAC_Y - 40.0, 600.0), yaw=0.0, label="처마_L%d" % i)
        if kind == "Shop":
            C.place("SM_Rasel_Awning_A" if i == 1 else "SM_Rasel_Awning_B",
                    (x, FAC_Y - 60.0, 262.0), yaw=0.0, label="차양_L%d" % i)
            C.place("SM_Rasel_ShopSign", (x + 150.0, FAC_Y - 60.0, 300.0), yaw=0.0, label="간판_L%d" % i)
            C.place("SM_Rasel_HangLantern", (x, FAC_Y - 70.0, 560.0), yaw=0.0, label="처마 등롱_L%d" % i)
            C.point_light((x, FAC_Y - 70.0, 500.0), 1400.0, (1.0, 0.72, 0.40), 620.0, "등롱빛_L%d" % i)

    # ── 오른쪽 줄: 물러선 집 둘(깊이감) ─────────────────────────
    for (i, kind) in ((2, "Tall"), (4, "Low")):
        x = xc(i)
        C.place("SM_Rasel_Facade_%s" % kind, (x, -FAC_Y, 0.0), yaw=180.0, label="파사드_R%d" % i)
        C.place("SM_Rasel_Win_Unit", (x, -FAC_Y + 6.0, STOREY + 40.0), yaw=180.0, label="창_R%d" % i)
        C.place("SM_Rasel_Eave", (x, -FAC_Y + 40.0, 600.0), yaw=180.0, label="처마_R%d" % i)

    # ── 세간: 벽보판(도는 그림이 붙는 자리) · 좌판 · 통 ──────────
    C.place("SM_Rasel_NoticeBoard", (xc(2) + 60.0, FAC_Y - 120.0, 0.0), yaw=0.0, label="벽보판")
    C.place("SM_Rasel_Stall_A", (xc(3) + 40.0, -330.0, 0.0), yaw=180.0, label="좌판")
    C.place("SM_Rasel_Barrel", (xc(1) + 180.0, -300.0, 0.0), label="통")
    C.place("SM_Rasel_Crate", (xc(3) - 150.0, 330.0, 0.0), label="궤짝")
    C.place("SM_Rasel_RopeCoil", (xc(2) - 120.0, -340.0, 0.0), label="사린 밧줄")
    C.place("SM_Rasel_Lantern", (xc(4) - 100.0, 300.0, 0.0), label="세운 등")
    C.point_light((xc(4) - 100.0, 300.0, 180.0), 900.0, (1.0, 0.68, 0.34), 520.0, "세운 등빛")

    # ── 밤 하늘·안개·노출 ───────────────────────────────────────
    C.sky_and_exposure((xc(2), 0.0), "night")

    # ── 시작 화면 카메라 — 거리를 비스듬히 훑는 자리 ─────────────
    A = C.A()
    cam = A.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(-620.0, -520.0, 300.0))
    cam.set_actor_rotation(unreal.Rotator(0.0, -6.0, 28.0), False)
    try:
        cam.camera_component.set_field_of_view(58.0)
    except Exception:
        pass
    cam.set_actor_label("TITLE_CAMERA")

    ps = A.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-620.0, -520.0, 120.0))
    ps.set_actor_label("PlayerStart")

    n = C.finish_level()
    C.log("시작 화면 다시 지음 — 부재 %d개, 전부 우리 키트" % n)


build()
