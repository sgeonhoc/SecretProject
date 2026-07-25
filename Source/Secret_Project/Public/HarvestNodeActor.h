#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HarvestNodeActor.generated.h"

class UStaticMeshComponent;

/**
 * 채집물(약초/광맥 등). E로 상호작용하면 아이템 획득. 하루 1회(시간 시스템 있으면 날짜 기준 재생).
 * 로직 C++, 메시/배치는 BP·에디터. 탐험↔인벤토리↔시간 연결.
 */
UCLASS()
class SECRET_PROJECT_API AHarvestNodeActor : public AActor
{
    GENERATED_BODY()

public:
    AHarvestNodeActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Harvest")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest")
    FString DisplayName = TEXT("약초");

    // 획득할 인벤토리 아이템 Id (InventoryComponent 카탈로그 Id). None이면 아이템 없음(골드만).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest")
    FName ItemId = TEXT("HealPotion");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest")
    int32 ItemCount = 1;

    // 함께 지급할 골드(0이면 없음). 아이템 없이 GoldReward만 두면 "일일 골드" 지점이 됨.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest")
    int32 GoldReward = 0;

    // true면 밤(Night)에만 채집 가능(야행성 약초 등). 시간시스템 없으면 무시.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest")
    bool bNightOnly = false;

    // 상호작용 채집. 가능하면 아이템 지급 + true.
    UFUNCTION(BlueprintCallable, Category = "Harvest")
    bool Harvest(AActor* Gatherer);

private:
    // 마지막으로 채집한 게임 날짜(0=아직). 시간 시스템 없으면 1회용으로 동작.
    int32 LastHarvestDay = 0;
    bool bHarvestedNoTime = false; // 시간 시스템 없을 때 1회 채집 여부
};
