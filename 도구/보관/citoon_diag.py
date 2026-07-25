# 현황 리포트(읽기 전용). 아무것도 안 바꿈. 결과를 C:/Secret_Project/기획/99_보관/구로그/citoon_diag.txt 에 씀.
#   ★반드시 PIE 정지(Esc/Stop) 상태에서 실행할 것. (플레이 중이면 에셋 못 읽음)
# 실행:  py "C:/Secret_Project/citoon_diag.py"
import unreal

out = []
def w(s):
    s = str(s); out.append(s); unreal.log(s)

EAL = unreal.EditorAssetLibrary

# --- 플레이모드 감지 ---
SK = '/Game/TestCharacter/MAXIMO/mixamo/SK_VRM'
sk_ok = False
try: sk_ok = EAL.does_asset_exist(SK)
except Exception as e: w("does_asset_exist 예외: %s" % e)
w("=== PLAY MODE CHECK ===")
w("SK_VRM exists? %s   (False면 = 플레이모드/에디터제한. 정지 후 다시 실행할 것)" % sk_ok)
w("")

def mesh_name(c):
    for getter in ('get_skeletal_mesh_asset',):
        try:
            sk = getattr(c, getter)()
            if sk: return sk.get_name()
        except Exception: pass
    for prop in ('skeletal_mesh_asset', 'skeletal_mesh', 'skinned_asset'):
        try:
            sk = c.get_editor_property(prop)
            if sk: return sk.get_name()
        except Exception: pass
    return '(none)'

# --- 플레이어 BP 후보 ---
w("=== PLAYER BP 후보 ===")
bp_paths = [
    '/Game/BP_Characters/NewFolder/BP_PlayerCharacter',
    '/Game/BP_Characters/BP_PlayerCharacter',
    '/Game/BP_PlayerCharacter',
]
for p in bp_paths:
    try:
        ex = EAL.does_asset_exist(p)
    except Exception as e:
        w("[BP] %s does_asset_exist 예외: %s" % (p, e)); continue
    w("[BP] %s  exists=%s" % (p, ex))
    if not ex: continue
    try:
        bp = unreal.load_asset(p)
        w("     parent_class = %s" % bp.get_editor_property('parent_class').get_name())
        cdo = unreal.get_default_object(bp.generated_class())
        comps = list(cdo.get_components_by_class(unreal.SkeletalMeshComponent))
        # ACharacter.Mesh 폴백
        try:
            m = cdo.get_editor_property('mesh')
            if m and m not in comps: comps.append(m)
        except Exception: pass
        w("     SkeletalMeshComponent 개수 = %d" % len(comps))
        for c in comps:
            try: cs = c.get_editor_property('cast_shadow')
            except Exception as e: cs = "ERR(%s)" % e
            try: ccs = c.get_editor_property('cast_contact_shadow')
            except Exception as e: ccs = "ERR(%s)" % e
            w("       - %-16s mesh=%-20s cast_shadow=%s cast_contact_shadow=%s" % (c.get_name(), mesh_name(c), cs, ccs))
    except Exception as e:
        w("     처리 예외: %s" % e)
w("")

# --- 디렉셔널 라이트 ---
w("=== DIRECTIONAL LIGHT ===")
try:
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in eas.get_all_level_actors():
        for c in a.get_components_by_class(unreal.DirectionalLightComponent):
            def gp(n):
                try: return c.get_editor_property(n)
                except Exception: return '?'
            w("[DirLight] '%s' intensity=%s contact_shadow_length=%s dynamic_shadow=%s" % (
                a.get_actor_label(), gp('intensity'), gp('contact_shadow_length'), gp('cast_dynamic_shadows')))
except Exception as e:
    w("라이트 조회 예외: %s" % e)

# --- 레벨의 VRM 스켈레탈 액터 ---
w("")
w("=== LEVEL의 VRM 스켈레탈 메시 액터 ===")
try:
    for a in eas.get_all_level_actors():
        for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
            nm = mesh_name(c)
            if 'VRM' in nm:
                try: cs = c.get_editor_property('cast_shadow')
                except Exception: cs = '?'
                w("[Level] '%s' mesh=%s cast_shadow=%s" % (a.get_actor_label(), nm, cs))
except Exception as e:
    w("레벨 액터 조회 예외: %s" % e)

try:
    with open("C:/Secret_Project/기획/99_보관/구로그/citoon_diag.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    unreal.log("==== WROTE C:/Secret_Project/기획/99_보관/구로그/citoon_diag.txt")
except Exception as e:
    unreal.log_error("리포트 쓰기 실패: " + str(e))
