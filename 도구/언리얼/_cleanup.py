import unreal
EAL = unreal.EditorAssetLibrary
for j in ("_next", "_scratch", "_park"):
    p = "/Game/Maps/Rasel/L01_Jangteo_Street" + j if j == "_next" else "/Game/Maps/Rasel/" + j
    if EAL.does_asset_exist(p):
        print("삭제 %s -> %s" % (p, EAL.delete_asset(p)))
    else:
        print("없음 %s" % p)
