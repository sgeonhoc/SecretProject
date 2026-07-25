#include "SalanCardWidget.h"
#include "HexBattleScreen.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Styling/CoreStyle.h"

void USalanCardWidget::Setup(UHexBattleScreen* InOwner, int32 InPoolIndex, const FString& Token, const FString& Meaning, const FLinearColor& InAccent)
{
	Owner = InOwner;
	PoolIndex = InPoolIndex;
	TokenStr = Token;
	MeaningStr = Meaning;
	Accent = InAccent;
	SetVisibility(ESlateVisibility::Visible); // 마우스 이벤트 받게
	if (!bBuilt) { BuildTree(); }
	if (TokenText) { TokenText->SetText(FText::FromString(TokenStr)); }
	if (MeaningText) { MeaningText->SetText(FText::FromString(MeaningStr)); }
	if (Stripe) { Stripe->SetBrushColor(Accent); }
	if (TokenText) { TokenText->SetColorAndOpacity(FSlateColor(FLinearColor::White)); }
	if (MeaningText) { MeaningText->SetColorAndOpacity(FSlateColor(Accent)); }
}

void USalanCardWidget::BuildTree()
{
	if (WidgetTree->RootWidget) { bBuilt = true; return; }

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(118.0f);
	Size->SetHeightOverride(168.0f);
	WidgetTree->RootWidget = Size;

	UOverlay* Ov = WidgetTree->ConstructWidget<UOverlay>();
	Size->AddChild(Ov);

	GlowBorder = WidgetTree->ConstructWidget<UBorder>();
	GlowBorder->SetBrushColor(FLinearColor(0.4f, 1.0f, 0.55f, 0.0f));
	if (UOverlaySlot* GS = Ov->AddChildToOverlay(GlowBorder)) { GS->SetHorizontalAlignment(HAlign_Fill); GS->SetVerticalAlignment(VAlign_Fill); }

	BodyBorder = WidgetTree->ConstructWidget<UBorder>();
	BodyBorder->SetBrushColor(FLinearColor(0.08f, 0.10f, 0.14f, 0.98f));
	if (UOverlaySlot* BS = Ov->AddChildToOverlay(BodyBorder)) { BS->SetHorizontalAlignment(HAlign_Fill); BS->SetVerticalAlignment(VAlign_Fill); BS->SetPadding(FMargin(3.0f)); }

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
	BodyBorder->SetContent(Col);

	// 상단 계열 색띠
	Stripe = WidgetTree->ConstructWidget<UBorder>();
	Stripe->SetBrushColor(Accent);
	Stripe->SetPadding(FMargin(0, 2.5f, 0, 0));
	Col->AddChildToVerticalBox(Stripe);

	// 큰 살란 글자(가운데)
	TokenText = WidgetTree->ConstructWidget<UTextBlock>();
	TokenText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 34));
	TokenText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TokenText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TS = Col->AddChildToVerticalBox(TokenText)) { TS->SetHorizontalAlignment(HAlign_Center); TS->SetVerticalAlignment(VAlign_Center); TS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

	// 뜻(아래)
	MeaningText = WidgetTree->ConstructWidget<UTextBlock>();
	MeaningText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 13));
	MeaningText->SetColorAndOpacity(FSlateColor(Accent));
	MeaningText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* MS = Col->AddChildToVerticalBox(MeaningText)) { MS->SetHorizontalAlignment(HAlign_Center); MS->SetPadding(FMargin(0, 0, 0, 8)); }

	SetRenderTransformPivot(FVector2D(0.5f, 1.0f)); // 바닥-가운데를 축으로 부챗살
	bBuilt = true;
}

void USalanCardWidget::SetPicked(bool bInPicked) { bPicked = bInPicked; }

void USalanCardWidget::PlayDeal(float Delay)
{
	DealDelay = Delay;
	DealTimer = 0.0f;
	bDealing = true;
	CurY = 130.0f; CurScale = 0.75f; CurOpacity = 0.0f;
	SetRenderTranslation(FVector2D(0, CurY));
	SetRenderScale(FVector2D(CurScale, CurScale));
	SetRenderOpacity(0.0f);
}

TSharedRef<SWidget> USalanCardWidget::RebuildWidget()
{
	if (!bBuilt) { BuildTree(); }
	return Super::RebuildWidget();
}

void USalanCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!bBuilt) { BuildTree(); }
}

void USalanCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 목표 상태: 조합에 든 조각은 떠오르고 또렷, 아니면 살짝 가라앉음. 호버는 더 크게.
	float TgtY = 0.0f, TgtScale = 0.95f, TgtOpacity = 0.86f;
	if (bPicked) { TgtY = -26.0f; TgtScale = 1.04f; TgtOpacity = 1.0f; }
	if (bHover)  { TgtY = -40.0f; TgtScale = 1.13f; TgtOpacity = 1.0f; }

	// 부채: 기본은 FanAngle만큼 기울고, 호버·선택 시 똑바로. 딜인 동안 각이 펴진다.
	float TgtAngle = FanAngle;
	if (bHover || bPicked) { TgtAngle = 0.0f; }

	float DealAlpha = 1.0f;
	if (bDealing)
	{
		DealTimer += InDeltaTime;
		const float t = FMath::Clamp((DealTimer - DealDelay) / 0.30f, 0.0f, 1.0f);
		DealAlpha = 1.0f - FMath::Pow(1.0f - t, 3.0f);
		if (t >= 1.0f) { bDealing = false; }
		TgtY = FMath::Lerp(150.0f, TgtY, DealAlpha);
		TgtScale = FMath::Lerp(0.75f, TgtScale, DealAlpha);
		TgtOpacity = FMath::Lerp(0.0f, TgtOpacity, DealAlpha);
		TgtAngle = FanAngle * DealAlpha;
	}
	// 부챗살 아래로 처지는 곡선(가장자리 조각이 살짝 내려앉음)
	if (!bHover && !bPicked) { TgtY += FMath::Abs(FanAngle) * 1.1f; }

	const float S = FMath::Clamp(InDeltaTime * 14.0f, 0.0f, 1.0f);
	CurY = FMath::Lerp(CurY, TgtY, S);
	CurScale = FMath::Lerp(CurScale, TgtScale, S);
	CurOpacity = FMath::Lerp(CurOpacity, TgtOpacity, S);
	CurAngle = FMath::Lerp(CurAngle, TgtAngle, S);
	SetRenderTranslation(FVector2D(0.0f, CurY));
	SetRenderScale(FVector2D(CurScale, CurScale));
	SetRenderTransformAngle(CurAngle);
	SetRenderOpacity(CurOpacity);

	// 본체 밝기 + 글로우: 조합에 들면 초록빛, 호버면 은은
	if (BodyBorder)
	{
		BodyBorder->SetBrushColor(bPicked ? FLinearColor(0.10f, 0.16f, 0.12f, 0.98f) : FLinearColor(0.08f, 0.10f, 0.14f, 0.98f));
	}
	if (GlowBorder)
	{
		float GlowA = 0.0f;
		if (bPicked) { GlowPhase += InDeltaTime * 3.5f; GlowA = 0.5f + 0.3f * FMath::Sin(GlowPhase); }
		else if (bHover) { GlowA = 0.3f; }
		GlowBorder->SetBrushColor(FLinearColor(0.4f, 1.0f, 0.55f, GlowA));
	}
}

void USalanCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bHover = true;
}

void USalanCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bHover = false;
}

FReply USalanCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Owner.IsValid()) { Owner->ToggleSalan(PoolIndex); }
	return FReply::Handled();
}
