#include "HexBattleGameMode.h"
#include "HexGridManager.h"
#include "HexUnit.h"
#include "HexBattlePlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/DefaultPawn.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Kismet/GameplayStatics.h"

AHexBattleGameMode::AHexBattleGameMode()
{
	DefaultPawnClass = ADefaultPawn::StaticClass();
	PlayerControllerClass = AHexBattlePlayerController::StaticClass();
	// HUD는 UMG(UHexBattleScreen)로 — Canvas HUD 미사용(기본 AHUD, 아무것도 안 그림).
}

void AHexBattleGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* W = GetWorld();
	if (!W) { return; }

	FActorSpawnParameters Sp;

	// 빈 레벨도 바로 보이게 조명 절차 생성(디렉셔널 + 스카이). 나중에 레벨에 직접 배치하면 이 코드는 지워도 됨.
	if (ADirectionalLight* Sun = W->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0, 0, 2000), FRotator(-55.0f, -40.0f, 0.0f), Sp))
	{
		if (UDirectionalLightComponent* LC = Cast<UDirectionalLightComponent>(Sun->GetLightComponent())) { LC->SetIntensity(6.0f); }
	}
	if (ASkyLight* Sky = W->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector(0, 0, 2000), FRotator::ZeroRotator, Sp))
	{
		if (USkyLightComponent* SC = Sky->GetLightComponent())
		{
			SC->SetIntensity(1.0f);
			SC->SetLightColor(FLinearColor(0.6f, 0.7f, 0.9f));
			SC->SetMobility(EComponentMobility::Movable);
			SC->RecaptureSky();
		}
	}

	// 어느 레벨을 띄울지: GameMode 오버라이드(0이상) > 맵 이름 접미사. 정본=게임_전투레벨_정본.md
	int32 Lvl = LevelOverride;
	if (Lvl < 0)
	{
		FString MapName = UWorld::RemovePIEPrefix(W->GetMapName());
		int32 UnderPos;
		if (MapName.FindLastChar('_', UnderPos))
		{
			const FString Tail = MapName.Mid(UnderPos + 1);
			if (Tail.IsNumeric()) { Lvl = FCString::Atoi(*Tail); }
		}
		if (Lvl < 0) { Lvl = 0; } // HexBattle(접미사 없음)=L1
	}

	// 그리드 매니저 절차 생성(원점). 지연 스폰으로 LevelIndex를 BeginPlay(StartBattle) 전에 넣는다.
	Grid = W->SpawnActorDeferred<AHexGridManager>(AHexGridManager::StaticClass(), FTransform::Identity, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Grid)
	{
		Grid->LevelIndex = Lvl;
		UGameplayStatics::FinishSpawningActor(Grid, FTransform::Identity);
	}

	// 대각 대치 구도: 아군을 화면 좌하단, 상대를 우상단에 둔다.
	// 두 캐릭터를 잇는 선(P→E)의 옆으로 카메라를 빼서, 카메라의 오른쪽 축이 P→E와 나란해지게 한다.
	// 그러면 상대(+Dir 쪽)가 화면 오른쪽에, 아군이 왼쪽에 온다. 카메라를 높이고 아군 쪽으로 당겨
	// 아군은 크고 낮게(좌하단), 상대는 작고 높게(우상단) 잡힌다.
	FVector CamLoc(-1100.0f, 900.0f, 700.0f);
	FRotator CamRot(-18.0f, -35.0f, 0.0f);
	if (Grid && Grid->PlayerUnit && Grid->EnemyUnit)
	{
		const FVector P = Grid->PlayerUnit->GetActorLocation();
		const FVector E = Grid->EnemyUnit->GetActorLocation();
		FVector Dir = (E - P); Dir.Z = 0.0f; Dir = Dir.GetSafeNormal();
		const FVector Side(Dir.Y, -Dir.X, 0.0f); // P→E에 수직(부호를 뒤집으면 좌우가 바뀐다)
		// 아군 쪽(0.28)으로 치우친 지점에서 옆으로 크게 빼고 높이 든다 → 아군이 크고 낮게 좌하단.
		CamLoc = FMath::Lerp(P, E, 0.28f) + Side * 1000.0f + FVector(0.0f, 0.0f, 540.0f);
		// 시선은 상대 쪽으로 치우쳐 위로 — 상대가 우상단에, 아군이 좌하단에 걸리게.
		const FVector Target = FMath::Lerp(P, E, 0.62f) + FVector(0.0f, 0.0f, 90.0f);
		CamRot = (Target - CamLoc).Rotation();
	}
	ACameraActor* Cam = W->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CamLoc, CamRot, Sp);
	if (Cam && Cam->GetCameraComponent()) { Cam->GetCameraComponent()->SetFieldOfView(55.0f); }

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0))
	{
		if (Cam) { PC->SetViewTargetWithBlend(Cam, 0.0f); }
	}
}
