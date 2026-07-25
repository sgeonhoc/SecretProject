#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "StatusWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class APlayerController;

/**
 * 상태 화면(캐릭터 정보 요약): 레벨/HP/SP/공격/방어/골드/날짜/완료 퀘스트.
 * 로직·바인딩 전부 C++. WBP는 레이아웃만(Txt_Stats 하나에 다 표시, Btn_Close).
 */
UCLASS()
class SECRET_PROJECT_API UStatusWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Status")
    static UStatusWidget* OpenStatus(APlayerController* PC, TSubclassOf<UStatusWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Stats;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Status;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

private:
    void Refresh();
    UFUNCTION() void OnCloseClicked();
};
