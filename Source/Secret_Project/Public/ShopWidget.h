#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "ShopWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UStatComponent;
class UInventoryComponent;
class APlayerController;

// 상점 아이템 종류 (효과 분기)
UENUM(BlueprintType)
enum class EShopItemType : uint8
{
    BoostMaxHP   UMETA(DisplayName = "최대 HP 강화"),
    BoostAttack  UMETA(DisplayName = "공격력 강화"),
    BoostDefense UMETA(DisplayName = "방어력 강화"),
    BoostMaxSP   UMETA(DisplayName = "최대 SP 강화"),
    FullHeal     UMETA(DisplayName = "완전 회복"),
    Consumable   UMETA(DisplayName = "소비아이템 구매")
};

USTRUCT(BlueprintType)
struct FShopItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FString Name = TEXT("아이템");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    int32 Cost = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    EShopItemType Type = EShopItemType::BoostMaxHP;

    // 강화량 (영구 강화에만 사용, 완전회복/소비아이템은 무시)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    float Amount = 20.f;

    // Type == Consumable 일 때 인벤토리에 적립할 아이템 Id (InventoryComponent 카탈로그의 Id)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FName ConsumableId;
};

/**
 * 골드 사용처(상점). 로직·UI 바인딩 전부 C++. WBP_Shop은 레이아웃만(reparent).
 * 버튼/텍스트는 BindWidgetOptional — BP에서 만든 것만 연결됨(이름 정확히 일치).
 */
UCLASS()
class SECRET_PROJECT_API UShopWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    // 어디서든 상점 열기 (위젯 생성 + 뷰포트 + 입력 UI 모드 + 커서). 생성된 위젯 반환.
    // InShopKind: 상점 테마(편의점/정비/카페/바/꽃집/만화방 등). None이면 종합 기본 재고.
    UFUNCTION(BlueprintCallable, Category = "Shop")
    static UShopWidget* OpenShop(APlayerController* PC, TSubclassOf<UShopWidget> WidgetClass, FName InShopKind = NAME_None);

    // 판매 목록 — 비어 있으면 NativeConstruct에서 ShopKind별 기본값 채움. BP에서 편집 가능(채우면 그게 우선).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    TArray<FShopItem> Items;

    // 상점 테마(상인별 차별 재고). NPC가 OpenShop에 전달. None=종합.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FName ShopKind;

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Gold;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Message;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;
    // 전용 UI: C++가 판매 항목을 동적 클릭 카드로 채우는 컨테이너(개수 무제한)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Items;

private:
    void InitDefaultItems();
    void Refresh();
    void Buy(int32 Index);
    UStatComponent* GetPlayerStat() const;
    UInventoryComponent* GetPlayerInventory() const;

    UFUNCTION() void OnCardClicked(int32 Index);
    UFUNCTION() void OnCloseClicked();

    FString LastMessage;
};
