#include "AchievementWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "AchievementComponent.h"
#include "UIRuntime.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UAchievementWidget* UAchievementWidget::OpenAchievements(APlayerController* PC, TSubclassOf<UAchievementWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UAchievementWidget* W = CreateWidget<UAchievementWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UAchievementWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UAchievementWidget::OnCloseClicked);
    Refresh();
}

void UAchievementWidget::Refresh()
{
    UAchievementComponent* Ach = nullptr;
    if (APlayerController* PC = GetOwningPlayer())
        if (APawn* Pawn = PC->GetPawn())
            Ach = Pawn->FindComponentByClass<UAchievementComponent>();

    if (Ach) Ach->CheckAll(); // 열 때 최신화

    int32 Unlocked = 0, Total = 0;
    if (List_Achievements)
    {
        UUIRuntime::Clear(List_Achievements);
        TArray<FAchievementStatus> Statuses;
        if (Ach) Ach->GetStatuses(Statuses);
        Total = Statuses.Num();
        for (const FAchievementStatus& S : Statuses)
        {
            if (S.bUnlocked) ++Unlocked;
            UVerticalBox* Card = UUIRuntime::AddCard(List_Achievements, UIColor::Card, 8.f);
            UHorizontalBox* Row = UUIRuntime::AddRow(Card, 4.f);
            UUIRuntime::RowText(Row, FString::Printf(TEXT("%s %s"), S.bUnlocked ? TEXT("★") : TEXT("☆"), *S.Title),
                                S.bUnlocked ? UIColor::Accent : UIColor::Title, 19, 0, true);
            UUIRuntime::RowText(Row, S.bUnlocked ? TEXT("달성") : FString::Printf(TEXT("%d/%d"), S.Current, S.Threshold),
                                S.bUnlocked ? UIColor::Good : UIColor::Sub, 15, 2, false);
            if (!S.bUnlocked && S.Threshold > 0)
                UUIRuntime::AddBar(Card, (float)S.Current / (float)S.Threshold, UIColor::Fill, 12.f, 3.f);
            if (!S.Description.IsEmpty())
                UUIRuntime::AddText(Card, S.Description, UIColor::Sub, 14, 0, 0.f, true);
            if (!S.Reward.IsEmpty())
                UUIRuntime::AddText(Card, FString::Printf(TEXT("보상: %s"), *S.Reward), UIColor::Accent, 13, 0, 0.f);
        }
        if (Total == 0)
            UUIRuntime::AddText(List_Achievements, TEXT("업적 데이터가 없습니다."), UIColor::Sub, 18, 1, 0.f);
        StaggerIntro(List_Achievements);
    }

    if (Txt_Title)
        Txt_Title->SetText(FText::FromString(FString::Printf(TEXT("도전과제   %d / %d"), Unlocked, Total)));
}

void UAchievementWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
