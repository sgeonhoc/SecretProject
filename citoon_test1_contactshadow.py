# 테스트1: 디렉셔널 라이트의 Contact Shadow 끄기.
#   Contact Shadow = 화면공간 그림자라 카메라 각도에 따라 생겼다 사라졌다 함 → "각도 따라 겹치는" 1순위 용의자.
# 실행:  py "C:/Secret_Project/citoon_test1_contactshadow.py"
#   ※ PIE 켜져 있으면 일단 멈추고 → 이 스크립트 실행 → 다시 Play 해서 확인(에디터 월드를 PIE가 복제해 가니까).
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()
found = 0
for a in actors:
    for c in a.get_components_by_class(unreal.DirectionalLightComponent):
        cur = c.get_editor_property('contact_shadow_length')
        unreal.log("DirLight '%s': contact_shadow_length(before) = %s" % (a.get_actor_label(), cur))
        c.set_editor_property('contact_shadow_length', 0.0)
        unreal.log("  -> 0.0 으로 설정함 (원복하려면 이 값을 %s 로 되돌리면 됨)" % cur)
        found += 1

if found == 0:
    unreal.log_warning("DirectionalLightComponent를 못 찾음 — 하늘 BP 안에 있을 수 있음. 알려줘.")
else:
    unreal.log("==== 완료: 라이트 %d개 처리. 이제 Play(PIE) 다시 눌러서 검은 얼룩 보세요." % found)
