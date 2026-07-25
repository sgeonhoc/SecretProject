#include "CombatArchetype.h"
#include "ANPCCharacter.h"
#include "ABaseCharacter.h"
#include "StatComponent.h"
#include "NPCArchetype.h" // 소셜 카탈로그 교차검증용
#include "SkillLibrary.h" // 깊은 1~100 스킬 진행 주입(A)

// ── 스킬 빌더 (가독성용) ──────────────────────────────────
static FSkillDef Skill(const FString& Name, ESkillType Type, int32 SP, float Pow,
    EBattleElement Elem = EBattleElement::Almighty, int32 Hits = 1,
    EAilment Ail = EAilment::None, float AilChance = 0.f, float InstaKill = 0.f)
{
    FSkillDef S;
    S.SkillName = Name;
    S.SkillType = Type;
    S.SPCost = SP;
    S.PowerMultiplier = Pow;
    S.Element = Elem;
    S.HitCount = Hits;
    S.InflictAilment = Ail;
    S.AilmentChance = AilChance;
    S.InstaKillChance = InstaKill;

    // ★ 속성 기반 효과음/이펙트 이름 자동 부여 — 28명 전 스킬에 일괄 적용.
    //   사용자는 SFX_<속성>(예 SFX_fire) · NS_<속성>(예 NS_fire) 파일만 넣으면 모든 해당속성 스킬에 소리·이펙트.
    static const TCHAR* ElemNames[] = {
        TEXT("physical"), TEXT("fire"), TEXT("ice"), TEXT("elec"),
        TEXT("wind"), TEXT("light"), TEXT("dark"), TEXT("almighty")
    };
    const int32 Ei = static_cast<int32>(Elem);
    const FName EName = (Ei >= 0 && Ei < 8) ? FName(ElemNames[Ei]) : FName(TEXT("almighty"));
    S.SkillSFX = EName;
    S.SkillVFX = EName;
    S.SkillAnimName = EName;   // 몽타주 미지정 시 AM_<속성> 자동 로드(있으면), 없으면 기본공격 폴백
    return S;
}
static FLevelUpSkill LvSkill(int32 Level, const FSkillDef& S)
{
    FLevelUpSkill L; L.Level = Level; L.Skill = S; return L;
}

// ── 전투 양산 카탈로그 ────────────────────────────────────
// 세계관: 현대 배경 + 페르소나풍. 속성 체계가 그대로 매핑됨(Light=축복/Hama계, Dark=저주/Mudo계, Almighty=만능/Megido계,
//   즉사=Hama/Mudo, 원모어→총공격, 테크니컬, 차지). 캐릭터 이름/직업은 B 소셜 프로필(현대 직업)에서 정의 — 여기선 ArchetypeId로만 매칭.
// 항목 1개 = 캐릭터 1명의 전투면. B의 소셜 카탈로그(char_01~12)와 ArchetypeId로 1:1 대응.
// 밸런스 의도: 영입 파티원(01/04/05/06/10)은 역할 차별화된 핵심 빌드, 그 외는 역할 맞춤 경량 빌드.
// 주석은 직업이 아니라 "전투 역할"로 표기(현대 직업명은 B가 정의 → 역할은 설정 무관 유지).
const TArray<FNPCCombatProfile>& UCombatArchetypeLibrary::GetCatalog()
{
    static TArray<FNPCCombatProfile> Catalog;
    if (Catalog.Num() > 0) return Catalog;

    auto Add = [](FName Id) -> FNPCCombatProfile&
    {
        FNPCCombatProfile P; P.ArchetypeId = Id;
        return Catalog[Catalog.Add(P)];
    };

    // 1) 지훈 — 활발한 고등학생 / 영입 파티원 · 근접 올라운더(입문형, 약점 없음=생존성↑)
    {
        FNPCCombatProfile& P = Add(TEXT("char_01"));
        P.MaxHP = 100.f; P.Attack = 16.f; P.Defense = 6.f;
        P.GrowMaxHP = 12.f; P.GrowAttack = 2.5f; P.GrowDefense = 1.f; P.GrowMaxSP = 5.f; // 올라운더: 균형
        P.Skills = {
            Skill(TEXT("강타"),    ESkillType::DamageOne, 8,  1.6f, EBattleElement::Physical),
            Skill(TEXT("질풍참"),  ESkillType::DamageOne, 14, 1.8f, EBattleElement::Wind),
            Skill(TEXT("회오리"),  ESkillType::DamageAll, 18, 1.5f, EBattleElement::Wind),
            Skill(TEXT("연격"),    ESkillType::DamageOne, 16, 0.9f, EBattleElement::Physical, 3),
            Skill(TEXT("기합"),    ESkillType::BuffAttack, 12, 1.4f),
            Skill(TEXT("응급처치"),ESkillType::HealOne,   12, 2.0f),
        };
        P.LearnableSkills = {
            LvSkill(3, Skill(TEXT("회오리"), ESkillType::DamageAll, 18, 1.5f, EBattleElement::Wind)),
            LvSkill(6, Skill(TEXT("폭풍연격"), ESkillType::DamageOne, 16, 1.0f, EBattleElement::Wind, 3)),
        };
        P.ResistElements = { EBattleElement::Wind };
        P.CounterChance = 0.1f;
        P.XPReward = 32; P.GoldReward = 22;
    }
    // 2) 서연 — 내성적 도서부원 / 마법형(낮HP·높SP, 물리 약점=글래스캐넌). 책 속 지식이 페르소나로
    {
        FNPCCombatProfile& P = Add(TEXT("char_02"));
        P.MaxHP = 64.f; P.Attack = 11.f; P.Defense = 4.f;
        P.GrowMaxHP = 7.f; P.GrowAttack = 3.f; P.GrowDefense = 0.5f; P.GrowMaxSP = 9.f; // 마법형
        P.Skills = {
            Skill(TEXT("프레이"),    ESkillType::DamageOne, 10, 1.7f, EBattleElement::Light),
            Skill(TEXT("아기"),      ESkillType::DamageOne, 10, 1.7f, EBattleElement::Fire),
            Skill(TEXT("부프"),      ESkillType::DamageOne, 10, 1.7f, EBattleElement::Ice),
            Skill(TEXT("지오"),      ESkillType::DamageOne, 10, 1.7f, EBattleElement::Elec),
            Skill(TEXT("가루다인"),  ESkillType::DamageAll, 20, 1.5f, EBattleElement::Wind),
            Skill(TEXT("사이코킬"),  ESkillType::DamageOne, 12, 1.7f, EBattleElement::Almighty),
        };
        P.LearnableSkills = {
            LvSkill(3, Skill(TEXT("가루다인"), ESkillType::DamageAll, 20, 1.5f, EBattleElement::Wind)),
        };
        P.WeakElements = { EBattleElement::Physical };
        P.XPReward = 30; P.GoldReward = 22;
    }
    // 3) 윤재 선생님 — 지적 교사 / 서포트(버프·디버프·힐 겸비, 균형 스탯). 파티의 지휘자
    {
        FNPCCombatProfile& P = Add(TEXT("char_03"));
        P.MaxHP = 88.f; P.Attack = 12.f; P.Defense = 6.f;
        P.GrowMaxHP = 10.f; P.GrowAttack = 2.f; P.GrowDefense = 1.5f; P.GrowMaxSP = 7.f; // 서포트
        P.Skills = {
            Skill(TEXT("타룬다"),   ESkillType::DebuffAttack, 12, 0.7f), // 적 공격↓
            Skill(TEXT("라쿤다"),   ESkillType::DebuffDefense,12, 0.7f), // 적 방어↓
            Skill(TEXT("타루카자"), ESkillType::BuffAttack,   16, 1.3f), // 아군 공격↑
            Skill(TEXT("라쿠카자"), ESkillType::BuffDefense,  16, 1.3f), // 아군 방어↑
            Skill(TEXT("응급처치"), ESkillType::HealOne,      12, 2.0f),
            Skill(TEXT("디아라마"), ESkillType::HealAll,      20, 2.0f),
            Skill(TEXT("정화"),     ESkillType::Cure,         8,  1.0f),
        };
        P.LearnableSkills = {
            LvSkill(3, Skill(TEXT("타루카자"), ESkillType::BuffAttack, 16, 1.3f)), // 아군 공격↑
            LvSkill(4, Skill(TEXT("디아라마"), ESkillType::HealAll,    20, 2.0f)),
        };
        P.ResistElements = { EBattleElement::Light };
        P.XPReward = 32; P.GoldReward = 24;
    }
    // 4) 강현 — 거친 복싱부 주장 / 영입 파티원 · 물리 탱커딜러(높HP·공격·반격↑·역경·근성)
    {
        FNPCCombatProfile& P = Add(TEXT("char_04"));
        P.MaxHP = 140.f; P.Attack = 20.f; P.Defense = 10.f;
        P.GrowMaxHP = 16.f; P.GrowAttack = 2.5f; P.GrowDefense = 2.f; P.GrowMaxSP = 3.f; // 브루저: HP/방어↑, SP↓
        P.Skills = {
            Skill(TEXT("분쇄"),   ESkillType::DamageOne,  14, 2.0f, EBattleElement::Physical),
            Skill(TEXT("광폭화"), ESkillType::BuffAttack, 12, 1.4f),
        };
        P.LearnableSkills = {
            LvSkill(4, Skill(TEXT("강철 난타"), ESkillType::DamageAll, 20, 1.6f, EBattleElement::Physical)),
        };
        P.WeakElements = { EBattleElement::Ice };
        P.ResistElements = { EBattleElement::Physical };
        P.CounterChance = 0.25f;
        P.LowHPDamageBonus = 0.4f;
        P.bEndureOnce = true;
        P.XPReward = 45; P.GoldReward = 34;
    }
    // 5) 민지 쌤 — 상냥한 보건교사 / 영입 파티원 · 힐러(HealOne/All·Cure·소생, 낮은 공격, 축복)
    {
        FNPCCombatProfile& P = Add(TEXT("char_05"));
        P.MaxHP = 90.f; P.Attack = 11.f; P.Defense = 5.f;
        P.GrowMaxHP = 9.f; P.GrowAttack = 1.5f; P.GrowDefense = 1.f; P.GrowMaxSP = 8.f; // 힐러: SP↑, 공↓
        P.Skills = {
            Skill(TEXT("치유"),   ESkillType::HealOne,   12, 2.5f),
            Skill(TEXT("정화"),   ESkillType::Cure,      8,  1.0f),
            Skill(TEXT("성광"),   ESkillType::DamageOne, 12, 1.5f, EBattleElement::Light),
        };
        P.LearnableSkills = {
            LvSkill(3, Skill(TEXT("전체치유"), ESkillType::HealAll, 22, 2.0f)),
            LvSkill(5, Skill(TEXT("소생"),     ESkillType::Revive,  30, 3.0f)),
        };
        P.WeakElements = { EBattleElement::Dark };
        P.ResistElements = { EBattleElement::Light };
        P.XPReward = 34; P.GoldReward = 26;
    }
    // 6) 탐정 진 — 냉정한 사립탐정 / 영입 파티원 · 기교형(Analyze로 약점 간파 + 상태이상 제압, 약점 적음)
    {
        FNPCCombatProfile& P = Add(TEXT("char_06"));
        P.MaxHP = 96.f; P.Attack = 15.f; P.Defense = 6.f;
        P.GrowMaxHP = 10.f; P.GrowAttack = 2.5f; P.GrowDefense = 1.f; P.GrowMaxSP = 6.f; // 기교형
        P.Skills = {
            Skill(TEXT("간파"),     ESkillType::Analyze,   8,  1.0f),
            Skill(TEXT("급소사격"), ESkillType::DamageOne, 12, 1.6f, EBattleElement::Physical, 1, EAilment::Shock, 0.35f), // 권총=물리+감전제압
            Skill(TEXT("마취침"),   ESkillType::DamageOne, 12, 1.2f, EBattleElement::Physical, 1, EAilment::Sleep, 0.4f),
        };
        P.LearnableSkills = {
            LvSkill(4, Skill(TEXT("연막탄"), ESkillType::DebuffAttack, 14, 0.7f)),
        };
        // 약점 적음(기교형): 약점 없음 + 저주 내성
        P.ResistElements = { EBattleElement::Dark };
        P.bEndureOnce = true;
        P.XPReward = 40; P.GoldReward = 30;
    }
    // 7) 점장 태수 — 활발한 편의점 점장 / 상인·비전투 권장(bCanEnterCombat=false). 만약 싸우면 경량 물리
    {
        FNPCCombatProfile& P = Add(TEXT("char_07"));
        P.MaxHP = 70.f; P.Attack = 10.f; P.Defense = 4.f;
        P.Skills = { Skill(TEXT("물건 던지기"), ESkillType::DamageOne, 6, 1.3f, EBattleElement::Physical) };
        P.XPReward = 18; P.GoldReward = 16;
    }
    // 8) 바텐더 레이 — 냉정한 바텐더 / 상인·비전투 권장. 만약 싸우면 저주 속성
    {
        FNPCCombatProfile& P = Add(TEXT("char_08"));
        P.MaxHP = 75.f; P.Attack = 12.f; P.Defense = 5.f;
        P.Skills = { Skill(TEXT("암습"), ESkillType::DamageOne, 10, 1.6f, EBattleElement::Dark) };
        P.WeakElements = { EBattleElement::Light };
        P.ResistElements = { EBattleElement::Dark };
        P.XPReward = 22; P.GoldReward = 20;
    }
    // 9) 순자씨 — 상냥한 분식집 사장님 / 상인·비전투 권장. 만약 싸우면 힐형
    {
        FNPCCombatProfile& P = Add(TEXT("char_09"));
        P.MaxHP = 70.f; P.Attack = 8.f; P.Defense = 4.f;
        P.Skills = {
            Skill(TEXT("할머니의 손길"), ESkillType::HealOne, 10, 2.0f),
            Skill(TEXT("정화"),          ESkillType::Cure,    8,  1.0f),
        };
        P.XPReward = 16; P.GoldReward = 14;
    }
    // 10) 은우 — 지적 해커 / 영입 파티원 · 마법형(전격 주력+화염, 물리 약점·낮HP=글래스캐넌)
    {
        FNPCCombatProfile& P = Add(TEXT("char_10"));
        P.MaxHP = 72.f; P.Attack = 11.f; P.Defense = 4.f;
        P.GrowMaxHP = 7.f; P.GrowAttack = 3.5f; P.GrowDefense = 0.5f; P.GrowMaxSP = 9.f; // 마법사: 글래스캐넌(공/SP↑, HP/방↓)
        P.Skills = {
            Skill(TEXT("지오"),     ESkillType::DamageOne, 10, 1.7f, EBattleElement::Elec, 1, EAilment::Shock, 0.3f),  // 해커=전격 주력
            Skill(TEXT("아기"),     ESkillType::DamageOne, 12, 1.8f, EBattleElement::Fire, 1, EAilment::Burn,  0.2f),
            Skill(TEXT("부피"),     ESkillType::DamageOne, 12, 1.8f, EBattleElement::Ice,  1, EAilment::Freeze, 0.2f), // 빙결 — 파티 유일 Ice 딜러(shadow_pyre 약점 공략 커버리지)
        };
        P.LearnableSkills = {
            LvSkill(3, Skill(TEXT("마하라지온"), ESkillType::DamageAll, 22, 1.6f, EBattleElement::Elec)), // 전격 전체기
            LvSkill(5, Skill(TEXT("메기도"),     ESkillType::DamageOne, 26, 2.5f, EBattleElement::Almighty)),
        };
        P.WeakElements = { EBattleElement::Physical };
        P.XPReward = 36; P.GoldReward = 28;
    }
    // 11) 정비공 두식 — 거친 정비공 / 물리형 고방어 탱커(화상 면역·반격). 작업장 단련
    {
        FNPCCombatProfile& P = Add(TEXT("char_11"));
        P.MaxHP = 130.f; P.Attack = 17.f; P.Defense = 12.f;
        P.Skills = {
            Skill(TEXT("망치질"), ESkillType::DamageOne,   12, 1.8f, EBattleElement::Physical),
            Skill(TEXT("단단히"), ESkillType::BuffDefense, 12, 1.4f),
        };
        P.ResistElements = { EBattleElement::Fire, EBattleElement::Physical };
        P.AilmentImmune = { EAilment::Burn };
        P.CounterChance = 0.3f;
        P.XPReward = 40; P.GoldReward = 30;
    }
    // 12) 버스커 하늘 — 내성적 버스커(거리 음악가) / 서포트·감성형 버퍼(질풍). 노래로 아군 강화
    {
        FNPCCombatProfile& P = Add(TEXT("char_12"));
        P.MaxHP = 80.f; P.Attack = 10.f; P.Defense = 5.f;
        P.Skills = {
            Skill(TEXT("용기의 노래"), ESkillType::BuffAttack,  14, 1.3f),
            Skill(TEXT("수호의 노래"), ESkillType::BuffDefense, 14, 1.3f),
            Skill(TEXT("바람의 가락"), ESkillType::DamageOne,   10, 1.4f, EBattleElement::Wind),
        };
        P.XPReward = 24; P.GoldReward = 20;
    }

    // ── 확장 로스터 char_13~20 (B 21:23 소셜 추가분 페어링) ──
    // 13) 보라 — 활발한 운동부 매니저 / 영입 파티원 · 서포트 버퍼(팀 사기 진작). 빙결 약점.
    {
        FNPCCombatProfile& P = Add(TEXT("char_13"));
        P.MaxHP = 92.f; P.Attack = 12.f; P.Defense = 6.f;
        P.GrowMaxHP = 10.f; P.GrowAttack = 2.f; P.GrowDefense = 1.f; P.GrowMaxSP = 7.f; // 서포트
        P.Skills = {
            Skill(TEXT("응원의 함성"), ESkillType::BuffAttack,  14, 1.3f),
            Skill(TEXT("작전 지시"),   ESkillType::BuffDefense, 14, 1.3f),
            Skill(TEXT("가루"),        ESkillType::DamageOne,   10, 1.4f, EBattleElement::Wind),
        };
        P.WeakElements = { EBattleElement::Ice };
        P.XPReward = 34; P.GoldReward = 24;
    }
    // 14) 학생회장 도현 — 냉정한 리더 / 영입 파티원 · 균형형(물리+축복+자버프). 약점 적은 에이스.
    {
        FNPCCombatProfile& P = Add(TEXT("char_14"));
        P.MaxHP = 108.f; P.Attack = 16.f; P.Defense = 7.f;
        P.GrowMaxHP = 12.f; P.GrowAttack = 2.5f; P.GrowDefense = 1.5f; P.GrowMaxSP = 5.f; // 균형
        P.Skills = {
            Skill(TEXT("정의의 일격"), ESkillType::DamageOne,  12, 1.8f, EBattleElement::Physical),
            Skill(TEXT("코우하"),      ESkillType::DamageOne,  12, 1.6f, EBattleElement::Light),
            Skill(TEXT("리더의 결단"), ESkillType::BuffAttack, 14, 1.3f),
        };
        P.LearnableSkills = {
            LvSkill(5, Skill(TEXT("코우가"), ESkillType::DamageAll, 22, 1.5f, EBattleElement::Light)),
        };
        P.ResistElements = { EBattleElement::Light };
        P.XPReward = 42; P.GoldReward = 32;
    }
    // 15) 바리스타 유나 — 상냥한 카페 사장 / 상인·비전투 권장(bCanEnterCombat=false). 만약 싸우면 경량.
    {
        FNPCCombatProfile& P = Add(TEXT("char_15"));
        P.MaxHP = 70.f; P.Attack = 9.f; P.Defense = 4.f;
        P.Skills = { Skill(TEXT("뜨거운 커피"), ESkillType::DamageOne, 8, 1.3f, EBattleElement::Fire, 1, EAilment::Burn, 0.2f) };
        P.XPReward = 18; P.GoldReward = 16;
    }
    // 16) 점술가 셀린 — 지적·밤 / 마법·기교형(약점간파+저주 상태이상). 축복 약점.
    {
        FNPCCombatProfile& P = Add(TEXT("char_16"));
        P.MaxHP = 70.f; P.Attack = 13.f; P.Defense = 4.f;
        P.GrowMaxHP = 7.f; P.GrowAttack = 3.f; P.GrowDefense = 0.5f; P.GrowMaxSP = 8.f; // 마법형
        P.Skills = {
            Skill(TEXT("간파"),     ESkillType::Analyze,   8,  1.0f),
            Skill(TEXT("무도"),     ESkillType::DamageOne, 14, 1.5f, EBattleElement::Dark, 1, EAilment::None, 0.f, 0.2f), // Mudo계 즉사 20%
            Skill(TEXT("저주인형"), ESkillType::DamageOne, 12, 1.4f, EBattleElement::Dark, 1, EAilment::Poison, 0.35f),
        };
        P.WeakElements = { EBattleElement::Light };
        P.ResistElements = { EBattleElement::Dark };
        P.XPReward = 38; P.GoldReward = 28;
    }
    // 17) 라이더 철민 — 거친·저녁/밤 / 영입 파티원 · 물리 스피드형(다단히트+전격). 파티 다단히트 복귀.
    {
        FNPCCombatProfile& P = Add(TEXT("char_17"));
        P.MaxHP = 98.f; P.Attack = 17.f; P.Defense = 5.f;
        P.GrowMaxHP = 11.f; P.GrowAttack = 3.f; P.GrowDefense = 1.f; P.GrowMaxSP = 4.f; // 스트라이커
        P.Skills = {
            Skill(TEXT("러시"),       ESkillType::DamageOne, 16, 0.9f, EBattleElement::Physical, 3),
            Skill(TEXT("강슬라이딩"), ESkillType::DamageOne, 12, 2.0f, EBattleElement::Physical),
        };
        P.LearnableSkills = {
            LvSkill(4, Skill(TEXT("지오"), ESkillType::DamageOne, 12, 1.7f, EBattleElement::Elec, 1, EAilment::Shock, 0.25f)),
        };
        P.WeakElements = { EBattleElement::Wind };
        P.CounterChance = 0.15f;
        P.XPReward = 40; P.GoldReward = 30;
    }
    // 18) 하린 — 내성적 미술부 / 마법형(낮HP·높SP, 물리 약점). 질풍·빙결 채색.
    {
        FNPCCombatProfile& P = Add(TEXT("char_18"));
        P.MaxHP = 64.f; P.Attack = 11.f; P.Defense = 4.f;
        P.GrowMaxHP = 7.f; P.GrowAttack = 3.f; P.GrowDefense = 0.5f; P.GrowMaxSP = 9.f; // 마법형
        P.Skills = {
            Skill(TEXT("가루"),  ESkillType::DamageOne, 10, 1.7f, EBattleElement::Wind),
            Skill(TEXT("부피"),  ESkillType::DamageOne, 12, 1.8f, EBattleElement::Ice, 1, EAilment::Freeze, 0.2f),
        };
        P.LearnableSkills = {
            LvSkill(3, Skill(TEXT("마하가루"), ESkillType::DamageAll, 20, 1.5f, EBattleElement::Wind)),
        };
        P.WeakElements = { EBattleElement::Physical };
        P.XPReward = 32; P.GoldReward = 24;
    }
    // 19) DJ 제이 — 활발 방송부·저녁/밤 / 서포트 디버퍼(적 약화). 사운드로 교란.
    {
        FNPCCombatProfile& P = Add(TEXT("char_19"));
        P.MaxHP = 85.f; P.Attack = 12.f; P.Defense = 5.f;
        P.GrowMaxHP = 9.f; P.GrowAttack = 2.f; P.GrowDefense = 1.f; P.GrowMaxSP = 7.f; // 서포트
        P.Skills = {
            Skill(TEXT("노이즈"),     ESkillType::DebuffAttack,  12, 0.7f),
            Skill(TEXT("베이스 드롭"), ESkillType::DebuffDefense, 12, 0.7f),
            Skill(TEXT("사운드웨이브"),ESkillType::DamageOne,     10, 1.5f, EBattleElement::Elec),
        };
        P.WeakElements = { EBattleElement::Dark };
        P.XPReward = 34; P.GoldReward = 26;
    }
    // 20) 수의사 선우 — 상냥 / 영입 파티원 · 힐러(HealOne/Cure/소생, 축복). 두 번째 힐러.
    {
        FNPCCombatProfile& P = Add(TEXT("char_20"));
        P.MaxHP = 88.f; P.Attack = 11.f; P.Defense = 5.f;
        P.GrowMaxHP = 9.f; P.GrowAttack = 1.5f; P.GrowDefense = 1.f; P.GrowMaxSP = 8.f; // 힐러
        P.Skills = {
            Skill(TEXT("디아"),   ESkillType::HealOne,   10, 2.2f),
            Skill(TEXT("패트라"), ESkillType::Cure,      8,  1.0f),
            Skill(TEXT("코우하"), ESkillType::DamageOne, 12, 1.5f, EBattleElement::Light),
        };
        P.LearnableSkills = {
            LvSkill(3, Skill(TEXT("메디아"), ESkillType::HealAll, 22, 2.0f)),
            LvSkill(5, Skill(TEXT("리카암"), ESkillType::Revive,  30, 3.0f)),
        };
        P.WeakElements = { EBattleElement::Dark };
        P.ResistElements = { EBattleElement::Light };
        P.XPReward = 34; P.GoldReward = 26;
    }

    // ── 확장 로스터 char_21~28 (B 21:50 소셜 추가분 페어링) ──
    // 21) 연습생 루나 — 활발 아이돌·저녁/밤 / 영입 파티원 · 스피드 지원형(다단히트+버프). 질풍/축복.
    {
        FNPCCombatProfile& P = Add(TEXT("char_21"));
        P.MaxHP = 86.f; P.Attack = 14.f; P.Defense = 5.f;
        P.GrowMaxHP = 9.f; P.GrowAttack = 2.5f; P.GrowDefense = 1.f; P.GrowMaxSP = 6.f; // 스피드 지원
        P.Skills = {
            Skill(TEXT("응원의 무대"), ESkillType::BuffAttack, 14, 1.3f),
            Skill(TEXT("스텝 연타"),   ESkillType::DamageOne,  14, 0.9f, EBattleElement::Wind, 2),
            Skill(TEXT("트윙클"),      ESkillType::DamageOne,  10, 1.4f, EBattleElement::Light),
        };
        P.WeakElements = { EBattleElement::Dark };
        P.XPReward = 36; P.GoldReward = 26;
    }
    // 22) 강 형사 — 냉정 형사·낮/저녁 / 영입 파티원 · 균형 물리(권총+제압). 약점 적은 베테랑.
    {
        FNPCCombatProfile& P = Add(TEXT("char_22"));
        P.MaxHP = 106.f; P.Attack = 16.f; P.Defense = 7.f;
        P.GrowMaxHP = 12.f; P.GrowAttack = 2.5f; P.GrowDefense = 1.5f; P.GrowMaxSP = 4.f; // 균형 물리
        P.Skills = {
            Skill(TEXT("권총 사격"), ESkillType::DamageOne, 10, 1.7f, EBattleElement::Physical),
            Skill(TEXT("제압 사격"), ESkillType::DamageOne, 14, 1.4f, EBattleElement::Physical, 1, EAilment::Shock, 0.35f),
        };
        P.LearnableSkills = {
            LvSkill(4, Skill(TEXT("간파"), ESkillType::Analyze, 8, 1.0f)),
        };
        P.ResistElements = { EBattleElement::Physical };
        P.CounterChance = 0.15f;
        P.XPReward = 42; P.GoldReward = 32;
    }
    // 23) 사서 한지원 — 지적 도서관 / 마법·지원형(만능 마법+분석). 비전투도 OK. 물리 약점.
    {
        FNPCCombatProfile& P = Add(TEXT("char_23"));
        P.MaxHP = 72.f; P.Attack = 12.f; P.Defense = 4.f;
        P.GrowMaxHP = 7.f; P.GrowAttack = 3.f; P.GrowDefense = 0.5f; P.GrowMaxSP = 8.f; // 마법 지원
        P.Skills = {
            Skill(TEXT("마인드"),   ESkillType::Analyze,   8,  1.0f),
            Skill(TEXT("사이코킬"), ESkillType::DamageOne, 12, 1.6f, EBattleElement::Almighty),
        };
        P.LearnableSkills = {
            LvSkill(3, Skill(TEXT("디아라마"), ESkillType::HealAll, 20, 2.0f)),
        };
        P.WeakElements = { EBattleElement::Physical };
        P.XPReward = 32; P.GoldReward = 24;
    }
    // 24) 정 선생님 — 상냥 의사 / 영입 파티원 · 힐러(디아/패트라/메디아/리카암). 3번째 힐러. 암흑 약점.
    {
        FNPCCombatProfile& P = Add(TEXT("char_24"));
        P.MaxHP = 88.f; P.Attack = 11.f; P.Defense = 5.f;
        P.GrowMaxHP = 9.f; P.GrowAttack = 1.5f; P.GrowDefense = 1.f; P.GrowMaxSP = 8.f; // 힐러
        P.Skills = {
            Skill(TEXT("디아"),   ESkillType::HealOne,   10, 2.2f),
            Skill(TEXT("패트라"), ESkillType::Cure,      8,  1.0f),
            Skill(TEXT("코우하"), ESkillType::DamageOne, 12, 1.5f, EBattleElement::Light),
        };
        P.LearnableSkills = {
            LvSkill(3, Skill(TEXT("메디아"), ESkillType::HealAll, 22, 2.0f)),
            LvSkill(5, Skill(TEXT("리카암"), ESkillType::Revive,  30, 3.0f)),
        };
        P.WeakElements = { EBattleElement::Dark };
        P.ResistElements = { EBattleElement::Light };
        P.XPReward = 34; P.GoldReward = 26;
    }
    // 25) 프로게이머 제로 — 거친·밤 / 영입 파티원 · 기교 물리 딜러(콤보+감전 글리치). 다단히트.
    {
        FNPCCombatProfile& P = Add(TEXT("char_25"));
        P.MaxHP = 96.f; P.Attack = 17.f; P.Defense = 5.f;
        P.GrowMaxHP = 10.f; P.GrowAttack = 3.f; P.GrowDefense = 1.f; P.GrowMaxSP = 5.f; // 기교 딜러
        P.Skills = {
            Skill(TEXT("래피드 콤보"), ESkillType::DamageOne, 16, 0.85f, EBattleElement::Physical, 3),
            Skill(TEXT("글리치"),      ESkillType::DamageOne, 12, 1.5f,  EBattleElement::Elec, 1, EAilment::Shock, 0.4f),
        };
        P.WeakElements = { EBattleElement::Fire };
        P.CounterChance = 0.2f;
        P.XPReward = 40; P.GoldReward = 30;
    }
    // 26) 만화방 구씨 — 내성적 / 상점·비전투 권장(bCanEnterCombat=false). 만약 싸우면 경량.
    {
        FNPCCombatProfile& P = Add(TEXT("char_26"));
        P.MaxHP = 68.f; P.Attack = 9.f; P.Defense = 4.f;
        P.Skills = { Skill(TEXT("던지기"), ESkillType::DamageOne, 6, 1.2f, EBattleElement::Physical) };
        P.XPReward = 16; P.GoldReward = 14;
    }
    // 27) 꽃집 민들레 — 활발 / 상점·비전투 권장. 만약 싸우면 경량 질풍.
    {
        FNPCCombatProfile& P = Add(TEXT("char_27"));
        P.MaxHP = 68.f; P.Attack = 9.f; P.Defense = 4.f;
        P.Skills = { Skill(TEXT("가루"), ESkillType::DamageOne, 8, 1.3f, EBattleElement::Wind) };
        P.XPReward = 16; P.GoldReward = 14;
    }
    // 28) 윤 기자 — 냉정 기자·낮/저녁 / 디버프·기교형(취재=Analyze + 폭로 디버프). 암흑 약점.
    {
        FNPCCombatProfile& P = Add(TEXT("char_28"));
        P.MaxHP = 82.f; P.Attack = 13.f; P.Defense = 5.f;
        P.GrowMaxHP = 9.f; P.GrowAttack = 2.f; P.GrowDefense = 1.f; P.GrowMaxSP = 7.f; // 디버프 지원
        P.Skills = {
            Skill(TEXT("취재"),       ESkillType::Analyze,       8,  1.0f),
            Skill(TEXT("약점 폭로"),   ESkillType::DebuffDefense, 12, 0.7f),
            Skill(TEXT("플래시"),     ESkillType::DamageOne,     10, 1.4f, EBattleElement::Light, 1, EAilment::Sleep, 0.25f),
        };
        P.WeakElements = { EBattleElement::Dark };
        P.XPReward = 34; P.GoldReward = 26;
    }

    // ── 적: 섀도우(Shadow) 양산 ───────────────────────────
    // 현대 페르소나풍 적. 소셜 프로필 불필요(ApplySocialProfile은 미등록 시 무동작) → 전투 카탈로그만으로 완성.
    // 페르소나 루프: 각 섀도우에 명확한 약점 1~2 → 플레이어가 찌르면 원모어/총공격. 보스는 약점 적고 분노(bIsBoss).
    // ★ 커버리지 원칙: 적 약점은 파티가 가격 가능한 속성으로 둘 것(공략 가능 보장).
    //   현재 파티 보유 속성: Light(서연/민지쌤) · Elec/Fire/Ice(은우) · Wind(지훈/서연) · Physical(강현/탐정진).
    //   → lost/hex/tyrant=Light, brute=Elec, pyre=Ice, frost=Fire 전부 공략 가능. 새 적 약점도 이 풀 안에서.
    // 사용자: 적 NPC BP에 ArchetypeId(shadow_xx) + bIsAlly=false + bCanEnterCombat=true + 메시/애니만.

    // 잡몹 — 떠도는 그림자: 약하고 축복 약점
    {
        FNPCCombatProfile& P = Add(TEXT("shadow_lost"));
        P.MaxHP = 50.f; P.Attack = 12.f; P.Defense = 3.f;
        P.Skills = { Skill(TEXT("할퀴기"), ESkillType::DamageOne, 6, 1.2f, EBattleElement::Physical) };
        P.WeakElements = { EBattleElement::Light };
        P.DropItemId = TEXT("HealPotion"); P.DropChance = 0.30f;
        P.XPReward = 25; P.GoldReward = 10; // 골드 하향(전투=보조수입, B 경제 정렬 22:00)
    }
    // 물리 브루저 — 광폭한 그림자: 전격 약점, 물리 내성, 반격
    {
        FNPCCombatProfile& P = Add(TEXT("shadow_brute"));
        P.MaxHP = 110.f; P.Attack = 18.f; P.Defense = 8.f;
        P.Skills = {
            Skill(TEXT("후려치기"), ESkillType::DamageOne,  10, 1.6f, EBattleElement::Physical),
            Skill(TEXT("포효"),     ESkillType::BuffAttack, 12, 1.3f),
        };
        P.WeakElements = { EBattleElement::Elec };
        P.ResistElements = { EBattleElement::Physical };
        P.CounterChance = 0.15f;
        P.DropItemId = TEXT("HiPotion"); P.DropChance = 0.35f;
        P.XPReward = 50; P.GoldReward = 20;
    }
    // 화염 술사 — 잿불 그림자: 빙결 약점, 화염 흡수
    {
        FNPCCombatProfile& P = Add(TEXT("shadow_pyre"));
        P.MaxHP = 70.f; P.Attack = 14.f; P.Defense = 5.f;
        P.Skills = {
            Skill(TEXT("파이어볼"), ESkillType::DamageOne, 12, 1.7f, EBattleElement::Fire, 1, EAilment::Burn, 0.25f),
            Skill(TEXT("화염폭발"), ESkillType::DamageAll, 20, 1.5f, EBattleElement::Fire),
        };
        P.WeakElements = { EBattleElement::Ice };
        P.AbsorbElements = { EBattleElement::Fire };
        P.DropItemId = TEXT("SPPotion"); P.DropChance = 0.30f;
        P.XPReward = 45; P.GoldReward = 16;
    }
    // 빙결 술사 — 서리 그림자: 화염 약점, 빙결 흡수
    {
        FNPCCombatProfile& P = Add(TEXT("shadow_frost"));
        P.MaxHP = 70.f; P.Attack = 14.f; P.Defense = 5.f;
        P.Skills = {
            Skill(TEXT("아이스 랜스"), ESkillType::DamageOne, 12, 1.7f, EBattleElement::Ice, 1, EAilment::Freeze, 0.25f),
            Skill(TEXT("눈보라"),     ESkillType::DamageAll, 20, 1.5f, EBattleElement::Ice),
        };
        P.WeakElements = { EBattleElement::Fire };
        P.AbsorbElements = { EBattleElement::Ice };
        P.DropItemId = TEXT("SPPotion"); P.DropChance = 0.30f;
        P.XPReward = 45; P.GoldReward = 16;
    }
    // 저주술사 — 흉조의 그림자: 축복 약점, 저주 내성, 즉사기(Mudo계) + 독
    {
        FNPCCombatProfile& P = Add(TEXT("shadow_hex"));
        P.MaxHP = 80.f; P.Attack = 13.f; P.Defense = 5.f;
        P.Skills = {
            Skill(TEXT("저주의 손길"), ESkillType::DamageOne, 12, 1.4f, EBattleElement::Dark, 1, EAilment::Poison, 0.3f),
            Skill(TEXT("암흑")  ,      ESkillType::DamageOne, 14, 1.5f, EBattleElement::Dark, 1, EAilment::None, 0.f, 0.25f), // 즉사 25%
        };
        P.WeakElements = { EBattleElement::Light };
        P.ResistElements = { EBattleElement::Dark };
        P.DropItemId = TEXT("Antidote"); P.DropChance = 0.40f;
        P.XPReward = 48; P.GoldReward = 18;
    }
    // 후방 지원형 — 수호하는 그림자: 아군 섀도우 회복/방어버프(AI가 아군 위급 시 힐 발동). "힐러부터 잡아라" 유도. 전격 약점.
    {
        FNPCCombatProfile& P = Add(TEXT("shadow_warden"));
        P.MaxHP = 90.f; P.Attack = 11.f; P.Defense = 7.f;
        P.Skills = {
            Skill(TEXT("힐링 오브"),   ESkillType::HealAll,     16, 1.6f),
            Skill(TEXT("수호 결계"),   ESkillType::BuffDefense, 14, 1.4f),
            Skill(TEXT("저주의 손길"), ESkillType::DamageOne,   10, 1.3f, EBattleElement::Dark),
        };
        P.WeakElements = { EBattleElement::Elec };
        P.ResistElements = { EBattleElement::Dark };
        P.DropItemId = TEXT("Elixir"); P.DropChance = 0.25f;
        P.XPReward = 55; P.GoldReward = 22;
    }
    // 원소 반사형 — 거울 그림자: 화염 반사·빙결 흡수·전격 내성 → 맹목적 마법 스팸 응징(Analyze 권장). 물리/만능으로 공략.
    {
        FNPCCombatProfile& P = Add(TEXT("shadow_mirror"));
        P.MaxHP = 80.f; P.Attack = 13.f; P.Defense = 6.f;
        P.Skills = {
            Skill(TEXT("반사파"), ESkillType::DamageOne,    12, 1.5f, EBattleElement::Almighty),
            Skill(TEXT("현혹"),   ESkillType::DebuffAttack, 12, 0.7f),
        };
        P.WeakElements   = { EBattleElement::Physical }; // 물리로 공략(강현/탐정진/지훈)
        P.RepelElements  = { EBattleElement::Fire };     // 화염 반사 → 시전자 피해
        P.AbsorbElements = { EBattleElement::Ice };      // 빙결 흡수 → 오히려 회복
        P.ResistElements = { EBattleElement::Elec };
        P.DropItemId = TEXT("HiPotion"); P.DropChance = 0.35f;
        P.XPReward = 60; P.GoldReward = 24;
    }
    // 보스 — 군림하는 그림자: 고HP, 물리·화염 내성, 분노(bIsBoss=수면/빙결 자동면역), 전체기 + 강타
    {
        FNPCCombatProfile& P = Add(TEXT("shadow_tyrant"));
        P.MaxHP = 420.f; P.Attack = 22.f; P.Defense = 12.f;
        P.Skills = {
            Skill(TEXT("멸시의 일격"), ESkillType::DamageOne,   16, 2.2f, EBattleElement::Almighty),
            Skill(TEXT("절망의 파동"), ESkillType::DamageAll,   24, 1.7f, EBattleElement::Dark),
            Skill(TEXT("군림"),        ESkillType::BuffAttack,  14, 1.4f),
        };
        P.LearnableSkills = {};
        P.ResistElements = { EBattleElement::Physical, EBattleElement::Fire };
        P.WeakElements = { EBattleElement::Light };  // 유일 약점 — 공략 포인트
        P.bIsBoss = true;
        P.DropItemId = TEXT("Elixir"); P.DropChance = 1.0f; P.DropCount = 2; // 보스 확정 드롭
        P.XPReward = 220; P.GoldReward = 90; // 보스 골드도 하향(보스전 보상은 드롭/XP 위주)
    }
    // 보스2 — 흑마술사 그림자: tyrant와 대조 퍼즐. 물리 favoring(물리 약점) + 상태이상 압박 + 자힐.
    //   요구: 물리 딜러(강현/철민/제로/강형사) · 상태이상 관리(Cure/해독제) · 자힐 압도하는 지속딜. 화염/저주 마법은 막힘.
    {
        FNPCCombatProfile& P = Add(TEXT("shadow_warlock"));
        P.MaxHP = 400.f; P.Attack = 20.f; P.Defense = 10.f;
        P.Skills = {
            Skill(TEXT("흑염 폭풍"), ESkillType::DamageAll, 24, 1.6f, EBattleElement::Dark),
            Skill(TEXT("악몽"),      ESkillType::DamageOne, 14, 1.4f, EBattleElement::Dark, 1, EAilment::Sleep,  0.5f),
            Skill(TEXT("맹독 안개"), ESkillType::DamageAll, 20, 1.1f, EBattleElement::Almighty, 1, EAilment::Poison, 0.45f),
            Skill(TEXT("흡정"),      ESkillType::HealSelf,  16, 3.0f), // 자힐 — AI가 위급 시 발동, 지속딜로 압도해야 함
        };
        P.WeakElements   = { EBattleElement::Physical }; // 약점 = 물리(연약한 술사)
        P.ResistElements = { EBattleElement::Fire, EBattleElement::Dark };
        P.bIsBoss = true; // 분노 + 수면/빙결 자동면역
        P.DropItemId = TEXT("Elixir"); P.DropChance = 1.0f; P.DropCount = 2;
        P.XPReward = 240; P.GoldReward = 95;
    }

    return Catalog;
}

bool UCombatArchetypeLibrary::FindCombatProfile(FName ArchetypeId, FNPCCombatProfile& OutProfile)
{
    if (ArchetypeId.IsNone()) return false;
    for (const FNPCCombatProfile& P : GetCatalog())
        if (P.ArchetypeId == ArchetypeId) { OutProfile = P; return true; }
    return false;
}

TArray<FName> UCombatArchetypeLibrary::GetAllArchetypeIds()
{
    TArray<FName> Ids;
    for (const FNPCCombatProfile& P : GetCatalog()) Ids.Add(P.ArchetypeId);
    return Ids;
}

void UCombatArchetypeLibrary::ValidateCatalog()
{
    const TArray<FNPCCombatProfile>& Combat = GetCatalog();
    const TArray<FName> SocialIds = UNPCArchetypeLibrary::GetAllArchetypeIds();

    // 1) 전투 카탈로그 중복 ID
    TSet<FName> Seen;
    for (const FNPCCombatProfile& P : Combat)
    {
        if (Seen.Contains(P.ArchetypeId))
            UE_LOG(LogTemp, Warning, TEXT("[CombatArchetype] 중복 ID: %s"), *P.ArchetypeId.ToString());
        Seen.Add(P.ArchetypeId);
    }

    // 2) 소셜에 있는데 전투 프로필 없음 → 기본 스탯으로 떨어짐(의도치 않은 누락)
    for (FName Id : SocialIds)
    {
        FNPCCombatProfile Tmp;
        if (!FindCombatProfile(Id, Tmp))
            UE_LOG(LogTemp, Warning, TEXT("[CombatArchetype] 소셜 '%s'에 전투 프로필 없음 → 기본스탯. 전투 페어링 필요."), *Id.ToString());
    }

    // 3) char_* 전투인데 소셜 없음(이름 미설정) / 4) 적(shadow_*) 비보스 약점없음(공략 불가)
    for (const FNPCCombatProfile& P : Combat)
    {
        const FString IdStr = P.ArchetypeId.ToString();
        FNPCSocialProfile S;
        if (IdStr.StartsWith(TEXT("char_")) && !UNPCArchetypeLibrary::FindSocialProfile(P.ArchetypeId, S))
            UE_LOG(LogTemp, Warning, TEXT("[CombatArchetype] 전투 '%s'에 소셜 프로필 없음 → 이름 미설정."), *IdStr);

        if (IdStr.StartsWith(TEXT("shadow_")) && !P.bIsBoss && P.WeakElements.Num() == 0)
            UE_LOG(LogTemp, Warning, TEXT("[CombatArchetype] 적 '%s' 약점 없음 → 약점공략 루프 불가."), *IdStr);
    }

    UE_LOG(LogTemp, Display, TEXT("[CombatArchetype] 카탈로그 검증 완료: 전투 %d종 / 소셜 %d종."), Combat.Num(), SocialIds.Num());
}

void UCombatArchetypeLibrary::ApplyCombatProfile(AANPCCharacter* NPC)
{
#if !UE_BUILD_SHIPPING
    // dev 빌드: 첫 적용 시 1회 카탈로그 정합성 자동 검증(누락 조기 발견). 출시 빌드는 스킵.
    static bool bValidatedOnce = false;
    if (!bValidatedOnce) { bValidatedOnce = true; ValidateCatalog(); }
#endif

    if (!NPC || NPC->ArchetypeId.IsNone()) return; // None = 수동 BP 모드(기존 동작 보존)

    FNPCCombatProfile P;
    if (!FindCombatProfile(NPC->ArchetypeId, P)) return;

    // 스탯: NPCMaxHP/Attack/Defense를 채워두면 직후 InitStats(이 훅 다음 줄)가 집어감
    NPC->NPCMaxHP  = P.MaxHP;
    NPC->NPCAttack = P.Attack;
    NPC->NPCDefense = P.Defense;

    // 스킬/상성/패시브 (AABaseCharacter 필드)
    NPC->Skills           = P.Skills;
    NPC->LearnableSkills  = P.LearnableSkills;
    NPC->WeakElements     = P.WeakElements;
    NPC->ResistElements   = P.ResistElements;
    NPC->NullElements     = P.NullElements;
    NPC->AbsorbElements   = P.AbsorbElements;
    NPC->RepelElements    = P.RepelElements;
    NPC->AilmentImmune    = P.AilmentImmune;
    NPC->bIsBoss          = P.bIsBoss;
    NPC->CounterChance    = P.CounterChance;
    NPC->LowHPDamageBonus = P.LowHPDamageBonus;
    NPC->DropItemId       = P.DropItemId;
    NPC->DropChance       = P.DropChance;
    NPC->DropCount        = P.DropCount;

    // StatComponent 설정(근성/보상) — InitStats가 건드리지 않는 값이라 훅 시점 설정이 보존됨
    if (UStatComponent* St = NPC->GetStatComponent())
    {
        St->SetEndureOnce(P.bEndureOnce);
        St->SetXPReward(P.XPReward);
        St->SetGoldReward(P.GoldReward);
        St->SetGrowthRates(P.GrowMaxHP, P.GrowAttack, P.GrowDefense, P.GrowMaxSP); // 역할별 차등 성장
    }

    // ★ 세부 스탯(STR/MAG/VIT/AGI/LUK) 시드 — 영입 캐릭터 + 적 **모두**.
    //   베이스 공격력/HP/방어 + 역할에서 환산하되 주공격스탯=BaseAtk라 기존 데미지 밸런스 보존.
    //   → 적도 역할별 AGI(턴순서/명중회피)·VIT(방어)·LUK(치명)을 가짐(AGI 턴순서가 적에게도 의미).
    //   적은 고정스탯이라 ApplyAttributes가 함께 설정하는 성장률은 미사용(무해).
    USkillLibrary::ApplyAttributes(NPC, NPC->ArchetypeId, P.Attack, P.MaxHP, P.Defense);

    // 깊은 1~100 스킬 진행 트리(A)는 영입 캐릭터(char_*)만 — 적은 레벨업 안 하므로 제외.
    if (NPC->ArchetypeId.ToString().StartsWith(TEXT("char_")))
    {
        USkillLibrary::ApplyDeepProgression(NPC);
    }
}
