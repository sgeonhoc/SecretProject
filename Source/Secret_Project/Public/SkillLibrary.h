#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SkillLibrary.generated.h"

class AABaseCharacter;

/**
 * ★ 깊은 스킬 진행 생성기 (레벨 1~100).
 * 캐릭터의 기존 base Skills(레벨1 시작기, B가 짠 것)를 보고 역할/주속성을 추론한 뒤,
 * 레벨 6~100에 걸쳐 속성 티어 스킬(약→강→극→멸→궁극) + 전체기 + 버프/디버프/회복/만능 궁극을
 * ~15개 이상 자동 생성해 LearnableSkills로 채운다. 원본 스킬명(상표 회피) + 속성기반 SFX/VFX/애니 이름 자동.
 *
 * 역할 추론: 회복기 보유=힐러, 버프/디버프 보유=서포트, 다단히트=스트라이커, 그 외=어태커.
 * → B의 캐릭터 스탯/상성/특수는 보존, 스킬 트리만 깊게.
 */
UCLASS()
class SECRET_PROJECT_API USkillLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // 캐릭터에 깊은 1~100 진행 트리 적용 (ApplyCombatProfile 끝에서 호출). 기존 base Skills 유지.
    UFUNCTION(BlueprintCallable, Category = "Battle|Skills")
    static void ApplyDeepProgression(AABaseCharacter* C);

    // 세부 스탯(STR/MAG/VIT/AGI/LUK) + 성장률 시드. ArchetypeId가 손튜닝 테이블에 있으면 그 값,
    // 없으면 역할/베이스스탯에서 자동 환산. 기존 데미지 밸런스 보존.
    UFUNCTION(BlueprintCallable, Category = "Battle|Skills")
    static void ApplyAttributes(AABaseCharacter* C, FName ArchetypeId, float BaseAtk, float BaseHP, float BaseDef);
};
