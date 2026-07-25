#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "DiscoveryWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class APlayerController;

/**
 * 발견 지역 도감. 배치된 AreaTrigger 레지스트리를 순회해 발견/미발견 표시.
 * 로직·바인딩 C++. WBP는 Txt_Regions(목록) + Txt_Title + Btn_Close.
 */
UCLASS()
class SECRET_PROJECT_API UDiscoveryWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Discovery")
    static UDiscoveryWidget* OpenDiscovery(APlayerController* PC, TSubclassOf<UDiscoveryWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Regions;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Regions;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

private:
    void Refresh();
    UFUNCTION() void OnCloseClicked();
};
