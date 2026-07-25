#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FastTravelPointActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

// 빠른 이동 지점 레지스트리 항목 (C++ 전용, 위젯이 순회)
struct FFastTravelPoint
{
    FName Id;
    FString Name;
    FVector Location = FVector::ZeroVector;
    FName LevelName;
};

/**
 * 빠른 이동(여행) 지점. 플레이어가 오버랩하면 "발견"으로 기록(세이브)되고, 이후 시스템 메뉴의
 * 빠른 이동에서 목적지로 선택해 텔레포트할 수 있다.
 * - v1: 같은 레벨 내 텔레포트(SetActorLocation). 다른 레벨 지점은 목록에서 제외(추후 확장).
 * - 발견 기록은 보물상자/지역과 동일하게 SecretSaveGame의 CollectedIds 재사용(IsCollected/MarkCollected).
 * - 로직 C++, 배치/메시는 BP·에디터. PointId/PointName만 지정하면 됨.
 */
UCLASS()
class SECRET_PROJECT_API AFastTravelPointActor : public AActor
{
    GENERATED_BODY()

public:
    AFastTravelPointActor();

    // 배치된 지점 레지스트리 (BeginPlay에서 자동 등록). 위젯이 순회.
    static const TArray<FFastTravelPoint>& GetRegisteredPoints();
    static void RegisterPoint(const FFastTravelPoint& Point);
    // 레벨 전환 시 잔여 항목 정리용(현재 레벨 것만 남김은 위젯이 필터)
    static void ClearRegistry();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FastTravel")
    TObjectPtr<USphereComponent> Trigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FastTravel")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    // 발견/저장용 고유 키. 비면 동작 안 함.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FastTravel")
    FName PointId;

    // 메뉴에 표시할 지점 이름
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FastTravel")
    FString PointName = TEXT("여행 지점");

    UFUNCTION()
    void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
};
