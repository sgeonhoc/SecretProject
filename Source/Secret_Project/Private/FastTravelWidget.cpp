#include "FastTravelWidget.h"
#include "UIRuntime.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "FastTravelPointActor.h"
#include "SecretSaveGame.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UFastTravelWidget* UFastTravelWidget::OpenFastTravel(APlayerController* PC, TSubclassOf<UFastTravelWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UFastTravelWidget* W = CreateWidget<UFastTravelWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UFastTravelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UFastTravelWidget::OnCloseClicked);

    Refresh();
}

void UFastTravelWidget::Refresh()
{
    DestLocations.Reset();

    const FName CurLevel(*UGameplayStatics::GetCurrentLevelName(this, true));
    APawn* Pawn = GetOwningPlayer() ? GetOwningPlayer()->GetPawn() : nullptr;
    const FVector PlayerLoc = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;

    if (List_Dests) UUIRuntime::Clear(List_Dests);

    int32 Discovered = 0;
    for (const FFastTravelPoint& P : AFastTravelPointActor::GetRegisteredPoints())
    {
        // 같은 레벨 + 발견한 지점만
        if (P.LevelName != CurLevel) continue;
        if (!USecretSaveGame::IsCollected(P.Id)) continue;
        ++Discovered;

        const int32 Index = DestLocations.Add(P.Location);

        if (List_Dests)
        {
            UCardButton* Btn = nullptr;
            UVerticalBox* Inner = UUIRuntime::AddClickCard(List_Dests, UIColor::Card, Index, Btn);
            if (Inner)
            {
                const float Dist = Pawn ? FVector::Dist(PlayerLoc, P.Location) : 0.f;
                UHorizontalBox* Row = UUIRuntime::AddRow(Inner, 0.f);
                UUIRuntime::RowText(Row, P.Name, UIColor::Title, 18, ETextJustify::Left, true);
                UUIRuntime::RowText(Row, FString::Printf(TEXT("%.0fm"), Dist / 100.f),
                                    UIColor::Sub, 14, ETextJustify::Right, false);
                if (Btn) Btn->OnCardClicked.AddDynamic(this, &UFastTravelWidget::OnCardClicked);
            }
        }
    }

    if (Txt_Title)
    {
        const FString Title = (Discovered == 0)
            ? FString(TEXT("빠른 이동 — 발견한 지점이 없습니다"))
            : FString::Printf(TEXT("빠른 이동 — 목적지 %d곳"), DestLocations.Num());
        Txt_Title->SetText(FText::FromString(Title));
    }
}

void UFastTravelWidget::TravelToIndex(int32 Index)
{
    if (!DestLocations.IsValidIndex(Index)) return;

    if (APlayerController* PC = GetOwningPlayer())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            // 바닥에 살짝 띄워 텔레포트(충돌 끼임 방지)
            const FVector Target = DestLocations[Index] + FVector(0.f, 0.f, 50.f);
            Pawn->SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
        }
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}

void UFastTravelWidget::OnCardClicked(int32 Index) { TravelToIndex(Index); }

void UFastTravelWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
