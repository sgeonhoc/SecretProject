#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimeComponent.h" // EDayPhase (OnTimeChanged 시그니처)
#include "BankComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBankChanged, int32, StoredGold);

/**
 * 은행(예치/인출 + 일일 이자). 플레이어에 장착, 세이브 영구화.
 * 예치한 골드는 날짜가 바뀔 때마다 이자가 붙음(골드 싱크 + 저축 보상). 로직 전부 C++.
 * UI(BankWidget)는 잔액 표시 + 예치/인출 버튼만.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UBankComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBankComponent();

    UPROPERTY(BlueprintAssignable, Category = "Bank")
    FOnBankChanged OnBankChanged;

    // 일일 이자율 (0.02 = 2%/일). 캐릭터/난이도별로 BP 조정 가능.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bank")
    float InterestRatePerDay = 0.02f;

    // 하루 이자 상한 (폭주 방지)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bank")
    int32 MaxInterestPerDay = 500;

    UFUNCTION(BlueprintPure, Category = "Bank")
    FORCEINLINE int32 GetStoredGold() const { return StoredGold; }

    // 보유 골드에서 예치 (실제 옮긴 액수 반환). amount<=0이면 0.
    UFUNCTION(BlueprintCallable, Category = "Bank")
    int32 Deposit(int32 Amount);

    // 예치금에서 인출 (실제 옮긴 액수 반환).
    UFUNCTION(BlueprintCallable, Category = "Bank")
    int32 Withdraw(int32 Amount);

    // 세이브/로드
    void LoadStoredGold(int32 InGold) { StoredGold = FMath::Max(0, InGold); }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Bank")
    int32 StoredGold = 0;

private:
    int32 LastInterestDay = 0;

    void InitBank();
    void Persist();

    UFUNCTION()
    void OnWorldTimeChanged(int32 Day, EDayPhase Phase);
};
