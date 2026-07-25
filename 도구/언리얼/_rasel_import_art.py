# -*- coding: utf-8 -*-
"""라셀 아트 반입 — 우리가 구운 PNG를 텍스처로 들이고, 그 텍스처를 물린 머티리얼을 짓는다.
외부 팩 참조 0. 기존 M_Rasel_* 이름을 그대로 쓰므로 이미 지어진 레벨도 같이 좋아진다.

실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript="C:/Secret_Project/_rasel_import_art.py"
"""
import unreal, os, traceback

SRC = "C:/Secret_Project/_ArtSource/Textures"
TEXDIR = "/Game/Rasel/Textures"
MATDIR = "/Game/Rasel/Materials"
LOG = "C:/Secret_Project/Saved/rasel_art.log"
lines = []


def log(m):
    lines.append(str(m))
    unreal.log("ART: " + str(m))


def flush():
    with open(LOG, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()

# ── 텍스처 반입 ────────────────────────────────────────────────────────
# (파일이름, 쓰임)  쓰임: C=색 / N=결(노멀) / R=거칠기
TEXTURES = [
    ("T_Rasel_Cobble_C", "C"), ("T_Rasel_Cobble_N", "N"), ("T_Rasel_Cobble_R", "R"),
    ("T_Rasel_Plaster_C", "C"), ("T_Rasel_Plaster_N", "N"), ("T_Rasel_Plaster_R", "R"),
    ("T_Rasel_Wood_C", "C"), ("T_Rasel_Wood_N", "N"), ("T_Rasel_Wood_R", "R"),
    ("T_Rasel_Roof_C", "C"), ("T_Rasel_Roof_N", "N"), ("T_Rasel_Roof_R", "R"),
    ("T_Rasel_OldStone_C", "C"), ("T_Rasel_OldStone_N", "N"), ("T_Rasel_OldStone_R", "R"),
    ("T_Rasel_ClothA_C", "C"), ("T_Rasel_ClothA_N", "N"),
    ("T_Rasel_ClothB_C", "C"), ("T_Rasel_ClothB_N", "N"),
    ("T_Rasel_Paper_C", "C"),
]


def import_textures():
    # ★비동기 에셋 컴파일을 끈다 — 켜 두면 커맨들릿이 텍스처를 다 굽기 전에 끝나 버려서
    #   나중에 찍으면 온 표면이 엔진 기본 체커로 나온다.
    try:
        unreal.SystemLibrary.execute_console_command(None, "Editor.AsyncAssetCompilation 0")
    except Exception as e:
        log("  ! 동기 컴파일 전환 실패 %s" % e)
    tasks = []
    for name, kind in TEXTURES:
        p = os.path.join(SRC, name + ".png")
        if not os.path.exists(p):
            log("  !! 원본 없음 %s" % p)
            continue
        t = unreal.AssetImportTask()
        t.filename = p
        t.destination_path = TEXDIR
        t.destination_name = name
        t.automated = True
        t.replace_existing = True
        t.save = True
        tasks.append(t)
    AT.import_asset_tasks(tasks)

    ok = 0
    for name, kind in TEXTURES:
        path = "%s/%s" % (TEXDIR, name)
        if not EAL.does_asset_exist(path):
            log("  !! 반입 실패 %s" % name)
            continue
        tex = EAL.load_asset(path)
        if kind == "N":
            tex.set_editor_property("compression_settings",
                                    unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
        elif kind == "R":
            tex.set_editor_property("compression_settings",
                                    unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
        else:
            tex.set_editor_property("srgb", True)
        # 스트리밍을 끈다 — 커맨들릿 촬영에서 밉이 안 올라와 자리표시 무늬가 나오는 것을 막고,
        # 거리 텍스처가 스무 장 남짓이라 상시 적재해도 부담이 없다.
        tex.set_editor_property("never_stream", True)
        EAL.save_asset(path, False)
        ok += 1
    log("[텍스처] %d/%d 반입·설정 완료" % (ok, len(TEXTURES)))


# ── 머티리얼 저작 ──────────────────────────────────────────────────────
def get_or_make_material(name):
    path = "%s/%s" % (MATDIR, name)
    if EAL.does_asset_exist(path):
        mat = EAL.load_asset(path)
        MEL.delete_all_material_expressions(mat)     # 헌 단색 그래프 비우기
        return mat
    return AT.create_asset(name, MATDIR, unreal.Material, unreal.MaterialFactoryNew())


def tex(name):
    path = "%s/%s" % (TEXDIR, name)
    return EAL.load_asset(path) if EAL.does_asset_exist(path) else None


def _grime_mask(mat, height=260.0, power=2.4, patch_scale=0.004):
    """때가 앉는 정도(0~1) — 바닥에 가까울수록 짙고, 얼룩덜룩하게 흩어진다.
    월드 높이를 쓰므로 어느 부재에 붙어도 알아서 아랫도리부터 탄다."""
    wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1500, 900)
    mask = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1320, 900)
    mask.set_editor_property("r", False)
    mask.set_editor_property("g", False)
    mask.set_editor_property("b", True)
    mask.set_editor_property("a", False)
    MEL.connect_material_expressions(wp, "", mask, "")

    div = MEL.create_material_expression(mat, unreal.MaterialExpressionDivide, -1150, 900)
    hc = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -1320, 1010)
    hc.set_editor_property("r", height)
    MEL.connect_material_expressions(mask, "", div, "A")
    MEL.connect_material_expressions(hc, "", div, "B")

    inv = MEL.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -1000, 900)
    MEL.connect_material_expressions(div, "", inv, "")
    clamp = MEL.create_material_expression(mat, unreal.MaterialExpressionSaturate, -880, 900)
    MEL.connect_material_expressions(inv, "", clamp, "")
    pw = MEL.create_material_expression(mat, unreal.MaterialExpressionPower, -760, 900)
    pc = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -880, 1010)
    pc.set_editor_property("r", power)
    MEL.connect_material_expressions(clamp, "", pw, "Base")
    MEL.connect_material_expressions(pc, "", pw, "Exp")

    # 얼룩 — 띠처럼 균일하게 타지 않도록 잡음을 곱한다
    nz = MEL.create_material_expression(mat, unreal.MaterialExpressionNoise, -1000, 1120)
    try:
        nz.set_editor_property("scale", patch_scale)
        nz.set_editor_property("levels", 3)
        nz.set_editor_property("output_min", 0.35)
        nz.set_editor_property("output_max", 1.15)
    except Exception:
        pass
    mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -620, 900)
    MEL.connect_material_expressions(pw, "", mul, "A")
    MEL.connect_material_expressions(nz, "", mul, "B")
    out = MEL.create_material_expression(mat, unreal.MaterialExpressionSaturate, -500, 900)
    MEL.connect_material_expressions(mul, "", out, "")
    return out


def make_textured_material(name, base, normal=None, rough=None, tiling=1.0,
                           tint=None, rough_lo=0.35, rough_hi=0.95, spec=0.5,
                           grime=None, grime_height=260.0):
    """색·결·거칠기 텍스처를 물린 머티리얼. tiling = UV 한 칸을 몇 번 반복할지.
    grime=(r,g,b) 를 주면 아랫도리부터 때가 앉는 층을 얹는다."""
    mat = get_or_make_material(name)
    tc = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    tc.set_editor_property("u_tiling", tiling)
    tc.set_editor_property("v_tiling", tiling)

    bt = tex(base)
    if bt is None:
        log("  !! %s 색 텍스처 없음(%s)" % (name, base))
        return None
    bs = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, -200)
    bs.set_editor_property("texture", bt)
    MEL.connect_material_expressions(tc, "", bs, "UVs")

    out, pin = bs, "RGB"
    if tint is not None:
        mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, -200)
        c = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -520, -60)
        c.set_editor_property("constant", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0))
        MEL.connect_material_expressions(bs, "RGB", mul, "A")
        MEL.connect_material_expressions(c, "", mul, "B")
        out, pin = mul, ""

    gm = None
    if grime is not None:
        gm = _grime_mask(mat, height=grime_height)
        # 때 낀 색 = 원래 색을 어둡게 눌러 만든 것(따로 칠한 색이 아니라 같은 벽이 탄 것)
        gc = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -520, 120)
        gc.set_editor_property("constant",
                               unreal.LinearColor(grime[0], grime[1], grime[2], 1.0))
        dirty = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -360, 120)
        MEL.connect_material_expressions(out, pin, dirty, "A")
        MEL.connect_material_expressions(gc, "", dirty, "B")
        lp = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -200, -60)
        MEL.connect_material_expressions(out, pin, lp, "A")
        MEL.connect_material_expressions(dirty, "", lp, "B")
        MEL.connect_material_expressions(gm, "", lp, "Alpha")
        out, pin = lp, ""

    MEL.connect_material_property(out, pin, unreal.MaterialProperty.MP_BASE_COLOR)

    nt = tex(normal) if normal else None
    if nt is not None:
        ns = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, 120)
        ns.set_editor_property("texture", nt)
        ns.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        MEL.connect_material_expressions(tc, "", ns, "UVs")
        MEL.connect_material_property(ns, "RGB", unreal.MaterialProperty.MP_NORMAL)

    rt = tex(rough) if rough else None
    if rt is not None:
        rs = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, 420)
        rs.set_editor_property("texture", rt)
        st = None
        for cand in ("SAMPLERTYPE_MASKS", "SAMPLERTYPE_LINEAR_GRAYSCALE",
                     "SAMPLERTYPE_GRAYSCALE"):
            st = getattr(unreal.MaterialSamplerType, cand, None)
            if st is not None:
                break
        if st is not None:
            rs.set_editor_property("sampler_type", st)
        MEL.connect_material_expressions(tc, "", rs, "UVs")
        lerp = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -280, 420)
        a = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -460, 560)
        a.set_editor_property("r", rough_lo)
        b = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -460, 620)
        b.set_editor_property("r", rough_hi)
        MEL.connect_material_expressions(a, "", lerp, "A")
        MEL.connect_material_expressions(b, "", lerp, "B")
        MEL.connect_material_expressions(rs, "R", lerp, "Alpha")
        rough_out = lerp
    else:
        rc = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 420)
        rc.set_editor_property("r", rough_hi)
        rough_out = rc
    if gm is not None:                      # 때 낀 자리는 빛을 안 되쏜다
        gr = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -140, 420)
        gv = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 500)
        gv.set_editor_property("r", 0.99)
        MEL.connect_material_expressions(rough_out, "", gr, "A")
        MEL.connect_material_expressions(gv, "", gr, "B")
        MEL.connect_material_expressions(gm, "", gr, "Alpha")
        rough_out = gr
    MEL.connect_material_property(rough_out, "", unreal.MaterialProperty.MP_ROUGHNESS)

    sc = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 560)
    sc.set_editor_property("r", spec)
    MEL.connect_material_property(sc, "", unreal.MaterialProperty.MP_SPECULAR)

    try:
        MEL.recompile_material(mat)
    except Exception:
        pass
    EAL.save_asset("%s/%s" % (MATDIR, name), False)
    log("  · %s" % name)
    return mat


def make_flat_material(name, rgb, rough=0.6, metal=0.0, emissive=None, spec=0.5):
    mat = get_or_make_material(name)
    c = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    c.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
    r.set_editor_property("r", rough)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if metal > 0.0:
        m = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 300)
        m.set_editor_property("r", metal)
        MEL.connect_material_property(m, "", unreal.MaterialProperty.MP_METALLIC)
    s = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 360)
    s.set_editor_property("r", spec)
    MEL.connect_material_property(s, "", unreal.MaterialProperty.MP_SPECULAR)
    if emissive is not None:
        e = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 440)
        e.set_editor_property("constant", unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0))
        MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    try:
        MEL.recompile_material(mat)
    except Exception:
        pass
    EAL.save_asset("%s/%s" % (MATDIR, name), False)
    log("  · %s (단색)" % name)
    return mat


def build_materials():
    log("[머티리얼]")
    # 결이 있는 것 — 우리가 구운 텍스처를 물린다
    make_textured_material("M_Rasel_Cobble", "T_Rasel_Cobble_C", "T_Rasel_Cobble_N",
                           "T_Rasel_Cobble_R", tiling=1.0, rough_lo=0.45, rough_hi=0.95)
    # 회벽·나무·옛 돌은 아랫도리부터 때가 앉는다(비 튀고 발길 닿는 높이).
    make_textured_material("M_Rasel_Plaster", "T_Rasel_Plaster_C", "T_Rasel_Plaster_N",
                           "T_Rasel_Plaster_R", tiling=1.0, rough_lo=0.6, rough_hi=1.0,
                           grime=(0.42, 0.40, 0.36), grime_height=300.0)
    make_textured_material("M_Rasel_Wood", "T_Rasel_Wood_C", "T_Rasel_Wood_N",
                           "T_Rasel_Wood_R", tiling=2.0, rough_lo=0.5, rough_hi=0.95,
                           grime=(0.55, 0.52, 0.48), grime_height=180.0)
    make_textured_material("M_Rasel_Roof", "T_Rasel_Roof_C", "T_Rasel_Roof_N",
                           "T_Rasel_Roof_R", tiling=1.15, rough_lo=0.4, rough_hi=0.9)
    # 실내 회벽 — 밖보다 따뜻하고 덜 때 탄다(등불 아래 사는 방). 때 층 없음.
    make_textured_material("M_Rasel_PlasterIn", "T_Rasel_Plaster_C", "T_Rasel_Plaster_N",
                           "T_Rasel_Plaster_R", tiling=1.0, rough_lo=0.55, rough_hi=0.9,
                           tint=(1.08, 0.98, 0.86))
    # 실내 마루 — 밖 나무보다 짙고 반들거린다(발길에 닳은 마루널)
    make_textured_material("M_Rasel_FloorWood", "T_Rasel_Wood_C", "T_Rasel_Wood_N",
                           "T_Rasel_Wood_R", tiling=1.4, rough_lo=0.35, rough_hi=0.7,
                           tint=(0.82, 0.66, 0.48))
    make_textured_material("M_Rasel_OldStone", "T_Rasel_OldStone_C", "T_Rasel_OldStone_N",
                           "T_Rasel_OldStone_R", tiling=1.0, rough_lo=0.6, rough_hi=1.0,
                           grime=(0.48, 0.46, 0.42), grime_height=220.0)
    make_textured_material("M_Rasel_ClothA", "T_Rasel_ClothA_C", "T_Rasel_ClothA_N",
                           None, tiling=1.5, rough_hi=0.85)
    make_textured_material("M_Rasel_ClothB", "T_Rasel_ClothB_C", "T_Rasel_ClothB_N",
                           None, tiling=1.5, rough_hi=0.85)
    make_textured_material("M_Rasel_Paper", "T_Rasel_Paper_C", None, None,
                           tiling=1.0, rough_hi=0.92)
    # 결 없는 것 — 단색으로 충분한 재료
    make_flat_material("M_Rasel_Iron", (0.055, 0.055, 0.062), rough=0.42, metal=0.9)
    make_flat_material("M_Rasel_Dark", (0.018, 0.018, 0.021), rough=0.75)
    # 유리 — 거의 매끈해 등불·하늘을 되비친다(검은 판때기 아님). 살짝 청색.
    make_flat_material("M_Rasel_Glass", (0.016, 0.022, 0.030), rough=0.04, spec=1.0)
    make_flat_material("M_Rasel_Lantern", (0.9, 0.62, 0.3), rough=0.3,
                       emissive=(9.0, 5.4, 2.1))
    make_flat_material("M_Rasel_WinLight", (0.85, 0.66, 0.4), rough=0.4,
                       emissive=(3.4, 2.2, 1.0))
    make_flat_material("M_Rasel_Stone", (0.22, 0.215, 0.2), rough=0.85)
    # 물웅덩이 — 거의 거울처럼 매끄러워 등불·하늘을 되비친다(젖은 골목·저녁 거리)
    make_flat_material("M_Rasel_Puddle", (0.020, 0.028, 0.032), rough=0.04, spec=1.0)
    # 젖은 돌 갓 — 웅덩이 둘레의 짙게 젖은 자리
    make_flat_material("M_Rasel_WetStone", (0.10, 0.10, 0.095), rough=0.35, spec=0.7)
    # 탄 자국·소각재 — L02 골동상 사건 뒤의 뒷방(검은 무광)
    make_flat_material("M_Rasel_Ash", (0.030, 0.026, 0.024), rough=0.97)
    # 감정대 깔개 — 짙은 초록 융
    make_flat_material("M_Rasel_Felt", (0.055, 0.085, 0.062), rough=0.94)
    # 진의 새김 — 바닥에 그은 선이 스스로 빛난다(청백). L03 뒷골목의 주광원.
    make_flat_material("M_Rasel_Rune", (0.30, 0.55, 0.72), rough=0.45,
                       emissive=(1.6, 4.4, 6.2))
    # 급히 그어 이가 덜 맞물린 자리 — 같은 선인데 흐리다
    make_flat_material("M_Rasel_RuneFaint", (0.22, 0.34, 0.42), rough=0.6,
                       emissive=(0.30, 0.80, 1.10))


if __name__ == "__main__":
    try:
        import_textures()
        build_materials()
        log("=== 아트 반입 완료 ===")
    except Exception:
        log("실패\n" + traceback.format_exc())
    flush()
