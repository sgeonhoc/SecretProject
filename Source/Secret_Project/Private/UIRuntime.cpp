#include "UIRuntime.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/SlateWrapperTypes.h"

void UCardButton::Init(int32 InIndex)
{
    CardIndex = InIndex;
    OnClicked.AddDynamic(this, &UCardButton::HandleClicked);
}

void UCardButton::HandleClicked()
{
    OnCardClicked.Broadcast(CardIndex);
}

static void StyleText(UTextBlock* T, FLinearColor Color, int32 Size, int32 Justify)
{
    if (!T) return;
    T->SetColorAndOpacity(FSlateColor(Color));
    if (Size > 0)
    {
        FSlateFontInfo F = T->GetFont();
        F.Size = Size;
        T->SetFont(F);
    }
    if (Justify >= 0) T->SetJustification(static_cast<ETextJustify::Type>(Justify));
    T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
    T->SetShadowOffset(FVector2D(1.f, 1.f));
}

// 부모 박스에 자식 추가 + 아래 여백
static void AddTo(UPanelWidget* Parent, UWidget* Child, float GapBelow)
{
    if (!Parent || !Child) return;
    if (UVerticalBox* VB = Cast<UVerticalBox>(Parent))
    {
        if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(Child))
            S->SetPadding(FMargin(0.f, 0.f, 0.f, GapBelow));
    }
    else
    {
        Parent->AddChild(Child);
    }
}

void UUIRuntime::Clear(UPanelWidget* Panel)
{
    if (Panel) Panel->ClearChildren();
}

UTextBlock* UUIRuntime::AddText(UPanelWidget* Parent, const FString& Text,
                                FLinearColor Color, int32 Size, int32 Justify,
                                float GapBelow, bool bAutoWrap)
{
    if (!Parent) return nullptr;
    UTextBlock* T = NewObject<UTextBlock>(Parent);
    T->SetText(FText::FromString(Text));
    StyleText(T, Color, Size, Justify);
    if (bAutoWrap) T->SetAutoWrapText(true);
    AddTo(Parent, T, GapBelow);
    return T;
}

UHorizontalBox* UUIRuntime::AddRow(UPanelWidget* Parent, float GapBelow)
{
    if (!Parent) return nullptr;
    UHorizontalBox* HB = NewObject<UHorizontalBox>(Parent);
    AddTo(Parent, HB, GapBelow);
    return HB;
}

UVerticalBox* UUIRuntime::AddCard(UPanelWidget* Parent, FLinearColor Bg, float GapBelow)
{
    if (!Parent) return nullptr;
    UBorder* Card = NewObject<UBorder>(Parent);
    Card->SetBrushColor(Bg);
    Card->SetPadding(FMargin(14.f, 10.f, 14.f, 10.f));
    UVerticalBox* Inner = NewObject<UVerticalBox>(Parent);
    Card->SetContent(Inner);
    AddTo(Parent, Card, GapBelow);
    return Inner;
}

UVerticalBox* UUIRuntime::AddCardFill(UHorizontalBox* Row, FLinearColor Bg)
{
    if (!Row) return nullptr;
    UBorder* Card = NewObject<UBorder>(Row);
    Card->SetBrushColor(Bg);
    Card->SetPadding(FMargin(16.f, 14.f, 16.f, 14.f));
    UVerticalBox* Inner = NewObject<UVerticalBox>(Row);
    Card->SetContent(Inner);
    if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Card))
    {
        FSlateChildSize Sz(ESlateSizeRule::Fill); Sz.Value = 1.f;
        S->SetSize(Sz);
        S->SetVerticalAlignment(VAlign_Fill);
        S->SetPadding(FMargin(6.f, 0.f, 6.f, 0.f));
    }
    return Inner;
}

UTextBlock* UUIRuntime::RowText(UHorizontalBox* Row, const FString& Text,
                                FLinearColor Color, int32 Size, int32 Justify, bool bFill)
{
    if (!Row) return nullptr;
    UTextBlock* T = NewObject<UTextBlock>(Row);
    T->SetText(FText::FromString(Text));
    StyleText(T, Color, Size, Justify);
    if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(T))
    {
        S->SetVerticalAlignment(VAlign_Center);
        if (bFill)
        {
            FSlateChildSize Sz(ESlateSizeRule::Fill); Sz.Value = 1.f;
            S->SetSize(Sz);
        }
        S->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
    }
    return T;
}

UVerticalBox* UUIRuntime::AddClickCard(UPanelWidget* Parent, FLinearColor Bg, int32 Index,
                                       UCardButton*& OutButton, float GapBelow)
{
    OutButton = nullptr;
    if (!Parent) return nullptr;

    UCardButton* Btn = NewObject<UCardButton>(Parent);

    // 기본 버튼 브러시(둥근 박스)를 카드 색으로 틴트 → 카드 모양 + 호버/눌림/비활성 피드백 무료.
    FButtonStyle S = Btn->GetStyle();
    auto Tint = [](FSlateBrush& B, const FLinearColor& C) { B.TintColor = FSlateColor(C); };
    Tint(S.Normal,   Bg);
    Tint(S.Hovered,  FLinearColor(FMath::Min(Bg.R * 1.35f, 1.f), FMath::Min(Bg.G * 1.35f, 1.f), FMath::Min(Bg.B * 1.35f, 1.f), Bg.A));
    Tint(S.Pressed,  FLinearColor(Bg.R * 0.8f, Bg.G * 0.8f, Bg.B * 0.8f, Bg.A));
    Tint(S.Disabled, FLinearColor(Bg.R * 0.5f, Bg.G * 0.5f, Bg.B * 0.55f, Bg.A * 0.7f));
    Btn->SetStyle(S);

    UVerticalBox* Inner = NewObject<UVerticalBox>(Parent);
    Btn->SetContent(Inner);
    Btn->Init(Index);
    AddTo(Parent, Btn, GapBelow);

    OutButton = Btn;
    return Inner;
}

void UUIRuntime::AddBar(UPanelWidget* Parent, float Percent, FLinearColor Fill,
                        float Height, float GapBelow)
{
    if (!Parent) return;
    Percent = FMath::Clamp(Percent, 0.f, 1.f);

    UBorder* Track = NewObject<UBorder>(Parent);
    Track->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f)); // 트랙(어두운 배경)
    Track->SetPadding(FMargin(0.f));

    UHorizontalBox* HB = NewObject<UHorizontalBox>(Parent);
    Track->SetContent(HB);

    UBorder* FillB = NewObject<UBorder>(Parent);
    FillB->SetBrushColor(Fill);
    if (UHorizontalBoxSlot* S1 = HB->AddChildToHorizontalBox(FillB))
    {
        FSlateChildSize Sz(ESlateSizeRule::Fill); Sz.Value = FMath::Max(Percent, 0.0001f);
        S1->SetSize(Sz);
    }
    UBorder* RestB = NewObject<UBorder>(Parent);
    RestB->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f)); // 투명(빈 부분)
    if (UHorizontalBoxSlot* S2 = HB->AddChildToHorizontalBox(RestB))
    {
        FSlateChildSize Sz(ESlateSizeRule::Fill); Sz.Value = FMath::Max(1.f - Percent, 0.0001f);
        S2->SetSize(Sz);
    }

    USizeBox* SB = NewObject<USizeBox>(Parent);
    SB->SetHeightOverride(Height);
    SB->SetContent(Track);
    AddTo(Parent, SB, GapBelow);
}
