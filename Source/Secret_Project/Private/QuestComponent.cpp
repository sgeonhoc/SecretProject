#include "QuestComponent.h"
#include "StatComponent.h"
#include "InventoryComponent.h"
#include "SecretSaveGame.h"
#include "StoryManager.h"            // 스토리 플래그 게이트(읽기 전용 — A 스파인 비침범)
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"

UQuestComponent::UQuestComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

const TArray<FQuestDef>& UQuestComponent::GetCatalog()
{
    static TArray<FQuestDef> Catalog;
    if (Catalog.Num() == 0)
    {
        // ── 사이드/탐험 퀘스트 (데이터 주도) — 메인 스토리 스파인은 A의 StoryManager가 소유 ──
        {
            FQuestDef Q;
            Q.QuestId = TEXT("Q_FindTreasure");
            Q.Title = TEXT("첫 보물 사냥");
            Q.Description = TEXT("주변을 탐험해 보물상자 2개를 열어라.");
            Q.Objective = EQuestObjective::OpenChests;
            Q.TargetCount = 2;
            Q.RewardGold = 100;
            Q.NextQuestId = TEXT("Q_MeetMerchant");
            Catalog.Add(Q);
        }
        {
            FQuestDef Q;
            Q.QuestId = TEXT("Q_MeetMerchant");
            Q.Title = TEXT("단골 트기");
            Q.Description = TEXT("편의점 점장과 인사하고 단골을 트자.");
            Q.Objective = EQuestObjective::TalkToNPC;
            Q.TargetNPCName = TEXT("점장 태수");
            Q.RewardItemId = TEXT("HiPotion");
            Q.RewardItemCount = 1;
            Q.NextQuestId = TEXT("Q_ClearMonsters");
            Catalog.Add(Q);
        }
        {
            FQuestDef Q;
            Q.QuestId = TEXT("Q_ClearMonsters");
            Q.Title = TEXT("위협 소탕");
            Q.Description = TEXT("주변 적 3마리를 쓰러뜨려라.");
            Q.Objective = EQuestObjective::DefeatEnemies;
            Q.TargetCount = 3;
            Q.RewardGold = 200;
            Q.NextQuestId = TEXT("Q_Survive3Days");
            Catalog.Add(Q);
        }
        {
            FQuestDef Q;
            Q.QuestId = TEXT("Q_Survive3Days");
            Q.Title = TEXT("사흘을 버텨라");
            Q.Description = TEXT("3일차까지 생존하라(휴식으로 날짜 진행).");
            Q.Objective = EQuestObjective::ReachDay;
            Q.TargetCount = 3;
            Q.RewardItemId = TEXT("HiPotion");
            Q.RewardItemCount = 1;
            Catalog.Add(Q);
        }
        // 퀘스트지급 NPC(GrantsQuestId)로 시작하는 독립 퀘스트 — 자동시작 안 됨
        {
            FQuestDef Q;
            Q.QuestId = TEXT("Q_TalkElder");
            Q.Title = TEXT("담임 면담");
            Q.Description = TEXT("윤재 선생님과 면담하라.");
            Q.Objective = EQuestObjective::TalkToNPC;
            Q.TargetNPCName = TEXT("윤재 선생님");
            Q.RewardGold = 80;
            Catalog.Add(Q);
        }
        {
            FQuestDef Q;
            Q.QuestId = TEXT("Q_BountyHunt");
            Q.Title = TEXT("현상금 사냥");
            Q.Description = TEXT("적 5마리를 처치하라.");
            Q.Objective = EQuestObjective::DefeatEnemies;
            Q.TargetCount = 5;
            Q.RewardGold = 300;
            Catalog.Add(Q);
        }
        {
            FQuestDef Q;
            Q.QuestId = TEXT("Q_Treasures5");
            Q.Title = TEXT("보물 수집가");
            Q.Description = TEXT("보물상자 5개를 열어라.");
            Q.Objective = EQuestObjective::OpenChests;
            Q.TargetCount = 5;
            Q.RewardItemId = TEXT("OldKey");
            Q.RewardItemCount = 1;
            Catalog.Add(Q);
        }
        // ── 현대 배경 NPC 양산 퀘스트 (소셜 카탈로그 GrantsQuestId와 매칭) ──
        // 윤재 선생님(char_03)이 주는 퀘스트: 교내 괴담 조사 (단서 2곳 확인 = 상자열기로 추적)
        {
            FQuestDef Q;
            Q.QuestId = TEXT("quest_lore");
            Q.Title = TEXT("교내 괴담 조사");
            Q.Description = TEXT("윤재 선생님의 부탁 — 학교에 떠도는 괴담의 단서 2곳을 확인하라.");
            Q.Objective = EQuestObjective::OpenChests;
            Q.TargetCount = 2;
            Q.RewardGold = 120;
            Q.RewardItemId = TEXT("HiPotion");
            Q.RewardItemCount = 1;
            Catalog.Add(Q);
        }
        // 분식집 순자씨(char_09)가 주는 퀘스트: 배달 심부름 (단골 서연에게 전달 = 서연과 대화)
        {
            FQuestDef Q;
            Q.QuestId = TEXT("quest_errand");
            Q.Title = TEXT("분식집 배달 심부름");
            Q.Description = TEXT("순자씨의 부탁 — 단골손님 서연에게 도시락을 전해주자.");
            Q.Objective = EQuestObjective::TalkToNPC;
            Q.TargetNPCName = TEXT("서연");
            Q.RewardGold = 80;
            Q.RewardItemId = TEXT("HealPotion");
            Q.RewardItemCount = 2;
            Catalog.Add(Q);
        }

        // ── 스토리 게이팅 사이드 퀘스트 (RequiredStoryFlag로 메인 진행에 맞춰 자동 개방) ──
        // 메인 스파인(A)이 한 단계 나아갈 때마다 거리에 그 국면에 맞는 임무가 열린다. 메인 스토리 자체는 아님(B 콘텐츠).
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_team_first");
            Q.Title = TEXT("팀 첫 임무 — 호흡 맞추기");
            Q.Description = TEXT("막 뭉친 팀. 강현의 제안으로 이면의 그림자 3마리를 함께 정리하며 손발을 맞춰본다.");
            Q.Objective = EQuestObjective::DefeatEnemies;
            Q.TargetCount = 3;
            Q.RequiredStoryFlag = TEXT("TeamFormed");
            Q.RewardGold = 150;
            Q.RewardItemId = TEXT("HiPotion"); Q.RewardItemCount = 1;
            Catalog.Add(Q);
        }
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_clue_hunt");
            Q.Title = TEXT("흉흉한 거리 — 단서 수집");
            Q.Description = TEXT("윤 기자의 의뢰 — 흑마술사의 흔적이 남은 장소 3곳을 뒤져 단서를 모아라.");
            Q.Objective = EQuestObjective::OpenChests;
            Q.TargetCount = 3;
            Q.RequiredStoryFlag = TEXT("WarlockRumor");
            Q.RewardGold = 180;
            Catalog.Add(Q);
        }
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_rumor_rooftop");
            Q.Title = TEXT("괴담 확인 — 옥상의 가면");
            Q.Description = TEXT("'옥상의 가면' 괴담의 진위를 점술가 셀린에게 물어 단서를 얻는다.");
            Q.Objective = EQuestObjective::TalkToNPC;
            Q.TargetNPCName = TEXT("점술가 셀린");
            Q.RequiredStoryFlag = TEXT("WarlockRumor");
            Q.RewardGold = 120;
            Q.RewardItemId = TEXT("Antidote"); Q.RewardItemCount = 2;
            Catalog.Add(Q);
        }
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_before_raid");
            Q.Title = TEXT("결전 준비 — 거리를 비워라");
            Q.Description = TEXT("거점 진입 전, 길에 새어 나온 그림자 5마리를 처치해 시민들의 길을 터준다.");
            Q.Objective = EQuestObjective::DefeatEnemies;
            Q.TargetCount = 5;
            Q.RequiredStoryFlag = TEXT("WarlockLocated");
            Q.RewardGold = 220;
            Q.RewardItemId = TEXT("HiPotion"); Q.RewardItemCount = 2;
            Catalog.Add(Q);
        }
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_aftermath");
            Q.Title = TEXT("여진 — 남은 그림자 청소");
            Q.Description = TEXT("흑마술사는 쓰러졌지만 잔당이 남았다. 거리를 떠도는 그림자 4마리를 마저 정리한다.");
            Q.Objective = EQuestObjective::DefeatEnemies;
            Q.TargetCount = 4;
            Q.RequiredStoryFlag = TEXT("WarlockDefeated");
            Q.RewardGold = 200;
            Q.RewardItemId = TEXT("Elixir"); Q.RewardItemCount = 1;
            Catalog.Add(Q);
        }
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_back_to_days");
            Q.Title = TEXT("일상으로 — 고마운 사람들에게");
            Q.Description = TEXT("모든 게 끝났다. 늘 곁을 지켜준 민지 쌤에게 고맙다는 인사를 전하러 간다.");
            Q.Objective = EQuestObjective::TalkToNPC;
            Q.TargetNPCName = TEXT("민지 쌤");
            Q.RequiredStoryFlag = TEXT("StoryClear");
            Q.RewardGold = 300;
            Q.RewardItemId = TEXT("Elixir"); Q.RewardItemCount = 2;
            Catalog.Add(Q);
        }

        // ── NPC 부여 내러티브 사이드 퀘스트 (분위기 NPC에 작은 이야기 부여) ──
        // 셀린(char_16): 점술 흉조 확인
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_tarot_reading");
            Q.Title = TEXT("별이 가리키는 곳");
            Q.Description = TEXT("점술가 셀린이 흉조를 읽었다. \"카드가 가리키는 그림자 넷을 직접 쳐서, 흉이 맞는지 확인해 줘.\"");
            Q.Objective = EQuestObjective::DefeatEnemies;
            Q.TargetCount = 4;
            Q.RewardGold = 160;
            Q.RewardItemId = TEXT("SPPotion"); Q.RewardItemCount = 2;
            Catalog.Add(Q);
        }
        // 한지원(char_23): 향토자료실 필사본 조각 수집
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_archive");
            Q.Title = TEXT("사라진 페이지");
            Q.Description = TEXT("사서 한지원의 부탁 — 향토자료실에서 흩어진 '군림하는 그림자' 필사본 조각 3개를 찾아 와라.");
            Q.Objective = EQuestObjective::OpenChests;
            Q.TargetCount = 3;
            Q.RewardGold = 140;
            Q.RewardItemId = TEXT("HiPotion"); Q.RewardItemCount = 1;
            Catalog.Add(Q);
        }
        // 하늘(char_12): 잃어버린 멜로디
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_lost_song");
            Q.Title = TEXT("잃어버린 멜로디");
            Q.Description = TEXT("버스커 하늘이 어떤 곡의 후렴을 잊었다. \"그 멜로디를 기억하는 사람이… 카페 바리스타 유나 씨일 거예요. 물어봐 줄래요?\"");
            Q.Objective = EQuestObjective::TalkToNPC;
            Q.TargetNPCName = TEXT("바리스타 유나");
            Q.RewardGold = 100;
            Q.RewardItemId = TEXT("HiPotion"); Q.RewardItemCount = 1;
            Catalog.Add(Q);
        }
        // 읽을거리 수집(80편 로어에 게임플레이 보상 루프) — 한울시의 진실을 파고드는 동기
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_archivist");
            Q.Title = TEXT("한울시의 기록");
            Q.Description = TEXT("거리의 신문·괴담·메모를 읽어 한울시의 진실에 다가가자. 읽을거리 5개를 읽어라.");
            Q.Objective = EQuestObjective::ReadLore;
            Q.TargetCount = 5;
            Q.RewardGold = 120;
            Q.RewardItemId = TEXT("HiPotion"); Q.RewardItemCount = 1;
            Q.NextQuestId = TEXT("sq_archivist2");
            Catalog.Add(Q);
        }
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_archivist2");
            Q.Title = TEXT("한울시의 기록 II");
            Q.Description = TEXT("더 깊이 — 읽을거리 12개를 읽어 흩어진 단서를 모아라.");
            Q.Objective = EQuestObjective::ReadLore;
            Q.TargetCount = 12;
            Q.RewardGold = 250;
            Q.RewardItemId = TEXT("Elixir"); Q.RewardItemCount = 1;
            Catalog.Add(Q);
        }
        // 구제 아크(§8): 공허에 흔들리는 이웃에게 손 내밀기 — 전투 없이 '한 사람'을 구한다.
        {
            FQuestDef Q;
            Q.QuestId = TEXT("sq_reach_out");
            Q.Title = TEXT("혼자 두지 않기");
            Q.Description = TEXT("미술부 하린이 요즘 회색만 칠한다. 공허에 잠기기 전에 — 그저 곁에 가서, 안부를 물어봐 주자. 그거면 충분할지도 모른다.");
            Q.Objective = EQuestObjective::TalkToNPC;
            Q.TargetNPCName = TEXT("하린");
            Q.RequiredStoryFlag = TEXT("WarlockRumor");
            Q.RewardGold = 130;
            Q.RewardItemId = TEXT("Antidote"); Q.RewardItemCount = 1;
            Catalog.Add(Q);
        }
    }
    return Catalog;
}

bool UQuestComponent::FindDef(FName QuestId, FQuestDef& OutDef)
{
    for (const FQuestDef& D : GetCatalog())
    {
        if (D.QuestId == QuestId) { OutDef = D; return true; }
    }
    return false;
}

void UQuestComponent::BeginPlay()
{
    Super::BeginPlay();

    // 진행 이력이 전혀 없으면 카탈로그 첫 퀘스트 자동 시작(데모 흐름)
    if (Records.Num() == 0)
    {
        const TArray<FQuestDef>& Cat = GetCatalog();
        if (Cat.Num() > 0)
            StartQuest(Cat[0].QuestId);
    }

    // 이미 진행된 스토리 단계가 있으면 그에 맞는 게이트 퀘스트도 즉시 개방(세이브 로드 후)
    CheckStoryQuests();
}

void UQuestComponent::CheckStoryQuests()
{
    UStoryManagerSubsystem* Story = nullptr;
    if (UWorld* W = GetWorld())
        if (UGameInstance* GI = W->GetGameInstance())
            Story = GI->GetSubsystem<UStoryManagerSubsystem>();
    if (!Story) return;

    for (const FQuestDef& Def : GetCatalog())
    {
        if (Def.RequiredStoryFlag.IsNone()) continue;          // 게이트 없는 일반 퀘스트는 무시
        if (GetQuestState(Def.QuestId) != EQuestState::Inactive) continue; // 이미 시작/완료
        if (Story->HasFlag(Def.RequiredStoryFlag))
            StartQuest(Def.QuestId);                            // 플래그 충족 → 임무 개방
    }
}

FQuestRecord* UQuestComponent::FindRecord(FName QuestId)
{
    for (FQuestRecord& R : Records)
        if (R.QuestId == QuestId) return &R;
    return nullptr;
}

void UQuestComponent::StartQuest(FName QuestId)
{
    if (QuestId.IsNone()) return;

    FQuestDef Def;
    if (!FindDef(QuestId, Def)) return;

    FQuestRecord* Rec = FindRecord(QuestId);
    if (Rec)
    {
        if (Rec->State != EQuestState::Inactive) return; // 이미 진행/완료
        Rec->State = EQuestState::Active;
    }
    else
    {
        FQuestRecord New;
        New.QuestId = QuestId;
        New.Progress = 0;
        New.State = EQuestState::Active;
        Records.Add(New);
    }

    PersistAndNotify(FString::Printf(TEXT("새 퀘스트: %s"), *Def.Title));
}

void UQuestComponent::NotifyTalkedTo(FName NPCName)
{
    if (NPCName.IsNone()) return;

    for (FQuestRecord& R : Records)
    {
        if (R.State != EQuestState::Active) continue;
        FQuestDef Def;
        if (!FindDef(R.QuestId, Def)) continue;
        if (Def.Objective == EQuestObjective::TalkToNPC && Def.TargetNPCName == NPCName)
            CompleteQuest(R, Def);
    }
}

void UQuestComponent::NotifyChestOpened()
{
    for (FQuestRecord& R : Records)
    {
        if (R.State != EQuestState::Active) continue;
        FQuestDef Def;
        if (!FindDef(R.QuestId, Def)) continue;
        if (Def.Objective != EQuestObjective::OpenChests) continue;

        R.Progress++;
        if (R.Progress >= Def.TargetCount)
        {
            CompleteQuest(R, Def);
        }
        else
        {
            PersistAndNotify(FString::Printf(TEXT("%s (%d/%d)"), *Def.Title, R.Progress, Def.TargetCount));
        }
    }
}

void UQuestComponent::NotifyEnemyDefeated(int32 Count)
{
    if (Count <= 0) return;

    for (FQuestRecord& R : Records)
    {
        if (R.State != EQuestState::Active) continue;
        FQuestDef Def;
        if (!FindDef(R.QuestId, Def)) continue;
        if (Def.Objective != EQuestObjective::DefeatEnemies) continue;

        R.Progress += Count;
        if (R.Progress >= Def.TargetCount)
            CompleteQuest(R, Def);
        else
            PersistAndNotify(FString::Printf(TEXT("%s (%d/%d)"), *Def.Title, R.Progress, Def.TargetCount));
    }
}

void UQuestComponent::NotifyDayReached(int32 CurrentDay)
{
    // 시간/날짜가 바뀔 때마다 스토리 진행 점검 → 새로 열린 단계의 사이드 임무 자동 개방
    CheckStoryQuests();

    for (FQuestRecord& R : Records)
    {
        if (R.State != EQuestState::Active) continue;
        FQuestDef Def;
        if (!FindDef(R.QuestId, Def)) continue;
        if (Def.Objective != EQuestObjective::ReachDay) continue;

        R.Progress = CurrentDay; // 표시용 현재 날짜
        if (CurrentDay >= Def.TargetCount)
            CompleteQuest(R, Def);
    }
}

void UQuestComponent::NotifyLoreRead(int32 Count)
{
    if (Count <= 0) return;

    // 첫 읽을거리를 읽으면 '한울시의 기록' 수집 퀘스트가 자연히 시작된다(체인으로 II 이어짐).
    if (GetQuestState(TEXT("sq_archivist")) == EQuestState::Inactive
        && GetQuestState(TEXT("sq_archivist2")) == EQuestState::Inactive)
        StartQuest(TEXT("sq_archivist"));

    for (FQuestRecord& R : Records)
    {
        if (R.State != EQuestState::Active) continue;
        FQuestDef Def;
        if (!FindDef(R.QuestId, Def)) continue;
        if (Def.Objective != EQuestObjective::ReadLore) continue;

        R.Progress += Count;
        if (R.Progress >= Def.TargetCount)
            CompleteQuest(R, Def);
        else
            PersistAndNotify(FString::Printf(TEXT("%s (%d/%d)"), *Def.Title, R.Progress, Def.TargetCount));
    }
}

void UQuestComponent::CompleteQuest(FQuestRecord& Rec, const FQuestDef& Def)
{
    Rec.State = EQuestState::Completed;

    // 보상 지급
    if (AActor* Owner = GetOwner())
    {
        if (UStatComponent* Stat = Owner->FindComponentByClass<UStatComponent>())
        {
            if (Def.RewardGold > 0)
                Stat->AddGold(Def.RewardGold);
        }
        if (!Def.RewardItemId.IsNone() && Def.RewardItemCount > 0)
        {
            if (UInventoryComponent* Inv = Owner->FindComponentByClass<UInventoryComponent>())
                Inv->AddItem(Def.RewardItemId, Def.RewardItemCount);
        }
    }

    FString Msg = FString::Printf(TEXT("퀘스트 완료: %s"), *Def.Title);
    if (Def.RewardGold > 0)
        Msg += FString::Printf(TEXT("  +%d G"), Def.RewardGold);
    if (!Def.RewardItemId.IsNone() && Def.RewardItemCount > 0)
        Msg += FString::Printf(TEXT("  +%s x%d"), *Def.RewardItemId.ToString(), Def.RewardItemCount);
    PersistAndNotify(Msg);

    // 체인: 다음 퀘스트 자동 시작
    if (!Def.NextQuestId.IsNone())
        StartQuest(Def.NextQuestId);
}

void UQuestComponent::PersistAndNotify(const FString& Message)
{
    // 퀘스트 진행은 플레이어 진행 저장 경로에 함께 영구화(SavePlayerProgression가 QuestComponent도 수집)
    if (AActor* Owner = GetOwner())
    {
        if (UStatComponent* Stat = Owner->FindComponentByClass<UStatComponent>())
            USecretSaveGame::SavePlayerProgression(Stat);
    }

    if (GEngine && !Message.IsEmpty())
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, Message);
}

EQuestState UQuestComponent::GetQuestState(FName QuestId) const
{
    for (const FQuestRecord& R : Records)
        if (R.QuestId == QuestId) return R.State;
    return EQuestState::Inactive;
}

int32 UQuestComponent::GetProgress(FName QuestId) const
{
    for (const FQuestRecord& R : Records)
        if (R.QuestId == QuestId) return R.Progress;
    return 0;
}

int32 UQuestComponent::GetCompletedCount() const
{
    int32 N = 0;
    for (const FQuestRecord& R : Records)
        if (R.State == EQuestState::Completed) ++N;
    return N;
}

void UQuestComponent::GetActiveQuests(TArray<FQuestDef>& OutQuests) const
{
    OutQuests.Reset();
    for (const FQuestRecord& R : Records)
    {
        if (R.State != EQuestState::Active) continue;
        FQuestDef Def;
        if (FindDef(R.QuestId, Def))
            OutQuests.Add(Def);
    }
}
