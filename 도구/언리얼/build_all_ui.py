# -*- coding: utf-8 -*-
# =============================================================================
#  build_all_ui.py  —  전 UI 위젯 자동 생성 + 클래스 참조 자동 배선 (A 작성)
#
#  ★ 사용법 (언리얼 에디터에서, C++ 빌드가 먼저 그린이어야 함):
#      에디터 → Output Log 하단 콘솔을 "Python"으로 → 아래 입력:
#         py "C:/Secret_Project/build_all_ui.py"
#
#  하는 일 (전부 멱등 — 여러 번 돌려도 안전):
#    1) /Game/UI 아래 모든 WBP_* 위젯 BP 생성(+부모 C++ 클래스로 reparent)
#    2) 비어있는 위젯 내부를 BindWidget 이름 그대로 버튼/텍스트/패널 자동 배치
#       (이미 내용 있으면 건드리지 않음 — 사용자가 디자인한 위젯 보존)
#    3) 클래스 참조 자동 배선:
#         - WBP_SystemMenu: 11개 하위 위젯 클래스(인벤/퀘스트/도감/인연/은행/제작/업적/사회/상태/발견)
#         - WBP_TalkUserWidget(기존 작동 대화창): ShopWidgetClass = WBP_Shop (빈칸일 때만)
#         - BP_PlayerCharacter(있으면): DialogueWidgetClass/SystemMenuWidgetClass/WorldHUDClass (빈칸만 채움)
#       ※ 이미 연결된 참조는 절대 덮어쓰지 않음(overwrite=False) — 작동 중인 전투/대화 보존.
#                                       + StoryDirector 컴포넌트 부착 + StoryWidgetClass
#         - BP_BattleManager(있으면): BattleHUDClass = WBP_BattleHUD
#    4) 결과를 로그로 출력 — 안 된 항목은 [MANUAL] 로 표시(사용자/Claude가 보고 처리)
# =============================================================================

import unreal

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
BEL = unreal.BlueprintEditorLibrary
AR  = unreal.AssetRegistryHelpers.get_asset_registry()
UI_PATH = "/Game/UI"

def log(msg):   print("[build_all_ui] %s" % msg)
def manual(msg):print("[MANUAL] %s" % msg)

# ★ 강제 재생성: True면 기존 위젯 내부를 비우고 새 레이아웃으로 다시 만듦(변수연동 일관성).
#   KEEP에 든 위젯은 강제재생성에서 제외(사용자 디자인 보존).
FORCE_REBUILD = False  # 06-05 타이틀 시스루 레이아웃 반영 후 끔(다음 실행이 디자인 안 덮게)
KEEP = set()   # 예: {"WBP_TalkUserWidget"} — 직접 디자인한 위젯 지키려면 여기 추가

# -----------------------------------------------------------------------------
# 위젯 정의: (WBP 이름, 부모 C++ 클래스명, 자식 배치 스펙)
#   스펙 항목:
#     ("title", name, text)            제목 TextBlock(텍스트 지정)
#     ("text",  name)                  빈 TextBlock(C++가 갱신)
#     ("btn",   name, label)           버튼(안에 라벨 TextBlock)
#     ("optbtn", btn_pfx, txt_pfx, n)  Btn_X0..Xn-1 (각 안에 Txt_X0..) — 보유/선택지용
#     ("panelopt", panel, btn_pfx, txt_pfx, n)  이름붙은 VerticalBox + 그 안에 optbtn
#   빈 리스트 = 자동배치 안 함(사용자 디자인 보존 — 예: WBP_BattleHUD는 별도 빌더).
# -----------------------------------------------------------------------------
WIDGETS = [
 ("WBP_MainMenu", "MainMenuWidget", "MAINMENU"),

 ("WBP_BattleHUD", "BattleHUDWidget", "BATTLEHUD"),   # 전용 빌더

 ("WBP_DamageNumber", "DamageNumberWidget", "DMGNUM"),   # 데미지 숫자(타격감)

 ("WBP_BattleFlair", "BattleFlairWidget", "BATTLEFLAIR"),   # 페르소나 플레어 배너(WEAK!/1 MORE!)

 ("WBP_Shop", "ShopWidget", "SHOP"),   # 전용 UI: 판매 항목 동적 클릭 카드

 ("WBP_Inventory", "InventoryWidget", "INVENTORY"),   # 전용 UI: 아이템 카드 동적 생성

 ("WBP_QuestLog", "QuestLogWidget", "QUESTS"),   # 전용 UI: 퀘스트 카드+진행바

 ("WBP_SystemMenu", "SystemMenuWidget", "SYSMENU"),   # (레거시) 카테고리+2열 그리드 박스 메뉴

 ("WBP_MenuFlow", "MenuFlowWidget", "MENUFLOW"),   # ★ 풀스크린 M 메뉴(P5식 사선 와이프 takeover)

 ("WBP_Status", "StatusWidget", "STATUS"),   # 전용 UI: 능력 게이지 + 스탯 카드

 ("WBP_Discovery", "DiscoveryWidget", "DISCOVERY"),   # 전용 UI: 지역 카드

 ("WBP_Help", "HelpWidget", [
    ("text","Txt_Help"),("btn","Btn_Close","닫기")]),

 ("WBP_Bond", "BondWidget", "BONDS"),   # 전용 UI: 인연 게이지 카드

 ("WBP_Bestiary", "BestiaryWidget", "BESTIARY"),   # 전용 UI: 적 카드

 ("WBP_FastTravel", "FastTravelWidget", "FASTTRAVEL"),   # 전용 UI: 목적지 동적 클릭 카드

 ("WBP_Bank", "BankWidget", [
    ("title","Txt_OnHand","소지금: 0"),("text","Txt_Stored"),
    ("btn","Btn_Deposit100","100 예치"),("btn","Btn_DepositAll","전액 예치"),
    ("btn","Btn_Withdraw100","100 인출"),("btn","Btn_WithdrawAll","전액 인출"),
    ("btn","Btn_Close","닫기")]),

 ("WBP_Crafting", "CraftingWidget", "CRAFTING"),   # 전용 UI: 레시피 동적 클릭 카드

 ("WBP_Achievements", "AchievementWidget", "ACHIEVEMENTS"),   # 전용 UI: 업적 카드+진행바

 ("WBP_Social", "SocialStatsWidget", "SOCIAL"),   # 전용 UI: 스탯 게이지 카드 동적 생성

 ("WBP_WorldHUD", "WorldHUDWidget", "WORLDHUD"),

 ("WBP_Equipment", "EquipmentWidget", "EQUIPMENT"),   # 전용 UI: 장비 동적 클릭 카드

 ("WBP_Story", "StoryWidget", [
    ("image","Img_Illust"),("image","Img_Portrait"),
    ("title","Txt_Title","스토리"),("text","Txt_Speaker"),("text","Txt_Line"),
    ("text","Txt_Progress"),
    ("btn","Btn_Next","▶ 다음"),("btn","Btn_Skip","건너뛰기")]),

 ("WBP_StoryJournal", "StoryJournalWidget", [
    ("title","Txt_Title","회상 저널"),("text","Txt_Entries"),
    ("optbtn","Btn_Beat","Txt_Beat",8),
    ("btn","Btn_Close","닫기")]),

 ("WBP_Settings", "SettingsWidget", [
    ("title","Txt_Title","설정"),
    ("text","Txt_Master"),("slider","Slider_Master"),
    ("text","Txt_Bgm"),("slider","Slider_Bgm"),
    ("text","Txt_Sfx"),("slider","Slider_Sfx"),
    ("btn","Btn_WindowMode","화면 모드"),("text","Txt_WindowMode"),
    ("checkbox","Chk_VSync"),
    ("btn","Btn_ResPrev","◀"),("text","Txt_Res"),("btn","Btn_ResNext","▶"),
    ("btn","Btn_TextPrev","◀"),("text","Txt_TextSpeed"),("btn","Btn_TextNext","▶"),
    ("btn","Btn_Language","언어"),("text","Txt_Language"),
    ("btn","Btn_Apply","적용"),("btn","Btn_Reset","기본값"),("btn","Btn_Close","닫기")]),

 # WBP_TalkUserWidget: 사용자가 이미 만들어 작동 중인 대화창(=현재 페르소나 전투 진입에 쓰임).
 # 이미 존재+내용 있으면 자동배치 생략(보존). 새로 만들지 않음(이름 정확히 일치시켜 기존 것 타겟).
 ("WBP_TalkUserWidget", "TalkUserWidget", "TALK"),
]

# -----------------------------------------------------------------------------
# 위젯 트리 빌드 헬퍼
# -----------------------------------------------------------------------------
# ★ UE5.7은 Python에서 WidgetTree/RootWidget 접근을 막음 → C++ UIBuilderLibrary로 트리 조작.
def has_uib():
    return hasattr(unreal, "UIBuilderLibrary")

def mk(bp, cls, name):
    # bp의 트리에 cls 위젯을 name으로 생성(변수로) → 반환
    try: return unreal.UIBuilderLibrary.add_widget(bp, cls.static_class(), name)
    except Exception as e:
        log("add_widget(%s) 실패: %s" % (name, e)); return None

def add(parent, child):
    if parent is not None and child is not None:
        try: unreal.UIBuilderLibrary.add_child(parent, child)
        except Exception as e: log("add_child 실패: %s" % e)

def set_text(w, txt):
    if w:
        try: unreal.UIBuilderLibrary.set_widget_text(w, txt)
        except Exception: pass

def set_root(bp, root):
    try: unreal.UIBuilderLibrary.set_root(bp, root)
    except Exception as e: log("set_root 실패: %s" % e)

def labeled_button(bp, parent, btn_name, label, txt_name=None):
    b = mk(bp, unreal.Button, btn_name); add(parent, b)
    set_btn(b)
    t = mk(bp, unreal.TextBlock, txt_name if txt_name else ("Lbl_%s" % btn_name))
    set_text(t, label); set_style(t, COL_BTNTXT, 18, JC); add(b, t)
    slot_pad(t, 14, 9, 14, 9)   # 라벨 패딩 → 버튼 높이/여백
    return b

# ── 레이아웃 헬퍼 (C++ 슬롯 조작) ──
def V2(x, y):
    return unreal.Vector2D(x, y)

def set_slot(child, x, y, w, h, amin=(0.0, 0.0), amax=(0.0, 0.0), align=(0.0, 0.0)):
    if child is None: return
    try:
        unreal.UIBuilderLibrary.set_canvas_slot(child, V2(x, y), V2(w, h),
                                                V2(amin[0], amin[1]), V2(amax[0], amax[1]), V2(align[0], align[1]))
    except Exception: pass

def collapse(w):
    try: unreal.UIBuilderLibrary.set_collapsed(w, True)
    except Exception: pass

def full_screen(w):
    try: unreal.UIBuilderLibrary.set_canvas_full_screen(w)
    except Exception: pass

# ── 색/스타일 (가독성·대비) ──
def C(r, g, b, a=1.0):
    return unreal.LinearColor(r, g, b, a)

COL_DIM     = C(0.0, 0.0, 0.0, 0.62)    # 팝업 뒤 전체화면 딤
COL_PANEL   = C(0.05, 0.06, 0.10, 0.98) # 팝업 패널 배경(불투명 다크네이비)
COL_FRAME   = C(0.86, 0.18, 0.24, 1.0)  # 패널 강조 테두리(크림슨 accent)
COL_ACCENT  = C(0.86, 0.18, 0.24, 1.0)  # 제목 구분선
COL_BAR     = C(0.05, 0.06, 0.10, 0.86) # 액션바/대화/HUD 배경(반투명)
COL_TITLE   = C(0.98, 0.86, 0.50, 1.0)  # 제목(골드)
COL_TEXT    = C(0.88, 0.90, 0.94, 1.0)  # 본문(밝은 회백)
COL_BTNTXT  = C(0.97, 0.97, 1.0, 1.0)   # 버튼 라벨(흰색)
COL_BTNBG   = C(0.16, 0.19, 0.30, 1.0)  # 버튼 기본
COL_BTNHOV  = C(0.30, 0.36, 0.54, 1.0)  # 버튼 호버(밝게)
COL_BTNPRS  = C(0.10, 0.12, 0.20, 1.0)  # 버튼 눌림(진하게)
COL_SHADOW  = C(0.0, 0.0, 0.0, 0.85)    # 글자 그림자
JL, JC, JR  = 0, 1, 2                    # ETextJustify Left/Center/Right

def set_color(w, col):
    try: unreal.UIBuilderLibrary.set_color(w, col)
    except Exception: pass

# ── 폰트 (노토 산스 KR 임포트 → Font 에셋 → 전 텍스트 적용) ──
FONT_DIR = "/Game/UI/Fonts"
FONT_TTF = "C:/Windows/Fonts/NotoSansKR-VF.ttf"
FONT_OBJ = None   # ensure_font()가 채움. None이면 기본 폰트 유지(무해).

def ensure_font():
    """시스템 NotoSansKR ttf를 UE Font 에셋으로 임포트/연결. 실패해도 기본폰트로 진행."""
    global FONT_OBJ
    fp = "%s/Font_NotoKR" % FONT_DIR
    if EAL.does_asset_exist(fp):
        FONT_OBJ = unreal.load_asset(fp)
        if FONT_OBJ: log("폰트 로드: %s" % fp); return
    try:
        import os
        if not os.path.exists(FONT_TTF):
            log("[폰트] %s 없음 — 기본폰트 유지" % FONT_TTF); return
        if not EAL.does_directory_exist(FONT_DIR): EAL.make_directory(FONT_DIR)
        # 1) ttf → FontFace
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", FONT_TTF)
        task.set_editor_property("destination_path", FONT_DIR)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        task.set_editor_property("replace_existing", True)
        ASSET_TOOLS.import_asset_tasks([task])
        face = None
        try:
            for p in task.get_editor_property("imported_object_paths"):
                o = unreal.load_asset(p)
                if isinstance(o, unreal.FontFace): face = o; break
        except Exception: pass
        if face is None:
            fpath = "%s/NotoSansKR-VF" % FONT_DIR
            if EAL.does_asset_exist(fpath): face = unreal.load_asset(fpath)
        if face is None:
            log("[폰트] FontFace 임포트 실패 — 기본폰트 유지"); return
        # 2) UFont 생성 + composite default typeface에 FontFace 연결
        font = ASSET_TOOLS.create_asset("Font_NotoKR", FONT_DIR, unreal.Font, unreal.FontFactory())
        if font is None: log("[폰트] Font 생성 실패"); return
        try: font.set_editor_property("font_cache_type", unreal.FontCacheType.RUNTIME)
        except Exception: pass
        cf = font.get_editor_property("composite_font")
        tf = cf.get_editor_property("default_typeface")
        entry = unreal.TypefaceEntry()
        entry.set_editor_property("name", "Default")
        fd = unreal.FontData(); fd.set_editor_property("font_face_asset", face)
        entry.set_editor_property("font", fd)
        tf.set_editor_property("fonts", [entry])
        cf.set_editor_property("default_typeface", tf)
        font.set_editor_property("composite_font", cf)
        EAL.save_loaded_asset(font)
        FONT_OBJ = font
        log("폰트 생성+연결 완료: %s" % fp)
    except Exception as e:
        log("[폰트] 처리 실패(%s) — 기본폰트 유지" % e)

def apply_font(w, size):
    if not FONT_OBJ: return
    if not isinstance(w, unreal.TextBlock): return
    try:
        info = w.get_font()
        info.set_editor_property("font_object", FONT_OBJ)
        info.set_editor_property("typeface_font_name", "Default")
        if size and size > 0: info.set_editor_property("size", size)
        w.set_font(info)
    except Exception: pass

def set_style(w, col, size=-1, justify=-1, shadow=True):
    try: unreal.UIBuilderLibrary.set_text_style(w, col, size, justify)
    except Exception: pass
    apply_font(w, size)
    if shadow:
        try: unreal.UIBuilderLibrary.set_text_shadow(w, COL_SHADOW, V2(1.0, 1.0))
        except Exception: pass

# ── 비스듬한 accent 그래픽(페르소나 시그니처) — 회전한 얇은 Border 줄무늬 ──
def add_diagonals(bp, root):
    """딤 위/패널 뒤에 비스듬한 크림슨 줄무늬. 입력 통과(클릭 안막음)."""
    for i, (ang, a, yoff, th) in enumerate([(-20, 0.18, -210, 64),
                                            (-20, 0.11, 30, 130),
                                            (-20, 0.16, 250, 50)]):
        d = mk(bp, unreal.Border, "Diag%d" % i); add(root, d)
        set_slot(d, 0, yoff, 2400, th, amin=(0.5, 0.5), amax=(0.5, 0.5), align=(0.5, 0.5))
        set_color(d, C(0.86, 0.18, 0.24, a))
        try: d.set_render_transform_angle(float(ang))
        except Exception: pass
        try: unreal.UIBuilderLibrary.set_hit_test_invisible(d, False)
        except Exception: pass

def set_pad(w, l, t, r, b):
    try: unreal.UIBuilderLibrary.set_border_padding(w, float(l), float(t), float(r), float(b))
    except Exception: pass

def set_btn(b):
    try: unreal.UIBuilderLibrary.set_button_colors(b, COL_BTNBG, COL_BTNHOV, COL_BTNPRS)
    except Exception: pass

def slot_pad(w, l, t, r, b):
    try: unreal.UIBuilderLibrary.set_slot_padding(w, float(l), float(t), float(r), float(b))
    except Exception: pass

def fill_slot(w):
    """박스 슬롯이 남은 공간을 채우게(SizeRule=Fill)."""
    try:
        scs = unreal.SlateChildSize()
        scs.set_editor_property("size_rule", unreal.SlateSizeRule.FILL)
        scs.set_editor_property("value", 1.0)
        w.slot.set_editor_property("size", scs)
    except Exception: pass

def add_divider(bp, box):
    """제목 아래 accent 구분선(콘텐츠 없는 얇은 Border = 패딩 높이)."""
    d = mk(bp, unreal.Border, "Divider"); add(box, d)
    set_color(d, COL_ACCENT); set_pad(d, 0, 1.5, 0, 1.5)
    slot_pad(d, 0, 2, 0, 10)
    return d

# 스펙 항목들을 세로 박스에 채우는 공용 루틴
def fill_box(bp, box, spec):
    for item in spec:
        kind = item[0]
        if kind == "title":
            t = mk(bp, unreal.TextBlock, item[1]); set_text(t, item[2])
            set_style(t, COL_TITLE, 30, JC); add(box, t); slot_pad(t, 0, 2, 0, 4)
            add_divider(bp, box)   # 제목 아래 accent 구분선
        elif kind == "text":
            t = mk(bp, unreal.TextBlock, item[1]); set_style(t, COL_TEXT, 18, JL)
            add(box, t); slot_pad(t, 2, 5, 2, 5)
        elif kind == "btn":
            if item[1] == "Btn_Close":   # 닫기는 popup_shell 헤더에 고정 생성됨 → 본문 중복 생략
                continue
            b = labeled_button(bp, box, item[1], item[2]); slot_pad(b, 0, 4, 0, 4)
        elif kind == "optbtn":
            _, bpfx, tpfx, n = item
            for i in range(n):
                b = labeled_button(bp, box, "%s%d" % (bpfx, i), "", "%s%d" % (tpfx, i))
                slot_pad(b, 0, 3, 0, 3)
        elif kind == "panelopt":
            _, panel, bpfx, tpfx, n = item
            p = mk(bp, unreal.VerticalBox, panel); add(box, p)
            for i in range(n):
                b = labeled_button(bp, p, "%s%d" % (bpfx, i), "", "%s%d" % (tpfx, i))
                slot_pad(b, 0, 3, 0, 3)
        elif kind == "slider":
            s = mk(bp, unreal.Slider, item[1]); add(box, s); slot_pad(s, 2, 6, 2, 10)
        elif kind == "checkbox":
            c = mk(bp, unreal.CheckBox, item[1]); add(box, c); slot_pad(c, 2, 6, 2, 6)
        elif kind == "image":
            add(box, mk(bp, unreal.Image, item[1]))

def popup_shell(bp, w=720, h=820):
    """팝업 공용 껍데기: RootCanvas → [전체화면 딤] + [중앙 불투명 패널] → 스크롤 콘텐츠.
       반환: 콘텐츠를 채울 ScrollBox(='Window'). 자식은 BindWidget 이름 그대로 여기에 add."""
    root = mk(bp, unreal.CanvasPanel, "RootCanvas"); set_root(bp, root)
    # 1) 전체화면 딤 — 게임 화면을 어둡게 + 패널 밖 클릭 차단(모달)
    dim = mk(bp, unreal.Border, "Dimmer"); add(root, dim)
    full_screen(dim); set_color(dim, COL_DIM)
    # 1.5) 비스듬한 accent 줄무늬(딤 위, 패널 뒤)
    add_diagonals(bp, root)
    # 2) 강조 테두리(accent frame) — 얇은 크림슨 외곽
    frame = mk(bp, unreal.Border, "PanelFrame"); add(root, frame)
    set_slot(frame, 0, 0, w, h, amin=(0.5, 0.5), amax=(0.5, 0.5), align=(0.5, 0.5))
    set_color(frame, COL_FRAME); set_pad(frame, 2, 2, 2, 2)
    # 3) 중앙 불투명 패널(배경) — 프레임 안쪽
    panel = mk(bp, unreal.Border, "Panel"); add(frame, panel)
    set_color(panel, COL_PANEL); set_pad(panel, 24, 16, 24, 18)
    col = mk(bp, unreal.VerticalBox, "PanelCol"); add(panel, col)
    # 3.5) 상단 헤더: 우측 고정 ✕ 닫기(스크롤과 무관하게 항상 보임)
    header = mk(bp, unreal.HorizontalBox, "PanelHeader"); add(col, header)
    sp = mk(bp, unreal.Spacer, "HdrSpacer"); add(header, sp); fill_slot(sp)
    cb = labeled_button(bp, header, "Btn_Close", "✕"); slot_pad(cb, 6, 0, 0, 4)
    # 4) 스크롤 콘텐츠(항목 많아도 안 넘침) — 남은 높이 채움
    box = mk(bp, unreal.ScrollBox, "Window"); add(col, box); fill_slot(box)
    return box

def build_spec(bp, spec):
    """팝업형: 중앙 패널 + 딤 배경 + 스크롤 콘텐츠."""
    box = popup_shell(bp)
    fill_box(bp, box, spec)

def build_battlehud(bp):
    root = mk(bp, unreal.CanvasPanel, "RootCanvas"); set_root(bp, root)
    # ★ 페르소나풍 전투 HUD: 크림슨 프레임 + 골드 강조. C++가 게이지 카드를 List_*에 동적 생성.
    #   화면 중앙/우측(캐릭터·오버숄더)을 비우도록 좌측 구석에 컴팩트하게.
    ACCENT  = C(0.86, 0.18, 0.24, 0.96)   # 크림슨 프레임(페르소나 시그니처)
    PANEL_BG = C(0.05, 0.06, 0.10, 0.80)  # 카드 패널 내부(반투명 다크네이비)

    # 크림슨 외곽 + 다크 내부 프레임 헬퍼. 반환 = 내용 담을 컨테이너(VBox 또는 outer).
    def framed(name, x, y, w, h, amin, amax, align, bg=PANEL_BG, container=True):
        outer = mk(bp, unreal.Border, name + "_F"); add(root, outer)
        set_slot(outer, x, y, w, h, amin=amin, amax=amax, align=align)
        set_color(outer, ACCENT); set_pad(outer, 2, 2, 2, 2)
        inner = mk(bp, unreal.Border, name + "_BG"); add(outer, inner)
        set_color(inner, bg); set_pad(inner, 9, 7, 9, 7)
        if not container:
            return inner
        vb = mk(bp, unreal.VerticalBox, name); add(inner, vb)
        return vb

    # 적 카드 패널(상단 좌측) / 아군 카드 패널(하단 좌측) — 좁게(310)
    framed("List_Enemies", 14, 12, 310, 268, (0.0, 0.0), (0.0, 0.0), (0.0, 0.0))
    framed("List_Party",   14, -98, 310, 290, (0.0, 1.0), (0.0, 1.0), (0.0, 1.0))

    # 턴 순서 바(상단 중앙) — 크림슨 프레임 + 골드 텍스트
    turn_in = framed("TurnFrame", 0, 12, 880, 40, (0.5, 0.0), (0.5, 0.0), (0.5, 0.0),
                     bg=C(0.05, 0.06, 0.10, 0.92), container=False)
    turn = mk(bp, unreal.TextBlock, "Txt_TurnOrder"); add(turn_in, turn)
    set_style(turn, COL_TITLE, 16, JC)

    # 하단 액션바 — 크림슨 상단 라인 + 다크 배경 + 버튼(중앙 하단)
    aacc = mk(bp, unreal.Border, "ActionAccent"); add(root, aacc)
    set_slot(aacc, 0, -12, 1040, 90, amin=(0.5, 1.0), amax=(0.5, 1.0), align=(0.5, 1.0))
    set_color(aacc, ACCENT); set_pad(aacc, 0, 3, 0, 0)   # 위쪽만 크림슨 라인
    abg = mk(bp, unreal.Border, "ActionBarBG"); add(aacc, abg)
    set_color(abg, C(0.05, 0.06, 0.10, 0.94)); set_pad(abg, 12, 9, 12, 9)
    bar = mk(bp, unreal.HorizontalBox, "ActionBar"); add(abg, bar)
    for bn, label in [("Btn_Attack","공격"),("Btn_Skill","스킬"),("Btn_Guard","방어"),
                      ("Btn_Charge","차지"),("Btn_Item","아이템"),("Btn_Escape","도망"),
                      ("Btn_AllOut","총공격"),("Btn_Baton","바톤")]:
        b = labeled_button(bp, bar, bn, label); slot_pad(b, 4, 0, 4, 0)

    # 스킬/아이템 서브패널 — 액션바 위, 제목 달린 크림슨 프레임 패널(기본 숨김).
    #   ※ C++가 Panel_*(여기선 외곽 Border) 가시성을 직접 토글 → 패널 통째로 표시/숨김.
    for panel, bpfx, tpfx, n, title in [
        ("Panel_Skills", "Btn_SkillOpt", "Txt_SkillOpt", 6, "스킬"),
        ("Panel_Items",  "Btn_ItemOpt",  "Txt_ItemOpt",  6, "아이템")]:
        outer = mk(bp, unreal.Border, panel); add(root, outer)   # ← C++ 토글 대상
        set_slot(outer, 0, -106, 600, 360, amin=(0.5, 1.0), amax=(0.5, 1.0), align=(0.5, 1.0))
        set_color(outer, ACCENT); set_pad(outer, 2, 2, 2, 2)
        pin = mk(bp, unreal.Border, panel + "_BG"); add(outer, pin)
        set_color(pin, C(0.05, 0.06, 0.10, 0.97)); set_pad(pin, 12, 8, 12, 8)
        vb = mk(bp, unreal.VerticalBox, panel + "_VB"); add(pin, vb)
        ht = mk(bp, unreal.TextBlock, panel + "_Title"); set_text(ht, title)
        set_style(ht, COL_TITLE, 18, JL); add(vb, ht); slot_pad(ht, 2, 0, 2, 6)
        for i in range(n):
            labeled_button(bp, vb, "%s%d" % (bpfx, i), "", "%s%d" % (tpfx, i))
        collapse(outer)

    # 타겟/아군 선택 패널 — C++가 카드클릭 타겟팅으로 대체해 항상 숨김. 버튼은 바인딩 위해 존재만.
    for panel, bpfx, tpfx, n in [
        ("Panel_Targets", "Btn_TargetOpt", "Txt_TargetOpt", 4),
        ("Panel_Allies",  "Btn_AllyOpt",   "Txt_AllyOpt",   4)]:
        p = mk(bp, unreal.VerticalBox, panel); add(root, p)
        set_slot(p, 0, -106, 540, 310, amin=(0.5, 1.0), amax=(0.5, 1.0), align=(0.5, 1.0))
        collapse(p)
        for i in range(n):
            labeled_button(bp, p, "%s%d" % (bpfx, i), "", "%s%d" % (tpfx, i))

def build_talk(bp):
    # ★ 전투는 페르소나 전투(BattleManager) 하나로 통일 — 레거시 턴제/실시간/스타일 UI 제거.
    #   상태: 일반 대화(Panel_Dialogue) / 전투 가능 NPC 선택(Panel_CombatMode). 둘뿐.
    root = mk(bp, unreal.CanvasPanel, "RootCanvas"); set_root(bp, root)
    tbg = mk(bp, unreal.Border, "TalkBG"); add(root, tbg)
    set_slot(tbg, 0, -16, 1180, 320, amin=(0.5, 1.0), amax=(0.5, 1.0), align=(0.5, 1.0))
    set_color(tbg, COL_PANEL); set_pad(tbg, 26, 18, 26, 18)
    box = mk(bp, unreal.VerticalBox, "TalkBox"); add(tbg, box)

    # 상단 바: 닫기 버튼(항상 표시) — 어느 상태에서든 대화 탈출
    topbar = mk(bp, unreal.HorizontalBox, "TalkTopBar"); add(box, topbar)
    cb = labeled_button(bp, topbar, "Btn_CloseTalk", "✕ 닫기"); slot_pad(cb, 0, 0, 0, 4)

    # 이름 + 대사(항상)
    name = mk(bp, unreal.TextBlock, "CharacterNameText"); set_style(name, COL_TITLE, 22, JL); add(box, name); slot_pad(name, 2, 2, 2, 2)
    line = mk(bp, unreal.TextBlock, "DialogueText");      set_style(line, COL_TEXT, 19, JL); add(box, line); slot_pad(line, 2, 4, 2, 8)

    # 일반 대화 힌트 패널
    pdlg = mk(bp, unreal.VerticalBox, "Panel_Dialogue"); add(box, pdlg)
    hint = mk(bp, unreal.TextBlock, "Txt_Hint"); set_text(hint, "(클릭하여 계속 ▼)"); set_style(hint, COL_TEXT, 14, JR); add(pdlg, hint)

    # 전투 가능 NPC 선택 패널 — 전투(페르소나)/상점. 기본 숨김(ShowPanel이 노출).
    pcm = mk(bp, unreal.HorizontalBox, "Panel_CombatMode"); add(box, pcm); slot_pad(pcm, 0, 6, 0, 0)
    for bn, label in [("Btn_PersonaBattle", "⚔ 전투"), ("Btn_Shop", "상점")]:
        b = labeled_button(bp, pcm, bn, label); slot_pad(b, 4, 0, 4, 0)
    collapse(pcm)

def build_worldhud(bp):
    # 상시 표시 HUD — 우상단 구석. ★게임 클릭을 막지 않게 전체 입력 통과.
    root = mk(bp, unreal.CanvasPanel, "RootCanvas"); set_root(bp, root)
    bg = mk(bp, unreal.Border, "HudBG"); add(root, bg)
    set_slot(bg, -16, 14, 540, 50, amin=(1.0, 0.0), amax=(1.0, 0.0), align=(1.0, 0.0))  # 우상단
    set_color(bg, COL_BAR); set_pad(bg, 14, 6, 14, 6)
    bar = mk(bp, unreal.HorizontalBox, "HudBar"); add(bg, bar)
    for nm in ("Txt_DayTime", "Txt_Gold", "Txt_Weather"):
        t = mk(bp, unreal.TextBlock, nm); set_style(t, COL_TEXT, 16, JC); add(bar, t); slot_pad(t, 8, 0, 8, 0)
    # 상시 HUD는 입력을 통과시켜 이동/상호작용 클릭을 막지 않음(표시 전용).
    try: unreal.UIBuilderLibrary.set_hit_test_invisible(bg, False)
    except Exception: pass

def build_mainmenu(bp):
    # ★ 부팅 시작화면(타이틀) — 게임 켜면 처음 나오는 화면.
    #   배경은 /Game/Title 레벨의 3D 흰 공간 스테이지(히어로 캐릭터+VFX)가 비치도록 "시스루".
    #   → 풀스크린 불투명 배경 제거. 좌측에만 반투명 패널(메뉴 가독성), 우측은 비워 캐릭터가 보이게.
    root = mk(bp, unreal.CanvasPanel, "RootCanvas"); set_root(bp, root)

    # 전체 은은한 비네트(흰 스테이지를 살짝 가라앉혀 UI 대비 확보 — 3D는 그대로 보임)
    vig = mk(bp, unreal.Border, "MenuVignette"); add(root, vig)
    full_screen(vig); set_color(vig, C(0.02, 0.02, 0.05, 0.28))

    # 좌측 메뉴 패널(반투명 다크) — 로고/버튼 가독성. 우측 ~60%는 비워 캐릭터 스테이지 노출.
    bg = mk(bp, unreal.Border, "MenuBG"); add(root, bg)
    set_slot(bg, -40, -60, 820, 1500, amin=(0.0, 0.0), amax=(0.0, 0.0), align=(0.0, 0.0))
    set_color(bg, C(0.02, 0.02, 0.05, 0.60))
    add_diagonals(bp, root)                                     # 페르소나 시그니처 사선 크림슨

    # 타이틀 블록(좌상단) — 큰 로고 + 부제. (Txt_Title/Subtitle = MainMenuWidget NativeTick이 펄스/시머 구동)
    tbox = mk(bp, unreal.VerticalBox, "TitleBox"); add(root, tbox)
    set_slot(tbox, 96, 120, 640, 240, amin=(0.0, 0.0), amax=(0.0, 0.0), align=(0.0, 0.0))
    title = mk(bp, unreal.TextBlock, "Txt_Title"); set_text(title, "REVERIE")
    set_style(title, COL_TITLE, 84, JL); add(tbox, title); slot_pad(title, 0, 0, 0, 2)
    sub = mk(bp, unreal.TextBlock, "Txt_Subtitle"); set_text(sub, "환 영 의   도 시")
    set_style(sub, COL_FRAME, 26, JL); add(tbox, sub); slot_pad(sub, 4, 6, 0, 0)

    # 메뉴 버튼 열(좌측 중앙)
    box = mk(bp, unreal.VerticalBox, "Window"); add(root, box)
    set_slot(box, 96, 40, 560, 400, amin=(0.0, 0.5), amax=(0.0, 0.5), align=(0.0, 0.5))
    for bn, label in [("Btn_NewGame", "새 게임"), ("Btn_Continue", "이어하기"),
                      ("Btn_Settings", "설정"), ("Btn_Quit", "종료")]:
        b = labeled_button(bp, box, bn, label); slot_pad(b, 0, 6, 0, 6)

    # 푸터 힌트(좌하단)
    foot = mk(bp, unreal.TextBlock, "Txt_Foot"); set_text(foot, "ⓒ 2026  ·  마우스로 선택")
    add(root, foot); set_style(foot, COL_TEXT, 14, JL)
    set_slot(foot, 100, -46, 620, 24, amin=(0.0, 1.0), amax=(0.0, 1.0), align=(0.0, 1.0))

def menu_section(bp, box, key, title, items, cols=2):
    """카테고리 섹션: 제목 + N열 그리드(세로박스 안에 가로행, 각 버튼 균등 채움 — 견고)."""
    h = mk(bp, unreal.TextBlock, "Sec_" + key); set_text(h, title)
    set_style(h, COL_TITLE, 20, JL); add(box, h); slot_pad(h, 2, 12, 2, 4)
    vb = mk(bp, unreal.VerticalBox, "Grid_" + key); add(box, vb)
    row = None
    for i, (bn, label) in enumerate(items):
        if i % cols == 0:
            row = mk(bp, unreal.HorizontalBox, "Row_%s_%d" % (key, i // cols)); add(vb, row)
            slot_pad(row, 0, 3, 0, 3)
        b = labeled_button(bp, row, bn, label); fill_slot(b); slot_pad(b, 4, 0, 4, 0)

def build_systemmenu(bp):
    # M 메뉴 허브 — 플레이어 정보 + 카테고리(정보/월드/시스템) 2열 그리드. 우상단 ✕ 닫기.
    box = popup_shell(bp, w=780, h=760)
    info = mk(bp, unreal.TextBlock, "Txt_Info"); set_style(info, COL_TEXT, 18, JC); add(box, info); slot_pad(info, 2, 2, 2, 2)
    status = mk(bp, unreal.TextBlock, "Txt_Status"); set_style(status, COL_TITLE, 16, JC); add(box, status); slot_pad(status, 2, 2, 2, 6)
    menu_section(bp, box, "info", "정보", [
        ("Btn_Inventory", "가방"), ("Btn_Equipment", "장비"), ("Btn_Quests", "퀘스트"),
        ("Btn_Status", "상태"), ("Btn_Social", "사회 스탯"), ("Btn_Bond", "인연"),
        ("Btn_Bestiary", "적 도감"), ("Btn_Discovery", "지역"), ("Btn_Achievements", "도전과제")])
    menu_section(bp, box, "world", "월드", [
        ("Btn_Travel", "빠른 이동"), ("Btn_Bank", "은행"), ("Btn_Craft", "제작"), ("Btn_Help", "도움말")])
    menu_section(bp, box, "sys", "시스템", [
        ("Btn_Save", "저장"), ("Btn_Load", "불러오기"), ("Btn_NewGame", "새 게임"), ("Btn_Resume", "닫기")])

def list_popup(bp, title, container_name, w=720, h=760):
    """제목 + 동적 컨테이너(VerticalBox)만 있는 전용 UI 껍데기. C++가 컨테이너를 카드로 채움."""
    box = popup_shell(bp, w, h)
    t = mk(bp, unreal.TextBlock, "Txt_Title"); set_text(t, title)
    set_style(t, COL_TITLE, 28, JC); add(box, t); slot_pad(t, 0, 2, 0, 4)
    add_divider(bp, box)
    lst = mk(bp, unreal.VerticalBox, container_name); add(box, lst)
    return lst

def list_popup_ex(bp, readouts, container_name, w=720, h=760):
    """제목/요약 텍스트 여러 줄 + 동적 클릭카드 컨테이너. readouts=[(name,text,size),...].
       C++가 컨테이너를 클릭 카드로 채움(상점/장비 등 상호작용 메뉴)."""
    box = popup_shell(bp, w, h)
    for (nm, txt, sz) in readouts:
        t = mk(bp, unreal.TextBlock, nm); set_text(t, txt)
        set_style(t, COL_TITLE if sz >= 24 else COL_TEXT, sz, JC); add(box, t); slot_pad(t, 0, 2, 0, 2)
    add_divider(bp, box)
    lst = mk(bp, unreal.VerticalBox, container_name); add(box, lst)
    return lst

def build_shop(bp):        list_popup_ex(bp, [("Txt_Gold","골드: 0",24),("Txt_Message","",16)], "List_Items")
def build_equipment(bp):   list_popup_ex(bp, [("Txt_Title","장비",28),("Txt_Equipped","",16)], "List_Items")
def build_crafting(bp):    list_popup(bp, "제작", "List_Recipes")
def build_fasttravel(bp):  list_popup(bp, "빠른 이동", "List_Dests")

def build_inventory(bp):   list_popup(bp, "가방", "List_Items")
def build_social(bp):      list_popup(bp, "사회 스탯", "List_Stats")
def build_quests(bp):      list_popup(bp, "퀘스트", "List_Quests")
def build_bonds(bp):       list_popup(bp, "인연", "List_Bonds")
def build_bestiary(bp):    list_popup(bp, "적 도감", "List_Bestiary")
def build_achievements(bp):list_popup(bp, "도전과제", "List_Achievements")
def build_discovery(bp):   list_popup(bp, "발견 지역", "List_Regions")
def build_status(bp):      list_popup(bp, "상태", "List_Status")

def build_menuflow(bp):
    # ★ 풀스크린 M 메뉴 — 루트 캔버스 + 불투명 배경 + 사선 줄무늬만 제공.
    #   허브(좌측 정보+우측 카테고리) / 스테이지(전용 풀스크린 장면) / 사선 와이프는 전부 C++가 런타임 조립.
    root = mk(bp, unreal.CanvasPanel, "RootCanvas"); set_root(bp, root)
    bg = mk(bp, unreal.Border, "BG"); add(root, bg)
    full_screen(bg); set_color(bg, C(0.03, 0.03, 0.06, 1.0))   # 불투명 풀배경(박스 아님)
    add_diagonals(bp, root)                                     # 페르소나 시그니처 사선 줄무늬(배경)

def build_damagenum(bp):
    # 데미지 숫자: 루트 = Txt_Number 하나(위젯이 SetPositionInViewport로 스스로 배치/애니).
    t = mk(bp, unreal.TextBlock, "Txt_Number"); set_root(bp, t)
    set_style(t, COL_BTNTXT, 30, JC)

def build_battleflair(bp):
    # 페르소나 플레어 배너: 화면 상단 중앙에 강조색 바 + 큰 텍스트. C++가 색/텍스트/슬라이드 애니 구동.
    root = mk(bp, unreal.CanvasPanel, "RootCanvas"); set_root(bp, root)
    frame = mk(bp, unreal.Border, "Bg_FlairFrame"); add(root, frame)   # 검은 외곽(대비)
    set_slot(frame, 0, -40, 780, 100, amin=(0.5, 0.34), amax=(0.5, 0.34), align=(0.5, 0.5))
    set_color(frame, C(0.03, 0.03, 0.05, 0.96)); set_pad(frame, 4, 4, 4, 4)
    bg = mk(bp, unreal.Border, "Bg_Flair"); add(frame, bg)             # 강조색 바(C++가 색 지정)
    set_color(bg, C(0.86, 0.18, 0.24, 0.96)); set_pad(bg, 24, 6, 24, 6)
    t = mk(bp, unreal.TextBlock, "Txt_Flair"); add(bg, t); set_text(t, "WEAK!")
    set_style(t, COL_BTNTXT, 46, JC)

BUILDERS = {"BATTLEHUD": build_battlehud, "TALK": build_talk,
            "BATTLEFLAIR": build_battleflair,
            "WORLDHUD": build_worldhud, "MAINMENU": build_mainmenu,
            "DMGNUM": build_damagenum, "SYSMENU": build_systemmenu,
            "INVENTORY": build_inventory, "SOCIAL": build_social,
            "QUESTS": build_quests, "BONDS": build_bonds, "BESTIARY": build_bestiary,
            "ACHIEVEMENTS": build_achievements, "DISCOVERY": build_discovery,
            "STATUS": build_status, "MENUFLOW": build_menuflow,
            "SHOP": build_shop, "EQUIPMENT": build_equipment,
            "CRAFTING": build_crafting, "FASTTRAVEL": build_fasttravel}

# -----------------------------------------------------------------------------
# BP 생성/리페어런트
# -----------------------------------------------------------------------------
def parent_class(cpp_name):
    cls = getattr(unreal, cpp_name, None)
    if cls is None: log("[WARN] unreal.%s 없음 — C++ 빌드 먼저 그린인지 확인. skip." % cpp_name)
    return cls

def ensure_bp(wbp_name, pcls):
    full = "%s/%s" % (UI_PATH, wbp_name)
    if EAL.does_asset_exist(full):
        return unreal.load_asset(full)
    factory = unreal.WidgetBlueprintFactory()
    try: factory.set_editor_property("parent_class", pcls)
    except Exception: pass
    bp = ASSET_TOOLS.create_asset(wbp_name, UI_PATH, None, factory)
    if bp:
        try: BEL.reparent_blueprint(bp, pcls)
        except Exception: pass
        try: EAL.save_loaded_asset(bp)
        except Exception: pass
        log("생성+부모연결: %s" % full)   # ※ pcls.get_name() 5.7 크래시 → 제거
    return bp

def fill_widget(bp, wbp_name, spec):
    if not has_uib():
        manual("%s: UIBuilderLibrary(C++) 없음 — 리빌드(에디터 닫고 풀빌드) 후 재실행 필요. 수동은 기획/03_규약/UI_레이아웃_가이드.md." % wbp_name); return
    if FORCE_REBUILD and wbp_name not in KEEP:
        try: unreal.UIBuilderLibrary.clear_tree(bp)   # 기존 내용 비우고 새 레이아웃으로
        except Exception: pass
    elif unreal.UIBuilderLibrary.has_root(bp):
        log("%s: 이미 내용 있음 — 자동배치 생략(사용자 디자인 보존)." % wbp_name); return
    try:
        if isinstance(spec, str): BUILDERS[spec](bp)
        else: build_spec(bp, spec)
        unreal.UIBuilderLibrary.compile_bp(bp); EAL.save_loaded_asset(bp)
        log("%s: 내부 자동배치 완료." % wbp_name)
    except Exception as e:
        manual("%s: 자동배치 실패(%s) — 기획/03_규약/UI_레이아웃_가이드.md 보고 수동." % (wbp_name, e))

# -----------------------------------------------------------------------------
# 클래스 참조 배선
# -----------------------------------------------------------------------------
def gen_class(wbp_name):
    full = "%s/%s.%s_C" % (UI_PATH, wbp_name, wbp_name)
    c = unreal.load_object(None, full)
    if c is None:
        bp = unreal.load_asset("%s/%s" % (UI_PATH, wbp_name))
        if bp:
            try: c = bp.generated_class()
            except Exception: c = None
    return c

def set_cdo_props(bp_or_class, props, label, overwrite=False):
    """CDO에 클래스/값 프로퍼티 지정 후 저장. props = {name: value}
       overwrite=False(기본): 이미 값이 있는 프로퍼티는 건드리지 않음 → 사용자가 이미 연결해
       작동 중인 배선(DialogueWidgetClass/BattleHUDClass 등)을 덮어써 깨뜨리지 않음. 빈 칸만 채움."""
    bp = bp_or_class
    try:
        gen = bp.generated_class()
    except Exception:
        manual("%s: generated_class 실패." % label); return
    cdo = unreal.get_default_object(gen)
    if cdo is None:
        manual("%s: CDO 없음." % label); return
    done = []; kept = []
    for k, v in props.items():
        if v is None:
            manual("%s.%s: 대상 클래스 없음(해당 WBP 생성 실패?)." % (label, k)); continue
        try:
            cur = cdo.get_editor_property(k)
        except Exception:
            cur = None
        if cur and not overwrite:
            kept.append(k); continue   # 이미 연결돼 있음 → 보존
        try:
            cdo.set_editor_property(k, v); done.append(k)
        except Exception as e:
            manual("%s.%s 설정 실패: %s" % (label, k, e))
    try:
        BEL.compile_blueprint(bp); EAL.save_loaded_asset(bp)
    except Exception: pass
    if done: log("%s 배선(빈칸 채움): %s" % (label, ", ".join(done)))
    if kept: log("%s 기존값 보존(안건드림): %s" % (label, ", ".join(kept)))

def find_bp_of_parent(parent_cpp):
    """부모 C++ 클래스를 상속한 첫 Blueprint 에셋 반환(없으면 None)."""
    pcls = getattr(unreal, parent_cpp, None)
    # 액터 클래스는 A 접두사를 떼고 노출됨(APlayerCharacter → unreal.PlayerCharacter). 폴백.
    if pcls is None and len(parent_cpp) > 1 and parent_cpp[0] == "A":
        pcls = getattr(unreal, parent_cpp[1:], None)
    if pcls is None: return None
    datas = AR.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "Blueprint"), True)
    for d in datas:
        try:
            bp = d.get_asset()
            if not bp: continue
            gen = bp.generated_class()
            if gen and unreal.MathLibrary.class_is_child_of(gen, pcls):
                return bp
        except Exception:
            continue
    return None

def attach_story_director(player_bp):
    """BP_PlayerCharacter SCS에 StoryDirectorComponent 부착 + StoryWidgetClass 지정.
       실패 시 [MANUAL] 안내."""
    sdc = getattr(unreal, "StoryDirectorComponent", None)
    story_cls = gen_class("WBP_Story")
    if sdc is None:
        manual("StoryDirectorComponent 클래스 없음 — C++ 빌드 확인."); return
    try:
        sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
        handles = sds.k2_gather_subobject_data_for_blueprint(player_bp)
        if not handles:
            raise RuntimeError("subobject handles 없음")
        # 이미 StoryDirector가 붙어 있으면 중복 추가 안 함(재실행 안전)
        for h in handles:
            try:
                data = sds.k2_find_subobject_data_from_handle(h)
                ex = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
                if ex and isinstance(ex, unreal.StoryDirectorComponent):
                    try:
                        if story_cls and not ex.get_editor_property("StoryWidgetClass"):
                            ex.set_editor_property("StoryWidgetClass", story_cls)
                            BEL.compile_blueprint(player_bp); EAL.save_loaded_asset(player_bp)
                    except Exception: pass
                    log("StoryDirector 이미 부착됨 — 생략(StoryWidgetClass만 확인).")
                    return
            except Exception:
                continue
        root = handles[0]
        params = unreal.AddNewSubobjectParams(
            parent_handle=root, new_class=sdc, blueprint_context=player_bp)
        new_handle, fail = sds.add_new_subobject(params)
        fail_str = str(fail) if fail else ""
        if fail_str:
            raise RuntimeError(fail_str)
        # 컴포넌트 이름 변경(선택 — 실패해도 동작 무관, 5.7 Text API 차이 대응)
        for nm in (lambda: unreal.Text("StoryDirector"), lambda: "StoryDirector"):
            try: sds.rename_subobject(new_handle, nm()); break
            except Exception: continue
        # 인스턴스 StoryWidgetClass 지정
        try:
            obj = sds.k2_find_subobject_data_from_handle(new_handle)
            comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(obj)
            if comp and story_cls:
                comp.set_editor_property("StoryWidgetClass", story_cls)
        except Exception as e:
            manual("StoryDirector.StoryWidgetClass 인스턴스 지정 실패: %s — BP에서 수동 지정." % e)
        BEL.compile_blueprint(player_bp); EAL.save_loaded_asset(player_bp)
        log("StoryDirector 컴포넌트 부착 완료(+StoryWidgetClass).")
    except Exception as e:
        manual("StoryDirector SCS 자동부착 실패: %s" % e)
        manual("→ 수동: BP_PlayerCharacter 열고 Add Component → StoryDirector → Details의 StoryWidgetClass=WBP_Story.")

# -----------------------------------------------------------------------------
# 실행
# -----------------------------------------------------------------------------
def run():
    if not EAL.does_directory_exist(UI_PATH): EAL.make_directory(UI_PATH)
    ensure_font()   # 노토산스KR 폰트 준비(실패해도 기본폰트로 진행)
    log("==== 1) 위젯 생성/자동배치 ====")
    made = {}
    for wbp_name, cpp, spec in WIDGETS:
        pcls = parent_class(cpp)
        if pcls is None: continue
        bp = ensure_bp(wbp_name, pcls)
        if not bp:
            manual("%s 생성 실패." % wbp_name); continue
        made[wbp_name] = bp
        fill_widget(bp, wbp_name, spec)

    log("==== 2) 클래스 참조 배선 ====")
    # 2-a. SystemMenu 하위 위젯 클래스
    if "WBP_SystemMenu" in made:
        set_cdo_props(made["WBP_SystemMenu"], {
            "InventoryWidgetClass":   gen_class("WBP_Inventory"),
            "QuestLogWidgetClass":    gen_class("WBP_QuestLog"),
            "StatusWidgetClass":      gen_class("WBP_Status"),
            "DiscoveryWidgetClass":   gen_class("WBP_Discovery"),
            "BestiaryWidgetClass":    gen_class("WBP_Bestiary"),
            "BondWidgetClass":        gen_class("WBP_Bond"),
            "FastTravelWidgetClass":  gen_class("WBP_FastTravel"),
            "BankWidgetClass":        gen_class("WBP_Bank"),
            "CraftingWidgetClass":    gen_class("WBP_Crafting"),
            "AchievementWidgetClass": gen_class("WBP_Achievements"),
            "SocialStatsWidgetClass": gen_class("WBP_Social"),
            "EquipmentWidgetClass":   gen_class("WBP_Equipment"),   # B가 Btn_Equipment 추가(06-03)
            "HelpWidgetClass":        gen_class("WBP_Help"),        # B가 Btn_Help 추가(06-03)
        }, "WBP_SystemMenu")
    # 2-b. Talk → Shop
    if "WBP_TalkUserWidget" in made:
        set_cdo_props(made["WBP_TalkUserWidget"], {"ShopWidgetClass": gen_class("WBP_Shop")}, "WBP_TalkUserWidget")

    # 2-b2. StoryJournal → Story(다시보기용)
    if "WBP_StoryJournal" in made:
        set_cdo_props(made["WBP_StoryJournal"], {"StoryWidgetClass": gen_class("WBP_Story")}, "WBP_StoryJournal")

    # 2-b3. MainMenu → Settings
    if "WBP_MainMenu" in made:
        set_cdo_props(made["WBP_MainMenu"], {"SettingsWidgetClass": gen_class("WBP_Settings")}, "WBP_MainMenu")

    # 2-c. BP_PlayerCharacter
    player_bp = find_bp_of_parent("APlayerCharacter")
    if player_bp:
        set_cdo_props(player_bp, {
            "DialogueWidgetClass":   gen_class("WBP_TalkUserWidget"),
            "SystemMenuWidgetClass": gen_class("WBP_SystemMenu"),
            "MenuFlowWidgetClass":   gen_class("WBP_MenuFlow"),   # ★ 풀스크린 M 메뉴 우선
            "WorldHUDClass":         gen_class("WBP_WorldHUD"),
        }, "BP_PlayerCharacter(%s)" % player_bp.get_name())
        attach_story_director(player_bp)
    else:
        manual("APlayerCharacter 상속 BP를 못 찾음 — 플레이어 BP에서 Dialogue/SystemMenu/WorldHUD/StoryDirector 수동 지정.")

    # 2-d. BP_BattleManager
    bm_bp = find_bp_of_parent("BattleManager")
    if bm_bp:
        set_cdo_props(bm_bp, {
            "BattleHUDClass":    gen_class("WBP_BattleHUD"),
            "DamageNumberClass": gen_class("WBP_DamageNumber"),
            "FlairWidgetClass":  gen_class("WBP_BattleFlair"),
        }, "BP_BattleManager(%s)" % bm_bp.get_name())
    else:
        manual("BattleManager 상속 BP를 못 찾음 — BP_BattleManager의 BattleHUDClass=WBP_BattleHUD 수동 지정.")

    log("==== 완료. 위 [MANUAL] 항목만 사람이 처리하면 됨. /Game/UI 확인. ====")

run()
