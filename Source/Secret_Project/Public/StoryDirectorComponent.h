#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimeComponent.h"     // EDayPhase, OnTimeChanged 시그니처
#include "StoryManager.h"      // FStoryBeat
#include "StoryDirectorComponent.generated.h"

// 다음 스토리/미스터리 비트가 준비됨 → BP가 구독해 스토리 위젯을 띄움
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryBeatReady, FStoryBeat, Beat);

/**
 * 스토리 디렉터 — 플레이어에 붙여 메인/미스터리 스토리를 "일상 진행 중 자동으로" 흘려보낸다.
 * 날짜(시간대) 변화 시 StoryManagerSubsystem.GetNextAvailableBeat를 확인해 준비된 비트를 OnStoryBeatReady로 방송.
 * → 사용자 BP 작업 최소화: 이 컴포넌트 추가 + OnStoryBeatReady에 "스토리 위젯 띄우기"만 바인딩.
 * 보스 처치/인연 랭크/날짜로 해금된 비트가 자연스럽게 등장(메인>인터루드>미스터리>인연 우선순위).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UStoryDirectorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStoryDirectorComponent();

    // 날짜/시간대 변화 시 자동으로 다음 비트 체크 (끄면 수동 CheckForStory만)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    bool bAutoCheckOnDayChange = true;

    // 시간대 바뀔 때마다(아침/낮/저녁/밤) 체크할지, 날(Day) 바뀔 때만 체크할지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    bool bCheckEveryPhase = true;

    // 비트 준비 시 C++가 직접 스토리 위젯을 띄울지(true=BP 그래프 작업 불필요).
    // 끄면 OnStoryBeatReady 방송만 하고 표시는 BP가 담당.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    bool bAutoShowWidget = true;

    // 자동 표시에 쓸 스토리 위젯 클래스 (BP에서 WBP_Story 할당). 비면 표시 안 함(방송만).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    TSubclassOf<class UStoryWidget> StoryWidgetClass;

    // 지금 해금된 다음 비트가 있으면 OnStoryBeatReady 방송하고 true. (수동 트리거 — 시스템메뉴 "스토리" 버튼 등)
    UFUNCTION(BlueprintCallable, Category = "Story")
    bool CheckForStory();

    // 비트를 다 본 뒤 BP가 호출 → 완료 처리(보상/플래그/저장) + 곧바로 다음 비트 있으면 연쇄 방송
    UFUNCTION(BlueprintCallable, Category = "Story")
    void CompleteAndContinue(FName BeatId);

    UPROPERTY(BlueprintAssignable, Category = "Story")
    FOnStoryBeatReady OnStoryBeatReady;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    int32 LastCheckedDay = -1;

    UFUNCTION()
    void HandleTimeChanged(int32 Day, EDayPhase Phase);

    UTimeComponent* FindTimeComp() const;
    class UStoryManagerSubsystem* GetStory() const;
    int32 CurrentDay() const;
};
