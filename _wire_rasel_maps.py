# -*- coding: utf-8 -*-
# ▶ 라셀 플레이 레벨 배선기 — 블록아웃 위에 정본의 "연결·짚을 것·사는 사람"을 실제 액터로 얹는다.
#   정본: C:/Secret_Project/기획/02_게임설계/2_레벨/게임_플레이레벨_정본.md  (§2 로스터의 나가는 곳 / 짚을 것 / 사는 사람)
#
#   _make_rasel_maps.py 가 세운 빈 블록아웃(바닥·벽·기둥)을 지우지 않고 그대로 연다(load_level).
#   그 위에:
#     · 나가는 곳  → APortalActor  (TargetLevelName = 목표 맵 이름)
#     · 전차역     → AFastTravelPointActor  (구역 이동 허브)
#     · 거점 취침  → ASavePointActor (bAdvanceDayOnRest — 자면 날짜 넘김)
#     · 짚을 것    → ALoreNoteActor (게시판·장부·새김·안내 — 조사 텍스트)
#     · 잠긴 문    → ALockedGateActor (GateId·필요 아이템)
#     · 사는 사람  → AANPCCharacter (역할 이름 + 대사 + 출현 시간대)  ※ 이름 없는 '역할' — 인물 대진표 아님
#   기존 배선 액터(이 여섯 클래스)는 먼저 지우고 다시 얹어 재실행이 중복을 만들지 않게 한다.
#
#   실행(레벨 하나당 프로세스 하나 — new_level 반복/액터 전멸이 엔진을 죽이므로):
#     RASEL_LEVEL=<0..22> UnrealEditor-Cmd.exe <uproject> \
#         -ExecutePythonScript="C:/Secret_Project/_wire_rasel_maps.py" -RenderOffScreen -unattended -nosplash
#   전체: _wire_rasel_maps.sh
import os
import math
import unreal

# 규모 → 대략 크기(uu) — 생성기와 동일해야 위치가 블록아웃 안에 떨어진다
SIZE = {"S": (900, 700), "M": (2400, 1600), "L": (4200, 3000)}
WALL_H = 400

# (맵경로, 종류, 규모, 사람이 읽는 이름) — _make_rasel_maps.py 와 같은 순서·같은 인덱스
LEVELS = [
    ("/Game/Maps/Rasel/L01_Jangteo_Street",     "street", "M", "L01 장터 큰길"),
    ("/Game/Maps/Rasel/L02_Antique_Shop",       "room",   "S", "L02 골동상 안"),
    ("/Game/Maps/Rasel/L03_Backalley",          "alley",  "S", "L03 장터 뒷골목"),
    ("/Game/Maps/Rasel/L04_Rooftop_Room",       "room",   "S", "L04 셋집 옥탑 (거점)"),
    ("/Game/Maps/Rasel/L05_Eatery",             "room",   "S", "L05 밥집 두 그릇"),
    ("/Game/Maps/Rasel/L06_Pawnshop_Back",      "room",   "S", "L06 전당포 뒷방"),
    ("/Game/Maps/Rasel/L07_Tram_Jangteo",       "hall",   "S", "L07 전차역 승강장"),
    ("/Game/Maps/Rasel/L08_Academy_Street",     "street", "M", "L08 학당 앞 거리"),
    ("/Game/Maps/Rasel/L09_Academy_Hall",       "hall",   "M", "L09 교실동"),
    ("/Game/Maps/Rasel/L10_Clinic",             "hall",   "M", "L10 의원 병동"),
    ("/Game/Maps/Rasel/L11_Riverbank",          "open",   "M", "L11 학당가 강둑"),
    ("/Game/Maps/Rasel/L12_Selan_Court",        "court",  "M", "L12 셀란 수련원 안뜰"),
    ("/Game/Maps/Rasel/L13_Newport_Site",       "open",   "L", "L13 신항 공사판"),
    ("/Game/Maps/Rasel/L14_Newport_Shaft",      "shaft",  "M", "L14 굴착 갱"),
    ("/Game/Maps/Rasel/L15_Warehouse_Roof",     "open",   "M", "L15 부두 창고 지붕"),
    ("/Game/Maps/Rasel/L16_Tower_Lobby",        "court",  "M", "L16 유리탑 로비"),
    ("/Game/Maps/Rasel/L17_Sanatorium",         "hall",   "L", "L17 재단 요양원"),
    ("/Game/Maps/Rasel/L18_Council_Hall",       "hall",   "M", "L18 의원 회관 뒷복도"),
    ("/Game/Maps/Rasel/L19_Abandoned_Platform", "hall",   "M", "L19 폐선 승강장"),
    ("/Game/Maps/Rasel/L20_Sewer_Junction",     "alley",  "L", "L20 지하수로 갈래"),
    ("/Game/Maps/Rasel/L21_Underlayer_Gallery", "court",  "L", "L21 아래층 회랑"),
    ("/Game/Maps/Rasel/L22_Ruin_Gate",          "open",   "L", "L22 무너진 성문 앞뜰"),
    ("/Game/Maps/Rasel/L23_Ruin_Keep",          "court",  "L", "L23 빈 성 안"),
]

# 짧은 맵 이름(OpenLevel 목표) — 인덱스로 참조
SHORT = [p.rsplit("/", 1)[1] for (p, _, _, _) in LEVELS]

# ── 정본 배선표 (§2 로스터 그대로) ─────────────────────────────────────────
#   exits : 나가는 곳 (짧은 맵 이름)
#   ft    : 전차역이면 (PointId, 표시이름)
#   save  : 거점 취침점이면 True
#   lore  : 짚을 것 [(제목, [본문 줄...])]
#   gate  : 잠긴 문 [(GateId, 필요아이템Id)]
#   npc   : 사는 사람 [(역할이름, [대사...], 출현시간대 или None=항상, shopKind или None)]
D, N, E, M = "Day", "Night", "Evening", "Morning"
FIX = {
    0: {  # L01 장터 큰길
        "exits": ["L02_Antique_Shop", "L03_Backalley", "L04_Rooftop_Room",
                  "L05_Eatery", "L06_Pawnshop_Back", "L07_Tram_Jangteo"],
        "lore": [("벽보판", ["도시 소문과 의뢰가 붙는 판.",
                            "신항 공사장에서 밤마다 등불이 오르내린다는 말.",
                            "의원에 실려 온 사람이 늘었다는 말."])],
        "npc": [("좌판 상인", ["오늘 물건은 좋아. 골라 봐.", "밤엔 셔터 내리니까 낮에 와."], D, "convenience"),
                ("골동상 호객꾼", ["안쪽에 진짜가 있어. 감정도 해 주고."], D, None),
                ("뜨내기", ["뱃말 섞어 쓰는 자들이 요즘 부쩍 늘었어."], N, None)],
    },
    1: {  # L02 골동상 안
        "exits": ["L01_Jangteo_Street"],
        "lore": [("감정대", ["가진 낱장을 여기 올리면 진위를 봐 준다.",
                           "가짜에 진짜가 한 장씩 섞여 들어온다."])],
        "gate": [("L02_BackRoom", "AntiqueToken")],
        "npc": [("골동상 주인", ["뭘 팔러 왔나, 사러 왔나.", "출처는 안 묻는다. 값만 맞으면."], None, "gear")],
    },
    2: {  # L03 장터 뒷골목
        "exits": ["L01_Jangteo_Street", "L06_Pawnshop_Back", "L19_Abandoned_Platform"],
        "lore": [("담벼락 낙서", ["누가 분필로 그은 표식.", "잿마당 사람들이 길을 남기는 방식이라 한다."])],
        "npc": [("밤 중개인", ["낮엔 없다. 이 시간에만 얘기해.", "물건 있으면 보여 봐."], N, None)],
    },
    3: {  # L04 셋집 옥탑 (거점)
        "exits": ["L01_Jangteo_Street"],
        "save": True,
        "lore": [("책상 위 연공서", ["익힌 것을 되짚는 자리.", "펼치면 오늘까지 세운 길이 정리돼 있다."]),
                 ("문 앞 쪽지", ["누가 문틈에 끼워 둔 쪽지.", "'장터에서 보자. 낮에.'"])],
        "npc": [("아래층 주인 노인", ["방세는 밀리지 말고.", "밤엔 조용히 다녀."], None, None)],
    },
    4: {  # L05 밥집 두 그릇
        "exits": ["L01_Jangteo_Street"],
        "lore": [("옆자리 이야기", ["앉아 있으면 도시 소문이 가장 먼저 도는 자리.",
                              "요 며칠 학당 쪽이 시끄럽다는 말."])],
        "npc": [("밥집 주인 내외", ["두 그릇 시키면 한 그릇은 덤이야.", "앉아서 좀 쉬다 가."], None, "cafe")],
    },
    5: {  # L06 전당포 뒷방
        "exits": ["L01_Jangteo_Street", "L03_Backalley"],
        "lore": [("전당포 장부", ["맡기고 간 물건들의 기록.",
                             "이름 없이 표식만 적힌 줄이 몇 개 있다."])],
        "npc": [("전당포 주인", ["출처는 안 묻는다니까.", "값이 센 물건은 뒷문으로 와."], None, "gear")],
    },
    6: {  # L07 전차역 승강장 — 구역 이동 허브
        "exits": ["L01_Jangteo_Street", "L08_Academy_Street", "L13_Newport_Site",
                  "L16_Tower_Lobby", "L19_Abandoned_Platform"],
        "ft": ("Tram_Jangteo", "아랫장터 승강장"),
        "lore": [("노선도", ["지하 열두 갈래.", "승강장 끝 철문 너머는 폐선 구간으로 이어진다."])],
        "gate": [("L07_OldGate", "TramKey")],
    },
    7: {  # L08 학당 앞 거리
        "exits": ["L07_Tram_Jangteo", "L09_Academy_Hall", "L10_Clinic",
                  "L11_Riverbank", "L12_Selan_Court"],
        "lore": [("전단", ["갓 눈뜬 자를 찾는 강습 광고.",
                         "그 아래 작게 붙은 실종자 전단 몇 장."])],
        "npc": [("학생", ["수업 종 치면 다들 흩어져.", "밤엔 강둑에 아무도 없어."], D, None),
                ("전단 붙이는 사람", ["한 장 봐 줘. 사람 찾는 거야."], None, None)],
    },
    8: {  # L09 교실동
        "exits": ["L08_Academy_Street"],
        "lore": [("칠판", ["지운 자국 위에 다시 쓴 글씨.", "누가 살란 낱말을 연습하다 지운 흔적."]),
                 ("책상 서랍", ["쪽지 한 장.", "'방과 후 빈 강의실. 아무도 안 봐.'"])],
        "npc": [("강사", ["늦게까지 남는 학생이 요즘 늘었어."], D, None),
                ("늦게 남는 학생", ["혼자 시험해 볼 게 있어서."], E, None)],
    },
    9: {  # L10 의원 병동
        "exits": ["L08_Academy_Street"],
        "lore": [("접수 장부", ["발작·중독·헛것으로 실려 온 이름들.",
                            "같은 증상이 며칠 새 부쩍 늘었다."])],
        "gate": [("L10_BackDoor", "ClinicKey")],
        "npc": [("당직 의사", ["밤엔 당직실 하나만 켜 둬.", "요즘 실려 오는 게 예사 병이 아니야."], None, None),
                ("병상의 환자", ["…불이 보여. 눈 감아도 보여."], None, None)],
    },
    10: {  # L11 학당가 강둑
        "exits": ["L08_Academy_Street"],
        "lore": [("둑 안내판", ["물이 바로 옆이라 불도 얼음도 마음 놓고 세운다.",
                            "밤이면 사람이 없어 혼자 힘을 켜 보는 자리."])],
        "npc": [("밤낚시 노인", ["밤에 여기 오는 건 낚시꾼이랑 자네뿐이야."], N, None)],
    },
    11: {  # L12 셀란 수련원 안뜰
        "exits": ["L08_Academy_Street"],
        "lore": [("게시판", ["명상·호흡 강습 일정.", "회비를 내면 누구나 안뜰까지는 들어온다."])],
        "gate": [("L12_InnerHall", "SelanToken")],
        "npc": [("수련원 강사", ["숨을 고르는 것부터. 길은 그다음이야.", "안채는 아직 자네가 들 자리가 아냐."], D, None),
                ("수련생", ["여기 마루가 매끈해서 시합엔 딱이야."], D, None)],
    },
    12: {  # L13 신항 공사판
        "exits": ["L07_Tram_Jangteo", "L14_Newport_Shaft", "L15_Warehouse_Roof"],
        "lore": [("굴착 도면", ["옛 부두 아래를 파 내려간 층위 그림.",
                            "지하 열여덟 자에 석축이 드러났다고 표시돼 있다."])],
        "gate": [("L13_SiteGate", "SitePass")],
        "npc": [("현장 감독", ["낮엔 출입 통제야. 명분 없이는 못 들어와.", "새 조각이 여기서 실제로 나와."], D, None),
                ("경비", ["밤엔 조명탑 몇 개랑 나뿐이야."], N, None)],
    },
    13: {  # L14 굴착 갱
        "exits": ["L13_Newport_Site", "L20_Sewer_Junction"],
        "lore": [("옛 석축", ["굴착 먼지 너머 드러난 오래된 돌쌓기.", "함이 있던 자리만 비어 있다."]),
                 ("더 아래로 뚫린 틈", ["갱 끝에서 아래로 더 내려가는 틈.", "수로 그물로 이어진다."])],
    },
    14: {  # L15 부두 창고 지붕
        "exits": ["L13_Newport_Site"],
        "lore": [("망원", ["항구 전체가 내려다보인다.", "어느 배가 무엇을 내리는지 셀 수 있다."])],
        "npc": [("밤에 올라오는 자", ["…서로 못 본 걸로 하지."], N, None)],
    },
    15: {  # L16 유리탑 로비
        "exits": ["L07_Tram_Jangteo", "L17_Sanatorium", "L18_Council_Hall"],
        "lore": [("방문자 명부", ["대리석 데스크의 명부.", "약속 없이 위층으로 오른 이름은 없다 — 겉으로는."])],
        "gate": [("L16_Elevator", "TowerPass")],
        "npc": [("안내원", ["약속 없으면 위로는 못 올라갑니다.", "대다수는 제가 무엇의 일부인지도 몰라요."], D, None),
                ("경비", ["파국이 나기 전엔 여기서 싸움 안 나."], None, None)],
    },
    16: {  # L17 재단 요양원
        "exits": ["L16_Tower_Lobby"],
        "lore": [("입소자 명부", ["'가라앉히러' 데려온 이들의 이름.",
                            "눈이 풀린 사람과 멀쩡한 사람이 섞여 있다."])],
        "gate": [("L17_Basement", "SanatoriumKey")],
        "npc": [("요양원 원장", ["여긴 조용히 쉬는 곳입니다.", "밤엔 안 열리던 문이 열리죠."], D, None),
                ("간호인", ["소각로 쪽으로도 지하가 붙어 있어요."], None, None)],
    },
    17: {  # L18 의원 회관 뒷복도
        "exits": ["L16_Tower_Lobby"],
        "lore": [("회의 일정표", ["여기서 오간 말이 며칠 뒤 거리에서 사람 목숨이 된다.",
                            "문틈으로 새는 말을 청소부가 가장 많이 듣는다."])],
        "npc": [("청소부", ["나만큼 이 복도를 오래 보는 사람도 없지.", "문틈으로 별말이 다 새어 나와."], None, None)],
    },
    18: {  # L19 폐선 승강장
        "exits": ["L07_Tram_Jangteo", "L03_Backalley", "L20_Sewer_Junction"],
        "lore": [("불 피운 자리", ["여기 사는 사람들이 남긴 자리.", "도시가 세지 않는 사람들이다."])],
        "npc": [("승강장 사람", ["여기도 사람 사는 데야. 조용히 지나가."], None, None)],
    },
    19: {  # L20 지하수로 갈래
        "exits": ["L19_Abandoned_Platform", "L14_Newport_Shaft", "L21_Underlayer_Gallery"],
        "lore": [("벽에 그은 표식", ["누가 먼저 지나갔는지 남긴 표식.",
                              "발굴가와 회수조가 가장 자주 마주치는 어둠."])],
    },
    20: {  # L21 아래층 회랑
        "exits": ["L20_Sewer_Junction", "L22_Ruin_Gate"],
        "lore": [("기둥의 새김", ["줄지어 선 기둥마다 살란 글이 새겨져 있다.",
                            "방 하나가 아니라 회랑이 이어진다 — 도시 규모다.",
                            "사람은 없는데 무언가 아직 돌아간다."])],
        "gate": [("L21_SealedDoor", "GalleryToken")],
    },
    21: {  # L22 무너진 성문 앞뜰
        "exits": ["L21_Underlayer_Gallery", "L23_Ruin_Keep"],
        "lore": [("성문 새김", ["여기서부터 도시의 법이 안 닿는다."]),
                 ("앞서 온 자들의 흔적", ["버려진 야영 자리와 장비.", "돌아간 흔적은 없다."])],
    },
    22: {  # L23 빈 성 안
        "exits": ["L22_Ruin_Gate"],
        "lore": [("아직 도는 물건들", ["주인 없이도 제 일을 하고 있는 고대의 물건.",
                              "건드리면 반응한다.",
                              "그 곁에서 잔 자가 아침에 없어졌다."])],
    },
}


def log(m):
    unreal.log("[WireRasel] " + m)
    try:
        with open("C:/Secret_Project/Saved/rasel_wire.log", "a", encoding="utf-8") as f:
            f.write(m + "\n")
    except Exception:
        pass


_AS = None


def actor_sys():
    global _AS
    if _AS is None:
        _AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return _AS


def resolve(name):
    """게임 모듈 UCLASS를 Python 바인딩 또는 /Script 경로로 해석."""
    c = getattr(unreal, name, None)
    if c is not None:
        return c
    return unreal.load_class(None, "/Script/Secret_Project." + name)


def spawn(cls, loc, rot=None):
    return actor_sys().spawn_actor_from_class(cls, unreal.Vector(*loc),
                                              rot or unreal.Rotator(0, 0, 0))


def setp(actor, prop, val):
    try:
        actor.set_editor_property(prop, val)
        return True
    except Exception as e:
        log("  ! set %s 실패: %s" % (prop, e))
        return False


def ring_positions(n, w, d, inset=250, z=100):
    """블록아웃 안쪽 둘레를 따라 n개 지점을 고르게. 출입 포탈을 벽 앞에 흩는다."""
    if n <= 0:
        return []
    hw, hd = w / 2.0 - inset, d / 2.0 - inset
    pts = []
    for i in range(n):
        t = (i + 0.5) / n * 2.0 * math.pi
        # 사각 둘레에 사상
        x = hw * math.cos(t)
        y = hd * math.sin(t)
        # 벽 쪽으로 밀어 붙임
        if abs(math.cos(t)) > abs(math.sin(t)):
            x = hw * (1 if math.cos(t) > 0 else -1)
        else:
            y = hd * (1 if math.sin(t) > 0 else -1)
        pts.append((x, y, z))
    return pts


def clear_prior_wiring():
    """이전 배선 액터만 지운다(블록아웃 지오메트리는 보존). 클래스 이름으로 판별."""
    kill = {"PortalActor", "FastTravelPointActor", "SavePointActor",
            "LoreNoteActor", "LockedGateActor", "ANPCCharacter"}
    removed = 0
    for a in actor_sys().get_all_level_actors():
        cls = a.get_class()
        # 자기 클래스 또는 부모 사슬에 배선 클래스가 있으면 제거(BP 변형 대비)
        name = cls.get_name()
        hit = name in kill
        if not hit:
            try:
                p = cls.get_super_class()
                while p is not None:
                    if p.get_name() in kill:
                        hit = True
                        break
                    p = p.get_super_class()
            except Exception:
                pass
        if hit:
            try:
                actor_sys().destroy_actor(a)
                removed += 1
            except Exception:
                pass
    return removed


def phase_enum(name):
    # UENUM EDayPhase → 파이썬 바인딩은 unreal.DayPhase (E 접두사 제거)
    if name is None:
        return None
    enum = getattr(unreal, "DayPhase", None)
    if enum is None:
        return None
    return getattr(enum, name.upper(), None)


def wire(idx):
    path, kind, sz, name = LEVELS[idx]
    w, d = SIZE[sz]
    spec = FIX.get(idx, {})

    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level(path)
    removed = clear_prior_wiring()

    counts = {"portal": 0, "ft": 0, "save": 0, "lore": 0, "gate": 0, "npc": 0}

    # 1) 나가는 곳 → 포탈 (둘레에 흩어 배치)
    exits = spec.get("exits", [])
    epos = ring_positions(len(exits), w, d, inset=260, z=100)
    Portal = resolve("PortalActor")
    for tgt, p in zip(exits, epos):
        a = spawn(Portal, p)
        if a:
            setp(a, "TargetLevelName", tgt)
            a.set_actor_label("→ " + tgt)
            counts["portal"] += 1

    # 2) 전차역
    ft = spec.get("ft")
    if ft:
        FT = resolve("FastTravelPointActor")
        a = spawn(FT, (0, 0, 100))
        if a:
            setp(a, "PointId", ft[0])
            setp(a, "PointName", ft[1])
            a.set_actor_label("전차역 " + ft[1])
            counts["ft"] += 1

    # 3) 거점 취침 세이브
    if spec.get("save"):
        SP = resolve("SavePointActor")
        a = spawn(SP, (-w / 2 + 300, d / 2 - 300, 60))
        if a:
            setp(a, "bRestoreOnSave", True)
            setp(a, "bAdvanceDayOnRest", True)
            a.set_actor_label("침상 (취침·저장)")
            counts["save"] += 1

    # 4) 짚을 것 → 조사 노트 (북쪽 벽 안쪽에 줄지어)
    lore = spec.get("lore", [])
    Lore = resolve("LoreNoteActor")
    for i, (title, lines) in enumerate(lore):
        y = (-d / 2 + 300) + (d - 600) * (i + 1) / (len(lore) + 1.0)
        a = spawn(Lore, (w / 2 - 260, y, 120))
        if a:
            setp(a, "Title", title)
            setp(a, "Lines", list(lines))
            a.set_actor_label("조사: " + title)
            counts["lore"] += 1

    # 5) 잠긴 문
    gates = spec.get("gate", [])
    Gate = resolve("LockedGateActor")
    for i, (gid, item) in enumerate(gates):
        a = spawn(Gate, (w / 2 - 120, (-d / 4) + i * (d / 3), WALL_H / 2))
        if a:
            setp(a, "GateId", gid)
            setp(a, "RequiredItemId", item)
            setp(a, "bConsumeKey", False)
            a.set_actor_label("잠긴 문 " + gid)
            counts["gate"] += 1

    # 6) 사는 사람 → 역할 NPC (가운데 줄에 흩어). 이름은 '역할' — 인물 대진표 아님.
    npcs = spec.get("npc", [])
    NPC = resolve("ANPCCharacter")
    for i, entry in enumerate(npcs):
        role, lines, phase, shopkind = entry
        x = (-w / 2 + 500) + (w - 1000) * (i + 0.5) / max(1, len(npcs))
        a = spawn(NPC, (x, 0, 120))
        if a:
            setp(a, "NPCName", role)
            setp(a, "DialogueLines", lines)
            setp(a, "bCanEnterCombat", False)
            if shopkind:
                setp(a, "bIsShopkeeper", True)
                setp(a, "ShopKind", shopkind)
                setp(a, "bClosedAtNight", True)
            ph = phase_enum(phase)
            if ph is not None:
                try:
                    a.set_editor_property("ActivePhases", [ph])
                except Exception:
                    pass
            a.set_actor_label("NPC " + role)
            counts["npc"] += 1

    les.save_current_level()
    log("OK  %s  지움%d  포탈%d 전차%d 취침%d 조사%d 문%d NPC%d" %
        (name, removed, counts["portal"], counts["ft"], counts["save"],
         counts["lore"], counts["gate"], counts["npc"]))


def main():
    idx = os.environ.get("RASEL_LEVEL")
    if idx is None:
        log("RASEL_LEVEL 없음 — 0~%d 지정." % (len(LEVELS) - 1))
        return
    wire(int(idx))


main()
