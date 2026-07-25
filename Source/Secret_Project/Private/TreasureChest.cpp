#include "TreasureChest.h"
#include "Components/StaticMeshComponent.h"
#include "StatComponent.h"
#include "InventoryComponent.h"
#include "QuestComponent.h"
#include "SecretSaveGame.h"
#include "Engine/Engine.h"

ATreasureChest::ATreasureChest()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = MeshComp;
    // 라인트레이스(가시성)로 상호작용 → Visibility 블록
    MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void ATreasureChest::BeginPlay()
{
    Super::BeginPlay();

    // 이미 수집된 상자면 열린 상태로 시작
    if (!ChestId.IsNone() && USecretSaveGame::IsCollected(ChestId))
    {
        bOpened = true;
        ApplyOpenedVisual();
    }
}

void ATreasureChest::Open(AActor* Opener)
{
    if (bOpened || !Opener) return;

    // 골드 지급
    if (UStatComponent* Stat = Opener->FindComponentByClass<UStatComponent>())
    {
        if (GoldContents > 0)
            Stat->AddGold(GoldContents);

        // 소비아이템 지급
        if (!ItemId.IsNone() && ItemCount > 0)
        {
            if (UInventoryComponent* Inv = Opener->FindComponentByClass<UInventoryComponent>())
                Inv->AddItem(ItemId, ItemCount);
        }

        // 골드/인벤토리 영구 저장 (상점·승리와 동일 경로)
        USecretSaveGame::SavePlayerProgression(Stat);
    }

    // 상자 자체를 수집됨으로 기록(재방문 시 빈 상자)
    if (!ChestId.IsNone())
        USecretSaveGame::MarkCollected(ChestId);

    // 퀘스트: "보물상자 열기" 목표 진행
    if (UQuestComponent* Quest = Opener->FindComponentByClass<UQuestComponent>())
        Quest->NotifyChestOpened();

    bOpened = true;

    // 피드백
    if (GEngine)
    {
        FString Msg = FString::Printf(TEXT("보물 획득! +%d G"), GoldContents);
        if (!ItemId.IsNone() && ItemCount > 0)
            Msg += FString::Printf(TEXT("  +%s x%d"), *ItemId.ToString(), ItemCount);
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, Msg);
    }

    OnOpened();
    ApplyOpenedVisual();
}

void ATreasureChest::ApplyOpenedVisual()
{
    if (bHideWhenOpened)
    {
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
    }
}
