#include "InventoryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "InventoryComponent.h"
#include "UIRuntime.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

// 색 팔레트(전용 UI 카드)
static const FLinearColor INV_CARD(0.12f, 0.14f, 0.20f, 0.96f);
static const FLinearColor INV_NAME(0.97f, 0.97f, 1.0f, 1.f);
static const FLinearColor INV_COUNT(0.98f, 0.86f, 0.50f, 1.f);
static const FLinearColor INV_DESC(0.72f, 0.76f, 0.84f, 1.f);

static FString EffectText(const FConsumableDef& D)
{
    switch (D.Effect)
    {
    case EConsumableEffect::HealHP:      return FString::Printf(TEXT("HP 회복 +%.0f"), D.Magnitude);
    case EConsumableEffect::HealSP:      return FString::Printf(TEXT("SP 회복 +%.0f"), D.Magnitude);
    case EConsumableEffect::FullHeal:    return TEXT("HP·SP 완전 회복");
    case EConsumableEffect::CureAilment: return TEXT("상태이상 치료");
    case EConsumableEffect::KeyItem:     return TEXT("열쇠 아이템");
    default:                             return TEXT("");
    }
}

UInventoryWidget* UInventoryWidget::OpenInventory(APlayerController* PC, TSubclassOf<UInventoryWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UInventoryWidget* W = CreateWidget<UInventoryWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UInventoryWidget::OnCloseClicked);
    if (Txt_Title) Txt_Title->SetText(FText::FromString(TEXT("가방")));

    Refresh();
}

UInventoryComponent* UInventoryWidget::GetPlayerInventory() const
{
    if (APawn* P = GetOwningPlayerPawn())
        return P->FindComponentByClass<UInventoryComponent>();
    return nullptr;
}

void UInventoryWidget::Refresh()
{
    UInventoryComponent* Inv = GetPlayerInventory();
    const TArray<FItemStack> Empty;
    const TArray<FItemStack>& Stacks = Inv ? Inv->GetStacks() : Empty;

    // 전용 UI: 아이템 카드를 동적 생성
    if (List_Items)
    {
        UUIRuntime::Clear(List_Items);
        if (Stacks.Num() == 0)
        {
            UUIRuntime::AddText(List_Items, TEXT("가방이 비어 있습니다."), INV_DESC, 18, 1, 0.f);
            return;
        }
        for (const FItemStack& S : Stacks)
        {
            if (S.Count <= 0) continue;
            FConsumableDef Def;
            const bool bHas = UInventoryComponent::FindDef(S.Id, Def);
            const FString Name = bHas ? Def.Name : S.Id.ToString();

            UVerticalBox* Card = UUIRuntime::AddCard(List_Items, INV_CARD, 8.f);
            UHorizontalBox* Row = UUIRuntime::AddRow(Card, 2.f);
            UUIRuntime::RowText(Row, Name, INV_NAME, 20, 0, true);                          // 이름(좌, 채움)
            UUIRuntime::RowText(Row, FString::Printf(TEXT("x%d"), S.Count), INV_COUNT, 20, 2, false); // 개수(우)

            FString Desc = bHas ? EffectText(Def) : FString();
            if (bHas && !Def.Description.IsEmpty())
                Desc += (Desc.IsEmpty() ? TEXT("") : TEXT("   —  ")) + Def.Description;
            if (!Desc.IsEmpty())
                UUIRuntime::AddText(Card, Desc, INV_DESC, 14, 0, 0.f, true);
        }
        StaggerIntro(List_Items);   // 카드 순차 등장 연출
        return;
    }

    // 폴백(컨테이너 없을 때 옛 텍스트 슬롯)
    UTextBlock* Slots[6] = { Txt_Item0, Txt_Item1, Txt_Item2, Txt_Item3, Txt_Item4, Txt_Item5 };
    for (int32 i = 0; i < 6; ++i)
    {
        if (!Slots[i]) continue;
        if (Stacks.IsValidIndex(i))
        {
            FConsumableDef Def;
            const FString Name = UInventoryComponent::FindDef(Stacks[i].Id, Def) ? Def.Name : Stacks[i].Id.ToString();
            Slots[i]->SetText(FText::FromString(FString::Printf(TEXT("%s  x%d"), *Name, Stacks[i].Count)));
        }
        else if (i == 0 && Stacks.Num() == 0) Slots[i]->SetText(FText::FromString(TEXT("가방이 비어 있습니다.")));
        else Slots[i]->SetText(FText::GetEmpty());
    }
}

void UInventoryWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
