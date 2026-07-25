# 스폰되는 플레이어 BP의 메시 cast_shadow 끄기 = 머리카락이 몸에 드리우는 자기그림자 제거.
#   ⚠ 끄면 바닥 그림자도 사라짐(진단/임시). 이게 범인 맞으면 '바닥그림자만 따로 살리기'는 다음 단계.
# 실행:  py "C:/Secret_Project/citoon_fix_playershadow.py"   (★PIE 정지 상태!)
import unreal

EAL = unreal.EditorAssetLibrary
log = []
def w(s):
    s = str(s); log.append(s); unreal.log(s)

bps = [
    '/Game/BP_Characters/NewFolder/BP_PlayerCharacter',
    '/Game/BP_Characters/BP_PlayerCharacter',
    '/Game/BP_PlayerCharacter',
]

def mesh_name(c):
    try:
        sk = c.get_skeletal_mesh_asset()
        return sk.get_name() if sk else '(none)'
    except Exception:
        return '?'

for p in bps:
    if not EAL.does_asset_exist(p):
        w("[BP] 없음(또는 플레이모드): %s" % p); continue
    try:
        bp = unreal.load_asset(p)
        cdo = unreal.get_default_object(bp.generated_class())
    except Exception as e:
        w("[BP] %s 로드/CDO 실패: %s" % (p, e)); continue

    comps = []
    try: comps = list(cdo.get_components_by_class(unreal.SkeletalMeshComponent))
    except Exception as e: w("  get_components_by_class 예외: %s" % e)
    try:
        m = cdo.get_editor_property('mesh')   # ACharacter.Mesh (상속 컴포넌트)
        if m and m not in comps: comps.append(m)
    except Exception as e:
        w("  get_editor_property('mesh') 예외: %s" % e)

    w("[BP] %-22s :: SkelMeshComp %d개" % (p.split('/')[-1], len(comps)))
    for c in comps:
        try:
            before = c.get_editor_property('cast_shadow')
            c.set_editor_property('cast_shadow', False)
            after = c.get_editor_property('cast_shadow')
            w("   - %-16s mesh=%-20s cast_shadow %s -> %s" % (c.get_name(), mesh_name(c), before, after))
        except Exception as e:
            w("   - set 실패: %s" % e)
    try:
        EAL.save_asset(p); w("   saved.")
    except Exception as e:
        w("   save 실패: %s" % e)

try:
    with open("C:/Secret_Project/기획/99_보관/구로그/citoon_playershadow_log.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(log))
except Exception as e:
    unreal.log_error("로그 실패: " + str(e))
w("==== 완료. PIE 다시 Play해서 머리/등의 검은 그림자 보세요.")
