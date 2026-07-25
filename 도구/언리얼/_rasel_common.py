# -*- coding: utf-8 -*-
"""라셀 레벨 조립 공통 살림 — 여러 레벨 빌더가 함께 쓴다.

여기 모은 것: 맵 열기 가드 · 부재 놓기 · 바닥 자리 예약 · 하루 세 때 조명 · 저장.
★맵 열기 가드가 특히 중요하다 — `new_level`이 실패하면 조용히 옛 월드에 머물러
  스폰이 엉뚱한 맵(시작 화면 Title)에 쏟아진다. 실제로 그 사고로 Title이 1,848 액터까지
  불어난 적이 있다(2026-07-24). 그래서 열린 월드 이름을 확인하고 아니면 즉시 멈춘다.
★에디터가 그 맵을 열어 두면 .umap이 잠겨 저장이 전부 실패한다 → `_next`에 짓는다.

쓰는 법:
    import sys; sys.path.append("C:/Secret_Project/도구/언리얼")
    import _rasel_common as C
    C.begin("/Game/Maps/Rasel/L03_Backalley", "l03")
    C.place("SM_Rasel_AlleyFloor", (0, 0, 0))
    C.finish_level()
"""
import unreal, json, os

MESHDIR = "/Game/Rasel/Meshes"
MANIFEST = "C:/Secret_Project/_ArtSource/kit_manifest.json"

lines = []
_AS = None
_MATS = {}
_MESHES = {}
KIT = {}
MAP = ""
LOG = "C:/Secret_Project/Saved/rasel_build.log"


def log(m):
    lines.append(str(m))
    unreal.log("[rasel] " + str(m))


def flush():
    with open(LOG, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


def A():
    global _AS
    if _AS is None:
        _AS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return _AS


def mat(path):
    if path not in _MATS:
        _MATS[path] = unreal.EditorAssetLibrary.load_asset(path)
    return _MATS[path]


def mesh(name):
    if name not in _MESHES:
        _MESHES[name] = unreal.EditorAssetLibrary.load_asset("%s/%s" % (MESHDIR, name))
    return _MESHES[name]


# ── 맵 열기 ───────────────────────────────────────────────────────────
def begin(map_path, log_name="rasel_build"):
    """대상 맵을 열어 비운다. 잠겨 있으면 `_next`에 짓는다. 실패하면 예외로 멈춘다."""
    global MAP, KIT, LOG
    MAP = map_path
    LOG = "C:/Secret_Project/Saved/%s.log" % log_name
    with open(MANIFEST, "r", encoding="utf-8") as f:
        KIT = json.load(f)

    disk = "C:/Secret_Project/Content" + MAP[len("/Game"):] + ".umap"
    if os.path.exists(disk):
        try:
            with open(disk, "r+b"):
                pass
        except Exception:
            MAP = MAP + "_next"
            log("★대상 맵이 에디터에 열려 잠김 → 대신 %s 에 짓는다" % MAP)

    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    want = MAP.rsplit("/", 1)[1]
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        les.load_level(MAP)
    else:
        les.new_level(MAP)
    got = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_name()
    if got != want:
        raise RuntimeError("맵 열기 실패 — 열린 월드가 '%s'다. 여기에 스폰하면 그 맵을 망친다." % got)
    old = list(A().get_all_level_actors())
    for a in old:
        A().destroy_actor(a)
    log("%s 열어서 옛 액터 %d개 비움" % (got, len(old)))
    _TAKEN[:] = []
    return MAP


def finish_level():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    n = sum(1 for a in A().get_all_level_actors()
            if isinstance(a, unreal.StaticMeshActor))
    ok = les.save_current_level()
    log("저장 -> %s · 부재 액터 %d개" % (ok, n))
    if not ok:
        raise RuntimeError("맵 저장 실패 — 디스크에 안 쓰였다.")
    flush()
    return n


# ── 부재 놓기 ─────────────────────────────────────────────────────────
def place(name, loc, yaw=0.0, pitch=0.0, roll=0.0, label=None, scale=None, mats=None):
    sm = mesh(name)
    if sm is None:
        log("  !! 부재 없음 %s" % name)
        return None
    a = A().spawn_actor_from_object(sm, unreal.Vector(*loc),
                                    unreal.Rotator(roll, pitch, yaw))
    if a is None:
        return None
    comp = a.static_mesh_component
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    if scale:
        a.set_actor_scale3d(unreal.Vector(*scale))
    for i, mp in enumerate(mats if mats is not None else KIT.get(name, [])):
        m = mat(mp)
        if m:
            comp.set_material(i, m)
    a.set_actor_label(label or name)
    return a


# ── 바닥 자리 예약 ────────────────────────────────────────────────────
_TAKEN = []
FOOT = {"Stall_A": 130, "Stall_B": 130, "Crate": 48, "Barrel": 36, "Sack": 32,
        "Basket": 34, "RopeCoil": 38, "Bucket": 26, "Stool": 26, "Pot": 36,
        "Poles": 34, "Handcart": 125, "Lantern": 28, "Well": 115, "Bench": 100,
        "NoticeBoard": 95, "Manhole": 60,
        # 실내 부재 — 이게 비어 있으면 기본 40으로 잡혀 상이 문을 막는다
        "Table": 65, "Counter": 70, "Partition": 70, "Hearth": 110,
        "Shelf": 155, "Bowl": 12}


def ground_free(x, y, r, pad=16.0):
    for (tx, ty, tr) in _TAKEN:
        dx, dy = x - tx, y - ty
        if dx * dx + dy * dy < (r + tr + pad) ** 2:
            return False
    return True


def reserve(x, y, r):
    _TAKEN.append((x, y, r))


def reserve_line(x1, y1, x2, y2, r, n=3):
    """★긴 물건(카운터·상 줄)은 원 하나로 잡으면 반경이 터무니없이 커져
    옆의 상까지 밀어낸다. 길이를 따라 작은 원 여러 개로 잡는다."""
    for k in range(n):
        t = k / float(n - 1) if n > 1 else 0.0
        reserve(x1 + (x2 - x1) * t, y1 + (y2 - y1) * t, r)


def put_ground(kind, x, y, yaw=0.0, label=None, tries=None):
    """빈자리면 놓고 발자국을 적는다. 막히면 몇 자리 옮겨 본다."""
    r = FOOT.get(kind, 40)
    for (ox, oy) in (tries or ((0, 0),)):
        if ground_free(x + ox, y + oy, r):
            place("SM_Rasel_%s" % kind, (x + ox, y + oy, 0.0), yaw=yaw, label=label)
            reserve(x + ox, y + oy, r)
            return True
    log("  · 자리 없어 건너뜀: %s (%s)" % (label or kind, kind))
    return False


# ── 배선(우리 C++ 액터) ───────────────────────────────────────────────
def cls(name):
    c = getattr(unreal, name, None)
    return c if c is not None else unreal.load_class(None, "/Script/Secret_Project." + name)


def setp(a, k, v):
    try:
        a.set_editor_property(k, v)
    except Exception as e:
        log("  ! set %s = %s" % (k, e))


def portal(loc, target, label, req=None, forb=None):
    a = A().spawn_actor_from_class(cls("PortalActor"), unreal.Vector(*loc))
    setp(a, "TargetLevelName", target)
    if req:
        setp(a, "RequiredFlag", req)
    if forb:
        setp(a, "ForbiddenFlag", forb)
    a.set_actor_label(label)
    return a


def lore(loc, title, body, req=None, forb=None, yaw=0.0):
    a = A().spawn_actor_from_class(cls("LoreNoteActor"), unreal.Vector(*loc),
                                   unreal.Rotator(0.0, 0.0, yaw))
    setp(a, "Title", title)
    setp(a, "Lines", body)
    if req:
        setp(a, "RequiredFlag", req)
    if forb:
        setp(a, "ForbiddenFlag", forb)
    a.set_actor_label("조사: " + title)
    return a


def npc(loc, name, dialogue, yaw=0.0, phase=None, shop=None):
    a = A().spawn_actor_from_class(cls("ANPCCharacter"), unreal.Vector(*loc),
                                   unreal.Rotator(0.0, 0.0, yaw))
    setp(a, "NPCName", name)
    setp(a, "DialogueLines", dialogue)
    setp(a, "bCanEnterCombat", False)
    if shop:
        setp(a, "bIsShopkeeper", True)
        setp(a, "ShopKind", shop)
    if phase is not None:
        setp(a, "ActivePhases", [phase])
    a.set_actor_label("NPC " + name)
    return a


def day_phase(name=None):
    p = name or os.environ.get("RASEL_PHASE", "evening")
    return p if p in PHASES else "evening"


# (해 pitch, 해 yaw, 해 lux, 해 색, 하늘 세기, 안개 짙기, 안개 색, 가로등 lumen, 노출 보정)
PHASES = {
    "day":     (-68.0, -20.0, 7.0, (1.0, 0.97, 0.92), 5.5, 0.005,
                (0.46, 0.50, 0.56), 0.0, -0.2),
    "evening": (-34.0, -62.0, 6.5, (1.0, 0.80, 0.56), 2.6, 0.008,
                (0.30, 0.25, 0.22), 2600.0, 0.6),
    "night":   (-14.0, 118.0, 0.35, (0.55, 0.62, 0.85), 0.9, 0.013,
                (0.10, 0.12, 0.18), 3200.0, 1.4),
}


def sky_and_exposure(center, phase, exposure_bias=None, volumetric=True):
    """해·보조광·하늘·안개·노출 한 벌."""
    (pitch, yaw, lux, col, sky_i, fog_d, fog_c, lamp, exp) = PHASES[phase]
    cx, cy = center
    dl = A().spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(cx, cy, 1600))
    dl.set_actor_rotation(unreal.Rotator(0.0, pitch, yaw), False)
    lc = dl.get_component_by_class(unreal.DirectionalLightComponent)
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_intensity(lux)
    lc.set_light_color(unreal.LinearColor(*col))
    lc.set_editor_property("atmosphere_sun_light", True)
    dl.set_actor_label("해 (%s)" % phase)

    if lux > 1.0:
        fl = A().spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(cx, cy, 1500))
        fl.set_actor_rotation(unreal.Rotator(0.0, -32.0, yaw + 180.0), False)
        flc = fl.get_component_by_class(unreal.DirectionalLightComponent)
        flc.set_mobility(unreal.ComponentMobility.MOVABLE)
        flc.set_intensity(lux * 0.30)
        flc.set_light_color(unreal.LinearColor(0.72, 0.80, 1.0))
        flc.set_editor_property("cast_shadows", False)
        flc.set_editor_property("atmosphere_sun_light", False)
        fl.set_actor_label("보조광 (%s)" % phase)

    A().spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(cx, cy, 0))
    fog = A().spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(cx, cy, 100))
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property("fog_density", fog_d)
    fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(*fog_c))
    fc.set_editor_property("fog_height_falloff", 0.22)
    if volumetric:
        for nm_ in ("volumetric_fog", "enable_volumetric_fog"):
            try:
                fc.set_editor_property(nm_, True)
                break
            except Exception:
                continue
        try:
            fc.set_editor_property("volumetric_fog_extinction_scale", 1.4)
        except Exception:
            pass
    sky = A().spawn_actor_from_class(unreal.SkyLight, unreal.Vector(cx, cy, 900))
    skc = sky.get_component_by_class(unreal.SkyLightComponent)
    skc.set_mobility(unreal.ComponentMobility.MOVABLE)
    skc.set_editor_property("real_time_capture", True)
    skc.set_editor_property("intensity", sky_i)

    ppv = A().spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(cx, cy, 400))
    ppv.set_editor_property("unbound", True)
    s = ppv.get_editor_property("settings")
    s.set_editor_property("override_auto_exposure_method", True)
    s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    s.set_editor_property("override_auto_exposure_bias", True)
    s.set_editor_property("auto_exposure_bias", exp if exposure_bias is None else exposure_bias)
    s.set_editor_property("override_auto_exposure_min_brightness", True)
    s.set_editor_property("auto_exposure_min_brightness", 0.05)
    s.set_editor_property("override_auto_exposure_max_brightness", True)
    s.set_editor_property("auto_exposure_max_brightness", 2.2)
    try:
        s.set_editor_property("override_bloom_intensity", True)
        s.set_editor_property("bloom_intensity", 0.75)
    except Exception:
        pass
    ppv.set_editor_property("settings", s)
    ppv.set_actor_label("노출·색")
    return lamp


def point_light(loc, lumens, color, radius=700.0, label="점광",
                shadows=True, volumetric=2.0, source_radius=6.0):
    pl = A().spawn_actor_from_class(unreal.PointLight, unreal.Vector(*loc))
    c = pl.get_component_by_class(unreal.PointLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    c.set_intensity(lumens)
    c.set_attenuation_radius(radius)
    c.set_light_color(unreal.LinearColor(*color))
    c.set_editor_property("cast_shadows", shadows)
    c.set_editor_property("source_radius", source_radius)
    c.set_editor_property("volumetric_scattering_intensity", volumetric)
    pl.set_actor_label(label)
    return pl
