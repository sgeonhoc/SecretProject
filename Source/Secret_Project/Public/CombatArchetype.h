#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BattleTypes.h" // FSkillDef, FLevelUpSkill, EBattleElement, EAilment
#include "CombatArchetype.generated.h"

class AANPCCharacter;

/**
 * 캐릭터 전투 프로필 — 한 캐릭터의 전투 콘텐츠 전체(스탯/스킬/약점·내성/패시브/보상).
 * 카탈로그(UCombatArchetypeLibrary)에 항목 추가 = 캐릭터 1명의 "전투 양산".
 * B의 소셜 프로필(NPCArchetype)과 같은 ArchetypeId로 짝지어 = 메시/애니만 입히면 완전체 캐릭터.
 * ArchetypeId가 None인 NPC는 적용 안 됨(수동 BP 모드 보존 = 하위호환).
 */
USTRUCT(BlueprintType)
struct FNPCCombatProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    FName ArchetypeId;

    // 기본 스탯 (레벨1 기준)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float MaxHP = 80.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float Attack = 12.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float Defense = 5.f;

    // 레벨업 성장률 (역할별 차등 밸런싱 — 비우면 기본 10/2/1/5 = 균등 성장)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Growth")
    float GrowMaxHP = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Growth")
    float GrowAttack = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Growth")
    float GrowDefense = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Growth")
    float GrowMaxSP = 5.f;

    // 스킬셋 + 레벨업 습득
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TArray<FSkillDef> Skills;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TArray<FLevelUpSkill> LearnableSkills;

    // 속성 상성
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Affinity")
    TArray<EBattleElement> WeakElements;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Affinity")
    TArray<EBattleElement> ResistElements;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Affinity")
    TArray<EBattleElement> NullElements;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Affinity")
    TArray<EBattleElement> AbsorbElements;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Affinity")
    TArray<EBattleElement> RepelElements;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Affinity")
    TArray<EAilment> AilmentImmune;

    // 패시브/역할
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Passive")
    bool bIsBoss = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Passive", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CounterChance = 0.f;       // 물리 반격 확률
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Passive", meta = (ClampMin = "0.0"))
    float LowHPDamageBonus = 0.f;    // 역경(빈사 시 데미지↑)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Passive")
    bool bEndureOnce = false;        // 근성(전투당 1회 치명타 버팀)

    // 처치 보상 (적으로 등장할 때만 의미 — 아군/주민은 무해)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reward")
    int32 XPReward = 30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reward")
    int32 GoldReward = 20;

    // 아이템 드롭 (인벤토리 카탈로그 Id, 비면 드롭 없음 / 확률 0~1 / 개수)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reward")
    FName DropItemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reward", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DropChance = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reward", meta = (ClampMin = "1"))
    int32 DropCount = 1;
};

/**
 * 전투 양산 카탈로그 + 자동 적용. ArchetypeId로 전투 프로필을 찾아 NPC의 전투 필드를 채운다.
 * ANPCCharacter::BeginPlay의 [A 전투 프로필 훅]에서 ApplyCombatProfile(this) 1줄 호출 → 나머지 자동.
 * B의 UNPCArchetypeLibrary(소셜)와 짝. char_01~char_12 = 같은 캐릭터의 사회/전투 양면.
 */
UCLASS()
class SECRET_PROJECT_API UCombatArchetypeLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ArchetypeId로 전투 프로필 조회 (찾으면 true)
    UFUNCTION(BlueprintCallable, Category = "Combat|Archetype")
    static bool FindCombatProfile(FName ArchetypeId, FNPCCombatProfile& OutProfile);

    // NPC->ArchetypeId 기준으로 전투 프로필 자동 적용. None이면 무동작(수동 BP 모드 보존).
    // ★ StatComponent::InitStats 전에 호출해야 함(NPCMaxHP/Attack/Defense를 InitStats가 집어감).
    static void ApplyCombatProfile(AANPCCharacter* NPC);

    // 카탈로그에 등록된 ArchetypeId 목록 (양산 현황/툴링용)
    UFUNCTION(BlueprintCallable, Category = "Combat|Archetype")
    static TArray<FName> GetAllArchetypeIds();

    // 정합성 검증(개발용): 소셜↔전투 ID 짝 누락·중복ID·적 약점없음을 로그 경고.
    // 양산이 두 인스턴스에서 커질 때 사일런트 누락(기본스탯 추락) 조기 발견. dev 빌드에서 첫 적용 시 1회 자동 실행.
    UFUNCTION(BlueprintCallable, Category = "Combat|Archetype")
    static void ValidateCatalog();

    // 전체 카탈로그 (C++ 전용)
    static const TArray<FNPCCombatProfile>& GetCatalog();
};
