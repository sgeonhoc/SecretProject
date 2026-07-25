#include "TimeComponent.h"
#include "Engine/Engine.h"

UTimeComponent::UTimeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTimeComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UTimeComponent::AdvancePhase()
{
    switch (Phase)
    {
    case EDayPhase::Morning: Phase = EDayPhase::Day;     break;
    case EDayPhase::Day:     Phase = EDayPhase::Evening; break;
    case EDayPhase::Evening: Phase = EDayPhase::Night;   break;
    case EDayPhase::Night:   Phase = EDayPhase::Morning; ++Day; break;
    }
    NotifyChanged();
}

void UTimeComponent::AdvanceToNextDay()
{
    ++Day;
    Phase = EDayPhase::Morning;
    NotifyChanged();
}

FString UTimeComponent::GetTimeLabel() const
{
    const TCHAR* PhaseName = TEXT("");
    switch (Phase)
    {
    case EDayPhase::Morning: PhaseName = TEXT("아침"); break;
    case EDayPhase::Day:     PhaseName = TEXT("낮");   break;
    case EDayPhase::Evening: PhaseName = TEXT("저녁"); break;
    case EDayPhase::Night:   PhaseName = TEXT("밤");   break;
    }
    return FString::Printf(TEXT("%d일차 %s"), Day, PhaseName);
}

void UTimeComponent::LoadTime(int32 InDay, EDayPhase InPhase)
{
    Day = FMath::Max(1, InDay);
    Phase = InPhase;
    NotifyChanged();
}

void UTimeComponent::NotifyChanged()
{
    OnTimeChanged.Broadcast(Day, Phase);
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, GetTimeLabel());
}
