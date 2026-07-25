# test7 텍스처를 원본 <-> 스타일(v4) 토글. 에디터 콘솔에서:  py "C:/Secret_Project/toggle_test7.py"
# 머티리얼이 같은 T__* 에셋 참조 → 재임포트로 텍스처만 갈아끼우면 뷰포트 즉시 반영. (리그 0변경)
import unreal, os
STATE = "C:/Secret_Project/_tripo/_toggle_state.txt"
ORIG  = "C:/Secret_Project/_tripo/our_chars/test7_tex"           # 원본 아틀라스 PNG
STYL  = "C:/Secret_Project/_tripo/our_chars/test7_tex_styled4"   # v4 스타일 PNG
DEST  = "/Game/TestCharacter/test7"
NAMES = ["T__04", "T__10", "T__13", "T__12", "T__02", "T__17"]

cur = "styled"
try:
    if os.path.exists(STATE): cur = open(STATE).read().strip() or "styled"
except Exception: pass
nxt = "orig" if cur == "styled" else "styled"
SRC = ORIG if nxt == "orig" else STYL

at = unreal.AssetToolsHelpers.get_asset_tools()
tasks = []
for n in NAMES:
    f = SRC + "/" + n + ".png"
    if not os.path.exists(f):
        unreal.log_warning("toggle: missing %s" % f); continue
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", f)
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", n)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    tasks.append(t)
at.import_asset_tasks(tasks)
try: open(STATE, "w").write(nxt)
except Exception: pass
unreal.log("=== TEST7 TEXTURE TOGGLED -> %s ===" % nxt.upper())
