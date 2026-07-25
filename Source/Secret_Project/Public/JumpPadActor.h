#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JumpPadActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * 점프패드/이동 기믹. 플레이어가 밟으면 지정 속도로 발사(LaunchCharacter). 로직 C++, 메시/배치 BP.
 */
UCLASS()
class SECRET_PROJECT_API AJumpPadActor : public AActor
{
    GENERATED_BODY()

public:
    AJumpPadActor();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpPad")
    TObjectPtr<UBoxComponent> Trigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JumpPad")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    // 발사 속도(월드 기준). 기본은 위로 강하게.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JumpPad")
    FVector LaunchVelocity = FVector(0.f, 0.f, 1300.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JumpPad")
    bool bOverrideXY = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JumpPad")
    bool bOverrideZ = true;

    UFUNCTION()
    void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
};
