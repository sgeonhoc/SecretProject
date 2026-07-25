#include "UIBuilderLibrary.h"

#if WITH_EDITOR
#include "WidgetBlueprint.h"               // UMGEditor
#include "Blueprint/WidgetTree.h"          // UMG
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/ContentWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/OverlaySlot.h"
#include "Styling/SlateTypes.h"           // FButtonStyle
#include "Kismet2/KismetEditorUtilities.h" // UnrealEd
#include "Kismet2/BlueprintEditorUtils.h"  // UnrealEd
#include "UObject/UObjectHash.h"           // GetObjectsWithOuter
#endif

UWidget* UUIBuilderLibrary::AddWidget(UWidgetBlueprint* BP, UClass* WidgetClass, FName Name)
{
#if WITH_EDITOR
    if (!BP || !BP->WidgetTree || !WidgetClass) return nullptr;
    if (!WidgetClass->IsChildOf(UWidget::StaticClass())) return nullptr;
    UWidget* W = BP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, Name);
    if (W)
    {
        W->bIsVariable = true;   // BindWidget/BindWidgetOptional 매칭을 위해 변수로
        // 변수 위젯은 WidgetVariableNameToGuidMap에 GUID가 있어야 컴파일 시 ensure가 안 터짐
        // (디버거 미부착 시엔 경고만, 부착 시 __debugbreak로 멈춤). 직접 등록해 안전화.
        BP->WidgetVariableNameToGuidMap.Add(W->GetFName(), FGuid::NewGuid());
    }
    return W;
#else
    return nullptr;
#endif
}

void UUIBuilderLibrary::AddChild(UWidget* Parent, UWidget* Child)
{
#if WITH_EDITOR
    if (!Child) return;
    if (UPanelWidget* P = Cast<UPanelWidget>(Parent)) { P->AddChild(Child); return; }
    // Border 등 단일 자식 컨테이너(UContentWidget)도 지원 — 배경 패널용.
    if (UContentWidget* C = Cast<UContentWidget>(Parent)) { C->SetContent(Child); return; }
#endif
}

void UUIBuilderLibrary::SetRoot(UWidgetBlueprint* BP, UWidget* Root)
{
#if WITH_EDITOR
    if (BP && BP->WidgetTree) BP->WidgetTree->RootWidget = Root;
#endif
}

bool UUIBuilderLibrary::HasRoot(UWidgetBlueprint* BP)
{
#if WITH_EDITOR
    return BP && BP->WidgetTree && BP->WidgetTree->RootWidget != nullptr;
#else
    return true;
#endif
}

void UUIBuilderLibrary::SetWidgetText(UWidget* W, const FString& Text)
{
#if WITH_EDITOR
    if (UTextBlock* T = Cast<UTextBlock>(W))
        T->SetText(FText::FromString(Text));
#endif
}

void UUIBuilderLibrary::CompileBP(UWidgetBlueprint* BP)
{
#if WITH_EDITOR
    if (BP)
    {
        // 트리에 실제로 존재하는 '변수' 위젯 이름만 GUID 맵에 남김.
        // 삭제된 위젯의 스테일 GUID가 남아있으면 컴파일 시
        // "Variable X was deleted but still has a GUID" ensure가 발생(보존형 WBP에서 흔함).
        if (BP->WidgetTree)
        {
            TSet<FName> LiveVarNames;
            TArray<UWidget*> All;
            BP->WidgetTree->GetAllWidgets(All);
            for (UWidget* W : All)
                if (W && W->bIsVariable) LiveVarNames.Add(W->GetFName());
            for (auto It = BP->WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
                if (!LiveVarNames.Contains(It.Key())) It.RemoveCurrent();
        }
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        FKismetEditorUtilities::CompileBlueprint(BP);
    }
#endif
}

void UUIBuilderLibrary::SetCanvasSlot(UWidget* Child, FVector2D Pos, FVector2D Size,
                                      FVector2D AnchorMin, FVector2D AnchorMax, FVector2D Alignment)
{
#if WITH_EDITOR
    if (!Child) return;
    if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Child->Slot))
    {
        S->SetAnchors(FAnchors(AnchorMin.X, AnchorMin.Y, AnchorMax.X, AnchorMax.Y));
        S->SetAlignment(Alignment);
        S->SetPosition(Pos);
        S->SetSize(Size);
    }
#endif
}

void UUIBuilderLibrary::SetCollapsed(UWidget* W, bool bCollapsed)
{
#if WITH_EDITOR
    if (W) W->SetVisibility(bCollapsed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
#endif
}

void UUIBuilderLibrary::SetColor(UWidget* W, FLinearColor Color)
{
#if WITH_EDITOR
    if (UBorder* B = Cast<UBorder>(W)) { B->SetBrushColor(Color); return; }
    if (UImage* I = Cast<UImage>(W)) { I->SetColorAndOpacity(Color); return; }
    if (UButton* Bt = Cast<UButton>(W)) { Bt->SetBackgroundColor(Color); return; }
#endif
}

void UUIBuilderLibrary::SetTextStyle(UWidget* W, FLinearColor Color, int32 FontSize, int32 Justify)
{
#if WITH_EDITOR
    if (UTextBlock* T = Cast<UTextBlock>(W))
    {
        T->SetColorAndOpacity(FSlateColor(Color));
        if (FontSize > 0)
        {
            FSlateFontInfo F = T->GetFont();
            F.Size = FontSize;
            T->SetFont(F);
        }
        if (Justify >= 0)
            T->SetJustification(static_cast<ETextJustify::Type>(Justify));
    }
#endif
}

void UUIBuilderLibrary::SetBorderPadding(UWidget* W, float Left, float Top, float Right, float Bottom)
{
#if WITH_EDITOR
    if (UBorder* B = Cast<UBorder>(W))
        B->SetPadding(FMargin(Left, Top, Right, Bottom));
#endif
}

void UUIBuilderLibrary::SetCanvasFullScreen(UWidget* W)
{
#if WITH_EDITOR
    if (W)
        if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(W->Slot))
        {
            S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            S->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
        }
#endif
}

void UUIBuilderLibrary::SetHitTestInvisible(UWidget* W, bool bChildrenInteractive)
{
#if WITH_EDITOR
    if (W)
        W->SetVisibility(bChildrenInteractive ? ESlateVisibility::SelfHitTestInvisible
                                              : ESlateVisibility::HitTestInvisible);
#endif
}

void UUIBuilderLibrary::SetTextShadow(UWidget* W, FLinearColor ShadowColor, FVector2D Offset)
{
#if WITH_EDITOR
    if (UTextBlock* T = Cast<UTextBlock>(W))
    {
        T->SetShadowColorAndOpacity(ShadowColor);
        T->SetShadowOffset(Offset);
    }
#endif
}

void UUIBuilderLibrary::SetButtonColors(UWidget* W, FLinearColor Normal, FLinearColor Hovered, FLinearColor Pressed)
{
#if WITH_EDITOR
    if (UButton* B = Cast<UButton>(W))
    {
        FButtonStyle S = B->GetStyle();
        S.Normal.TintColor  = FSlateColor(Normal);
        S.Hovered.TintColor = FSlateColor(Hovered);
        S.Pressed.TintColor = FSlateColor(Pressed);
        B->SetStyle(S);
    }
#endif
}

void UUIBuilderLibrary::SetSlotPadding(UWidget* Child, float Left, float Top, float Right, float Bottom)
{
#if WITH_EDITOR
    if (!Child) return;
    const FMargin M(Left, Top, Right, Bottom);
    if (UVerticalBoxSlot* S   = Cast<UVerticalBoxSlot>(Child->Slot))   { S->SetPadding(M); return; }
    if (UHorizontalBoxSlot* S = Cast<UHorizontalBoxSlot>(Child->Slot)) { S->SetPadding(M); return; }
    if (UScrollBoxSlot* S     = Cast<UScrollBoxSlot>(Child->Slot))     { S->SetPadding(M); return; }
    if (UButtonSlot* S        = Cast<UButtonSlot>(Child->Slot))        { S->SetPadding(M); return; }
    if (UOverlaySlot* S       = Cast<UOverlaySlot>(Child->Slot))       { S->SetPadding(M); return; }
#endif
}

void UUIBuilderLibrary::ClearTree(UWidgetBlueprint* BP)
{
#if WITH_EDITOR
    if (BP && BP->WidgetTree)
    {
        UWidgetTree* Tree = BP->WidgetTree;
        Tree->RootWidget = nullptr;
        // ★ GetAllWidgets는 RootWidget부터 순회하므로 루트를 비운 뒤엔 빈 배열을 반환한다.
        //   → 트리에 Outer로 매달린 모든 위젯 오브젝트를 직접 수집(도달 불가/고아 위젯 포함).
        TArray<UObject*> Subs;
        GetObjectsWithOuter(Tree, Subs, /*bIncludeNestedObjects=*/true);
        for (UObject* O : Subs)
            if (UWidget* W = Cast<UWidget>(O))
            {
                Tree->RemoveWidget(W);
                // 이름 점유 해제(transient로 rename) — 같은 이름을 다른 클래스로 재생성할 때
                // "Cannot replace existing object of a different class" 크래시 방지
                // (예: Window를 VerticalBox→ScrollBox로 교체).
                W->Rename(nullptr, GetTransientPackage(),
                          REN_DontCreateRedirectors | REN_NonTransactional);
            }
        // 위젯을 다 지웠으니 변수명→GUID 맵도 비워 동기화(스테일 엔트리 제거).
        // AddWidget이 새 위젯마다 다시 등록함.
        BP->WidgetVariableNameToGuidMap.Empty();
    }
#endif
}
