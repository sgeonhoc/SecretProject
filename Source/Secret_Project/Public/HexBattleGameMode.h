#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HexBattleGameMode.generated.h"

class AHexGridManager;

/**
 * 헥스 칸/살란 전투 시뮬 게임모드. 빈 레벨에 이 게임모드만 지정하면 그리드·유닛·카메라가 절차 생성된다.
 * (World Settings > GameMode Override = BP_HexBattleGameMode 또는 이 클래스)
 */
UCLASS()
class SECRET_PROJECT_API AHexBattleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHexBattleGameMode();

	virtual void BeginPlay() override;

	// 띄울 레벨. -1이면 맵 이름 접미사로 결정(HexBattle=0, HexBattle_01=1 …). 0이상이면 강제.
	UPROPERTY(EditAnywhere, Category = "Hex")
	int32 LevelOverride = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Hex")
	TObjectPtr<AHexGridManager> Grid;
};
