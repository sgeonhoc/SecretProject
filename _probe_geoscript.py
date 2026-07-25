# 탐침 3 — 키트 저작에 쓸 함수들의 정확한 시그니처/옵션 구조체 확인.
import unreal

LOG = "C:/Secret_Project/Saved/probe_geo.log"
lines = []


def log(m):
    lines.append(str(m))


def doc(lib, fn):
    try:
        d = getattr(getattr(unreal, lib), fn).__doc__
        log("### %s.%s\n%s" % (lib, fn, (d or "").strip()[:900]))
    except Exception as e:
        log("### %s.%s -> 없음 (%s)" % (lib, fn, e))
    log("")


def props(cls):
    try:
        c = getattr(unreal, cls)
        o = c()
        names = [n for n in dir(o) if not n.startswith("_") and not callable(getattr(o, n, None))]
        log("@@@ %s: %s" % (cls, ", ".join(names)))
    except Exception as e:
        log("@@@ %s -> %s" % (cls, e))
    log("")


doc("GeometryScript_MeshModeling", "apply_mesh_polygroup_bevel")
doc("GeometryScript_MeshModeling", "apply_mesh_bevel_selection")
doc("GeometryScript_UVs", "set_mesh_u_vs_from_box_projection")
doc("GeometryScript_UVs", "set_mesh_u_vs_from_planar_projection")
doc("GeometryScript_Normals", "recompute_normals")
doc("GeometryScript_Normals", "compute_tangents")
doc("GeometryScript_NewAssetUtils", "create_new_static_mesh_asset_from_mesh")
doc("GeometryScript_Materials", "set_material_id_on_triangles")
doc("GeometryScript_MeshSelection", "select_mesh_element_s_by_material_id")
doc("GeometryScript_Primitives", "append_box")
doc("GeometryScript_Primitives", "append_cylinder")
doc("GeometryScript_Primitives", "append_revolve_polygon")
doc("GeometryScript_Primitives", "append_linear_stairs")
doc("GeometryScript_Collision", "set_static_mesh_collision_from_mesh")
doc("GeometryScript_MeshTransforms", "translate_mesh")
doc("GeometryScript_MeshBooleans", "apply_mesh_boolean")

props("GeometryScriptPrimitiveOptions")
props("GeometryScriptMeshBevelOptions")
props("GeometryScriptCalculateNormalsOptions")
props("GeometryScriptCreateNewStaticMeshAssetOptions")
props("GeometryScriptMeshBooleanOptions")
props("GeometryScriptTangentsOptions")

sel = [n for n in dir(unreal.GeometryScript_MeshSelection) if not n.startswith("_")]
log("MeshSelection: " + ", ".join(sel))
tr = [n for n in dir(unreal.GeometryScript_MeshTransforms) if not n.startswith("_")]
log("MeshTransforms: " + ", ".join(tr))

with open(LOG, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
