#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuestComponent.generated.h"

UENUM(BlueprintType)
enum class EQuestState : uint8
{
    Inactive  UMETA(DisplayName = "비활성"),
    Active    UMETA(DisplayName = "진행중"),
    Completed UMETA(DisplayName = "완료")
};

// B가 독립적으로 추적 가능한 목표만 (전투 처치 등은 추후 A 연동)
UENUM(BlueprintType)
enum class EQuestObjective : uint8
{
    TalkToNPC      UMETA(DisplayName = "NPC와 대화"),
    OpenChests     UMETA(DisplayName = "보물상자 열기"),
    DefeatEnemies  UMETA(DisplayName = "적 처치"),
    ReachDay       UMETA(DisplayName = "특정 날짜 도달"),
    ReadLore       UMETA(DisplayName = "읽을거리 읽기")
};

// 퀘스트 정의(카탈로그). 데이터 주도 — BP/카탈로그에서 추가 가능.
USTRUCT(BlueprintType)
struct FQuestDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest") FName QuestId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest") FString Title = TEXT("퀘스트");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest") FString Description;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest") EQuestObjective Objective = EQuestObjective::TalkToNPC;

    // TalkToNPC일 때 대상 NPCName
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest") FName TargetNPCName;
    // OpenChests일 때 목표 개수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest") int32 TargetCount = 1;

    // 보상
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest") int32 RewardGold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest") FName RewardItemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest") int32 RewardItemCount = 0;

    // 완료 시 자동 시작할 다음 퀘스트(체인). None이면 끝.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest") FName NextQuestId;

    // 스토리 게이트(옵트인): 이 스토리 플래그(StoryManager)가 세워지면 자동 시작. None이면 게이트 없음.
    // 메인 스파인(A)이 진행될 때마다 거리에 그 단계에 맞는 사이드 임무가 열린다(B 콘텐츠 레이어).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest") FName RequiredStoryFlag;
};

// 퀘스트 진행 상태(런타임 + 세이브 공용)
USTRUCT(BlueprintType)
struct FQuestRecord
{
    GENERATED_BODY()

    UPROPERTY() FName QuestId;
    UPROPERTY() int32 Progress = 0;
    UPROPERTY() EQuestState State = EQuestState::Inactive;
};

/**
 * 플레이어 퀘스트 추적. 로직 전부 C++. 목표(대화/상자열기) 충족 시 자동 완료 + 보상 지급 + 세이브.
 * 첫 플레이 시 카탈로그 첫 퀘스트 자동 시작(데모용). 카탈로그는 데이터 주도라 교체 쉬움.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UQuestComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UQuestComponent();

    static const TArray<FQuestDef>& GetCatalog();
    static bool FindDef(FName QuestId, FQuestDef& OutDef);

    UFUNCTION(BlueprintCallable, Category = "Quest")
    void StartQuest(FName QuestId);

    // 진행 이벤트 (월드/상호작용에서 호출)
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void NotifyTalkedTo(FName NPCName);

    UFUNCTION(BlueprintCallable, Category = "Quest")
    void NotifyChestOpened();

    // 전투 승리 시 호출 (A의 BattleManager에서 처치 수 전달). DefeatEnemies 목표 진행.
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void NotifyEnemyDefeated(int32 Count = 1);

    // 날짜 변경 시 호출 (TimeComponent 연동). ReachDay 목표를 현재 날짜로 평가.
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void NotifyDayReached(int32 CurrentDay);

    // 읽을거리(LoreNote)를 새로 읽었을 때 호출. ReadLore 목표 진행.
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void NotifyLoreRead(int32 Count = 1);

    UFUNCTION(BlueprintPure, Category = "Quest")
    EQuestState GetQuestState(FName QuestId) const;

    // 진행중 퀘스트 정의 목록(UI 표시용, by-value)
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void GetActiveQuests(TArray<FQuestDef>& OutQuests) const;

    // 특정 퀘스트의 현재 진행 수치
    UFUNCTION(BlueprintPure, Category = "Quest")
    int32 GetProgress(FName QuestId) const;

    // 완료한 퀘스트 수 (상태 화면 등)
    UFUNCTION(BlueprintPure, Category = "Quest")
    int32 GetCompletedCount() const;

    // 세이브/로드
    const TArray<FQuestRecord>& GetRecords() const { return Records; }
    void LoadRecords(const TArray<FQuestRecord>& InRecords) { Records = InRecords; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Quest")
    TArray<FQuestRecord> Records;

    FQuestRecord* FindRecord(FName QuestId);
    void CompleteQuest(FQuestRecord& Rec, const FQuestDef& Def);
    void PersistAndNotify(const FString& Message);

    // 스토리 플래그가 세워진 게이트 퀘스트를 자동 시작 (시간변경/시작 시 호출). StoryManager 읽기 전용.
    void CheckStoryQuests();
};
