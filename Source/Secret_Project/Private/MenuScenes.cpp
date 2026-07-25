#include "MenuScenes.h"
#include "UIRuntime.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

#include "InventoryComponent.h"
#include "StatComponent.h"
#include "TimeComponent.h"
#include "QuestComponent.h"
#include "SocialStatsComponent.h"
#include "RelationshipComponent.h"
#include "BestiarySubsystem.h"
#include "BattleTypes.h"
#include "AchievementComponent.h"
#include "AreaTriggerActor.h"
#include "SecretSaveGame.h"

static FString InvEffectText(const FConsumableDef& D)
{
    switch (D.Effect)
    {
    case EConsumableEffect::HealHP:      return FString::Printf(TEXT("HP 회복 +%.0f"), D.Magnitude);
    case EConsumableEffect::HealSP:      return FString::Printf(TEXT("SP 회복 +%.0f"), D.Magnitude);
    case EConsumableEffect::FullHeal:    return TEXT("HP·SP 완전 회복");
    case EConsumableEffect::CureAilment: return TEXT("상태이상 치료");
    case EConsumableEffect::KeyItem:     return TEXT("열쇠 아이템");
    default:                             return TEXT("");
    }
}

FString UMenuScenes::BuildBag(UPanelWidget* Into, APawn* P)
{
    UInventoryComponent* Inv = P ? P->FindComponentByClass<UInventoryComponent>() : nullptr;
    static const TArray<FItemStack> Empty;
    const TArray<FItemStack>& Stacks = Inv ? Inv->GetStacks() : Empty;
    if (Stacks.Num() == 0)
    {
        UUIRuntime::AddText(Into, TEXT("가방이 비어 있습니다."), UIColor::Sub, 18, 1, 0.f);
        return TEXT("가방");
    }
    // 전용 레이아웃: 2열 아이템 그리드
    int32 Count = 0, Col = 0;
    UHorizontalBox* GridRow = nullptr;
    for (const FItemStack& S : Stacks)
    {
        if (S.Count <= 0) continue;
        ++Count;
        if (Col == 0) GridRow = UUIRuntime::AddRow(Into, 8.f);
        FConsumableDef Def;
        const bool bHas = UInventoryComponent::FindDef(S.Id, Def);
        const FString Name = bHas ? Def.Name : S.Id.ToString();

        UVerticalBox* Card = UUIRuntime::AddCardFill(GridRow, UIColor::Card);
        UHorizontalBox* NameRow = UUIRuntime::AddRow(Card, 2.f);
        UUIRuntime::RowText(NameRow, Name, UIColor::Title, 18, 0, true);
        UUIRuntime::RowText(NameRow, FString::Printf(TEXT("x%d"), S.Count), UIColor::Accent, 18, 2, false);

        FString Desc = bHas ? InvEffectText(Def) : FString();
        if (bHas && !Def.Description.IsEmpty())
            Desc += (Desc.IsEmpty() ? TEXT("") : TEXT("  —  ")) + Def.Description;
        if (!Desc.IsEmpty())
            UUIRuntime::AddText(Card, Desc, UIColor::Sub, 12, 0, 0.f, true);
        Col = (Col + 1) % 2;
    }
    return FString::Printf(TEXT("가방   %d종"), Count);
}

FString UMenuScenes::BuildStatus(UPanelWidget* Into, APawn* P)
{
    UStatComponent* St = P ? P->FindComponentByClass<UStatComponent>() : nullptr;
    if (!St)
    {
        UUIRuntime::AddText(Into, TEXT("플레이어 정보 없음"), UIColor::Sub, 18, 1, 0.f);
        return TEXT("상태");
    }
    const FLinearColor HpCol(0.92f, 0.34f, 0.32f, 1.f), SpCol(0.35f, 0.62f, 0.95f, 1.f), XpCol(0.98f, 0.86f, 0.5f, 1.f);
    {
        UVerticalBox* C = UUIRuntime::AddCard(Into, UIColor::Card, 10.f);
        UHorizontalBox* H = UUIRuntime::AddRow(C, 6.f);
        UUIRuntime::RowText(H, FString::Printf(TEXT("Lv %d"), St->GetLevel()), UIColor::Title, 24, 0, true);
        UUIRuntime::RowText(H, FString::Printf(TEXT("%d G"), St->GetGold()), UIColor::Accent, 18, 2, false);

        UUIRuntime::AddText(C, FString::Printf(TEXT("HP   %.0f / %.0f"), St->GetCurrentHP(), St->GetMaxHP()), UIColor::Sub, 14, 0, 1.f);
        UUIRuntime::AddBar(C, St->GetMaxHP() > 0 ? St->GetCurrentHP() / St->GetMaxHP() : 0.f, HpCol, 14.f, 6.f);
        UUIRuntime::AddText(C, FString::Printf(TEXT("SP   %.0f / %.0f"), St->GetCurrentSP(), St->GetMaxSP()), UIColor::Sub, 14, 0, 1.f);
        UUIRuntime::AddBar(C, St->GetMaxSP() > 0 ? St->GetCurrentSP() / St->GetMaxSP() : 0.f, SpCol, 14.f, 6.f);
        UUIRuntime::AddText(C, FString::Printf(TEXT("EXP  %d / %d"), St->GetCurrentXP(), St->GetXPToNext()), UIColor::Sub, 14, 0, 1.f);
        UUIRuntime::AddBar(C, St->GetXPToNext() > 0 ? (float)St->GetCurrentXP() / St->GetXPToNext() : 1.f, XpCol, 12.f, 6.f);

        UHorizontalBox* H2 = UUIRuntime::AddRow(C, 0.f);
        UUIRuntime::RowText(H2, FString::Printf(TEXT("공격  %.0f"), St->GetAttack()), UIColor::Title, 18, 0, true);
        UUIRuntime::RowText(H2, FString::Printf(TEXT("방어  %.0f"), St->GetDefense()), UIColor::Title, 18, 0, true);
    }
    if (St->GetSTR() > 0.f || St->GetMAG() > 0.f)
    {
        UVerticalBox* C = UUIRuntime::AddCard(Into, UIColor::Card, 10.f);
        UUIRuntime::AddText(C, TEXT("세부 스탯"), UIColor::Accent, 18, 0, 4.f);
        const float Vals[5] = { St->GetSTR(), St->GetMAG(), St->GetVIT(), St->GetAGI(), St->GetLUK() };
        const TCHAR* Names[5] = { TEXT("힘 STR"), TEXT("마력 MAG"), TEXT("체력 VIT"), TEXT("민첩 AGI"), TEXT("운 LUK") };
        float Mx = 1.f; for (float V : Vals) Mx = FMath::Max(Mx, V);
        for (int32 i = 0; i < 5; ++i)
        {
            UHorizontalBox* R = UUIRuntime::AddRow(C, 2.f);
            UUIRuntime::RowText(R, Names[i], UIColor::Sub, 15, 0, true);
            UUIRuntime::RowText(R, FString::Printf(TEXT("%.0f"), Vals[i]), UIColor::Title, 15, 2, false);
            UUIRuntime::AddBar(C, Vals[i] / Mx, UIColor::Fill, 10.f, 5.f);
        }
    }
    {
        UVerticalBox* C = UUIRuntime::AddCard(Into, UIColor::Card, 0.f);
        if (UTimeComponent* T = P->FindComponentByClass<UTimeComponent>())
            UUIRuntime::AddText(C, FString::Printf(TEXT("날짜   %s"), *T->GetTimeLabel()), UIColor::Sub, 15, 0, 3.f);
        if (UQuestComponent* Q = P->FindComponentByClass<UQuestComponent>())
            UUIRuntime::AddText(C, FString::Printf(TEXT("완료한 퀘스트   %d"), Q->GetCompletedCount()), UIColor::Sub, 15, 0, 0.f);
    }
    return TEXT("상태");
}

FString UMenuScenes::BuildSocial(UPanelWidget* Into, APawn* P)
{
    USocialStatsComponent* Social = P ? P->FindComponentByClass<USocialStatsComponent>() : nullptr;
    static const FLinearColor Fill5[5] = {
        FLinearColor(0.35f, 0.62f, 0.95f, 1.f), FLinearColor(0.92f, 0.45f, 0.70f, 1.f),
        FLinearColor(0.93f, 0.35f, 0.30f, 1.f), FLinearColor(0.45f, 0.85f, 0.55f, 1.f),
        FLinearColor(0.85f, 0.70f, 0.35f, 1.f)
    };
    if (!Social)
    {
        UUIRuntime::AddText(Into, TEXT("사회 스탯 정보를 찾을 수 없습니다."), UIColor::Sub, 18, 1, 0.f);
        return TEXT("사회 스탯");
    }
    // 전용 레이아웃: 2열 큰 스탯 게이지 그리드
    int32 Maxed = 0, Col = 0;
    UHorizontalBox* GridRow = nullptr;
    for (int32 i = 0; i < USocialStatsComponent::StatCount; ++i)
    {
        const ESocialStat Stat = static_cast<ESocialStat>(i);
        const int32 Rank = Social->GetRank(Stat);
        if (Rank >= USocialStatsComponent::MaxRank) ++Maxed;
        int32 Cur = 0, Needed = 0;
        Social->GetRankProgress(Stat, Cur, Needed);
        const float Pct = (Needed > 0) ? (float)Cur / (float)Needed : 1.f;

        if (Col == 0) GridRow = UUIRuntime::AddRow(Into, 8.f);
        UVerticalBox* Card = UUIRuntime::AddCardFill(GridRow, UIColor::Card);
        UUIRuntime::AddText(Card, LexSocialStat(Stat), UIColor::Title, 19, 0, 2.f);
        UUIRuntime::AddText(Card, FString::Printf(TEXT("Rank %d / %d"), Rank, USocialStatsComponent::MaxRank), UIColor::Accent, 22, 0, 4.f);
        UUIRuntime::AddBar(Card, Pct, Fill5[i % 5], 18.f, 4.f);
        const FString SubTxt = (Rank >= USocialStatsComponent::MaxRank)
            ? FString(TEXT("최대 랭크"))
            : FString::Printf(TEXT("%d / %d"), Cur, Needed);
        UUIRuntime::AddText(Card, SubTxt, UIColor::Sub, 12, 0, 0.f);
        Col = (Col + 1) % 2;
    }
    return FString::Printf(TEXT("사회 스탯   (최대치 %d/%d)"), Maxed, USocialStatsComponent::StatCount);
}

FString UMenuScenes::BuildQuests(UPanelWidget* Into, APawn* P)
{
    UQuestComponent* Quests = P ? P->FindComponentByClass<UQuestComponent>() : nullptr;
    TArray<FQuestDef> Active;
    if (Quests) Quests->GetActiveQuests(Active);
    if (Active.Num() == 0)
    {
        UUIRuntime::AddText(Into, TEXT("진행중인 퀘스트가 없습니다."), UIColor::Sub, 18, 1, 0.f);
        return TEXT("퀘스트");
    }
    for (const FQuestDef& Q : Active)
    {
        UVerticalBox* Card = UUIRuntime::AddCard(Into, UIColor::Card, 8.f);
        UUIRuntime::AddText(Card, Q.Title, UIColor::Title, 20, 0, 2.f);
        if (!Q.Description.IsEmpty())
            UUIRuntime::AddText(Card, Q.Description, UIColor::Sub, 14, 0, 4.f, true);

        if ((Q.Objective == EQuestObjective::OpenChests || Q.Objective == EQuestObjective::DefeatEnemies) && Quests)
        {
            const int32 Prog = Quests->GetProgress(Q.QuestId);
            const float Pct = (Q.TargetCount > 0) ? (float)Prog / (float)Q.TargetCount : 0.f;
            UUIRuntime::AddBar(Card, Pct, UIColor::Fill, 14.f, 3.f);
            UUIRuntime::AddText(Card, FString::Printf(TEXT("%d / %d"), Prog, Q.TargetCount), UIColor::Accent, 13, 2, 0.f);
        }
        else if (Q.Objective == EQuestObjective::ReachDay)
            UUIRuntime::AddText(Card, FString::Printf(TEXT("목표: %d일차까지"), Q.TargetCount), UIColor::Accent, 14, 0, 0.f);
        else if (Q.Objective == EQuestObjective::TalkToNPC && !Q.TargetNPCName.IsNone())
            UUIRuntime::AddText(Card, FString::Printf(TEXT("→ %s 와(과) 대화"), *Q.TargetNPCName.ToString()), UIColor::Accent, 14, 0, 0.f);
    }
    return FString::Printf(TEXT("퀘스트   %d개"), Active.Num());
}

FString UMenuScenes::BuildBond(UPanelWidget* Into, APawn* P)
{
    URelationshipComponent* Rel = P ? P->FindComponentByClass<URelationshipComponent>() : nullptr;
    TArray<FRelationshipRecord> Records;
    if (Rel) Rel->GetAllRelationships(Records);
    if (Records.Num() == 0)
    {
        UUIRuntime::AddText(Into, TEXT("아직 인연을 맺은 상대가 없습니다."), UIColor::Sub, 18, 1, 2.f);
        UUIRuntime::AddText(Into, TEXT("NPC와 대화해 인연을 쌓아보세요."), UIColor::Dim, 14, 1, 0.f);
        return TEXT("인연");
    }
    // 전용 레이아웃: 2열 인연 카드 그리드
    const FLinearColor BondFill(0.92f, 0.45f, 0.70f, 1.f);
    int32 Count = 0, Maxed = 0, Col = 0;
    UHorizontalBox* GridRow = nullptr;
    for (const FRelationshipRecord& R : Records)
    {
        ++Count;
        const bool bMax = (R.Rank >= URelationshipComponent::MaxRank);
        if (bMax) ++Maxed;
        int32 Cur = 0, Needed = URelationshipComponent::PointsPerRank;
        Rel->GetRankProgress(R.NPCName, Cur, Needed);
        const float Pct = bMax ? 1.f : ((Needed > 0) ? (float)Cur / (float)Needed : 0.f);

        if (Col == 0) GridRow = UUIRuntime::AddRow(Into, 8.f);
        UVerticalBox* Card = UUIRuntime::AddCardFill(GridRow, UIColor::Card);
        UUIRuntime::AddText(Card, R.NPCName.ToString(), UIColor::Title, 18, 0, 2.f);
        UUIRuntime::AddText(Card, bMax ? FString(TEXT("MAX")) : FString::Printf(TEXT("Rank %d"), R.Rank), UIColor::Accent, 16, 0, 4.f);
        UUIRuntime::AddBar(Card, Pct, BondFill, 14.f, 2.f);
        UUIRuntime::AddText(Card, bMax ? FString(TEXT("최고의 인연")) : FString::Printf(TEXT("%d / %d"), Cur, Needed), UIColor::Sub, 12, 0, 0.f);
        Col = (Col + 1) % 2;
    }
    return FString::Printf(TEXT("인연   %d명   (최고 %d)"), Count, Maxed);
}

FString UMenuScenes::BuildBestiary(UPanelWidget* Into, APawn* P)
{
    UBestiarySubsystem* Bst = nullptr;
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(P))
        Bst = GI->GetSubsystem<UBestiarySubsystem>();
    // 전용 레이아웃: 2열 적 카드 그리드(약점 강조)
    int32 Count = 0, Col = 0;
    UHorizontalBox* GridRow = nullptr;
    if (Bst)
    {
        for (const TPair<FName, FBestiaryEntry>& Pair : Bst->GetAll())
        {
            ++Count;
            const FBestiaryEntry& E = Pair.Value;
            FString Weak;
            for (int32 i = 0; i < E.KnownWeak.Num(); ++i)
            {
                if (i > 0) Weak += TEXT(", ");
                Weak += LexBattleElement(E.KnownWeak[i]);
            }
            const bool bKnown = !Weak.IsEmpty();

            if (Col == 0) GridRow = UUIRuntime::AddRow(Into, 8.f);
            UVerticalBox* Card = UUIRuntime::AddCardFill(GridRow, UIColor::Card);
            UUIRuntime::AddText(Card, Pair.Key.ToString(), UIColor::Title, 18, 0, 2.f);
            UUIRuntime::AddText(Card, FString::Printf(TEXT("처치 %d"), E.DefeatedCount), UIColor::Sub, 14, 0, 4.f);
            UUIRuntime::AddText(Card, FString::Printf(TEXT("약점: %s"), bKnown ? *Weak : TEXT("??? (분석 필요)")),
                                bKnown ? UIColor::Accent : UIColor::Dim, 14, 0, 0.f, true);
            Col = (Col + 1) % 2;
        }
    }
    if (Count == 0)
        UUIRuntime::AddText(Into, TEXT("아직 조우한 적이 없습니다."), UIColor::Sub, 18, 1, 0.f);
    return FString::Printf(TEXT("적 도감   %d종"), Count);
}

FString UMenuScenes::BuildAchievements(UPanelWidget* Into, APawn* P)
{
    UAchievementComponent* Ach = P ? P->FindComponentByClass<UAchievementComponent>() : nullptr;
    if (Ach) Ach->CheckAll();
    // 전용 레이아웃: 2열 도전과제 카드 그리드(★해금/☆진행)
    int32 Unlocked = 0, Total = 0, Col = 0;
    UHorizontalBox* GridRow = nullptr;
    TArray<FAchievementStatus> Statuses;
    if (Ach) Ach->GetStatuses(Statuses);
    Total = Statuses.Num();
    for (const FAchievementStatus& S : Statuses)
    {
        if (S.bUnlocked) ++Unlocked;
        if (Col == 0) GridRow = UUIRuntime::AddRow(Into, 8.f);
        UVerticalBox* Card = UUIRuntime::AddCardFill(GridRow, UIColor::Card);
        UUIRuntime::AddText(Card, FString::Printf(TEXT("%s %s"), S.bUnlocked ? TEXT("★") : TEXT("☆"), *S.Title),
                            S.bUnlocked ? UIColor::Accent : UIColor::Title, 17, 0, 3.f);
        UUIRuntime::AddText(Card, S.bUnlocked ? FString(TEXT("달성")) : FString::Printf(TEXT("%d / %d"), S.Current, S.Threshold),
                            S.bUnlocked ? UIColor::Good : UIColor::Sub, 14, 0, 3.f);
        if (!S.bUnlocked && S.Threshold > 0)
            UUIRuntime::AddBar(Card, (float)S.Current / (float)S.Threshold, UIColor::Fill, 12.f, 3.f);
        if (!S.Description.IsEmpty())
            UUIRuntime::AddText(Card, S.Description, UIColor::Sub, 12, 0, 0.f, true);
        if (!S.Reward.IsEmpty())
            UUIRuntime::AddText(Card, FString::Printf(TEXT("보상: %s"), *S.Reward), UIColor::Accent, 12, 0, 0.f);
        Col = (Col + 1) % 2;
    }
    if (Total == 0)
        UUIRuntime::AddText(Into, TEXT("업적 데이터가 없습니다."), UIColor::Sub, 18, 1, 0.f);
    return FString::Printf(TEXT("도전과제   %d / %d"), Unlocked, Total);
}

FString UMenuScenes::BuildDiscovery(UPanelWidget* Into, APawn* /*P*/)
{
    // 전용 레이아웃: 2열 지역 그리드(✓ 발견 / ??? 미발견)
    const TArray<FRegionInfo>& Regions = AAreaTriggerActor::GetRegisteredRegions();
    int32 Found = 0, Col = 0;
    UHorizontalBox* GridRow = nullptr;
    for (const FRegionInfo& R : Regions)
    {
        const bool bDisc = USecretSaveGame::IsCollected(R.Id);
        if (bDisc) ++Found;
        if (Col == 0) GridRow = UUIRuntime::AddRow(Into, 8.f);
        UVerticalBox* Card = UUIRuntime::AddCardFill(GridRow, UIColor::Card);
        UUIRuntime::AddText(Card, bDisc ? FString::Printf(TEXT("✓ %s"), *R.Name) : FString(TEXT("??? 미발견")),
                            bDisc ? UIColor::Title : UIColor::Dim, 17, 0, 2.f);
        UUIRuntime::AddText(Card, bDisc ? TEXT("발견함") : TEXT("미발견 지역"),
                            bDisc ? UIColor::Good : UIColor::Sub, 12, 0, 0.f);
        Col = (Col + 1) % 2;
    }
    if (Regions.Num() == 0)
        UUIRuntime::AddText(Into, TEXT("등록된 지역이 없습니다."), UIColor::Sub, 18, 1, 0.f);
    return FString::Printf(TEXT("발견 지역   %d / %d"), Found, Regions.Num());
}
