#include "FastTravelPointActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "StatComponent.h"
#include "SecretSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

namespace
{
    // 게임 인스턴스 수명 동안 유지되는 배치 지점 레지스트리
    TArray<FFastTravelPoint>& Registry()
    {
        static TArray<FFastTravelPoint> Points;
        return Points;
    }
}

AFastTravelPointActor::AFastTravelPointActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
    RootComponent = Trigger;
    Trigger->InitSphereRadius(120.f);
    Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Trigger->SetGenerateOverlapEvents(true);

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComp->SetupAttachment(RootComponent);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

const TArray<FFastTravelPoint>& AFastTravelPointActor::GetRegisteredPoints()
{
    return Registry();
}

void AFastTravelPointActor::RegisterPoint(const FFastTravelPoint& Point)
{
    if (Point.Id.IsNone()) return;
    // 같은 Id 중복 방지(레벨 재진입 시 갱신)
    for (FFastTravelPoint& P : Registry())
    {
        if (P.Id == Point.Id) { P = Point; return; }
    }
    Registry().Add(Point);
}

void AFastTravelPointActor::ClearRegistry()
{
    Registry().Reset();
}

void AFastTravelPointActor::BeginPlay()
{
    Super::BeginPlay();
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AFastTravelPointActor::OnTriggerOverlap);

    // 배치된 지점을 레지스트리에 등록(위치/레벨 캡처). 발견 여부와 무관하게 등록하되 목록은 위젯이 IsCollected로 필터.
    if (!PointId.IsNone())
    {
        FFastTravelPoint P;
        P.Id = PointId;
        P.Name = PointName;
        P.Location = GetActorLocation();
        P.LevelName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));
        RegisterPoint(P);
    }
}

void AFastTravelPointActor::OnTriggerOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
    // 플레이어(StatComponent 보유)만
    if (!OtherActor || OtherActor == this || !OtherActor->FindComponentByClass<UStatComponent>())
        return;
    if (PointId.IsNone()) return;

    const bool bFirst = !USecretSaveGame::IsCollected(PointId);
    if (bFirst)
    {
        USecretSaveGame::MarkCollected(PointId);
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
                FString::Printf(TEXT("여행 지점 발견: %s"), *PointName));
    }
}
