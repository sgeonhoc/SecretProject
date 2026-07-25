# headless 렌더 v2: 해 오는 쪽에 카메라(역광 실루엣 방지) + 노출 강제 고정 → 캐릭터 실제 모습 추출.
import unreal
LOG = "C:/Secret_Project/기획/99_보관/구로그/citoon_render_log.txt"; log = []
def w(s):
    s = str(s); log.append(s)
def flush():
    try: open(LOG, "w", encoding="utf-8").write("\n".join(log))
    except Exception: pass

world = None
try:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/CityPark/Maps/Showcase')
    world = unreal.EditorLevelLibrary.get_editor_world()
    w("loaded Showcase")
except Exception as e:
    w("load fail %s" % e)

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# 태양(디렉셔널 라이트) 방향 = 빛이 진행하는 방향
sun_fwd = unreal.Vector(1.0, 0.3, -0.7)
try:
    for a in eas.get_all_level_actors():
        if a.get_components_by_class(unreal.DirectionalLightComponent):
            sun_fwd = a.get_actor_forward_vector(); w("sun_fwd=%s (%s)" % (sun_fwd, a.get_actor_label())); break
except Exception as e:
    w("sun find fail %s" % e)

def mesh_of(a):
    for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
        try: sk = c.get_skeletal_mesh_asset(); nm = sk.get_name() if sk else ''
        except Exception: nm = ''
        return c, nm
    return None, ''

def render_actor(a, fname, ev=11.0):
    try:
        c, nm = mesh_of(a)
        loc = a.get_actor_location()
        # 해가 오는 쪽(=-sun_fwd)에 카메라 → 빛 받는 면이 카메라를 향함
        cam_loc = unreal.Vector(loc.x - sun_fwd.x*260.0, loc.y - sun_fwd.y*260.0, loc.z + 95.0)
        look = unreal.Vector(loc.x, loc.y, loc.z + 90.0)
        rot = unreal.MathLibrary.find_look_at_rotation(cam_loc, look)
        cap = eas.spawn_actor_from_class(unreal.SceneCapture2D, cam_loc, rot)
        comp = None
        try: comp = cap.get_editor_property('capture_component2d')
        except Exception: pass
        if comp is None:
            cs = cap.get_components_by_class(unreal.SceneCaptureComponent2D)
            comp = cs[0] if cs else None
        if comp is None:
            w("  %s: capture comp 못 얻음" % fname); eas.destroy_actor(cap); return
        rt = unreal.RenderingLibrary.create_render_target2d(world, 720, 960, unreal.TextureRenderTargetFormat.RTF_RGBA8)
        comp.set_editor_property('texture_target', rt)
        comp.set_editor_property('capture_source', unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
        comp.set_editor_property('fov_angle', 50.0)
        # 노출 강제 고정: min=max로 클램프 + manual
        try:
            pps = comp.get_editor_property('post_process_settings')
            pps.set_editor_property('override_auto_exposure_method', True)
            pps.set_editor_property('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL)
            pps.set_editor_property('override_auto_exposure_bias', True)
            pps.set_editor_property('auto_exposure_bias', float(ev))
            pps.set_editor_property('override_auto_exposure_min_brightness', True)
            pps.set_editor_property('auto_exposure_min_brightness', 1.0)
            pps.set_editor_property('override_auto_exposure_max_brightness', True)
            pps.set_editor_property('auto_exposure_max_brightness', 1.0)
            comp.set_editor_property('post_process_settings', pps)
        except Exception as e:
            w("  expo set 실패: %s" % e)
        for _ in range(6):
            comp.capture_scene()
        unreal.RenderingLibrary.export_render_target(world, rt, "C:/Secret_Project", fname)
        w("RENDER %s | %s mesh=%s" % (fname, a.get_actor_label(), nm))
        eas.destroy_actor(cap)
    except Exception as e:
        w("render %s 실패: %s" % (fname, e))

sk_vrm = None; npc = None
for a in eas.get_all_level_actors():
    c, nm = mesh_of(a)
    lbl = a.get_actor_label()
    if nm == 'SK_VRM' and lbl == 'SK_VRM' and sk_vrm is None: sk_vrm = a
    if nm and 'test' in nm.lower() and npc is None: npc = a

w("found SK_VRM=%s npc=%s" % (sk_vrm.get_actor_label() if sk_vrm else None, npc.get_actor_label() if npc else None))
if sk_vrm:
    render_actor(sk_vrm, "render_skvrm_lit.png", 11.0)
    render_actor(sk_vrm, "render_skvrm_lit15.png", 15.0)
if npc:
    render_actor(npc, "render_npc_lit.png", 11.0)
flush()

try:
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
except Exception: pass
