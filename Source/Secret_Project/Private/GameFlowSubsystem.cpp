#include "GameFlowSubsystem.h"
#include "BattleReturnWatcher.h"
#include <initializer_list>
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UGameFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    BuildDefaultStages();

    // 전투 레벨이 열릴 때 결과를 지켜볼 배우를 세우기 위해 세계가 서는 순간을 듣는다.
    // (전투 판은 전투 담당의 게임모드가 돌린다 — 그 파일을 안 건드리고 결과만 받아 오는 길.)
    FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &UGameFlowSubsystem::HandleWorldActorsInitialized);

    // 저장된 진행이 있으면 메모리로 올려둔다(타이틀의 이어하기 버튼이 바로 판단할 수 있게).
    if (UGameplayStatics::DoesSaveGameExist(FlowSlotName(), 0))
    {
        if (UGameFlowSave* S = Cast<UGameFlowSave>(UGameplayStatics::LoadGameFromSlot(FlowSlotName(), 0)))
        {
            CurrentStageId   = S->CurrentStageId;
            CurrentLevelPath = S->CurrentLevelPath;
            Chapter          = S->Chapter;
            PlaySeconds      = S->PlaySeconds;
        }
    }
}

/**
 * 진행표 = ACT 0 (`게임_유저플로우_시나리오_정본_20260724.md` §2·§13).
 *
 * 방세에서 시작해 첫 불, 그리고 다리 아래에서 저 아닌 사람을 처음 눈으로 보는 데까지.
 * 큰 진실은 하나도 안 나온다.
 *
 * 규율 셋:
 *  - **지금 있는 맵만 가리킨다.** 없는 맵을 가리키면 PLAY가 검은 화면으로 끝난다.
 *  - **자리 좌표는 맵에서 읽은 실제 값이다**(`_dump_stage_spots.py` → Saved/stage_spots.json).
 *    레벨이 바뀌면 그 스크립트를 다시 돌려 여기 값을 맞춘다.
 *  - **StageId는 세이브에 박힌다** — 기존 칸의 Id는 바꾸지 말 것(칸 추가는 자유).
 *
 * 아직 안 지어진 자리(부두 창고 L12·딘의 자리 L21)는 이 표에서 빠져 있다.
 * 그 맵이 서면 0-1의 삯일과 0-4의 첫 실전 전투가 칸으로 들어온다.
 */
void UGameFlowSubsystem::BuildDefaultStages()
{
    // 대사 한 줄 — 화자가 비면 지문(누가 말하는 게 아니라 보이는 것).
    auto L = [](const TCHAR* Speaker, const TCHAR* Text)
    {
        FStoryLine Line;
        Line.Speaker = Speaker;
        Line.Line = Text;
        return Line;
    };

    auto Add = [this](FName Id, int32 Ch, const TCHAR* Name, const TCHAR* Level, FName Entry) -> FGameStage&
    {
        FGameStage S;
        S.StageId     = Id;
        S.Chapter     = Ch;
        S.DisplayName = Name;
        S.LevelPath   = FName(Level);
        S.EntryTag    = Entry;
        Stages.Add(S);
        return Stages.Last();
    };

    const TCHAR* L_ROOM  = TEXT("/Game/Maps/Rasel/L22_Yoa_Room");
    const TCHAR* L_STREET= TEXT("/Game/Maps/Rasel/L01_Jangteo_Street");
    const TCHAR* L_EAT   = TEXT("/Game/Maps/Rasel/L05_Eatery");
    const TCHAR* L_RIVER = TEXT("/Game/Maps/Rasel/L11_Riverbank");

    Stages.Reset();

    // ── 0-1. 방세 (셋집 삼층, 아침) ─────────────────────────────
    {
        FGameStage& S = Add(TEXT("A0_Room"), 1, TEXT("셋집 삼층"), L_ROOM, TEXT("Start"));
        S.Kind = EStageKind::Episode;   // 인트로 컷신
        S.bOverrideSpawn = true;
        S.SpawnLocation  = FVector(-260.f, 0.f, 0.f);   // 맵의 PlayerStart 자리(바닥은 게임모드가 맞춘다)
        S.SpawnYaw       = 0.f;
        S.TransitionCard = FText::FromString(TEXT("라셀 · 아랫장터\n셋집 삼층, 아침"));
        S.ArrivalScene = {
            L(TEXT(""),          TEXT("문을 두드리는 소리가 세 번. 두드리는 사이가 짧다.")),
            L(TEXT("셋집 주인"), TEXT("요아. 넉 달치다.")),
            L(TEXT("셋집 주인"), TEXT("이레 안에 못 내면 자물쇠를 바꾼다. 내 손해도 여기까지야.")),
            L(TEXT("요아"),      TEXT("…오늘 창고 삯일 나갑니다.")),
            L(TEXT("셋집 주인"), TEXT("삯일 값은 나도 안다. 넉 달치는 안 나와.")),
            L(TEXT(""),          TEXT("발소리가 계단을 내려간다. 어젯밤 받아 둔 물그릇에 손을 담근다 — 물이 미지근하다.")),
        };
        S.Goal      = EStageGoal::EnterLevel;
        S.GoalLevel = TEXT("L01_Jangteo_Street");
        S.ObjectiveLabel = FText::FromString(TEXT("문을 열고 장터 큰길로 나가라"));
    }

    // ── 0-1(뒤). 장터의 소문 → 벽보판 ───────────────────────────
    {
        FGameStage& S = Add(TEXT("A0_Street"), 1, TEXT("아랫장터 큰길"), L_STREET, TEXT("FromRoom"));
        // ★허브 — 진입해도 대사를 강제하지 않는다. 좌판 사이를 돌고, 서 있는 사람에게 다가가
        //   말 걸어(E) 소문을 듣는다. 소문을 들었든 아니든 벽보판에 서면 이야기가 한 칸 진행된다.
        S.Kind = EStageKind::Hub;
        S.bOverrideSpawn = true;
        S.SpawnLocation  = FVector(500.f, 0.f, 0.f);
        S.SpawnYaw       = 0.f;
        S.TransitionCard = FText::FromString(TEXT("아랫장터 큰길\n낮"));
        // 좌판 곁에 선 두 사람 — 다가가 말 걸면 각자 같은 사건을 다른 각도로 흘린다(강제 아님).
        {
            FStageNpc N; N.Name = TEXT("골동 중개"); N.Location = FVector(1000.f, 300.f, 0.f); N.Yaw = 200.f;
            N.Lines = {
                TEXT("신항에서 뭐가 나왔다지. 파낸 게 삭지도 않았대."),
                TEXT("삭지 않은 게 더 무서운 거야. 땅속에서 몇 백 년을 버틴 물건이라는 뜻이니까."),
            };
            S.SceneNpcs.Add(N);
        }
        {
            FStageNpc N; N.Name = TEXT("뱃말 뜨내기"); N.Location = FVector(1500.f, -260.f, 0.f); N.Yaw = 150.f;
            N.Lines = {
                TEXT("옛 글자 그림 한 장에 두 달치를 부른 사람이 있어. 한 장에."),
                TEXT("사겠다는 게 아니야. 그런 걸 왜 그렇게 급히 걷어가나 싶어서 그렇지."),
            };
            S.SceneNpcs.Add(N);
        }
        S.bHasObjective     = true;
        S.Goal              = EStageGoal::ReachSpot;
        S.ObjectiveLocation = FVector(2000.f, 470.f, 150.f);   // 맵의 벽보판
        // 벽보판은 길가 벽에 붙어 있고 사람은 길 한가운데로 걷는다 — 반경이 좁으면 옆을 스쳐 지나간다.
        S.ObjectiveRadius   = 520.f;
        S.ObjectiveLabel    = FText::FromString(TEXT("벽보판 앞에 서라"));
        S.ObjectiveScene = {
            L(TEXT(""),     TEXT("부고 두 장이 겹쳐 붙어 있다. 이름은 다르고 날짜는 사흘 차이다.")),
            L(TEXT(""),     TEXT("그 아래, 누가 분필로 적어 두었다 — \"손이 뜨거운 사람. 잿마당.\"")),
            L(TEXT("요아"), TEXT("…잿마당.")),
        };
    }

    // ── 0-2. 낱장과 첫 불 (옥상, 밤) ────────────────────────────
    {
        FGameStage& S = Add(TEXT("A0_Roof"), 1, TEXT("셋집 옥상"), L_ROOM, TEXT("Start"));
        S.Kind = EStageKind::Episode;   // 첫 불 — 컷신
        S.bOverrideSpawn = true;
        S.SpawnLocation  = FVector(-260.f, 0.f, 0.f);
        S.SpawnYaw       = 0.f;
        S.TransitionCard = FText::FromString(TEXT("그날 밤\n셋집 옥상"));
        S.ArrivalScene = {
            L(TEXT(""), TEXT("잿마당에 올라온 그림을 종이에 옮겨 적었다. 접힌 자국이 손바닥에서 축축하다.")),
            L(TEXT(""), TEXT("글 아래에 한 줄이 붙어 있었다 — \"소리 내어 읽으면, 뜨거운 게 불이 된다.\"")),
        };
        S.bHasObjective     = true;
        S.Goal              = EStageGoal::ReachSpot;
        S.ObjectiveLocation = FVector(300.f, 190.f, 90.f);     // 맵의 '옥상 끝 (첫 발현)'
        S.ObjectiveRadius   = 320.f;
        S.ObjectiveLabel    = FText::FromString(TEXT("옥상 끝에서 낱장을 읽어라"));
        S.ObjectiveScene = {
            L(TEXT(""),     TEXT("종이를 폈다. 글자가 세 뭉치다. 첫 뭉치를 소리 내어 읽는다.")),
            L(TEXT("요아"), TEXT("에 자르.")),
            L(TEXT(""),     TEXT("아무 일도 없다. 손끝만 아까보다 뜨겁다. 다음 뭉치.")),
            L(TEXT("요아"), TEXT("자르타.")),
            L(TEXT(""),     TEXT("손바닥 위에 불씨가 붙는다. 붙은 자리가 한 뼘까지 서더니 그대로 선 채 흔들린다.")),
            L(TEXT(""),     TEXT("손이 아프지 않다. 며칠째 앓던 것이 지금은 손바닥 위에 얹혀 있다.")),
            L(TEXT(""),     TEXT("불을 든 채로 물통 위 빨래 장대를 겨눠 본다. 겨누자 손이 무거워진다 — 겨눌 데를 정해야 서는 것이다.")),
        };
    }

    // ── 0-2(뒤). 첫 판 — 옥상의 허깨비 과녁 (튜토리얼 전투) ─────
    // 시나리오 정본 §2 0-2: 실제 술사가 아니라 **가장 낮은 난이도의 연습 상대**.
    // 판은 전투 담당의 것이라 규칙을 안 건드리고, 판이 끝나면 다시 이야기로 돌아온다.
    {
        FGameStage& S = Add(TEXT("A0_FirstBattle"), 1, TEXT("허깨비 과녁"), TEXT("/Game/Maps/HexBattle"), TEXT("Start"));
        S.Goal      = EStageGoal::Battle;
        S.GoalLevel = TEXT("HexBattle");
        S.ObjectiveLabel = FText::FromString(TEXT("불을 세워 과녁을 무너뜨려라"));
        // 져도 막히지 않는다 — 연습이라 다음 날이 그대로 온다(패배=대가, 정본 §1).
    }

    // ── 0-3. 소문 두 겹 (밥집, 사흘 뒤) ─────────────────────────
    {
        FGameStage& S = Add(TEXT("A0_Eatery"), 1, TEXT("조용한 밥집"), L_EAT, TEXT("FromStreet"));
        S.Kind = EStageKind::Episode;   // 구석 자리에서 엿듣는 컷신
        S.bOverrideSpawn = true;
        S.SpawnLocation  = FVector(-200.f, -110.f, 0.f);
        S.SpawnYaw       = 0.f;
        S.TransitionCard = FText::FromString(TEXT("사흘 뒤\n조용한 밥집"));
        S.ArrivalScene = {
            L(TEXT("밥집 주인 내외"), TEXT("골동상 봤어? 어젯밤에 탔대. 배선이 오래됐다더라고.")),
            L(TEXT("저녁 단골"),      TEXT("주인은 안에 있었대. 관에서는 사고사로 접수했다던데.")),
            L(TEXT("저녁 단골"),      TEXT("그 그림 만졌던 사람들 말이야. 하나는 앓다가, 하나는 굴러서, 하나는 자다가.")),
        };
        S.bHasObjective     = true;
        S.Goal              = EStageGoal::ReachSpot;
        S.ObjectiveLocation = FVector(305.f, -105.f, 90.f);     // 맵의 '구석 자리'
        S.ObjectiveRadius   = 300.f;
        S.ObjectiveLabel    = FText::FromString(TEXT("구석 자리에 앉아 들어라"));
        S.ObjectiveScene = {
            L(TEXT(""),     TEXT("숟가락을 든 채로 셈이 맞아 버린다. 죽은 셋은 그 그림을 만졌다.")),
            L(TEXT(""),     TEXT("나는 그 그림을 읽었다. 읽자 손바닥에 불이 섰다.")),
            L(TEXT("요아"), TEXT("…")),
        };
    }

    // ── 0-3(뒤). 강둑 — 저 아닌 사람 (넷째 날 밤) ───────────────
    {
        FGameStage& S = Add(TEXT("A0_River"), 1, TEXT("다리 아래 강둑"), L_RIVER, TEXT("Start"));
        S.Kind = EStageKind::Episode;   // 딘과의 만남 — 컷신
        S.bOverrideSpawn = true;
        S.SpawnLocation  = FVector(-900.f, 0.f, 0.f);
        S.SpawnYaw       = 0.f;
        S.TransitionCard = FText::FromString(TEXT("넷째 날 밤\n다리 아래 강둑"));
        S.ArrivalScene = {
            L(TEXT(""), TEXT("잿마당에 \"뜨겁던 게 이제 불이 된다\"고 적었더니, 한 사람이 자리를 짚어 답했다.")),
            L(TEXT(""), TEXT("다리 아래는 물소리가 커서 말소리가 멀리 안 간다. 그래서 여기로 부른 모양이다.")),
        };
        // 딘 — 아직 이 사람의 몸(메시)은 정하지 않았다. 지금은 아무 몸이나 세워 자리만 잡는다.
        {
            FStageNpc N;
            N.Name = TEXT("딘");
            N.Location = FVector(420.f, -60.f, 60.f);
            N.Yaw = 180.f;
            S.SceneNpcs.Add(N);
        }
        S.bHasObjective     = true;
        S.Goal              = EStageGoal::ReachSpot;
        S.ObjectiveLocation = FVector(420.f, -60.f, 60.f);
        S.ObjectiveRadius   = 340.f;
        S.ObjectiveLabel    = FText::FromString(TEXT("다리 아래에서 기다리는 사람에게 가라"));
        S.ObjectiveScene = {
            L(TEXT("딘"),   TEXT("혼자 왔네. 잘했어.")),
            L(TEXT(""),     TEXT("딘이 손바닥을 위로 폈다. 짧게 한 마디 — \"마이타.\"")),
            L(TEXT(""),     TEXT("손바닥 가운데에 물이 고인다. 강물에서 퍼 온 것도 아닌데 손금을 따라 모인다.")),
            L(TEXT("딘"),   TEXT("이제 네 차례.")),
            L(TEXT("요아"), TEXT("자르타.")),
            L(TEXT(""),     TEXT("한 뼘 불이 선다. 물과 불이 각각 한 뼘씩, 사람 하나 사이를 두고 마주 선다.")),
            L(TEXT("딘"),   TEXT("잿마당은 오늘로 끊어. 글 올린 자리가 곧 주소고, 주소는 요양원으로 간다.")),
            L(TEXT("딘"),   TEXT("우리 자리로 가자. 규율은 하나야 — 뭉치지 않는다.")),
        };
    }

    // ── 0-5. 막 닫힘 ────────────────────────────────────────────
    {
        FGameStage& S = Add(TEXT("A0_Close"), 1, TEXT("첫 덱, 첫 오싹함"), L_RIVER, TEXT("Start"));
        S.Kind = EStageKind::Episode;   // 막 닫는 컷신
        S.bOverrideSpawn = true;
        S.SpawnLocation  = FVector(-900.f, 0.f, 0.f);
        S.Goal = EStageGoal::SceneOnly;
        S.TransitionCard = FText::FromString(TEXT("ACT 0 — 끝"));
        S.ArrivalScene = {
            L(TEXT(""), TEXT("남은 것 — 아직 못 낸 넉 달치, 손 안의 한 뼘 불, 그리고 이름을 아는 사람 하나.")),
            L(TEXT(""), TEXT("이 도시엔 내가 모르는 판이 있고, 나는 방금 거기에 발을 들였다.")),
            L(TEXT(""), TEXT("(여기까지가 지금 지어진 데까지다. 딘의 자리 — 첫 실전 전투 — 는 그 맵이 서면 이어진다.)")),
        };
    }
}

TArray<FTravelDistrict> UGameFlowSubsystem::GetTravelDistricts()
{
    auto D = [](const TCHAR* Name, const TCHAR* Level, const TCHAR* Note)
    {
        FTravelDistrict T;
        T.Name = Name;
        T.LevelPath = FName(Level);
        T.EntryTag = NAME_None;   // 아무 PlayerStart(구역마다 도착 태그가 서면 채운다)
        T.Note = Note;
        return T;
    };

    // 지어진 라셀 맵 중 자유로 오갈 수 있는 "동네" 구역들. 던전·스토리 전용 무대는 넣지 않는다.
    return {
        D(TEXT("아랫장터 큰길"),  TEXT("/Game/Maps/Rasel/L01_Jangteo_Street"), TEXT("좌판과 소문이 도는 저지대의 심장")),
        D(TEXT("네사의 골동상"),  TEXT("/Game/Maps/Rasel/L02_Antique_Shop"),  TEXT("옛것을 사고파는 뒷가게")),
        D(TEXT("장터 뒷골목"),    TEXT("/Game/Maps/Rasel/L03_Backalley"),     TEXT("좁고 어두운 지름길")),
        D(TEXT("조용한 밥집"),    TEXT("/Game/Maps/Rasel/L05_Eatery"),        TEXT("저녁이면 소문이 겹치는 자리")),
        D(TEXT("학당가 거리"),    TEXT("/Game/Maps/Rasel/L08_Academy_Street"),TEXT("서생과 학인이 오가는 윗동네")),
        D(TEXT("다리 아래 강둑"), TEXT("/Game/Maps/Rasel/L11_Riverbank"),     TEXT("물소리가 말소리를 지우는 곳")),
        D(TEXT("부두 하역장"),    TEXT("/Game/Maps/Rasel/L11_Dock_Wharf"),    TEXT("삯과 사람 수로 다투는 밤 부두")),
        D(TEXT("부두 창고"),      TEXT("/Game/Maps/Rasel/L12_Warehouse"),     TEXT("낮에 짐을 지는 일터")),
    };
}

TArray<FStageNpc> UGameFlowSubsystem::GetResidentNpcs(FName LevelPath)
{
    auto N = [](const TCHAR* Name, float X, float Y, float Yaw, std::initializer_list<const TCHAR*> Lines)
    {
        FStageNpc P;
        P.Name = Name;
        P.Location = FVector(X, Y, 0.f);   // z는 GameMode가 바닥을 훑어 맞춘다
        P.Yaw = Yaw;
        for (const TCHAR* L : Lines) P.Lines.Add(FString(L));
        return P;
    };

    const FString Lv = LevelPath.ToString();

    if (Lv.Contains(TEXT("L01_Jangteo")))
        return {
            N(TEXT("좌판 아낙"), 800.f, -320.f, 120.f, {
                TEXT("오늘 물건은 아침에 다 나갔어. 소금이 특히 빨리 빠지더라고."),
                TEXT("요샌 다들 뭐라도 쟁여 두려고 해. 불안하면 손이 그렇게 움직이지.") }),
            N(TEXT("짐꾼 노인"), 1300.f, 360.f, 200.f, {
                TEXT("신항 쪽 일이 늘었어. 파낸 걸 나르는 손이 모자란대."),
                TEXT("나이 든 나까지 부르는 걸 보면, 급하긴 급한 모양이지.") }),
            N(TEXT("떡집 딸"), 1750.f, -200.f, 150.f, {
                TEXT("벽보판 봤어? 부고가 자꾸 붙어. 요 며칠 새 셋이나."),
                TEXT("사고사라는데… 사고가 그렇게 줄지어 나나 싶어.") }),
        };

    if (Lv.Contains(TEXT("L02_Antique")))
        return {
            N(TEXT("골동상 조수"), 120.f, 220.f, 210.f, {
                TEXT("주인장은 안에 계셔. 요샌 옛 글자 새겨진 건 다 값이 뛰어."),
                TEXT("사가는 사람이 정해져 있는 것 같기도 하고. 늘 같은 심부름꾼이 와.") }),
            N(TEXT("단골 수집가"), -180.f, -160.f, 40.f, {
                TEXT("삭지 않은 물건을 찾고 있소. 땅에서 갓 나온 것일수록 좋고."),
                TEXT("왜냐고는 묻지 마시오. 나도 부탁받은 처지라.") }),
        };

    if (Lv.Contains(TEXT("L05_Eatery")))
        return {
            N(TEXT("밥집 주인"), -160.f, 120.f, 90.f, {
                TEXT("앉아. 저녁엔 국이 좋아. 소문도 여기가 제일 빨라."),
                TEXT("골동상 탄 거 들었지? 주인이 안에 있었다더라. 쯧.") }),
            N(TEXT("저녁 단골"), 220.f, -140.f, 160.f, {
                TEXT("손이 뜨겁다는 사람들 얘기 말이야. 잿마당에 그런 글이 돈대."),
                TEXT("나야 뭐, 손 시린 사람이라 상관없지만.") }),
        };

    if (Lv.Contains(TEXT("L08_Academy")))
        return {
            N(TEXT("늙은 서생"), 200.f, 260.f, 200.f, {
                TEXT("학당은 옛 글자를 안 가르쳐. 위에서 막았거든. 위험하다면서."),
                TEXT("허나 막을수록 궁금해지는 게 사람 아니겠나.") }),
            N(TEXT("학인"), -240.f, -120.f, 30.f, {
                TEXT("도서고 안쪽 서가는 잠겨 있어요. 열쇠는 관에서 쥐고 있고요."),
                TEXT("거기 뭐가 있길래 그렇게까지 잠가 뒀을까요.") }),
        };

    if (Lv.Contains(TEXT("L11_Riverbank")))
        return {
            N(TEXT("빨래하는 이"), -300.f, 180.f, 120.f, {
                TEXT("밤엔 여기 물소리가 커. 그래서 남 얘기 하기 딱 좋지."),
                TEXT("다리 밑에서 사람들이 모였다 흩어지는 걸 몇 번 봤어.") }),
            N(TEXT("낚시꾼"), 260.f, -220.f, 150.f, {
                TEXT("강이 요새 차. 예전 같지 않아. 물고기도 깊이만 다녀."),
                TEXT("검은물 지난 뒤로 물이 다 이래. 다들 그러려니 하지.") }),
        };

    if (Lv.Contains(TEXT("L11_Dock")))
        return {
            N(TEXT("하역 조장"), 600.f, 200.f, 200.f, {
                TEXT("삯은 사람 수로 나눠. 손이 많으면 각자 몫이 준다는 거, 다들 알면서도 와."),
                TEXT("여긴 위층이야. 여기선 아무도 그 이상한 재주 안 써. 쓰면 쫓겨나.") }),
            N(TEXT("선원"), 1200.f, -240.f, 150.f, {
                TEXT("저 배는 요양원 쪽으로 물건을 대. 뭘 싣는지는 안 물어."),
                TEXT("묻는 놈이 먼저 내리게 되거든.") }),
        };

    if (Lv.Contains(TEXT("L12_Warehouse")))
        return {
            N(TEXT("창고 감독"), 200.f, 160.f, 200.f, {
                TEXT("짐은 저 안쪽부터 채워. 무거운 건 아래, 가벼운 건 위."),
                TEXT("요아, 너 요새 몸이 가볍다며? 이상한 소리 마. 그냥 익숙해진 거야.") }),
            N(TEXT("인부"), -220.f, -120.f, 40.f, {
                TEXT("창고 앞에 낯선 사람이 서 있곤 해. 사는 것도 아니면서 며칠째."),
                TEXT("누굴 보는 건지, 뭘 보는 건지 모르겠어.") }),
        };

    return {};
}

int32 UGameFlowSubsystem::IndexOfStage(FName StageId) const
{
    return Stages.IndexOfByPredicate([StageId](const FGameStage& S) { return S.StageId == StageId; });
}

bool UGameFlowSubsystem::GetStage(FName StageId, FGameStage& OutStage) const
{
    const int32 Idx = IndexOfStage(StageId);
    if (Idx == INDEX_NONE) return false;
    OutStage = Stages[Idx];
    return true;
}

bool UGameFlowSubsystem::HasSavedRun() const
{
    return UGameplayStatics::DoesSaveGameExist(FlowSlotName(), 0);
}

void UGameFlowSubsystem::StartNewGame()
{
    if (Stages.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameFlow] 진행표가 비어 새 게임을 시작할 수 없다."));
        return;
    }

    // 새 게임은 진행을 처음으로 되돌린다. 스탯/스토리 세이브는 각 담당 시스템이 자기 슬롯에서 지운다.
    DeleteRun();

    const FGameStage& First = Stages[0];
    CurrentStageId  = First.StageId;
    Chapter         = First.Chapter;
    PlaySeconds     = 0.f;
    bOpeningPending = true;   // 도착한 레벨에서 오프닝을 튼다(이어하기는 안 튼다)

    UE_LOG(LogTemp, Log, TEXT("[GameFlow] 새 게임 → %s (%s)"), *First.DisplayName, *First.LevelPath.ToString());
    OnStageChanged.Broadcast(CurrentStageId);
    DoTravel(First.LevelPath, First.EntryTag);
}

void UGameFlowSubsystem::ContinueGame()
{
    UGameFlowSave* S = HasSavedRun()
        ? Cast<UGameFlowSave>(UGameplayStatics::LoadGameFromSlot(FlowSlotName(), 0))
        : nullptr;

    if (!S || !S->bHasRun || S->CurrentLevelPath.IsNone())
    {
        // 저장이 깨졌거나 비었으면 새 게임으로 — 버튼을 눌렀는데 아무 일도 안 일어나는 상황은 만들지 않는다.
        UE_LOG(LogTemp, Warning, TEXT("[GameFlow] 이어할 진행이 없어 새 게임으로 시작한다."));
        StartNewGame();
        return;
    }

    CurrentStageId   = S->CurrentStageId;
    CurrentLevelPath = S->CurrentLevelPath;
    Chapter          = S->Chapter;
    PlaySeconds      = S->PlaySeconds;

    UE_LOG(LogTemp, Log, TEXT("[GameFlow] 이어하기 → %s (스테이지 %s)"),
        *CurrentLevelPath.ToString(), *CurrentStageId.ToString());
    OnStageChanged.Broadcast(CurrentStageId);
    DoTravel(CurrentLevelPath, S->CurrentEntryTag);
}

void UGameFlowSubsystem::TravelToLevel(FName LevelPath, FName EntryTag)
{
    if (LevelPath.IsNone()) return;
    DoTravel(LevelPath, EntryTag);
}

void UGameFlowSubsystem::GoToStage(FName StageId)
{
    const int32 Idx = IndexOfStage(StageId);
    if (Idx == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameFlow] 없는 스테이지: %s"), *StageId.ToString());
        return;
    }

    const FGameStage& St = Stages[Idx];
    CurrentStageId = St.StageId;
    Chapter        = St.Chapter;

    // 전투 칸이면 "이 판을 부른 칸"을 적어 둔다 — 판이 끝나면 여기서부터 다음이 정해진다.
    BattleFromStageId = (St.Goal == EStageGoal::Battle) ? St.StageId : NAME_None;

    OnStageChanged.Broadcast(CurrentStageId);

    // 이미 그 무대에 서 있으면 화면을 다시 깜빡이지 않고 진행만 갱신한다.
    if (CurrentLevelPath == St.LevelPath)
    {
        SaveRun();
        return;
    }
    DoTravel(St.LevelPath, St.EntryTag);
}

bool UGameFlowSubsystem::AdvanceStage()
{
    const int32 Idx = IndexOfStage(CurrentStageId);
    if (Idx == INDEX_NONE || Idx + 1 >= Stages.Num()) return false;

    GoToStage(Stages[Idx + 1].StageId);
    return true;
}

bool UGameFlowSubsystem::HasNextStage() const
{
    const int32 Idx = IndexOfStage(CurrentStageId);
    return Idx != INDEX_NONE && Idx + 1 < Stages.Num();
}

FName UGameFlowSubsystem::GetNextStageLevel() const
{
    const int32 Idx = IndexOfStage(CurrentStageId);
    return (Idx != INDEX_NONE && Idx + 1 < Stages.Num()) ? Stages[Idx + 1].LevelPath : NAME_None;
}

/**
 * 레벨 이름 견주기 — 한쪽은 긴 경로(/Game/Maps/Rasel/L01_Jangteo_Street)로,
 * 다른 쪽은 짧은 이름(L01_Jangteo_Street)으로 오는 자리가 많다(포탈은 짧은 이름을 쓴다).
 * 그래서 마지막 토막만 견준다.
 */
bool UGameFlowSubsystem::LevelNameEquals(FName A, FName B)
{
    if (A.IsNone() || B.IsNone()) return false;
    if (A == B) return true;

    FString SA = A.ToString();
    FString SB = B.ToString();
    int32 P;
    if (SA.FindLastChar('/', P)) SA = SA.RightChop(P + 1);
    if (SB.FindLastChar('/', P)) SB = SB.RightChop(P + 1);
    SA = SA.Left(SA.Find(TEXT(".")) == INDEX_NONE ? SA.Len() : SA.Find(TEXT(".")));
    SB = SB.Left(SB.Find(TEXT(".")) == INDEX_NONE ? SB.Len() : SB.Find(TEXT(".")));
    return SA.Equals(SB, ESearchCase::IgnoreCase);
}

void UGameFlowSubsystem::DoTravel(FName LevelPath, FName EntryTag)
{
    PendingEntryTag  = EntryTag;
    LastEntryTag     = EntryTag;   // 소모되는 Pending과 달리 세이브에 남는다
    CurrentLevelPath = LevelPath;

    // 이동 전에 먼저 저장한다 — 이동 도중 종료돼도 이어하기가 이 지점을 가리키도록.
    SaveRun();

    if (UGameInstance* GI = GetGameInstance())
        UGameplayStatics::OpenLevel(GI, LevelPath);
}

bool UGameFlowSubsystem::ConsumeOpeningPending()
{
    const bool b = bOpeningPending;
    bOpeningPending = false;
    return b;
}

FName UGameFlowSubsystem::ConsumePendingEntryTag()
{
    const FName Tag = PendingEntryTag;
    PendingEntryTag = NAME_None;   // 한 번만 쓰이고 사라진다(다음 레벨로 새지 않게)
    return Tag;
}

void UGameFlowSubsystem::NotifyLevelEntered(FName LevelPath)
{
    // 진행표에 없는 무대(시작 화면·전투 시험판 등)는 진행에 기록하지 않는다.
    // 기록하면 "이어하기"가 그 화면을 가리켜, 이어하기를 눌렀는데 시작 화면이 다시 뜬다.
    const bool bInProgression = Stages.ContainsByPredicate(
        [LevelPath](const FGameStage& S) { return LevelNameEquals(S.LevelPath, LevelPath); });
    if (!bInProgression)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameFlow] %s 는 진행표의 무대가 아니다 — 진행에 기록하지 않는다."),
            *LevelPath.ToString());
        return;
    }

    // 타이틀을 거치지 않고 레벨을 바로 재생한 경우 진행이 비어 있다 → 그 레벨에 맞는 스테이지로 맞추고,
    // 저장된 진행도 없으면 "지금 새로 시작하는 것"으로 보고 오프닝을 튼다.
    // (레벨·스토리 담당이 에디터에서 그냥 재생해도 게임의 시작이 그대로 보이게 하려는 것)
    if (CurrentStageId.IsNone())
    {
        const int32 Idx = Stages.IndexOfByPredicate(
            [LevelPath](const FGameStage& S) { return S.LevelPath == LevelPath; });
        if (Idx != INDEX_NONE)
        {
            CurrentStageId = Stages[Idx].StageId;
            Chapter        = Stages[Idx].Chapter;
            if (!HasSavedRun()) bOpeningPending = true;
            OnStageChanged.Broadcast(CurrentStageId);
        }
    }

    CurrentLevelPath = LevelPath;

    // ★문으로 진행하기 — 지금 스테이지의 조건이 "어느 레벨로 들어가면"이고 방금 그 레벨에 들어왔으면
    //   여기서 한 칸 넘어간다. 이러면 진행이 컷신이 아니라 **플레이어가 문을 연 결과**가 된다.
    //   (스테이지가 옮겨 준 이동에는 안 걸린다 — 그때는 이미 다음 칸이 현재 칸이라 조건이 안 맞는다.)
    FGameStage Cur;
    if (GetStage(CurrentStageId, Cur)
        && Cur.Goal == EStageGoal::EnterLevel
        && LevelNameEquals(Cur.GoalLevel, LevelPath))
    {
        const int32 Idx = IndexOfStage(CurrentStageId);
        if (Idx != INDEX_NONE && Idx + 1 < Stages.Num())
        {
            const FGameStage& Next = Stages[Idx + 1];
            // 다음 칸의 무대가 방금 들어온 이 레벨이어야 자연스럽다. 아니면 그 무대로 옮겨 준다.
            CurrentStageId = Next.StageId;
            Chapter        = Next.Chapter;
            UE_LOG(LogTemp, Log, TEXT("[GameFlow] 문으로 진행: %s"), *CurrentStageId.ToString());
            OnStageChanged.Broadcast(CurrentStageId);

            if (!LevelNameEquals(Next.LevelPath, LevelPath))
            {
                DoTravel(Next.LevelPath, Next.EntryTag);
                return;
            }
        }
    }

    SaveRun();
}

void UGameFlowSubsystem::SaveRun()
{
    UGameFlowSave* S = Cast<UGameFlowSave>(UGameplayStatics::CreateSaveGameObject(UGameFlowSave::StaticClass()));
    if (!S) return;

    S->bHasRun          = true;
    S->CurrentStageId   = CurrentStageId;
    S->CurrentLevelPath = CurrentLevelPath;
    S->CurrentEntryTag  = LastEntryTag;
    S->Chapter          = Chapter;
    S->PlaySeconds      = PlaySeconds;
    S->SavedAt          = FDateTime::Now();

    UGameplayStatics::SaveGameToSlot(S, FlowSlotName(), 0);
}

/**
 * 전투 레벨이 열렸다 — 결과를 지켜볼 배우를 하나 세운다.
 * 전투를 부른 칸이 없으면(전투 담당이 판을 시험 중이면) 아무것도 안 한다.
 */
void UGameFlowSubsystem::HandleWorldActorsInitialized(const UWorld::FActorsInitializedParams& Params)
{
    UWorld* W = Params.World;
    if (!W || BattleFromStageId.IsNone()) return;
    if (W->GetGameInstance() != GetGameInstance()) return;   // 다른 세계(에디터 미리보기 등)는 건너뛴다

    FGameStage From;
    if (!GetStage(BattleFromStageId, From) || From.Goal != EStageGoal::Battle) return;

    const FName Here = FName(*W->GetOutermost()->GetName());
    const FName Want = From.GoalLevel.IsNone() ? FName(TEXT("HexBattle")) : From.GoalLevel;
    if (!LevelNameEquals(Want, Here)) return;

    FActorSpawnParameters Sp;
    Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (ABattleReturnWatcher* Watcher = W->SpawnActor<ABattleReturnWatcher>(ABattleReturnWatcher::StaticClass(), Sp))
    {
#if WITH_EDITOR
        Watcher->SetActorLabel(TEXT("[진행] 전투 결과 지켜보기"));
#endif
        UE_LOG(LogTemp, Log, TEXT("[GameFlow] 전투 판(%s)에 결과 지켜보기를 세웠다 — 부른 칸=%s"),
            *Here.ToString(), *BattleFromStageId.ToString());
    }
}

void UGameFlowSubsystem::NotifyBattleFinished(bool bWon)
{
    if (BattleFromStageId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameFlow] 전투가 끝났는데 부른 칸이 없다 — 돌아갈 데를 모른다."));
        return;
    }

    FGameStage From;
    const FName FromId = BattleFromStageId;
    BattleFromStageId = NAME_None;   // 한 판에 한 번만
    if (!GetStage(FromId, From)) return;

    // 지면 갈래가 따로 정해져 있으면 그리로. 없으면 이겼든 졌든 다음 칸으로 간다
    // (패배는 막힘이 아니라 대가 — 진 결과는 그 뒤 장면이 진다).
    if (!bWon && !From.StageOnLose.IsNone())
    {
        UE_LOG(LogTemp, Log, TEXT("[GameFlow] 졌다 — %s 갈래로."), *From.StageOnLose.ToString());
        GoToStage(From.StageOnLose);
        return;
    }

    const int32 Idx = IndexOfStage(FromId);
    if (Idx != INDEX_NONE && Idx + 1 < Stages.Num())
    {
        UE_LOG(LogTemp, Log, TEXT("[GameFlow] 전투 뒤 다음 칸 — %s"), *Stages[Idx + 1].StageId.ToString());
        GoToStage(Stages[Idx + 1].StageId);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[GameFlow] 전투가 진행표의 마지막 칸이었다."));
    }
}

void UGameFlowSubsystem::FlowNewGame()  { StartNewGame(); }
void UGameFlowSubsystem::FlowContinue() { ContinueGame(); }
void UGameFlowSubsystem::FlowGoStage(const FString& StageId) { GoToStage(FName(*StageId)); }

void UGameFlowSubsystem::FlowWhere()
{
    UE_LOG(LogTemp, Log, TEXT("[GameFlow] 스테이지=%s · 레벨=%s · 장=%d · 진행세이브=%s"),
        *CurrentStageId.ToString(), *CurrentLevelPath.ToString(), Chapter,
        HasSavedRun() ? TEXT("있음") : TEXT("없음"));
}

void UGameFlowSubsystem::DeleteRun()
{
    if (UGameplayStatics::DoesSaveGameExist(FlowSlotName(), 0))
        UGameplayStatics::DeleteGameInSlot(FlowSlotName(), 0);

    CurrentStageId   = NAME_None;
    CurrentLevelPath = NAME_None;
    PendingEntryTag  = NAME_None;
    LastEntryTag     = NAME_None;
    Chapter          = 1;
    PlaySeconds      = 0.f;
    bOpeningPending  = false;
}
