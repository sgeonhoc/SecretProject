#include "StageObjectiveActor.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "SecretProjectGameMode.h"
#include "StatComponent.h"
#include "Engine/World.h"

AStageObjectiveActor::AStageObjectiveActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
    RootComponent = Trigger;
    Trigger->InitSphereRadius(240.f);
    Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Trigger->SetGenerateOverlapEvents(true);

    // 자리 표시 = 따뜻한 빛 한 점. 메시·머티리얼 에셋을 안 쓴다(외부 에셋 금지 규율).
    Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
    Glow->SetupAttachment(RootComponent);
    Glow->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
    Glow->SetLightColor(FLinearColor(1.0f, 0.78f, 0.42f));
    Glow->SetAttenuationRadius(420.f);
    Glow->SetIntensity(2600.f);
    Glow->SetCastShadows(false);
}

void AStageObjectiveActor::Setup(float Radius, const FText& Label)
{
    // 너무 좁으면 옆을 스쳐 지나가면서도 "닿았다"가 안 뜬다 — 아무리 좁아도 3m는 준다.
    if (Trigger) Trigger->SetSphereRadius(FMath::Max(Radius, 300.f));
    ObjectiveLabel = Label;
}

void AStageObjectiveActor::BeginPlay()
{
    Super::BeginPlay();
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AStageObjectiveActor::OnOverlap);
}

void AStageObjectiveActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    PulseTime += DeltaSeconds;

    // 3.2초에 한 번 숨쉬듯 — 거리의 고정 불빛과 눈에 구분된다.
    if (Glow)
        Glow->SetIntensity(2200.f + 900.f * (0.5f + 0.5f * FMath::Sin(PulseTime * 1.95f)));
}

void AStageObjectiveActor::OnOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
    if (bFired || !OtherActor || OtherActor == this) return;

    // 플레이어만(StatComponent 보유 = 우리 캐릭터). NPC가 지나가며 장면을 넘겨 버리면 안 된다.
    if (!OtherActor->FindComponentByClass<UStatComponent>()) return;

    bFired = true;
    if (Glow) Glow->SetVisibility(false);

    // 진행을 여기서 직접 넘기지 않는다 — 게임모드가 이 자리의 장면을 먼저 틀고, 그 장면이 끝나야 넘어간다.
    if (UWorld* W = GetWorld())
    {
        if (ASecretProjectGameMode* GM = W->GetAuthGameMode<ASecretProjectGameMode>())
        {
            GM->NotifyObjectiveReached();
            return;
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[GameFlow] 목표에 닿았으나 본편 게임모드가 아니다 — 진행이 안 넘어간다."));
}
