#include "EquipmentWidget.h"
#include "EquipmentComponent.h"
#include "UIRuntime.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

static const TCHAR* EquipSlotName(EEquipSlot Slot)
{
    switch (Slot)
    {
    case EEquipSlot::Weapon:    return TEXT("무기");
    case EEquipSlot::Armor:     return TEXT("방어구");
    case EEquipSlot::Accessory: return TEXT("장신구");
    default:                    return TEXT("");
    }
}

UEquipmentWidget* UEquipmentWidget::OpenEquipment(APlayerController* PC, TSubclassOf<UEquipmentWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UEquipmentWidget* W = CreateWidget<UEquipmentWidget>(PC, WidgetClass);
    if (W)
    {
        W->AddToViewport(50);
        FInputModeGameAndUI Mode;
        PC->SetInputMode(Mode);
        PC->bShowMouseCursor = true;
    }
    return W;
}

UEquipmentComponent* UEquipmentWidget::GetEquip() const
{
    if (APlayerController* PC = GetOwningPlayer())
        if (APawn* Pawn = PC->GetPawn())
            return Pawn->FindComponentByClass<UEquipmentComponent>();
    return nullptr;
}

void UEquipmentWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UEquipmentWidget::OnCloseClicked);
    Refresh();
}

void UEquipmentWidget::Refresh()
{
    UEquipmentComponent* Eq = GetEquip();

    if (Txt_Equipped && Eq)
        Txt_Equipped->SetText(FText::FromString(FString::Printf(
            TEXT("무기:%s  방어구:%s  장신구:%s"),
            *Eq->GetEquipped(EEquipSlot::Weapon).ToString(),
            *Eq->GetEquipped(EEquipSlot::Armor).ToString(),
            *Eq->GetEquipped(EEquipSlot::Accessory).ToString())));

    if (!List_Items) return;
    UUIRuntime::Clear(List_Items);
    if (!Eq) return;

    for (int32 i = 0; i < Eq->Catalog.Num(); ++i)
    {
        const FEquipItem& It = Eq->Catalog[i];
        const bool bOn = (Eq->GetEquipped(It.Slot) == It.Id);
        const FLinearColor Bg = bOn ? FLinearColor(0.14f, 0.22f, 0.16f, 0.96f) : UIColor::Card;

        UCardButton* Btn = nullptr;
        UVerticalBox* Inner = UUIRuntime::AddClickCard(List_Items, Bg, i, Btn);
        if (!Inner) continue;

        // 제목 행: [장착중] 이름(좌) ── 슬롯(우)
        UHorizontalBox* Row = UUIRuntime::AddRow(Inner, 2.f);
        const FString Title = FString::Printf(TEXT("%s%s"), bOn ? TEXT("[E] ") : TEXT(""), *It.Name);
        UUIRuntime::RowText(Row, Title, bOn ? UIColor::Good : UIColor::Title, 18, ETextJustify::Left, true);
        UUIRuntime::RowText(Row, EquipSlotName(It.Slot), UIColor::Sub, 14, ETextJustify::Right, false);

        // 스탯 보너스 줄(0 아닌 것만)
        TArray<FString> Parts;
        if (!FMath::IsNearlyZero(It.AtkBonus)) Parts.Add(FString::Printf(TEXT("공+%.0f"), It.AtkBonus));
        if (!FMath::IsNearlyZero(It.DefBonus)) Parts.Add(FString::Printf(TEXT("방+%.0f"), It.DefBonus));
        if (!FMath::IsNearlyZero(It.HPBonus))  Parts.Add(FString::Printf(TEXT("HP+%.0f"), It.HPBonus));
        if (!FMath::IsNearlyZero(It.SPBonus))  Parts.Add(FString::Printf(TEXT("SP+%.0f"), It.SPBonus));
        UUIRuntime::AddText(Inner, FString::Join(Parts, TEXT("  ")), UIColor::Accent, 14);

        if (Btn) Btn->OnCardClicked.AddDynamic(this, &UEquipmentWidget::OnCardClicked);
    }
}

void UEquipmentWidget::OnCardClicked(int32 Index)
{
    EquipIndex(Index);
    Refresh();
}

void UEquipmentWidget::EquipIndex(int32 Index)
{
    if (UEquipmentComponent* Eq = GetEquip())
        if (Eq->Catalog.IsValidIndex(Index))
            Eq->EquipById(Eq->Catalog[Index].Id);
}

void UEquipmentWidget::OnCloseClicked()
{
    // 다른 메뉴 위젯과 동일하게 입력모드/커서 복원(누락 시 닫아도 커서 남고 GameAndUI 고착)
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    CloseWithOutro();
}
