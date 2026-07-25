#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "StoryManager.h"   // FStoryLine(화자+대사) — 스토리 담당이 쓰는 것을 그대로 쓴다(같은 것을 두 번 만들지 않는다)
#include "GameFlowSubsystem.generated.h"

/** 이 스테이지에서만 서 있는 사람(장면용). 레벨 파일에 안 심고 진행표를 보고 세운다. */
USTRUCT(BlueprintType)
struct FStageNpc
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    float Yaw = 0.f;

    /** 쓸 몸(블루프린트). 비면 프로젝트 기본 NPC. ※지금은 아무거나 세워 두는 자리다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FString BlueprintPath;

    /** 말 걸었을 때(E) 할 대사. 비면 몸(아키타입) 기본 대사. 허브에서 사람에게 다가가 듣는 소문·정보. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    TArray<FString> Lines;
};

/**
 * 이 스테이지를 어떻게 노나 — 페르소나 기본 구조의 뼈대.
 *   - Hub     : 자유 탐험이 기본. 진입 즉시 대사를 강제하지 않는다. 사람에게 말 걸고,
 *               거리를 돌고, 목적지 이동 맵으로 오간다. 목표는 "여기 가면 이야기가 진행된다"는
 *               소프트 힌트일 뿐 — 그 자리에 닿으면 그때 에피소드(ObjectiveScene)가 열린다.
 *   - Episode : 스크립트 장면(컷신). 진입하면 ArrivalScene이 흐른다. 이야기의 구두점.
 * 기본은 Hub — 특별히 컷신으로 지정하지 않는 한 플레이어는 늘 자유롭게 논다.
 */
UENUM(BlueprintType)
enum class EStageKind : uint8
{
    Hub,
    Episode,
};

/** 이 스테이지를 무엇으로 끝내나. */
UENUM(BlueprintType)
enum class EStageGoal : uint8
{
    SceneOnly,   // 장면만 보고 다음 칸으로 (막을 닫는 자리)
    ReachSpot,   // 이 레벨 안의 어떤 자리에 발을 들이면
    EnterLevel,  // 어떤 레벨로 문을 열고 들어가면 (문은 레벨에 이미 있는 포탈을 쓴다)
    Battle,      // 전투 판으로 넘어갔다가, 판이 끝나면 이기고 진 것에 따라 돌아온다
};

/**
 * 진행 단계(스테이지) 한 칸.
 *
 * ★개념 구분 — 여기가 이 시스템의 핵심이다.
 *   - **스테이지** = 이야기가 어디까지 왔나(진행 지점). 순서가 있다.
 *   - **레벨**     = 지금 발을 딛고 선 장소. 라셀은 허브형이라 플레이어는 스테이지와 무관하게
 *                    큰길↔골동상↔뒷골목을 오간다. 문을 오간다고 진행이 바뀌지는 않는다.
 * 그래서 이어하기는 "현재 스테이지"와 "마지막으로 있던 레벨"을 따로 복원한다.
 */
USTRUCT(BlueprintType)
struct FGameStage
{
    GENERATED_BODY()

    // 스테이지 식별자 (세이브에 남는 값 — 한 번 정하면 바꾸지 말 것)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    FName StageId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    int32 Chapter = 1;

    // 화면에 뜨는 이름 ("장터 큰길")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    FString DisplayName;

    // 이 칸을 어떻게 노나 — 기본 Hub(자유 탐험). Episode면 진입 시 ArrivalScene 컷신이 흐른다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    EStageKind Kind = EStageKind::Hub;

    // 이 스테이지가 열릴 때 데려다 놓을 레벨 (긴 패키지 경로: /Game/Maps/Rasel/L01_Jangteo_Street)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    FName LevelPath;

    // 그 레벨의 어느 문으로 들어갈지 — APlayerStart의 PlayerStartTag와 맞춘다. None이면 아무 PlayerStart.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    FName EntryTag;

    // 이 스테이지에서 재생할 스토리 비트(StoryManager의 BeatId). None이면 연출 없이 조작 시작.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
    FName StoryBeatId;

    // ── 시작 자리 덮어쓰기 ──
    // 레벨의 PlayerStart가 쓸 자리에 있지 않을 때(모서리·벽 앞) 진행표에서 바로잡는다.
    // 레벨 파일은 레벨 담당이 계속 고쳐 저장하므로 .umap을 건드리지 않고 여기서 세운다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Spawn")
    bool bOverrideSpawn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Spawn")
    FVector SpawnLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Spawn")
    float SpawnYaw = 0.f;

    // ── 장면 ──
    // 이 스테이지에 막 도착했을 때 검은 화면에 뜨는 때·자리 카드 ("그날 밤 — 셋집 옥상").
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Scene")
    FText TransitionCard;

    // 도착하자마자 흐르는 대사. 비면 바로 조작이 시작된다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Scene")
    TArray<FStoryLine> ArrivalScene;

    // 목표에 닿았을 때 흐르는 대사. 이게 끝나야 다음 칸으로 넘어간다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Scene")
    TArray<FStoryLine> ObjectiveScene;

    // 이 스테이지에만 서 있는 사람들(장면용). 레벨에 이미 있는 사람은 여기 안 적는다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Scene")
    TArray<FStageNpc> SceneNpcs;

    // ── 이 스테이지를 끝내는 조건 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Objective")
    EStageGoal Goal = EStageGoal::ReachSpot;

    // Goal이 EnterLevel일 때 — 이 레벨로 가는 문을 목표로 잡는다(문 위치는 레벨에서 찾는다).
    // Goal이 Battle일 때 — 판을 깔 전투 레벨(비면 /Game/Maps/HexBattle).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Objective")
    FName GoalLevel;

    // Goal이 Battle이고 **졌을 때** 갈 칸. 비면 진 판도 다음 칸으로 간다
    // (패배는 막힘이 아니라 대가 — 시나리오 정본 §1).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Objective")
    FName StageOnLose;

    // 이 자리에 발을 들이면 다음 스테이지로 넘어간다. 표식은 레벨 파일이 아니라 진행표를 보고 런타임에 세운다
    // (레벨은 레벨 담당이 계속 고쳐 저장하는 중이라, 진행 장치를 .umap 안에 박아 두면 서로 덮어쓴다).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Objective")
    bool bHasObjective = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Objective")
    FVector ObjectiveLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Objective")
    float ObjectiveRadius = 240.f;

    // 화면에 뜨는 목표 한 줄
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Objective")
    FText ObjectiveLabel;
};

/**
 * 진행 전용 세이브. 슬롯 "FlowSave".
 * 다른 담당이 쓰는 "PlayerSave"(스탯/인벤)·"StorySave"(비트)를 건드리지 않는다 — 침범하면 서로 덮어쓴다.
 */
UCLASS()
class SECRET_PROJECT_API UGameFlowSave : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY() bool bHasRun = false;          // 시작한 게임이 있나(=이어하기 가능)
    UPROPERTY() FName CurrentStageId;          // 진행 지점
    UPROPERTY() FName CurrentLevelPath;        // 마지막으로 있던 장소
    UPROPERTY() FName CurrentEntryTag;         // 그 장소의 어느 문으로 들어와 있었나
    UPROPERTY() int32 Chapter = 1;
    UPROPERTY() FDateTime SavedAt;             // 이어하기 화면에 "언제"를 띄우기 위해
    UPROPERTY() float PlaySeconds = 0.f;       // 누적 플레이 시간
};

/** 목적지 이동 맵(페르소나식)에 뜨는 라셀의 한 구역 — 자유롭게 오갈 수 있는 곳. */
USTRUCT(BlueprintType)
struct FTravelDistrict
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Flow") FString Name;
    UPROPERTY(BlueprintReadOnly, Category = "Flow") FName LevelPath;
    UPROPERTY(BlueprintReadOnly, Category = "Flow") FName EntryTag;
    UPROPERTY(BlueprintReadOnly, Category = "Flow") FString Note;   // 한 줄 설명(구역 성격)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStageChanged, FName, StageId);

/**
 * 게임 전체 진행 담당.
 *
 * 타이틀에서 PLAY를 누른 순간부터 레벨을 오가며 이어하기까지 — 그 사슬 전체를 여기서 쥔다.
 * 레벨 제작·전투 시스템·스토리 집필은 각자 담당이 있고, 이 시스템은 그것들을 **잇는다**:
 *   타이틀 → 새 게임/이어하기 → 레벨 진입(도착 지점 배치) → 진행 갱신 → 자동 저장 → 다음 레벨
 *
 * 사용:
 *   UGameFlowSubsystem* Flow = GetGameInstance()->GetSubsystem<UGameFlowSubsystem>();
 *   Flow->StartNewGame();                       // 새 게임 (첫 스테이지로)
 *   Flow->ContinueGame();                       // 이어하기 (저장된 장소로)
 *   Flow->TravelToLevel(Path, TEXT("FromStreet")); // 문/포탈로 장소 이동
 *   Flow->AdvanceStage();                       // 스토리가 한 칸 진행됐을 때
 */
UCLASS()
class SECRET_PROJECT_API UGameFlowSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // ── 시작/이어하기 ────────────────────────────────
    // 저장된 진행이 있나 (타이틀의 "이어하기" 활성화 판단)
    UFUNCTION(BlueprintPure, Category = "Flow")
    bool HasSavedRun() const;

    // 새 게임 — 진행을 첫 스테이지로 되돌리고 그 레벨로 이동. 기존 진행 세이브는 지운다.
    UFUNCTION(BlueprintCallable, Category = "Flow")
    void StartNewGame();

    // 이어하기 — 저장된 레벨/도착 지점으로 이동. 저장이 없으면 새 게임으로 대신한다.
    UFUNCTION(BlueprintCallable, Category = "Flow")
    void ContinueGame();

    // ── 장소 이동 ────────────────────────────────────
    // 진행은 그대로 두고 장소만 옮긴다(문·포탈). EntryTag = 도착 레벨에서 설 자리.
    UFUNCTION(BlueprintCallable, Category = "Flow")
    void TravelToLevel(FName LevelPath, FName EntryTag);

    // ── 진행 ─────────────────────────────────────────
    // 특정 스테이지로 진행을 옮기고, 그 스테이지의 무대 레벨로 데려간다.
    UFUNCTION(BlueprintCallable, Category = "Flow")
    void GoToStage(FName StageId);

    // 다음 스테이지로 한 칸. 마지막이면 무동작(false).
    UFUNCTION(BlueprintCallable, Category = "Flow")
    bool AdvanceStage();

    // 다음 칸이 있나(마지막 칸이면 false — 게임모드가 "여기까지"를 띄운다).
    UFUNCTION(BlueprintPure, Category = "Flow")
    bool HasNextStage() const;

    // 다음 칸의 무대 레벨. 없으면 None.
    UFUNCTION(BlueprintPure, Category = "Flow")
    FName GetNextStageLevel() const;

    UFUNCTION(BlueprintPure, Category = "Flow")
    FName GetCurrentStageId() const { return CurrentStageId; }

    UFUNCTION(BlueprintPure, Category = "Flow")
    bool GetStage(FName StageId, FGameStage& OutStage) const;

    // 진행표 전체(읽기 전용) — 디버그 메뉴/챕터 선택 화면용
    UFUNCTION(BlueprintPure, Category = "Flow")
    const TArray<FGameStage>& GetStages() const { return Stages; }

    // 목적지 이동 맵에 뜰 라셀 구역들(자유롭게 오갈 수 있는 곳). 이동 화면(SDestinationMenu)이 순회한다.
    static TArray<FTravelDistrict> GetTravelDistricts();

    // 이 레벨에 늘 서 있는 상주 사람들(거리를 살아 있게). 스테이지와 무관하게 진입할 때마다 세운다.
    // 스토리 장면용 사람(SceneNpcs)과 별개 — 이쪽은 말 걸면 소문·정보를 주는 배경 인물.
    static TArray<FStageNpc> GetResidentNpcs(FName LevelPath);

    // ── 레벨 진입 시 GameMode가 부르는 것들 ──────────
    // 이번 이동에서 설 자리 태그를 꺼내 쓴다(한 번 쓰면 비워진다 — 다음 이동에 새는 것 방지).
    FName ConsumePendingEntryTag();

    // 새 게임으로 막 들어온 참인가 = 오프닝을 틀어야 하나. 한 번 꺼내면 꺼진다(문 드나들 때마다 다시 틀리지 않게).
    bool ConsumeOpeningPending();

    // 도착한 레벨을 진행 세이브에 기록 + 자동 저장. GameMode::BeginPlay에서 호출.
    void NotifyLevelEntered(FName LevelPath);

    // ── 전투 ─────────────────────────────────────────
    // 전투 판이 끝났다고 알린다(전투 레벨에 세워 둔 ABattleReturnWatcher가 부른다).
    // 이겼으면 다음 칸으로, 졌으면 StageOnLose로(없으면 역시 다음 칸으로 — 패배는 막힘이 아니라 대가).
    void NotifyBattleFinished(bool bWon);

    // 진행 저장/삭제
    UFUNCTION(BlueprintCallable, Category = "Flow")
    void SaveRun();

    UFUNCTION(BlueprintCallable, Category = "Flow")
    void DeleteRun();

    // 스테이지가 바뀔 때 (HUD의 목표 표시·스토리 트리거용)
    UPROPERTY(BlueprintAssignable, Category = "Flow")
    FOnStageChanged OnStageChanged;

    static const TCHAR* FlowSlotName() { return TEXT("FlowSave"); }

    /** 긴 경로와 짧은 이름을 섞어 써도 같은 레벨인지 가려낸다(포탈은 짧은 이름을 쓴다). */
    static bool LevelNameEquals(FName A, FName B);

    // ── 콘솔 명령 (~ 콘솔에서 바로 진행을 굴려 볼 수 있게) ──
    // FlowNewGame            : 타이틀을 거치지 않고 새 게임 시작(오프닝 포함)
    // FlowContinue           : 이어하기
    // FlowGoStage S03_Backalley : 그 스테이지로 건너뛰기
    // FlowWhere              : 지금 어느 스테이지·어느 레벨인지 로그로
    UFUNCTION(Exec) void FlowNewGame();
    UFUNCTION(Exec) void FlowContinue();
    UFUNCTION(Exec) void FlowGoStage(const FString& StageId);
    UFUNCTION(Exec) void FlowWhere();

protected:
    // 진행표. 지금은 C++ 기본표(BuildDefaultStages)로 깔고, 스토리가 확정되는 대로 늘린다.
    UPROPERTY()
    TArray<FGameStage> Stages;

    void BuildDefaultStages();
    int32 IndexOfStage(FName StageId) const;

    // 전투 레벨이 열리면 결과를 지켜볼 배우를 하나 세운다(전투 담당 파일은 안 건드린다).
    void HandleWorldActorsInitialized(const UWorld::FActorsInitializedParams& Params);

    // 실제 레벨 이동 한 곳으로 모음 — 저장·태그 세팅을 빠뜨리지 않도록.
    void DoTravel(FName LevelPath, FName EntryTag);

private:
    UPROPERTY() FName CurrentStageId;
    UPROPERTY() FName CurrentLevelPath;
    UPROPERTY() FName PendingEntryTag;   // 이동 중에만 들고 있는 값(도착하면서 소모된다)
    UPROPERTY() FName LastEntryTag;      // 마지막으로 어느 문으로 들어왔나(세이브에 남아 이어하기가 그 자리로)
    UPROPERTY() bool bOpeningPending = false;  // 새 게임 → 도착 레벨에서 오프닝 1회
    UPROPERTY() FName BattleFromStageId;       // 지금 전투 중이면, 그 전투를 부른 칸
    UPROPERTY() int32 Chapter = 1;
    UPROPERTY() float PlaySeconds = 0.f;
};
