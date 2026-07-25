#include "CraftingComponent.h"
#include "InventoryComponent.h"
#include "GameFramework/Actor.h"

UCraftingComponent::UCraftingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UInventoryComponent* UCraftingComponent::GetInventory() const
{
    if (AActor* Owner = GetOwner())
        return Owner->FindComponentByClass<UInventoryComponent>();
    return nullptr;
}

bool UCraftingComponent::CanCraft(int32 RecipeIndex) const
{
    if (!Recipes.IsValidIndex(RecipeIndex)) return false;
    UInventoryComponent* Inv = GetInventory();
    if (!Inv) return false;

    const FCraftRecipe& R = Recipes[RecipeIndex];
    for (const FCraftCost& C : R.Inputs)
    {
        if (Inv->GetCount(C.ItemId) < C.Count)
            return false;
    }
    return true;
}

bool UCraftingComponent::Craft(int32 RecipeIndex)
{
    if (!CanCraft(RecipeIndex)) return false;
    UInventoryComponent* Inv = GetInventory();
    if (!Inv) return false;

    const FCraftRecipe& R = Recipes[RecipeIndex];
    // 재료 소모 (CanCraft로 이미 충분 확인)
    for (const FCraftCost& C : R.Inputs)
        Inv->RemoveItem(C.ItemId, C.Count);
    // 결과물 적립
    if (!R.OutputId.IsNone())
        Inv->AddItem(R.OutputId, FMath::Max(1, R.OutputCount));
    return true;
}

FText UCraftingComponent::GetRecipeLabel(int32 RecipeIndex) const
{
    if (!Recipes.IsValidIndex(RecipeIndex)) return FText::GetEmpty();
    const FCraftRecipe& R = Recipes[RecipeIndex];

    FString Mats;
    for (int32 i = 0; i < R.Inputs.Num(); ++i)
    {
        if (i > 0) Mats += TEXT(", ");
        Mats += FString::Printf(TEXT("%s x%d"), *R.Inputs[i].ItemId.ToString(), R.Inputs[i].Count);
    }
    const TCHAR* Mark = CanCraft(RecipeIndex) ? TEXT("") : TEXT(" (재료부족)");
    return FText::FromString(FString::Printf(TEXT("%s (재료: %s)%s"), *R.Name, *Mats, Mark));
}
