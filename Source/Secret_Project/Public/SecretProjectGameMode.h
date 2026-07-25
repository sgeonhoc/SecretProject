// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SecretProjectGameMode.generated.h"

class AStageObjectiveActor;
class AStageHudActor;
class AStorySceneDirector;

/**
 * 본편(라셀 레벨) 게임모드.
 *
 * 레벨에 도착한 순간부터 다음 장면으로 넘어가기까지의 사슬을 여기서 쥔다:
 *   ①우리 캐릭터/컨트롤러를 세운다(엔진 기본 폰이 아니라 실제로 조작되는 캐릭터)
 *   ②어느 문으로 들어왔는지(GameFlow 도착 태그)에 맞는 자리에 세운다
 *   ③**때·자리 카드 → 도착 장면(대사) → 목표 세우기** — 이게 "스토리가 진행되는" 몸통이다
 *   ④목표에 닿으면 그 자리의 장면을 틀고, 끝나면 진행표를 한 칸 넘긴다
 *
 * 장면·목표·서 있는 사람은 전부 **진행표(FGameStage)를 보고 런타임에** 세운다.
 * .umap에 심지 않는 까닭: 레벨 담당이 레벨을 계속 다시 저장하므로 서로 덮어쓴다.
 */
UCLASS()
class SECRET_PROJECT_API ASecretProjectGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASecretProjectGameMode();

	// 도착 태그와 PlayerStartTag가 맞는 자리에 세운다. 맞는 게 없으면 기본 동작(아무 PlayerStart).
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** 목표 자리가 부른다 — 그 자리의 장면을 틀고, 끝나면 한 칸 넘긴다. */
	void NotifyObjectiveReached();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	// 오프닝 연출가(비면 AGameOpeningDirector). 새 게임 첫 진입에만 쓰인다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	TSubclassOf<class AGameOpeningDirector> OpeningDirectorClass;

	// 진행표에 몸이 안 적힌 사람은 이 블루프린트로 세운다. ※지금은 아무거나 세워 두는 자리다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow")
	FString DefaultNpcBlueprintPath = TEXT("/Game/BP_ANPCCharacter1.BP_ANPCCharacter1_C");

	// 레벨 메시에 충돌이 없을 때만 깔리는 안 보이는 받침(근본 해결은 레벨 쪽 충돌)
	void SpawnFallbackFloor(float TopZ);
	bool bFallbackFloorSpawned = false;

	// ── 떨어졌을 때 건져 올리기 ───────────────────────────
	// 레벨 메시에 충돌이 없는 자리가 남아 있어, 걷다가 바닥이 끊기면 캐릭터가 한없이 떨어진다.
	// 그러면 게임이 그대로 멈춘 것과 같으므로, 지면보다 한참 아래로 내려가면 이 스테이지의 시작 자리로 되돌린다.
	// (근본 해결은 레벨 쪽 충돌 — 이건 그때까지 플레이가 안 끊기게 하는 그물이다.)
	virtual void Tick(float DeltaSeconds) override;
	float StageGroundZ = 0.f;      // 이 스테이지에서 발이 닿은 높이
	FVector StageSpawnLoc = FVector::ZeroVector;
	bool bHasStageSpawn = false;
	float FallCheckTime = 0.f;
	int32 RescueCount = 0;
	bool bHidLegacyHud = false;   // 옛 기능 시험용 상시 HUD를 한 번만 끄면 된다

private:
	// ── 스테이지 차림 사슬 ────────────────────────────────
	void SetupCurrentStage(bool bFreshLevel);
	// 사람 한 묶음 스폰(장면용·상주 공용)
	void SpawnNpcRoster(const TArray<struct FStageNpc>& Roster, const TCHAR* LabelTag);
	void PlaceSceneNpcs();
	// 이 구역 상주 사람들(스테이지 무관, 거리를 살아 있게)
	void PlaceResidentNpcs();
	// 진입/오프닝 뒤 갈림길: Episode면 컷신, Hub면 자유 탐험.
	void ProceedAfterEntry();
	// 허브 진입 — 대사 강제 없이 곧바로 조작 + 목표 표시.
	void EnterHubExplore();
	void PlayArrivalScene();
	void OnArrivalSceneDone();
	void SpawnObjective();
	void OnObjectiveSceneDone();
	void ClearStageActors();

	// 옛 시험판 표시(상시 월드 HUD·옛 스토리 창)가 저절로 뜨는 것만 끈다. 지우지는 않는다.
	void QuietLegacyOverlays();

	UFUNCTION()
	void HandleStageChanged(FName StageId);

	UPROPERTY() TObjectPtr<AStageObjectiveActor> ObjectiveActor;
	UPROPERTY() TObjectPtr<AStageHudActor> StageHud;
	UPROPERTY() TArray<TObjectPtr<AActor>> StageNpcs;

	bool bStageSetupDone = false;      // 이 레벨에서 이미 차렸나(중복 방지)
	bool bStageIsHere = false;         // 지금 발 딛은 레벨이 현재 스토리 스테이지의 무대인가(딴 구역=자유 탐험)
	bool bCardShownByOpening = false;  // 오프닝이 때·자리 자막을 이미 띄웠나(두 번 안 띄우게)
};
