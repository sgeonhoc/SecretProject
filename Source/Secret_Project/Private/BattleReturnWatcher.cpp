#include "BattleReturnWatcher.h"
#include "GameFlowSubsystem.h"
#include "HexGridManager.h"
#include "HexUnit.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ABattleReturnWatcher::ABattleReturnWatcher()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ABattleReturnWatcher::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bReported) return;

    UWorld* W = GetWorld();
    if (!W) return;

    // 이 판을 돌리는 것을 찾는다(전투 담당 소유 — 읽기만 한다).
    AHexGridManager* Grid = nullptr;
    for (TActorIterator<AHexGridManager> It(W); It; ++It) { Grid = *It; break; }
    if (!Grid) return;

    if (!Grid->bOver)
    {
        SinceOver = -1.f;
        return;
    }

    if (SinceOver < 0.f) SinceOver = 0.f;
    SinceOver += DeltaSeconds;
    if (SinceOver < HoldAfterOver) return;   // 마지막 타격이 화면에 남을 시간

    // 이겼나 — 내 쪽이 살아 있으면 이긴 것.
    const bool bWon = Grid->PlayerUnit && Grid->PlayerUnit->IsAlive();
    bReported = true;

    UE_LOG(LogTemp, Log, TEXT("[GameFlow] 전투 끝(%s) — 이야기로 돌아간다."),
        bWon ? TEXT("이김") : TEXT("짐"));

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGameFlowSubsystem* Flow = GI->GetSubsystem<UGameFlowSubsystem>())
        {
            Flow->NotifyBattleFinished(bWon);
            return;
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[GameFlow] 전투가 끝났는데 진행 담당이 없다 — 돌아갈 데를 모른다."));
}
