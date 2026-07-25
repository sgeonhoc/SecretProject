#include "BondWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "RelationshipComponent.h"
#include "UIRuntime.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UBondWidget* UBondWidget::OpenBond(APlayerController* PC, TSubclassOf<UBondWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UBondWidget* W = CreateWidget<UBondWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UBondWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UBondWidget::OnCloseClicked);
    Refresh();
}

void UBondWidget::Refresh()
{
    URelationshipComponent* Rel = nullptr;
    if (APlayerController* PC = GetOwningPlayer())
        if (APawn* Pawn = PC->GetPawn())
            Rel = Pawn->FindComponentByClass<URelationshipComponent>();

    int32 Count = 0, Maxed = 0;
    if (List_Bonds)
    {
        UUIRuntime::Clear(List_Bonds);
        TArray<FRelationshipRecord> Records;
        if (Rel) Rel->GetAllRelationships(Records);

        if (Records.Num() == 0)
        {
            UUIRuntime::AddText(List_Bonds, TEXT("아직 인연을 맺은 상대가 없습니다."), UIColor::Sub, 18, 1, 2.f);
            UUIRuntime::AddText(List_Bonds, TEXT("NPC와 대화해 인연을 쌓아보세요."), UIColor::Dim, 14, 1, 0.f);
        }
        else
        {
            const FLinearColor BondFill(0.92f, 0.45f, 0.70f, 1.f); // 인연=분홍
            for (const FRelationshipRecord& R : Records)
            {
                ++Count;
                const bool bMax = (R.Rank >= URelationshipComponent::MaxRank);
                if (bMax) ++Maxed;
                int32 Into = 0, Needed = URelationshipComponent::PointsPerRank;
                Rel->GetRankProgress(R.NPCName, Into, Needed);
                const float Pct = bMax ? 1.f : ((Needed > 0) ? (float)Into / (float)Needed : 0.f);

                UVerticalBox* Card = UUIRuntime::AddCard(List_Bonds, UIColor::Card, 8.f);
                UHorizontalBox* Row = UUIRuntime::AddRow(Card, 4.f);
                UUIRuntime::RowText(Row, R.NPCName.ToString(), UIColor::Title, 20, 0, true);
                UUIRuntime::RowText(Row, bMax ? TEXT("MAX") : FString::Printf(TEXT("Rank %d"), R.Rank),
                                    UIColor::Accent, 18, 2, false);
                UUIRuntime::AddBar(Card, Pct, BondFill, 16.f, 2.f);
                UUIRuntime::AddText(Card, bMax ? TEXT("최고의 인연") : FString::Printf(TEXT("%d / %d"), Into, Needed),
                                    UIColor::Sub, 13, 2, 0.f);
            }
        }
        StaggerIntro(List_Bonds);
    }

    if (Txt_Title)
        Txt_Title->SetText(FText::FromString(
            FString::Printf(TEXT("인연   %d명   (최고 인연 %d)"), Count, Maxed)));
}

void UBondWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
