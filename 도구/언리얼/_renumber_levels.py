"""
라셀 레벨 재번호 — 맵 이름을 도면(_rasel_plan.json) 번호에 맞춘다.

  UnrealEditor-Cmd.exe C:/Secret_Project/Secret_Project.uproject ^
    -ExecutePythonScript="C:/Secret_Project/도구/언리얼/_renumber_levels.py" -unattended -nosplash

★왜 파이썬(UE)이어야 하나
  탐색기에서 .umap 파일 이름을 바꾸면 다른 맵의 문(PortalActor)이 가리키던 경로가 끊긴다.
  UE의 rename_asset은 리다이렉터를 남겨 참조가 따라온다.

★왜 두 걸음인가
  L10_Clinic → L08_Clinic 을 먼저 하면 아직 L08_Academy_Street 가 그 이름을 쓰고 있어 충돌한다.
  전부 임시 이름으로 옮긴 뒤 최종 이름으로 내린다.

근거 문서: 기획/02_게임설계/2_레벨/좌표/02_모순대장과_재번호.md §2·§3
"""
import unreal

EAL = unreal.EditorAssetLibrary
DIR = "/Game/Maps/Rasel"

# (지금 이름, 바꿀 이름) — 도면 번호가 정본
RENAMES = [
    ("L08_Academy_Street",     "L06_Academy_Street"),      # 학당가 거리
    ("L11_Riverbank",          "L07_Riverbank"),           # 학당 뒤 강둑
    ("L10_Clinic",             "L08_Clinic"),              # 의원 병동
    ("L13_Newport_Site",       "L09_Newport_Site"),        # 신항 공사판
    ("L14_Newport_Shaft",      "L10_Newport_Shaft"),       # 굴착 갱
    ("L17_Sanatorium",         "L13_Seir_Sanatorium"),     # 세이르 재단
    ("L12_Selan_Court",        "L14_Selan_Court"),         # 셀란 수련원
    ("L19_Abandoned_Platform", "L16_Abandoned_Platform"),  # 폐선 승강장
    ("L21_Underlayer_Gallery", "L17_Underlayer_Gallery"),  # 아래층 묻힌 도시
    ("L22_Ruin_Gate",          "L18_Ruin_Gate"),           # 무너진 성문 앞뜰
    ("L23_Ruin_Keep",          "L19_Ruin_Keep"),           # 빈 성 안
]

LOG = []


def log(s):
    LOG.append(s)
    unreal.log(s)


def rename(src, dst):
    sp, dp = "%s/%s" % (DIR, src), "%s/%s" % (DIR, dst)
    if not EAL.does_asset_exist(sp):
        log("  건너뜀 (없음): %s" % src)
        return False
    if EAL.does_asset_exist(dp):
        log("  ✗ 충돌 — 이미 있음: %s" % dst)
        return False
    ok = EAL.rename_asset(sp, dp)
    log("  %s %s → %s" % ("✔" if ok else "✗", src, dst))
    return ok


def main():
    log("=== 라셀 레벨 재번호 ===")

    log("\n[1단계] 임시 이름으로")
    moved = []
    for src, dst in RENAMES:
        tmp = "_TMP_" + dst
        if rename(src, tmp):
            moved.append((tmp, dst))

    log("\n[2단계] 최종 이름으로")
    done = 0
    for tmp, dst in moved:
        if rename(tmp, dst):
            done += 1

    log("\n완료 %d / %d" % (done, len(RENAMES)))
    log("남은 일: ①C++ 경로 수정(GameFlowSubsystem.cpp) ②빌더 경로 수정 "
        "③_scan_links.py 재실행 ④node 도구/진단/_verify_coords.js")

    EAL.save_directory(DIR, only_if_is_dirty=False, recursive=True)

    with open("C:/Secret_Project/Saved/renumber.log", "w", encoding="utf-8") as f:
        f.write("\n".join(LOG))


main()
