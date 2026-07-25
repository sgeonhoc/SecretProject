#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimeComponent.generated.h"

UENUM(BlueprintType)
enum class EDayPhase : uint8
{
    Morning UMETA(DisplayName = "아침"),
    Day     UMETA(DisplayName = "낮"),
    Evening UMETA(DisplayName = "저녁"),
    Night   UMETA(DisplayName = "밤")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeChanged, int32, Day, EDayPhase, Phase);

/**
 * 게임 내 날짜/시간대(페르소나식). 플레이어에 장착, 세이브 영구화.
 * 시간은 자동으로 흐르지 않고 휴식(세이브포인트)·이벤트로 진행. 조명/배경은 BP가 OnTimeChanged 구독해 처리.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UTimeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTimeComponent();

    UPROPERTY(BlueprintAssignable, Category = "Time")
    FOnTimeChanged OnTimeChanged;

    // 다음 시간대로 (밤 다음은 다음날 아침)
    UFUNCTION(BlueprintCallable, Category = "Time")
    void AdvancePhase();

    // 다음날 아침으로 (휴식/취침)
    UFUNCTION(BlueprintCallable, Category = "Time")
    void AdvanceToNextDay();

    UFUNCTION(BlueprintPure, Category = "Time")
    FORCEINLINE int32 GetDay() const { return Day; }
    UFUNCTION(BlueprintPure, Category = "Time")
    FORCEINLINE EDayPhase GetPhase() const { return Phase; }

    // "3일차 아침" 형태 라벨
    UFUNCTION(BlueprintPure, Category = "Time")
    FString GetTimeLabel() const;

    // 세이브/로드
    void LoadTime(int32 InDay, EDayPhase InPhase);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Time")
    int32 Day = 1;

    UPROPERTY(VisibleAnywhere, Category = "Time")
    EDayPhase Phase = EDayPhase::Morning;

private:
    void NotifyChanged();
};
