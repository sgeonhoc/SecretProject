#include "StatusWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "StatComponent.h"
#include "TimeComponent.h"
#include "QuestComponent.h"
#include "UIRuntime.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UStatusWidget* UStatusWidget::OpenStatus(APlayerController* PC, TSubclassOf<UStatusWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UStatusWidget* W = CreateWidget<UStatusWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UStatusWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UStatusWidget::OnCloseClicked);
    Refresh();
}

void UStatusWidget::Refresh()
{
    if (!List_Status) return;
    UUIRuntime::Clear(List_Status);

    APawn* P = GetOwningPlayerPawn();
    UStatComponent* St = P ? P->FindComponentByClass<UStatComponent>() : nullptr;
    if (!St)
    {
        UUIRuntime::AddText(List_Status, TEXT("플레이어 정보 없음"), UIColor::Sub, 18, 1, 0.f);
        return;
    }

    const FLinearColor HpCol(0.92f, 0.34f, 0.32f, 1.f), SpCol(0.35f, 0.62f, 0.95f, 1.f), XpCol(0.98f, 0.86f, 0.5f, 1.f);

    // 전투 능력 카드
    {
        UVerticalBox* C = UUIRuntime::AddCard(List_Status, UIColor::Card, 10.f);
        UHorizontalBox* H = UUIRuntime::AddRow(C, 6.f);
        UUIRuntime::RowText(H, FString::Printf(TEXT("Lv %d"), St->GetLevel()), UIColor::Title, 24, 0, true);
        UUIRuntime::RowText(H, FString::Printf(TEXT("%d G"), St->GetGold()), UIColor::Accent, 18, 2, false);

        UUIRuntime::AddText(C, FString::Printf(TEXT("HP   %.0f / %.0f"), St->GetCurrentHP(), St->GetMaxHP()), UIColor::Sub, 14, 0, 1.f);
        UUIRuntime::AddBar(C, St->GetMaxHP() > 0 ? St->GetCurrentHP() / St->GetMaxHP() : 0.f, HpCol, 14.f, 6.f);
        UUIRuntime::AddText(C, FString::Printf(TEXT("SP   %.0f / %.0f"), St->GetCurrentSP(), St->GetMaxSP()), UIColor::Sub, 14, 0, 1.f);
        UUIRuntime::AddBar(C, St->GetMaxSP() > 0 ? St->GetCurrentSP() / St->GetMaxSP() : 0.f, SpCol, 14.f, 6.f);
        UUIRuntime::AddText(C, FString::Printf(TEXT("EXP  %d / %d"), St->GetCurrentXP(), St->GetXPToNext()), UIColor::Sub, 14, 0, 1.f);
        UUIRuntime::AddBar(C, St->GetXPToNext() > 0 ? (float)St->GetCurrentXP() / St->GetXPToNext() : 1.f, XpCol, 12.f, 6.f);

        UHorizontalBox* H2 = UUIRuntime::AddRow(C, 0.f);
        UUIRuntime::RowText(H2, FString::Printf(TEXT("공격  %.0f"), St->GetAttack()), UIColor::Title, 18, 0, true);
        UUIRuntime::RowText(H2, FString::Printf(TEXT("방어  %.0f"), St->GetDefense()), UIColor::Title, 18, 0, true);
    }

    // 세부 스탯 카드 (시드된 캐릭터만)
    if (St->GetSTR() > 0.f || St->GetMAG() > 0.f)
    {
        UVerticalBox* C = UUIRuntime::AddCard(List_Status, UIColor::Card, 10.f);
        UUIRuntime::AddText(C, TEXT("세부 스탯"), UIColor::Accent, 18, 0, 4.f);
        const float Vals[5] = { St->GetSTR(), St->GetMAG(), St->GetVIT(), St->GetAGI(), St->GetLUK() };
        const TCHAR* Names[5] = { TEXT("힘 STR"), TEXT("마력 MAG"), TEXT("체력 VIT"), TEXT("민첩 AGI"), TEXT("운 LUK") };
        float Mx = 1.f; for (float V : Vals) Mx = FMath::Max(Mx, V);
        for (int32 i = 0; i < 5; ++i)
        {
            UHorizontalBox* R = UUIRuntime::AddRow(C, 2.f);
            UUIRuntime::RowText(R, Names[i], UIColor::Sub, 15, 0, true);
            UUIRuntime::RowText(R, FString::Printf(TEXT("%.0f"), Vals[i]), UIColor::Title, 15, 2, false);
            UUIRuntime::AddBar(C, Vals[i] / Mx, UIColor::Fill, 10.f, 5.f);
        }
    }

    // 정보 카드
    {
        UVerticalBox* C = UUIRuntime::AddCard(List_Status, UIColor::Card, 0.f);
        if (UTimeComponent* T = P->FindComponentByClass<UTimeComponent>())
            UUIRuntime::AddText(C, FString::Printf(TEXT("날짜   %s"), *T->GetTimeLabel()), UIColor::Sub, 15, 0, 3.f);
        if (UQuestComponent* Q = P->FindComponentByClass<UQuestComponent>())
            UUIRuntime::AddText(C, FString::Printf(TEXT("완료한 퀘스트   %d"), Q->GetCompletedCount()), UIColor::Sub, 15, 0, 0.f);
    }

    StaggerIntro(List_Status);
}

void UStatusWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
