#include "PickupActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "StatComponent.h"
#include "InventoryComponent.h"
#include "SecretSaveGame.h"
#include "Engine/Engine.h"

APickupActor::APickupActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
    RootComponent = Trigger;
    Trigger->InitSphereRadius(80.f);
    Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Trigger->SetGenerateOverlapEvents(true);

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComp->SetupAttachment(RootComponent);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APickupActor::BeginPlay()
{
    Super::BeginPlay();

    // 이미 획득한 1회성 픽업이면 등장하지 않음
    if (!PickupId.IsNone() && USecretSaveGame::IsCollected(PickupId))
    {
        Destroy();
        return;
    }

    Trigger->OnComponentBeginOverlap.AddDynamic(this, &APickupActor::OnTriggerOverlap);
}

void APickupActor::OnTriggerOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
    // 플레이어(=StatComponent 보유 액터)만 획득
    if (OtherActor && OtherActor != this && OtherActor->FindComponentByClass<UStatComponent>())
        Collect(OtherActor);
}

void APickupActor::Collect(AActor* Collector)
{
    if (!Collector) return;

    if (UStatComponent* Stat = Collector->FindComponentByClass<UStatComponent>())
    {
        if (GoldAmount > 0)
            Stat->AddGold(GoldAmount);

        if (!ItemId.IsNone() && ItemCount > 0)
        {
            if (UInventoryComponent* Inv = Collector->FindComponentByClass<UInventoryComponent>())
                Inv->AddItem(ItemId, ItemCount);
        }

        // 1회성만 영구 저장(상자와 동일). 반복 픽업은 저장 안 함(코인 다수 시 디스크 폭주 방지
        // — 획득한 골드는 다음 자연 저장 시점[전투 승리/상점/상자]에 함께 영구화됨).
        if (!PickupId.IsNone())
        {
            USecretSaveGame::SavePlayerProgression(Stat);
            USecretSaveGame::MarkCollected(PickupId);
        }
    }

    if (GEngine)
    {
        FString Msg = (GoldAmount > 0) ? FString::Printf(TEXT("+%d G"), GoldAmount) : FString();
        if (!ItemId.IsNone() && ItemCount > 0)
            Msg += FString::Printf(TEXT("  +%s x%d"), *ItemId.ToString(), ItemCount);
        if (!Msg.IsEmpty())
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, Msg);
    }

    Destroy();
}
