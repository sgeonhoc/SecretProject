#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AreaTriggerActor.generated.h"

class UBoxComponent;

// 지역 레지스트리 항목 (도감 표시용, C++ 전용)
struct FRegionInfo
{
    FName Id;
    FString Name;
};

/**
 * 지역 진입 트리거. 플레이어가 영역에 들어오면 지역명을 화면에 표시.
 * RegionId가 있으면 최초 진입 시 "발견!"(세이브에 기록, 재방문 시엔 일반 표시).
 * 로직 C++, 배치/크기는 BP·에디터. 탐험 피드백용.
 */
UCLASS()
class SECRET_PROJECT_API AAreaTriggerActor : public AActor
{
    GENERATED_BODY()

public:
    AAreaTriggerActor();

    // 배치된 지역들의 레지스트리 (BeginPlay에서 자동 등록). 도감 위젯이 순회.
    static const TArray<FRegionInfo>& GetRegisteredRegions();
    static void RegisterRegion(FName Id, const FString& Name);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Area")
    TObjectPtr<UBoxComponent> Trigger;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area")
    FString AreaName = TEXT("이름 없는 지역");

    // 최초 발견 추적용 세이브 키(고유). 비면 매번 일반 표시(발견 기록 안 함).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Area")
    FName RegionId;

    UFUNCTION()
    void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
};
