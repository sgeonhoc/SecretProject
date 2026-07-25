#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleReturnWatcher.generated.h"

/**
 * 전투에서 이야기로 돌아오는 다리.
 *
 * 스토리가 전투를 부르면(스테이지 Goal=Battle) 진행이 전투 판으로 데려간다. 그런데 그 판은
 * **전투 담당의 화면과 게임모드**가 돌린다 — 그쪽 파일을 건드리지 않고 결과만 받아 오기 위해,
 * 진행(GameFlow)이 전투 레벨이 열릴 때 이 배우를 하나 몰래 세운다.
 *
 * 하는 일은 하나뿐이다: 판이 끝났는지(`AHexGridManager::bOver`) 지켜보다가, 끝나면 이기고 졌는지를
 * 읽어 잠깐 뜸을 들인 뒤 진행에 알린다. 그러면 진행이 다음 장면(또는 진 뒤의 갈래)으로 데려간다.
 *
 * ★전투 규칙·화면에는 손대지 않는다. 읽기만 한다.
 */
UCLASS()
class SECRET_PROJECT_API ABattleReturnWatcher : public AActor
{
    GENERATED_BODY()

public:
    ABattleReturnWatcher();

    virtual void Tick(float DeltaSeconds) override;

private:
    float SinceOver = -1.f;   // 판이 끝난 뒤 흐른 시간(< 0 = 아직 안 끝남)
    bool bReported = false;

    // 판이 끝나고 이만큼 두었다가 넘어간다 — 마지막 타격이 화면에 남을 시간.
    static constexpr float HoldAfterOver = 2.6f;
};
