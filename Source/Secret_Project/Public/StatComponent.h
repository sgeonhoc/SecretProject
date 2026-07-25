#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BattleTypes.h"
#include "StatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, float, CurrentHP, float, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UStatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStatComponent();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float MaxHP = 100.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    float CurrentHP = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float Attack = 15.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float Defense = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float MaxSP = 50.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    float CurrentSP = 50.f;

    // ── RPG 세부 스탯 (5속성). 0 = 미설정 → 기존 Attack/Defense 폴백(하위호환). 시드되면 적용. ──
    //  힘 STR=물리 데미지 / 마력 MAG=마법 데미지 / 체력 VIT=방어+HP / 민첩 AGI=명중·회피·속도 / 운 LUK=치명타·상태이상·드롭
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Attributes") float STR = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Attributes") float MAG = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Attributes") float VIT = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Attributes") float AGI = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Attributes") float LUK = 0.f;
    // 레벨업 시 속성 성장량
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Attributes") float GrowSTR = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Attributes") float GrowMAG = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Attributes") float GrowVIT = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Attributes") float GrowAGI = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Attributes") float GrowLUK = 0.f;

    // 근성(Endure): 켜면 전투당 1회, 치명적 데미지를 받아도 죽지 않고 HP 1로 버팀 (BP 캐릭터별 옵트인)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    bool bEndureOnce = false;

    // ── 레벨/경험치 ──
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Level")
    int32 Level = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Level")
    int32 CurrentXP = 0;

    // 이 캐릭터를 쓰러뜨렸을 때 주는 경험치 (적에 설정)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Level")
    int32 XPReward = 30;

    // 골드(재화): 적이 주는 양 GoldReward, 보유량 Gold(플레이어가 누적)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Level")
    int32 GoldReward = 20;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Level")
    int32 Gold = 0;

    // 레벨업 성장 / 경험치 곡선 (밸런싱 — BP에서 캐릭터별 조절 가능)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Level")
    int32 XPPerLevel = 100;     // 다음 레벨 필요 XP = Level * XPPerLevel
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Level")
    float GrowMaxHP = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Level")
    float GrowAttack = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Level")
    float GrowDefense = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Level")
    float GrowMaxSP = 5.f;

    // 버프/디버프: 배율 + 남은 턴 (1.0 / 0 = 효과 없음)
    float AttackMod = 1.f;
    int32 AttackModTurns = 0;
    float DefenseMod = 1.f;
    int32 DefenseModTurns = 0;

    bool bDefending = false; // 이번 적 페이즈 동안 방어 중 (캐릭터별)

    bool bCharged = false;   // 차지: 다음 데미지 행동을 강화하고 소모됨

    // 근성(Endure) 런타임 상태
    bool bEnduredThisBattle = false; // 이번 전투에 이미 발동했는가
    bool bJustEndured = false;       // 직전 ApplyDamage에서 버팀 발동(피드백 표시 후 소비)

    float BattleScale = 1.f; // 난이도 스케일링: 공격/방어 배율 (전투 시작 시 설정, 전투마다 리셋)

    // 장비 보너스 (EquipmentComponent가 장착 시 합산해 설정). 평상시 0.
    float EquipAtk = 0.f;
    float EquipDef = 0.f;
    float EquipHP = 0.f;
    float EquipSP = 0.f;

    // 상태이상
    EAilment Ailment = EAilment::None;
    int32 AilmentTurns = 0;

    bool bIsDead = false;

public:
    UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
    FOnHPChanged OnHPChanged;

    UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
    FOnDeath OnDeath;

    // Defense를 뺀 후 최소 1 보장. 실제 들어간 데미지 반환
    UFUNCTION(BlueprintCallable, Category = "Stats")
    float ApplyDamage(float RawDamage);

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void Heal(float Amount);

    // SP 소모 — 부족하면 false 반환(소모 안 함)
    UFUNCTION(BlueprintCallable, Category = "Stats")
    bool SpendSP(float Amount);

    // 경험치 획득 → 레벨업 처리(스탯 성장 + 풀 회복). 오른 레벨 수 반환
    UFUNCTION(BlueprintCallable, Category = "Stats|Level")
    int32 GainXP(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Stats|Level")
    FORCEINLINE int32 GetLevel() const { return Level; }
    UFUNCTION(BlueprintPure, Category = "Stats|Level")
    FORCEINLINE int32 GetCurrentXP() const { return CurrentXP; }
    UFUNCTION(BlueprintPure, Category = "Stats|Level")
    FORCEINLINE int32 GetXPToNext() const { return Level * XPPerLevel; }
    UFUNCTION(BlueprintPure, Category = "Stats|Level")
    FORCEINLINE int32 GetXPReward() const { return XPReward; }
    UFUNCTION(BlueprintPure, Category = "Stats|Level")
    FORCEINLINE int32 GetGold() const { return Gold; }
    UFUNCTION(BlueprintPure, Category = "Stats|Level")
    FORCEINLINE int32 GetGoldReward() const { return GoldReward; }
    // 처치 보상 설정 (전투 프로필 양산용 — 적 아키타입의 XP/골드 보상)
    UFUNCTION(BlueprintCallable, Category = "Stats|Level")
    void SetXPReward(int32 Value) { XPReward = FMath::Max(0, Value); }
    UFUNCTION(BlueprintCallable, Category = "Stats|Level")
    void SetGoldReward(int32 Value) { GoldReward = FMath::Max(0, Value); }

    // 레벨업 성장률 설정 (전투 프로필 양산용 — 역할별 차등 성장 밸런싱). 음수는 0으로 클램프.
    UFUNCTION(BlueprintCallable, Category = "Stats|Level")
    void SetGrowthRates(float HP, float Atk, float Def, float SP)
    {
        GrowMaxHP   = FMath::Max(0.f, HP);
        GrowAttack  = FMath::Max(0.f, Atk);
        GrowDefense = FMath::Max(0.f, Def);
        GrowMaxSP   = FMath::Max(0.f, SP);
    }
    UFUNCTION(BlueprintCallable, Category = "Stats|Level")
    void AddGold(int32 Amount) { Gold += Amount; }
    UFUNCTION(BlueprintCallable, Category = "Stats|Level")
    void SetGold(int32 Amount) { Gold = Amount; }
    // 골드 소모 — 부족/음수면 false(소모 안 함)
    UFUNCTION(BlueprintCallable, Category = "Stats|Level")
    bool SpendGold(int32 Amount) { if (Amount < 0 || Amount > Gold) return false; Gold -= Amount; return true; }

    // ── 상점 영구 강화 (구매 시 호출). HP/SP 강화는 현재치도 새 최대로 채움 ──
    UFUNCTION(BlueprintCallable, Category = "Stats|Shop")
    void BoostMaxHP(float Amount) { MaxHP += Amount; CurrentHP = GetMaxHP(); }
    UFUNCTION(BlueprintCallable, Category = "Stats|Shop")
    void BoostAttack(float Amount) { Attack += Amount; }
    UFUNCTION(BlueprintCallable, Category = "Stats|Shop")
    void BoostDefense(float Amount) { Defense += Amount; }
    UFUNCTION(BlueprintCallable, Category = "Stats|Shop")
    void BoostMaxSP(float Amount) { MaxSP += Amount; CurrentSP = GetMaxSP(); }
    // HP/SP 완전 회복 (상점 회복 서비스)
    UFUNCTION(BlueprintCallable, Category = "Stats|Shop")
    void FullRestore() { CurrentHP = GetMaxHP(); CurrentSP = GetMaxSP(); }

    // SP 부분 회복 (소비아이템/SP회복 스킬용). MaxSP 초과/음수 클램프
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void RestoreSP(float Amount) { CurrentSP = FMath::Clamp(CurrentSP + Amount, 0.f, GetMaxSP()); }

    // 세이브/로드용 — 성장 상태 입출력 (C++ 전용)
    void GetProgression(int32& OutLevel, int32& OutXP, float& OutMaxHP, float& OutAttack, float& OutDefense, float& OutMaxSP) const;
    void SetProgression(int32 InLevel, int32 InXP, float InMaxHP, float InAttack, float InDefense, float InMaxSP);

    // 버프/디버프 적용 (Mult>1 버프, <1 디버프) + 지속 턴
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void ApplyAttackMod(float Mult, int32 Turns);

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void ApplyDefenseMod(float Mult, int32 Turns);

    // 매 턴 시작 시 호출 — 지속 턴 감소, 만료 시 해제
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void TickStatMods();

    // 방어 상태 (캐릭터별)
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void SetDefending(bool bValue) { bDefending = bValue; }
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE bool IsDefending() const { return bDefending; }

    // 차지 (다음 데미지 행동 강화 → 소모형). 공격/공격스킬만 소모, 힐/버프는 유지
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void SetCharged(bool bValue) { bCharged = bValue; }
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE bool IsCharged() const { return bCharged; }

    // ── 근성(Endure) ──
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE bool HasEndure() const { return bEndureOnce; }

    // 근성 옵트인 설정 (전투 프로필 양산용 — CombatArchetype이 ArchetypeId 적용 시 호출)
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void SetEndureOnce(bool bValue) { bEndureOnce = bValue; }

    // 직전 ApplyDamage에서 근성이 발동했는가 (읽으면 소비) — 전투 피드백/연출용
    UFUNCTION(BlueprintCallable, Category = "Stats")
    bool ConsumeJustEndured() { const bool b = bJustEndured; bJustEndured = false; return b; }

    // 전투 시작 시 호출 — 전투 한정 1회성 상태(근성) 리셋
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void ResetBattleOnce() { bEnduredThisBattle = false; bJustEndured = false; }

    // ── 상태이상 ──
    UFUNCTION(BlueprintCallable, Category = "Stats|Ailment")
    void ApplyAilment(EAilment NewAilment, int32 Turns);

    // 턴 시작 시: 독 도트뎀 + 상태이상 지속 감소. 입힌 도트 데미지 반환(없으면 0 — 전투 피드백용)
    UFUNCTION(BlueprintCallable, Category = "Stats|Ailment")
    float OnTurnStartAilment();

    // 이번 턴 행동 불가인가 (수면=항상, 마비=50%)
    UFUNCTION(BlueprintCallable, Category = "Stats|Ailment")
    bool IsIncapacitated() const;

    UFUNCTION(BlueprintPure, Category = "Stats|Ailment")
    FORCEINLINE EAilment GetAilment() const { return AilmentTurns > 0 ? Ailment : EAilment::None; }

    // 상태이상 치료 (소비아이템/치료 스킬용)
    UFUNCTION(BlueprintCallable, Category = "Stats|Ailment")
    void CureAilment() { Ailment = EAilment::None; AilmentTurns = 0; }

    // 전투 시작 시 스탯 일괄 초기화
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void InitStats(float InMaxHP, float InAttack, float InDefense, float InMaxSP = 50.f);

    UFUNCTION(BlueprintCallable, Category = "Stats")
    void ResetHP();

    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE float GetCurrentHP() const { return CurrentHP; }
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE float GetMaxHP() const { return MaxHP + EquipHP; }
    // 버프/디버프 + 난이도 스케일 + 장비 반영된 실효 스탯
    // 물리 공격력 = STR(미설정 0이면 기존 Attack 폴백). GetAttack은 물리 데미지 원천.
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE float GetAttack() const { return ((STR > 0.f ? STR : Attack) + EquipAtk) * AttackMod * BattleScale; }
    // 마법 공격력 = MAG(미설정이면 Attack 폴백). 비물리 스킬 데미지 원천.
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE float GetMagPower() const { return ((MAG > 0.f ? MAG : Attack) + EquipAtk) * AttackMod * BattleScale; }
    // 방어력 = Defense + 체력(VIT) 보정.
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE float GetDefense() const { return (Defense + EquipDef + VIT * 0.4f) * DefenseMod * BattleScale; }

    // ── 세부 스탯 접근 ──
    UFUNCTION(BlueprintPure, Category = "Stats|Attributes") FORCEINLINE float GetSTR() const { return STR; }
    UFUNCTION(BlueprintPure, Category = "Stats|Attributes") FORCEINLINE float GetMAG() const { return MAG; }
    UFUNCTION(BlueprintPure, Category = "Stats|Attributes") FORCEINLINE float GetVIT() const { return VIT; }
    UFUNCTION(BlueprintPure, Category = "Stats|Attributes") FORCEINLINE float GetAGI() const { return AGI; }
    UFUNCTION(BlueprintPure, Category = "Stats|Attributes") FORCEINLINE float GetLUK() const { return LUK; }
    // 운(LUK) 1당 치명타 +0.5%p (BattleManager가 기본 CritChance에 가산)
    UFUNCTION(BlueprintPure, Category = "Stats|Attributes") FORCEINLINE float GetCritBonus() const { return LUK * 0.005f; }

    // 세부 스탯 일괄 설정 (CombatArchetype 적용 시 AttributeLibrary가 호출)
    UFUNCTION(BlueprintCallable, Category = "Stats|Attributes")
    void SetAttributes(float S, float M, float V, float A, float L) { STR = FMath::Max(0.f, S); MAG = FMath::Max(0.f, M); VIT = FMath::Max(0.f, V); AGI = FMath::Max(0.f, A); LUK = FMath::Max(0.f, L); }
    UFUNCTION(BlueprintCallable, Category = "Stats|Attributes")
    void SetAttributeGrowth(float S, float M, float V, float A, float L) { GrowSTR = FMath::Max(0.f, S); GrowMAG = FMath::Max(0.f, M); GrowVIT = FMath::Max(0.f, V); GrowAGI = FMath::Max(0.f, A); GrowLUK = FMath::Max(0.f, L); }

    // 난이도 스케일링 배율 설정 (BattleManager가 전투 시작 시 적에 적용)
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void SetBattleScale(float Scale) { BattleScale = FMath::Max(0.1f, Scale); }

    // 영속 누적 곱(보스 페이즈 강화 등) — 기존 스케일에 곱(AttackMod와 독립적, 공/방/마법 일괄).
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void MultiplyBattleScale(float M) { SetBattleScale(BattleScale * M); }

    // 레이드 스케일: MaxHP에 HPMult 곱(체력 스펀지), 공/방에 PowerMult(BattleScale). 전투 시작 시 1회 적용.
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void ApplyRaidScale(float HPMult, float PowerMult)
    {
        if (HPMult > 1.f) { MaxHP *= HPMult; CurrentHP = GetMaxHP(); }
        SetBattleScale(FMath::Max(BattleScale, PowerMult)); // 기존 스케일과 합쳐 더 큰 쪽
    }

    // 장비 보너스 설정 (EquipmentComponent가 장착 변경 시 호출)
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void SetEquipBonuses(float Atk, float Def, float HP, float SP)
    { EquipAtk = Atk; EquipDef = Def; EquipHP = HP; EquipSP = SP; }

    // 현재 보정 배율 (UI 표시용, 효과 없으면 1.0)
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE float GetAttackMod() const { return AttackModTurns > 0 ? AttackMod : 1.f; }
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE float GetDefenseMod() const { return DefenseModTurns > 0 ? DefenseMod : 1.f; }
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE float GetCurrentSP() const { return CurrentSP; }
    // 최대 SP = 기본 + 장비 + 마력(MAG) 보정(마법사는 SP 풀이 더 큼). 미설정(MAG 0)이면 기존과 동일.
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE float GetMaxSP() const { return MaxSP + EquipSP + MAG * 0.4f; }
    UFUNCTION(BlueprintPure, Category = "Stats")
    FORCEINLINE bool IsDead() const { return bIsDead; }

    // 즉사: 대상을 즉시 전투불능으로 (즉사 스킬용 — 근성/데미지 계산 우회). 이미 죽었으면 무시
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void Kill()
    {
        if (bIsDead) return;
        CurrentHP = 0.f;
        bIsDead = true;
        OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
        OnDeath.Broadcast();
    }

    // 부활: 죽은 대상을 지정 HP로 되살림 (이미 살아있으면 무시)
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void Revive(float HP)
    {
        if (!bIsDead) return;
        bIsDead = false;
        CurrentHP = FMath::Clamp(HP, 1.f, GetMaxHP());
        OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
    }
};
