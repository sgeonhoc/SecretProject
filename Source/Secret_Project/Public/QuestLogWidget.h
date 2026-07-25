#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "QuestLogWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UQuestComponent;
class APlayerController;

/**
 * 퀘스트 로그(진행중 퀘스트 목록). 로직·바인딩 전부 C++. WBP는 레이아웃만.
 * 최대 4개 진행중 퀘스트를 Txt_Quest0~3에 표시. BindWidgetOptional이라 만든 것만 채워짐.
 */
UCLASS()
class SECRET_PROJECT_API UQuestLogWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Quest")
    static UQuestLogWidget* OpenQuestLog(APlayerController* PC, TSubclassOf<UQuestLogWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Quests;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Quest0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Quest1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Quest2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Quest3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

private:
    void Refresh();
    UQuestComponent* GetPlayerQuests() const;

    UFUNCTION() void OnCloseClicked();
};
