#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFlowSubsystem.h"   // FStoryLine
#include "StorySceneDirector.generated.h"

class SOverlay;
class APlayerController;

/**
 * 장면 재생기 — 스토리가 "진행되는" 것이 화면에 실제로 보이게 하는 조각.
 *
 * 한 장면 = ①때·자리 자막 카드(검은 화면, "그날 밤 — 셋집 옥상") → ②대사 줄줄이.
 * 대사는 말한 사람 이름표 + 한 글자씩 드러나는 글. Space/좌클릭이 한 줄을 건너뛴다.
 *
 * 위젯 에셋(WBP)에 기대지 않는다 — Slate로 직접 그린다. 그래서 레벨·UI 담당이
 * 무엇을 저장하든 이 장면은 그대로 돈다.
 *
 * 쓰는 법:
 *   AStorySceneDirector::Play(World, PC, CardText, Lines, FSimpleDelegate::CreateUObject(this, &X::OnSceneDone));
 */
UCLASS()
class SECRET_PROJECT_API AStorySceneDirector : public AActor
{
    GENERATED_BODY()

public:
    AStorySceneDirector();

    /** 장면 하나를 튼다. 끝나면 OnDone이 불리고 이 액터는 스스로 사라진다. */
    static AStorySceneDirector* Play(UWorld* World, APlayerController* PC,
        const FText& Card, const TArray<FStoryLine>& Lines, FSimpleDelegate OnDone);

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    void BuildOverlay();
    void RemoveOverlay();
    void NextLine();
    void Finish();

    enum class EPhase : uint8 { Card, CardOut, Line, Done };

    UPROPERTY() APlayerController* Player = nullptr;
    UPROPERTY() TArray<FStoryLine> Lines;
    FText CardText;
    FSimpleDelegate OnFinished;

    TSharedPtr<SOverlay> Overlay;

    EPhase Phase = EPhase::Card;
    float PhaseTime = 0.f;
    int32 LineIndex = -1;

    // 한 글자씩 드러나는 진행도(0~1). 다 드러난 뒤에 Space를 누르면 다음 줄.
    float Revealed = 0.f;
    FString FullLine;
    FString SpeakerName;

    // 화면에 그려질 값들 — Slate 람다가 매 프레임 읽어 간다.
    float BlackAlpha = 1.f;     // 자막 카드용 검은 막
    float BoxAlpha = 0.f;       // 대사 상자
    float LetterboxAlpha = 0.f; // 위아래 띠
    FText VisibleText;          // 드러난 만큼의 대사
    FText VisibleSpeaker;
    FText CardVisible;

    // 조율 값 (초)
    static constexpr float CardHold = 2.2f;    // 자막 카드가 머무는 시간
    static constexpr float CardFade = 0.8f;    // 카드가 걷히는 시간
    static constexpr float CharsPerSec = 38.f; // 글자가 드러나는 속도
    static constexpr float AutoHold = 1.2f;    // 다 드러난 뒤 저절로 넘어가기까지(입력 없을 때는 안 넘어감)
};
