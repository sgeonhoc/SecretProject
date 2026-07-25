#include "ShopWidget.h"
#include "UIRuntime.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "StatComponent.h"
#include "InventoryComponent.h"
#include "SecretSaveGame.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UShopWidget* UShopWidget::OpenShop(APlayerController* PC, TSubclassOf<UShopWidget> WidgetClass, FName InShopKind)
{
    if (!PC || !WidgetClass) return nullptr;
    UShopWidget* W = CreateWidget<UShopWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->ShopKind = InShopKind;     // AddToViewport(NativeConstruct→InitDefaultItems) 전에 설정해야 테마 재고 반영
    W->AddToViewport(50);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UShopWidget::NativeConstruct()
{
    Super::NativeConstruct();

    InitDefaultItems();

    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UShopWidget::OnCloseClicked);

    // 상점 입장 인사말(상인 개성) — 상점은 대화 대신 바로 열리므로 여기서 목소리를 들려준다.
    if (LastMessage.IsEmpty())
    {
        const FName K = ShopKind;
        if      (K == TEXT("convenience")) LastMessage = TEXT("어서 와~! 단골 할인 들어간다, 뭐 필요해?");
        else if (K == TEXT("gear"))        LastMessage = TEXT("뭐 고장 났어? …아니면 몸을 손볼 거면, 좋은 거 있지.");
        else if (K == TEXT("cafe"))        LastMessage = TEXT("어서 오세요~ 따뜻한 거 한 잔 하면서 천천히 골라요.");
        else if (K == TEXT("bar"))         LastMessage = TEXT("……어서 와. 고민은 두고, 필요한 것만 말해.");
        else if (K == TEXT("flower"))      LastMessage = TEXT("어서 와요! 오늘은 물망초가 예쁘게 폈어요~");
        else if (K == TEXT("manhwa"))      LastMessage = TEXT("…어, 왔어요? 천천히 봐요. 라면도 끓여 줄까…");
        else                                LastMessage = TEXT("어서 오세요. 필요한 걸 골라 보세요.");
    }

    Refresh();
}

void UShopWidget::InitDefaultItems()
{
    if (Items.Num() > 0) return;

    auto Make = [](const TCHAR* N, int32 C, EShopItemType T, float A)
    {
        FShopItem I;
        I.Name = N; I.Cost = C; I.Type = T; I.Amount = A;
        return I;
    };
    // 소비아이템: 구매 시 인벤토리에 적립 (ConsumableId = InventoryComponent 카탈로그 Id)
    auto MakeConsumable = [](const TCHAR* N, int32 C, const TCHAR* Id)
    {
        FShopItem I;
        I.Name = N; I.Cost = C; I.Type = EShopItemType::Consumable; I.ConsumableId = Id;
        return I;
    };

    // 기본 6칸(표시 슬롯 한계). 상점 테마(ShopKind)별 차별 재고. None=종합.
    // 영구 강화는 반복구매 가능 → 골드로 전투력 인플레 방지 위해 가격 높임(다회 퀘스트 분량의 투자).
    const FName K = ShopKind;
    if (K == TEXT("convenience"))        // 편의점(태수) — 소비아이템 종합
    {
        Items.Add(MakeConsumable(TEXT("회복약"),       40, TEXT("HealPotion")));
        Items.Add(MakeConsumable(TEXT("고급 회복약"),  120, TEXT("HiPotion")));
        Items.Add(MakeConsumable(TEXT("SP 회복약"),     60, TEXT("SPPotion")));
        Items.Add(MakeConsumable(TEXT("해독제"),        50, TEXT("Antidote")));
        Items.Add(MakeConsumable(TEXT("에너지바"),      25, TEXT("Herb")));
        Items.Add(MakeConsumable(TEXT("에너지 드링크"), 30, TEXT("ManaFlower")));
    }
    else if (K == TEXT("gear"))          // 정비공(두식) — 방어/체력 강화 위주
    {
        Items.Add(Make(TEXT("차체 보강 (최대 HP +20)"), 250, EShopItemType::BoostMaxHP,  20.f));
        Items.Add(Make(TEXT("방어 튜닝 (방어력 +3)"),   350, EShopItemType::BoostDefense, 3.f));
        Items.Add(Make(TEXT("출력 향상 (공격력 +3)"),   350, EShopItemType::BoostAttack,  3.f));
        Items.Add(MakeConsumable(TEXT("회복약"),       40, TEXT("HealPotion")));
        Items.Add(MakeConsumable(TEXT("고급 회복약"),  120, TEXT("HiPotion")));
    }
    else if (K == TEXT("cafe"))          // 카페(유나) — SP/기분전환 위주
    {
        Items.Add(MakeConsumable(TEXT("SP 회복약"),     60, TEXT("SPPotion")));
        Items.Add(MakeConsumable(TEXT("에너지 드링크"), 30, TEXT("ManaFlower")));
        Items.Add(Make(TEXT("정신 수련 (최대 SP +15)"), 300, EShopItemType::BoostMaxSP, 15.f));
        Items.Add(MakeConsumable(TEXT("회복약"),       40, TEXT("HealPotion")));
        Items.Add(Make(TEXT("커피 한 잔의 여유 (완전 회복)"), 150, EShopItemType::FullHeal, 0.f));
    }
    else if (K == TEXT("bar"))           // 심야 바(레이) — 고급 소비 + SP
    {
        Items.Add(MakeConsumable(TEXT("고급 회복약"),  120, TEXT("HiPotion")));
        Items.Add(MakeConsumable(TEXT("해독제"),        50, TEXT("Antidote")));
        Items.Add(MakeConsumable(TEXT("SP 회복약"),     60, TEXT("SPPotion")));
        Items.Add(Make(TEXT("담력 단련 (공격력 +3)"),   350, EShopItemType::BoostAttack, 3.f));
        Items.Add(Make(TEXT("한 잔의 위로 (완전 회복)"), 150, EShopItemType::FullHeal, 0.f));
    }
    else if (K == TEXT("flower"))        // 꽃집(민들레) — 회복/체력 + 선물용
    {
        Items.Add(MakeConsumable(TEXT("회복약"),       40, TEXT("HealPotion")));
        Items.Add(MakeConsumable(TEXT("고급 회복약"),  120, TEXT("HiPotion")));
        Items.Add(Make(TEXT("활력 (최대 HP +20)"),     250, EShopItemType::BoostMaxHP, 20.f));
        Items.Add(MakeConsumable(TEXT("해독제"),        50, TEXT("Antidote")));
    }
    else if (K == TEXT("manhwa"))        // 만화방(구씨) — 잡화/SP/간식
    {
        Items.Add(MakeConsumable(TEXT("SP 회복약"),     60, TEXT("SPPotion")));
        Items.Add(MakeConsumable(TEXT("에너지바"),      25, TEXT("Herb")));
        Items.Add(MakeConsumable(TEXT("에너지 드링크"), 30, TEXT("ManaFlower")));
        Items.Add(MakeConsumable(TEXT("회복약"),       40, TEXT("HealPotion")));
    }
    else                                 // 종합(기본) — 기존과 동일
    {
        Items.Add(Make(TEXT("체력 단련 (최대 HP +20)"), 250, EShopItemType::BoostMaxHP,  20.f));
        Items.Add(Make(TEXT("근력 단련 (공격력 +3)"),   350, EShopItemType::BoostAttack,  3.f));
        Items.Add(MakeConsumable(TEXT("회복약"),      40, TEXT("HealPotion")));
        Items.Add(MakeConsumable(TEXT("고급 회복약"), 120, TEXT("HiPotion")));
        Items.Add(MakeConsumable(TEXT("SP 회복약"),    60, TEXT("SPPotion")));
        Items.Add(MakeConsumable(TEXT("해독제"),       50, TEXT("Antidote")));
    }
}

UStatComponent* UShopWidget::GetPlayerStat() const
{
    if (APawn* P = GetOwningPlayerPawn())
        return P->FindComponentByClass<UStatComponent>();
    return nullptr;
}

UInventoryComponent* UShopWidget::GetPlayerInventory() const
{
    if (APawn* P = GetOwningPlayerPawn())
        return P->FindComponentByClass<UInventoryComponent>();
    return nullptr;
}

void UShopWidget::Refresh()
{
    UStatComponent* Stat = GetPlayerStat();
    const int32 Gold = Stat ? Stat->GetGold() : 0;

    if (Txt_Gold)
        Txt_Gold->SetText(FText::FromString(FString::Printf(TEXT("골드: %d"), Gold)));
    if (Txt_Message)
        Txt_Message->SetText(FText::FromString(LastMessage));

    if (!List_Items) return;
    UUIRuntime::Clear(List_Items);

    for (int32 i = 0; i < Items.Num(); ++i)
    {
        const FShopItem& It = Items[i];
        const bool bAfford = (Gold >= It.Cost);

        UCardButton* Btn = nullptr;
        UVerticalBox* Inner = UUIRuntime::AddClickCard(List_Items, UIColor::Card, i, Btn);
        if (!Inner) continue;

        // 항목명(좌) ── 가격(우, 살 수 있으면 금색/없으면 흐림)
        UHorizontalBox* Row = UUIRuntime::AddRow(Inner, 0.f);
        UUIRuntime::RowText(Row, It.Name, UIColor::Title, 18, ETextJustify::Left, true);
        UUIRuntime::RowText(Row, FString::Printf(TEXT("%d G"), It.Cost),
                            bAfford ? UIColor::Accent : UIColor::Dim, 16, ETextJustify::Right, false);

        if (Btn)
        {
            Btn->SetIsEnabled(bAfford); // 골드 부족이면 비활성(클릭 불가)
            Btn->OnCardClicked.AddDynamic(this, &UShopWidget::OnCardClicked);
        }
    }
}

void UShopWidget::Buy(int32 Index)
{
    if (!Items.IsValidIndex(Index)) return;

    UStatComponent* Stat = GetPlayerStat();
    if (!Stat)
    {
        LastMessage = TEXT("플레이어 스탯을 찾을 수 없음");
        Refresh();
        return;
    }

    const FShopItem It = Items[Index];

    // 소비아이템은 적립 대상(인벤토리)이 있어야 구매 가능 → 골드 차감 전에 확인
    UInventoryComponent* Inv = nullptr;
    if (It.Type == EShopItemType::Consumable)
    {
        Inv = GetPlayerInventory();
        if (!Inv)
        {
            LastMessage = TEXT("인벤토리를 찾을 수 없음");
            Refresh();
            return;
        }
    }

    if (!Stat->SpendGold(It.Cost))
    {
        LastMessage = FString::Printf(TEXT("골드 부족! — %s"), *It.Name);
        Refresh();
        return;
    }

    switch (It.Type)
    {
    case EShopItemType::BoostMaxHP:   Stat->BoostMaxHP(It.Amount);   break;
    case EShopItemType::BoostAttack:  Stat->BoostAttack(It.Amount);  break;
    case EShopItemType::BoostDefense: Stat->BoostDefense(It.Amount); break;
    case EShopItemType::BoostMaxSP:   Stat->BoostMaxSP(It.Amount);   break;
    case EShopItemType::FullHeal:     Stat->FullRestore();           break;
    case EShopItemType::Consumable:   Inv->AddItem(It.ConsumableId, 1); break;
    }

    // 구매 즉시 영구 저장 (전투 승리 없이도 보존)
    USecretSaveGame::SavePlayerProgression(Stat);

    LastMessage = FString::Printf(TEXT("구매 완료: %s"), *It.Name);
    Refresh();
}

void UShopWidget::OnCardClicked(int32 Index) { Buy(Index); }

void UShopWidget::OnCloseClicked()
{
    RemoveFromParent();
    // 상점을 대화창에서 열었으면 대화창이 남아있음 → 커서/UI 입력 복원(안 그러면 대화 클릭 불가).
    UPersonaWidgetBase::RefreshInputMode(this);
}
