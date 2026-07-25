#include "CombatComponent.h"
#include "ABaseCharacter.h"
#include "ANPCCharacter.h"
#include "StatComponent.h"
#include "BoxingCombatStyle.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(EnemyTurnTimer);
        GetWorld()->GetTimerManager().ClearTimer(RealTimeEnemyTimer);
    }
}

// ── 전투 시작 ────────────────────────────────────────────

void UCombatComponent::StartCombat(ECombatMode Mode, ECombatStyle Style, AABaseCharacter* Target)
{
    if (!Target || !GetWorld()) return;

    CurrentMode = Mode;
    CombatTarget = Target;
    SetupStyle(Style);
    SetCharacterState(ECharacterState::Combat);
    ResetTurnState();

    OnCombatStarted.Broadcast(Mode, Style);

    if (Mode == ECombatMode::TurnBased)
    {
        StartPlayerTurn();
    }
    else if (Mode == ECombatMode::RealTime)
    {
        // 실시간 모드: NPC가 일정 간격으로 자동 공격
        if (AANPCCharacter* NPC = Cast<AANPCCharacter>(Target))
        {
            NPC->StartCombatAI(Cast<AABaseCharacter>(GetOwner()), 2.5f);
        }
    }
}

void UCombatComponent::SetupStyle(ECombatStyle Style)
{
    CurrentStyle = Style;
    if (Style == ECombatStyle::Boxing)
    {
        ActiveStyle = NewObject<UBoxingCombatStyle>(this);
    }
    // 추후 검투사 등 추가 시 여기서 분기
}

// ── 플레이어 턴 ──────────────────────────────────────────

void UCombatComponent::StartPlayerTurn()
{
    TurnState = ETurnState::PlayerTurn;
    bIsDefendingThisTurn = false;
    AccumulatedAttackDamage = 0.f;
    BasicAttackCount = 0;
    InputBuffer.Empty();
    OnPlayerTurnStarted.Broadcast();
}

// ── 공격 입력 처리 (U/I/J/K/L) ──────────────────────────

void UCombatComponent::OnAttackInput(EBoxingInput Input)
{
    // 턴제일 때는 PlayerTurn 중에만 허용
    if (CurrentMode == ECombatMode::TurnBased && TurnState != ETurnState::PlayerTurn)
        return;
    if (!IsInCombat() || !ActiveStyle) return;

    // 기본 공격 데미지 누적
    float BasicDmg = CalculateBasicDamage();
    AccumulatedAttackDamage += BasicDmg;
    BasicAttackCount++;

    // Owner 캐릭터에게 랜덤 기본 공격 몽타주 재생 요청
    if (AABaseCharacter* Owner = Cast<AABaseCharacter>(GetOwner()))
    {
        Owner->PlayRandomBasicAttackMontage();
    }

    // 실시간 모드: 기본 공격 즉시 데미지 적용
    if (CurrentMode == ECombatMode::RealTime)
    {
        if (AABaseCharacter* Target = CombatTarget.Get())
        {
            if (UStatComponent* TargetStat = Target->GetStatComponent())
            {
                float Dealt = TargetStat->ApplyDamage(BasicDmg);
                OnDamageEvent.Broadcast(Dealt, false);
                if (TargetStat->IsDead())
                {
                    OnCombatTargetDied.Broadcast();
                    EndCombat();
                    return;
                }
            }
        }
    }

    // 콤보 버퍼 업데이트
    InputBuffer.Add(Input);

    // 완성된 콤보 확인
    int32 MatchedCombo = ActiveStyle->FindMatchedCombo(InputBuffer);
    if (MatchedCombo >= 0)
    {
        FireComboSkill(MatchedCombo);
        return;
    }

    // 아직 유효한 콤보 접두어인지 확인
    if (ActiveStyle->IsValidComboPrefix(InputBuffer))
    {
        // 콤보 체인 유지 중 → 계속 입력 가능
        return;
    }

    // 콤보 체인 끊김: 이 입력부터 새 체인 시도
    InputBuffer.Empty();
    InputBuffer.Add(Input);
    if (!ActiveStyle->IsValidComboPrefix(InputBuffer))
    {
        InputBuffer.Empty(); // 새 체인도 안 됨
    }

    // 턴 종료 조건 확인 (최소 5번 이후에만)
    if (CurrentMode == ECombatMode::TurnBased && ShouldEndTurnAfterInput())
    {
        // 턴제이면서 최소 공격 횟수를 넘었고 진행 중인 콤보도 없음 → 턴 종료
        StartEnemyTurn();
    }
}

bool UCombatComponent::ShouldEndTurnAfterInput() const
{
    if (!ActiveStyle) return false;
    if (BasicAttackCount < ActiveStyle->MinAttacksPerTurn) return false;
    // 최소 횟수 이후: 진행 중인 콤보가 없으면 턴 종료
    return !ActiveStyle->IsValidComboPrefix(InputBuffer);
}

float UCombatComponent::CalculateBasicDamage() const
{
    AABaseCharacter* Owner = Cast<AABaseCharacter>(GetOwner());
    if (!Owner || !Owner->GetStatComponent()) return 1.f;
    return FMath::Max(1.f, Owner->GetStatComponent()->GetAttack());
}

// ── 콤보 완성 → 스킬 자동 발동 ─────────────────────────

void UCombatComponent::FireComboSkill(int32 ComboIndex)
{
    if (!ActiveStyle || !ActiveStyle->ComboDefs.IsValidIndex(ComboIndex)) return;

    const FComboDefinition& Combo = ActiveStyle->ComboDefs[ComboIndex];
    OnComboCompleted.Broadcast(Combo.SkillName);

    // 콤보 피니셔 몽타주 재생
    if (Combo.ComboFinisherMontage)
    {
        if (AABaseCharacter* Owner = Cast<AABaseCharacter>(GetOwner()))
        {
            Owner->PlayAnimMontage(Combo.ComboFinisherMontage);
        }
    }

    // 최종 데미지: 누적 기본 공격 + 스킬 배율 적용
    float TotalDamage = (AccumulatedAttackDamage + CalculateBasicDamage()) * Combo.SkillDamageMultiplier;

    if (AABaseCharacter* Target = CombatTarget.Get())
    {
        if (UStatComponent* TargetStat = Target->GetStatComponent())
        {
            float Dealt = TargetStat->ApplyDamage(TotalDamage);
            OnDamageEvent.Broadcast(Dealt, false);
            if (TargetStat->IsDead())
            {
                OnCombatTargetDied.Broadcast();
                EndCombat();
                return;
            }
        }
    }

    if (CurrentMode == ECombatMode::TurnBased)
    {
        StartEnemyTurn();
    }
    else
    {
        // 실시간: 콤보 완성 후 버퍼 리셋만
        InputBuffer.Empty();
        AccumulatedAttackDamage = 0.f;
        BasicAttackCount = 0;
    }
}

// ── 턴제 메뉴 선택 ──────────────────────────────────────

void UCombatComponent::UseMenuSkill(int32 SkillIndex)
{
    if (CurrentMode != ECombatMode::TurnBased || TurnState != ETurnState::PlayerTurn) return;
    if (!ActiveStyle || !ActiveStyle->MenuSkills.IsValidIndex(SkillIndex)) return;

    const FMenuSkillDefinition& Skill = ActiveStyle->MenuSkills[SkillIndex];

    // 메뉴 스킬: 기본 공격 없이 스킬 데미지만
    AABaseCharacter* Owner = Cast<AABaseCharacter>(GetOwner());
    float AttackVal = Owner ? Owner->GetStatComponent()->GetAttack() : 15.f;
    float TotalDamage = AttackVal * Skill.DamageMultiplier;

    if (Skill.SkillMontage && Owner)
        Owner->PlayAnimMontage(Skill.SkillMontage);

    if (AABaseCharacter* Target = CombatTarget.Get())
    {
        if (UStatComponent* TargetStat = Target->GetStatComponent())
        {
            float Dealt = TargetStat->ApplyDamage(TotalDamage);
            OnDamageEvent.Broadcast(Dealt, false);
            if (TargetStat->IsDead())
            {
                OnCombatTargetDied.Broadcast();
                EndCombat();
                return;
            }
        }
    }

    StartEnemyTurn();
}

void UCombatComponent::UseDefend()
{
    if (CurrentMode != ECombatMode::TurnBased || TurnState != ETurnState::PlayerTurn) return;
    bIsDefendingThisTurn = true;
    StartEnemyTurn();
}

void UCombatComponent::UseFlee()
{
    if (CurrentMode != ECombatMode::TurnBased || TurnState != ETurnState::PlayerTurn) return;
    EndCombat();
}

// ── 적 턴 ────────────────────────────────────────────────

void UCombatComponent::StartEnemyTurn()
{
    TurnState = ETurnState::EnemyTurn;
    OnEnemyTurnStarted.Broadcast();

    // 1.5초 후 적 공격 실행
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            EnemyTurnTimer,
            this, &UCombatComponent::ExecuteEnemyAttack,
            1.5f, false);
    }
}

void UCombatComponent::ExecuteEnemyAttack()
{
    AABaseCharacter* Target = Cast<AABaseCharacter>(GetOwner()); // 플레이어
    AABaseCharacter* Attacker = CombatTarget.Get();              // NPC
    if (!Target || !Attacker) return;

    // NPC 랜덤 공격 몽타주
    if (AANPCCharacter* NPC = Cast<AANPCCharacter>(Attacker))
    {
        NPC->PlayRandomBasicAttackMontage();
    }

    // 플레이어에게 데미지
    if (UStatComponent* PlayerStat = Target->GetStatComponent())
    {
        float NpcAttack = 0.f;
        if (UStatComponent* NpcStat = Attacker->GetStatComponent())
            NpcAttack = NpcStat->GetAttack();

        // 방어 중이면 데미지 50% 감소
        float IncomingDmg = bIsDefendingThisTurn ? NpcAttack * 0.5f : NpcAttack;
        float Dealt = PlayerStat->ApplyDamage(IncomingDmg);
        OnDamageEvent.Broadcast(Dealt, true);

        if (PlayerStat->IsDead())
        {
            OnPlayerDied.Broadcast();
            EndCombat();
            return;
        }
    }

    // 다시 플레이어 턴
    StartPlayerTurn();
}

void UCombatComponent::ExecuteRealTimeEnemyAttack()
{
    AABaseCharacter* Player = Cast<AABaseCharacter>(GetOwner());
    AABaseCharacter* Attacker = CombatTarget.Get();
    if (!Player || !Attacker || !IsInCombat()) return;

    if (AANPCCharacter* NPC = Cast<AANPCCharacter>(Attacker))
    {
        NPC->PlayRandomBasicAttackMontage();
    }

    if (UStatComponent* PlayerStat = Player->GetStatComponent())
    {
        float NpcAttack = 0.f;
        if (UStatComponent* NpcStat = Attacker->GetStatComponent())
            NpcAttack = NpcStat->GetAttack();

        float Dealt = PlayerStat->ApplyDamage(NpcAttack);
        OnDamageEvent.Broadcast(Dealt, true);

        if (PlayerStat->IsDead())
        {
            OnPlayerDied.Broadcast();
            EndCombat();
        }
    }
}

// ── 전투 종료 ────────────────────────────────────────────

void UCombatComponent::EndCombat()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(EnemyTurnTimer);
        GetWorld()->GetTimerManager().ClearTimer(RealTimeEnemyTimer);
    }

    // 실시간 모드였다면 NPC AI도 중지
    if (CurrentMode == ECombatMode::RealTime)
    {
        if (AANPCCharacter* NPC = Cast<AANPCCharacter>(CombatTarget.Get()))
        {
            NPC->StopCombatAI();
        }
    }

    CurrentMode = ECombatMode::None;
    CurrentStyle = ECombatStyle::None;
    TurnState = ETurnState::Idle;
    CombatTarget.Reset();
    InputBuffer.Empty();
    AccumulatedAttackDamage = 0.f;
    BasicAttackCount = 0;
    bIsDefendingThisTurn = false;

    SetCharacterState(ECharacterState::Default);
    OnCombatEnded.Broadcast();
}

void UCombatComponent::ResetTurnState()
{
    InputBuffer.Empty();
    AccumulatedAttackDamage = 0.f;
    BasicAttackCount = 0;
    bIsDefendingThisTurn = false;
    TurnState = ETurnState::Idle;
}

// ── 기존 호환 ────────────────────────────────────────────

void UCombatComponent::SetCharacterState(ECharacterState NewState)
{
    if (CurrentState == NewState) return;
    CurrentState = NewState;

    if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
    {
        OwnerChar->GetCharacterMovement()->MaxWalkSpeed =
            (CurrentState == ECharacterState::Combat) ? 400.f : 600.f;
    }
}
