# headless 덤프2: CiciToon 데모맵의 노출/포스트프로세스/캐릭터 커스텀뎁스 배선 + 노출PP 재질 + Showcase 캐릭터 상태
import unreal
OUT = "C:/Secret_Project/기획/99_보관/구로그/citoon_demo_dump.txt"
out = []
def w(s):
    s = str(s); out.append(s)
def flush():
    try:
        with open(OUT, "w", encoding="utf-8") as f: f.write("\n".join(out))
    except Exception: pass
def gp(o, n):
    try: return o.get_editor_property(n)
    except Exception as e: return "ERR(%s)" % e
EAL = unreal.EditorAssetLibrary

w("===== DEMO / EXPOSURE WIRING DUMP =====")

# [A] 노출 PP 머티리얼 + 인스턴스 파라미터
w("")
w("[A] CharacterExposure PP material")
mp = '/Game/OUTLINE/M_CharacterExposure_PP'
if EAL.does_asset_exist(mp):
    m = unreal.load_asset(mp)
    w("  %s domain=%s blendable_location=%s blend=%s" % (mp, gp(m,'material_domain'), gp(m,'blendable_location'), gp(m,'blend_mode')))
for p in ['/Game/OUTLINE/M_CharacterExposure_PP_Inst','/Game/OUTLINE/M_CharacterExposure_PP_Inst2','/Game/M_CharacterExposure_PP_Inst']:
    if EAL.does_asset_exist(p):
        mi = unreal.load_asset(p)
        w("  INST %s parent=%s" % (p, gp(mi,'parent')))
        sps = gp(mi,'scalar_parameter_values')
        if not isinstance(sps, str):
            for sp in sps:
                try: w("     scalar %s = %s" % (sp.get_editor_property('parameter_info').get_editor_property('name'), sp.get_editor_property('parameter_value')))
                except Exception as e: w("     scalar ? %s" % e)

def dump_level(tag):
    w("")
    w("[LEVEL %s]" % tag)
    try:
        eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        actors = list(eas.get_all_level_actors())
    except Exception as e:
        w("  actors 실패: %s" % e); return
    w("  actor count=%d" % len(actors))
    for a in actors:
        try:
            for c in a.get_components_by_class(unreal.DirectionalLightComponent):
                w("  [DirLight] %s intensity=%s" % (a.get_actor_label(), gp(c,'intensity')))
            for c in a.get_components_by_class(unreal.SkyLightComponent):
                w("  [SkyLight] %s intensity=%s" % (a.get_actor_label(), gp(c,'intensity')))
            if isinstance(a, unreal.PostProcessVolume):
                en = gp(a,'enabled'); ub = gp(a,'unbound')
                ps = gp(a,'settings')
                method = gp(ps,'auto_exposure_method') if not isinstance(ps,str) else '?'
                ovm = gp(ps,'override_auto_exposure_method') if not isinstance(ps,str) else '?'
                bias = gp(ps,'auto_exposure_bias') if not isinstance(ps,str) else '?'
                w("  [PPV] %s enabled=%s unbound=%s prio=%s | method=%s(ov=%s) bias=%s" % (a.get_actor_label(), en, ub, gp(a,'priority'), method, ovm, bias))
                # weighted blendables (post process materials)
                if not isinstance(ps,str):
                    wb = gp(ps,'weighted_blendables')
                    arr = gp(wb,'array') if not isinstance(wb,str) else []
                    if not isinstance(arr,str):
                        for it in arr:
                            obj = gp(it,'object'); wt = gp(it,'weight')
                            nm = obj.get_name() if (obj and not isinstance(obj,str)) else obj
                            w("        blendable: %s (w=%s)" % (nm, wt))
            for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
                try: sk = c.get_skeletal_mesh_asset(); nm = sk.get_name() if sk else ''
                except Exception: nm = ''
                if nm:
                    mats = gp(c,'override_materials')
                    m0 = ''
                    try:
                        if mats and not isinstance(mats,str) and len(mats)>0 and mats[0]: m0 = mats[0].get_name()
                    except Exception: pass
                    w("  [SKEL] %s mesh=%s render_custom_depth=%s stencil=%s ov_mat0=%s" % (a.get_actor_label(), nm, gp(c,'render_custom_depth'), gp(c,'custom_depth_stencil_value'), m0))
        except Exception as e:
            w("  actor 예외: %s" % e)

# [B] 데모맵 찾기 + 덤프
w("")
w("[B] CiciToon 데모맵 탐색")
try:
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    worlds = ar.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Engine','World'), False)
    demo = [str(a.package_name) for a in worlds if 'CiciToon' in str(a.package_name)]
    w("  데모맵 후보: %s" % demo)
except Exception as e:
    demo = []; w("  AssetRegistry 실패: %s" % e)

for mpath in demo[:2]:
    try:
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(mpath)
        dump_level("DEMO " + mpath.split('/')[-1])
        flush()
    except Exception as e:
        w("  데모맵 %s 로드 실패: %s" % (mpath, e))

# [C] Showcase 캐릭터 커스텀뎁스 상태
try:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/CityPark/Maps/Showcase')
    dump_level("Showcase(VRM 커스텀뎁스 확인)")
except Exception as e:
    w("Showcase 재로드 실패: %s" % e)

w("")
w("===== DONE =====")
flush()

# headless 종료
try:
    world = None
    try: world = unreal.EditorLevelLibrary.get_editor_world()
    except Exception: pass
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
except Exception:
    pass
