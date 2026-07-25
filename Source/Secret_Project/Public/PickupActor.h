#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * 월드에 배치하는 자동 획득물(골드 코인/아이템). 플레이어가 밟으면(오버랩) 즉시 획득 후 사라짐.
 * - PickupId가 설정되면 1회성(세이브에 기록 → 재방문 시 다시 안 나옴). None이면 매번 재생성(반복 획득).
 * - 로직 전부 C++. 메시/머티리얼은 BP/에디터에서 지정.
 */
UCLASS()
class SECRET_PROJECT_API APickupActor : public AActor
{
    GENERATED_BODY()

public:
    APickupActor();

protected:
    virtual void BeginPlay() override;

    // 오버랩 트리거
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
    TObjectPtr<USphereComponent> Trigger;

    // 외형
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    // 1회성 세이브 키. None이면 반복 획득 가능(세이브 안 함).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    FName PickupId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    int32 GoldAmount = 10;

    // 지급할 소비아이템 Id (InventoryComponent 카탈로그 Id). None이면 골드만.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    FName ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    int32 ItemCount = 1;

    UFUNCTION()
    void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

    // 획득 처리(골드/아이템 지급 + 세이브 + 파괴)
    void Collect(AActor* Collector);
};
