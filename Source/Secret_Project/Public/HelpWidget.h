#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "HelpWidget.generated.h"

class UButton;
class UTextBlock;
class APlayerController;

/**
 * 조작/도움말 안내. 기본 조작 설명을 표시(BP에서 Txt_Help 비우면 C++ 기본문구 사용).
 * 로직·바인딩 C++. WBP는 Txt_Help + Btn_Close.
 */
UCLASS()
class SECRET_PROJECT_API UHelpWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Help")
    static UHelpWidget* OpenHelp(APlayerController* PC, TSubclassOf<UHelpWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Help;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

private:
    UFUNCTION() void OnCloseClicked();
};
