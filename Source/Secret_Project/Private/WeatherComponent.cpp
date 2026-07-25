#include "WeatherComponent.h"
#include "StatComponent.h"
#include "SecretSaveGame.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

UWeatherComponent::UWeatherComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FString UWeatherComponent::GetWeatherLabel() const
{
    switch (Weather)
    {
    case EWeather::Clear: return TEXT("맑음");
    case EWeather::Rain:  return TEXT("비");
    case EWeather::Snow:  return TEXT("눈");
    case EWeather::Fog:   return TEXT("안개");
    case EWeather::Storm: return TEXT("폭풍");
    default:              return TEXT("맑음");
    }
}

void UWeatherComponent::BeginPlay()
{
    Super::BeginPlay();

    // 플레이어 TimeComponent 구독 + 세이브 로드 순서 보장 위해 다음 틱에 init
    if (GetWorld())
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UWeatherComponent::InitWeather);
}

void UWeatherComponent::InitWeather()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    if (UTimeComponent* Time = Owner->FindComponentByClass<UTimeComponent>())
    {
        Time->OnTimeChanged.RemoveDynamic(this, &UWeatherComponent::OnWorldTimeChanged);
        Time->OnTimeChanged.AddDynamic(this, &UWeatherComponent::OnWorldTimeChanged);
        LastRolledDay = Time->GetDay();
    }

    // 로드된(또는 기본) 날씨를 BP에 1회 알림 → 시작 시점 스카이/파티클 동기화
    OnWeatherChanged.Broadcast(Weather);
}

void UWeatherComponent::OnWorldTimeChanged(int32 Day, EDayPhase /*Phase*/)
{
    // 날짜가 바뀌면(증가) 새 날씨를 굴림. 같은 날 시간대만 바뀌면 유지.
    if (Day > LastRolledDay)
    {
        LastRolledDay = Day;
        RollNewWeather();
    }
}

void UWeatherComponent::RollNewWeather()
{
    // 가중 랜덤: 맑음이 가장 흔함
    const int32 Roll = FMath::RandRange(0, 99);
    EWeather Next;
    if (Roll < 50)      Next = EWeather::Clear; // 50%
    else if (Roll < 70) Next = EWeather::Rain;  // 20%
    else if (Roll < 82) Next = EWeather::Fog;   // 12%
    else if (Roll < 94) Next = EWeather::Snow;  // 12%
    else                Next = EWeather::Storm; // 6%

    SetWeather(Next);
}

void UWeatherComponent::SetWeather(EWeather NewWeather)
{
    if (NewWeather == Weather)
    {
        OnWeatherChanged.Broadcast(Weather); // 동일해도 BP 동기화는 보장
        return;
    }
    Weather = NewWeather;

    // 영구화 (다음 로드 시 같은 날씨 복원)
    if (AActor* Owner = GetOwner())
        if (UStatComponent* Stat = Owner->FindComponentByClass<UStatComponent>())
            USecretSaveGame::SavePlayerProgression(Stat);

    OnWeatherChanged.Broadcast(Weather);
}
