# 진짜 수정: 플레이어 메시에 render_custom_depth=True + custom_depth_stencil_value=1
#   → CiciToon 캐릭터 노출 PP가 NPC처럼 플레이어도 잡아줘서 노출 크러시(검은 얼룩) 해결.
#   BP CDO(스폰본) + Showcase 배치 액터 둘 다 적용. headless 전용(에디터 꺼진 상태).
import unreal

EAL = unreal.EditorAssetLibrary
log = []
def w(s):
    s = str(s); log.append(s);
    try: unreal.log("FIX> " + s)
    except Exception: pass
def gp(o, n):
    try: return o.get_editor_property(n)
    except Exception as e: return "ERR(%s)" % e
def setcd(c, tag):
    try:
        b1 = gp(c,'render_custom_depth'); b2 = gp(c,'custom_depth_stencil_value')
        c.set_editor_property('render_custom_depth', True)
        c.set_editor_property('custom_depth_stencil_value', 1)
        w("  %s: render_custom_depth %s->%s, stencil %s->%s" % (tag, b1, gp(c,'render_custom_depth'), b2, gp(c,'custom_depth_stencil_value')))
        return True
    except Exception as e:
        w("  %s 실패: %s" % (tag, e)); return False

# 1) 플레이어 BP CDO (스폰되는 본체)
w("[1] PLAYER BP CDO")
for p in ['/Game/BP_Characters/NewFolder/BP_PlayerCharacter',
          '/Game/BP_Characters/BP_PlayerCharacter',
          '/Game/BP_PlayerCharacter']:
    if not EAL.does_asset_exist(p): w("  없음: %s" % p); continue
    try:
        bp = unreal.load_asset(p); cdo = unreal.get_default_object(bp.generated_class())
    except Exception as e:
        w("  %s CDO 실패: %s" % (p, e)); continue
    comps = []
    try: comps = list(cdo.get_components_by_class(unreal.SkeletalMeshComponent))
    except Exception: pass
    try:
        m = cdo.get_editor_property('mesh')
        if m and m not in comps: comps.append(m)
    except Exception: pass
    any_set = False
    for c in comps:
        if setcd(c, "%s::%s" % (p.split('/')[-1], c.get_name())): any_set = True
    if any_set:
        try: EAL.save_asset(p); w("  saved %s" % p.split('/')[-1])
        except Exception as e: w("  save 실패: %s" % e)

# 2) Showcase 배치 액터(SK_VRM/BP_PlayerCharacter 등)
w("[2] Showcase 배치 액터")
try:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/CityPark/Maps/Showcase')
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    n = 0
    for a in eas.get_all_level_actors():
        for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
            try: sk = c.get_skeletal_mesh_asset(); nm = sk.get_name() if sk else ''
            except Exception: nm = ''
            if 'VRM' in nm:
                if setcd(c, "Level::%s" % a.get_actor_label()): n += 1
    if n > 0:
        try:
            unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
            w("  Showcase 저장: VRM 액터 %d개 적용" % n)
        except Exception as e:
            w("  레벨 저장 실패: %s" % e)
    else:
        w("  VRM 액터 못 찾음")
except Exception as e:
    w("  Showcase 처리 실패: %s" % e)

try:
    with open("C:/Secret_Project/기획/99_보관/구로그/citoon_customdepth_log.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(log))
except Exception: pass

# headless 종료
try:
    world = None
    try: world = unreal.EditorLevelLibrary.get_editor_world()
    except Exception: pass
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
except Exception: pass
