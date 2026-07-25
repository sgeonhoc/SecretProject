#include "SavePointActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "StatComponent.h"
#include "TimeComponent.h"
#include "SecretSaveGame.h"
#include "Engine/Engine.h"

ASavePointActor::ASavePointActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
    RootComponent = Trigger;
    Trigger->InitSphereRadius(100.f);
    Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Trigger->SetGenerateOverlapEvents(true);

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComp->SetupAttachment(RootComponent);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASavePointActor::BeginPlay()
{
    Super::BeginPlay();
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &ASavePointActor::OnTriggerOverlap);
}

void ASavePointActor::OnTriggerOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
    if (!OtherActor || OtherActor == this) return;

    UStatComponent* Stat = OtherActor->FindComponentByClass<UStatComponent>();
    if (!Stat) return; // 플레이어만

    if (bRestoreOnSave)
        Stat->FullRestore();

    // 취침형: 다음날로 진행 (저장 전에 시간 갱신 → 세이브에 반영)
    if (bAdvanceDayOnRest)
    {
        if (UTimeComponent* Time = OtherActor->FindComponentByClass<UTimeComponent>())
            Time->AdvanceToNextDay();
    }

    // 진행상황 저장 (골드/인벤토리/퀘스트/시간 포함)
    USecretSaveGame::SavePlayerProgression(Stat);

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
            bRestoreOnSave ? TEXT("저장 완료 — 체력/SP 회복") : TEXT("저장 완료"));
}
