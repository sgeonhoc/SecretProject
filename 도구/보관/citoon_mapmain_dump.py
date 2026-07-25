# Map_Main 덤프: Hayakawa 액터/바운드/카메라/조명 파악 (헤드리스, 렌더 없음)
import unreal
LOG = "C:/Secret_Project/기획/99_보관/구로그/citoon_mapmain_dump.txt"; lines = []
def w(s):
    s = str(s); lines.append(s)
    try: print("DUMP> %s" % s, flush=True)
    except Exception: pass

try:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/CiciToonCharacterShaderPak/Levels/Map_Main')
    w("loaded Map_Main")
except Exception as e:
    w("load fail %s" % e)

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
alla = eas.get_all_level_actors()
w("total actors=%d" % len(alla))

def bounds(a):
    try:
        o, e = a.get_actor_bounds(False)
        return o, e
    except Exception as ex:
        return None, None

w("===== SKELETAL MESH ACTORS =====")
for a in alla:
    sks = a.get_components_by_class(unreal.SkeletalMeshComponent)
    if not sks: continue
    for c in sks:
        try: sk = c.get_skeletal_mesh_asset(); nm = sk.get_name() if sk else 'None'
        except Exception: nm = '?'
        loc = a.get_actor_location(); rot = a.get_actor_rotation()
        o, e = bounds(a)
        w("ACTOR label=%s | class=%s | mesh=%s" % (a.get_actor_label(), a.get_class().get_name(), nm))
        w("   loc=(%.1f,%.1f,%.1f) yaw=%.1f" % (loc.x, loc.y, loc.z, rot.yaw))
        if o: w("   bounds origin=(%.1f,%.1f,%.1f) extent=(%.1f,%.1f,%.1f) top_z=%.1f bot_z=%.1f" % (o.x,o.y,o.z,e.x,e.y,e.z,o.z+e.z,o.z-e.z))
        # 머티리얼 슬롯
        try:
            mats = c.get_materials()
            w("   materials(%d): %s" % (len(mats), [ (m.get_name() if m else 'None') for m in mats]))
        except Exception: pass

w("===== CAMERAS =====")
for a in alla:
    cn = a.get_class().get_name()
    if 'Camera' in cn:
        loc = a.get_actor_location(); rot = a.get_actor_rotation()
        w("CAM label=%s class=%s loc=(%.1f,%.1f,%.1f) rot=(p%.1f,y%.1f,r%.1f)" % (a.get_actor_label(), cn, loc.x,loc.y,loc.z, rot.pitch,rot.yaw,rot.roll))

w("===== DIRECTIONAL LIGHTS =====")
for a in alla:
    if a.get_components_by_class(unreal.DirectionalLightComponent):
        fwd = a.get_actor_forward_vector(); rot = a.get_actor_rotation()
        w("SUN label=%s fwd=(%.2f,%.2f,%.2f) rot=(p%.1f,y%.1f)" % (a.get_actor_label(), fwd.x,fwd.y,fwd.z, rot.pitch, rot.yaw))

w("===== POSTPROCESS / EXPOSURE =====")
for a in alla:
    if a.get_components_by_class(unreal.PostProcessComponent):
        w("PPV label=%s class=%s" % (a.get_actor_label(), a.get_class().get_name()))

try: open(LOG, "w", encoding="utf-8").write("\n".join(lines))
except Exception: pass
try: unreal.SystemLibrary.execute_console_command(unreal.EditorLevelLibrary.get_editor_world(), "QUIT_EDITOR")
except Exception: pass
