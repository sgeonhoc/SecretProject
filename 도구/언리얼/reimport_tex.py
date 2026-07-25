# test7 텍스처 아틀라스를 styled PNG로 재임포트(교체). 머티리얼은 같은 에셋을 참조하므로 즉시 반영.
# 원본 .uasset 백업: _tripo/our_chars/test7_tex_backup_uasset/ (롤백용).
import unreal, os
SRC  = "C:/Secret_Project/_tripo/our_chars/test7_tex_styled4"
DEST = "/Game/TestCharacter/test7"
NAMES = ["T__04", "T__10", "T__13", "T__12", "T__02", "T__17"]
log = []
def w(s):
    s = str(s); log.append(s)
    try: unreal.log("REIMP> %s" % s)
    except Exception: pass

at = unreal.AssetToolsHelpers.get_asset_tools()
tasks = []
for n in NAMES:
    f = SRC + "/" + n + ".png"
    if not os.path.exists(f):
        w("MISSING %s" % f); continue
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", f)
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", n)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    tasks.append((n, t))
    w("queued %s <- %s" % (n, f))

at.import_asset_tasks([t for _, t in tasks])
for n, t in tasks:
    imp = list(t.get_editor_property("imported_object_paths") or [])
    w("imported %s -> %s" % (n, imp))
    # 색상 아틀라스이므로 sRGB 보장
    try:
        tex = unreal.load_asset(DEST + "/" + n)
        if isinstance(tex, unreal.Texture2D):
            tex.set_editor_property("srgb", True)
            unreal.EditorAssetLibrary.save_asset(DEST + "/" + n, only_if_is_dirty=False)
    except Exception as e:
        w("srgb/save warn %s: %s" % (n, e))

w("DONE")
try: open("C:/Secret_Project/기획/99_보관/구로그/reimport_tex_log.txt", "w", encoding="utf-8").write("\n".join(log))
except Exception: pass
