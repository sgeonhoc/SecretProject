#include "BestiaryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "BestiarySubsystem.h"
#include "BattleTypes.h"
#include "UIRuntime.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UBestiaryWidget* UBestiaryWidget::OpenBestiary(APlayerController* PC, TSubclassOf<UBestiaryWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UBestiaryWidget* W = CreateWidget<UBestiaryWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UBestiaryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UBestiaryWidget::OnCloseClicked);
    Refresh();
}

void UBestiaryWidget::Refresh()
{
    UBestiarySubsystem* Bst = nullptr;
    if (APlayerController* PC = GetOwningPlayer())
        if (UGameInstance* GI = PC->GetGameInstance())
            Bst = GI->GetSubsystem<UBestiarySubsystem>();

    int32 Count = 0;
    if (List_Bestiary)
    {
        UUIRuntime::Clear(List_Bestiary);
        if (Bst)
        {
            for (const TPair<FName, FBestiaryEntry>& Pair : Bst->GetAll())
            {
                ++Count;
                const FBestiaryEntry& E = Pair.Value;

                FString Weak;
                for (int32 i = 0; i < E.KnownWeak.Num(); ++i)
                {
                    if (i > 0) Weak += TEXT(", ");
                    Weak += LexBattleElement(E.KnownWeak[i]);
                }
                const bool bKnown = !Weak.IsEmpty();

                UVerticalBox* Card = UUIRuntime::AddCard(List_Bestiary, UIColor::Card, 8.f);
                UHorizontalBox* Row = UUIRuntime::AddRow(Card, 4.f);
                UUIRuntime::RowText(Row, Pair.Key.ToString(), UIColor::Title, 20, 0, true);
                UUIRuntime::RowText(Row, FString::Printf(TEXT("처치 %d"), E.DefeatedCount), UIColor::Sub, 16, 2, false);
                UUIRuntime::AddText(Card, FString::Printf(TEXT("약점: %s"), bKnown ? *Weak : TEXT("??? (분석 필요)")),
                                    bKnown ? UIColor::Accent : UIColor::Dim, 15, 0, 0.f);
            }
        }
        if (Count == 0)
            UUIRuntime::AddText(List_Bestiary, TEXT("아직 조우한 적이 없습니다."), UIColor::Sub, 18, 1, 0.f);
        StaggerIntro(List_Bestiary);
    }

    if (Txt_Title)
        Txt_Title->SetText(FText::FromString(FString::Printf(TEXT("적 도감   %d종"), Count)));
}

void UBestiaryWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
