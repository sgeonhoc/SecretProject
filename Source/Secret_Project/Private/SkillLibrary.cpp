#include "SkillLibrary.h"
#include "ABaseCharacter.h"
#include "StatComponent.h"

// 속성별 스킬 이름표 [속성][0=T1 1=T2 2=T3 3=T4 4=궁극 5=전체1 6=전체2 7=전체3]
// 원본 이름(상표 회피). 속성 인덱스: Physical0 Fire1 Ice2 Elec3 Wind4 Light5 Dark6 Almighty7
static const TCHAR* GElemNames[8][8] = {
    { TEXT("가격"),   TEXT("강타"),     TEXT("연격"),   TEXT("분쇄"),   TEXT("파천왕"),   TEXT("난타"),     TEXT("폭쇄"),       TEXT("파멸진") },     // Physical
    { TEXT("불티"),   TEXT("화염탄"),   TEXT("겁화"),   TEXT("업화"),   TEXT("멸화천"),   TEXT("불의비"),   TEXT("화염폭풍"),   TEXT("연옥") },       // Fire
    { TEXT("성에"),   TEXT("빙결창"),   TEXT("한설"),   TEXT("빙하"),   TEXT("절대영도"), TEXT("서리비"),   TEXT("눈보라"),     TEXT("동결지옥") },   // Ice
    { TEXT("정전"),   TEXT("전격탄"),   TEXT("낙뢰"),   TEXT("뇌격"),   TEXT("뇌신강림"), TEXT("스파크비"), TEXT("감전폭풍"),   TEXT("멸뢰") },       // Elec
    { TEXT("산들바람"),TEXT("질풍참"),  TEXT("폭풍"),   TEXT("태풍"),   TEXT("회천"),     TEXT("바람칼날"), TEXT("질풍난무"),   TEXT("천풍") },       // Wind
    { TEXT("성광"),   TEXT("축복"),     TEXT("심판"),   TEXT("천벌"),   TEXT("신성림"),   TEXT("빛무리"),   TEXT("성단"),       TEXT("신의빛") },     // Light
    { TEXT("음영"),   TEXT("암흑"),     TEXT("저주"),   TEXT("심연"),   TEXT("멸혼"),     TEXT("어둠비"),   TEXT("암흑파동"),   TEXT("나락") },       // Dark
    { TEXT("충격"),   TEXT("파동"),     TEXT("공명"),   TEXT("붕괴"),   TEXT("종말"),     TEXT("파동탄"),   TEXT("대붕괴"),     TEXT("천멸") },       // Almighty
};
// 속성 → SFX/VFX/애니 이름(CombatArchetype과 동일 규칙)
static const TCHAR* GElemKey[8] = {
    TEXT("physical"), TEXT("fire"), TEXT("ice"), TEXT("elec"),
    TEXT("wind"), TEXT("light"), TEXT("dark"), TEXT("almighty")
};

static int32 ElemIdx(EBattleElement E)
{
    const int32 i = static_cast<int32>(E);
    return (i >= 0 && i < 8) ? i : 7;
}

// 스킬 1개 생성 (속성 기반 SFX/VFX/애니 이름 자동 부여)
static FSkillDef Mk(const FString& Name, ESkillType Type, int32 SP, float Pow,
    EBattleElement Elem = EBattleElement::Almighty, int32 Hits = 1,
    EAilment Ail = EAilment::None, float AilCh = 0.f, float Insta = 0.f)
{
    FSkillDef S;
    S.SkillName = Name; S.SkillType = Type; S.SPCost = SP; S.PowerMultiplier = Pow;
    S.Element = Elem; S.HitCount = Hits; S.InflictAilment = Ail; S.AilmentChance = AilCh; S.InstaKillChance = Insta;
    const FName Key(GElemKey[ElemIdx(Elem)]);
    S.SkillSFX = Key; S.SkillVFX = Key; S.SkillAnimName = Key;
    return S;
}

// 속성 공격기(티어 0~7) — 0~4=단일, 5~7=전체
static FSkillDef ElemSkill(EBattleElement E, int32 Tier, int32 Lvl)
{
    const int32 ei = ElemIdx(E);
    const FString Name = GElemNames[ei][FMath::Clamp(Tier, 0, 7)];
    // 단일 티어 위력/SP
    static const float SPow[5] = { 1.6f, 2.0f, 2.7f, 3.4f, 4.4f };
    static const int32 SSp[5]  = { 8, 14, 22, 36, 54 };
    static const float APow[3] = { 1.4f, 1.9f, 2.5f };
    static const int32 ASp[3]  = { 22, 38, 56 };
    if (Tier <= 4)
        return Mk(Name, ESkillType::DamageOne, SSp[Tier], SPow[Tier], E);
    else
        return Mk(Name, ESkillType::DamageAll, ASp[Tier - 5], APow[Tier - 5], E);
}

// 캐릭터별 손튜닝 5스탯 (base STR/MAG/VIT/AGI/LUK + 성장 gSTR/gMAG/gVIT/gAGI/gLUK)
struct FAttrSpread { const TCHAR* Id; float S, M, V, A, L, gS, gM, gV, gA, gL; };
static const FAttrSpread GAttrTable[] = {
    // 영입/전투 캐릭터 — 역할별 정밀 배분 + 밸런스. 주공격스탯이 데미지 원천.
    { TEXT("char_01"), 15,7,12,12,10,  2.4f,0.7f,1.9f,1.3f,1.1f }, // 지훈 올라운더
    { TEXT("char_02"),  5,16,7,11,11,  0.6f,2.7f,1.3f,1.2f,1.1f }, // 서연 마법글캐
    { TEXT("char_03"),  9,13,12,10,12, 1.0f,1.8f,1.9f,1.1f,1.3f }, // 윤재쌤 서포트
    { TEXT("char_04"), 19,5,17,8,9,    2.5f,0.5f,2.6f,0.8f,0.9f }, // 강현 물리탱딜
    { TEXT("char_05"),  5,15,12,10,13, 0.6f,2.2f,1.9f,1.1f,1.4f }, // 민지쌤 힐러
    { TEXT("char_06"), 15,9,11,15,13,  2.0f,1.0f,1.6f,1.7f,1.5f }, // 탐정진 기교
    { TEXT("char_07"),  8,5,9,9,8,     1.0f,0.6f,1.0f,1.0f,0.9f }, // 점장 상인
    { TEXT("char_08"),  8,7,9,10,8,    1.0f,0.8f,1.0f,1.1f,0.9f }, // 바텐더 상인
    { TEXT("char_09"),  6,9,9,8,9,     0.7f,1.0f,1.0f,0.9f,1.0f }, // 순자 상인
    { TEXT("char_10"),  5,17,7,12,11,  0.6f,2.8f,1.2f,1.3f,1.1f }, // 은우 마법글캐
    { TEXT("char_11"), 17,5,18,7,9,    2.3f,0.5f,2.7f,0.7f,0.9f }, // 정비공 물탱
    { TEXT("char_12"),  9,12,12,11,12, 1.0f,1.6f,1.8f,1.2f,1.3f }, // 버스커 서포트
    { TEXT("char_13"), 11,11,13,11,12, 1.2f,1.4f,1.9f,1.2f,1.3f }, // 보라 서포트
    { TEXT("char_14"), 16,11,13,11,11, 2.2f,1.4f,2.0f,1.2f,1.2f }, // 도현 균형에이스
    { TEXT("char_15"),  7,8,9,9,9,     0.8f,0.9f,1.0f,1.0f,1.0f }, // 유나 상인
    { TEXT("char_16"),  6,16,8,12,14,  0.7f,2.5f,1.3f,1.3f,1.6f }, // 셀린 마법기교(운/즉사)
    { TEXT("char_17"), 17,6,12,16,11,  2.3f,0.6f,1.8f,1.8f,1.2f }, // 철민 물리스트라이커
    { TEXT("char_18"),  5,15,7,11,11,  0.6f,2.6f,1.3f,1.2f,1.1f }, // 하린 마법글캐
    { TEXT("char_19"), 10,13,12,12,12, 1.1f,1.7f,1.8f,1.3f,1.3f }, // DJ제이 디버퍼
    { TEXT("char_20"),  6,14,12,10,13, 0.7f,2.1f,1.9f,1.1f,1.4f }, // 선우 힐러
    { TEXT("char_21"), 13,10,11,16,12, 1.6f,1.2f,1.7f,1.9f,1.3f }, // 루나 스피드지원
    { TEXT("char_22"), 16,7,13,12,11,  2.2f,0.7f,2.0f,1.4f,1.2f }, // 강형사 균형물리
    { TEXT("char_23"),  6,15,9,11,12,  0.7f,2.3f,1.5f,1.2f,1.3f }, // 한지원 마법지원
    { TEXT("char_24"),  6,14,12,10,13, 0.7f,2.1f,1.9f,1.1f,1.4f }, // 정선생 힐러
    { TEXT("char_25"), 17,7,11,16,13,  2.3f,0.7f,1.7f,1.8f,1.5f }, // 제로 기교물리(치명)
    { TEXT("char_26"),  7,6,9,9,8,     0.8f,0.7f,1.0f,1.0f,0.9f }, // 구씨 상인
    { TEXT("char_27"),  7,7,9,10,9,    0.8f,0.8f,1.0f,1.1f,1.0f }, // 민들레 상인
    { TEXT("char_28"), 11,12,11,13,13, 1.2f,1.6f,1.7f,1.5f,1.4f }, // 윤기자 디버프기교
};

void USkillLibrary::ApplyAttributes(AABaseCharacter* C, FName ArchetypeId, float BaseAtk, float BaseHP, float BaseDef)
{
    if (!C) return;
    UStatComponent* St = C->GetStatComponent();
    if (!St) return;

    // 1) 손튜닝 테이블 우선 — 캐릭터별 정밀 스프레드
    const FString IdStr = ArchetypeId.ToString();
    for (const FAttrSpread& A : GAttrTable)
    {
        if (IdStr == A.Id)
        {
            St->SetAttributes(A.S, A.M, A.V, A.A, A.L);
            St->SetAttributeGrowth(A.gS, A.gM, A.gV, A.gA, A.gL);
            return;
        }
    }
    // 2) 테이블에 없으면 역할/베이스스탯에서 자동 환산(폴백)

    // 역할 추론(스킬 기반): 마법형(비물리 공격 위주)/물리형/스트라이커/탱커/힐러서포트
    int32 PhysCnt = 0, MagCnt = 0; bool bHeal = false, bSupport = false, bMulti = false;
    for (const FSkillDef& S : C->Skills)
    {
        switch (S.SkillType)
        {
        case ESkillType::DamageOne: case ESkillType::DamageAll:
            if (S.Element == EBattleElement::Physical) ++PhysCnt; else ++MagCnt;
            if (S.HitCount > 1) bMulti = true;
            break;
        case ESkillType::HealOne: case ESkillType::HealAll: case ESkillType::HealSelf:
        case ESkillType::Revive: case ESkillType::Cure: bHeal = true; break;
        case ESkillType::BuffAttack: case ESkillType::BuffDefense:
        case ESkillType::DebuffAttack: case ESkillType::DebuffDefense: bSupport = true; break;
        default: break;
        }
    }
    const bool bCaster = (MagCnt > PhysCnt) || bHeal;     // 마법/힐 위주 = 마력형
    const bool bTanky  = (BaseHP >= 120.f) || (BaseDef >= 9.f);

    // 공격 주스탯 = 베이스 공격력에 맞춤(밸런스 보존). 부스탯은 40%.
    const float Prim = FMath::RoundToFloat(BaseAtk);
    const float Off  = FMath::RoundToFloat(BaseAtk * 0.4f);
    const float Str  = bCaster ? Off  : Prim;
    const float Mag  = bCaster ? Prim : Off;
    // 체력(VIT): HP/방어에서 환산(표시 + 방어보정). 운/민첩: 역할 가미.
    const float Vit  = FMath::Max(5.f, FMath::RoundToFloat((BaseHP - 50.f) / 9.f + BaseDef * 0.5f));
    const float Agi  = bMulti ? 15.f : (bTanky ? 7.f : (bCaster ? 9.f : 11.f));
    const float Luk  = bSupport ? 11.f : 9.f;
    St->SetAttributes(Str, Mag, Vit, Agi, Luk);

    // 성장률: 주스탯↑, 탱커는 VIT↑. (GetAttack/MagPower가 STR/MAG를 쓰므로 레벨 스케일 보장)
    const float GPrim = 2.6f, GOff = 0.8f;
    St->SetAttributeGrowth(
        bCaster ? GOff : GPrim,                  // STR
        bCaster ? GPrim : GOff,                  // MAG
        bTanky ? 2.4f : 1.5f,                    // VIT
        bMulti ? 1.4f : (bTanky ? 0.6f : 1.0f),  // AGI
        0.8f                                     // LUK
    );
}

void USkillLibrary::ApplyDeepProgression(AABaseCharacter* C)
{
    if (!C) return;

    // ── 1) 기존 base 스킬로 역할/주속성 추론 ──
    TMap<EBattleElement, int32> ElemCount;
    bool bHealer = false, bSupport = false, bMultiHit = false;
    for (const FSkillDef& S : C->Skills)
    {
        switch (S.SkillType)
        {
        case ESkillType::DamageOne:
        case ESkillType::DamageAll:
            ElemCount.FindOrAdd(S.Element)++;
            if (S.HitCount > 1) bMultiHit = true;
            break;
        case ESkillType::HealOne: case ESkillType::HealAll: case ESkillType::HealSelf:
        case ESkillType::Revive:  case ESkillType::Cure:
            bHealer = true; break;
        case ESkillType::BuffAttack: case ESkillType::BuffDefense:
        case ESkillType::DebuffAttack: case ESkillType::DebuffDefense:
            bSupport = true; break;
        default: break;
        }
    }
    // 주/부 속성
    EBattleElement Prim = EBattleElement::Almighty, Sec = EBattleElement::Physical;
    int32 Best = 0, Best2 = -1;
    for (const TPair<EBattleElement, int32>& KV : ElemCount)
    {
        if (KV.Value > Best) { Best2 = Best; Sec = Prim; Best = KV.Value; Prim = KV.Key; }
        else if (KV.Value > Best2) { Best2 = KV.Value; Sec = KV.Key; }
    }
    if (Best == 0) Prim = EBattleElement::Almighty;
    if (Sec == Prim) Sec = EBattleElement::Almighty;

    // ── 2) 역할별 1~100 진행 트리 생성 ──
    TArray<FLevelUpSkill> Tree;
    auto Add = [&](int32 Lvl, const FSkillDef& S) { FLevelUpSkill L; L.Level = Lvl; L.Skill = S; Tree.Add(L); };

    if (bHealer)
    {
        // 힐러: 회복/소생/정화 + 주속성(보통 축복) 보조
        Add(5,  Mk(TEXT("치유"),   ESkillType::HealOne, 10, 2.2f));
        Add(9,  Mk(TEXT("정화"),   ESkillType::Cure,    8,  1.0f));
        Add(14, Mk(TEXT("회복"),   ESkillType::HealOne, 16, 2.8f));
        Add(20, Mk(TEXT("전체치유"),ESkillType::HealAll, 22, 2.0f));
        Add(26, ElemSkill(Prim, 0, 26));
        Add(32, Mk(TEXT("대치유"), ESkillType::HealOne, 24, 3.4f));
        Add(40, Mk(TEXT("소생"),   ESkillType::Revive,  30, 3.0f));
        Add(48, ElemSkill(Prim, 1, 48));
        Add(56, Mk(TEXT("낙원"),   ESkillType::HealAll, 40, 2.8f));
        Add(66, Mk(TEXT("부활"),   ESkillType::Revive,  48, 5.0f));
        Add(76, ElemSkill(Prim, 5, 76));
        Add(86, ElemSkill(Prim, 4, 86));
        Add(100, ElemSkill(EBattleElement::Almighty, 4, 100));
    }
    else if (bSupport && Best == 0)
    {
        // 순수 서포트(공격 거의 없음): 버프/디버프 풀 + 만능 보조
        Add(6,  Mk(TEXT("기합"), ESkillType::BuffAttack,  14, 1.4f));
        Add(10, Mk(TEXT("수호"), ESkillType::BuffDefense, 14, 1.4f));
        Add(14, Mk(TEXT("약화"), ESkillType::DebuffAttack,12, 0.65f));
        Add(20, ElemSkill(EBattleElement::Almighty, 0, 20));
        Add(26, Mk(TEXT("둔화"), ESkillType::DebuffDefense,12,0.65f));
        Add(34, Mk(TEXT("투지"), ESkillType::BuffAttack,  18, 1.55f));
        Add(42, ElemSkill(EBattleElement::Almighty, 5, 42));
        Add(52, Mk(TEXT("철벽"), ESkillType::BuffDefense, 18, 1.55f));
        Add(62, Mk(TEXT("와해"), ESkillType::DebuffAttack,16, 0.55f));
        Add(74, ElemSkill(EBattleElement::Almighty, 1, 74));
        Add(88, ElemSkill(EBattleElement::Almighty, 6, 88));
        Add(100, ElemSkill(EBattleElement::Almighty, 4, 100));
    }
    else if (bMultiHit || Prim == EBattleElement::Physical)
    {
        // 스트라이커: 다단히트 + 물리 티어 + 보조속성 + 만능 궁극
        Add(6,  ElemSkill(Prim, 1, 6));
        Add(10, Mk(TEXT("연타"),   ESkillType::DamageOne, 14, 0.95f, Prim, 2));
        Add(16, Mk(TEXT("기합"),   ESkillType::BuffAttack, 14, 1.4f));
        Add(22, Mk(TEXT("난무"),   ESkillType::DamageOne, 20, 0.95f, Prim, 3));
        Add(30, ElemSkill(Prim, 2, 30));
        Add(38, ElemSkill(Sec, 1, 38));
        Add(46, Mk(TEXT("폭렬난무"),ESkillType::DamageOne, 30, 1.0f, Prim, 4));
        Add(56, ElemSkill(Prim, 3, 56));
        Add(66, Mk(TEXT("약화"),   ESkillType::DebuffAttack, 16, 0.6f));
        Add(76, ElemSkill(Prim, 6, 76));
        Add(86, ElemSkill(Prim, 4, 86));
        Add(100, ElemSkill(EBattleElement::Almighty, 4, 100));
    }
    else
    {
        // 어태커(마법형): 주속성 티어 풀 + 보조속성 + 전체기 + 만능 궁극
        Add(6,  ElemSkill(Prim, 0, 6));
        Add(10, Mk(TEXT("기합"), ESkillType::BuffAttack, 14, 1.4f));
        Add(14, ElemSkill(Prim, 5, 14));
        Add(18, ElemSkill(Prim, 1, 18));
        Add(24, ElemSkill(Sec, 0, 24));
        Add(30, ElemSkill(Prim, 2, 30));
        Add(38, ElemSkill(EBattleElement::Almighty, 1, 38));
        Add(44, ElemSkill(Prim, 6, 44));
        Add(52, ElemSkill(Sec, 1, 52));
        Add(60, ElemSkill(Prim, 3, 60));
        Add(70, Mk(TEXT("약화"), ESkillType::DebuffDefense, 16, 0.6f));
        Add(78, ElemSkill(Prim, 7, 78));
        Add(86, ElemSkill(EBattleElement::Almighty, 6, 86));
        Add(92, ElemSkill(Prim, 4, 92));
        Add(100, ElemSkill(EBattleElement::Almighty, 4, 100));
    }

    C->LearnableSkills = Tree;
}
