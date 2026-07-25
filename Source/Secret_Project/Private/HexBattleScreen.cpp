#include "HexBattleScreen.h"
#include "HexCardWidget.h"
#include "SalanCardWidget.h"
#include "HexGridManager.h"
#include "HexUnit.h"
#include "UIRuntime.h" // UCardButton, UIColor
#include "EngineUtils.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Styling/CoreStyle.h"

FString HexCardEffectText(const FSalanCard& C); // HexCardWidget.cpp 정의 재사용

static FString PathMechanic(ESalanPath P)
{
	switch (P)
	{
	case ESalanPath::Sori: return TEXT("소리길(언령) — 과녁은 눈. 벽이 시야를 끊으면 못 건다. 큰 영창은 말문 막기로 끊긴다.");
	case ESalanPath::Mom:  return TEXT("몸길(연공) — 근접·즉발. 잠들지 않아 안 끊긴다.");
	case ESalanPath::Grim: return TEXT("그림길(진) — 미리 새겨 두고(선새김) 적이 밟으면 깬다.");
	case ESalanPath::Mae:  return TEXT("매개길(주술) — 인연으로 벽 뒤도 과녁. 말로 못 끊는다.");
	default:               return TEXT("");
	}
}
static FString ScreenElementLabel(ESalanElement E)
{
	switch (E)
	{
	case ESalanElement::Frost: return TEXT("서리"); case ESalanElement::Fire: return TEXT("불");
	case ESalanElement::Water: return TEXT("물");   case ESalanElement::Wind: return TEXT("바람");
	case ESalanElement::Stone: return TEXT("돌");   case ESalanElement::Body: return TEXT("몸");
	case ESalanElement::Curse: return TEXT("주술"); default: return TEXT("?");
	}
}

UTextBlock* UHexBattleScreen::MakeText(int32 FontSize, const FLinearColor& Col, bool bWrap)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	T->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", FontSize));
	T->SetColorAndOpacity(FSlateColor(Col));
	T->SetAutoWrapText(bWrap);
	T->SetVisibility(ESlateVisibility::HitTestInvisible);
	return T;
}

UButton* UHexBattleScreen::MakeTextButton(const FString& Label, const FLinearColor& Col)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle St = B->GetStyle();
	St.Normal.TintColor = FSlateColor(Col);
	St.Hovered.TintColor = FSlateColor(Col * 1.35f);
	St.Pressed.TintColor = FSlateColor(Col * 0.8f);
	B->SetStyle(St);
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	T->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
	T->SetText(FText::FromString(Label));
	T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UButtonSlot* BS = Cast<UButtonSlot>(B->AddChild(T))) { BS->SetPadding(FMargin(12, 6, 12, 6)); }
	return B;
}

// ── 살란 조합대 ──
void UHexBattleScreen::BuildSalan()
{
	// 손에 든 살란 조각 = 부름말(씨) + 곁말 + 맺음 (전부 캐논 낱말, 신규 창작 0)
	SalanPool = { TEXT("자르"), TEXT("시스"), TEXT("마이"), TEXT("후르"), TEXT("켈"), TEXT("잔"),
	              TEXT("하"), TEXT("케"), TEXT("모"), TEXT("네"), TEXT("누"),
	              TEXT("타"), TEXT("사"), TEXT("노") };
	Combo.Reset();
	RecipeByName.Reset();
	// 언령 기술 → 짜야 하는 살란 토큰(공백 구분). 캐논 문장꼴을 따른다(§4·§5):
	//   부름말(계열 씨앗) + 곁말 꼴지음(하=여럿·케=~처럼·모=과녁·네=하나·누=제외) + 맺음(타=터짐·사=머묾·노=거둠).
	//   맺음은 반드시 하나 붙는다 — 맺음 없는 문장은 문장이 아니다(§4-4).
	// ── 서리(시스) ──
	RecipeByName.Add(TEXT("얼음화살"),          TEXT("시스 타"));       // 서리, 이루라 — 꽂히고 끝
	RecipeByName.Add(TEXT("무릎 서리"),         TEXT("시스 모 사"));    // 과녁의 다리에 머물러 묶음
	RecipeByName.Add(TEXT("손목 서리"),         TEXT("시스 케 타"));    // 채찍처럼 손목을 쳐 떨굼
	RecipeByName.Add(TEXT("고드름 비"),         TEXT("시스 하 타"));    // 여럿이 쏟아져 터짐
	RecipeByName.Add(TEXT("얼음 성벽 세우기"),  TEXT("시스 사"));       // 벽으로 머묾
	RecipeByName.Add(TEXT("내려앉는 겨울"),     TEXT("시스 하 사"));    // 널리, 오래 머묾(대주문)
	// ── 불(자르) ──
	RecipeByName.Add(TEXT("불덩이"),            TEXT("자르 타"));       // §5-1 자르타 — 붙고 끝
	RecipeByName.Add(TEXT("불살"),              TEXT("자르 케 타"));    // 칼처럼 베며 터짐
	RecipeByName.Add(TEXT("불씨 뿌리기"),       TEXT("자르 누 사"));    // 나를 뺀 자리에 남는 불씨(§5-3 누)
	RecipeByName.Add(TEXT("불비"),              TEXT("자르 하 타"));    // 여럿이 쏟아져 터짐
	RecipeByName.Add(TEXT("불지옥"),            TEXT("자르 하 사"));    // 널리, 오래 머묾(대주문)
	// ── 물(마이) ──
	RecipeByName.Add(TEXT("물송곳"),            TEXT("마이 타"));       // 꿰뚫고 끝
	RecipeByName.Add(TEXT("젖은 바닥"),         TEXT("마이 하 사"));    // 여러 물이 바닥에 머묾
	// ── 바람(후르) ──
	RecipeByName.Add(TEXT("바람칼"),            TEXT("후르 타"));       // 베고 끝
	RecipeByName.Add(TEXT("몰이 바람"),         TEXT("후르 모 타"));    // 과녁을 향해 터뜨림
	RecipeByName.Add(TEXT("바람 부치기"),       TEXT("후르 하 노"));    // 여럿을 거두어 쓸어냄
	// ── 돌(켈) ──
	RecipeByName.Add(TEXT("돌팔매"),            TEXT("켈 타"));         // 날아가 부딪고 끝
	RecipeByName.Add(TEXT("돌창"),              TEXT("켈 네 타"));      // 하나로 모아 꿰뚫음
	// ── 소리(잔) ──
	RecipeByName.Add(TEXT("말문 막기"),         TEXT("잔 노"));         // 맺히기 전 소리를 거둠(§6-3)
}

bool UHexBattleScreen::IsActivated(int32 CardIndex) const
{
	if (!Grid || !Grid->Cards.IsValidIndex(CardIndex)) { return false; }
	const FSalanCard& C = Grid->Cards[CardIndex];
	if (C.Path != ESalanPath::Sori) { return true; } // 몸길/진/매개 = 미리 준비 · 조합 불필요
	const FString* Rec = RecipeByName.Find(C.Name);
	if (!Rec) { return true; }
	TArray<FString> Need; Rec->ParseIntoArray(Need, TEXT(" "), true);
	for (const FString& Tok : Need) { if (!Combo.Contains(Tok)) { return false; } }
	return true;
}

FString UHexBattleScreen::ActivatedNames() const
{
	// 지금 짠 조합(레시피 ⊆ Combo)이 켜는 언령 기술 이름을 모은다. Combo가 비면 아무것도 없음.
	TArray<FString> On;
	if (Grid && Combo.Num() > 0)
	{
		for (const FSalanCard& C : Grid->Cards)
		{
			if (C.Path != ESalanPath::Sori) { continue; }
			const FString* Rec = RecipeByName.Find(C.Name);
			if (!Rec) { continue; }
			TArray<FString> Need; Rec->ParseIntoArray(Need, TEXT(" "), true);
			bool bAll = true;
			for (const FString& Tok : Need) { if (!Combo.Contains(Tok)) { bAll = false; break; } }
			if (bAll) { On.AddUnique(C.Name); }
		}
	}
	return FString::Join(On, TEXT(", "));
}

void UHexBattleScreen::RefreshCombo()
{
	// 발현 성공 연출 중에는 조합 텍스트만 손대지 않는다(NativeTick이 되돌림). 칩·기술 활성은 계속 갱신.
	if (ComboText && CastFlash <= 0.0f)
	{
		const FString On = ActivatedNames();
		FString S;
		if (!On.IsEmpty())
		{
			// 여문 문장이 켜졌다 — 화살표와 기술명을 앞세워 확 띄운다.
			S = TEXT("▶  ") + On + TEXT("  켜짐\n조합: ") + FString::Join(Combo, TEXT(" "));
		}
		else if (Combo.Num() > 0)
		{
			S = TEXT("조합: ") + FString::Join(Combo, TEXT(" "));
			// 지금 고른 조각이 모두 어떤 언령 레시피의 부분집합이면 그 문장으로 "향하는 중" — 낚시 대신 길잡이.
			TArray<FString> Toward;
			if (Grid)
			{
				for (const FSalanCard& Cd : Grid->Cards)
				{
					if (Cd.Path != ESalanPath::Sori) { continue; }
					const FString* Rec = RecipeByName.Find(Cd.Name);
					if (!Rec) { continue; }
					TArray<FString> Need; Rec->ParseIntoArray(Need, TEXT(" "), true);
					bool bSubset = true;
					for (const FString& Tok : Combo) { if (!Need.Contains(Tok)) { bSubset = false; break; } }
					if (bSubset && Need.Num() > Combo.Num()) { Toward.AddUnique(Cd.Name); }
				}
			}
			if (Toward.Num() > 0)
			{
				if (Toward.Num() > 3) { Toward.SetNum(3); }
				S += TEXT("\n→ 향하는 문장: ") + FString::Join(Toward, TEXT(", "));
			}
			else
			{
				S += TEXT("\n(이 조각으로 여물 문장이 없다 — 다시 골라 보라)");
			}
		}
		else
		{
			S = TEXT("살란 조각을 눌러 문장을 짜기");
		}
		ComboText->SetText(FText::FromString(S));
		ComboText->SetColorAndOpacity(FSlateColor(On.IsEmpty() ? FLinearColor(0.85f, 0.78f, 0.55f) : FLinearColor(0.55f, 1.0f, 0.70f)));
	}
	// 살란 카드 하이라이트(조합에 든 조각은 떠오르고 켜짐)
	for (int32 i = 0; i < SalanCards.Num(); ++i)
	{
		if (SalanCards[i] && SalanPool.IsValidIndex(i)) { SalanCards[i]->SetPicked(Combo.Contains(SalanPool[i])); }
	}
	RefreshStates(); // 기술 활성 갱신
}

void UHexBattleScreen::ToggleSalan(int32 PoolIndex)
{
	if (!SalanPool.IsValidIndex(PoolIndex)) { return; }
	const FString Tok = SalanPool[PoolIndex];
	if (Combo.Contains(Tok)) { Combo.Remove(Tok); } else { Combo.Add(Tok); } // 토글
	RefreshCombo();
}

void UHexBattleScreen::HandleSalanChip(int32 PoolIndex) { ToggleSalan(PoolIndex); }

void UHexBattleScreen::ClearComboClicked() { Combo.Reset(); RefreshCombo(); }

void UHexBattleScreen::BuildSalanCards()
{
	if (!SalanBox || SalanCards.Num() > 0) { return; } // 한 번만 채운다
	SalanBox->ClearChildren();

	// 살란 조각별 뜻·계열색(캐논 §3: 부름말=계열, 곁말=자리, 맺음=꼬리)
	auto Info = [](const FString& T, FString& M, FLinearColor& A)
	{
		if      (T == TEXT("자르")) { M = TEXT("불");     A = FLinearColor(1.00f, 0.50f, 0.22f); }
		else if (T == TEXT("시스")) { M = TEXT("서리");   A = FLinearColor(0.60f, 0.85f, 1.00f); }
		else if (T == TEXT("마이")) { M = TEXT("물");     A = FLinearColor(0.35f, 0.60f, 1.00f); }
		else if (T == TEXT("후르")) { M = TEXT("바람");   A = FLinearColor(0.60f, 0.95f, 0.75f); }
		else if (T == TEXT("켈"))   { M = TEXT("돌");     A = FLinearColor(0.82f, 0.72f, 0.52f); }
		else if (T == TEXT("잔"))   { M = TEXT("소리");   A = FLinearColor(0.85f, 0.80f, 1.00f); }
		else if (T == TEXT("하"))   { M = TEXT("여럿");   A = FLinearColor(0.62f, 0.68f, 0.78f); }
		else if (T == TEXT("케"))   { M = TEXT("~처럼");  A = FLinearColor(0.62f, 0.68f, 0.78f); }
		else if (T == TEXT("모"))   { M = TEXT("과녁");   A = FLinearColor(0.62f, 0.68f, 0.78f); }
		else if (T == TEXT("네"))   { M = TEXT("하나");   A = FLinearColor(0.62f, 0.68f, 0.78f); }
		else if (T == TEXT("누"))   { M = TEXT("제외");   A = FLinearColor(0.62f, 0.68f, 0.78f); }
		else if (T == TEXT("타"))   { M = TEXT("이루라");  A = FLinearColor(1.00f, 0.83f, 0.30f); }
		else if (T == TEXT("사"))   { M = TEXT("머물라");  A = FLinearColor(1.00f, 0.83f, 0.30f); }
		else if (T == TEXT("노"))   { M = TEXT("거둠");   A = FLinearColor(1.00f, 0.83f, 0.30f); }
		else                        { M = TEXT("");       A = FLinearColor::White; }
	};

	// 카드를 겹쳐 부챗살로 편다(기술 손패와 같은 방식).
	const int32 N = SalanPool.Num();
	const float CardW = 118.0f;
	const float FanW = 640.0f;
	const float Step = (N > 1) ? FMath::Clamp((FanW - CardW) / (N - 1), 30.0f, 118.0f) : 0.0f;
	const float Overlap = CardW - Step;
	const float Mid = (N - 1) * 0.5f;
	const float PerCard = (N > 1) ? FMath::Min(4.5f, 28.0f / (N - 1)) : 0.0f;

	for (int32 i = 0; i < N; ++i)
	{
		USalanCardWidget* Cd = CreateWidget<USalanCardWidget>(this, USalanCardWidget::StaticClass());
		if (!Cd) { continue; }
		FString M; FLinearColor A; Info(SalanPool[i], M, A);
		Cd->Setup(this, i, SalanPool[i], M, A);
		Cd->SetFanAngle((i - Mid) * PerCard);
		Cd->SetPicked(Combo.Contains(SalanPool[i]));
		Cd->PlayDeal(0.03f * i);
		const bool bLast = (i == N - 1);
		if (UHorizontalBoxSlot* HS = SalanBox->AddChildToHorizontalBox(Cd))
		{
			HS->SetPadding(FMargin(0, 0, bLast ? 0.0f : -Overlap, 0));
		}
		SalanCards.Add(Cd);
	}
}

void UHexBattleScreen::FlashCast(const FString& TechName)
{
	// 짠 살란 조각이 기술로 합쳐져 발현하는 순간의 연출. NativeTick이 CastFlash를 감쇠시키며 되돌린다.
	CastFlash = 0.9f;
	CastFlashName = TechName;
	if (ComboText)
	{
		ComboText->SetText(FText::FromString(FString::Printf(TEXT("✦ 발현 —  %s"), *TechName)));
		ComboText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.95f, 0.55f)));
	}
}

void UHexBattleScreen::BuildTree()
{
	if (WidgetTree->RootWidget) { bBuilt = true; return; }

	UOverlay* RootOv = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = RootOv;
	RootOv->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	BuildSalan(); // 살란 조각 풀·레시피

	// 유닛 HP 한 줄을 짓는 헬퍼 — 이름/HP/게이지/상태를 세로로 쌓는다(각 캐릭터 근처 코너에 배치).
	auto UnitPanel = [&](UVerticalBox* V, TObjectPtr<UTextBlock>& NameT, TObjectPtr<UTextBlock>& HpT, TObjectPtr<UProgressBar>& Bar, TObjectPtr<UTextBlock>& StT, const FLinearColor& Fill)
	{
		UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>();
		V->AddChildToVerticalBox(H);
		NameT = MakeText(14, FLinearColor::White);
		if (UHorizontalBoxSlot* NS = H->AddChildToHorizontalBox(NameT)) { NS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
		HpT = MakeText(13, FLinearColor(0.85f, 0.9f, 0.95f));
		H->AddChildToHorizontalBox(HpT);
		Bar = WidgetTree->ConstructWidget<UProgressBar>();
		Bar->SetFillColorAndOpacity(Fill);
		Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
		// 얇은 기본 높이 대신 또렷한 게이지로 — 높이 14px 고정. 한눈에 HP를 읽게 한다.
		USizeBox* BarSz = WidgetTree->ConstructWidget<USizeBox>();
		BarSz->SetHeightOverride(14.0f);
		BarSz->AddChild(Bar);
		if (UVerticalBoxSlot* BS = V->AddChildToVerticalBox(BarSz)) { BS->SetPadding(FMargin(0, 3, 0, 2)); }
		StT = MakeText(11, FLinearColor(1.0f, 0.83f, 0.3f));
		V->AddChildToVerticalBox(StT);
	};

	// ── 우상단: 상대 캐릭터의 HP(상대는 화면 우상단에 선다) ──
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
		Panel->SetBrushColor(FLinearColor(0.09f, 0.05f, 0.06f, 0.82f));
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
		Panel->SetPadding(FMargin(12));
		if (UOverlaySlot* S = RootOv->AddChildToOverlay(Panel)) { S->SetHorizontalAlignment(HAlign_Right); S->SetVerticalAlignment(VAlign_Top); S->SetPadding(FMargin(0, 16, 20, 0)); }
		USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>(); Sz->SetWidthOverride(300.0f); Panel->SetContent(Sz);
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(); Sz->AddChild(V);
		UnitPanel(V, EnemyName, EnemyHpText, EnemyHp, EnemyStatus, FLinearColor(0.88f, 0.34f, 0.29f));
	}

	// ── 좌하단: 아군 캐릭터의 HP(아군은 화면 좌하단에 선다) ──
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
		Panel->SetBrushColor(FLinearColor(0.05f, 0.06f, 0.09f, 0.82f));
		Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
		Panel->SetPadding(FMargin(12));
		if (UOverlaySlot* S = RootOv->AddChildToOverlay(Panel)) { S->SetHorizontalAlignment(HAlign_Left); S->SetVerticalAlignment(VAlign_Bottom); S->SetPadding(FMargin(20, 0, 0, 20)); }
		USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>(); Sz->SetWidthOverride(300.0f); Panel->SetContent(Sz);
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(); Sz->AddChild(V);
		UnitPanel(V, PlayerName, PlayerHpText, PlayerHp, PlayerStatus, FLinearColor(0.23f, 0.53f, 0.88f));
	}

	// ── 상단 중앙: 턴/영창 + 칸 도트 + 행동 버튼 ──
	{
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		V->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UOverlaySlot* S = RootOv->AddChildToOverlay(V)) { S->SetHorizontalAlignment(HAlign_Center); S->SetVerticalAlignment(VAlign_Top); S->SetPadding(FMargin(0, 14, 0, 0)); }
		TurnText = MakeText(15, FLinearColor(1.0f, 0.9f, 0.6f));
		if (UVerticalBoxSlot* TS = V->AddChildToVerticalBox(TurnText)) { TS->SetHorizontalAlignment(HAlign_Center); }
		KanBox = WidgetTree->ConstructWidget<UHorizontalBox>();
		if (UVerticalBoxSlot* KS = V->AddChildToVerticalBox(KanBox)) { KS->SetHorizontalAlignment(HAlign_Center); KS->SetPadding(FMargin(0, 6, 0, 6)); }

		// 대주문 영창 진행 바 — 무방비 창이 얼마나 찼는지 양쪽이 보게. 영창 중에만 뜬다.
		USizeBox* ChSz = WidgetTree->ConstructWidget<USizeBox>();
		ChSz->SetWidthOverride(240.0f); ChSz->SetHeightOverride(6.0f);
		ChargeBar = WidgetTree->ConstructWidget<UProgressBar>();
		ChargeBar->SetVisibility(ESlateVisibility::Collapsed);
		ChSz->AddChild(ChargeBar);
		if (UVerticalBoxSlot* CS2 = V->AddChildToVerticalBox(ChSz)) { CS2->SetHorizontalAlignment(HAlign_Center); CS2->SetPadding(FMargin(0, 0, 0, 4)); }

		UHorizontalBox* Btns = WidgetTree->ConstructWidget<UHorizontalBox>();
		if (UVerticalBoxSlot* BR = V->AddChildToVerticalBox(Btns)) { BR->SetHorizontalAlignment(HAlign_Center); BR->SetPadding(FMargin(0, 4, 0, 0)); }

		// 턴 넘기기 — 주(主) 행동이라 가장 크고 밝게(카드게임 관습). 인라인으로 지어 폰트·패딩을 키운다.
		EndBtn = WidgetTree->ConstructWidget<UButton>();
		{
			const FLinearColor Amber(0.62f, 0.44f, 0.12f);
			FButtonStyle St = EndBtn->GetStyle();
			St.Normal.TintColor  = FSlateColor(Amber);
			St.Hovered.TintColor = FSlateColor(Amber * 1.4f);
			St.Pressed.TintColor = FSlateColor(Amber * 0.8f);
			EndBtn->SetStyle(St);
			EndBtnText = WidgetTree->ConstructWidget<UTextBlock>();
			EndBtnText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16));
			EndBtnText->SetText(FText::FromString(TEXT("턴 넘기기  ▸  Space")));
			EndBtnText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			if (UButtonSlot* EBS = Cast<UButtonSlot>(EndBtn->AddChild(EndBtnText))) { EBS->SetPadding(FMargin(26, 10, 26, 10)); }
		}
		EndBtn->OnClicked.AddDynamic(this, &UHexBattleScreen::EndTurnClicked);
		if (UHorizontalBoxSlot* HS = Btns->AddChildToHorizontalBox(EndBtn)) { HS->SetPadding(FMargin(3, 0, 10, 0)); }

		UButton* DesBtn = MakeTextButton(TEXT("선택 해제 (Esc)"), FLinearColor(0.14f, 0.16f, 0.2f));
		DesBtn->OnClicked.AddDynamic(this, &UHexBattleScreen::DeselectClicked);
		if (UHorizontalBoxSlot* HS = Btns->AddChildToHorizontalBox(DesBtn)) { HS->SetPadding(FMargin(3, 0, 3, 0)); }
		NewBtn = MakeTextButton(TEXT("새 판"), FLinearColor(0.14f, 0.16f, 0.2f));
		NewBtn->OnClicked.AddDynamic(this, &UHexBattleScreen::NewClicked);
		if (UHorizontalBoxSlot* HS = Btns->AddChildToHorizontalBox(NewBtn)) { HS->SetPadding(FMargin(3, 0, 3, 0)); }
	}

	// ── 좌측 가장자리(세로 가운데): 전투 기록 — 코너를 비켜 얇게 ──
	{
		UBorder* LB = WidgetTree->ConstructWidget<UBorder>();
		LB->SetBrushColor(FLinearColor(0.04f, 0.05f, 0.07f, 0.55f));
		LB->SetVisibility(ESlateVisibility::HitTestInvisible);
		LB->SetPadding(FMargin(10));
		if (UOverlaySlot* S = RootOv->AddChildToOverlay(LB)) { S->SetHorizontalAlignment(HAlign_Left); S->SetVerticalAlignment(VAlign_Center); S->SetPadding(FMargin(16, 0, 0, 0)); }
		USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>(); Sz->SetWidthOverride(300.0f); LB->SetContent(Sz);
		UVerticalBox* LV = WidgetTree->ConstructWidget<UVerticalBox>(); Sz->AddChild(LV);
		// 패널 이름표 — 이 영역이 전투 기록임을 알린다(먹돌 톤의 흐린 소제목).
		UTextBlock* LTitle = MakeText(9, FLinearColor(0.5f, 0.55f, 0.62f));
		LTitle->SetText(FText::FromString(TEXT("전 투 기 록")));
		if (UVerticalBoxSlot* TS = LV->AddChildToVerticalBox(LTitle)) { TS->SetPadding(FMargin(0, 0, 0, 4)); }
		// 가장 최근 사건 — 밝고 크게 강조(방금 무슨 일이 났는지 한눈에).
		LogHeadText = MakeText(13, FLinearColor(1.0f, 0.95f, 0.78f), true);
		if (UVerticalBoxSlot* HS = LV->AddChildToVerticalBox(LogHeadText)) { HS->SetPadding(FMargin(0, 0, 0, 5)); }
		// 지난 사건들 — 흐리게 가라앉혀 최신 줄과 구분.
		LogText = MakeText(11, FLinearColor(0.62f, 0.68f, 0.75f), true);
		LV->AddChildToVerticalBox(LogText);
	}

	// ── 화면 상단 가운데: 새 라운드 배너("라운드 N") — 크게 떴다 위로 사라지며 "내 차례다"를 알린다. ──
	{
		TurnBanner = MakeText(42, FLinearColor(1.0f, 0.92f, 0.68f));
		TurnBanner->SetJustification(ETextJustify::Center);
		TurnBanner->SetRenderOpacity(0.0f);
		TurnBanner->SetVisibility(ESlateVisibility::HitTestInvisible); // 클릭은 3D 판으로 통과
		if (UOverlaySlot* S = RootOv->AddChildToOverlay(TurnBanner)) { S->SetHorizontalAlignment(HAlign_Center); S->SetVerticalAlignment(VAlign_Center); }
	}

	// ── 우측 가운데: 상세(호버) 카드 — 카드를 짚으면 떠오름. 정중앙이면 전장을 가리므로 우측으로 비켜 둔다. ──
	{
		InspectBorder = WidgetTree->ConstructWidget<UBorder>();
		InspectBorder->SetBrushColor(FLinearColor(0.08f, 0.10f, 0.14f, 0.96f));
		InspectBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		InspectBorder->SetPadding(FMargin(14));
		InspectBorder->SetRenderOpacity(0.0f);
		if (UOverlaySlot* S = RootOv->AddChildToOverlay(InspectBorder)) { S->SetHorizontalAlignment(HAlign_Right); S->SetVerticalAlignment(VAlign_Center); S->SetPadding(FMargin(0, 0, 28, 0)); }
		USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>(); Sz->SetWidthOverride(340.0f); InspectBorder->SetContent(Sz);
		UVerticalBox* IV = WidgetTree->ConstructWidget<UVerticalBox>();
		Sz->AddChild(IV);
		InspectTitle = MakeText(17, FLinearColor::White);
		IV->AddChildToVerticalBox(InspectTitle);
		InspectBody = MakeText(12, FLinearColor(0.82f, 0.87f, 0.93f), true);
		if (UVerticalBoxSlot* IS = IV->AddChildToVerticalBox(InspectBody)) { IS->SetPadding(FMargin(0, 8, 0, 0)); }
	}

	// ── 좌상단: 기술 카드(손패) + 발현방식 필터 탭 ──
	{
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
		Col->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UOverlaySlot* CS = RootOv->AddChildToOverlay(Col)) { CS->SetHorizontalAlignment(HAlign_Left); CS->SetVerticalAlignment(VAlign_Top); CS->SetPadding(FMargin(48, 14, 0, 0)); }

		// 필터 탭
		FilterBox = WidgetTree->ConstructWidget<UHorizontalBox>();
		Col->AddChildToVerticalBox(FilterBox);
		const TCHAR* FiltNames[5] = { TEXT("전체"), TEXT("소리길"), TEXT("몸길"), TEXT("그림길"), TEXT("매개길") };
		for (int32 i = 0; i < 5; ++i)
		{
			UCardButton* Tab = WidgetTree->ConstructWidget<UCardButton>();
			Tab->Init(i);
			Tab->OnCardClicked.AddDynamic(this, &UHexBattleScreen::HandleFilter);
			UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
			T->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
			T->SetText(FText::FromString(FiltNames[i]));
			T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			FilterLabels.Add(T); // 활성 탭 강조를 위해 라벨을 붙들어 둔다
			if (UButtonSlot* BS = Cast<UButtonSlot>(Tab->AddChild(T))) { BS->SetPadding(FMargin(10, 4, 10, 4)); }
			if (UHorizontalBoxSlot* HS = FilterBox->AddChildToHorizontalBox(Tab)) { HS->SetPadding(FMargin(0, 0, 4, 0)); }
		}

		// 손패 — 좌상단에서 오른쪽으로 펼쳐지되 상대(우상단)를 덮지 않게 너비 제한.
		// 스크롤 없이 카드를 겹쳐 부챗살로 편다(부채 중심이 화면 안에 오도록). 겹침·기울기는 RebuildHand가 장수에 맞춰 준다.
		USizeBox* HandSize = WidgetTree->ConstructWidget<USizeBox>();
		HandSize->SetWidthOverride(1120.0f);
		HandSize->SetHeightOverride(320.0f);
		if (UVerticalBoxSlot* HSS = Col->AddChildToVerticalBox(HandSize)) { HSS->SetPadding(FMargin(0, 8, 0, 0)); }
		HandBox = WidgetTree->ConstructWidget<UHorizontalBox>();
		HandSize->AddChild(HandBox);
	}

	// ── 우하단: 살란 조각 — 손에 쥔 카드 부챗살(조각 카드는 런타임에 채운다) ──
	{
		UVerticalBox* SalanCol = WidgetTree->ConstructWidget<UVerticalBox>();
		SalanCol->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UOverlaySlot* SPS = RootOv->AddChildToOverlay(SalanCol)) { SPS->SetHorizontalAlignment(HAlign_Right); SPS->SetVerticalAlignment(VAlign_Bottom); SPS->SetPadding(FMargin(0, 0, 24, 16)); }

		ComboText = MakeText(14, FLinearColor(1.0f, 0.9f, 0.6f), true);
		ComboText->SetJustification(ETextJustify::Right);
		if (UVerticalBoxSlot* CTS = SalanCol->AddChildToVerticalBox(ComboText)) { CTS->SetHorizontalAlignment(HAlign_Right); CTS->SetPadding(FMargin(0, 0, 4, 6)); }

		// 부챗살 카드가 들어갈 자리. BuildSalanCards()가 채운다.
		USizeBox* SalanSize = WidgetTree->ConstructWidget<USizeBox>();
		SalanSize->SetWidthOverride(680.0f);
		SalanSize->SetHeightOverride(210.0f);
		SalanCol->AddChildToVerticalBox(SalanSize);
		SalanBox = WidgetTree->ConstructWidget<UHorizontalBox>();
		SalanSize->AddChild(SalanBox);

		UButton* ClearBtn = MakeTextButton(TEXT("조합 비우기"), FLinearColor(0.14f, 0.16f, 0.2f));
		ClearBtn->OnClicked.AddDynamic(this, &UHexBattleScreen::ClearComboClicked);
		if (UVerticalBoxSlot* CBS2 = SalanCol->AddChildToVerticalBox(ClearBtn)) { CBS2->SetHorizontalAlignment(HAlign_Right); CBS2->SetPadding(FMargin(0, 6, 4, 0)); }
	}

	RefreshCombo(); // 초기 조합 표시

	bBuilt = true;
}

void UHexBattleScreen::Init(AHexGridManager* InGrid)
{
	if (!bBuilt) { BuildTree(); }
	Grid = InGrid;
	EnsureGrid();
}

void UHexBattleScreen::EnsureGrid()
{
	if (!Grid)
	{
		if (UWorld* W = GetWorld())
		{
			for (TActorIterator<AHexGridManager> It(W); It; ++It) { Grid = *It; break; }
		}
	}
	if (Grid && !bBound)
	{
		Grid->OnChanged.AddDynamic(this, &UHexBattleScreen::HandleChanged);
		bBound = true;
		RefreshStates(); // 첫 드로우 연출은 RefreshStates의 새 턴 감지가 처리
	}
}

bool UHexBattleScreen::CardPassesFilter(int32 CardIndex) const
{
	if (!Grid || !Grid->Cards.IsValidIndex(CardIndex)) { return false; }
	if (FilterIndex == 0) { return true; }
	const ESalanPath P = Grid->Cards[CardIndex].Path;
	switch (FilterIndex)
	{
	case 1: return P == ESalanPath::Sori;
	case 2: return P == ESalanPath::Mom;
	case 3: return P == ESalanPath::Grim;
	case 4: return P == ESalanPath::Mae;
	default: return true;
	}
}

void UHexBattleScreen::RebuildHand()
{
	if (!Grid || !HandBox) { return; }
	HandBox->ClearChildren();
	CardWidgets.Reset();

	// 먼저 보일 카드 수를 센다(겹침 간격을 장수에 맞추기 위해).
	int32 Count = 0;
	for (int32 i = 0; i < Grid->Cards.Num(); ++i) { if (CardPassesFilter(i)) { ++Count; } }

	// 필터를 걸었는데 그 길에 낼 기술이 없으면, 텅 빈 손패가 "멈춘 것"처럼 보인다 → 빈 상태를 말로 알린다.
	if (Count == 0)
	{
		UTextBlock* Empty = MakeText(14, FLinearColor(0.6f, 0.64f, 0.7f));
		Empty->SetText(FText::FromString(FilterIndex == 0
			? TEXT("손에 든 기술이 없다 — 턴을 넘겨 새로 뽑으라")
			: TEXT("이 길에는 지금 낼 기술이 없다 — [전체] 탭으로 돌아가라")));
		if (UHorizontalBoxSlot* HS = HandBox->AddChildToHorizontalBox(Empty)) { HS->SetVerticalAlignment(VAlign_Center); HS->SetPadding(FMargin(12, 40, 0, 0)); }
		return;
	}

	// 카드 폭 196. 손 너비(≈1080) 안에 전부 들어오도록 카드 사이 간격(step)을 정한다.
	// 장수가 적으면 살짝만 겹치고(넓게), 많으면 촘촘히 겹쳐 부챗살처럼 편다.
	const float CardW = 196.0f;
	const float HandW = 1080.0f;
	const float Step = (Count > 1) ? FMath::Clamp((HandW - CardW) / (Count - 1), 34.0f, 150.0f) : 0.0f;
	const float Overlap = CardW - Step; // 다음 카드를 당겨 겹치는 양(음수 패딩)
	const float Mid = (Count - 1) * 0.5f;
	const float PerCard = (Count > 1) ? FMath::Min(5.0f, 30.0f / (Count - 1)) : 0.0f; // 장당 각(도), 총 ≤30°

	int32 Shown = 0;
	for (int32 i = 0; i < Grid->Cards.Num(); ++i)
	{
		if (!CardPassesFilter(i)) { continue; }
		UHexCardWidget* Card = CreateWidget<UHexCardWidget>(this, UHexCardWidget::StaticClass());
		if (!Card) { continue; }
		Card->SetupCard(this, i);
		Card->UpdateFromCard(Grid->Cards[i], Grid->CanStartCardIndex(i), Grid->GetSelectedCard() == i);
		Card->PlayDeal(0.03f * Shown); // 인덱스별 stagger 딜인
		Card->SetFanAngle((Shown - Mid) * PerCard); // 가운데 0, 좌우 대칭으로 기운다
		// 마지막 장 빼고 오른쪽 패딩을 음수로 줘 다음 카드를 끌어당겨 겹친다.
		const bool bLast = (Shown == Count - 1);
		if (UHorizontalBoxSlot* HS = HandBox->AddChildToHorizontalBox(Card))
		{
			HS->SetPadding(FMargin(0, 0, bLast ? 0.0f : -Overlap, 0));
		}
		CardWidgets.Add(Card);
		++Shown;
	}
}

void UHexBattleScreen::RefreshStates()
{
	if (!Grid) { return; }

	// 새 턴(라운드 변화) = 손패 다시 딜인 = 드로우 연출(게임 시작·매 내 턴)
	if (Grid->Round != LastRoundSeen)
	{
		LastRoundSeen = Grid->Round;
		RebuildHand();
		// 새 라운드 배너를 크게 띄운다 — "내 차례다"를 한눈에. NativeTick이 위로 떠오르며 페이드아웃.
		if (TurnBanner && !Grid->bOver)
		{
			TurnBanner->SetText(FText::FromString(FString::Printf(TEXT("라운드 %d"), Grid->Round)));
			BannerTimer = 1.4f;
		}
	}

	// 유닛 패널
	if (Grid->EnemyUnit)
	{
		EnemyName->SetText(FText::FromString(Grid->EnemyUnit->DisplayName));
		EnemyHpText->SetText(FText::FromString(FString::Printf(TEXT("HP %d / %d"), Grid->EnemyUnit->HP, Grid->EnemyUnit->MaxHP)));
		FString St; AHexUnit* U = Grid->EnemyUnit;
		if (U->Shield > 0) St += FString::Printf(TEXT("방패 %d  "), U->Shield);
		if (U->Slow > 0) St += TEXT("둔화  "); if (U->Frozen > 0) St += TEXT("결빙  "); if (U->bExposed) St += TEXT("영창 무방비");
		EnemyStatus->SetText(FText::FromString(St));
	}
	if (Grid->PlayerUnit)
	{
		PlayerName->SetText(FText::FromString(Grid->PlayerUnit->DisplayName));
		PlayerHpText->SetText(FText::FromString(FString::Printf(TEXT("HP %d / %d"), Grid->PlayerUnit->HP, Grid->PlayerUnit->MaxHP)));
		FString St; AHexUnit* U = Grid->PlayerUnit;
		if (U->Shield > 0) St += FString::Printf(TEXT("방패 %d  "), U->Shield);
		if (U->Slow > 0) St += TEXT("둔화  "); if (U->Frozen > 0) St += TEXT("결빙  "); if (U->bExposed) St += TEXT("영창 무방비");
		PlayerStatus->SetText(FText::FromString(St));
	}

	// 턴/영창
	if (TurnText)
	{
		FString S = FString::Printf(TEXT("라운드 %d · %s"), Grid->Round, Grid->bPlayerTurn ? TEXT("내 턴") : TEXT("적 턴"));
		if (Grid->IsCharging()) { S += TEXT(" · ") + Grid->ChargeText(); }
		if (Grid->bOver) { S = (Grid->PlayerUnit && Grid->PlayerUnit->IsAlive()) ? TEXT("★ 승리 — 새 판") : TEXT("★ 패배 — 새 판"); }
		// 카드를 골라 대상을 기다리는 중이면, 판에서 칸을 고르라고 또렷이 안내(클릭 후 막막함 제거).
		bool bAwaitTarget = false;
		const int32 Sel = Grid->GetSelectedCard();
		if (Sel >= 0 && Grid->bPlayerTurn && !Grid->bOver && Grid->Cards.IsValidIndex(Sel)
			&& AHexGridManager::CardNeedsTarget(Grid->Cards[Sel]))
		{
			S += TEXT("   ▷ 판에서 대상 칸을 고르라 (Esc 취소)");
			bAwaitTarget = true;
		}
		TurnText->SetText(FText::FromString(S));
		// 대상 대기 중엔 청록으로 눈에 띄게, 평소엔 은은한 금빛
		TurnText->SetColorAndOpacity(FSlateColor(bAwaitTarget
			? FLinearColor(0.5f, 1.0f, 0.85f) : FLinearColor(1.0f, 0.9f, 0.6f)));
	}

	// 영창 진행 바 — 영창 중에만 뜬다. 내 영창=금빛, 적 영창=붉은(무방비 창을 노리라).
	if (ChargeBar)
	{
		if (Grid->IsCharging())
		{
			ChargeBar->SetVisibility(ESlateVisibility::HitTestInvisible);
			ChargeBar->SetPercent(FMath::Clamp(Grid->ChargeFraction(), 0.0f, 1.0f));
			ChargeBar->SetFillColorAndOpacity(Grid->IsPlayerCharging()
				? FLinearColor(1.0f, 0.83f, 0.30f) : FLinearColor(0.90f, 0.35f, 0.30f));
		}
		else { ChargeBar->SetVisibility(ESlateVisibility::Collapsed); }
	}

	// 턴 넘기기 버튼 — 내 턴에만 활성. 적 턴/영창 중엔 흐리게 잠그고 "적이 두는 중…"으로 바꿔 지금 못 두는 걸 알린다.
	if (EndBtn)
	{
		const bool bMyTurn = Grid->bPlayerTurn && !Grid->bOver && !Grid->IsCharging();
		EndBtn->SetIsEnabled(bMyTurn);
		EndBtn->SetRenderOpacity(bMyTurn ? 1.0f : 0.45f);
		if (EndBtnText)
		{
			EndBtnText->SetText(FText::FromString(
				Grid->bOver ? TEXT("판이 끝났다") :
				Grid->IsCharging() ? TEXT("영창 중…") :
				bMyTurn ? TEXT("턴 넘기기  ▸  Space") : TEXT("적이 두는 중…")));
		}
	}

	// 칸 도트
	if (KanBox)
	{
		if (KanDots.Num() != Grid->KanMax)
		{
			KanBox->ClearChildren(); KanDots.Reset();
			// 앞머리 숫자 표기 — 도트가 많으면 세기 어려우니 "칸 N/M"을 크게 붙인다.
			KanText = MakeText(15, FLinearColor(1.0f, 0.83f, 0.3f));
			if (UHorizontalBoxSlot* TS = KanBox->AddChildToHorizontalBox(KanText)) { TS->SetVerticalAlignment(VAlign_Center); TS->SetPadding(FMargin(0, 0, 10, 0)); }
			for (int32 i = 0; i < Grid->KanMax; ++i)
			{
				UBorder* D = WidgetTree->ConstructWidget<UBorder>();
				D->SetVisibility(ESlateVisibility::HitTestInvisible);
				if (UHorizontalBoxSlot* HS = KanBox->AddChildToHorizontalBox(D)) { HS->SetPadding(FMargin(2, 0, 2, 0)); HS->SetVerticalAlignment(VAlign_Center); }
				KanDots.Add(D);
			}
		}
		// 호버 카드의 완성 칸(Cast) — 칸 트랙에 소모될 칸을 미리 비춘다(비용↔칸 배분 연결).
		int32 PreviewCost = 0;
		if (HoverCardIndex >= 0 && Grid->Cards.IsValidIndex(HoverCardIndex))
		{
			PreviewCost = FMath::Max(0, Grid->Cards[HoverCardIndex].Cast);
		}
		if (KanText)
		{
			FString KS = FString::Printf(TEXT("칸 %d/%d"), Grid->Kan, Grid->KanMax);
			if (PreviewCost > 0) { KS += FString::Printf(TEXT("  (이 기술 %d칸)"), PreviewCost); }
			KanText->SetText(FText::FromString(KS));
		}
		const bool bAfford = PreviewCost <= Grid->Kan;
		for (int32 i = 0; i < KanDots.Num(); ++i)
		{
			const bool bOn = i < Grid->Kan;
			FLinearColor C = bOn ? FLinearColor(1.0f, 0.83f, 0.3f) : FLinearColor(0.12f, 0.14f, 0.17f);
			if (PreviewCost > 0)
			{
				if (bAfford && i >= Grid->Kan - PreviewCost && i < Grid->Kan)
				{
					C = FLinearColor(1.0f, 0.55f, 0.18f); // 소모될 칸 = 주황
				}
				else if (!bAfford && i >= Grid->Kan && i < PreviewCost)
				{
					C = FLinearColor(0.75f, 0.16f, 0.14f); // 모자란 칸 = 붉게(못 냄)
				}
			}
			KanDots[i]->SetBrushColor(C);
			KanDots[i]->SetPadding(FMargin(7, 8, 7, 8));
		}
	}

	// 로그 — 최신 한 줄은 밝게 강조(LogHeadText), 나머지는 흐리게(LogText)
	if (LogText)
	{
		const TArray<FString>& Lines = Grid->GetLogLines(); // index 0 = 가장 최근
		if (LogHeadText) { LogHeadText->SetText(FText::FromString(Lines.Num() > 0 ? Lines[0] : TEXT(""))); }
		FString L; const int32 N = FMath::Min(15, Lines.Num());
		for (int32 i = 1; i < N; ++i) { L += Lines[i]; if (i < N - 1) L += TEXT("\n"); }
		LogText->SetText(FText::FromString(L));
	}

	// 필터 탭 — 지금 보고 있는 길만 금빛으로 또렷하게, 나머지는 흐리게
	for (int32 i = 0; i < FilterLabels.Num(); ++i)
	{
		if (!FilterLabels[i]) { continue; }
		FilterLabels[i]->SetColorAndOpacity(FSlateColor(i == FilterIndex
			? FLinearColor(1.0f, 0.86f, 0.42f) : FLinearColor(0.55f, 0.58f, 0.62f)));
	}

	// 카드 상태(사용 가능/선택)
	for (UHexCardWidget* Card : CardWidgets)
	{
		if (!Card) { continue; }
		const int32 idx = Card->GetCardIndex();
		if (Grid->Cards.IsValidIndex(idx))
		{
			const bool bCast = Grid->CanStartCardIndex(idx) && IsActivated(idx); // 칸 + 살란 조합 활성
			Card->UpdateFromCard(Grid->Cards[idx], bCast, Grid->GetSelectedCard() == idx);
		}
	}
}

void UHexBattleScreen::ShowInspect(int32 CardIndex)
{
	if (!Grid || !Grid->Cards.IsValidIndex(CardIndex)) { return; }
	const FSalanCard& C = Grid->Cards[CardIndex];
	// 지금 낼 수 있는지 판정 — 회색 카드를 짚었을 때 "왜 못 내는지"를 알려준다.
	const bool bActivated = IsActivated(CardIndex);
	const bool bCanStart = Grid->CanStartCardIndex(CardIndex);
	const bool bCastable = bActivated && bCanStart && Grid->bPlayerTurn && !Grid->bOver;
	FString StatusLine;
	if (!Grid->bPlayerTurn || Grid->bOver) { StatusLine = TEXT("✕ 지금은 내 차례가 아니다"); }
	else if (!bActivated)                   { StatusLine = TEXT("✕ 살란 조합으로 문장을 여며야 한다"); }
	else if (C.Cast > Grid->Kan)            { StatusLine = FString::Printf(TEXT("✕ 칸 부족 — %d칸 필요 / %d칸 남음"), C.Cast, Grid->Kan); }
	else if (!bCanStart)                    { StatusLine = TEXT("✕ 지금은 낼 수 없다"); }
	else                                    { StatusLine = TEXT("▶ 지금 낼 수 있다"); }

	if (InspectTitle)
	{
		InspectTitle->SetText(FText::FromString(FString::Printf(TEXT("%s  ·  %d칸"), *C.Name, C.Cast)));
		InspectTitle->SetColorAndOpacity(FSlateColor(bCastable
			? FLinearColor(0.6f, 1.0f, 0.7f) : FLinearColor(1.0f, 0.62f, 0.55f)));
	}
	if (InspectBody)
	{
		FString B;
		B += StatusLine + TEXT("\n");
		B += FString::Printf(TEXT("계열 %s · T%d\n"), *ScreenElementLabel(C.Element), C.Tier);
		B += PathMechanic(C.Path) + TEXT("\n");
		if (C.Cast >= 4) { B += FString::Printf(TEXT("대주문 — %d칸 영창(완성까지 무방비, 피해 ×1.5)\n"), C.Cast); }
		if (C.Range > 0 && C.Type != ECardType::Defend) { B += FString::Printf(TEXT("사거리 %d%s\n"), C.Range, C.bThrough ? TEXT(" · 벽 뒤 가능") : TEXT("")); }
		B += TEXT("효과: ") + HexCardEffectText(C);
		if (C.ComboBonus > 0) { B += FString::Printf(TEXT("\n합: 맞는 표면 위면 %d"), C.ComboBonus); }
		InspectBody->SetText(FText::FromString(B));
	}
	InspectTargetOpacity = 1.0f;
}

void UHexBattleScreen::OnCardHover(int32 CardIndex)
{
	HoverCardIndex = CardIndex; // 칸 트랙에 이 카드의 칸 소모를 미리 비추기 위함
	// 카드에서 마우스가 벗어나면(-1) 상세 패널을 접는다 — 안 그러면 마지막 카드가 영영 떠 있는다.
	if (CardIndex < 0) { InspectTargetOpacity = 0.0f; }
	// 호버한 카드의 좌우 이웃을 바깥으로 밀어, 겹친 부챗살에서 호버 카드가 이웃에 안 가리게 한다(하스스톤식).
	// 손패는 화면 위치(hand index) 순이므로 CardWidgets 배열 순서로 좌우를 가른다.
	int32 HoverSlot = INDEX_NONE;
	for (int32 s = 0; s < CardWidgets.Num(); ++s)
	{
		if (CardWidgets[s] && CardWidgets[s]->GetCardIndex() == CardIndex) { HoverSlot = s; break; }
	}
	const float Spread = 46.0f; // 바로 옆 카드가 비켜서는 양(px)
	for (int32 s = 0; s < CardWidgets.Num(); ++s)
	{
		if (!CardWidgets[s]) { continue; }
		if (HoverSlot == INDEX_NONE) { CardWidgets[s]->SetShove(0.0f); continue; }
		const int32 d = s - HoverSlot;
		if (d == 0) { CardWidgets[s]->SetShove(0.0f); }
		else { CardWidgets[s]->SetShove(FMath::Sign(d) * Spread / FMath::Sqrt((float)FMath::Abs(d))); } // 멀수록 완만히 감쇠
	}
}

void UHexBattleScreen::OnCardClicked(int32 CardIndex)
{
	EnsureGrid();
	if (!Grid) { return; }
	// 못 내는 카드(조합 안 됨·칸 부족·내 턴 아님)를 누르면 조용히 무시하지 말고 흔들어 알린다(설계 §5).
	const bool bCanCast = IsActivated(CardIndex) && Grid->CanStartCardIndex(CardIndex) && Grid->bPlayerTurn && !Grid->bOver;
	if (!bCanCast)
	{
		for (UHexCardWidget* Cw : CardWidgets)
		{
			if (Cw && Cw->GetCardIndex() == CardIndex) { Cw->PlayReject(); break; }
		}
		return;
	}
	Grid->SelectCard(CardIndex);
	// 언령은 조합을 소모(짠 살란이 그 기술로 발현됨) — 소모 전에 발현 연출을 띄운다
	if (Grid->Cards.IsValidIndex(CardIndex) && Grid->Cards[CardIndex].Path == ESalanPath::Sori)
	{
		FlashCast(Grid->Cards[CardIndex].Name);
		Combo.Reset();
	}
	RefreshStates();
}

void UHexBattleScreen::HandleChanged() { RefreshStates(); }
void UHexBattleScreen::HandleFilter(int32 Index) { FilterIndex = Index; RebuildHand(); RefreshStates(); }
void UHexBattleScreen::EndTurnClicked() { EnsureGrid(); if (Grid) { Grid->RequestEndTurn(); } }
void UHexBattleScreen::NewClicked() { EnsureGrid(); if (Grid) { Grid->Restart(); LastRoundSeen = -1; RefreshStates(); } }
void UHexBattleScreen::DeselectClicked() { EnsureGrid(); if (Grid) { Grid->Deselect(); RefreshStates(); } }

TSharedRef<SWidget> UHexBattleScreen::RebuildWidget()
{
	if (!bBuilt) { BuildTree(); }
	return Super::RebuildWidget();
}

void UHexBattleScreen::NativeConstruct()
{
	if (!bBuilt) { BuildTree(); }
	Super::NativeConstruct();
	// 부모(UPersonaWidgetBase) 등장 연출이 opacity를 0에 두고 멈추는 경우 방지 — 항상 보이게.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetRenderOpacity(1.0f);
	BuildSalanCards(); // 우하단 살란 조각 부챗살 채우기(런타임 CreateWidget)
	RefreshCombo();
	EnsureGrid();
}

void UHexBattleScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bBound) { EnsureGrid(); }

	// HP 게이지 부드럽게 + 위급(30% 아래) 붉은 맥동
	const float S = FMath::Clamp(InDeltaTime * 8.0f, 0.0f, 1.0f);
	HpPulsePhase += InDeltaTime * 5.5f;
	// Frac=목표 HP비, Shown=지금 그려지는 비. 바가 줄어드는 동안(Shown>Frac)엔 하얗게 번쩍여 타격감을 준다.
	auto ApplyHpColor = [&](UProgressBar* Bar, float Frac, float Shown, const FLinearColor& Base)
	{
		FLinearColor Col = Base;
		if (Frac < 0.30f)
		{
			const float P = 0.5f + 0.5f * FMath::Sin(HpPulsePhase);          // 0..1 맥동
			Col = FMath::Lerp(Base, FLinearColor(1.0f, 0.13f, 0.10f), 0.45f + 0.45f * P);
		}
		const float Drain = FMath::Clamp((Shown - Frac) * 3.0f, 0.0f, 0.8f); // 줄어드는 중일수록 하얗게
		Col = FMath::Lerp(Col, FLinearColor::White, Drain);
		Bar->SetFillColorAndOpacity(Col);
	};
	if (Grid && Grid->EnemyUnit && EnemyHp)
	{
		const float Tgt = FMath::Clamp((float)Grid->EnemyUnit->HP / FMath::Max(1, Grid->EnemyUnit->MaxHP), 0.0f, 1.0f);
		EnemyHpShown = FMath::Lerp(EnemyHpShown, Tgt, S); EnemyHp->SetPercent(EnemyHpShown);
		ApplyHpColor(EnemyHp, Tgt, EnemyHpShown, FLinearColor(0.88f, 0.34f, 0.29f));
	}
	if (Grid && Grid->PlayerUnit && PlayerHp)
	{
		const float Tgt = FMath::Clamp((float)Grid->PlayerUnit->HP / FMath::Max(1, Grid->PlayerUnit->MaxHP), 0.0f, 1.0f);
		PlayerHpShown = FMath::Lerp(PlayerHpShown, Tgt, S); PlayerHp->SetPercent(PlayerHpShown);
		ApplyHpColor(PlayerHp, Tgt, PlayerHpShown, FLinearColor(0.23f, 0.53f, 0.88f));
	}

	// 판이 끝나면 "새 판"이 자연스러운 다음 행동 → 맥동 + 밝기로 눈에 띄게. 진행 중엔 조용히.
	if (NewBtn)
	{
		if (Grid && Grid->bOver)
		{
			const float P = 0.5f + 0.5f * FMath::Sin(HpPulsePhase);
			NewBtn->SetRenderScale(FVector2D(1.0f + 0.06f * P, 1.0f + 0.06f * P));
			NewBtn->SetRenderOpacity(0.85f + 0.15f * P);
		}
		else { NewBtn->SetRenderScale(FVector2D(1.0f, 1.0f)); NewBtn->SetRenderOpacity(1.0f); }
	}

	// 새 라운드 배너 — 팟 튀어나오며 위로 떠오르다 페이드아웃
	if (TurnBanner)
	{
		if (BannerTimer > 0.0f)
		{
			BannerTimer = FMath::Max(0.0f, BannerTimer - InDeltaTime);
			const float Elapsed = 1.4f - BannerTimer;                          // 0 → 1.4
			const float FadeIn = FMath::Clamp(Elapsed / 0.15f, 0.0f, 1.0f);    // 첫 0.15초 등장
			const float FadeOut = FMath::Clamp(BannerTimer / 0.5f, 0.0f, 1.0f);// 마지막 0.5초 소멸
			TurnBanner->SetRenderOpacity(FMath::Min(FadeIn, FadeOut));
			TurnBanner->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(24.0f, -34.0f, Elapsed / 1.4f)));
			const float Pop = 1.0f + 0.18f * (1.0f - FMath::Clamp(Elapsed / 0.25f, 0.0f, 1.0f)); // 초반 살짝 큰
			TurnBanner->SetRenderScale(FVector2D(Pop, Pop));
		}
		else { TurnBanner->SetRenderOpacity(0.0f); }
	}

	// 상세 패널 페이드
	InspectCurOpacity = FMath::Lerp(InspectCurOpacity, InspectTargetOpacity, FMath::Clamp(InDeltaTime * 10.0f, 0.0f, 1.0f));
	if (InspectBorder) { InspectBorder->SetRenderOpacity(InspectCurOpacity); }

	// 발현 성공 연출 감쇠 — 다 타면 조합 표시(빈 조합)로 되돌린다
	if (CastFlash > 0.0f)
	{
		CastFlash -= InDeltaTime;
		if (CastFlash <= 0.0f) { CastFlash = 0.0f; CastFlashName.Reset(); RefreshCombo(); }
	}

	// 루트는 항상 보이게(부모 등장 연출이 opacity를 0에 묶어두는 문제 방지)
	SetRenderOpacity(1.0f);
}
