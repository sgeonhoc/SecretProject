#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "InventoryWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UInventoryComponent;
class APlayerController;

/**
 * 인벤토리 보기(가방). 보유 소비아이템 + 개수 표시. 로직·바인딩 전부 C++. WBP는 레이아웃만.
 * 최대 6칸(Txt_Item0~5). BindWidgetOptional이라 만든 것만 채워짐.
 */
UCLASS()
class SECRET_PROJECT_API UInventoryWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    static UInventoryWidget* OpenInventory(APlayerController* PC, TSubclassOf<UInventoryWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    // 전용 UI: C++가 아이템 카드를 동적 생성해 채우는 컨테이너
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Items;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Item0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Item1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Item2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Item3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Item4;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Item5;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

private:
    void Refresh();
    UInventoryComponent* GetPlayerInventory() const;

    UFUNCTION() void OnCloseClicked();
};
