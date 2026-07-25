#include "StoryJournalWidget.h"
#include "StoryManager.h"
#include "StoryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

static UStoryManagerSubsystem* GetStorySubsystem(const UObject* Ctx)
{
    if (Ctx)
        if (UWorld* W = Ctx->GetWorld())
            if (UGameInstance* GI = W->GetGameInstance())
                return GI->GetSubsystem<UStoryManagerSubsystem>();
    return nullptr;
}

UStoryJournalWidget* UStoryJournalWidget::OpenJournal(APlayerController* PC, TSubclassOf<UStoryJournalWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UStoryJournalWidget* W = CreateWidget<UStoryJournalWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UStoryJournalWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UStoryJournalWidget::OnCloseClicked);
    if (Btn_Beat0) Btn_Beat0->OnClicked.AddDynamic(this, &UStoryJournalWidget::OnBeat0);
    if (Btn_Beat1) Btn_Beat1->OnClicked.AddDynamic(this, &UStoryJournalWidget::OnBeat1);
    if (Btn_Beat2) Btn_Beat2->OnClicked.AddDynamic(this, &UStoryJournalWidget::OnBeat2);
    if (Btn_Beat3) Btn_Beat3->OnClicked.AddDynamic(this, &UStoryJournalWidget::OnBeat3);
    if (Btn_Beat4) Btn_Beat4->OnClicked.AddDynamic(this, &UStoryJournalWidget::OnBeat4);
    if (Btn_Beat5) Btn_Beat5->OnClicked.AddDynamic(this, &UStoryJournalWidget::OnBeat5);
    if (Btn_Beat6) Btn_Beat6->OnClicked.AddDynamic(this, &UStoryJournalWidget::OnBeat6);
    if (Btn_Beat7) Btn_Beat7->OnClicked.AddDynamic(this, &UStoryJournalWidget::OnBeat7);

    Refresh();
}

void UStoryJournalWidget::Refresh()
{
    UStoryManagerSubsystem* Story = GetStorySubsystem(this);

    TArray<FName> Done;
    int32 Total = 0;
    if (Story)
    {
        Done = Story->GetCompletedBeatIds();   // 카탈로그 순
        Total = Story->GetTotalBeatCount();
    }

    if (Txt_Title)
        Txt_Title->SetText(FText::FromString(
            FString::Printf(TEXT("회상 저널   %d / %d"), Done.Num(), Total)));

    // 본 장면 목록(전체) + 다시보기 버튼 매핑(앞에서부터 최대 8개)
    ReplayIds.Reset();
    FString List;
    int32 N = 0;
    for (const FName& Id : Done)
    {
        FStoryBeat B;
        const FString Title = UStoryManagerSubsystem::FindBeat(Id, B) ? B.Title : Id.ToString();
        List += FString::Printf(TEXT("%d. %s\n"), ++N, *Title);
        if (ReplayIds.Num() < 8) ReplayIds.Add(Id);
    }
    if (Done.Num() == 0)
        List = TEXT("아직 본 장면이 없습니다.\n이야기를 진행하면 여기에 모입니다.");
    if (Txt_Entries) Txt_Entries->SetText(FText::FromString(List));

    // 다시보기 버튼 라벨/표시
    UButton* Btns[8] = { Btn_Beat0, Btn_Beat1, Btn_Beat2, Btn_Beat3, Btn_Beat4, Btn_Beat5, Btn_Beat6, Btn_Beat7 };
    UTextBlock* Txts[8] = { Txt_Beat0, Txt_Beat1, Txt_Beat2, Txt_Beat3, Txt_Beat4, Txt_Beat5, Txt_Beat6, Txt_Beat7 };
    for (int32 i = 0; i < 8; ++i)
    {
        if (!Btns[i]) continue;
        if (ReplayIds.IsValidIndex(i))
        {
            Btns[i]->SetVisibility(ESlateVisibility::Visible);
            FStoryBeat B;
            const FString Title = UStoryManagerSubsystem::FindBeat(ReplayIds[i], B) ? B.Title : ReplayIds[i].ToString();
            if (Txts[i]) Txts[i]->SetText(FText::FromString(Title));
        }
        else
        {
            Btns[i]->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UStoryJournalWidget::Replay(int32 ButtonIndex)
{
    if (!ReplayIds.IsValidIndex(ButtonIndex)) return;
    FStoryBeat B;
    if (!UStoryManagerSubsystem::FindBeat(ReplayIds[ButtonIndex], B)) return;

    APlayerController* PC = GetOwningPlayer();
    if (PC && StoryWidgetClass)
        // Director=nullptr → 다 봐도 CompleteAndContinue 호출 안 함(보상/플래그 재처리 없이 다시보기만)
        UStoryWidget::ShowBeat(PC, StoryWidgetClass, B, nullptr);

    // 저널은 닫음(스토리 위젯이 위에 뜨고, 닫히면 게임으로 복귀 — SystemMenu가 하위위젯 여는 패턴과 동일)
    RemoveFromParent();
}

void UStoryJournalWidget::CloseAndRestoreInput()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}

void UStoryJournalWidget::OnCloseClicked() { CloseAndRestoreInput(); }

void UStoryJournalWidget::OnBeat0() { Replay(0); }
void UStoryJournalWidget::OnBeat1() { Replay(1); }
void UStoryJournalWidget::OnBeat2() { Replay(2); }
void UStoryJournalWidget::OnBeat3() { Replay(3); }
void UStoryJournalWidget::OnBeat4() { Replay(4); }
void UStoryJournalWidget::OnBeat5() { Replay(5); }
void UStoryJournalWidget::OnBeat6() { Replay(6); }
void UStoryJournalWidget::OnBeat7() { Replay(7); }
