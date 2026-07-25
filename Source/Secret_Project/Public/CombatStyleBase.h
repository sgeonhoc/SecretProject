#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CombatStyleBase.generated.h"

UENUM(BlueprintType)
enum class EBoxingInput : uint8
{
    LeftJab     UMETA(DisplayName = "Left Jab (U)"),
    RightJab    UMETA(DisplayName = "Right Jab (I)"),
    LeftHook    UMETA(DisplayName = "Left Hook (J)"),
    RightHook   UMETA(DisplayName = "Right Hook (K)"),
    Uppercut    UMETA(DisplayName = "Uppercut (L)"),
    None        UMETA(DisplayName = "None")
};

// 콤보 1개 정의: 입력 시퀀스 + 스킬 이름 + 데미지 배율
USTRUCT(BlueprintType)
struct FComboDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    TArray<EBoxingInput> InputSequence;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    FString SkillName = TEXT("Unknown Skill");

    // 누적 기본 공격 데미지에 곱하는 배율
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    float SkillDamageMultiplier = 1.5f;

    // 콤보 피니셔 전용 몽타주 (ABP에서 지정)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    TObjectPtr<UAnimMontage> ComboFinisherMontage = nullptr;
};

// 메뉴 선택지 스킬 정의 (콤보 없이 마우스 클릭으로 사용하는 스킬)
USTRUCT(BlueprintType)
struct FMenuSkillDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    FString SkillName = TEXT("기본 스킬");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    float DamageMultiplier = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    TObjectPtr<UAnimMontage> SkillMontage = nullptr;
};

UCLASS(Abstract, Blueprintable)
class SECRET_PROJECT_API UCombatStyleBase : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    FString StyleName = TEXT("Base Style");

    // 최소 보장 공격 횟수 (이 횟수 이전에는 콤보 체인이 끊겨도 턴 유지)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    int32 MinAttacksPerTurn = 5;

    // 입력키 콤보 목록
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style|Combos")
    TArray<FComboDefinition> ComboDefs;

    // 마우스 메뉴 선택 스킬 목록
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style|Skills")
    TArray<FMenuSkillDefinition> MenuSkills;

    // 현재 버퍼가 어느 콤보와 완전 일치하는지 반환. 없으면 -1
    int32 FindMatchedCombo(const TArray<EBoxingInput>& Buffer) const;

    // 현재 버퍼가 아직 어떤 콤보의 앞부분인지 확인
    bool IsValidComboPrefix(const TArray<EBoxingInput>& Buffer) const;
};
