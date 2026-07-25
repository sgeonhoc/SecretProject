# test7 등 VRoid 캐릭터의 텍스처 아틀라스를 PNG로 추출 (img2img 스타일변환 입력용).
# 폴더 내 모든 Texture2D를 PNG로 내보내고, 크기/이름 로그를 남김 → 색상 아틀라스 식별.
import unreal, os
CHAR = "test7"
SRC  = "/Game/TestCharacter/test7"
OUT  = "C:/Secret_Project/_tripo/our_chars/%s_tex" % CHAR
os.makedirs(OUT, exist_ok=True)
log = []
def w(s):
    s = str(s); log.append(s)
    try: unreal.log("TEX> %s" % s)
    except Exception: pass

paths = unreal.EditorAssetLibrary.list_assets(SRC, recursive=False, include_folder=False)
w("scanning %d assets in %s" % (len(paths), SRC))
n = 0
for p in paths:
    try:
        a = unreal.load_asset(p)
    except Exception:
        a = None
    if not isinstance(a, unreal.Texture2D):
        continue
    name = a.get_name()
    try:
        sx = a.blueprint_get_size_x(); sy = a.blueprint_get_size_y()
    except Exception:
        sx = sy = -1
    t = unreal.AssetExportTask()
    t.set_editor_property('object', a)
    t.set_editor_property('filename', OUT + "/" + name + ".png")
    t.set_editor_property('automated', True)
    t.set_editor_property('prompt', False)
    t.set_editor_property('replace_identical', True)
    ok = False
    try:
        ok = unreal.Exporter.run_asset_export_task(t)
    except Exception as e:
        w("  EXPORT ERR %s: %s" % (name, e))
    if ok:
        n += 1
    w("  [%s] %dx%d exported=%s" % (name, sx, sy, ok))
w("DONE exported=%d -> %s" % (n, OUT))
try: open(OUT + "/_export_log.txt", "w", encoding="utf-8").write("\n".join(log))
except Exception: pass
try:
    world = unreal.EditorLevelLibrary.get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
except Exception: pass
