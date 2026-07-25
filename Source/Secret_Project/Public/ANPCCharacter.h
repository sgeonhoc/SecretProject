#pragma once
#include "CoreMinimal.h"
#include "ABaseCharacter.h"
#include "TimeComponent.h" // EDayPhase
#include "ANPCCharacter.generated.h"

class AABaseCharacter;
class UShopWidget;

/**
 * 스토리 반응 대사 한 묶음 — 플레이어가 특정 스토리 플래그를 보유하면 NPC가 그 사건에 반응해 말함.
 * "월드가 스토리에 살아있게": 공허현상 번짐 → 흑마술사 소문 → 격파 → 평화 회복 단계마다 거리 분위기가 바뀐다.
 * GetContextualLines가 가장 최근(최우선) 플래그를 골라 출력. 비우면 ArchetypeId NPC는 성격 기반 자동 생성.
 */
USTRUCT(BlueprintType)
struct FStoryReactiveLines
{
    GENERATED_BODY()

    // 이 대사가 활성화되는 스토리 플래그(StoryManager). 예: WarlockRumor, WarlockDefeated, StoryClear.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    FName RequiredFlag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TArray<FString> Lines;
};

UCLASS()
class SECRET_PROJECT_API AANPCCharacter : public AABaseCharacter
{
    GENERATED_BODY()

public:
    AANPCCharacter();

protected:
    virtual void BeginPlay() override;

public:
    // ── 양산 아키타입 (자동 콘텐츠) ───────────────────────
    // 설정 시 BeginPlay에서 카탈로그의 소셜 프로필(이름/대사/선물/스케줄/상점/퀘스트/영입)을 자동 적용.
    // 전투 프로필(스탯/스킬/약점)도 같은 Id로 A의 전투 카탈로그가 적용. None이면 아래 필드를 수동 사용(기존 동작).
    // 사용자: 이 Id만 맞추고 메시/애니만 입히면 캐릭터가 자동으로 굴러감.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    FName ArchetypeId;

    // ── 대화 ─────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    FString NPCName = TEXT("기본 이름");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TArray<FString> DialogueLines;

    // 저녁/밤 전용 대사(비면 기본 DialogueLines 사용). 플레이어 TimeComponent 기준.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TArray<FString> EveningDialogueLines;

    // 인연(소셜링크) 랭크가 BondLineMinRank 이상이고 비어있지 않으면 이 특별 대사를 사용.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TArray<FString> BondDialogueLines;

    // BondDialogueLines가 적용되는 최소 인연 랭크
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    int32 BondLineMinRank = 3;

    // 스토리 진행에 반응하는 대사(플래그별). 비우면 ArchetypeId NPC는 성격 기반 자동 생성.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TArray<FStoryReactiveLines> StoryReactiveLines;

    // 일상 균열(캐서린풍): 미스터리 단서를 모은 뒤(사건 해결 전), NPC가 잠깐 이상한 말을 흘리고 잊는다.
    // 단서 플래그(Clue_Voice/Clue_Throne/Clue_Truth) 보유 + WarlockDefeated 전에만 최우선 노출. 비우면 자동 생성.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TArray<FString> CreepyLines;

    // 시간대/인연/스토리에 맞는 대사 반환 (인연 > 스토리 반응 > 저녁/밤 > 기본 순 우선)
    const TArray<FString>& GetContextualLines(AActor* Player) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TObjectPtr<UAnimMontage> GreetingMontage;

    // ── 전투 ─────────────────────────────────────────────

    // false면 대화만 가능 (전투 선택지 비표시)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    bool bCanEnterCombat = true;

    // ── 상점 NPC ─────────────────────────────────────────
    // true면 상호작용(E) 시 대화 대신 상점을 바로 연다
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    bool bIsShopkeeper = false;

    // 열 상점 위젯 클래스 (BP에서 WBP_Shop 할당). 비면 일반 대화로 폴백.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    TSubclassOf<UShopWidget> ShopWidgetClass;

    // true면 밤(Night)엔 영업 종료(상점 안 열림)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    bool bClosedAtNight = false;

    // 상점 테마(convenience/gear/cafe/bar/flower/manhwa). 비면 종합 재고. OpenShop에 전달돼 차별 재고.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FName ShopKind;

    // ── 시간대 출현 스케줄 ────────────────────────────────
    // 이 NPC가 등장하는 시간대 목록. 비면 항상 등장(기존 동작). 설정 시 그 외 시간대엔 숨김+충돌끔.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
    TArray<EDayPhase> ActivePhases;

    // ── 스토리 상태 게이트(시간대 축과 곱해짐) ───────────
    // RequiredFlag 지정 시 그 플래그가 서 있어야 등장. ForbiddenFlag 지정 시 그 플래그가 서면 사라진다(사건 후 자리 비움 등).
    // 시간대(ActivePhases)와 AND로 곱해진다 — 둘 다 통과해야 보임. 둘 다 비면 시간대 규칙만 따름.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
    FName RequiredFlag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
    FName ForbiddenFlag;

    // ── 선물(소셜링크) ───────────────────────────────────
    // 이 NPC가 좋아하는 선물 아이템 Id(인벤토리 카탈로그). 비면 선물 안 받음.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
    FName FavoriteGiftId;

    // 선호 선물 1회 증정 시 오르는 호감도(대화 +3보다 큼)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
    int32 GiftAffinity = 8;

    // 선물 받았을 때 전용 반응 대사(비면 기본 "기뻐한다"). 인물 개성.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationship")
    FString GiftReactionLine;

    // ── 퀘스트 ───────────────────────────────────────────
    // 대화 시 시작할 퀘스트 Id (QuestComponent 카탈로그 Id). None이면 퀘스트 안 줌.
    // 이미 진행/완료한 퀘스트면 StartQuest가 무시함(중복 시작 방지).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName GrantsQuestId;

    // true면 페르소나 전투 시 플레이어 편(아군)으로 참전
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    bool bIsAlly = false;

    // ── 아군 영입 ────────────────────────────────────────
    // true면 대화로 영입 가능(영입 시 bIsAlly=true로 전환, 세이브 영구화)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    bool bCanBeRecruited = false;

    // 영입 영구화용 고유 키(세이브). 비면 영구화 안 함.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    FName RecruitId;

    // >0이면 영입에 필요한 최소 인연 랭크(옵트인). 0이면 게이트 없음(기존 동작 유지).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 RecruitMinBondRank = 0;

    // 영입 성공 순간 출력할 전용 대사(비면 기본 "X 영입!"만). 합류를 서사적 순간으로.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    FString RecruitLine;

    // 영입 시도: 가능하고 아직 아군 아니면 아군 전환 + 영구화. 성공 시 true.
    // Recruiter(플레이어)의 RelationshipComponent로 인연 게이트 평가(RecruitMinBondRank>0일 때).
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool TryRecruit(AActor* Recruiter = nullptr);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
    float NPCMaxHP = 80.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
    float NPCAttack = 12.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
    float NPCDefense = 5.f;

    // 실시간 모드 AI 공격 시작 (플레이어 CombatComponent에서 호출)
    void StartCombatAI(AABaseCharacter* Target, float Interval);
    void StopCombatAI();

private:
    FTimerHandle AIAttackTimer;
    TWeakObjectPtr<AABaseCharacter> AITarget;

    void AIAttack();

    UFUNCTION()
    void OnHPChanged_Handler(float CurrentHP, float MaxHP);

    // ── 시간대 출현 스케줄 ────────────────────────────────
    // 플레이어 TimeComponent 구독 + 현재 시간대 반영 (BeginPlay 다음 틱에 호출 — 플레이어 스폰 보장)
    void InitSchedule();
    void ApplyScheduleVisibility(EDayPhase Phase);

    UFUNCTION()
    void OnWorldTimeChanged(int32 Day, EDayPhase Phase);
};
