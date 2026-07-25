#include "StatComponent.h"

UStatComponent::UStatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStatComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentHP = MaxHP;
    CurrentSP = MaxSP;
    AttackMod = 1.f; AttackModTurns = 0;
    DefenseMod = 1.f; DefenseModTurns = 0;
    bDefending = false;
    bCharged = false;
    BattleScale = 1.f;
    Ailment = EAilment::None; AilmentTurns = 0;
    bEnduredThisBattle = false; bJustEndured = false;
    bIsDead = false;
}

float UStatComponent::ApplyDamage(float RawDamage)
{
    if (bIsDead) return 0.f;

    // GetDefense()는 방어 디버프/버프 반영된 실효 방어력
    float ActualDamage = FMath::Max(1.f, RawDamage - GetDefense());
    CurrentHP = FMath::Max(0.f, CurrentHP - ActualDamage);
    OnHPChanged.Broadcast(CurrentHP, MaxHP);

    // 피격 시 수면 해제
    if (Ailment == EAilment::Sleep)
    {
        Ailment = EAilment::None;
        AilmentTurns = 0;
    }

    if (CurrentHP <= 0.f)
    {
        // 근성(Endure): 전투당 1회, 치명적 데미지를 HP 1로 버팀 (직접 피격 한정 — 독/화상 도트는 제외)
        if (bEndureOnce && !bEnduredThisBattle)
        {
            bEnduredThisBattle = true;
            bJustEndured = true;
            CurrentHP = 1.f;
            OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
        }
        else
        {
            bIsDead = true;
            OnDeath.Broadcast();
        }
    }
    return ActualDamage;
}

void UStatComponent::Heal(float Amount)
{
    if (bIsDead) return;
    CurrentHP = FMath::Min(GetMaxHP(), CurrentHP + Amount);
    OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
}

bool UStatComponent::SpendSP(float Amount)
{
    if (CurrentSP < Amount) return false;
    CurrentSP = FMath::Max(0.f, CurrentSP - Amount);
    return true;
}

int32 UStatComponent::GainXP(int32 Amount)
{
    CurrentXP += Amount;

    const int32 MaxLevel = 100;   // 레벨 상한
    int32 LevelsGained = 0;
    while (Level < MaxLevel && CurrentXP >= GetXPToNext())
    {
        CurrentXP -= GetXPToNext();
        Level++;
        LevelsGained++;
        // 레벨업 스탯 성장 (튜닝 가능)
        MaxHP += GrowMaxHP;
        Attack += GrowAttack;
        Defense += GrowDefense;
        MaxSP += GrowMaxSP;
        // 세부 스탯 성장 (시드된 경우만 의미 — 미설정이면 0 성장)
        STR += GrowSTR; MAG += GrowMAG; VIT += GrowVIT; AGI += GrowAGI; LUK += GrowLUK;
    }
    if (Level >= MaxLevel) CurrentXP = 0;   // 만렙: 잉여 XP 버림(게이지 0)

    if (LevelsGained > 0)
    {
        // 레벨업 시 풀 회복 (장비 보너스 포함)
        CurrentHP = GetMaxHP();
        CurrentSP = GetMaxSP();
        OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
    }
    return LevelsGained;
}

void UStatComponent::GetProgression(int32& OutLevel, int32& OutXP, float& OutMaxHP, float& OutAttack, float& OutDefense, float& OutMaxSP) const
{
    OutLevel = Level;
    OutXP = CurrentXP;
    OutMaxHP = MaxHP;
    OutAttack = Attack;
    OutDefense = Defense;
    OutMaxSP = MaxSP;
}

void UStatComponent::SetProgression(int32 InLevel, int32 InXP, float InMaxHP, float InAttack, float InDefense, float InMaxSP)
{
    Level = InLevel;
    CurrentXP = InXP;
    MaxHP = InMaxHP;
    Attack = InAttack;
    Defense = InDefense;
    MaxSP = InMaxSP;
    CurrentHP = MaxHP;
    CurrentSP = MaxSP;
}

void UStatComponent::ApplyAttackMod(float Mult, int32 Turns)
{
    AttackMod = Mult;
    AttackModTurns = Turns;
}

void UStatComponent::ApplyDefenseMod(float Mult, int32 Turns)
{
    DefenseMod = Mult;
    DefenseModTurns = Turns;
}

void UStatComponent::TickStatMods()
{
    if (AttackModTurns > 0)
    {
        AttackModTurns--;
        if (AttackModTurns == 0) AttackMod = 1.f;
    }
    if (DefenseModTurns > 0)
    {
        DefenseModTurns--;
        if (DefenseModTurns == 0) DefenseMod = 1.f;
    }
}

void UStatComponent::ApplyAilment(EAilment NewAilment, int32 Turns)
{
    if (bIsDead || NewAilment == EAilment::None) return;
    Ailment = NewAilment;
    AilmentTurns = Turns;
}

float UStatComponent::OnTurnStartAilment()
{
    if (AilmentTurns <= 0) { Ailment = EAilment::None; return 0.f; }

    // 도트 데미지: 독 = MaxHP 8% / 화상 = MaxHP 12% (최소 1)
    float DotPct = 0.f;
    if (Ailment == EAilment::Poison) DotPct = 0.08f;
    else if (Ailment == EAilment::Burn) DotPct = 0.12f;

    float DotDealt = 0.f;
    if (DotPct > 0.f && !bIsDead)
    {
        DotDealt = FMath::Max(1.f, GetMaxHP() * DotPct);
        CurrentHP = FMath::Max(0.f, CurrentHP - DotDealt);
        OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
        if (CurrentHP <= 0.f)
        {
            bIsDead = true;
            OnDeath.Broadcast();
        }
    }

    AilmentTurns--;
    if (AilmentTurns <= 0) Ailment = EAilment::None;
    return DotDealt;
}

bool UStatComponent::IsIncapacitated() const
{
    if (AilmentTurns <= 0) return false;
    // 수면/빙결 = 항상 행동불가 (빙결은 피격에도 안 풀림)
    if (Ailment == EAilment::Sleep || Ailment == EAilment::Freeze) return true;
    // 마비/감전 = 50% 확률 행동불가
    if (Ailment == EAilment::Paralysis || Ailment == EAilment::Shock) return FMath::FRand() < 0.5f;
    return false;
}

void UStatComponent::InitStats(float InMaxHP, float InAttack, float InDefense, float InMaxSP)
{
    MaxHP = InMaxHP;
    CurrentHP = InMaxHP;
    Attack = InAttack;
    Defense = InDefense;
    MaxSP = InMaxSP;
    CurrentSP = InMaxSP;
    AttackMod = 1.f; AttackModTurns = 0;
    DefenseMod = 1.f; DefenseModTurns = 0;
    bDefending = false;
    bCharged = false;
    BattleScale = 1.f;
    Ailment = EAilment::None; AilmentTurns = 0;
    bEnduredThisBattle = false; bJustEndured = false;
    bIsDead = false;
}

void UStatComponent::ResetHP()
{
    CurrentHP = GetMaxHP();
    CurrentSP = GetMaxSP();
    AttackMod = 1.f; AttackModTurns = 0;
    DefenseMod = 1.f; DefenseModTurns = 0;
    bDefending = false;
    bCharged = false;
    BattleScale = 1.f;
    Ailment = EAilment::None; AilmentTurns = 0;
    bEnduredThisBattle = false; bJustEndured = false;
    bIsDead = false;
    OnHPChanged.Broadcast(CurrentHP, MaxHP);
}
