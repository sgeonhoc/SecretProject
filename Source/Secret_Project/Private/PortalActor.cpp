#include "PortalActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "StatComponent.h"
#include "SecretSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "StoryManager.h" // 스토리 플래그 게이트
#include "GameFlowSubsystem.h"
#include "Engine/GameInstance.h"

APortalActor::APortalActor()
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

void APortalActor::BeginPlay()
{
    Super::BeginPlay();
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &APortalActor::OnTriggerOverlap);

    // 스토리 상태 게이트: 조건 안 맞으면 이 문/포탈은 이 상태에서 닫힘(숨김+충돌끔).
    if (!UStoryManagerSubsystem::PassesFlagGate(this, RequiredFlag, ForbiddenFlag))
    {
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
    }
}

void APortalActor::OnTriggerOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
    // 플레이어(StatComponent 보유)만 이동
    if (OtherActor && OtherActor != this && OtherActor->FindComponentByClass<UStatComponent>())
        TravelTo(OtherActor);
}

void APortalActor::TravelTo(AActor* Traveler)
{
    if (bTraveling) return;

    if (TargetLevelName.IsNone())
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("포탈: TargetLevelName 미설정"));
        return;
    }

    // 이동 전 진행상황 저장 (다음 레벨로 골드/인벤토리/퀘스트 이어짐)
    if (bSaveBeforeTravel && Traveler)
    {
        if (UStatComponent* Stat = Traveler->FindComponentByClass<UStatComponent>())
            USecretSaveGame::SavePlayerProgression(Stat);
    }

    bTraveling = true;

    // 진행 담당(GameFlow)을 거쳐 이동한다 — 도착 레벨에서 이 문에 해당하는 자리에 서고,
    // 이어하기가 "마지막으로 있던 장소"로 이 레벨을 가리키게 된다.
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGameFlowSubsystem* Flow = GI->GetSubsystem<UGameFlowSubsystem>())
        {
            Flow->TravelToLevel(TargetLevelName, ArrivalEntryTag);
            return;
        }
    }
    UGameplayStatics::OpenLevel(this, TargetLevelName);
}
