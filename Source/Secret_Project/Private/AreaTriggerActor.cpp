#include "AreaTriggerActor.h"
#include "Components/BoxComponent.h"
#include "StatComponent.h"
#include "SecretSaveGame.h"
#include "Engine/Engine.h"

AAreaTriggerActor::AAreaTriggerActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    RootComponent = Trigger;
    Trigger->InitBoxExtent(FVector(400.f, 400.f, 200.f));
    Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Trigger->SetGenerateOverlapEvents(true);
}

const TArray<FRegionInfo>& AAreaTriggerActor::GetRegisteredRegions()
{
    static TArray<FRegionInfo> Registry;
    return Registry;
}

void AAreaTriggerActor::RegisterRegion(FName Id, const FString& Name)
{
    if (Id.IsNone()) return;
    TArray<FRegionInfo>& Registry = const_cast<TArray<FRegionInfo>&>(GetRegisteredRegions());
    for (const FRegionInfo& R : Registry)
        if (R.Id == Id) return; // 중복 방지
    Registry.Add({ Id, Name });
}

void AAreaTriggerActor::BeginPlay()
{
    Super::BeginPlay();
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AAreaTriggerActor::OnTriggerOverlap);

    if (!RegionId.IsNone())
        RegisterRegion(RegionId, AreaName);
}

void AAreaTriggerActor::OnTriggerOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
    // 플레이어만
    if (!OtherActor || OtherActor == this || !OtherActor->FindComponentByClass<UStatComponent>())
        return;

    bool bNewlyDiscovered = false;
    if (!RegionId.IsNone() && !USecretSaveGame::IsCollected(RegionId))
    {
        USecretSaveGame::MarkCollected(RegionId);
        bNewlyDiscovered = true;
    }

    if (GEngine)
    {
        const FString Msg = bNewlyDiscovered
            ? FString::Printf(TEXT("새 지역 발견 — %s"), *AreaName)
            : AreaName;
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White, Msg);
    }
}
