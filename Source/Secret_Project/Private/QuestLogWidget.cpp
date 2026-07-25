#include "QuestLogWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "QuestComponent.h"
#include "UIRuntime.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UQuestLogWidget* UQuestLogWidget::OpenQuestLog(APlayerController* PC, TSubclassOf<UQuestLogWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UQuestLogWidget* W = CreateWidget<UQuestLogWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UQuestLogWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UQuestLogWidget::OnCloseClicked);
    if (Txt_Title) Txt_Title->SetText(FText::FromString(TEXT("퀘스트")));

    Refresh();
}

UQuestComponent* UQuestLogWidget::GetPlayerQuests() const
{
    if (APawn* P = GetOwningPlayerPawn())
        return P->FindComponentByClass<UQuestComponent>();
    return nullptr;
}

void UQuestLogWidget::Refresh()
{
    UQuestComponent* Quests = GetPlayerQuests();
    TArray<FQuestDef> Active;
    if (Quests) Quests->GetActiveQuests(Active);

    if (!List_Quests) return;
    UUIRuntime::Clear(List_Quests);

    if (Active.Num() == 0)
    {
        UUIRuntime::AddText(List_Quests, TEXT("진행중인 퀘스트가 없습니다."), UIColor::Sub, 18, 1, 0.f);
        return;
    }

    for (const FQuestDef& Q : Active)
    {
        UVerticalBox* Card = UUIRuntime::AddCard(List_Quests, UIColor::Card, 8.f);
        UUIRuntime::AddText(Card, Q.Title, UIColor::Title, 20, 0, 2.f);
        if (!Q.Description.IsEmpty())
            UUIRuntime::AddText(Card, Q.Description, UIColor::Sub, 14, 0, 4.f, true);

        if ((Q.Objective == EQuestObjective::OpenChests || Q.Objective == EQuestObjective::DefeatEnemies) && Quests)
        {
            const int32 Prog = Quests->GetProgress(Q.QuestId);
            const float Pct = (Q.TargetCount > 0) ? (float)Prog / (float)Q.TargetCount : 0.f;
            UUIRuntime::AddBar(Card, Pct, UIColor::Fill, 14.f, 3.f);
            UUIRuntime::AddText(Card, FString::Printf(TEXT("%d / %d"), Prog, Q.TargetCount), UIColor::Accent, 13, 2, 0.f);
        }
        else if (Q.Objective == EQuestObjective::ReachDay)
            UUIRuntime::AddText(Card, FString::Printf(TEXT("목표: %d일차까지"), Q.TargetCount), UIColor::Accent, 14, 0, 0.f);
        else if (Q.Objective == EQuestObjective::TalkToNPC && !Q.TargetNPCName.IsNone())
            UUIRuntime::AddText(Card, FString::Printf(TEXT("→ %s 와(과) 대화"), *Q.TargetNPCName.ToString()), UIColor::Accent, 14, 0, 0.f);
    }
    StaggerIntro(List_Quests);
}

void UQuestLogWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
