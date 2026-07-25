# -*- coding: utf-8 -*-
# ▶ 환경 툰 외곽선 포스트프로세스 머티리얼 (M_PP_ToonOutline)
#   depth+normal 엣지 검출로 물체 윤곽에 어두운 선을 긋는다 → 셀셰이드 캐릭터와 배경 톤 통일.
#   Custom HLSL 노드로 SceneTextureLookup(깊이=1, 월드노멀=8)을 이웃 UV에서 샘플.
#   실행: UnrealEditor-Cmd.exe <proj> -ExecutePythonScript=_toon_pp.py
import unreal, traceback

MATDIR = "/Game/Rasel/Materials"
NAME = "M_PP_ToonOutline"
LOG = "C:/Secret_Project/Saved/toon.log"
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
_lines = []


def log(s):
    _lines.append(str(s)); unreal.log("[toon] " + str(s))


HLSL = r"""
float2 ts = View.ViewSizeAndInvSize.zw * Width;
float d0 = sd.r;
float dl = SceneTextureLookup(uv + float2(-ts.x, 0), 1, false).r;
float dr = SceneTextureLookup(uv + float2( ts.x, 0), 1, false).r;
float du = SceneTextureLookup(uv + float2(0, -ts.y), 1, false).r;
float dd = SceneTextureLookup(uv + float2(0,  ts.y), 1, false).r;
float de = (abs(d0 - dl) + abs(d0 - dr) + abs(d0 - du) + abs(d0 - dd)) / max(d0, 1.0);
float3 n0 = wn.rgb;
float3 nl = SceneTextureLookup(uv + float2(-ts.x, 0), 8, false).rgb;
float3 nr = SceneTextureLookup(uv + float2( ts.x, 0), 8, false).rgb;
float3 nu = SceneTextureLookup(uv + float2(0, -ts.y), 8, false).rgb;
float3 nd = SceneTextureLookup(uv + float2(0,  ts.y), 8, false).rgb;
float ne = distance(n0, nl) + distance(n0, nr) + distance(n0, nu) + distance(n0, nd);
float edge = saturate(de * 7.0 + ne * 1.0);
edge = edge > 0.33 ? 1.0 : 0.0;
// 화면 테두리에서 SceneDepth 가 튀어 생기는 가짜 외곽선 제거
float b = 0.012;
float border = (uv.x > b && uv.x < 1.0 - b && uv.y > b && uv.y < 1.0 - b) ? 1.0 : 0.0;
return edge * border;
"""


def build():
    p = "%s/%s" % (MATDIR, NAME)
    if EAL.does_asset_exist(p):
        EAL.delete_asset(p)
    mat = AT.create_asset(NAME, MATDIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)

    # 씬 텍스처 노드들 — PostProcessInput0(장면색+UV), SceneDepth, WorldNormal
    def scenetex(x, y, tid):
        n = MEL.create_material_expression(mat, unreal.MaterialExpressionSceneTexture, x, y)
        n.set_editor_property("scene_texture_id", tid)
        return n

    ppi = scenetex(-900, -200, unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    sdt = scenetex(-900, 120, unreal.SceneTextureId.PPI_SCENE_DEPTH)
    wnt = scenetex(-900, 380, unreal.SceneTextureId.PPI_WORLD_NORMAL)

    # 폭 상수
    width = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -900, 620)
    width.set_editor_property("r", 1.5)

    # Custom HLSL — 입력 uv, sd, wn, Width
    cust = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -520, 100)
    cust.set_editor_property("code", HLSL)
    try:
        cust.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    except Exception as e:
        log("output_type set 실패 %s" % e)

    def mk_input(name):
        ci = unreal.CustomInput()
        try:
            ci.set_editor_property("input_name", name)
        except Exception:
            ci.set_editor_property("InputName", name)
        return ci

    cust.set_editor_property("inputs", [mk_input("uv"), mk_input("sd"),
                                        mk_input("wn"), mk_input("Width")])
    MEL.connect_material_expressions(ppi, "UVs", cust, "uv")
    MEL.connect_material_expressions(sdt, "Color", cust, "sd")
    MEL.connect_material_expressions(wnt, "Color", cust, "wn")
    MEL.connect_material_expressions(width, "", cust, "Width")

    # 선 색(짙은 남색 — 쿨 톤에 녹게) 과 장면색을 엣지로 lerp
    line = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -520, 420)
    line.set_editor_property("constant", unreal.LinearColor(0.02, 0.025, 0.04, 1.0))
    lerp = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -240, 100)
    MEL.connect_material_expressions(ppi, "Color", lerp, "A")
    MEL.connect_material_expressions(line, "", lerp, "B")
    MEL.connect_material_expressions(cust, "", lerp, "Alpha")
    MEL.connect_material_property(lerp, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    try:
        MEL.recompile_material(mat)
        log("recompile OK")
    except Exception as e:
        log("recompile 실패 %s" % e)
    EAL.save_asset(p)
    log("저장 %s" % p)


open(LOG, "w", encoding="utf-8").close()
try:
    build()
    log("=== 툰 PP 완료 ===")
except Exception:
    log("실패\n" + traceback.format_exc())
with open(LOG, "w", encoding="utf-8") as f:
    f.write("\n".join(_lines))
