# 테스트2: 캐릭터 메시의 Cast Shadow 끄기 = 자기그림자(self-shadow)가 검은 얼룩의 범인인지 확인.
#   ⚠ 끄면 바닥 그림자도 같이 사라짐(진단용). 범인이면 'self만 완화' 처리는 따로.
# 실행:  py "C:/Secret_Project/citoon_test2_selfshadow.py"
#   ※ PIE 멈추고 실행 → 다시 Play 해서 확인.
import unreal

EAL = unreal.EditorAssetLibrary

bp_paths = [
    '/Game/BP_Characters/NewFolder/BP_PlayerCharacter',
    '/Game/BP_Characters/BP_PlayerCharacter',
    '/Game/BP_PlayerCharacter',
]

# (1) 플레이어 BP들의 스켈레탈 메시 컴포넌트 cast_shadow 끄기 + 어떤 BP가 SK_VRM 물고 있는지 보고
for p in bp_paths:
    if not EAL.does_asset_exist(p):
        unreal.log("[BP] 없음: %s" % p); continue
    bp = unreal.load_asset(p)
    try:
        cdo = unreal.get_default_object(bp.generated_class())
        comps = cdo.get_components_by_class(unreal.SkeletalMeshComponent)
        if not comps:
            unreal.log("[BP] %s :: SkeletalMeshComponent 없음" % p.split('/')[-1])
        for c in comps:
            try: sk = c.get_skeletal_mesh_asset()
            except Exception: sk = None
            skname = sk.get_name() if sk else '(none)'
            cur = c.get_editor_property('cast_shadow')
            c.set_editor_property('cast_shadow', False)
            unreal.log("[BP] %-22s :: mesh=%-18s cast_shadow %s -> False" % (p.split('/')[-1], skname, cur))
        EAL.save_asset(p)
    except Exception as e:
        unreal.log_error("[BP] %s 처리 실패: %s" % (p, e))

# (2) 레벨에 배치된 SK_VRM 액터도 같이(에디터 뷰포트에서도 바로 보이게)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
        try: sk = c.get_skeletal_mesh_asset()
        except Exception: sk = None
        nm = sk.get_name() if sk else ''
        if 'VRM' in nm:
            cur = c.get_editor_property('cast_shadow')
            c.set_editor_property('cast_shadow', False)
            unreal.log("[Level] '%s' (mesh=%s) cast_shadow %s -> False" % (a.get_actor_label(), nm, cur))

unreal.log("==== 테스트2 완료. PIE 다시 Play → 몸의 검은 얼룩이 사라졌는지 보세요(바닥그림자 사라지는 건 정상).")
