# 테스트4: 디렉셔널 라이트의 '동적 그림자'를 통째로 끔 = 검은 띠가 그림자인지 GI차폐인지 가르는 결정타.
#   (씬 전체 동적 그림자가 사라짐 — 진단용. 원복값은 로그에 남김.)
# 실행:  py "C:/Secret_Project/citoon_test4_nodynshadow.py"   (★PIE 정지 → 실행 → Play)
import unreal

log = []
def w(s):
    s = str(s); log.append(s); unreal.log(s)

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
found = 0
for a in eas.get_all_level_actors():
    for c in a.get_components_by_class(unreal.DirectionalLightComponent):
        def gp(n):
            try: return c.get_editor_property(n)
            except Exception: return '?'
        w("[DirLight] '%s' intensity=%s cast_dynamic_shadows(before)=%s cast_shadows=%s" % (
            a.get_actor_label(), gp('intensity'), gp('cast_dynamic_shadows'), gp('cast_shadows')))
        try:
            c.set_editor_property('cast_dynamic_shadows', False)
            w("   -> cast_dynamic_shadows = False (원복: True)")
            found += 1
        except Exception as e:
            w("   set 실패: %s" % e)

if found == 0:
    w("!! DirectionalLightComponent 못 찾음(플레이모드면 정지 후 다시 / 하늘BP면 알려줘).")
else:
    w("==== 완료: 라이트 %d개 동적그림자 OFF. PIE 다시 Play해서 캐릭터 검은 띠 보세요." % found)

try:
    with open("C:/Secret_Project/기획/99_보관/구로그/citoon_test4_log.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(log))
except Exception as e:
    unreal.log_error("로그 실패: " + str(e))
