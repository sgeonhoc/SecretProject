# 플레이어 SK_VRM을 VRM4U MToon 원본(/test2/)으로 복원 + 셀 강화(ShadeToony 크랭크) + MToon 파라미터 덤프.
#   머티리얼은 SK_VRM 에셋 레벨 → 배치/스폰 인스턴스 전부에 동시 적용.
# 실행:  py "C:/Secret_Project/citoon_mtoon_restore.py"   (PIE 정지 상태)
import unreal
mel = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
SK = '/Game/TestCharacter/MAXIMO/mixamo/SK_VRM'
ORIG = '/Game/TestCharacter/test2/'
DST = '/Game/TestCharacter/CiciVRoid'

log = []
def w(s):
    s = str(s); log.append(s)
def flush():
    try: open("C:/Secret_Project/기획/99_보관/구로그/citoon_mtoon_log.txt", "w", encoding="utf-8").write("\n".join(log))
    except Exception: pass

if not EAL.does_asset_exist(SK):
    w("!! SK_VRM 못 읽음 — 플레이모드? 정지 후 다시.")
    flush()
else:
    sk = unreal.load_asset(SK)
    mats = list(sk.get_editor_property('materials'))
    restored = 0; restored_mis = []
    for i, sm in enumerate(mats):
        slot = str(sm.get_editor_property('material_slot_name'))
        cur = sm.get_editor_property('material_interface')
        curp = cur.get_path_name() if cur else ''
        if (DST in curp) or ('OUTLINE/M_Outline' in curp):
            clean = slot.replace('(Instance)', '').strip()
            origp = ORIG + 'MI_' + clean + '__Instance_'
            if EAL.does_asset_exist(origp):
                om = unreal.load_asset(origp)
                ns = unreal.SkeletalMaterial()
                ns.set_editor_property('material_interface', om)
                ns.set_editor_property('material_slot_name', sm.get_editor_property('material_slot_name'))
                mats[i] = ns; restored += 1
                if origp not in restored_mis: restored_mis.append(origp)
                w("[%d] %-38s -> 복원 %s" % (i, slot, origp.split('/')[-1]))
            else:
                w("[%d] %-38s -> 원본없음 유지 (%s)" % (i, slot, curp.split('/')[-1] if curp else 'none'))
    sk.set_editor_property('materials', mats)
    EAL.save_asset(SK)
    w("==== 복원 슬롯 %d개" % restored)

    # MToon 파라미터 덤프 + 셀(ShadeToony) 크랭크
    w("---- MToon 파라미터(베이스에서 전체 이름) + 셀 튜닝 ----")
    seen_base = set()
    for p in restored_mis:
        mi = unreal.load_asset(p)
        try: base = mi.get_base_material()
        except Exception: base = None
        bname = base.get_name() if base else '?'
        try: scl = [str(n) for n in unreal.MaterialEditingLibrary.get_scalar_parameter_names(base)] if base else []
        except Exception as e: scl = ["ERR(%s)" % e]
        try: vec = [str(n) for n in unreal.MaterialEditingLibrary.get_vector_parameter_names(base)] if base else []
        except Exception: vec = []
        if bname not in seen_base:
            seen_base.add(bname)
            w("  BASE %s" % bname)
            w("    SCALARS: %s" % scl)
            w("    VECTORS: %s" % vec)
        # 셀 강화: toony/shading 관련 스칼라 크랭크
        changed = []
        for n in scl:
            ln = n.lower()
            try:
                if 'toony' in ln:
                    mel.set_material_instance_scalar_parameter_value(mi, n, 0.95); changed.append("%s=0.95" % n)
                elif 'shadingshift' in ln or 'shadeshift' in ln or ('shade' in ln and 'shift' in ln):
                    mel.set_material_instance_scalar_parameter_value(mi, n, 0.0); changed.append("%s=0.0" % n)
            except Exception as e:
                changed.append("%s 실패(%s)" % (n, e))
        try: mel.update_material_instance(mi)
        except Exception: pass
        EAL.save_asset(p)
        w("  %s 셀튜닝: %s" % (p.split('/')[-1], changed if changed else '(toony 파라미터 못 찾음=복원만)'))

    flush()

try:
    world = None
    try: world = unreal.EditorLevelLibrary.get_editor_world()
    except Exception: pass
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
except Exception: pass
