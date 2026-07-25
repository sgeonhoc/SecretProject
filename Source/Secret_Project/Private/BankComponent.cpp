#include "BankComponent.h"
#include "StatComponent.h"
#include "SecretSaveGame.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

UBankComponent::UBankComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UBankComponent::BeginPlay()
{
    Super::BeginPlay();

    // 세이브 로드 순서 보장 위해 다음 틱에 구독 (StoredGold는 APlayerCharacter가 LoadStoredGold로 복원)
    if (GetWorld())
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UBankComponent::InitBank);
}

void UBankComponent::InitBank()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    if (UTimeComponent* Time = Owner->FindComponentByClass<UTimeComponent>())
    {
        Time->OnTimeChanged.RemoveDynamic(this, &UBankComponent::OnWorldTimeChanged);
        Time->OnTimeChanged.AddDynamic(this, &UBankComponent::OnWorldTimeChanged);
        LastInterestDay = Time->GetDay();
    }
    OnBankChanged.Broadcast(StoredGold);
}

void UBankComponent::OnWorldTimeChanged(int32 Day, EDayPhase /*Phase*/)
{
    // 날짜가 지나면 예치금에 이자 (지난 날 수만큼, 상한 적용)
    if (Day <= LastInterestDay) return;

    const int32 DaysPassed = Day - LastInterestDay;
    LastInterestDay = Day;

    if (StoredGold <= 0 || InterestRatePerDay <= 0.f) return;

    int32 TotalInterest = 0;
    for (int32 i = 0; i < DaysPassed; ++i)
    {
        int32 DayInterest = FMath::FloorToInt(StoredGold * InterestRatePerDay);
        DayInterest = FMath::Clamp(DayInterest, 0, MaxInterestPerDay);
        StoredGold += DayInterest;
        TotalInterest += DayInterest;
    }

    if (TotalInterest > 0)
    {
        Persist();
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
                FString::Printf(TEXT("은행 이자 +%d G (예치금 %d G)"), TotalInterest, StoredGold));
        OnBankChanged.Broadcast(StoredGold);
    }
}

int32 UBankComponent::Deposit(int32 Amount)
{
    if (Amount <= 0) return 0;

    AActor* Owner = GetOwner();
    UStatComponent* Stat = Owner ? Owner->FindComponentByClass<UStatComponent>() : nullptr;
    if (!Stat) return 0;

    const int32 Move = FMath::Min(Amount, Stat->GetGold());
    if (Move <= 0) return 0;

    if (!Stat->SpendGold(Move)) return 0;
    StoredGold += Move;

    Persist();
    OnBankChanged.Broadcast(StoredGold);
    return Move;
}

int32 UBankComponent::Withdraw(int32 Amount)
{
    if (Amount <= 0) return 0;

    AActor* Owner = GetOwner();
    UStatComponent* Stat = Owner ? Owner->FindComponentByClass<UStatComponent>() : nullptr;
    if (!Stat) return 0;

    const int32 Move = FMath::Min(Amount, StoredGold);
    if (Move <= 0) return 0;

    StoredGold -= Move;
    Stat->AddGold(Move);

    Persist();
    OnBankChanged.Broadcast(StoredGold);
    return Move;
}

void UBankComponent::Persist()
{
    if (AActor* Owner = GetOwner())
        if (UStatComponent* Stat = Owner->FindComponentByClass<UStatComponent>())
            USecretSaveGame::SavePlayerProgression(Stat);
}
