#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WishingWellActor.generated.h"

class UStaticMeshComponent;

/**
 * 소원의 샘(골드 소비처). E로 상호작용하면 골드를 소비하고 확률로 아이템 획득(가챠형 골드 싱크).
 * 로직 C++, 메시/배치 BP. 골드 사용처를 상점 외에 하나 더 제공.
 */
UCLASS()
class SECRET_PROJECT_API AWishingWellActor : public AActor
{
    GENERATED_BODY()

public:
    AWishingWellActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Well")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    // 소원 1회 비용(골드)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Well")
    int32 CostPerWish = 20;

    // 보상 확률(0~1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Well")
    float RewardChance = 0.35f;

    // 당첨 시 줄 수 있는 아이템 풀(InventoryComponent 카탈로그 Id). 비면 회복약.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Well")
    TArray<FName> RewardPool;

    // 소원 빌기 (골드 소비 + 확률 보상). 성공 여부(=골드 소비됨) 반환.
    UFUNCTION(BlueprintCallable, Category = "Well")
    bool MakeWish(AActor* Wisher);
};
