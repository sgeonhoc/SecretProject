#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "AchievementWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class APlayerController;

/**
 * 도전과제/업적 화면. 플레이어 AchievementComponent의 상태(해금/진행)를 목록으로 표시.
 * 열 때 재평가(CheckAll). 로직·바인딩 C++. WBP는 Txt_Title + Txt_Entries + Btn_Close.
 */
UCLASS()
class SECRET_PROJECT_API UAchievementWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Achievement")
    static UAchievementWidget* OpenAchievements(APlayerController* PC, TSubclassOf<UAchievementWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Entries;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Achievements;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

private:
    void Refresh();
    UFUNCTION() void OnCloseClicked();
};
