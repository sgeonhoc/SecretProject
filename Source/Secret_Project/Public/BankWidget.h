#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "BankWidget.generated.h"

class UButton;
class UTextBlock;
class APlayerController;
class UBankComponent;
class UStatComponent;

/**
 * 은행 화면. 보유 골드 ↔ 예치금 예치/인출. 일일 이자는 BankComponent가 시간 경과로 자동 처리.
 * 로직·바인딩 C++. WBP는 Txt_OnHand/Txt_Stored + Btn_Deposit100/DepositAll/Withdraw100/WithdrawAll + Btn_Close.
 */
UCLASS()
class SECRET_PROJECT_API UBankWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Bank")
    static UBankWidget* OpenBank(APlayerController* PC, TSubclassOf<UBankWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_OnHand;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Stored;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Deposit100;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_DepositAll;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Withdraw100;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_WithdrawAll;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

private:
    void Refresh();
    UBankComponent* GetBank() const;
    UStatComponent* GetStat() const;

    UFUNCTION() void OnDeposit100();
    UFUNCTION() void OnDepositAll();
    UFUNCTION() void OnWithdraw100();
    UFUNCTION() void OnWithdrawAll();
    UFUNCTION() void OnCloseClicked();
};
