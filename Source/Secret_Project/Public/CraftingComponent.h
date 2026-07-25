#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingComponent.generated.h"

class UInventoryComponent;

// 제작 재료 한 종
USTRUCT(BlueprintType)
struct FCraftCost
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
    FName ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
    int32 Count = 1;
};

// 제작 레시피
USTRUCT(BlueprintType)
struct FCraftRecipe
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
    FString Name = TEXT("제작품");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
    FName OutputId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
    int32 OutputCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
    TArray<FCraftCost> Inputs;
};

/**
 * 제작 컴포넌트. 인벤토리(B) 재료를 소모해 결과물 생성.
 * - 플레이어에 부착. InventoryComponent를 같은 액터에서 찾아 사용(호출만).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UCraftingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCraftingComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
    TArray<FCraftRecipe> Recipes;

    // 레시피 재료가 충분한가
    UFUNCTION(BlueprintPure, Category = "Craft")
    bool CanCraft(int32 RecipeIndex) const;

    // 제작: 재료 소모 + 결과물 인벤 적립. 성공 시 true.
    UFUNCTION(BlueprintCallable, Category = "Craft")
    bool Craft(int32 RecipeIndex);

    // UI용 라벨 ("이름 (재료: a x2, b x1)")
    UFUNCTION(BlueprintPure, Category = "Craft")
    FText GetRecipeLabel(int32 RecipeIndex) const;

    UFUNCTION(BlueprintPure, Category = "Craft")
    int32 NumRecipes() const { return Recipes.Num(); }

private:
    UInventoryComponent* GetInventory() const;
};
