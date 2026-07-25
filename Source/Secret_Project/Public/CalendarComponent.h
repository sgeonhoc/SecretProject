#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimeComponent.h" // EDayPhase
#include "CalendarComponent.generated.h"

// 특정 날짜(+선택적 시간대)에 1회 발생하는 캘린더 이벤트(축제/마감 등). 데이터 주도.
USTRUCT(BlueprintType)
struct FCalendarEvent
{
    GENERATED_BODY()

    // 발견/중복발생 방지용 고유 키(세이브 CollectedIds 재사용). 비면 무시.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar") FName EventId;

    // 발생 날짜 (TimeComponent.Day 기준)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar") int32 Day = 1;

    // false면 특정 시간대(Phase)에만 발생. true면 그 날 아무 시간대 도달 시 발생.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar") bool bAnyPhase = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar") EDayPhase Phase = EDayPhase::Morning;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar") FString Title = TEXT("이벤트");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar") FString Message;

    // 발생 시 보상(선택)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar") int32 RewardGold = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar") FName RewardItemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar") int32 RewardItemCount = 0;

    // 시험 이벤트(페르소나식): true면 플레이어 지식(Knowledge) 사회스탯 랭크 × ExamGoldPerKnowledgeRank 만큼 장학금 추가
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar") bool bExamBonusByKnowledge = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar") int32 ExamGoldPerKnowledgeRank = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCalendarEvent, FName, EventId);

/**
 * 페르소나식 캘린더 이벤트. 플레이어에 장착, 플레이어 TimeComponent.OnTimeChanged 구독.
 * 해당 날짜/시간대 도달 시 1회 발생(보상 지급 + 메시지 + 델리게이트). 중복발생은 세이브 CollectedIds로 방지.
 * 카탈로그는 정적(샘플) + ExtraEvents(BP 데이터 주도)로 확장.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UCalendarComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCalendarComponent();

    static const TArray<FCalendarEvent>& GetCatalog();

    // BP/디자이너가 추가하는 이벤트 (카탈로그와 합쳐서 평가)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calendar")
    TArray<FCalendarEvent> ExtraEvents;

    UPROPERTY(BlueprintAssignable, Category = "Calendar")
    FOnCalendarEvent OnCalendarEvent;

    // 특정 날짜/시간대에 대해 발생 가능한 이벤트 평가(미발생 + 조건충족 → 발생)
    UFUNCTION(BlueprintCallable, Category = "Calendar")
    void CheckEvents(int32 Day, EDayPhase Phase);

protected:
    virtual void BeginPlay() override;

private:
    // 플레이어 스폰/세이브로드 순서 보장 위해 다음 틱에 구독 + 현재 날짜 평가
    void InitCalendar();

    UFUNCTION()
    void OnWorldTimeChanged(int32 Day, EDayPhase Phase);

    void FireEvent(const FCalendarEvent& Event);
};
