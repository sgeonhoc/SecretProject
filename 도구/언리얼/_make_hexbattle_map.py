# -*- coding: utf-8 -*-
# ▶ HexBattle 전투 레벨 맵 생성기 (에디터에서 1회 실행)
#   실행법: 에디터 상단 메뉴 Tools → Execute Python Script → 이 파일 선택
#          또는 Output Log 하단 입력창을 'Cmd'→'Python'으로 바꾸고:  py "C:/Secret_Project/_make_hexbattle_map.py"
#
#   하는 일: 레벨마다 빈 레벨(/Game/Maps/HexBattle, HexBattle_01 … HexBattle_05)을 만들고
#            World Settings의 GameMode를 HexBattleGameMode로 지정 후 저장.
#   레벨 결정: GameMode가 맵 이름 접미사로 레벨을 고른다 (HexBattle=L1(0), HexBattle_0N=L(N+1)).
#   그 뒤: 콘텐츠 브라우저에서 원하는 맵 더블클릭 → ▶Play 하면 그 레벨의 판·지형·상대·카드가 뜬다.
#   (전투 내용은 .umap이 아니라 게임모드/그리드 코드가 Play 때 절차 생성한다 — 수동 배치 없음.)
#   정본: C:/Secret_Project/기획/02_게임설계/1_전투/시스템/게임_전투레벨_정본.md
import unreal

GM_CLASS_PATH = "/Script/Secret_Project.HexBattleGameMode"

# (맵 경로, 사람이 읽는 레벨 이름) — 접미사 숫자 = LevelIndex
MAPS = [
    ("/Game/Maps/HexBattle",     "L1 폐선 지하수로"),
    ("/Game/Maps/HexBattle_01",  "L2 아랫장터 뒷골목"),
    ("/Game/Maps/HexBattle_02",  "L3 신항 굴착 갱"),
    ("/Game/Maps/HexBattle_03",  "L4 학당가 강둑"),
    ("/Game/Maps/HexBattle_04",  "L5 유리탑 로비"),
    ("/Game/Maps/HexBattle_05",  "L6 옛 도읍 폐허"),
    ("/Game/Maps/HexBattle_06",  "L7 화물창 지붕"),
    ("/Game/Maps/HexBattle_07",  "L8 아래층 회랑"),
    ("/Game/Maps/HexBattle_08",  "L9 침수된 승강장"),
    ("/Game/Maps/HexBattle_09",  "L10 무너진 성문 앞뜰"),
]

def log(msg):
    unreal.log("[HexBattle] " + msg)

def get_world():
    try:
        ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        w = ues.get_editor_world()
        if w:
            return w
    except Exception:
        pass
    return unreal.EditorLevelLibrary.get_editor_world()

def main():
    gm = unreal.load_class(None, GM_CLASS_PATH)
    if gm is None:
        unreal.log_error("[HexBattle] HexBattleGameMode 클래스를 못 찾음 — C++ 빌드가 됐는지 확인(에디터 재시작 필요할 수 있음).")
        return

    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    for map_path, nice in MAPS:
        if not les.new_level(map_path):
            les.load_level(map_path)  # 이미 있으면 로드
        world = get_world()
        ws = world.get_world_settings()
        ws.set_editor_property("default_game_mode", gm)
        les.save_current_level()
        log("레벨 생성/갱신: %s  (%s)" % (map_path, nice))

    log("완료! 콘텐츠 브라우저 Maps 폴더에서 원하는 레벨 더블클릭 → Play.")
    log("한 맵에서 다른 레벨을 보려면 World Settings > GameMode의 LevelOverride(0~5)를 바꿔도 된다.")

main()
