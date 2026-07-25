#include "HexCardWidget.h"
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

static FLinearColor CardPathColor(ESalanPath P)
{
	switch (P)
	{
	case ESalanPath::Sori: return FLinearColor(0.62f, 0.80f, 1.00f);
	case ESalanPath::Mom:  return FLinearColor(1.00f, 0.83f, 0.30f);
	case ESalanPath::Grim: return FLinearColor(0.78f, 0.64f, 1.00f);
	case ESalanPath::Mae:  return FLinearColor(1.00f, 0.60f, 0.82f);
	default:               return FLinearColor::White;
	}
}
static FString CardPathLabel(ESalanPath P)
{
	switch (P)
	{
	case ESalanPath::Sori: return TEXT("소리길 · 과녁=눈");
	case ESalanPath::Mom:  return TEXT("몸길 · 근접·안 끊김");
	case ESalanPath::Grim: return TEXT("그림길 · 선새김");
	case ESalanPath::Mae:  return TEXT("매개길 · 벽 뒤 우회");
	default:               return TEXT("");
	}
}
static FString CardElementLabel(ESalanElement E)
{
	switch (E)
	{
	case ESalanElement::Frost: return TEXT("서리");
	case ESalanElement::Fire:  return TEXT("불");
	case ESalanElement::Water: return TEXT("물");
	case ESalanElement::Wind:  return TEXT("바람");
	case ESalanElement::Stone: return TEXT("돌");
	case ESalanElement::Body:  return TEXT("몸");
	case ESalanElement::Curse: return TEXT("주술");
	default:                   return TEXT("?");
	}
}
// 마디(T)→등급 이름(디자인 §15의 테두리색과 짝) — 카드 앞면에 등급을 글자로도 알린다.
static FString CardGradeName(int32 Tier)
{
	if (Tier <= 2)      return TEXT("매직");
	else if (Tier <= 4) return TEXT("레어");
	else if (Tier == 5) return TEXT("스페셜");
	else if (Tier <= 7) return TEXT("유니크");
	else if (Tier == 8) return TEXT("전설");
	else                return TEXT("에픽");
}
FString HexCardEffectText(const FSalanCard& C)
{
	switch (C.Type)
	{
	case ECardType::Strike:     return FString::Printf(TEXT("직격 %d%s%s"), C.Damage, C.ComboBonus > 0 ? *FString::Printf(TEXT(" / 합 %d"), C.ComboBonus) : TEXT(""), C.Aoe > 0 ? TEXT(" · 광역") : TEXT(""));
	case ECardType::GreatSpell: return C.GreatKind == 1 ? TEXT("판에 겨울(서리·결빙·둔화)") : FString::Printf(TEXT("대주문 직격 %d / 합 %d · 광역"), C.Damage, C.ComboBonus);
	case ECardType::Control:    return FString::Printf(TEXT("걸음 느림 %d턴"), C.SlowDur);
	case ECardType::Surface:    return TEXT("표면 깔기");
	case ECardType::Move:       return FString::Printf(TEXT("이동 %d칸"), C.MoveRange);
	case ECardType::Defend:     return FString::Printf(TEXT("방패 %d"), C.Shield);
	case ECardType::Wall:       return C.bWallRing ? TEXT("둘레 얼음벽") : TEXT("벽 1칸");
	case ECardType::JinTrap:    return FString::Printf(TEXT("잠든 진 — 밟으면 %s%s"), C.FreezeDur > 0 ? TEXT("결빙") : TEXT("둔화"), C.TrapDamage > 0 ? *FString::Printf(TEXT("+%d"), C.TrapDamage) : TEXT(""));
	case ECardType::Push:       return FString::Printf(TEXT("밀침 %d칸"), C.Knock);
	case ECardType::WindSweep:  return TEXT("불→번짐불 / 표면 걷기");
	case ECardType::Disarm:     return TEXT("무장 떨굼");
	case ECardType::Interrupt:  return TEXT("적 언령/진 영창 끊기");
	default:                    return TEXT("");
	}
}

void UHexCardWidget::SetupCard(UHexBattleScreen* InOwner, int32 InCardIndex)
{
	Owner = InOwner;
	CardIndex = InCardIndex;
	SetVisibility(ESlateVisibility::Visible); // 마우스 이벤트 받게
	if (!bBuilt) { BuildTree(); }
}

void UHexCardWidget::BuildTree()
{
	if (WidgetTree->RootWidget) { bBuilt = true; return; }

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(196.0f);
	Size->SetHeightOverride(280.0f);
	WidgetTree->RootWidget = Size;

	UOverlay* Ov = WidgetTree->ConstructWidget<UOverlay>();
	Size->AddChild(Ov);

	// 글로우(선택·호버 시 테두리)
	GlowBorder = WidgetTree->ConstructWidget<UBorder>();
	GlowBorder->SetBrushColor(FLinearColor(1.0f, 0.83f, 0.3f, 0.0f));
	if (UOverlaySlot* GS = Ov->AddChildToOverlay(GlowBorder)) { GS->SetHorizontalAlignment(HAlign_Fill); GS->SetVerticalAlignment(VAlign_Fill); }

	// 본체
	BodyBorder = WidgetTree->ConstructWidget<UBorder>();
	BodyBorder->SetBrushColor(FLinearColor(0.07f, 0.09f, 0.12f, 0.97f));
	if (UOverlaySlot* BS = Ov->AddChildToOverlay(BodyBorder)) { BS->SetHorizontalAlignment(HAlign_Fill); BS->SetVerticalAlignment(VAlign_Fill); BS->SetPadding(FMargin(3.0f)); }

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	BodyBorder->SetContent(Row);

	// 좌측 길 색띠
	Stripe = WidgetTree->ConstructWidget<UBorder>();
	Stripe->SetBrushColor(PathCol);
	if (UHorizontalBoxSlot* SS = Row->AddChildToHorizontalBox(Stripe)) { SS->SetPadding(FMargin(0, 0, 6, 0)); }
	Stripe->SetPadding(FMargin(2.0f, 0, 0, 0));

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
	if (UHorizontalBoxSlot* CS = Row->AddChildToHorizontalBox(Col)) { CS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); CS->SetPadding(FMargin(4, 8, 6, 8)); }

	auto MkText = [&](int32 FontSize, const FLinearColor& C) -> UTextBlock*
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo F = FCoreStyle::GetDefaultFontStyle("Regular", FontSize);
		T->SetFont(F);
		T->SetColorAndOpacity(FSlateColor(C));
		T->SetAutoWrapText(true);
		return T;
	};

	// 이름 + 칸(같은 줄). 칸은 왼쪽 위 검은 원 배지에 흰 숫자로 가장 크게(디자인 §2).
	UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>();
	Col->AddChildToVerticalBox(Head);

	UBorder* CostBadge = WidgetTree->ConstructWidget<UBorder>();
	CostBadge->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.98f));
	CostBadge->SetPadding(FMargin(9.0f, 1.0f, 9.0f, 3.0f));
	CostBadge->SetVerticalAlignment(VAlign_Center);
	CostBadge->SetHorizontalAlignment(HAlign_Center);
	CostText = MkText(22, FLinearColor::White);
	CostText->SetJustification(ETextJustify::Center);
	CostText->SetAutoWrapText(false);
	CostBadge->SetContent(CostText);
	if (UHorizontalBoxSlot* KS = Head->AddChildToHorizontalBox(CostBadge)) { KS->SetPadding(FMargin(0, 0, 6, 0)); KS->SetVerticalAlignment(VAlign_Center); }

	NameText = MkText(15, FLinearColor::White);
	NameText->SetAutoWrapText(true);
	if (UHorizontalBoxSlot* NS = Head->AddChildToHorizontalBox(NameText)) { NS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); NS->SetVerticalAlignment(VAlign_Center); }

	MetaText = MkText(10, FLinearColor(0.60f, 0.66f, 0.73f));
	if (UVerticalBoxSlot* MS = Col->AddChildToVerticalBox(MetaText)) { MS->SetPadding(FMargin(0, 3, 0, 0)); }

	EffectText = MkText(11, FLinearColor(0.80f, 0.85f, 0.9f));
	if (UVerticalBoxSlot* ES = Col->AddChildToVerticalBox(EffectText)) { ES->SetPadding(FMargin(0, 6, 0, 0)); ES->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

	PathText = MkText(10, PathCol);
	if (UVerticalBoxSlot* PS = Col->AddChildToVerticalBox(PathText)) { PS->SetPadding(FMargin(0, 4, 0, 0)); }

	SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
	bBuilt = true;
}

void UHexCardWidget::UpdateFromCard(const FSalanCard& Card, bool bInCastable, bool bInSelected)
{
	if (!bBuilt) { BuildTree(); }
	bCastable = bInCastable;
	bSelected = bInSelected;
	PathCol = CardPathColor(Card.Path);

	// 마디(T)→등급 테두리 색(디자인 §15): 1~2 매직 파랑, 3~4 레어 노랑, 5 스페셜 초록, 6~7 유니크 보라, 8 전설 주황, 9+ 에픽 빨강.
	if (Card.Tier <= 2)      TierCol = FLinearColor(0.29f, 0.50f, 0.83f);
	else if (Card.Tier <= 4) TierCol = FLinearColor(0.83f, 0.71f, 0.29f);
	else if (Card.Tier == 5) TierCol = FLinearColor(0.29f, 0.83f, 0.50f);
	else if (Card.Tier <= 7) TierCol = FLinearColor(0.61f, 0.29f, 0.83f);
	else if (Card.Tier == 8) TierCol = FLinearColor(0.83f, 0.50f, 0.29f);
	else                     TierCol = FLinearColor(0.83f, 0.29f, 0.29f);

	if (NameText)   { NameText->SetText(FText::FromString(Card.Name)); }
	if (CostText)   { CostText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Card.Cast))); }
	if (MetaText)   { MetaText->SetText(FText::FromString(FString::Printf(TEXT("%s · %s · T%d"), *CardElementLabel(Card.Element), *CardGradeName(Card.Tier), Card.Tier))); MetaText->SetColorAndOpacity(FSlateColor(TierCol)); }
	if (EffectText) { EffectText->SetText(FText::FromString(HexCardEffectText(Card))); }
	if (PathText)   { PathText->SetText(FText::FromString(CardPathLabel(Card.Path))); PathText->SetColorAndOpacity(FSlateColor(PathCol)); }
	if (Stripe)     { Stripe->SetBrushColor(PathCol); }
	if (BodyBorder) { BodyBorder->SetBrushColor(bCastable ? FLinearColor(0.08f, 0.10f, 0.14f, 0.97f) : FLinearColor(0.05f, 0.05f, 0.06f, 0.9f)); }
}

void UHexCardWidget::PlayDeal(float Delay)
{
	DealDelay = Delay;
	DealTimer = 0.0f;
	bDealing = true;
	CurY = 140.0f; CurScale = 0.75f; CurOpacity = 0.0f;
	SetRenderTranslation(FVector2D(0, CurY));
	SetRenderScale(FVector2D(CurScale, CurScale));
	SetRenderOpacity(0.0f);
}

void UHexCardWidget::PlayReject()
{
	RejectTimer = 0.4f; // NativeTick이 좌우 흔들림 + 붉은 테두리로 표현하고 감쇠시킨다
}

TSharedRef<SWidget> UHexCardWidget::RebuildWidget()
{
	if (!bBuilt) { BuildTree(); }
	return Super::RebuildWidget();
}

void UHexCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!bBuilt) { BuildTree(); }
}

void UHexCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 목표 상태 계산
	float TgtY = 0.0f, TgtScale = 0.94f, TgtOpacity = 0.55f;
	if (bCastable) { TgtY = -10.0f; TgtScale = 1.0f; TgtOpacity = 1.0f; }
	if (bSelected) { TgtY = -30.0f; TgtScale = 1.08f; TgtOpacity = 1.0f; } // 고른 카드는 손패에서 떠올라 계속 보인다
	if (bHover)    { TgtY = -40.0f; TgtScale = 1.14f; TgtOpacity = 1.0f; } // 호버가 가장 강한 리프트

	// 딜인 진행(패에 들어오는 연출): stagger delay 후 아래→집, 페이드
	float DealAlpha = 1.0f;
	if (bDealing)
	{
		DealTimer += InDeltaTime;
		const float t = FMath::Clamp((DealTimer - DealDelay) / 0.32f, 0.0f, 1.0f);
		DealAlpha = 1.0f - FMath::Pow(1.0f - t, 3.0f); // ease-out
		if (t >= 1.0f) { bDealing = false; }
		TgtY = FMath::Lerp(150.0f, TgtY, DealAlpha);
		TgtScale = FMath::Lerp(0.75f, TgtScale, DealAlpha);
		TgtOpacity = FMath::Lerp(0.0f, TgtOpacity, DealAlpha);
	}

	// 부채꼴: 기본은 FanAngle만큼 기울고, 호버·선택 시 똑바로 세워 읽기 좋게. 딜인 동안은 각이 펴진다.
	float TgtAngle = FanAngle;
	if (bHover || bSelected) { TgtAngle = 0.0f; }
	else if (bDealing) { TgtAngle = FanAngle * DealAlpha; }
	// 부챗살 아래로 처지는 곡선(가장자리 카드가 살짝 내려앉음) — 각이 클수록 아래로
	if (!bHover && !bSelected) { TgtY += FMath::Abs(FanAngle) * 1.1f; }

	// 부드럽게 추종
	const float S = FMath::Clamp(InDeltaTime * 14.0f, 0.0f, 1.0f);
	CurY = FMath::Lerp(CurY, TgtY, S);
	CurScale = FMath::Lerp(CurScale, TgtScale, S);
	CurOpacity = FMath::Lerp(CurOpacity, TgtOpacity, S);
	CurAngle = FMath::Lerp(CurAngle, TgtAngle, S);
	CurShoveX = FMath::Lerp(CurShoveX, ShoveTargetX, S); // 이웃 호버 시 옆으로 비켜섬(가림 방지)
	// 거부 흔들림: 남은 시간에 비례해 좌우로 진동(빠르게 잦아듦) — "못 낸다"를 몸으로 알린다.
	float ShakeX = 0.0f;
	if (RejectTimer > 0.0f)
	{
		RejectTimer = FMath::Max(0.0f, RejectTimer - InDeltaTime);
		ShakeX = FMath::Sin(RejectTimer * 52.0f) * 11.0f * (RejectTimer / 0.4f);
	}
	SetRenderTranslation(FVector2D(CurShoveX + ShakeX, CurY));
	SetRenderScale(FVector2D(CurScale, CurScale));
	SetRenderTransformAngle(CurAngle);
	SetRenderOpacity(CurOpacity);

	// 테두리: 평소엔 등급 색이 은은히 늘 보이고(손패에서 좋은 카드 한눈에), 호버 시 밝아지고, 선택 시 금빛 맥동.
	if (GlowBorder)
	{
		if (RejectTimer > 0.0f)
		{
			// 거부 순간엔 붉은 테두리 한 번(설계 §5) — 흔들림과 함께 감쇠.
			GlowBorder->SetBrushColor(FLinearColor(0.95f, 0.20f, 0.16f, RejectTimer / 0.4f));
		}
		else if (bSelected)
		{
			GlowPhase += InDeltaTime * 4.0f;
			const float GlowA = 0.55f + 0.35f * FMath::Sin(GlowPhase);
			GlowBorder->SetBrushColor(FLinearColor(1.0f, 0.83f, 0.3f, GlowA));
		}
		else
		{
			// 등급 색 기본 프레임: 평소 0.28, 호버 0.7. 못 쓰는 카드는 흐리게.
			float Base = bHover ? 0.70f : 0.28f;
			if (!bCastable) { Base *= 0.4f; }
			GlowBorder->SetBrushColor(FLinearColor(TierCol.R, TierCol.G, TierCol.B, Base));
		}
	}
}

void UHexCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bHover = true;
	if (Owner.IsValid()) { Owner->ShowInspect(CardIndex); Owner->OnCardHover(CardIndex); }
}

void UHexCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bHover = false;
	if (Owner.IsValid()) { Owner->OnCardHover(-1); }
}

FReply UHexCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Owner.IsValid()) { Owner->OnCardClicked(CardIndex); }
	return FReply::Handled();
}
