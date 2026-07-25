#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "BestiaryWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class APlayerController;

/**
 * 적 도감 화면. UBestiarySubsystem(처치 수 + 발견 약점)을 순회해 표시.
 * 로직·바인딩 C++. WBP는 Txt_Title(헤더) + Txt_Entries(목록) + Btn_Close.
 * 패턴은 DiscoveryWidget과 동일(SystemMenu 허브에서 버튼으로 열기).
 */
UCLASS()
class SECRET_PROJECT_API UBestiaryWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Bestiary")
    static UBestiaryWidget* OpenBestiary(APlayerController* PC, TSubclassOf<UBestiaryWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Entries;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Bestiary;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

private:
    void Refresh();
    UFUNCTION() void OnCloseClicked();
};
