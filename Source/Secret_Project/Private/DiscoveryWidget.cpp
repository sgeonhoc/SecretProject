#include "DiscoveryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "AreaTriggerActor.h"
#include "SecretSaveGame.h"
#include "UIRuntime.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UDiscoveryWidget* UDiscoveryWidget::OpenDiscovery(APlayerController* PC, TSubclassOf<UDiscoveryWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UDiscoveryWidget* W = CreateWidget<UDiscoveryWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UDiscoveryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UDiscoveryWidget::OnCloseClicked);
    Refresh();
}

void UDiscoveryWidget::Refresh()
{
    const TArray<FRegionInfo>& Regions = AAreaTriggerActor::GetRegisteredRegions();

    int32 Found = 0;
    if (List_Regions)
    {
        UUIRuntime::Clear(List_Regions);
        for (const FRegionInfo& R : Regions)
        {
            const bool bDiscovered = USecretSaveGame::IsCollected(R.Id);
            if (bDiscovered) ++Found;
            UVerticalBox* Card = UUIRuntime::AddCard(List_Regions, UIColor::Card, 6.f);
            UHorizontalBox* Row = UUIRuntime::AddRow(Card, 0.f);
            UUIRuntime::RowText(Row, bDiscovered ? FString::Printf(TEXT("✓  %s"), *R.Name) : TEXT("???  미발견 지역"),
                                bDiscovered ? UIColor::Title : UIColor::Dim, 18, 0, true);
            UUIRuntime::RowText(Row, bDiscovered ? TEXT("발견") : TEXT(""), UIColor::Good, 14, 2, false);
        }
        if (Regions.Num() == 0)
            UUIRuntime::AddText(List_Regions, TEXT("등록된 지역이 없습니다."), UIColor::Sub, 18, 1, 0.f);
        StaggerIntro(List_Regions);
    }

    if (Txt_Title)
        Txt_Title->SetText(FText::FromString(FString::Printf(TEXT("발견 지역   %d / %d"), Found, Regions.Num())));
}

void UDiscoveryWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
