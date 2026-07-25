#include "SocialStatsWidget.h"
#include "SocialStatsComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "UIRuntime.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

static const FLinearColor SOC_CARD(0.12f, 0.14f, 0.20f, 0.96f);
static const FLinearColor SOC_NAME(0.97f, 0.97f, 1.0f, 1.f);
static const FLinearColor SOC_RANK(0.98f, 0.86f, 0.50f, 1.f);
static const FLinearColor SOC_SUB(0.72f, 0.76f, 0.84f, 1.f);
// 스탯별 게이지 색(지식/매력/용기/친절/숙련)
static const FLinearColor SOC_FILL[5] = {
    FLinearColor(0.35f, 0.62f, 0.95f, 1.f), FLinearColor(0.92f, 0.45f, 0.70f, 1.f),
    FLinearColor(0.93f, 0.35f, 0.30f, 1.f), FLinearColor(0.45f, 0.85f, 0.55f, 1.f),
    FLinearColor(0.85f, 0.70f, 0.35f, 1.f)
};

USocialStatsWidget* USocialStatsWidget::OpenSocialStats(APlayerController* PC, TSubclassOf<USocialStatsWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    USocialStatsWidget* W = CreateWidget<USocialStatsWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void USocialStatsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &USocialStatsWidget::OnCloseClicked);
    Refresh();
}

void USocialStatsWidget::Refresh()
{
    USocialStatsComponent* Social = nullptr;
    if (APlayerController* PC = GetOwningPlayer())
        if (APawn* Pawn = PC->GetPawn())
            Social = Pawn->FindComponentByClass<USocialStatsComponent>();

    int32 Maxed = 0;

    // 전용 UI: 스탯별 게이지 카드를 동적 생성
    if (List_Stats)
    {
        UUIRuntime::Clear(List_Stats);
        if (!Social)
        {
            UUIRuntime::AddText(List_Stats, TEXT("사회 스탯 정보를 찾을 수 없습니다."), SOC_SUB, 18, 1, 0.f);
        }
        else
        {
            for (int32 i = 0; i < USocialStatsComponent::StatCount; ++i)
            {
                const ESocialStat Stat = static_cast<ESocialStat>(i);
                const int32 Rank = Social->GetRank(Stat);
                if (Rank >= USocialStatsComponent::MaxRank) ++Maxed;
                int32 Into = 0, Needed = 0;
                Social->GetRankProgress(Stat, Into, Needed);
                const float Pct = (Needed > 0) ? (float)Into / (float)Needed : 1.f;

                UVerticalBox* Card = UUIRuntime::AddCard(List_Stats, SOC_CARD, 8.f);
                UHorizontalBox* Row = UUIRuntime::AddRow(Card, 4.f);
                UUIRuntime::RowText(Row, LexSocialStat(Stat), SOC_NAME, 20, 0, true);
                UUIRuntime::RowText(Row, FString::Printf(TEXT("Rank %d / %d"), Rank, USocialStatsComponent::MaxRank),
                                    SOC_RANK, 18, 2, false);
                UUIRuntime::AddBar(Card, Pct, SOC_FILL[i], 16.f, 4.f);
                const FString Sub = (Rank >= USocialStatsComponent::MaxRank)
                    ? TEXT("최대 랭크 달성")
                    : FString::Printf(TEXT("%d / %d"), Into, Needed);
                UUIRuntime::AddText(Card, Sub, SOC_SUB, 13, 2, 0.f);
            }
            StaggerIntro(List_Stats);   // 스탯 카드 순차 등장 연출
        }
    }

    if (Txt_Title)
        Txt_Title->SetText(FText::FromString(
            FString::Printf(TEXT("사회 스탯   (최대치 %d/%d)"), Maxed, USocialStatsComponent::StatCount)));
}

void USocialStatsWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
