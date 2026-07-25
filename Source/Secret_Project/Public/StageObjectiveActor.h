#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageObjectiveActor.generated.h"

class USphereComponent;
class UPointLightComponent;

/**
 * 지금 스테이지의 목표 자리.
 *
 * 레벨 파일에 심지 않는다 — 진행표(FGameStage)를 보고 GameMode가 레벨 도착 때 세운다.
 * 레벨은 레벨 담당이 계속 다시 저장하는 중이라, 진행 장치를 .umap 안에 박으면 서로 덮어쓴다.
 *
 * 눈에 보이는 것은 그 자리에 얹은 **따뜻한 빛 한 점**뿐이다(에셋 0 — 등롱이 켜진 것처럼 보인다).
 * 플레이어가 들어서면 게임모드에 알리고, 게임모드가 그 자리의 장면을 튼 뒤 한 칸 나아간다.
 */
UCLASS()
class SECRET_PROJECT_API AStageObjectiveActor : public AActor
{
    GENERATED_BODY()

public:
    AStageObjectiveActor();

    // 진행표 값으로 세운다(반경/문구).
    void Setup(float Radius, const FText& Label);

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Objective")
    TObjectPtr<USphereComponent> Trigger;

    // 자리를 알려 주는 빛. 숨 쉬듯 밝기가 오르내려 배경의 다른 불빛과 구분된다.
    UPROPERTY(VisibleAnywhere, Category = "Objective")
    TObjectPtr<UPointLightComponent> Glow;

    UPROPERTY() FText ObjectiveLabel;

    bool bFired = false;
    float PulseTime = 0.f;

    UFUNCTION()
    void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
};
