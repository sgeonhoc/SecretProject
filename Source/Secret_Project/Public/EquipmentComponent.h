#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EquipmentComponent.generated.h"

class UStatComponent;

// 장비 슬롯
UENUM(BlueprintType)
enum class EEquipSlot : uint8
{
    Weapon    UMETA(DisplayName = "무기"),
    Armor     UMETA(DisplayName = "방어구"),
    Accessory UMETA(DisplayName = "장신구")
};

// 장비 한 점 정의 (이름 + 슬롯 + 스탯 보너스)
USTRUCT(BlueprintType)
struct FEquipItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip")
    FName Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip")
    FString Name = TEXT("장비");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip")
    EEquipSlot Slot = EEquipSlot::Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip")
    float AtkBonus = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip")
    float DefBonus = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip")
    float HPBonus = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip")
    float SPBonus = 0.f;

    bool IsValid() const { return !Id.IsNone(); }
};

/**
 * 장비 컴포넌트. 슬롯별 장비를 들고, 합산 보너스를 StatComponent에 적용.
 * - 캐릭터에 부착(플레이어/동료).
 * - EquipById/Unequip 시 자동으로 StatComponent.SetEquipBonuses 갱신.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEquipmentComponent();

    // 보유/장착 가능한 장비 카탈로그 (BP에서 채우거나 코드 기본값)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equip")
    TArray<FEquipItem> Catalog;

    // 현재 장착 (슬롯별 Id)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equip")
    TMap<EEquipSlot, FName> Equipped;

    // 카탈로그의 장비를 장착(슬롯 자동). 성공 시 true.
    UFUNCTION(BlueprintCallable, Category = "Equip")
    bool EquipById(FName Id);

    // 슬롯 해제
    UFUNCTION(BlueprintCallable, Category = "Equip")
    void Unequip(EEquipSlot Slot);

    // 현재 장착 Id (없으면 None)
    UFUNCTION(BlueprintPure, Category = "Equip")
    FName GetEquipped(EEquipSlot Slot) const;

    // 합산 보너스를 StatComponent에 다시 밀어넣기 (장착 변경/로드 후)
    UFUNCTION(BlueprintCallable, Category = "Equip")
    void RecalcAndApply();

    // 세이브/로드용
    void GetEquippedIds(TArray<FName>& OutIds) const;
    void LoadEquippedIds(const TArray<FName>& InIds);

protected:
    virtual void BeginPlay() override;

private:
    bool FindInCatalog(FName Id, FEquipItem& Out) const;
    UStatComponent* GetStat() const;
};
