#include "AchievementComponent.h"
#include "StatComponent.h"
#include "InventoryComponent.h"
#include "QuestComponent.h"
#include "RelationshipComponent.h"
#include "SocialStatsComponent.h"
#include "AreaTriggerActor.h"
#include "BestiarySubsystem.h"
#include "SecretSaveGame.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

UAchievementComponent::UAchievementComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

const TArray<FAchievementDef>& UAchievementComponent::GetCatalog()
{
    static TArray<FAchievementDef> Catalog;
    if (Catalog.Num() == 0)
    {
        auto Add = [](FName Id, const FString& Title, const FString& Desc, EAchievementType Type, int32 Th,
                      int32 RewardGold, FName RewardItemId = NAME_None, int32 RewardItemCount = 0)
        {
            FAchievementDef A;
            A.Id = Id; A.Title = Title; A.Description = Desc; A.Type = Type; A.Threshold = Th;
            A.RewardGold = RewardGold; A.RewardItemId = RewardItemId; A.RewardItemCount = RewardItemCount;
            Catalog.Add(A);
        };
        Add(TEXT("ACH_Level5"),    TEXT("수련의 길"),   TEXT("레벨 5 도달"),        EAchievementType::ReachLevel,     5,  100);
        Add(TEXT("ACH_Level10"),   TEXT("일류의 증표"), TEXT("레벨 10 도달"),       EAchievementType::ReachLevel,     10, 300, TEXT("Elixir"), 1);
        Add(TEXT("ACH_Quests3"),   TEXT("해결사"),      TEXT("퀘스트 3개 완료"),    EAchievementType::CompleteQuests, 3,  150);
        Add(TEXT("ACH_Bond1"),     TEXT("둘도 없는 사이"), TEXT("최고 인연 1명 달성"), EAchievementType::MaxBonds,       1,  200);
        Add(TEXT("ACH_Explore3"),  TEXT("탐험가"),      TEXT("지역 3곳 발견"),      EAchievementType::DiscoverAreas,  3,  150, TEXT("HiPotion"), 2);
        Add(TEXT("ACH_Hunter10"),  TEXT("사냥꾼"),      TEXT("적 10마리 처치"),     EAchievementType::DefeatEnemies,  10, 200);
        Add(TEXT("ACH_Social3"),   TEXT("모범생"),      TEXT("사회 스탯 하나를 Rank 3까지"), EAchievementType::HighestSocialStat, 3, 150);
        Add(TEXT("ACH_Social5"),   TEXT("완성형 인간"), TEXT("사회 스탯 하나를 Rank 5(MAX)"), EAchievementType::HighestSocialStat, 5, 300, TEXT("Elixir"), 1);
    }
    return Catalog;
}

void UAchievementComponent::BeginPlay()
{
    Super::BeginPlay();

    // 세이브 로드/서브시스템 적재 순서 보장 위해 다음 틱에 구독 + 1회 평가
    if (GetWorld())
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UAchievementComponent::InitAchievements);
}

void UAchievementComponent::InitAchievements()
{
    if (AActor* Owner = GetOwner())
    {
        if (UTimeComponent* Time = Owner->FindComponentByClass<UTimeComponent>())
        {
            Time->OnTimeChanged.RemoveDynamic(this, &UAchievementComponent::OnWorldTimeChanged);
            Time->OnTimeChanged.AddDynamic(this, &UAchievementComponent::OnWorldTimeChanged);
        }
    }
    CheckAll();
}

void UAchievementComponent::OnWorldTimeChanged(int32 /*Day*/, EDayPhase /*Phase*/)
{
    CheckAll();
}

int32 UAchievementComponent::GetProgressFor(EAchievementType Type) const
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0;

    switch (Type)
    {
    case EAchievementType::ReachLevel:
        if (UStatComponent* Stat = Owner->FindComponentByClass<UStatComponent>())
            return Stat->GetLevel();
        return 0;

    case EAchievementType::CompleteQuests:
        if (UQuestComponent* Quest = Owner->FindComponentByClass<UQuestComponent>())
            return Quest->GetCompletedCount();
        return 0;

    case EAchievementType::MaxBonds:
        if (URelationshipComponent* Rel = Owner->FindComponentByClass<URelationshipComponent>())
            return Rel->GetMaxedCount();
        return 0;

    case EAchievementType::DiscoverAreas:
    {
        int32 Count = 0;
        for (const FRegionInfo& R : AAreaTriggerActor::GetRegisteredRegions())
            if (USecretSaveGame::IsCollected(R.Id)) ++Count;
        return Count;
    }

    case EAchievementType::DefeatEnemies:
    {
        int32 Total = 0;
        if (UWorld* W = Owner->GetWorld())
            if (UGameInstance* GI = W->GetGameInstance())
                if (UBestiarySubsystem* Bst = GI->GetSubsystem<UBestiarySubsystem>())
                    for (const TPair<FName, FBestiaryEntry>& P : Bst->GetAll())
                        Total += P.Value.DefeatedCount;
        return Total;
    }

    case EAchievementType::HighestSocialStat:
        if (USocialStatsComponent* Soc = Owner->FindComponentByClass<USocialStatsComponent>())
        {
            int32 Best = 0;
            for (int32 i = 0; i < USocialStatsComponent::StatCount; ++i)
                Best = FMath::Max(Best, Soc->GetRank(static_cast<ESocialStat>(i)));
            return Best;
        }
        return 0;
    }
    return 0;
}

void UAchievementComponent::CheckAll()
{
    for (const FAchievementDef& Def : GetCatalog())
    {
        if (Def.Id.IsNone()) continue;
        if (USecretSaveGame::IsCollected(Def.Id)) continue; // 이미 해금

        if (GetProgressFor(Def.Type) >= Def.Threshold)
        {
            USecretSaveGame::MarkCollected(Def.Id);

            // 보상 지급
            FString RewardMsg;
            if (AActor* Owner = GetOwner())
            {
                if (Def.RewardGold > 0)
                {
                    if (UStatComponent* Stat = Owner->FindComponentByClass<UStatComponent>())
                        Stat->AddGold(Def.RewardGold);
                    RewardMsg += FString::Printf(TEXT("  +%d G"), Def.RewardGold);
                }
                if (!Def.RewardItemId.IsNone() && Def.RewardItemCount > 0)
                {
                    if (UInventoryComponent* Inv = Owner->FindComponentByClass<UInventoryComponent>())
                        Inv->AddItem(Def.RewardItemId, Def.RewardItemCount);
                    RewardMsg += FString::Printf(TEXT("  +%s x%d"), *Def.RewardItemId.ToString(), Def.RewardItemCount);
                }
                // 보상 반영분 영구화
                if (UStatComponent* Stat = Owner->FindComponentByClass<UStatComponent>())
                    USecretSaveGame::SavePlayerProgression(Stat);
            }

            if (GEngine)
                GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow,
                    FString::Printf(TEXT("🏆 업적 달성: %s — %s%s"), *Def.Title, *Def.Description, *RewardMsg));
            OnAchievementUnlocked.Broadcast(Def.Id);
        }
    }
}

void UAchievementComponent::GetStatuses(TArray<FAchievementStatus>& OutList) const
{
    OutList.Reset();
    for (const FAchievementDef& Def : GetCatalog())
    {
        FAchievementStatus S;
        S.Title = Def.Title;
        S.Description = Def.Description;
        S.Threshold = Def.Threshold;
        S.Current = FMath::Min(GetProgressFor(Def.Type), Def.Threshold);
        S.bUnlocked = USecretSaveGame::IsCollected(Def.Id);

        // 보상 표기
        if (Def.RewardGold > 0)
            S.Reward += FString::Printf(TEXT("%d G"), Def.RewardGold);
        if (!Def.RewardItemId.IsNone() && Def.RewardItemCount > 0)
        {
            if (!S.Reward.IsEmpty()) S.Reward += TEXT(", ");
            S.Reward += FString::Printf(TEXT("%s x%d"), *Def.RewardItemId.ToString(), Def.RewardItemCount);
        }

        OutList.Add(S);
    }
}
