#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "CraftingWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class APlayerController;
class UCraftingComponent;

/**
 * 제작/요리 화면. 플레이어 CraftingComponent(레시피=재료→결과물)를 동적 클릭 카드 목록으로 표시.
 * 재료 충분(CanCraft)할 때만 카드 활성, 클릭 시 제작(재료 소모+결과물 적립). 레시피 개수 무제한.
 * 로직·바인딩 C++. WBP는 Txt_Title + List_Recipes 컨테이너 + Btn_Close(헤더).
 */
UCLASS()
class SECRET_PROJECT_API UCraftingWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Crafting")
    static UCraftingWidget* OpenCrafting(APlayerController* PC, TSubclassOf<UCraftingWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;
    // 전용 UI: C++가 레시피 카드를 동적 생성해 채우는 컨테이너
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Recipes;

private:
    void Refresh();
    void DoCraft(int32 RecipeIndex);

    UCraftingComponent* GetCrafting() const;

    UFUNCTION() void OnCardClicked(int32 Index);
    UFUNCTION() void OnCloseClicked();
};
