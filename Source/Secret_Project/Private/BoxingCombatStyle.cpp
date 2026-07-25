#include "BoxingCombatStyle.h"

// U=좌잽  I=우잽  J=좌훅  K=우훅  L=어퍼컷
// 콤보 5개: 각각 최소 5키 이상, 이 순서대로 입력하면 스킬 자동 발동
// ComboFinisherMontage는 에디터 BP에서 설정
UBoxingCombatStyle::UBoxingCombatStyle()
{
    StyleName = TEXT("복싱");
    MinAttacksPerTurn = 5;

    // ── 메뉴 선택 스킬 (콤보 없이 마우스로 고르는 것) ──
    {
        FMenuSkillDefinition S;
        S.SkillName = TEXT("원투");
        S.DamageMultiplier = 1.2f;
        MenuSkills.Add(S);
    }
    {
        FMenuSkillDefinition S;
        S.SkillName = TEXT("훅 콤보");
        S.DamageMultiplier = 1.5f;
        MenuSkills.Add(S);
    }
    {
        FMenuSkillDefinition S;
        S.SkillName = TEXT("어퍼컷 피니시");
        S.DamageMultiplier = 2.0f;
        MenuSkills.Add(S);
    }

    // ── 입력키 콤보 정의 ──

    // 콤보 1: U→I→U→I→L  "원투 러시 어퍼"  (5키)
    {
        FComboDefinition C;
        C.SkillName = TEXT("원투 러시 어퍼");
        C.SkillDamageMultiplier = 1.8f;
        C.InputSequence = {
            EBoxingInput::LeftJab, EBoxingInput::RightJab,
            EBoxingInput::LeftJab, EBoxingInput::RightJab,
            EBoxingInput::Uppercut
        };
        ComboDefs.Add(C);
    }

    // 콤보 2: J→K→J→K→L  "더블훅 피니시"  (5키)
    {
        FComboDefinition C;
        C.SkillName = TEXT("더블훅 피니시");
        C.SkillDamageMultiplier = 2.0f;
        C.InputSequence = {
            EBoxingInput::LeftHook,  EBoxingInput::RightHook,
            EBoxingInput::LeftHook,  EBoxingInput::RightHook,
            EBoxingInput::Uppercut
        };
        ComboDefs.Add(C);
    }

    // 콤보 3: U→J→I→K→L  "크로스 카운터"  (5키)
    {
        FComboDefinition C;
        C.SkillName = TEXT("크로스 카운터");
        C.SkillDamageMultiplier = 2.2f;
        C.InputSequence = {
            EBoxingInput::LeftJab,  EBoxingInput::LeftHook,
            EBoxingInput::RightJab, EBoxingInput::RightHook,
            EBoxingInput::Uppercut
        };
        ComboDefs.Add(C);
    }

    // 콤보 4: I→U→I→J→K→L  "훅 러시 피니시"  (6키)
    {
        FComboDefinition C;
        C.SkillName = TEXT("훅 러시 피니시");
        C.SkillDamageMultiplier = 2.5f;
        C.InputSequence = {
            EBoxingInput::RightJab, EBoxingInput::LeftJab,
            EBoxingInput::RightJab, EBoxingInput::LeftHook,
            EBoxingInput::RightHook, EBoxingInput::Uppercut
        };
        ComboDefs.Add(C);
    }

    // 콤보 5: U→I→J→I→K→U→L  "풀 러시 어퍼컷"  (7키)
    {
        FComboDefinition C;
        C.SkillName = TEXT("풀 러시 어퍼컷");
        C.SkillDamageMultiplier = 3.0f;
        C.InputSequence = {
            EBoxingInput::LeftJab,  EBoxingInput::RightJab,
            EBoxingInput::LeftHook, EBoxingInput::RightJab,
            EBoxingInput::RightHook, EBoxingInput::LeftJab,
            EBoxingInput::Uppercut
        };
        ComboDefs.Add(C);
    }
}
