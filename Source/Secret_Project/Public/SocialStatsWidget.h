#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "SocialStatsWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class APlayerController;

/**
 * 사회 스탯(지식/매력/용기/친절/숙련) 화면. 플레이어 USocialStatsComponent를 읽어 스탯별 랭크/진행 표시.
 * 로직·바인딩 C++. WBP는 Txt_Title(헤더) + Txt_Entries(목록) + Btn_Close.
 * 패턴은 BondWidget/BestiaryWidget과 동일(SystemMenu 허브에서 버튼으로 열기).
 */
UCLASS()
class SECRET_PROJECT_API USocialStatsWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Social")
    static USocialStatsWidget* OpenSocialStats(APlayerController* PC, TSubclassOf<USocialStatsWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Entries;
    // 전용 UI: C++가 스탯별 게이지 카드를 동적 생성해 채우는 컨테이너
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Stats;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

private:
    void Refresh();
    UFUNCTION() void OnCloseClicked();
};
