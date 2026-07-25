# -*- coding: utf-8 -*-
# 실행: 콘솔에  py "C:/Secret_Project/make_ui_widgets.py"
# 위젯 BP 생성(+부모연결) 후 MainMenu/BattleHUD 내부를 버튼/텍스트/패널까지 자동 구성.
# UE5.7 대응: 위젯트리 접근을 여러 방법으로 시도([tree] 로그로 어느 게 먹히는지 출력).

import unreal

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
BEL = unreal.BlueprintEditorLibrary
UI_PATH = "/Game/UI"

WIDGETS = [
    ("WBP_MainMenu",   "MainMenuWidget",  "mainmenu"),
    ("WBP_BattleHUD",  "BattleHUDWidget", "battlehud"),
    ("WBP_Shop",        "ShopWidget",        None),
    ("WBP_Inventory",   "InventoryWidget",   None),
    ("WBP_QuestLog",    "QuestLogWidget",    None),
    ("WBP_SystemMenu",  "SystemMenuWidget",  None),
    ("WBP_Status",      "StatusWidget",      None),
    ("WBP_Discovery",   "DiscoveryWidget",   None),
    ("WBP_Help",        "HelpWidget",        None),
]

def tree_of(bp):
    tries = [
        ("get_editor_property('WidgetTree')", lambda: bp.get_editor_property("WidgetTree")),
        ("attr .widget_tree", lambda: getattr(bp, "widget_tree")),
        ("cast.WidgetTree", lambda: unreal.WidgetBlueprint.cast(bp).get_editor_property("WidgetTree")),
    ]
    for label, fn in tries:
        try:
            wt = fn()
            if wt:
                print("[tree] OK via %s" % label); return wt
        except Exception as e:
            print("[tree] X %s : %s" % (label, e))
    print("[tree] !! 위젯트리 접근 실패 — 디자이너 수동 필요.")
    return None

def root_of(tree):
    try: return tree.get_editor_property("RootWidget")
    except Exception: return None

def mk(tree, cls, name):
    try: return tree.construct_widget(cls, name)
    except Exception as e:
        print("[warn] construct(%s) 실패: %s" % (name, e)); return None

def add(parent, child):
    if parent and child:
        try: parent.add_child(child)
        except Exception as e: print("[warn] add_child 실패: %s" % e)

def labeled_button(tree, parent, btn_name, text):
    b = mk(tree, unreal.Button, btn_name); add(parent, b)
    t = mk(tree, unreal.TextBlock, "Lbl_%s" % btn_name)
    if t:
        try: t.set_text(text)
        except Exception: pass
        add(b, t)
    return b

def opts(tree, panel, bpfx, tpfx, n):
    for i in range(n):
        add(panel, mk(tree, unreal.Button, "%s%d" % (bpfx, i)))
        add(mk(tree, unreal.Button, "%s%d" % (bpfx, i)) or panel, None)  # no-op guard
    # 위 줄은 호환용. 실제 버튼+텍스트는 아래로 다시:
def opts2(tree, panel, bpfx, tpfx, n):
    for i in range(n):
        b = mk(tree, unreal.Button, "%s%d" % (bpfx, i)); add(panel, b)
        t = mk(tree, unreal.TextBlock, "%s%d" % (tpfx, i)); add(b, t)

def cslot(slot, x, y, w, h):
    try: slot.set_position(unreal.Vector2D(x, y)); slot.set_size(unreal.Vector2D(w, h))
    except Exception: pass

def build_mainmenu(tree):
    root = mk(tree, unreal.CanvasPanel, "RootCanvas")
    tree.set_editor_property("RootWidget", root)
    box = mk(tree, unreal.VerticalBox, "MenuBox")
    cslot(root.add_child(box), 80, 200, 400, 400)
    for bn, label in [("Btn_NewGame","새 게임"),("Btn_Continue","이어하기"),
                      ("Btn_Settings","설정"),("Btn_Quit","종료")]:
        labeled_button(tree, box, bn, label)

def build_battlehud(tree):
    root = mk(tree, unreal.CanvasPanel, "RootCanvas")
    tree.set_editor_property("RootWidget", root)
    main = mk(tree, unreal.VerticalBox, "MainBox")
    cslot(root.add_child(main), 40, 40, 760, 900)
    add(main, mk(tree, unreal.TextBlock, "Txt_Status"))
    add(main, mk(tree, unreal.TextBlock, "Txt_TurnOrder"))
    bar = mk(tree, unreal.HorizontalBox, "ActionBar"); add(main, bar)
    for bn, label in [("Btn_Attack","공격"),("Btn_Skill","스킬"),("Btn_Guard","방어"),
                      ("Btn_Charge","차지"),("Btn_Item","아이템"),("Btn_Escape","도망"),
                      ("Btn_AllOut","총공격"),("Btn_Baton","바톤")]:
        labeled_button(tree, bar, bn, label)
    psk = mk(tree, unreal.VerticalBox, "Panel_Skills");  add(main, psk); opts2(tree, psk, "Btn_SkillOpt", "Txt_SkillOpt", 6)
    pit = mk(tree, unreal.VerticalBox, "Panel_Items");   add(main, pit); opts2(tree, pit, "Btn_ItemOpt", "Txt_ItemOpt", 6)
    ptg = mk(tree, unreal.VerticalBox, "Panel_Targets"); add(main, ptg); opts2(tree, ptg, "Btn_TargetOpt", "Txt_TargetOpt", 4)
    pal = mk(tree, unreal.VerticalBox, "Panel_Allies");  add(main, pal); opts2(tree, pal, "Btn_AllyOpt", "Txt_AllyOpt", 4)

BUILDERS = {"mainmenu": build_mainmenu, "battlehud": build_battlehud}

def get_parent_class(name):
    cls = getattr(unreal, name, None)
    if cls is None: print("[WARN] unreal.%s 없음. skip." % name)
    return cls

def ensure_bp(wbp_name, parent_cls, parent_name):
    full = "%s/%s" % (UI_PATH, wbp_name)
    if EAL.does_asset_exist(full):
        return unreal.load_asset(full)
    factory = unreal.WidgetBlueprintFactory()
    try: factory.set_editor_property("parent_class", parent_cls)
    except Exception: pass
    bp = ASSET_TOOLS.create_asset(wbp_name, UI_PATH, None, factory)
    if bp:
        try: BEL.reparent_blueprint(bp, parent_cls)
        except Exception: pass
        EAL.save_loaded_asset(bp)
        print("[ok] 생성: %s (parent=%s)" % (full, parent_name))
    return bp

def run():
    if not EAL.does_directory_exist(UI_PATH): EAL.make_directory(UI_PATH)
    print("==== UI 위젯 생성/구성 시작 ====")
    for wbp_name, parent_name, key in WIDGETS:
        pc = get_parent_class(parent_name)
        if pc is None: continue
        bp = ensure_bp(wbp_name, pc, parent_name)
        if not bp: continue
        if key in BUILDERS:
            tree = tree_of(bp)
            if tree is None: continue
            if root_of(tree) is not None:
                print("[skip-fill] %s 이미 내용 있음" % wbp_name); continue
            try:
                BUILDERS[key](tree)
                BEL.compile_blueprint(bp); EAL.save_loaded_asset(bp)
                print("[ok] %s 내부 구성 완료" % wbp_name)
            except Exception as e:
                print("[warn] %s 구성 실패: %s" % (wbp_name, e))
    print("==== 완료. /Game/UI 확인 ====")

run()
