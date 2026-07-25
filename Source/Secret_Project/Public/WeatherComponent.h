#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimeComponent.h" // EDayPhase (OnTimeChanged 시그니처)
#include "WeatherComponent.generated.h"

UENUM(BlueprintType)
enum class EWeather : uint8
{
    Clear UMETA(DisplayName = "맑음"),
    Rain  UMETA(DisplayName = "비"),
    Snow  UMETA(DisplayName = "눈"),
    Fog   UMETA(DisplayName = "안개"),
    Storm UMETA(DisplayName = "폭풍")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeatherChanged, EWeather, Weather);

/**
 * 날씨 시스템. 플레이어에 장착, 플레이어 TimeComponent 구독 → 날짜가 바뀌면 새 날씨로 전환.
 * 조명/스카이/파티클은 BP가 OnWeatherChanged 구독해 처리(로직 C++, 아트 BP). 세이브 영구화.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UWeatherComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeatherComponent();

    UPROPERTY(BlueprintAssignable, Category = "Weather")
    FOnWeatherChanged OnWeatherChanged;

    UFUNCTION(BlueprintPure, Category = "Weather")
    FORCEINLINE EWeather GetWeather() const { return Weather; }

    UFUNCTION(BlueprintPure, Category = "Weather")
    FString GetWeatherLabel() const;

    // 날씨 강제 지정(이벤트/캘린더 연출용). 변경 시 델리게이트 발사 + 영구화.
    UFUNCTION(BlueprintCallable, Category = "Weather")
    void SetWeather(EWeather NewWeather);

    // 가중 랜덤으로 다음 날씨 뽑아 적용(맑음이 가장 흔함)
    UFUNCTION(BlueprintCallable, Category = "Weather")
    void RollNewWeather();

    // 세이브/로드
    void LoadWeather(EWeather InWeather) { Weather = InWeather; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Weather")
    EWeather Weather = EWeather::Clear;

private:
    // 마지막으로 날씨를 굴린 날(날짜 증가 감지용)
    int32 LastRolledDay = 0;

    void InitWeather();

    UFUNCTION()
    void OnWorldTimeChanged(int32 Day, EDayPhase Phase);
};
