#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "BondWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class APlayerController;

/**
 * NPC 인연(소셜링크) 화면. 플레이어 RelationshipComponent를 순회해 NPC별 랭크/진행 표시.
 * 로직·바인딩 C++. WBP는 Txt_Title(헤더) + Txt_Entries(목록) + Btn_Close.
 * 패턴은 BestiaryWidget/DiscoveryWidget과 동일(SystemMenu 허브에서 버튼으로 열기).
 */
UCLASS()
class SECRET_PROJECT_API UBondWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Bond")
    static UBondWidget* OpenBond(APlayerController* PC, TSubclassOf<UBondWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Entries;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Bonds;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

private:
    void Refresh();
    UFUNCTION() void OnCloseClicked();
};
