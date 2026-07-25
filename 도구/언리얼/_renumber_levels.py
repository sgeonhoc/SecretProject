"""
라셀 레벨 재번호 — 맵 이름을 도면(_rasel_plan.json) 번호에 맞춘다.

  UnrealEditor-Cmd.exe C:/Secret_Project/Secret_Project.uproject ^
    -ExecutePythonScript="C:/Secret_Project/도구/언리얼/_renumber_levels.py" -nullrhi -unattended -nosplash

★2026-07-25 — 왜 rename 이 아니라 복제+삭제인가
  처음엔 `EditorAssetLibrary.rename_asset` / `AssetTools.rename_assets` 를 썼는데 **11건 전부 조용히
  취소**됐다(로그엔 "완료 0/11"만). 엔진 소스를 뒤져 보니 `AssetRenameManager.cpp:463` 이
      FMessageDialog::Open(EAppMsgType::OkCancel, EAppReturnType::Cancel, "... Continue with rename?")
  를 띄우고, **기본값이 Cancel** 이라 `-unattended` 에서는 무조건 취소된다. 설정으로 못 끈다.
  (뜨는 이유: 우리 맵은 CDO 소프트 참조 대상이다 — PortalActor.TargetLevelName 이 문자열이고
   C++ 에도 맵 경로가 하드코딩돼 있다.)

  → `duplicate_asset` + `delete_asset` 은 그 대화상자를 안 탄다.
  → 리다이렉터가 안 남지만 **어차피 우리 참조는 전부 문자열**이라 리다이렉터로는 안 고쳐진다.
     문자열은 아래 「남은 일」대로 손으로 고친다.
  → 라셀 맵은 OFPA(외부 액터)를 쓰지 않는 것을 확인하고 복제를 택했다
     (Content/__ExternalActors__/Maps 없음).

★임시 이름 2단계가 필요 없다
  02_모순대장과_재번호.md §3 은 "L10_Clinic → L08_Clinic 을 먼저 하면 L08_Academy_Street 와 충돌"
  이라 적었으나, 에셋 이름은 번호가 아니라 **전체 문자열**이라 L08_Clinic ≠ L08_Academy_Street 다.
  대상 11개를 기존 25개와 대조해 겹치는 이름이 0인 것을 확인하고 한 번에 간다.

근거 문서: 기획/02_게임설계/2_레벨/좌표/02_모순대장과_재번호.md §2
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


def move(src, dst):
    sp, dp = "%s/%s" % (DIR, src), "%s/%s" % (DIR, dst)
    if not EAL.does_asset_exist(sp):
        log("  - 건너뜀 (원본 없음): %s" % src)
        return False
    if EAL.does_asset_exist(dp):
        log("  X 충돌 — 이미 있음: %s" % dst)
        return False
    if EAL.duplicate_asset(sp, dp) is None:
        log("  X 복제 실패: %s" % src)
        return False
    if not EAL.does_asset_exist(dp):
        log("  X 복제본 없음: %s" % dst)
        return False
    if not EAL.delete_asset(sp):
        log("  ! 복제는 됐으나 원본 삭제 실패 — 손으로 지울 것: %s" % src)
        return False
    log("  O %s -> %s" % (src, dst))
    return True


def main():
    log("=== 라셀 레벨 재번호 (복제+삭제) ===")

    # 대상 이름이 기존과 겹치는지 먼저 전부 본다 — 하나라도 겹치면 아무것도 하지 않는다
    clash = [d for _, d in RENAMES if EAL.does_asset_exist("%s/%s" % (DIR, d))]
    if clash:
        log("중단 — 대상 이름이 이미 존재: %s" % ", ".join(clash))
        return

    done = 0
    for src, dst in RENAMES:
        if move(src, dst):
            done += 1

    log("\n완료 %d / %d" % (done, len(RENAMES)))
    log("남은 일: (1) C++ 경로 수정(GameFlowSubsystem.cpp) (2) 빌더 경로 수정 "
        "(3) _scan_links.py 재실행 (4) node 도구/진단/_verify_coords.js")

    EAL.save_directory(DIR, only_if_is_dirty=True, recursive=True)

    with open("C:/Secret_Project/Saved/renumber.log", "w", encoding="utf-8") as f:
        f.write("\n".join(LOG))


main()
