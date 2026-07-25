#include "CraftingWidget.h"
#include "UIRuntime.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CraftingComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UCraftingWidget* UCraftingWidget::OpenCrafting(APlayerController* PC, TSubclassOf<UCraftingWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UCraftingWidget* W = CreateWidget<UCraftingWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

UCraftingComponent* UCraftingWidget::GetCrafting() const
{
    if (APlayerController* PC = GetOwningPlayer())
        if (APawn* Pawn = PC->GetPawn())
            return Pawn->FindComponentByClass<UCraftingComponent>();
    return nullptr;
}

void UCraftingWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UCraftingWidget::OnCloseClicked);

    Refresh();
}

void UCraftingWidget::Refresh()
{
    UCraftingComponent* Craft = GetCrafting();
    const int32 Num = Craft ? Craft->NumRecipes() : 0;

    if (Txt_Title)
    {
        const FString Title = (Num == 0)
            ? FString(TEXT("제작 — 레시피가 없습니다"))
            : FString::Printf(TEXT("제작 — 레시피 %d종"), Num);
        Txt_Title->SetText(FText::FromString(Title));
    }

    if (!List_Recipes) return;
    UUIRuntime::Clear(List_Recipes);
    if (!Craft) return;

    for (int32 i = 0; i < Num; ++i)
    {
        const bool bCan = Craft->CanCraft(i);

        UCardButton* Btn = nullptr;
        UVerticalBox* Inner = UUIRuntime::AddClickCard(List_Recipes, UIColor::Card, i, Btn);
        if (!Inner) continue;

        // 레시피 라벨("이름 (재료: a x2, b x1)") — 재료 충분하면 흰/부족하면 흐림
        UUIRuntime::AddText(Inner, Craft->GetRecipeLabel(i).ToString(),
                            bCan ? UIColor::Title : UIColor::Dim, 17, ETextJustify::Left, 0.f, true);

        if (Btn)
        {
            Btn->SetIsEnabled(bCan); // 재료 충분할 때만 클릭 가능
            Btn->OnCardClicked.AddDynamic(this, &UCraftingWidget::OnCardClicked);
        }
    }
}

void UCraftingWidget::OnCardClicked(int32 Index) { DoCraft(Index); }

void UCraftingWidget::DoCraft(int32 RecipeIndex)
{
    if (UCraftingComponent* Craft = GetCrafting())
        Craft->Craft(RecipeIndex); // 내부에서 CanCraft 재확인 + 재료소모/적립
    Refresh();
}

void UCraftingWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
