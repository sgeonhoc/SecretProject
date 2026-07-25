#include "LockedGateActor.h"
#include "Components/StaticMeshComponent.h"
#include "InventoryComponent.h"
#include "StatComponent.h"
#include "SecretSaveGame.h"
#include "Engine/Engine.h"
#include "StoryManager.h" // 스토리 플래그 게이트

ALockedGateActor::ALockedGateActor()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = MeshComp;
    // 플레이어를 막고(Pawn) + 상호작용 트레이스(Visibility)도 맞도록 전부 블록
    MeshComp->SetCollisionProfileName(TEXT("BlockAll"));
}

void ALockedGateActor::BeginPlay()
{
    Super::BeginPlay();

    // 스토리 상태 게이트: 조건 안 맞으면 이 상태엔 이 관문이 없음(숨김+충돌끔).
    if (!UStoryManagerSubsystem::PassesFlagGate(this, RequiredFlag, ForbiddenFlag))
    {
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
        return;
    }

    if (!GateId.IsNone() && USecretSaveGame::IsCollected(GateId))
    {
        bOpen = true;
        ApplyOpenVisual();
    }
}

bool ALockedGateActor::TryOpen(AActor* Opener)
{
    if (bOpen) return true;
    if (!Opener) return false;

    UInventoryComponent* Inv = Opener->FindComponentByClass<UInventoryComponent>();
    const bool bHasKey = Inv && Inv->GetCount(RequiredItemId) > 0;

    if (!bHasKey)
    {
        // 필요한 열쇠 이름 안내
        FConsumableDef Def;
        const FString KeyName = UInventoryComponent::FindDef(RequiredItemId, Def)
            ? Def.Name : RequiredItemId.ToString();
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
                FString::Printf(TEXT("잠겨 있다 — '%s' 필요"), *KeyName));
        return false;
    }

    if (bConsumeKey)
        Inv->RemoveItem(RequiredItemId, 1);

    bOpen = true;
    ApplyOpenVisual();

    // 열림 상태 + 열쇠 소모 영구화
    if (!GateId.IsNone())
        USecretSaveGame::MarkCollected(GateId);
    if (UStatComponent* Stat = Opener->FindComponentByClass<UStatComponent>())
        USecretSaveGame::SavePlayerProgression(Stat);

    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("문을 열었다"));
    return true;
}

void ALockedGateActor::ApplyOpenVisual()
{
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
}
