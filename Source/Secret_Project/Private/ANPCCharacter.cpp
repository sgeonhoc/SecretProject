#include "ANPCCharacter.h"
#include "ABaseCharacter.h"
#include "StatComponent.h"
#include "SecretSaveGame.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "TimeComponent.h"
#include "RelationshipComponent.h"
#include "NPCArchetype.h"
#include "StoryManager.h" // 스토리 플래그 게이트
#include "CombatArchetype.h"
#include "StoryManager.h"            // 스토리 플래그 조회(읽기 전용 — A 스파인 비침범)
#include "Engine/GameInstance.h"

AANPCCharacter::AANPCCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AANPCCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 양산 아키타입: ArchetypeId 설정 시 소셜 프로필(이름/대사/선물/스케줄/상점/퀘스트/영입) 자동 적용.
    // 아래 RecruitId 복원·NPCName 기반 로드·스케줄 init이 이 값들을 읽으므로 반드시 맨 앞에서 적용.
    UNPCArchetypeLibrary::ApplySocialProfile(this);

    // [A 전투 프로필 훅] 같은 ArchetypeId로 스탯/스킬/약점/패시브/보상 자동 적용 (InitStats 전 — NPCMaxHP/Attack/Defense를 채워둠).
    UCombatArchetypeLibrary::ApplyCombatProfile(this);

    // 이전에 영입한 NPC면 아군 상태 복원 (아래 아군 진행 로드보다 먼저)
    if (!RecruitId.IsNone() && !bIsAlly && USecretSaveGame::IsCollected(RecruitId))
        bIsAlly = true;

    // NPC 전용 스탯 적용
    if (StatComponent)
    {
        StatComponent->InitStats(NPCMaxHP, NPCAttack, NPCDefense);

        // 피격 시 랜덤 피격 몽타주 재생
        StatComponent->OnHPChanged.AddDynamic(this, &AANPCCharacter::OnHPChanged_Handler);

        // 아군이면 세이브에서 진행 복원 (NPCName 기준)
        if (bIsAlly && UGameplayStatics::DoesSaveGameExist(TEXT("PlayerSave"), 0))
        {
            if (USecretSaveGame* Save = Cast<USecretSaveGame>(
                UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0)))
            {
                for (const FAllyProgress& AP : Save->Allies)
                {
                    if (AP.Name == NPCName)
                    {
                        StatComponent->SetProgression(AP.Level, AP.XP, AP.MaxHP, AP.Attack, AP.Defense, AP.MaxSP);
                        break;
                    }
                }
            }
        }
    }

    // 시간대 출현 스케줄: 플레이어 스폰 순서 보장 위해 다음 틱에 구독+반영 (ActivePhases 비면 스킵=항상 등장)
    if (ActivePhases.Num() > 0 && GetWorld())
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AANPCCharacter::InitSchedule);
}

void AANPCCharacter::InitSchedule()
{
    if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        if (UTimeComponent* Time = P->FindComponentByClass<UTimeComponent>())
        {
            // 중복 구독 방어 후 구독
            Time->OnTimeChanged.RemoveDynamic(this, &AANPCCharacter::OnWorldTimeChanged);
            Time->OnTimeChanged.AddDynamic(this, &AANPCCharacter::OnWorldTimeChanged);
            ApplyScheduleVisibility(Time->GetPhase());
            return;
        }
    }
    // 플레이어 TimeComponent 못 찾으면 시간대 스케줄은 미적용(항상 등장 유지).
    // 단 스토리 플래그 게이트는 시간과 무관하므로 여기서도 적용한다.
    if (!UStoryManagerSubsystem::PassesFlagGate(this, RequiredFlag, ForbiddenFlag))
    {
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
    }
}

void AANPCCharacter::OnWorldTimeChanged(int32 Day, EDayPhase Phase)
{
    ApplyScheduleVisibility(Phase);
}

void AANPCCharacter::ApplyScheduleVisibility(EDayPhase Phase)
{
    // 시간대 축: ActivePhases 비면 통과(항상), 아니면 현재 시간대 포함해야 통과.
    const bool bPhaseOK = (ActivePhases.Num() == 0) || ActivePhases.Contains(Phase);
    // 스토리 상태 축: 플래그 게이트.
    const bool bFlagOK = UStoryManagerSubsystem::PassesFlagGate(this, RequiredFlag, ForbiddenFlag);
    // 시간대 규칙도, 플래그도 없으면 기존 동작(항상 등장)과 동일.
    if (ActivePhases.Num() == 0 && RequiredFlag.IsNone() && ForbiddenFlag.IsNone()) return;

    const bool bActive = bPhaseOK && bFlagOK;
    SetActorHiddenInGame(!bActive);
    SetActorEnableCollision(bActive);
}

const TArray<FString>& AANPCCharacter::GetContextualLines(AActor* Player) const
{
    // 스토리 서브시스템 1회 조회(균열·스토리반응 공용, 읽기 전용 — A 스파인 비침범)
    UStoryManagerSubsystem* Story = nullptr;
    if (UWorld* W = GetWorld())
        if (UGameInstance* GI = W->GetGameInstance())
            Story = GI->GetSubsystem<UStoryManagerSubsystem>();

    // 0순위(최우선): 일상 균열 — 미스터리 단서를 쥔 채 아직 사건 해결 전. NPC가 잠깐 '저쪽'에 물든다.
    if (CreepyLines.Num() > 0 && Story && !Story->HasFlag(TEXT("WarlockDefeated")))
    {
        if (Story->HasFlag(TEXT("Clue_Voice")) || Story->HasFlag(TEXT("Clue_Throne")) || Story->HasFlag(TEXT("Clue_Truth")))
            return CreepyLines;
    }

    // 1순위: 인연이 깊으면(BondDialogueLines 있고 랭크 충족) 특별 대사
    if (BondDialogueLines.Num() > 0 && Player)
    {
        if (URelationshipComponent* Rel = Player->FindComponentByClass<URelationshipComponent>())
        {
            if (Rel->GetRank(FName(*NPCName)) >= BondLineMinRank)
                return BondDialogueLines;
        }
    }

    // 2순위: 스토리 반응 대사 — 월드가 사건에 살아있게. 가장 최근(최우선) 단계의 대사를 고름.
    // 우선순위: 평화 회복 > 흑마술사 격파 > 흑마술사 소문 > 각성/공허현상. 플레이어가 가진 가장 진행된 플래그가 이김.
    if (StoryReactiveLines.Num() > 0)
    {
        if (Story)
        {
            // 후반 사건일수록 앞에 — 플레이어가 도달한 가장 진행된 단계의 대사로 거리 분위기가 바뀐다.
            static const FName PhasePriority[] = {
                TEXT("StoryClear"), TEXT("TyrantDefeated"), TEXT("WarlockDefeated"),
                TEXT("WarlockLocated"), TEXT("WarlockRumor"), TEXT("TeamFormed"),
                TEXT("Awakened"), TEXT("Prologue")
            };
            for (const FName& Flag : PhasePriority)
            {
                if (!Story->HasFlag(Flag)) continue;
                for (const FStoryReactiveLines& SR : StoryReactiveLines)
                    if (SR.RequiredFlag == Flag && SR.Lines.Num() > 0)
                        return SR.Lines;
            }
        }
    }

    // 3순위: 저녁/밤 전용 대사
    if (EveningDialogueLines.Num() > 0 && Player)
    {
        if (UTimeComponent* Time = Player->FindComponentByClass<UTimeComponent>())
        {
            const EDayPhase Ph = Time->GetPhase();
            if (Ph == EDayPhase::Evening || Ph == EDayPhase::Night)
                return EveningDialogueLines;
        }
    }

    // 4순위: 기본 대사
    return DialogueLines;
}

bool AANPCCharacter::TryRecruit(AActor* Recruiter)
{
    if (!bCanBeRecruited || bIsAlly) return false;

    // 인연 게이트(옵트인): RecruitMinBondRank>0이면 그 랭크 이상이어야 영입 가능
    if (RecruitMinBondRank > 0)
    {
        int32 Rank = 0;
        if (Recruiter)
            if (URelationshipComponent* Rel = Recruiter->FindComponentByClass<URelationshipComponent>())
                Rank = Rel->GetRank(FName(*NPCName));

        if (Rank < RecruitMinBondRank)
        {
            if (GEngine)
                GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
                    FString::Printf(TEXT("%s: 아직 너를 더 알아가야겠어 (인연 랭크 %d 필요)"),
                        *NPCName, RecruitMinBondRank));
            return false;
        }
    }

    bIsAlly = true;
    if (!RecruitId.IsNone())
        USecretSaveGame::MarkCollected(RecruitId);

    if (GEngine)
    {
        // 영입 전용 대사(있으면) 먼저 — 합류를 서사적 순간으로
        if (!RecruitLine.IsEmpty())
            GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan,
                FString::Printf(TEXT("%s: %s"), *NPCName, *RecruitLine));
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
            FString::Printf(TEXT("%s 영입! 동료가 되었다."), *NPCName));
    }
    return true;
}

// ── 실시간 AI ────────────────────────────────────────────

void AANPCCharacter::StartCombatAI(AABaseCharacter* Target, float Interval)
{
    AITarget = Target;
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            AIAttackTimer,
            this, &AANPCCharacter::AIAttack,
            Interval, true, Interval); // 첫 공격도 딜레이 후 시작
    }
}

void AANPCCharacter::StopCombatAI()
{
    AITarget.Reset();
    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(AIAttackTimer);
}

void AANPCCharacter::OnHPChanged_Handler(float CurrentHP, float MaxHP)
{
    PlayRandomHitReactionMontage();
}

void AANPCCharacter::AIAttack()
{
    AABaseCharacter* Target = AITarget.Get();
    if (!Target || !StatComponent || StatComponent->IsDead()) return;

    // 랜덤 기본 공격 몽타주 재생
    PlayRandomBasicAttackMontage();

    // 플레이어에게 데미지 적용
    if (UStatComponent* TargetStat = Target->GetStatComponent())
    {
        TargetStat->ApplyDamage(NPCAttack);

        // 피격 시 플레이어도 피격 몽타주
        Target->PlayRandomHitReactionMontage();
    }
}
