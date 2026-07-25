#include "HarvestNodeActor.h"
#include "Components/StaticMeshComponent.h"
#include "InventoryComponent.h"
#include "StatComponent.h"
#include "SecretSaveGame.h"
#include "TimeComponent.h"
#include "Engine/Engine.h"

AHarvestNodeActor::AHarvestNodeActor()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = MeshComp;
    MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

bool AHarvestNodeActor::Harvest(AActor* Gatherer)
{
    if (!Gatherer) return false;

    const bool bGivesItem = !ItemId.IsNone() && ItemCount > 0;
    const bool bGivesGold = GoldReward > 0;
    if (!bGivesItem && !bGivesGold) return false; // 줄 게 없음

    UInventoryComponent* Inv = Gatherer->FindComponentByClass<UInventoryComponent>();
    if (bGivesItem && !Inv) return false;

    // 재생 판정: 시간 시스템이 있으면 날짜 기준, 없으면 1회용
    UTimeComponent* Time = Gatherer->FindComponentByClass<UTimeComponent>();

    // 야간 전용: 밤이 아니면 채집 불가
    if (bNightOnly && Time && Time->GetPhase() != EDayPhase::Night)
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Silver,
                FString::Printf(TEXT("%s — 밤에만 채집 가능"), *DisplayName));
        return false;
    }

    if (Time)
    {
        const int32 Today = Time->GetDay();
        if (Today <= LastHarvestDay)
        {
            if (GEngine)
                GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Silver,
                    FString::Printf(TEXT("%s — 다음날 다시 자랍니다"), *DisplayName));
            return false;
        }
        LastHarvestDay = Today;
    }
    else
    {
        if (bHarvestedNoTime)
        {
            if (GEngine)
                GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Silver,
                    FString::Printf(TEXT("%s — 이미 채집함"), *DisplayName));
            return false;
        }
        bHarvestedNoTime = true;
    }

    FString Msg = FString::Printf(TEXT("채집: %s"), *DisplayName);
    if (bGivesItem)
    {
        Inv->AddItem(ItemId, ItemCount);
        Msg += FString::Printf(TEXT(" x%d"), ItemCount);
    }
    if (bGivesGold)
    {
        if (UStatComponent* Stat = Gatherer->FindComponentByClass<UStatComponent>())
        {
            Stat->AddGold(GoldReward);
            USecretSaveGame::SavePlayerProgression(Stat); // 일일 골드는 빈도 낮아 즉시 저장
            Msg += FString::Printf(TEXT(" +%dG"), GoldReward);
        }
    }
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, Msg);
    return true;
}
