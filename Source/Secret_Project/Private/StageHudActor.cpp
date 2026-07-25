#include "StageHudActor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

AStageHudActor::AStageHudActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AStageHudActor::BeginPlay()
{
    Super::BeginPlay();
    BuildOverlay();
}

void AStageHudActor::BuildOverlay()
{
    if (Overlay.IsValid() || !GEngine || !GEngine->GameViewport) return;

    FSlateFontInfo StageFont = FCoreStyle::GetDefaultFontStyle("Regular", 15);
    FSlateFontInfo ObjFont   = FCoreStyle::GetDefaultFontStyle("Regular", 20);
    FSlateFontInfo DistFont  = FCoreStyle::GetDefaultFontStyle("Regular", 14);
    if (UObject* Roboto = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto")))
    {
        StageFont = FSlateFontInfo(Roboto, 15);
        ObjFont   = FSlateFontInfo(Roboto, 20);
        DistFont  = FSlateFontInfo(Roboto, 14);
    }

    TWeakObjectPtr<AStageHudActor> Self(this);
    auto StageTint = [Self]()
    {
        const float A = Self.IsValid() ? Self->Alpha : 0.f;
        return FSlateColor(FLinearColor(0.72f, 0.70f, 0.66f, A * 0.85f));
    };
    auto ObjTint = [Self]()
    {
        // 새 목표가 들어온 직후엔 밝게 — 바뀐 걸 놓치지 않게.
        const float A = Self.IsValid() ? Self->Alpha : 0.f;
        const float F = (Self.IsValid() && Self->FlashTime < 1.6f)
            ? (0.5f + 0.5f * FMath::Cos(Self->FlashTime * 9.f)) : 0.f;
        const FLinearColor Base(0.98f, 0.84f, 0.42f, 1.f);
        const FLinearColor Hi(1.0f, 0.98f, 0.88f, 1.f);
        FLinearColor C = FMath::Lerp(Base, Hi, F);
        C.A = A;
        return FSlateColor(C);
    };
    auto DistTint = [Self]()
    {
        const float A = Self.IsValid() ? Self->Alpha : 0.f;
        return FSlateColor(FLinearColor(0.66f, 0.72f, 0.78f, A * 0.9f));
    };

    Overlay = SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(34, 26, 0, 0))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock).Font(StageFont).ColorAndOpacity_Lambda(StageTint)
                .Text_Lambda([Self]() { return Self.IsValid() ? Self->StageText : FText::GetEmpty(); })
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 4, 0, 0))
            [
                SNew(STextBlock).Font(ObjFont).ColorAndOpacity_Lambda(ObjTint)
                .Text_Lambda([Self]() { return Self.IsValid() ? Self->ObjectiveText : FText::GetEmpty(); })
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 3, 0, 0))
            [
                SNew(STextBlock).Font(DistFont).ColorAndOpacity_Lambda(DistTint)
                .Text_Lambda([Self]() { return Self.IsValid() ? Self->DistanceText : FText::GetEmpty(); })
            ]
        ];

    // ZOrder 4 — 장면 대사(6)와 오프닝(5)보다 아래. 장면이 뜨면 그 위에 덮인다.
    GEngine->GameViewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 4);
}

void AStageHudActor::RemoveOverlay()
{
    if (Overlay.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
    Overlay.Reset();
}

void AStageHudActor::SetObjective(const FText& InStageName, const FText& InObjective, bool bInHasSpot, const FVector& InSpot)
{
    StageText = InStageName;
    ObjectiveText = InObjective;
    bHasObjectiveSpot = bInHasSpot;
    ObjectiveSpot = InSpot;
    DistanceText = FText::GetEmpty();
    FlashTime = 0.f;
}

void AStageHudActor::SetHidden(bool bHide)
{
    bHiddenNow = bHide;
}

void AStageHudActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    FlashTime += DeltaSeconds;

    const float Target = (bHiddenNow || ObjectiveText.IsEmpty()) ? 0.f : 1.f;
    Alpha = FMath::FInterpTo(Alpha, Target, DeltaSeconds, 6.f);

    // 남은 걸음 — 목표가 어디 있는지 감이 오게. 1m = 100유닛.
    if (bHasObjectiveSpot && Alpha > 0.01f)
    {
        if (UWorld* W = GetWorld())
        {
            if (APlayerController* PC = W->GetFirstPlayerController())
            {
                if (APawn* P = PC->GetPawn())
                {
                    const FVector D = ObjectiveSpot - P->GetActorLocation();
                    const float Meters = D.Size2D() / 100.f;

                    // 어느 쪽인지 — 보고 있는 방향 기준 여덟 방위 화살표.
                    const FVector Fwd = PC->GetControlRotation().Vector().GetSafeNormal2D();
                    const FVector Dir = D.GetSafeNormal2D();
                    const float Ang = FMath::RadiansToDegrees(FMath::Atan2(
                        FVector::CrossProduct(Fwd, Dir).Z, FVector::DotProduct(Fwd, Dir)));
                    const TCHAR* Arrow = TEXT("↑");
                    const float A = FMath::Abs(Ang);
                    if (A > 157.5f)      Arrow = TEXT("↓");
                    else if (A > 112.5f) Arrow = (Ang > 0) ? TEXT("↘") : TEXT("↙");
                    else if (A > 67.5f)  Arrow = (Ang > 0) ? TEXT("→") : TEXT("←");
                    else if (A > 22.5f)  Arrow = (Ang > 0) ? TEXT("↗") : TEXT("↖");

                    DistanceText = FText::FromString(FString::Printf(TEXT("%s  %.0f m"), Arrow, Meters));
                }
            }
        }
    }
    else if (!bHasObjectiveSpot)
    {
        DistanceText = FText::GetEmpty();
    }
}

void AStageHudActor::EndPlay(const EEndPlayReason::Type Reason)
{
    RemoveOverlay();
    Super::EndPlay(Reason);
}
