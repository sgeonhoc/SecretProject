#include "EquipmentComponent.h"
#include "StatComponent.h"
#include "GameFramework/Actor.h"

UEquipmentComponent::UEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();
    RecalcAndApply();
}

UStatComponent* UEquipmentComponent::GetStat() const
{
    if (AActor* Owner = GetOwner())
        return Owner->FindComponentByClass<UStatComponent>();
    return nullptr;
}

bool UEquipmentComponent::FindInCatalog(FName Id, FEquipItem& Out) const
{
    for (const FEquipItem& E : Catalog)
    {
        if (E.Id == Id) { Out = E; return true; }
    }
    return false;
}

bool UEquipmentComponent::EquipById(FName Id)
{
    FEquipItem Item;
    if (!FindInCatalog(Id, Item)) return false;
    Equipped.Add(Item.Slot, Id);     // 같은 슬롯이면 교체
    RecalcAndApply();
    return true;
}

void UEquipmentComponent::Unequip(EEquipSlot Slot)
{
    Equipped.Remove(Slot);
    RecalcAndApply();
}

FName UEquipmentComponent::GetEquipped(EEquipSlot Slot) const
{
    const FName* Found = Equipped.Find(Slot);
    return Found ? *Found : NAME_None;
}

void UEquipmentComponent::RecalcAndApply()
{
    float Atk = 0.f, Def = 0.f, HP = 0.f, SP = 0.f;
    for (const TPair<EEquipSlot, FName>& Pair : Equipped)
    {
        FEquipItem Item;
        if (FindInCatalog(Pair.Value, Item))
        {
            Atk += Item.AtkBonus;
            Def += Item.DefBonus;
            HP  += Item.HPBonus;
            SP  += Item.SPBonus;
        }
    }
    if (UStatComponent* Stat = GetStat())
        Stat->SetEquipBonuses(Atk, Def, HP, SP);
}

void UEquipmentComponent::GetEquippedIds(TArray<FName>& OutIds) const
{
    OutIds.Reset();
    for (const TPair<EEquipSlot, FName>& Pair : Equipped)
        OutIds.Add(Pair.Value);
}

void UEquipmentComponent::LoadEquippedIds(const TArray<FName>& InIds)
{
    Equipped.Reset();
    for (const FName& Id : InIds)
    {
        FEquipItem Item;
        if (FindInCatalog(Id, Item))
            Equipped.Add(Item.Slot, Id);
    }
    RecalcAndApply();
}
