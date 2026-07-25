#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "EquipmentWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UEquipmentComponent;

/**
 * 장비 화면. 플레이어의 EquipmentComponent를 보여주고 장착/해제.
 * - 로직 C++, 레이아웃 BP.
 * - List_Items 컨테이너에 카탈로그 장비를 동적 클릭 카드로 채움(개수 무제한) → 카드 클릭 시 장착.
 */
UCLASS()
class SECRET_PROJECT_API UEquipmentWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Equip")
    static UEquipmentWidget* OpenEquipment(APlayerController* PC, TSubclassOf<UEquipmentWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Equipped;  // 현재 장착 요약
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;
    // 전용 UI: C++가 장비 카드를 동적 생성해 채우는 컨테이너
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Items;

private:
    void Refresh();
    UFUNCTION() void OnCloseClicked();
    UFUNCTION() void OnCardClicked(int32 Index);

    void EquipIndex(int32 Index);
    UEquipmentComponent* GetEquip() const;
};
