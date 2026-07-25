#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameOpeningDirector.generated.h"

class ACameraActor;
class APlayerController;
class SWidget;

/**
 * 게임의 진짜 시작 — 오프닝 연출을 끝까지 끌고 가는 연출가.
 *
 * 새 게임을 누른 뒤 첫 레벨(아랫장터 큰길, 밤)에 도착하면 GameMode가 이 배우를 하나 깔고,
 * 아래 순서대로 화면을 넘긴다. 각 칸의 길이는 아래 값들로 조절한다.
 *
 *   ①검은 화면 → ②프롤로그 두어 줄(세계가 어떤 꼴인지) → ③거리를 훑는 카메라
 *   → ④카메라가 플레이어 뒤로 내려앉음 → ⑤조작 인계 + 첫 목표 한 줄
 *
 * 연출 중에는 플레이어 입력을 막고, 끝나면 돌려준다. 아무 키나 누르면 건너뛴다.
 * UMG 위젯 에셋에 기대지 않고 Slate로 직접 그린다 — 에디터에서 위젯을 만들어 두지 않아도 곧바로 돈다.
 */
UCLASS()
class SECRET_PROJECT_API AGameOpeningDirector : public AActor
{
    GENERATED_BODY()

public:
    AGameOpeningDirector();

    virtual void Tick(float DeltaSeconds) override;

    // 연출 시작(GameMode가 호출). 대상 플레이어와 그 화면을 잡는다.
    void Begin(APlayerController* PC);

    // 지금 당장 끝내고 조작을 돌려준다(스킵/중단 공용)
    UFUNCTION(BlueprintCallable, Category = "Opening")
    void FinishNow();

    // 연출이 끝난 뒤에 이어질 것(도착 장면). GameMode가 걸어 둔다.
    FSimpleDelegate OnFinished;

    // ── 연출 값 (여기만 만지면 호흡이 바뀐다) ──────────
    // 검은 화면에서 프롤로그 첫 줄이 뜨기까지
    UPROPERTY(EditAnywhere, Category = "Opening|Timing") float BlackHold = 1.2f;
    // 프롤로그 한 줄이 떠 있는 시간(페이드 포함)
    UPROPERTY(EditAnywhere, Category = "Opening|Timing") float LineDuration = 3.4f;
    UPROPERTY(EditAnywhere, Category = "Opening|Timing") float LineFade = 0.9f;
    // 거리를 훑는 카메라 이동 시간
    UPROPERTY(EditAnywhere, Category = "Opening|Timing") float SweepDuration = 7.0f;
    // 카메라가 플레이어 뒤로 붙는 시간
    UPROPERTY(EditAnywhere, Category = "Opening|Timing") float HandoffBlend = 1.6f;
    // 첫 목표 한 줄이 화면에 머무는 시간
    UPROPERTY(EditAnywhere, Category = "Opening|Timing") float ObjectiveHold = 4.5f;

    // 프롤로그 문구 — 이 세계가 지금 어떤 꼴인지만 세우고 물러난다.
    UPROPERTY(EditAnywhere, Category = "Opening|Text") TArray<FText> PrologueLines;

    // 조작을 넘겨주며 띄우는 첫 목표
    UPROPERTY(EditAnywhere, Category = "Opening|Text") FText ObjectiveText;

    // 장소·때 표시 (프롤로그 끝에 뜨는 자막)
    UPROPERTY(EditAnywhere, Category = "Opening|Text") FText PlaceCardText;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    enum class EPhase : uint8 { Idle, Black, Prologue, Sweep, Handoff, Objective, Done };

    EPhase  Phase = EPhase::Idle;
    float   PhaseTime = 0.f;      // 현재 칸에서 흐른 시간
    int32   LineIndex = 0;        // 프롤로그 몇째 줄

    // 화면 위에 얹은 것들 (Slate — 0=완전 투명, 1=꽉 참)
    float   BlackAlpha = 1.f;     // 검은 막
    float   TextAlpha  = 0.f;     // 가운데 글
    float   ObjAlpha   = 0.f;     // 아래쪽 목표 한 줄
    FText   CurrentText;

    UPROPERTY() TObjectPtr<APlayerController> Player;
    UPROPERTY() TObjectPtr<ACameraActor> SweepCam;

    FVector  SweepStartLoc, SweepEndLoc;
    FRotator SweepStartRot, SweepEndRot;

    TSharedPtr<SWidget> Overlay;

    void BuildOverlay();
    void RemoveOverlay();
    void EnterPhase(EPhase Next);
    void SetupSweepCamera();
    void HandControlBackToPlayer();
};
