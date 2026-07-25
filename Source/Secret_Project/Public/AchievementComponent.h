#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimeComponent.h" // EDayPhase (OnTimeChanged 시그니처)
#include "AchievementComponent.generated.h"

UENUM(BlueprintType)
enum class EAchievementType : uint8
{
    ReachLevel     UMETA(DisplayName = "레벨 도달"),
    CompleteQuests UMETA(DisplayName = "퀘스트 완료 수"),
    MaxBonds       UMETA(DisplayName = "최고 인연 수"),
    DiscoverAreas  UMETA(DisplayName = "지역 발견 수"),
    DefeatEnemies  UMETA(DisplayName = "적 처치 누계"),
    HighestSocialStat UMETA(DisplayName = "최고 사회 스탯 랭크")
};

USTRUCT(BlueprintType)
struct FAchievementDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement") FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement") FString Title = TEXT("업적");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement") FString Description;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement") EAchievementType Type = EAchievementType::ReachLevel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement") int32 Threshold = 1;

    // 해금 보상(선택)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement") int32 RewardGold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement") FName RewardItemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Achievement") int32 RewardItemCount = 0;
};

// UI 표시용 한 줄 상태(by-value)
USTRUCT(BlueprintType)
struct FAchievementStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Achievement") FString Title;
    UPROPERTY(BlueprintReadOnly, Category = "Achievement") FString Description;
    UPROPERTY(BlueprintReadOnly, Category = "Achievement") int32 Current = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Achievement") int32 Threshold = 1;
    UPROPERTY(BlueprintReadOnly, Category = "Achievement") bool bUnlocked = false;
    UPROPERTY(BlueprintReadOnly, Category = "Achievement") FString Reward; // "보상: 100 G" 등(없으면 빈 문자열)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAchievementUnlocked, FName, Id);

/**
 * 도전과제/업적. 기존 시스템 데이터(레벨/퀘스트완료/최고인연/지역발견/적처치)를 읽어 마일스톤 평가.
 * 해금은 1회성이라 세이브 CollectedIds 재사용(IsCollected/MarkCollected) — 새 세이브필드 0.
 * 시간 변경 + 위젯 오픈 시 재평가. 로직 전부 C++, 다른 시스템은 읽기/호출만(비침범).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UAchievementComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAchievementComponent();

    static const TArray<FAchievementDef>& GetCatalog();

    UPROPERTY(BlueprintAssignable, Category = "Achievement")
    FOnAchievementUnlocked OnAchievementUnlocked;

    // 모든 업적을 재평가 — 달성+미해금이면 해금(기록+메시지+델리게이트)
    UFUNCTION(BlueprintCallable, Category = "Achievement")
    void CheckAll();

    // 특정 타입의 현재 진행 수치
    UFUNCTION(BlueprintPure, Category = "Achievement")
    int32 GetProgressFor(EAchievementType Type) const;

    // UI용 전체 상태 목록
    UFUNCTION(BlueprintCallable, Category = "Achievement")
    void GetStatuses(TArray<FAchievementStatus>& OutList) const;

protected:
    virtual void BeginPlay() override;

private:
    void InitAchievements();

    UFUNCTION()
    void OnWorldTimeChanged(int32 Day, EDayPhase Phase);
};
