#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "StoryManager.generated.h"

// 메인 스토리 한 줄 (화자 + 대사). 스토리 표시 위젯이 순서대로 출력.
USTRUCT(BlueprintType)
struct FStoryLine
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FString Speaker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story", meta = (MultiLine = true))
    FString Line;
};

// 메인 스토리 비트(한 장면). 해금 조건 충족 시 등장, 보면 완료.
// 비트 카탈로그에 항목 추가 = 메인 시나리오 "확장". B의 NPC 잡담/사이드 퀘스트와 별개 레이어.
USTRUCT(BlueprintType)
struct FStoryBeat
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FName BeatId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    int32 Chapter = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    TArray<FStoryLine> Lines;

    // ── 해금 조건 (모두 충족해야 등장) ──
    // 선행 비트(완료돼야 함, None이면 무시)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Unlock")
    FName PrereqBeatId;

    // 이 날짜(Day) 이상 (0이면 무시). B TimeComponent의 Day 기준.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Unlock")
    int32 MinDay = 0;

    // 필요한 스토리 플래그(보스 처치 등, None이면 무시)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Unlock")
    FName RequiredFlag;

    // 모두 충족해야 하는 플래그 목록(트루 엔딩 등 다중조건용, 비면 무시)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Unlock")
    TArray<FName> RequiredAllFlags;

    // 인연(소셜링크) 게이트: 이 NPC와의 인연 랭크 조건(인연 에피소드용). None이면 무시.
    // B RelationshipComponent.GetRank(NPCName) 기준 — NPCName은 소셜 카탈로그 DisplayName.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Unlock")
    FName RequiredBondNPC;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Unlock")
    int32 RequiredBondRank = 0;

    // ── 완료 효과 ──
    // 완료 시 세우는 플래그(다음 비트의 RequiredFlag로 연쇄, None이면 없음)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Reward")
    FName GrantsFlag;

    // 완료 시 골드 보상(0이면 없음) — 플레이어 StatComponent에 적립
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Reward")
    int32 RewardGold = 0;

    // 완료 시 아이템 보상(인벤토리 카탈로그 Id, 비면 없음) + 개수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Reward")
    FName RewardItemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Reward", meta = (ClampMin = "1"))
    int32 RewardItemCount = 1;
};

// 스토리 진행 전용 세이브(공유 SecretSaveGame 비침범 — 자체 슬롯 "StorySave")
UCLASS()
class SECRET_PROJECT_API UStorySave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() TArray<FName> DoneBeats;
    UPROPERTY() TArray<FName> Flags;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryBeatUnlocked, FName, BeatId);

/**
 * 메인 스토리 아크 진행 관리(GameInstance 서브시스템). 페르소나식 메인 시나리오 스파인.
 * 비트 카탈로그(현대 배경) + 해금 판정(선행/날짜/플래그) + 완료/보상/자체 영구화.
 * 전투(보스 처치)가 SetFlag로 스토리 챕터를 여는 연결점. UI는 BP(기존 대화창 재사용 가능).
 */
UCLASS()
class SECRET_PROJECT_API UStoryManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // ── 카탈로그 ──
    static const TArray<FStoryBeat>& GetCatalog();
    UFUNCTION(BlueprintCallable, Category = "Story")
    static bool FindBeat(FName BeatId, FStoryBeat& OutBeat);

    // ── 진행 조회 ──
    UFUNCTION(BlueprintPure, Category = "Story")
    bool IsBeatDone(FName BeatId) const { return DoneBeats.Contains(BeatId); }

    UFUNCTION(BlueprintPure, Category = "Story")
    bool HasFlag(FName Flag) const { return Flags.Contains(Flag); }

    // 현재 가장 진척된 챕터(완료 비트 기준, 없으면 1)
    UFUNCTION(BlueprintPure, Category = "Story")
    int32 GetCurrentChapter() const;

    // 지금 해금됐고 아직 안 본 다음 비트(카탈로그 순서상 첫). CurrentDay는 B TimeComponent에서 전달(0이면 날짜조건 무시 안 함=충족 처리).
    UFUNCTION(BlueprintCallable, Category = "Story")
    bool GetNextAvailableBeat(int32 CurrentDay, FStoryBeat& OutBeat) const;

    UFUNCTION(BlueprintPure, Category = "Story")
    int32 GetCompletedCount() const { return DoneBeats.Num(); }

    UFUNCTION(BlueprintPure, Category = "Story")
    int32 GetTotalBeatCount() const { return GetCatalog().Num(); }

    // 완료한 비트 Id를 카탈로그 순서로 반환 (회상/스토리 저널 UI용 — 본 장면 다시보기)
    UFUNCTION(BlueprintCallable, Category = "Story")
    TArray<FName> GetCompletedBeatIds() const;

    // ── 진행 변경 ──
    // 외부(전투 보스 처치 등)에서 스토리 플래그 세움 → 다음 비트 해금 가능. 중복 무시 + 영구화.
    UFUNCTION(BlueprintCallable, Category = "Story")
    void SetFlag(FName Flag);

    // 비트 완료: 보상(골드) 지급 + GrantsFlag + 영구화. 이미 완료면 무시.
    UFUNCTION(BlueprintCallable, Category = "Story")
    void CompleteBeat(FName BeatId);

    UPROPERTY(BlueprintAssignable, Category = "Story")
    FOnStoryBeatUnlocked OnStoryBeatUnlocked;

    // ── 조건 게이트(레벨 상태 스왑) ──────────────────────
    // 월드 컨텍스트로 이 서브시스템을 얻는다(없으면 nullptr).
    static UStoryManagerSubsystem* Get(const UObject* WorldContextObject);

    // 상호작용 액터용 플래그 게이트: RequiredFlag가 있으면 그 플래그가 서 있어야, ForbiddenFlag가 있으면 없어야 통과.
    // 서브시스템이 없으면 통과(안전 기본). 레벨 재진입/시간대 변화 시 재평가에 쓴다.
    UFUNCTION(BlueprintPure, Category = "Story", meta = (WorldContext = "WorldContextObject"))
    static bool PassesFlagGate(const UObject* WorldContextObject, FName RequiredFlag, FName ForbiddenFlag);

private:
    UPROPERTY() TSet<FName> DoneBeats;
    UPROPERTY() TSet<FName> Flags;

    void Save() const;
    void Load();
};
