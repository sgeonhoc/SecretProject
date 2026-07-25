#include "SocialStatsComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

USocialStatsComponent::USocialStatsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USocialStatsComponent::BeginPlay()
{
    Super::BeginPlay();
    // 길이 보장 (세이브 로드 전 기본값)
    if (Points.Num() != StatCount)
    {
        Points.Init(0, StatCount);
    }
    LoadFromSlot();
}

// 랭크별 시작 누적 포인트 임계값 (Rank 1 = 0). Rank 2~5는 점증.
int32 USocialStatsComponent::ThresholdForRank(int32 Rank)
{
    // 압축 플레이(약 20일 캘린더) 기준 곡선: 초반 랭크는 빨리(시험·이벤트에 의미), 만렙은 꾸준함 요구.
    // +2/매칭대화, +5/표지판 → R2=5회, R3=12회, R4=22회, R5=35회 분량.
    static const int32 Thresholds[MaxRank + 1] = { 0, 0, 10, 25, 45, 70 };
    Rank = FMath::Clamp(Rank, 1, MaxRank);
    return Thresholds[Rank];
}

int32 USocialStatsComponent::RankForPoints(int32 P)
{
    int32 Rank = 1;
    for (int32 R = MaxRank; R >= 1; --R)
        if (P >= ThresholdForRank(R)) { Rank = R; break; }
    return Rank;
}

void USocialStatsComponent::AddPoints(ESocialStat Stat, int32 Amount)
{
    if (Amount <= 0) return;
    if (Points.Num() != StatCount) Points.Init(0, StatCount);

    const int32 Idx = static_cast<int32>(Stat);
    if (!Points.IsValidIndex(Idx)) return;

    const int32 OldRank = RankForPoints(Points[Idx]);
    Points[Idx] += Amount;
    const int32 NewRank = RankForPoints(Points[Idx]);

    SaveToSlot();

    if (NewRank > OldRank)
    {
        OnSocialStatChanged.Broadcast(Stat, NewRank);
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan,
                FString::Printf(TEXT("%s 랭크 상승! → Rank %d"), *LexSocialStat(Stat), NewRank));
    }
}

int32 USocialStatsComponent::GetPoints(ESocialStat Stat) const
{
    const int32 Idx = static_cast<int32>(Stat);
    return Points.IsValidIndex(Idx) ? Points[Idx] : 0;
}

int32 USocialStatsComponent::GetRank(ESocialStat Stat) const
{
    return RankForPoints(GetPoints(Stat));
}

void USocialStatsComponent::GetRankProgress(ESocialStat Stat, int32& OutInto, int32& OutNeeded) const
{
    const int32 P = GetPoints(Stat);
    const int32 Rank = RankForPoints(P);
    if (Rank >= MaxRank)
    {
        const int32 Span = ThresholdForRank(MaxRank) - ThresholdForRank(MaxRank - 1);
        OutInto = Span;
        OutNeeded = Span;
        return;
    }
    const int32 Base = ThresholdForRank(Rank);
    OutInto = P - Base;
    OutNeeded = ThresholdForRank(Rank + 1) - Base;
}

FString USocialStatsComponent::GetStatLabel(ESocialStat Stat) const
{
    int32 Into = 0, Needed = 0;
    GetRankProgress(Stat, Into, Needed);
    const int32 Rank = GetRank(Stat);
    if (Rank >= MaxRank)
        return FString::Printf(TEXT("%s — Rank %d (MAX)"), *LexSocialStat(Stat), Rank);
    return FString::Printf(TEXT("%s — Rank %d (%d/%d)"), *LexSocialStat(Stat), Rank, Into, Needed);
}

int32 USocialStatsComponent::GetMaxedCount() const
{
    int32 Count = 0;
    for (int32 i = 0; i < StatCount; ++i)
        if (RankForPoints(Points.IsValidIndex(i) ? Points[i] : 0) >= MaxRank) ++Count;
    return Count;
}

void USocialStatsComponent::SaveToSlot() const
{
    USocialStatsSave* Save = Cast<USocialStatsSave>(
        UGameplayStatics::CreateSaveGameObject(USocialStatsSave::StaticClass()));
    if (!Save) return;
    Save->Points = Points;
    UGameplayStatics::SaveGameToSlot(Save, SlotName(), 0);
}

void USocialStatsComponent::LoadFromSlot()
{
    if (!UGameplayStatics::DoesSaveGameExist(SlotName(), 0)) return;
    if (USocialStatsSave* Save = Cast<USocialStatsSave>(
        UGameplayStatics::LoadGameFromSlot(SlotName(), 0)))
    {
        Points = Save->Points;
        if (Points.Num() != StatCount) Points.SetNumZeroed(StatCount);
    }
}
