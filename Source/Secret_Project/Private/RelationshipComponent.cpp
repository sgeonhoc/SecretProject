#include "RelationshipComponent.h"
#include "StatComponent.h"
#include "SecretSaveGame.h"
#include "NPCArchetype.h"   // 인연 랭크업 시 인물 개성 대사(이름→프로필)
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"

URelationshipComponent::URelationshipComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FRelationshipRecord& URelationshipComponent::FindOrAddRecord(FName NPCName)
{
    for (FRelationshipRecord& R : Records)
        if (R.NPCName == NPCName) return R;

    FRelationshipRecord NewRec;
    NewRec.NPCName = NPCName;
    return Records[Records.Add(NewRec)];
}

bool URelationshipComponent::RegisterTalk(FName NPCName, int32 CurrentDay)
{
    if (NPCName.IsNone()) return false;

    FRelationshipRecord& Rec = FindOrAddRecord(NPCName);

    // 하루 1회 제한 (같은 날 또 말 걸어도 안 오름)
    if (Rec.LastTalkedDay == CurrentDay) return false;
    Rec.LastTalkedDay = CurrentDay;

    ApplyPoints(Rec, TalkPoints);
    return true; // 이번 호출에 실제로 호감도가 올랐음(하루 첫 대화) — 사회스탯 연동용
}

void URelationshipComponent::AddAffinity(FName NPCName, int32 Amount)
{
    if (NPCName.IsNone() || Amount <= 0) return;
    FRelationshipRecord& Rec = FindOrAddRecord(NPCName);
    ApplyPoints(Rec, Amount);
}

bool URelationshipComponent::TryGift(FName NPCName, int32 CurrentDay, int32 Amount)
{
    if (NPCName.IsNone() || Amount <= 0) return false;

    FRelationshipRecord& Rec = FindOrAddRecord(NPCName);
    if (Rec.LastGiftDay == CurrentDay) return false; // 하루 1회
    Rec.LastGiftDay = CurrentDay;

    ApplyPoints(Rec, Amount);
    return true;
}

void URelationshipComponent::ApplyPoints(FRelationshipRecord& Rec, int32 Amount)
{
    const int32 OldRank = Rec.Rank;
    Rec.Points += Amount;

    // 랭크 재계산 (최대 랭크 클램프)
    int32 NewRank = Rec.Points / PointsPerRank;
    if (NewRank > MaxRank) NewRank = MaxRank;
    Rec.Rank = NewRank;

    if (NewRank > OldRank)
    {
        OnRankUp(Rec);
    }
    else
    {
        // 진행만 됨 — 조용히 영구화 (랭크업 알림은 OnRankUp이 처리)
        PersistAndNotify(FString());
    }

    OnRelationshipChanged.Broadcast(Rec.NPCName, Rec.Rank);
}

void URelationshipComponent::OnRankUp(const FRelationshipRecord& Rec)
{
    // 보상: 랭크에 비례한 골드 (인연 깊어질수록 커짐)
    const int32 GoldReward = Rec.Rank * 10;
    if (AActor* Owner = GetOwner())
    {
        if (UStatComponent* Stat = Owner->FindComponentByClass<UStatComponent>())
            Stat->AddGold(GoldReward);
    }

    FString Msg = FString::Printf(TEXT("%s와의 인연이 깊어졌다! (랭크 %d)"),
        *Rec.NPCName.ToString(), Rec.Rank);
    if (Rec.Rank >= MaxRank)
        Msg = FString::Printf(TEXT("%s와의 인연이 최고조에 달했다! (랭크 MAX)"), *Rec.NPCName.ToString());
    if (GoldReward > 0)
        Msg += FString::Printf(TEXT("  +%d G"), GoldReward);

    PersistAndNotify(Msg);

    // 인물 개성 한마디(페르소나 코프 연출) — 이름으로 프로필 찾아 성격 톤 반응
    FNPCSocialProfile Prof;
    if (UNPCArchetypeLibrary::FindProfileByName(Rec.NPCName.ToString(), Prof))
    {
        const bool bMax = Rec.Rank >= MaxRank;
        FString Line;
        switch (Prof.Personality)
        {
        case ENPCPersonality::Cheerful:     Line = bMax ? TEXT("우리 영원한 베프다! 약속!") : TEXT("너랑 점점 더 친해지는 거 같아, 좋다!"); break;
        case ENPCPersonality::Shy:          Line = bMax ? TEXT("이젠… 당신이 제일 편해요. 정말로요.") : TEXT("조, 조금씩… 마음을 열게 되네요."); break;
        case ENPCPersonality::Intellectual: Line = bMax ? TEXT("자네는 내 가장 귀한 인연이야. 틀림없어.") : TEXT("자네와의 대화는 늘 배울 게 있군."); break;
        case ENPCPersonality::Tough:        Line = bMax ? TEXT("네 등은 내가 지킨다. 평생 간다, 우리.") : TEXT("흥… 너, 점점 마음에 드는데."); break;
        case ENPCPersonality::Kind:         Line = bMax ? TEXT("당신은 내게 둘도 없이 소중한 사람이에요.") : TEXT("당신과 가까워져서, 참 다행이에요."); break;
        case ENPCPersonality::Cool:         Line = bMax ? TEXT("……너한테만은, 전부 보여줘도 되겠어.") : TEXT("……너한테는 조금, 솔직해져도 되겠군."); break;
        }
        if (!Line.IsEmpty() && GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan,
                FString::Printf(TEXT("%s: %s"), *Rec.NPCName.ToString(), *Line));
    }
}

int32 URelationshipComponent::GetRank(FName NPCName) const
{
    for (const FRelationshipRecord& R : Records)
        if (R.NPCName == NPCName) return R.Rank;
    return 0;
}

int32 URelationshipComponent::GetPoints(FName NPCName) const
{
    for (const FRelationshipRecord& R : Records)
        if (R.NPCName == NPCName) return R.Points;
    return 0;
}

void URelationshipComponent::GetRankProgress(FName NPCName, int32& OutInto, int32& OutNeeded) const
{
    OutNeeded = PointsPerRank;
    OutInto = 0;
    for (const FRelationshipRecord& R : Records)
    {
        if (R.NPCName == NPCName)
        {
            if (R.Rank >= MaxRank) { OutInto = PointsPerRank; return; }
            OutInto = R.Points % PointsPerRank;
            return;
        }
    }
}

int32 URelationshipComponent::GetMaxedCount() const
{
    int32 Count = 0;
    for (const FRelationshipRecord& R : Records)
        if (R.Rank >= MaxRank) ++Count;
    return Count;
}

void URelationshipComponent::GetAllRelationships(TArray<FRelationshipRecord>& OutRecords) const
{
    OutRecords = Records;
}

void URelationshipComponent::PersistAndNotify(const FString& Message)
{
    // 인연 진행은 플레이어 진행 저장 경로에 함께 영구화 (SavePlayerProgression가 RelationshipComponent도 수집)
    if (AActor* Owner = GetOwner())
    {
        if (UStatComponent* Stat = Owner->FindComponentByClass<UStatComponent>())
            USecretSaveGame::SavePlayerProgression(Stat);
    }

    if (GEngine && !Message.IsEmpty())
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Magenta, Message);
}
