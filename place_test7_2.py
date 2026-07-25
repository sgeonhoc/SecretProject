# test7_2.vrm 임포트 -> test7/test7_2 둘 다 CiciToon(MI_Kawa_*) 적용 -> TestBattle에 비교 배치
# 비파괴: 소스 SK 에셋은 안 건드리고, 배치 액터 컴포넌트에 머티리얼 오버라이드로 CiciToon.
# ★ VRM4U 임포트 등 무거운 작업은 슬레이트 메인루프가 돌아야 하므로 post-tick 콜백 + 워밍업으로 실행.
# 실행(PowerShell, -unattended 금지):
#   & "<UE>\UnrealEditor.exe" "<.uproject>" -ExecCmds="py C:/Secret_Project/place_test7_2.py" -nosplash -nopause
import unreal, os

LOG = "C:/Secret_Project/기획/99_보관/구로그/place_test7_2_log.txt"
_log = []
def w(s):
    s = str(s); _log.append(s)
    try: open(LOG, "w", encoding="utf-8").write("\n".join(_log))
    except Exception: pass
    try: unreal.log("T72> %s" % s)
    except Exception: pass

EAL = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
at  = unreal.AssetToolsHelpers.get_asset_tools()
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# 비포커스에서도 슬레이트 틱 유지 (안 하면 헤드리스에서 콜백이 거의 안 돌아 멈춤)
try:
    _ps = unreal.get_default_object(unreal.EditorPerformanceSettings)
    _ps.set_editor_property('bThrottleCPUWhenNotForeground', False)
    try: _ps.set_editor_property('bMonitorEditorPerformance', False)
    except Exception: pass
except Exception: pass

MASTER_DIR = '/Game/CiciToonCharacterShaderPak/Character/Hayakawa/Materials/'
VRM2   = "C:/Secret_Project/Content/TestCharacter/test7_2/test7_2.vrm"
DEST_2 = "/Game/TestCharacter/test7_2"
SK7_PATH = "/Game/TestCharacter/test7/SK_test7"

def parent_for(slot):
    s = slot.lower()
    if 'body' in s and 'skin' in s: return 'MI_Kawa_Body'
    if 'face' in s and 'skin' in s: return 'MI_Kawa_Face'
    if 'hair' in s: return 'MI_Kawa_Hair'
    if 'cloth' in s: return 'MI_Kawa_Cloth'
    return None

def find_sk_in(path):
    try: assets = EAL.list_assets(path, True, False)
    except Exception: return []
    cands = []
    for a in assets:
        ap = a.split('.')[0]; nm = ap.split('/')[-1]
        if nm.startswith('SK_'):
            obj = unreal.load_asset(ap)
            if isinstance(obj, unreal.SkeletalMesh):
                cands.append((nm, obj, ap))
    return cands

def do_import():
    existing = find_sk_in(DEST_2)
    if existing:
        w("test7_2 SK 이미 존재 -> 재임포트 생략: %s" % [c[0] for c in existing])
        return existing[0][1], existing[0][2]
    w("test7_2.vrm 임포트 시작 (확장자 자동탐지, 메인루프 가동중)...")
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", VRM2)
    task.set_editor_property("destination_path", DEST_2)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    # 팩토리/옵션 지정 안 함 — .vrm 확장자에 등록된 VRM4U 팩토리가 자동 선택됨(기본 옵션).
    try: at.import_asset_tasks([task])
    except Exception as e: w("  import_asset_tasks 예외: %s" % e)
    try: w("  imported_object_paths: %s" % list(task.get_editor_property("imported_object_paths")))
    except Exception as e: w("  paths read 실패: %s" % e)
    return None, None  # scan 단계에서 확인

def build_cici_overrides(sk, cici_dir, tag):
    if not EAL.does_directory_exist(cici_dir):
        EAL.make_directory(cici_dir)
    mats = list(sk.get_editor_property('materials'))
    w("[%s] slot count: %d" % (tag, len(mats)))
    overrides = []
    for i, sm in enumerate(mats):
        slot = str(sm.get_editor_property('material_slot_name'))
        pname = parent_for(slot)
        if not pname:
            w("  [%d] %-42s skip(원본유지)" % (i, slot)); continue
        parent_path = MASTER_DIR + pname
        if not EAL.does_asset_exist(parent_path):
            w("  [%d] parent 없음: %s" % (i, parent_path)); continue
        src_mi = sm.get_editor_property('material_interface')
        diffuse = None
        try: diffuse = mel.get_material_instance_texture_parameter_value(src_mi, 'gltf_tex_diffuse')
        except Exception as e: w("    [%d] diffuse 읽기 실패: %s" % (i, e))
        safe = ''.join(c for c in slot if c.isalnum())[:24]
        new_path = "%s/MI_Cici_%02d_%s" % (cici_dir, i, safe)
        try:
            if EAL.does_asset_exist(new_path): EAL.delete_asset(new_path)
            new_mi = EAL.duplicate_asset(parent_path, new_path)
            if not new_mi:
                w("  [%d] duplicate 실패" % i); continue
            if diffuse:
                mel.set_material_instance_texture_parameter_value(new_mi, 'BaseTex', diffuse)
                mel.set_material_instance_texture_parameter_value(new_mi, 'SSSTex', diffuse)
            EAL.save_asset(new_path)
            overrides.append((i, new_mi))
            w("  [%d] %-42s -> %s (%s, diff=%s)" % (i, slot, new_path.split('/')[-1], pname, diffuse.get_name() if diffuse else "None"))
        except Exception as e:
            w("  [%d] ERROR: %s" % (i, e))
    return overrides

def place_all(sk7, ov7, sk2, ov72):
    les.load_level('/Game/TestBattle')
    world = ues.get_editor_world()
    import math
    base = unreal.Vector(0, 0, 0); yaw = 0.0; ps = None
    for a in eas.get_all_level_actors():
        if isinstance(a, unreal.PlayerStart): ps = a; break
    if ps:
        base = ps.get_actor_location(); yaw = ps.get_actor_rotation().yaw
        w("PlayerStart loc=%s yaw=%.1f" % (base, yaw))
    else:
        w("PlayerStart 없음 — 원점 기준")
    rad = math.radians(yaw)
    fwd   = unreal.Vector(math.cos(rad), math.sin(rad), 0.0)
    right = unreal.Vector(-math.sin(rad), math.cos(rad), 0.0)
    dist = 450.0; spread = 120.0; gz = base.z; face_yaw = yaw + 180.0
    for a in list(eas.get_all_level_actors()):
        try:
            if a.get_actor_label() in ("CICI_test7", "CICI_test7_2"):
                eas.destroy_actor(a); w("기존 %s 제거" % a.get_actor_label())
        except Exception: pass
    def place(sk, overrides, sign, label):
        if not sk: w("!! %s: SK 없음, 스킵" % label); return
        loc = unreal.Vector(base.x + fwd.x*dist + right.x*spread*sign,
                            base.y + fwd.y*dist + right.y*spread*sign, gz)
        actor = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, loc, unreal.Rotator(0, 0, face_yaw))
        comps = actor.get_components_by_class(unreal.SkeletalMeshComponent)
        comp = comps[0] if comps else None
        if not comp: w("!! %s: 컴포넌트 없음" % label); return
        try: comp.set_skeletal_mesh_asset(sk)
        except Exception:
            try: comp.set_editor_property('skeletal_mesh_asset', sk)
            except Exception as e: w("  %s mesh set 실패: %s" % (label, e))
        for (idx, mi) in overrides:
            try: comp.set_material(idx, mi)
            except Exception as e: w("  %s set_material[%d] 실패: %s" % (label, idx, e))
        try:
            comp.set_editor_property('render_custom_depth', True)
            comp.set_editor_property('custom_depth_stencil_value', 1)
        except Exception: pass
        try: actor.set_actor_label(label)
        except Exception: pass
        w("배치 %s @ %s (overrides=%d)" % (label, loc, len(overrides)))
    place(sk7, ov7, -1.0, "CICI_test7")
    place(sk2, ov72, +1.0, "CICI_test7_2")
    try: les.save_current_level(); w("TestBattle 저장됨")
    except Exception as e: w("레벨 저장 실패: %s" % e)

# ---------- post-tick 상태머신 ----------
ST = {"phase": "warmup", "warm": 0, "wait": 0, "scan": 0, "h": None,
      "sk2": None, "sk2p": None, "wd": 0, "lock": False}
def fin():
    try:
        if ST["h"]: unreal.unregister_slate_post_tick_callback(ST["h"])
    except Exception: pass
    w("=== DONE ===")
    try: unreal.SystemLibrary.execute_console_command(ues.get_editor_world(), "QUIT_EDITOR")
    except Exception: pass

def tick(dt):
    if ST["lock"]: return   # 재진입 가드: 무거운 작업이 슬레이트를 펌프해 콜백을 중첩호출하는 것 차단
    ST["wd"] += 1
    if ST["wd"] > 60000: w("WATCHDOG"); fin(); return
    p = ST["phase"]
    if p == "warmup":
        ST["warm"] += 1
        if ST["warm"] >= 60: w("warmup done"); ST["phase"] = "import"
        return
    if p == "import":
        ST["lock"] = True
        try:
            sk2, sk2p = do_import()
            ST["sk2"] = sk2; ST["sk2p"] = sk2p
        except Exception as e:
            w("import 예외: %s" % e)
        finally:
            ST["lock"] = False
        ST["phase"] = "scan"; ST["scan"] = 0; ST["wait"] = 0
        return
    if p == "scan":
        if ST["sk2"]:
            ST["phase"] = "apply"; return
        ST["wait"] += 1
        if ST["wait"] % 15 == 0:
            cands = find_sk_in(DEST_2)
            ST["scan"] += 1
            w("  scan #%d SK 후보: %s" % (ST["scan"], [c[0] for c in cands]))
            if cands:
                ST["sk2"] = cands[0][1]; ST["sk2p"] = cands[0][2]
                w("test7_2 SK = %s" % ST["sk2p"]); ST["phase"] = "apply"; return
            if ST["scan"] >= 20:
                w("!! test7_2 SK 못 찾음 — 임포트 실패. test7만 진행."); ST["phase"] = "apply"
        return
    if p == "apply":
        ST["lock"] = True
        try:
            sk7 = unreal.load_asset(SK7_PATH)
            if not sk7: w("!! SK_test7 로드 실패")
            ST["ov7"]  = build_cici_overrides(sk7, "/Game/TestCharacter/test7/Cici", "test7") if sk7 else []
            ST["sk7"]  = sk7
            ST["ov72"] = build_cici_overrides(ST["sk2"], DEST_2 + "/Cici", "test7_2") if ST["sk2"] else []
        except Exception as e:
            w("apply 예외: %s" % e); ST["sk7"] = None; ST["ov7"] = []; ST["ov72"] = []
        finally:
            ST["lock"] = False
        ST["phase"] = "place"
        return
    if p == "place":
        ST["lock"] = True
        try: place_all(ST.get("sk7"), ST.get("ov7", []), ST["sk2"], ST.get("ov72", []))
        except Exception as e: w("place 예외: %s" % e)
        finally: ST["lock"] = False
        ST["phase"] = "quit"; ST["wait"] = 0
        return
    if p == "quit":
        ST["wait"] += 1
        if ST["wait"] >= 20: fin()
        return

ST["h"] = unreal.register_slate_post_tick_callback(tick)
w("harness 시작 (warmup -> import -> apply -> place -> quit)")
