#include "WorldHUDWidget.h"
#include "Components/TextBlock.h"
#include "TimeComponent.h"
#include "StatComponent.h"
#include "WeatherComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UWorldHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    bCloseOnEsc = false;    // 상시 HUD는 ESC로 닫히면 안 됨
    bWantsCursor = false;   // 표시 전용 — 게임 입력/커서를 막지 않음

    APawn* Pawn = GetOwningPlayer() ? GetOwningPlayer()->GetPawn() : nullptr;
    if (!Pawn) return;

    if (Txt_DayTime)
    {
        if (UTimeComponent* Time = Pawn->FindComponentByClass<UTimeComponent>())
            Txt_DayTime->SetText(FText::FromString(Time->GetTimeLabel()));
    }

    if (Txt_Gold)
    {
        if (UStatComponent* Stat = Pawn->FindComponentByClass<UStatComponent>())
            Txt_Gold->SetText(FText::FromString(FString::Printf(TEXT("%d G"), Stat->GetGold())));
    }

    if (Txt_Weather)
    {
        if (UWeatherComponent* Weather = Pawn->FindComponentByClass<UWeatherComponent>())
            Txt_Weather->SetText(FText::FromString(Weather->GetWeatherLabel()));
    }
}
