#include "HexBattlePlayerController.h"
#include "HexGridManager.h"
#include "HexBattleScreen.h"
#include "HexTile.h"
#include "HexUnit.h"
#include "EngineUtils.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"

AHexBattlePlayerController::AHexBattlePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AHexBattlePlayerController::BeginPlay()
{
	Super::BeginPlay();
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	GetGrid();

	// UMG 전투 화면 생성(3D 판 위 오버레이). Grid는 화면이 Tick에서 지연 연결.
	if (!Screen)
	{
		Screen = CreateWidget<UHexBattleScreen>(this, UHexBattleScreen::StaticClass());
		if (Screen) { Screen->AddToViewport(0); Screen->Init(GetGrid()); }
	}
}

AHexGridManager* AHexBattlePlayerController::GetGrid()
{
	if (Grid) { return Grid; }
	if (UWorld* W = GetWorld())
	{
		for (TActorIterator<AHexGridManager> It(W); It; ++It) { Grid = *It; break; }
	}
	return Grid;
}

void AHexBattlePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent) { return; }

	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AHexBattlePlayerController::OnClick);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AHexBattlePlayerController::OnEndTurn);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AHexBattlePlayerController::OnDeselect);

	// 숫자키 1~9,0 = 손패 카드 선택(0=10번째)
	InputComponent->BindKey(EKeys::One,   IE_Pressed, this, &AHexBattlePlayerController::Key1);
	InputComponent->BindKey(EKeys::Two,   IE_Pressed, this, &AHexBattlePlayerController::Key2);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AHexBattlePlayerController::Key3);
	InputComponent->BindKey(EKeys::Four,  IE_Pressed, this, &AHexBattlePlayerController::Key4);
	InputComponent->BindKey(EKeys::Five,  IE_Pressed, this, &AHexBattlePlayerController::Key5);
	InputComponent->BindKey(EKeys::Six,   IE_Pressed, this, &AHexBattlePlayerController::Key6);
	InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AHexBattlePlayerController::Key7);
	InputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &AHexBattlePlayerController::Key8);
	InputComponent->BindKey(EKeys::Nine,  IE_Pressed, this, &AHexBattlePlayerController::Key9);
	InputComponent->BindKey(EKeys::Zero,  IE_Pressed, this, &AHexBattlePlayerController::Key0);
}

void AHexBattlePlayerController::Key1() { SelectCardKey(0); }
void AHexBattlePlayerController::Key2() { SelectCardKey(1); }
void AHexBattlePlayerController::Key3() { SelectCardKey(2); }
void AHexBattlePlayerController::Key4() { SelectCardKey(3); }
void AHexBattlePlayerController::Key5() { SelectCardKey(4); }
void AHexBattlePlayerController::Key6() { SelectCardKey(5); }
void AHexBattlePlayerController::Key7() { SelectCardKey(6); }
void AHexBattlePlayerController::Key8() { SelectCardKey(7); }
void AHexBattlePlayerController::Key9() { SelectCardKey(8); }
void AHexBattlePlayerController::Key0() { SelectCardKey(9); }

void AHexBattlePlayerController::SelectCardKey(int32 Index)
{
	// 숫자키 선택도 마우스 클릭과 같은 경로로 — 조합 활성 게이트·발현 연출이 일관되게 적용되도록.
	if (Screen) { Screen->OnCardClicked(Index); return; }
	if (AHexGridManager* G = GetGrid()) { G->SelectCard(Index); }
}

void AHexBattlePlayerController::OnClick()
{
	AHexGridManager* G = GetGrid();
	if (!G) { return; }

	// 손패·버튼은 UMG 카드 위젯이 직접 클릭을 먹는다(여기 안 옴).
	// 여기 오는 클릭 = 판의 빈 곳 → 타일/유닛 커서 트레이스.
	FHitResult Hit;
	if (!GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Hit)) { return; }
	AActor* Actor = Hit.GetActor();
	if (!Actor) { return; }

	if (AHexTile* T = Cast<AHexTile>(Actor)) { G->ClickCoord(T->Coord); return; }
	if (AHexUnit* U = Cast<AHexUnit>(Actor)) { G->ClickCoord(U->Coord); return; }
}

void AHexBattlePlayerController::OnEndTurn()
{
	if (AHexGridManager* G = GetGrid()) { G->RequestEndTurn(); }
}

void AHexBattlePlayerController::OnDeselect()
{
	if (AHexGridManager* G = GetGrid()) { G->Deselect(); }
}
