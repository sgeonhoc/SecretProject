#include "HexBattleHUD.h"
#include "HexBattlePlayerController.h"
#include "HexGridManager.h"
#include "HexUnit.h"
#include "HexTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

static FLinearColor PathColor(ESalanPath P)
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
static FString PathLabel(ESalanPath P)
{
	switch (P)
	{
	case ESalanPath::Sori: return TEXT("소리길");
	case ESalanPath::Mom:  return TEXT("몸길");
	case ESalanPath::Grim: return TEXT("그림길");
	case ESalanPath::Mae:  return TEXT("매개길");
	default:               return TEXT("");
	}
}

AHexGridManager* AHexBattleHUD::GetGrid()
{
	if (Grid) { return Grid; }
	if (UWorld* W = GetWorld())
	{
		for (TActorIterator<AHexGridManager> It(W); It; ++It) { Grid = *It; break; }
	}
	return Grid;
}

void AHexBattleHUD::DrawUnitPanel(AHexUnit* U, float X, float Y, float W)
{
	if (!U) { return; }
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	const FLinearColor Col = U->bPlayerSide ? FLinearColor(0.23f, 0.53f, 0.88f) : FLinearColor(0.88f, 0.34f, 0.29f);

	DrawText(U->DisplayName, FLinearColor::White, X, Y, Font, 1.0f);
	DrawText(FString::Printf(TEXT("HP %d / %d"), U->HP, U->MaxHP), FLinearColor(0.85f, 0.9f, 0.95f), X + W - 130.0f, Y, Font, 0.9f);

	const float BarY = Y + 20.0f, BarH = 12.0f;
	DrawRect(FLinearColor(0.05f, 0.06f, 0.08f), X, BarY, W, BarH);
	const float Pct = FMath::Clamp((float)U->HP / FMath::Max(1, U->MaxHP), 0.0f, 1.0f);
	DrawRect(Col, X, BarY, W * Pct, BarH);

	FString St;
	if (U->Shield > 0) { St += FString::Printf(TEXT("방패 %d  "), U->Shield); }
	if (U->Slow > 0)   { St += TEXT("둔화  "); }
	if (U->Frozen > 0) { St += TEXT("결빙  "); }
	if (U->bExposed)   { St += TEXT("영창 무방비"); }
	if (!St.IsEmpty()) { DrawText(St, FLinearColor(1.0f, 0.83f, 0.3f), X, BarY + 16.0f, Font, 0.85f); }
}

void AHexBattleHUD::DrawButton(const FString& Label, float X, float Y, float W, float H, int32 Kind, const FLinearColor& Col)
{
	DrawRect(Col, X, Y, W, H);
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f), X, Y, W, 2.0f);
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	DrawText(Label, FLinearColor::White, X + 10.0f, Y + H * 0.5f - 8.0f, Font, 0.95f);

	FHudHit Hit; Hit.Kind = Kind; Hit.Index = -1;
	Hit.Rect = FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H));
	Hits.Add(Hit);
}

void AHexBattleHUD::DrawHUD()
{
	Super::DrawHUD();
	Hits.Reset();

	// UMG 전투 화면(HexBattleScreen)이 떠 있으면, 이 캔버스 HUD는 같은 UI(유닛패널·턴·칸·로그·버튼·손패)를
	// 중복으로 그리게 된다 → 세련된 UMG 위에 조잡한 캔버스가 겹쳐 보이는 원흉. 화면이 있으면 그리지 않는다.
	if (AHexBattlePlayerController* HPC = Cast<AHexBattlePlayerController>(PlayerOwner))
	{
		if (HPC->HasBattleScreen()) { return; }
	}

	AHexGridManager* G = GetGrid();
	if (!G || !Canvas) { return; }

	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	const float SW = Canvas->SizeX, SH = Canvas->SizeY;

	// ── 좌상단: 유닛 패널 ──
	const float PanX = 24.0f, PanW = 300.0f;
	DrawRect(FLinearColor(0.06f, 0.07f, 0.09f, 0.85f), PanX - 12.0f, 16.0f, PanW + 24.0f, 150.0f);
	DrawUnitPanel(G->EnemyUnit, PanX, 30.0f, PanW);
	DrawUnitPanel(G->PlayerUnit, PanX, 105.0f, PanW);

	// ── 라운드/턴/칸 ──
	const FString TurnStr = FString::Printf(TEXT("라운드 %d · %s%s"), G->Round,
		G->bPlayerTurn ? TEXT("내 턴") : TEXT("적 턴"),
		G->IsCharging() ? *FString::Printf(TEXT(" · %s"), *G->ChargeText()) : TEXT(""));
	DrawText(TurnStr, FLinearColor(1.0f, 0.9f, 0.6f), PanX, 178.0f, Font, 1.0f);

	// 칸 도트
	DrawText(TEXT("칸"), FLinearColor::White, PanX, 202.0f, Font, 0.95f);
	for (int32 i = 0; i < G->KanMax; ++i)
	{
		const float dx = PanX + 34.0f + i * 20.0f;
		const bool bOn = i < G->Kan;
		DrawRect(bOn ? FLinearColor(1.0f, 0.83f, 0.3f) : FLinearColor(0.12f, 0.14f, 0.17f), dx, 204.0f, 15.0f, 15.0f);
	}

	// ── 승패 배너 ──
	if (G->bOver)
	{
		const bool bWin = (G->PlayerUnit && G->PlayerUnit->IsAlive());
		DrawText(bWin ? TEXT("★ 승리 — 새 판을 누르세요") : TEXT("★ 패배 — 새 판을 누르세요"),
			bWin ? FLinearColor(0.6f, 1.0f, 0.7f) : FLinearColor(1.0f, 0.5f, 0.5f), SW * 0.5f - 140.0f, 40.0f, Font, 1.4f);
	}

	// ── 우측: 전투 로그 ──
	const float LogX = SW - 380.0f, LogY = 24.0f, LogW = 360.0f;
	DrawRect(FLinearColor(0.05f, 0.06f, 0.08f, 0.8f), LogX - 10.0f, LogY - 6.0f, LogW + 20.0f, 320.0f);
	DrawText(TEXT("전투 기록"), FLinearColor(0.6f, 0.7f, 0.8f), LogX, LogY, Font, 0.9f);
	const TArray<FString>& Lines = G->GetLogLines();
	const int32 Shown = FMath::Min(18, Lines.Num());
	for (int32 i = 0; i < Shown; ++i)
	{
		DrawText(Lines[i], FLinearColor(0.78f, 0.84f, 0.9f), LogX, LogY + 22.0f + i * 16.0f, Font, 0.8f);
	}

	// ── 하단: 버튼 ──
	const float BtnY = SH - 190.0f;
	DrawButton(TEXT("턴 넘기기 (Space)"), 24.0f, BtnY, 160.0f, 30.0f, 1, FLinearColor(0.16f, 0.29f, 0.43f));
	DrawButton(TEXT("선택 해제 (Esc)"), 194.0f, BtnY, 140.0f, 30.0f, 3, FLinearColor(0.14f, 0.16f, 0.2f));
	DrawButton(TEXT("새 판"), 344.0f, BtnY, 90.0f, 30.0f, 2, FLinearColor(0.14f, 0.16f, 0.2f));

	// ── 하단: 손패 카드 ──
	const float CardW = 150.0f, CardH = 66.0f, Gap = 8.0f;
	const float HandY = SH - 150.0f;
	float cx = 24.0f, cy = HandY;
	const int32 Sel = G->GetSelectedCard();
	for (int32 i = 0; i < G->Cards.Num(); ++i)
	{
		const FSalanCard& C = G->Cards[i];
		if (cx + CardW > SW - 24.0f) { cx = 24.0f; cy += CardH + Gap; }

		const bool bCan = G->CanStartCardIndex(i);
		const bool bSel = (Sel == i);
		FLinearColor Bg = FLinearColor(0.07f, 0.09f, 0.12f, 0.95f);
		if (!bCan) { Bg = FLinearColor(0.05f, 0.05f, 0.06f, 0.6f); }
		DrawRect(Bg, cx, cy, CardW, CardH);
		// 좌측 길 색띠
		DrawRect(PathColor(C.Path), cx, cy, 4.0f, CardH);
		if (bSel) { DrawRect(FLinearColor(1.0f, 0.83f, 0.3f), cx, cy, CardW, 3.0f); DrawRect(FLinearColor(1.0f, 0.83f, 0.3f), cx, cy + CardH - 3.0f, CardW, 3.0f); }

		const FLinearColor Ink = bCan ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.55f);
		// 이름 + 칸
		DrawText(C.Name, Ink, cx + 10.0f, cy + 6.0f, Font, 0.9f);
		DrawText(FString::Printf(TEXT("%d칸%s"), C.Cast, C.Cast >= 4 ? TEXT("*") : TEXT("")), FLinearColor(1.0f, 0.83f, 0.3f), cx + CardW - 42.0f, cy + 6.0f, Font, 0.85f);
		// 계열·T
		DrawText(FString::Printf(TEXT("%s·T%d"), *G->ElementLabelPub(C.Element), C.Tier), FLinearColor(0.6f, 0.66f, 0.73f), cx + 10.0f, cy + 26.0f, Font, 0.75f);
		// 길 태그
		DrawText(PathLabel(C.Path), PathColor(C.Path), cx + 10.0f, cy + 44.0f, Font, 0.75f);

		FHudHit Hit; Hit.Kind = 0; Hit.Index = i;
		Hit.Rect = FBox2D(FVector2D(cx, cy), FVector2D(cx + CardW, cy + CardH));
		Hits.Add(Hit);

		cx += CardW + Gap;
	}
}

bool AHexBattleHUD::HitTest(const FVector2D& P, int32& OutKind, int32& OutIndex) const
{
	for (const FHudHit& H : Hits)
	{
		if (H.Rect.bIsValid && H.Rect.IsInside(P))
		{
			OutKind = H.Kind; OutIndex = H.Index; return true;
		}
	}
	return false;
}
