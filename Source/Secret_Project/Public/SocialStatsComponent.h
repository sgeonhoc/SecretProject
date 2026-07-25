#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SaveGame.h"
#include "SocialStatsComponent.generated.h"

// 페르소나식 사회 스탯 5종 (개인 능력치 — NPC 인연(RelationshipComponent)과는 별개)
UENUM(BlueprintType)
enum class ESocialStat : uint8
{
    Knowledge   UMETA(DisplayName = "지식"),
    Charm       UMETA(DisplayName = "매력"),
    Guts        UMETA(DisplayName = "용기"),
    Kindness    UMETA(DisplayName = "친절"),
    Proficiency UMETA(DisplayName = "숙련")
};

// 사회 스탯 한글명 공유 헬퍼 (컴포넌트/위젯 공용 단일 소스)
FORCEINLINE FString LexSocialStat(ESocialStat S)
{
    switch (S)
    {
    case ESocialStat::Knowledge:   return TEXT("지식");
    case ESocialStat::Charm:       return TEXT("매력");
    case ESocialStat::Guts:        return TEXT("용기");
    case ESocialStat::Kindness:    return TEXT("친절");
    case ESocialStat::Proficiency: return TEXT("숙련");
    default:                       return TEXT("?");
    }
}

// 사회 스탯 전용 세이브 (자체 슬롯 — 공유 SecretSaveGame 비침범)
UCLASS()
class SECRET_PROJECT_API USocialStatsSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() TArray<int32> Points;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSocialStatChanged, ESocialStat, Stat, int32, NewRank);

/**
 * 페르소나식 사회 스탯(지식/매력/용기/친절/숙련). 플레이어에 장착.
 * 활동(독서·대화·탐험 등)으로 포인트 누적 → 랭크업(최대 5). 다른 시스템이 MeetsRank로 콘텐츠 게이팅.
 * 로직 전부 C++. 자체 세이브 슬롯("SocialStatsSave")에 영구화(공유 세이브 비침범).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API USocialStatsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USocialStatsComponent();

    static constexpr int32 StatCount = 5;
    static constexpr int32 MaxRank = 5;

    UPROPERTY(BlueprintAssignable, Category = "Social")
    FOnSocialStatChanged OnSocialStatChanged;

    // 활동으로 포인트 적립 → 랭크업 시 메시지 + 델리게이트 + 저장
    UFUNCTION(BlueprintCallable, Category = "Social")
    void AddPoints(ESocialStat Stat, int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Social")
    int32 GetRank(ESocialStat Stat) const;

    UFUNCTION(BlueprintPure, Category = "Social")
    int32 GetPoints(ESocialStat Stat) const;

    // 현재 랭크에서 다음 랭크까지 누적/필요 포인트 (UI 게이지용). 최대 랭크면 둘 다 동일.
    UFUNCTION(BlueprintPure, Category = "Social")
    void GetRankProgress(ESocialStat Stat, int32& OutInto, int32& OutNeeded) const;

    // 콘텐츠 게이팅용: 해당 스탯이 RequiredRank 이상인가
    UFUNCTION(BlueprintPure, Category = "Social")
    bool MeetsRank(ESocialStat Stat, int32 RequiredRank) const { return GetRank(Stat) >= RequiredRank; }

    // "지식 — Rank 2 (15/30)" 형태 라벨 (위젯용)
    UFUNCTION(BlueprintPure, Category = "Social")
    FString GetStatLabel(ESocialStat Stat) const;

    // 최대 랭크 도달한 스탯 수 (요약/업적 연동용)
    UFUNCTION(BlueprintPure, Category = "Social")
    int32 GetMaxedCount() const;

protected:
    virtual void BeginPlay() override;

    // 인덱스별 누적 포인트 (ESocialStat 순서, 길이 StatCount)
    UPROPERTY(VisibleAnywhere, Category = "Social")
    TArray<int32> Points;

private:
    // 누적 포인트 → 랭크(1~MaxRank). 랭크별 시작 누적 포인트 임계값.
    static int32 ThresholdForRank(int32 Rank);
    static int32 RankForPoints(int32 P);

    void SaveToSlot() const;
    void LoadFromSlot();

    static const TCHAR* SlotName() { return TEXT("SocialStatsSave"); }
};
