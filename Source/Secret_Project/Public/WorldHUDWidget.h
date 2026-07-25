#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "WorldHUDWidget.generated.h"

class UTextBlock;

/**
 * 탐험 중 상시 표시되는 월드 HUD. 날짜/시간대(TimeComponent) + 골드(StatComponent)를 실시간 표시.
 * 옵트인 — APlayerCharacter.WorldHUDClass에 WBP 할당해야 BeginPlay에서 뜸(미할당 시 표시 안 됨).
 * 로직·바인딩 C++. WBP는 Txt_DayTime / Txt_Gold 레이아웃만(BindWidgetOptional, 만든 것만 갱신).
 */
UCLASS()
class SECRET_PROJECT_API UWorldHUDWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_DayTime;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Gold;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Weather;
};
