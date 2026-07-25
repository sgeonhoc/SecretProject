#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UStatComponent;

// 소비아이템 효과 종류 (StatComponent의 public 함수만 사용 — A 전투코어와 비충돌)
UENUM(BlueprintType)
enum class EConsumableEffect : uint8
{
    HealHP      UMETA(DisplayName = "HP 회복"),
    HealSP      UMETA(DisplayName = "SP 회복"),
    FullHeal    UMETA(DisplayName = "완전 회복(HP+SP)"),
    CureAilment UMETA(DisplayName = "상태이상 치료"),
    KeyItem     UMETA(DisplayName = "열쇠/키 아이템(사용 불가, 문 등에 소모)")
};

// 소비아이템 정의(카탈로그). Id로 식별, 세이브엔 Id+개수만 저장.
USTRUCT(BlueprintType)
struct FConsumableDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FString Name = TEXT("아이템");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EConsumableEffect Effect = EConsumableEffect::HealHP;

    // HealHP일 때 회복량. FullHeal은 무시.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    float Magnitude = 50.f;

    // 플레이버/설명 텍스트(현대 배경 질감). UI(인벤토리/상점 툴팁)에서 표시 가능. 비어도 기존 동작 무영향.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
    FString Description;
};

// 인벤토리 한 칸 (아이템 Id + 개수). 세이브에도 그대로 사용.
USTRUCT(BlueprintType)
struct FItemStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 Count = 0;
};

/**
 * 플레이어 소비아이템 보관함. 로직 전부 C++.
 * - 상점에서 구매하면 AddItem으로 적립, 세이브로 영구화.
 * - 전투/탐험에서 UseItemOn(대상 StatComponent)로 1개 소모하며 효과 적용.
 *   ※ StatComponent는 public 함수(Heal/FullRestore)만 호출 — 편집하지 않음.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    // ── 카탈로그 (정적 — 모든 소비아이템 정의) ──
    // 전체 카탈로그. 첫 호출 시 기본 아이템으로 초기화.
    static const TArray<FConsumableDef>& GetCatalog();
    // Id로 정의 찾기. 없으면 false.
    static bool FindDef(FName Id, FConsumableDef& OutDef);

    // ── 보유 조작 ──
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(FName Id, int32 Count = 1);

    // 개수만큼 차감. 보유 부족하면 아무것도 안 하고 false.
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(FName Id, int32 Count = 1);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetCount(FName Id) const;

    // C++ 전용 (세이브/UI 순회용) — UFUNCTION 참조반환 회피
    const TArray<FItemStack>& GetStacks() const { return Stacks; }

    // ── 사용 ──
    // 대상에게 아이템 효과 적용 + 1개 소모. 성공 시 true.
    // (전투 중엔 BattleManager가 ActiveBattler의 StatComponent를 넘겨 호출 → 턴 소모는 호출측 책임)
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool UseItemOn(UStatComponent* Target, FName Id);

    // ── 세이브/로드 ──
    void LoadStacks(const TArray<FItemStack>& InStacks) { Stacks = InStacks; }

private:
    UPROPERTY(VisibleAnywhere, Category = "Inventory")
    TArray<FItemStack> Stacks;
};
