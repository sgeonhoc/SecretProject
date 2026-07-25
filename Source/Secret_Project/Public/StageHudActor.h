#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageHudActor.generated.h"

class SOverlay;

/**
 * 진행 표시줄 — "지금 무엇을 하러 가는 중인가"를 화면 왼쪽 위에 늘 띄운다.
 *
 * 오프닝의 목표 한 줄은 몇 초 뒤 사라지므로, 걷다가 길을 잃으면 되돌아볼 데가 없었다.
 * 여기는 장면이 아니라 상시 표시다: 장(章)·자리 이름 · 목표 한 줄 · 목표까지 남은 걸음.
 * 목표가 없는 스테이지(대화만 있는 자리)에서는 목표 줄이 저절로 숨는다.
 */
UCLASS()
class SECRET_PROJECT_API AStageHudActor : public AActor
{
    GENERATED_BODY()

public:
    AStageHudActor();

    virtual void Tick(float DeltaSeconds) override;

    /** 스테이지가 바뀔 때 게임모드가 불러 준다. Location이 유효하면 남은 걸음을 같이 띄운다. */
    void SetObjective(const FText& StageName, const FText& Objective, bool bHasSpot, const FVector& Spot);

    /** 장면 재생 중에는 숨긴다(대사 상자와 겹치지 않게). */
    void SetHidden(bool bHide);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    void BuildOverlay();
    void RemoveOverlay();

    TSharedPtr<SOverlay> Overlay;

    FText StageText;
    FText ObjectiveText;
    FText DistanceText;

    bool bHasObjectiveSpot = false;
    FVector ObjectiveSpot = FVector::ZeroVector;

    float Alpha = 0.f;        // 나타남/사라짐
    float FlashTime = 99.f;   // 새 목표가 들어올 때 잠깐 밝아짐
    bool bHiddenNow = false;
};
