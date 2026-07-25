#include "WishingWellActor.h"
#include "Components/StaticMeshComponent.h"
#include "StatComponent.h"
#include "InventoryComponent.h"
#include "SecretSaveGame.h"
#include "Engine/Engine.h"

AWishingWellActor::AWishingWellActor()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = MeshComp;
    MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    // 기본 보상 풀(BP에서 덮어쓸 수 있음)
    RewardPool = { TEXT("HealPotion"), TEXT("SPPotion"), TEXT("HiPotion"), TEXT("Elixir") };
}

bool AWishingWellActor::MakeWish(AActor* Wisher)
{
    if (!Wisher) return false;

    UStatComponent* Stat = Wisher->FindComponentByClass<UStatComponent>();
    if (!Stat) return false;

    if (!Stat->SpendGold(CostPerWish))
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange,
                FString::Printf(TEXT("골드 부족 (소원 %dG)"), CostPerWish));
        return false;
    }

    // 당첨 판정
    FString Msg = TEXT("동전을 던졌다... 잔잔한 물결뿐.");
    if (FMath::FRand() <= RewardChance)
    {
        FName ItemId = RewardPool.Num() > 0
            ? RewardPool[FMath::RandRange(0, RewardPool.Num() - 1)]
            : FName(TEXT("HealPotion"));

        if (UInventoryComponent* Inv = Wisher->FindComponentByClass<UInventoryComponent>())
        {
            Inv->AddItem(ItemId, 1);
            FConsumableDef Def;
            const FString Name = UInventoryComponent::FindDef(ItemId, Def) ? Def.Name : ItemId.ToString();
            Msg = FString::Printf(TEXT("소원 성취! %s 획득"), *Name);
        }
    }

    USecretSaveGame::SavePlayerProgression(Stat); // 골드/인벤토리 변동 영구화
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, Msg);
    return true;
}
