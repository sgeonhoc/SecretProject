#include "CalendarComponent.h"
#include "StatComponent.h"
#include "InventoryComponent.h"
#include "SocialStatsComponent.h"
#include "SecretSaveGame.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

UCalendarComponent::UCalendarComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

const TArray<FCalendarEvent>& UCalendarComponent::GetCatalog()
{
    static TArray<FCalendarEvent> Catalog;
    if (Catalog.Num() == 0)
    {
        // ── 현대 학사/도시 캘린더 이벤트 (페르소나식 — 데이터 주도라 교체/추가 쉬움) ──
        {
            FCalendarEvent E;
            E.EventId = TEXT("CE_NewSemester_D2");
            E.Day = 2;
            E.Title = TEXT("신학기");
            E.Message = TEXT("2일차 — 새 학기가 시작됐다. 부모님이 용돈을 주셨다.");
            E.RewardGold = 80;
            Catalog.Add(E);
        }
        {
            FCalendarEvent E;
            E.EventId = TEXT("CE_Midterm_D5");
            E.Day = 5;
            E.bAnyPhase = false;
            E.Phase = EDayPhase::Morning;
            E.Title = TEXT("중간고사");
            E.Message = TEXT("5일차 — 중간고사 날이다. 지식이 높으면 좋은 성적을!");
            E.RewardItemId = TEXT("SPPotion");
            E.RewardItemCount = 1;
            E.bExamBonusByKnowledge = true;
            E.ExamGoldPerKnowledgeRank = 30; // 지식 Rank5면 +150G 장학금
            Catalog.Add(E);
        }
        {
            FCalendarEvent E;
            E.EventId = TEXT("CE_SportsDay_D8");
            E.Day = 8;
            E.Title = TEXT("체육대회");
            E.Message = TEXT("8일차 — 체육대회! 매점에서 간식을 받았다.");
            E.RewardItemId = TEXT("HiPotion");
            E.RewardItemCount = 1;
            Catalog.Add(E);
        }
        {
            FCalendarEvent E;
            E.EventId = TEXT("CE_SchoolFestival_D12");
            E.Day = 12;
            E.bAnyPhase = false;
            E.Phase = EDayPhase::Evening;
            E.Title = TEXT("학교 문화제");
            E.Message = TEXT("12일차 저녁 — 문화제가 열렸다! 모금 수익을 나눠 받았다.");
            E.RewardGold = 150;
            Catalog.Add(E);
        }
        {
            FCalendarEvent E;
            E.EventId = TEXT("CE_Fireworks_D15");
            E.Day = 15;
            E.bAnyPhase = false;
            E.Phase = EDayPhase::Night;
            E.Title = TEXT("여름 불꽃축제");
            E.Message = TEXT("15일차 밤 — 강변 불꽃축제! 노점에서 에너지 드링크를 얻었다.");
            E.RewardItemId = TEXT("ManaFlower");
            E.RewardItemCount = 2;
            Catalog.Add(E);
        }
        {
            FCalendarEvent E;
            E.EventId = TEXT("CE_Finals_D20");
            E.Day = 20;
            E.bAnyPhase = false;
            E.Phase = EDayPhase::Morning;
            E.Title = TEXT("기말고사");
            E.Message = TEXT("20일차 — 기말고사. 그동안 쌓은 지식이 빛을 발한다.");
            E.RewardItemId = TEXT("Elixir");
            E.RewardItemCount = 1;
            E.bExamBonusByKnowledge = true;
            E.ExamGoldPerKnowledgeRank = 50; // 기말은 더 큼: 지식 Rank5면 +250G
            Catalog.Add(E);
        }
        // ── 스토리 분위기 비트 (메인 퀘스트 체인과 맞물리는 서사 타임라인) ──
        {
            FCalendarEvent E;
            E.EventId = TEXT("CE_Story_News_D4");
            E.Day = 4;
            E.Title = TEXT("뉴스 속보");
            E.Message = TEXT("4일차 — 도시 곳곳에서 원인불명 실종·폭주 사건이 잇따른다는 뉴스. 무언가 잘못되어 간다.");
            Catalog.Add(E);
        }
        {
            FCalendarEvent E;
            E.EventId = TEXT("CE_Story_Unrest_D10");
            E.Day = 10;
            E.bAnyPhase = false;
            E.Phase = EDayPhase::Night;
            E.Title = TEXT("짙어지는 이변");
            E.Message = TEXT("10일차 밤 — 거리의 공기가 무겁다. 이변이 짙어지고, 결전이 가까워지는 예감.");
            Catalog.Add(E);
        }
        {
            FCalendarEvent E;
            E.EventId = TEXT("CE_Story_Eve_D15");
            E.Day = 15;
            E.bAnyPhase = false;
            E.Phase = EDayPhase::Evening;
            E.Title = TEXT("결전의 날");
            E.Message = TEXT("15일차 — 불꽃축제로 들뜬 도시. 그러나 오늘 밤, 배후가 움직인다. 각오를 다지자.");
            Catalog.Add(E);
        }
    }
    return Catalog;
}

void UCalendarComponent::BeginPlay()
{
    Super::BeginPlay();

    // 플레이어 TimeComponent의 LoadTime(세이브 복원)이 플레이어 BeginPlay 후반부에 일어나므로
    // 구독 + 현재 날짜 평가를 다음 틱으로 미뤄 로드된 날짜 기준으로 동작하게 함.
    if (GetWorld())
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UCalendarComponent::InitCalendar);
}

void UCalendarComponent::InitCalendar()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    if (UTimeComponent* Time = Owner->FindComponentByClass<UTimeComponent>())
    {
        Time->OnTimeChanged.RemoveDynamic(this, &UCalendarComponent::OnWorldTimeChanged);
        Time->OnTimeChanged.AddDynamic(this, &UCalendarComponent::OnWorldTimeChanged);
        // 현재(로드된) 날짜/시간대에 대기 중인 이벤트가 있으면 즉시 발생
        CheckEvents(Time->GetDay(), Time->GetPhase());
    }
}

void UCalendarComponent::OnWorldTimeChanged(int32 Day, EDayPhase Phase)
{
    CheckEvents(Day, Phase);
}

void UCalendarComponent::CheckEvents(int32 Day, EDayPhase Phase)
{
    auto Evaluate = [&](const FCalendarEvent& E)
    {
        if (E.EventId.IsNone()) return;
        if (E.Day != Day) return;
        if (!E.bAnyPhase && E.Phase != Phase) return;
        if (USecretSaveGame::IsCollected(E.EventId)) return; // 이미 발생
        FireEvent(E);
    };

    for (const FCalendarEvent& E : GetCatalog()) Evaluate(E);
    for (const FCalendarEvent& E : ExtraEvents)  Evaluate(E);
}

void UCalendarComponent::FireEvent(const FCalendarEvent& Event)
{
    // 중복 방지 기록 (보물상자/지역/여행지점과 동일한 CollectedIds 재사용)
    USecretSaveGame::MarkCollected(Event.EventId);

    // 시험 보너스(페르소나식 "공부=성적"): 지식 랭크에 비례한 장학금. 사회스탯 시스템에 실질 보상.
    int32 ExamBonusGold = 0;
    int32 KnowledgeRank = 0;
    if (Event.bExamBonusByKnowledge && Event.ExamGoldPerKnowledgeRank > 0)
        if (AActor* O = GetOwner())
            if (USocialStatsComponent* Soc = O->FindComponentByClass<USocialStatsComponent>())
            {
                KnowledgeRank = Soc->GetRank(ESocialStat::Knowledge);
                ExamBonusGold = KnowledgeRank * Event.ExamGoldPerKnowledgeRank;
            }

    const int32 TotalGold = Event.RewardGold + ExamBonusGold;

    // 보상 지급 + 영구화
    if (AActor* Owner = GetOwner())
    {
        if (TotalGold > 0)
            if (UStatComponent* Stat = Owner->FindComponentByClass<UStatComponent>())
                Stat->AddGold(TotalGold);

        if (!Event.RewardItemId.IsNone() && Event.RewardItemCount > 0)
            if (UInventoryComponent* Inv = Owner->FindComponentByClass<UInventoryComponent>())
                Inv->AddItem(Event.RewardItemId, Event.RewardItemCount);

        if (UStatComponent* Stat = Owner->FindComponentByClass<UStatComponent>())
            USecretSaveGame::SavePlayerProgression(Stat);
    }

    FString Msg = Event.Message.IsEmpty() ? Event.Title : Event.Message;
    if (ExamBonusGold > 0)
        Msg += FString::Printf(TEXT("  [성적 우수! 지식 Rank %d → 장학금 +%d G]"), KnowledgeRank, ExamBonusGold);
    else if (TotalGold > 0)
        Msg += FString::Printf(TEXT("  +%d G"), TotalGold);
    if (GEngine && !Msg.IsEmpty())
        GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, Msg);

    OnCalendarEvent.Broadcast(Event.EventId);
}
