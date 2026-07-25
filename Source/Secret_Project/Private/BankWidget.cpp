#include "BankWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "BankComponent.h"
#include "StatComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UBankWidget* UBankWidget::OpenBank(APlayerController* PC, TSubclassOf<UBankWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UBankWidget* W = CreateWidget<UBankWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

UBankComponent* UBankWidget::GetBank() const
{
    if (APlayerController* PC = GetOwningPlayer())
        if (APawn* Pawn = PC->GetPawn())
            return Pawn->FindComponentByClass<UBankComponent>();
    return nullptr;
}

UStatComponent* UBankWidget::GetStat() const
{
    if (APlayerController* PC = GetOwningPlayer())
        if (APawn* Pawn = PC->GetPawn())
            return Pawn->FindComponentByClass<UStatComponent>();
    return nullptr;
}

void UBankWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Deposit100)  Btn_Deposit100->OnClicked.AddDynamic(this, &UBankWidget::OnDeposit100);
    if (Btn_DepositAll)  Btn_DepositAll->OnClicked.AddDynamic(this, &UBankWidget::OnDepositAll);
    if (Btn_Withdraw100) Btn_Withdraw100->OnClicked.AddDynamic(this, &UBankWidget::OnWithdraw100);
    if (Btn_WithdrawAll) Btn_WithdrawAll->OnClicked.AddDynamic(this, &UBankWidget::OnWithdrawAll);
    if (Btn_Close)       Btn_Close->OnClicked.AddDynamic(this, &UBankWidget::OnCloseClicked);

    Refresh();
}

void UBankWidget::Refresh()
{
    const int32 OnHand = GetStat() ? GetStat()->GetGold() : 0;
    const int32 Stored = GetBank() ? GetBank()->GetStoredGold() : 0;

    if (Txt_OnHand) Txt_OnHand->SetText(FText::FromString(FString::Printf(TEXT("보유: %d G"), OnHand)));
    if (Txt_Stored) Txt_Stored->SetText(FText::FromString(FString::Printf(TEXT("예치금: %d G"), Stored)));
}

void UBankWidget::OnDeposit100()
{
    if (UBankComponent* Bank = GetBank()) Bank->Deposit(100);
    Refresh();
}

void UBankWidget::OnDepositAll()
{
    if (UBankComponent* Bank = GetBank())
        if (UStatComponent* Stat = GetStat())
            Bank->Deposit(Stat->GetGold());
    Refresh();
}

void UBankWidget::OnWithdraw100()
{
    if (UBankComponent* Bank = GetBank()) Bank->Withdraw(100);
    Refresh();
}

void UBankWidget::OnWithdrawAll()
{
    if (UBankComponent* Bank = GetBank()) Bank->Withdraw(Bank->GetStoredGold());
    Refresh();
}

void UBankWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
