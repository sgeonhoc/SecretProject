# CiciToon ↔ VRoid 머티리얼 구조 덤프 스크립트
# 실행: 에디터 하단 Cmd 콘솔 드롭다운을 "Python"으로 바꾸고  exec(open(r"C:/Secret_Project/citoon_inspect.py").read())
#       또는 Cmd(기본)에서  py "C:/Secret_Project/citoon_inspect.py"
# 결과: C:/Secret_Project/기획/99_보관/구로그/citoon_dump.txt 에 기록 + Output Log 출력

import unreal

out = []
def w(s=""):
    out.append(str(s))
    unreal.log(str(s))

mel = unreal.MaterialEditingLibrary

def dump_master(path):
    w("=" * 64)
    w("MASTER MATERIAL: " + path)
    m = unreal.load_asset(path)
    if not m:
        w("  [NOT FOUND]")
        return
    try:
        names = mel.get_texture_parameter_names(m)
        w("  -- Texture params (%d) --" % len(names))
        for n in names: w("    TEX  " + str(n))
        names = mel.get_scalar_parameter_names(m)
        w("  -- Scalar params (%d) --" % len(names))
        for n in names: w("    SCL  " + str(n))
        names = mel.get_vector_parameter_names(m)
        w("  -- Vector params (%d) --" % len(names))
        for n in names: w("    VEC  " + str(n))
        names = mel.get_static_switch_parameter_names(m)
        w("  -- Static switch params (%d) --" % len(names))
        for n in names: w("    SW   " + str(n))
    except Exception as e:
        w("  [ERR params] " + str(e))

def dump_instance(path):
    w("=" * 64)
    w("INSTANCE: " + path)
    mi = unreal.load_asset(path)
    if not mi:
        w("  [NOT FOUND]")
        return
    try:
        parent = mi.get_editor_property('parent')
        w("  parent: " + (parent.get_path_name() if parent else "None"))
    except Exception as e:
        w("  [ERR parent] " + str(e))
    try:
        for n in mel.get_texture_parameter_names(mi):
            v = mel.get_material_instance_texture_parameter_value(mi, n)
            w("    TEX  %s = %s" % (n, v.get_path_name() if v else "None"))
    except Exception as e:
        w("  [ERR tex] " + str(e))

def dump_skel_slots(path):
    w("=" * 64)
    w("SKELETAL MESH SLOTS: " + path)
    sk = unreal.load_asset(path)
    if not sk:
        w("  [NOT FOUND]")
        return
    try:
        mats = sk.get_editor_property('materials')
        w("  slot count: %d" % len(mats))
        for i, sm in enumerate(mats):
            mi = sm.get_editor_property('material_interface')
            slot = sm.get_editor_property('material_slot_name')
            w("  [%d] slot=%s  mat=%s" % (i, slot, mi.get_path_name() if mi else "None"))
    except Exception as e:
        w("  [ERR slots] " + str(e))

# --- CiciToon 마스터 ---
dump_master('/Game/CiciToonCharacterShaderPak/Stage/Materials/M_CiToon_Default')
dump_master('/Game/CiciToonCharacterShaderPak/Stage/Materials/M_Outline')

# --- CiciToon 데모 캐릭터 인스턴스 (어떤 텍스처를 어느 칸에 넣었나 = 참고용 정답지) ---
dump_instance('/Game/CiciToonCharacterShaderPak/Character/Hayakawa/Materials/MI_Kawa_Body')
dump_instance('/Game/CiciToonCharacterShaderPak/Character/Hayakawa/Materials/MI_Kawa_Face')
dump_instance('/Game/CiciToonCharacterShaderPak/Character/Hayakawa/Materials/MI_Kawa_Hair')

# --- VRoid 메시 슬롯 + VRoid 머티리얼 인스턴스(현재 MToon) ---
dump_skel_slots('/Game/TestCharacter/MAXIMO/mixamo/SK_VRM')
dump_instance('/Game/TestCharacter/test1/MI_N00_000_00_Body_00_SKIN__Instance_')

# --- 결과 파일 기록 ---
try:
    p = "C:/Secret_Project/기획/99_보관/구로그/citoon_dump.txt"
    with open(p, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    unreal.log("==== WROTE: " + p)
except Exception as e:
    unreal.log_error("write failed: " + str(e))
