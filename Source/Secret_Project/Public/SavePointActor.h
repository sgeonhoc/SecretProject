#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SavePointActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * 세이브/휴식 지점. 플레이어가 오버랩하면 진행상황 저장(+선택적 HP/SP 전체 회복).
 * 로직 전부 C++. 메시/배치는 BP·에디터. (페르소나의 '세이브 룸/휴식처' 역할)
 */
UCLASS()
class SECRET_PROJECT_API ASavePointActor : public AActor
{
    GENERATED_BODY()

public:
    ASavePointActor();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SavePoint")
    TObjectPtr<USphereComponent> Trigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SavePoint")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    // 저장 시 HP/SP 전체 회복(휴식). false면 저장만.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SavePoint")
    bool bRestoreOnSave = true;

    // 저장 시 다음날로 진행(취침형 세이브포인트). false면 시간 유지.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SavePoint")
    bool bAdvanceDayOnRest = false;

    UFUNCTION()
    void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
};
