#pragma once

#include "CoreMinimal.h"
#include "BattleTypes.generated.h"

class UAnimMontage;   // FSkillDef.SkillMontage 전방선언(스킬 전용 애니메이션)

// 전투 속성
UENUM(BlueprintType)
enum class EBattleElement : uint8
{
    Physical UMETA(DisplayName = "물리"),
    Fire     UMETA(DisplayName = "화염"),
    Ice      UMETA(DisplayName = "빙결"),
    Elec     UMETA(DisplayName = "전격"),
    Wind     UMETA(DisplayName = "질풍"),
    Light    UMETA(DisplayName = "축복"),
    Dark     UMETA(DisplayName = "저주"),
    Almighty UMETA(DisplayName = "만능")
};

// 속성 → 한글 라벨 (전투 매니저·도감 위젯 공용 단일 소스)
FORCEINLINE FString LexBattleElement(EBattleElement E)
{
    switch (E)
    {
    case EBattleElement::Physical: return TEXT("물리");
    case EBattleElement::Fire:     return TEXT("화염");
    case EBattleElement::Ice:      return TEXT("빙결");
    case EBattleElement::Elec:     return TEXT("전격");
    case EBattleElement::Wind:     return TEXT("질풍");
    case EBattleElement::Light:    return TEXT("축복");
    case EBattleElement::Dark:     return TEXT("저주");
    case EBattleElement::Almighty: return TEXT("만능");
    default:                       return TEXT("?");
    }
}

// 속성 → 영문 식별자 (VFX: NS_<id> / SFX: SFX_<id> 자동 해석용 단일 소스)
FORCEINLINE FName BattleElementId(EBattleElement E)
{
    switch (E)
    {
    case EBattleElement::Physical: return FName("physical");
    case EBattleElement::Fire:     return FName("fire");
    case EBattleElement::Ice:      return FName("ice");
    case EBattleElement::Elec:     return FName("elec");
    case EBattleElement::Wind:     return FName("wind");
    case EBattleElement::Light:    return FName("light");
    case EBattleElement::Dark:     return FName("dark");
    case EBattleElement::Almighty: return FName("almighty");
    default:                       return FName("physical");
    }
}

// 선제/기습 — 전투 진입 방식에 따른 선공권 (페르소나식 어드밴티지)
UENUM(BlueprintType)
enum class EBattleInitiative : uint8
{
    Normal UMETA(DisplayName = "보통"),
    Player UMETA(DisplayName = "선제공격 (플레이어 어드밴티지)"),
    Enemy  UMETA(DisplayName = "기습당함 (적 어드밴티지)")
};

// 속성 상성 (대상 기준)
UENUM(BlueprintType)
enum class EAffinity : uint8
{
    Normal UMETA(DisplayName = "보통"),
    Weak   UMETA(DisplayName = "약점"),
    Resist UMETA(DisplayName = "내성"),
    Null   UMETA(DisplayName = "무효"),
    Absorb UMETA(DisplayName = "흡수"),  // 맞으면 회복
    Repel  UMETA(DisplayName = "반사")   // 시전자에게 되돌림
};

// 전투 연출 이벤트 (C++가 극적 순간 감지 → BP가 페르소나식 애니/사운드/아트로 반응)
UENUM(BlueprintType)
enum class EBattleFlair : uint8
{
    None      UMETA(DisplayName = "없음"),
    Weakness  UMETA(DisplayName = "약점 (WEAK!)"),
    Critical  UMETA(DisplayName = "치명타 (CRITICAL!)"),
    Technical UMETA(DisplayName = "테크니컬 (TECHNICAL!)"),
    OneMore   UMETA(DisplayName = "원모어 (1 MORE!)"),
    BatonPass UMETA(DisplayName = "바톤터치 (BATON PASS!)"),
    AllOut    UMETA(DisplayName = "총공격 (ALL-OUT!)"),
    Miss      UMETA(DisplayName = "빗나감 (MISS)"),
    Repel     UMETA(DisplayName = "반사 (REPEL)"),
    Absorb    UMETA(DisplayName = "흡수 (ABSORB)"),
    Counter   UMETA(DisplayName = "반격 (COUNTER!)"),
    Ailment   UMETA(DisplayName = "상태이상 (AILMENT!)"),
    Endure    UMETA(DisplayName = "근성 (ENDURE!)"),
    Instakill UMETA(DisplayName = "즉사 (INSTA-KILL!)"),
    Ambush    UMETA(DisplayName = "선제공격 (AMBUSH!)"),
    Ambushed  UMETA(DisplayName = "기습당함 (AMBUSHED!)"),
    Victory   UMETA(DisplayName = "승리 (VICTORY)"),
    Defeat    UMETA(DisplayName = "패배 (DEFEAT)")
};

// 상태이상
UENUM(BlueprintType)
enum class EAilment : uint8
{
    None      UMETA(DisplayName = "없음"),
    Poison    UMETA(DisplayName = "독"),
    Sleep     UMETA(DisplayName = "수면"),
    Paralysis UMETA(DisplayName = "마비"),
    Burn      UMETA(DisplayName = "화상"),   // 강한 도트(MaxHP 12%)
    Freeze    UMETA(DisplayName = "빙결"),   // 행동불가 + 피격에도 안 풀림
    Shock     UMETA(DisplayName = "감전")    // 50% 행동불가
};

// 상태이상 한글명 공유 헬퍼 (전투 피드백/상태창 공용 단일 소스)
FORCEINLINE FString LexAilment(EAilment A)
{
    switch (A)
    {
    case EAilment::Poison:    return TEXT("독");
    case EAilment::Sleep:     return TEXT("수면");
    case EAilment::Paralysis: return TEXT("마비");
    case EAilment::Burn:      return TEXT("화상");
    case EAilment::Freeze:    return TEXT("빙결");
    case EAilment::Shock:     return TEXT("감전");
    default:                  return TEXT("");
    }
}

// 스킬 효과 타입
UENUM(BlueprintType)
enum class ESkillType : uint8
{
    DamageOne     UMETA(DisplayName = "단일 공격"),
    DamageAll     UMETA(DisplayName = "전체 공격"),
    HealSelf      UMETA(DisplayName = "자신 회복"),
    HealOne       UMETA(DisplayName = "단일 회복(최저HP)"),
    HealAll       UMETA(DisplayName = "아군 전체 회복"),
    BuffAttack    UMETA(DisplayName = "아군 공격력↑"),
    BuffDefense   UMETA(DisplayName = "아군 방어력↑"),
    DebuffAttack  UMETA(DisplayName = "적 공격력↓"),
    DebuffDefense UMETA(DisplayName = "적 방어력↓"),
    Analyze       UMETA(DisplayName = "분석(약점 공개)"),
    Cure          UMETA(DisplayName = "상태이상 치료"),
    Revive        UMETA(DisplayName = "부활")
};

// 스킬 1개 정의 — 캐릭터 BP에서 배열로 채움
USTRUCT(BlueprintType)
struct FSkillDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString SkillName = TEXT("스킬");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ESkillType SkillType = ESkillType::DamageOne;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 SPCost = 10;

    // 공격: 공격력 대비 위력 배율 / 회복: 회복량 = 배율 × 20
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float PowerMultiplier = 1.5f;

    // 공격 스킬일 때만 사용 (회복은 무시)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    EBattleElement Element = EBattleElement::Almighty;

    // 연타 횟수 (단일 공격에서만, 1 = 일반)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 HitCount = 1;

    // 명중 시 부여할 상태이상 (None이면 없음, 공격 스킬에서만)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    EAilment InflictAilment = EAilment::None;

    // 상태이상 부여 확률 (0~1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float AilmentChance = 0.f;

    // 즉사 확률 (0~1, 0=없음). 데미지기에서 명중 시 별도 판정 — 축복/저주 속성 즉사기에 권장.
    // 무효/흡수/반사 속성 대상과 보스에겐 안 통하고, 약점이면 확률↑·내성이면 확률↓.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float InstaKillChance = 0.f;

    // ★ 스킬 전용 애니메이션 몽타주 — 이 스킬을 쓸 때 시전자가 재생. 비우면 기본 공격 모션 폴백.
    //   사용자는 BP의 캐릭터 Skills 배열에서 스킬마다 몽타주 에셋만 지정하면 됨(C++ 자동 재생).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TObjectPtr<UAnimMontage> SkillMontage = nullptr;

    // ★ 스킬 효과음 이름 — 사용하면 /Game/Audio/SFX/SFX_<이름> 자동 재생(없으면 무음).
    //   예) "fire" → SFX_fire. 몽타주 대신/함께 소리만 넣고 싶을 때. 비우면 무음.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FName SkillSFX = NAME_None;

    // ★ 스킬 이펙트 이름 — 사용하면 /Game/VFX/NS_<이름> 을 대상 위치에 스폰(없으면 무시).
    //   예) "fire" → NS_fire. 사용자는 파일만 넣으면 됨.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FName SkillVFX = NAME_None;

    // ★ 스킬 애니메이션 이름(몽타주 에셋 미지정 시 폴백) — /Game/Anim/Montage/AM_<이름> 자동 로드.
    //   SkillMontage(에셋)가 있으면 그게 우선, 없으면 이 이름으로 로드, 그것도 없으면 기본 공격 모션.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FName SkillAnimName = NAME_None;
};

// 레벨업 습득 스킬 — 캐릭터 BP에서 배열로 채움 (Level 도달 시 Skills에 추가)
USTRUCT(BlueprintType)
struct FLevelUpSkill
{
    GENERATED_BODY()

    // 이 레벨 이상이 되면 습득
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 Level = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FSkillDef Skill;
};
