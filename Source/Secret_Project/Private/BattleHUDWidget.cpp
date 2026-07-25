#include "BattleHUDWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "UIRuntime.h"

void UBattleHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    bCloseOnEsc = false;   // 전투HUD는 ESC로 닫지 않음

    // 모든 버튼 클릭을 C++에서 바인딩 (BP 그래프 불필요)
    if (Btn_Attack) Btn_Attack->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnAttackClicked);
    if (Btn_Skill)  Btn_Skill->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnSkillClicked);
    if (Btn_Guard)  Btn_Guard->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnGuardClicked);
    if (Btn_Charge) Btn_Charge->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnChargeClicked);
    if (Btn_Escape) Btn_Escape->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnEscapeClicked);

    if (Btn_SkillOpt0) Btn_SkillOpt0->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnSkillOpt0Clicked);
    if (Btn_SkillOpt1) Btn_SkillOpt1->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnSkillOpt1Clicked);
    if (Btn_SkillOpt2) Btn_SkillOpt2->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnSkillOpt2Clicked);
    if (Btn_SkillOpt3) Btn_SkillOpt3->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnSkillOpt3Clicked);
    if (Btn_SkillOpt4) Btn_SkillOpt4->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnSkillOpt4Clicked);
    if (Btn_SkillOpt5) Btn_SkillOpt5->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnSkillOpt5Clicked);

    if (Btn_TargetOpt0) Btn_TargetOpt0->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnTargetOpt0Clicked);
    if (Btn_TargetOpt1) Btn_TargetOpt1->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnTargetOpt1Clicked);
    if (Btn_TargetOpt2) Btn_TargetOpt2->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnTargetOpt2Clicked);
    if (Btn_TargetOpt3) Btn_TargetOpt3->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnTargetOpt3Clicked);

    if (Btn_AllyOpt0) Btn_AllyOpt0->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnAllyOpt0Clicked);
    if (Btn_AllyOpt1) Btn_AllyOpt1->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnAllyOpt1Clicked);
    if (Btn_AllyOpt2) Btn_AllyOpt2->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnAllyOpt2Clicked);
    if (Btn_AllyOpt3) Btn_AllyOpt3->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnAllyOpt3Clicked);

    if (Btn_AllOut) Btn_AllOut->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnAllOutClicked);
    if (Btn_Baton)  Btn_Baton->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnBatonClicked);

    if (Btn_Item)     Btn_Item->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnItemClicked);
    if (Btn_ItemOpt0) Btn_ItemOpt0->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnItemOpt0Clicked);
    if (Btn_ItemOpt1) Btn_ItemOpt1->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnItemOpt1Clicked);
    if (Btn_ItemOpt2) Btn_ItemOpt2->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnItemOpt2Clicked);
    if (Btn_ItemOpt3) Btn_ItemOpt3->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnItemOpt3Clicked);
    if (Btn_ItemOpt4) Btn_ItemOpt4->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnItemOpt4Clicked);
    if (Btn_ItemOpt5) Btn_ItemOpt5->OnClicked.AddDynamic(this, &UBattleHUDWidget::OnItemOpt5Clicked);

    CloseSkillMenu();
    CloseItemMenu();
}

void UBattleHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 모던 HUD: 게이지 카드(List_Party/Enemies) — 상태 변할 때만 재생성(매 프레임 재생성 방지).
    if ((List_Party || List_Enemies) && BattleManagerRef)
    {
        const FString Sig = BattleManagerRef->GetBattleStatusText().ToString(); // 모든 상태 인코딩 = 변경 감지 시그니처
        if (Sig != LastCardSig)
        {
            LastCardSig = Sig;
            RebuildBattlerCards();
        }
    }
    // 레거시 폴백: 카드 컨테이너가 없을 때만 텍스트 덤프 사용
    else if (Txt_Status && BattleManagerRef)
        Txt_Status->SetText(BattleManagerRef->GetBattleStatusText());

    // 턴 순서 미리보기 (옵트인 텍스트블록)
    if (Txt_TurnOrder && BattleManagerRef)
        Txt_TurnOrder->SetText(BattleManagerRef->GetTurnOrderPreview());

    // 타겟 버튼 라벨/표시 실시간 갱신
    RefreshTargets();
    RefreshAllies();

    // 총공격 버튼: 모든 적 다운 시에만 표시
    if (Btn_AllOut && BattleManagerRef)
        Btn_AllOut->SetVisibility(BattleManagerRef->GetIsAllOutReady()
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    // 바톤 터치 버튼: One More 중 + 넘길 아군 있을 때만 표시
    if (Btn_Baton && BattleManagerRef)
        Btn_Baton->SetVisibility(BattleManagerRef->GetCanBatonPass()
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UBattleHUDWidget::SetBattleManager_Implementation(ABattleManager* Manager)
{
    BattleManagerRef = Manager;
    if (Manager)
    {
        Manager->OnTurnStarted.RemoveDynamic(this, &UBattleHUDWidget::OnTurnStarted_Handler);
        Manager->OnTurnStarted.AddDynamic(this, &UBattleHUDWidget::OnTurnStarted_Handler);

        Manager->OnBattleFlair.RemoveDynamic(this, &UBattleHUDWidget::HandleBattleFlair);
        Manager->OnBattleFlair.AddDynamic(this, &UBattleHUDWidget::HandleBattleFlair);
    }
}

void UBattleHUDWidget::HandleBattleFlair(EBattleFlair Kind, const FString& Label)
{
    // C++ 연출 신호 → BP 디자이너 구현(OnFlair)로 전달
    OnFlair(Kind, FText::FromString(Label));
}

void UBattleHUDWidget::OnTurnStarted_Handler(AABaseCharacter* /*CurrentActor*/, bool bIsPlayerTurn)
{
    CloseSkillMenu();
    CloseItemMenu();

    // 플레이어 턴에만 버튼 활성화
    if (Btn_Attack) Btn_Attack->SetIsEnabled(bIsPlayerTurn);
    if (Btn_Skill)  Btn_Skill->SetIsEnabled(bIsPlayerTurn);
    if (Btn_Guard)  Btn_Guard->SetIsEnabled(bIsPlayerTurn);
    if (Btn_Charge) Btn_Charge->SetIsEnabled(bIsPlayerTurn);
    if (Btn_Item)   Btn_Item->SetIsEnabled(bIsPlayerTurn);
    if (Btn_Escape) Btn_Escape->SetIsEnabled(bIsPlayerTurn);
}

// ── 메인 버튼 ────────────────────────────────────────────

void UBattleHUDWidget::OnAttackClicked()
{
    if (BattleManagerRef) BattleManagerRef->PlayerAttack();
    CloseSkillMenu();
}

void UBattleHUDWidget::OnGuardClicked()
{
    if (BattleManagerRef) BattleManagerRef->PlayerDefend();
    CloseSkillMenu();
}

void UBattleHUDWidget::OnChargeClicked()
{
    if (BattleManagerRef) BattleManagerRef->PlayerCharge();
    CloseSkillMenu();
}

void UBattleHUDWidget::OnEscapeClicked()
{
    if (BattleManagerRef) BattleManagerRef->PlayerFlee();
    CloseSkillMenu();
}

void UBattleHUDWidget::OnSkillClicked()
{
    OpenSkillMenu();
}

void UBattleHUDWidget::OnItemClicked()
{
    OpenItemMenu();
}

// ── 스킬 옵션 버튼 ───────────────────────────────────────

void UBattleHUDWidget::OnSkillOpt0Clicked() { UseSkillAndClose(0); }
void UBattleHUDWidget::OnSkillOpt1Clicked() { UseSkillAndClose(1); }
void UBattleHUDWidget::OnSkillOpt2Clicked() { UseSkillAndClose(2); }
void UBattleHUDWidget::OnSkillOpt3Clicked() { UseSkillAndClose(3); }
void UBattleHUDWidget::OnSkillOpt4Clicked() { UseSkillAndClose(4); }
void UBattleHUDWidget::OnSkillOpt5Clicked() { UseSkillAndClose(5); }

void UBattleHUDWidget::OnItemOpt0Clicked() { UseItemAndClose(0); }
void UBattleHUDWidget::OnItemOpt1Clicked() { UseItemAndClose(1); }
void UBattleHUDWidget::OnItemOpt2Clicked() { UseItemAndClose(2); }
void UBattleHUDWidget::OnItemOpt3Clicked() { UseItemAndClose(3); }
void UBattleHUDWidget::OnItemOpt4Clicked() { UseItemAndClose(4); }
void UBattleHUDWidget::OnItemOpt5Clicked() { UseItemAndClose(5); }

// ── 타겟 선택 ────────────────────────────────────────────

void UBattleHUDWidget::OnTargetOpt0Clicked() { if (BattleManagerRef) BattleManagerRef->SetEnemyTarget(0); }
void UBattleHUDWidget::OnTargetOpt1Clicked() { if (BattleManagerRef) BattleManagerRef->SetEnemyTarget(1); }
void UBattleHUDWidget::OnTargetOpt2Clicked() { if (BattleManagerRef) BattleManagerRef->SetEnemyTarget(2); }
void UBattleHUDWidget::OnTargetOpt3Clicked() { if (BattleManagerRef) BattleManagerRef->SetEnemyTarget(3); }

void UBattleHUDWidget::OnAllyOpt0Clicked() { if (BattleManagerRef) BattleManagerRef->SetAllyTarget(0); }
void UBattleHUDWidget::OnAllyOpt1Clicked() { if (BattleManagerRef) BattleManagerRef->SetAllyTarget(1); }
void UBattleHUDWidget::OnAllyOpt2Clicked() { if (BattleManagerRef) BattleManagerRef->SetAllyTarget(2); }
void UBattleHUDWidget::OnAllyOpt3Clicked() { if (BattleManagerRef) BattleManagerRef->SetAllyTarget(3); }

void UBattleHUDWidget::OnAllOutClicked()
{
    if (BattleManagerRef) BattleManagerRef->PlayerAllOutAttack();
    CloseSkillMenu();
}

void UBattleHUDWidget::OnBatonClicked()
{
    if (BattleManagerRef) BattleManagerRef->PlayerBatonPass();
    CloseSkillMenu();
    CloseItemMenu();
}

void UBattleHUDWidget::RefreshTargets()
{
    // 중앙 구식 선택 패널 폐기 — 이제 적 게이지 카드를 직접 클릭해 타겟 지정(OnEnemyCardClicked).
    // 레거시 WBP에 패널이 남아있어도 항상 숨김.
    if (Panel_Targets) Panel_Targets->SetVisibility(ESlateVisibility::Collapsed);
}

void UBattleHUDWidget::RefreshAllies()
{
    // 중앙 구식 선택 패널 폐기 — 이제 아군 게이지 카드를 직접 클릭해 힐/아이템 대상 지정(OnAllyCardClicked).
    if (Panel_Allies) Panel_Allies->SetVisibility(ESlateVisibility::Collapsed);
}

// ── 모던 HUD: 게이지 카드 동적 재생성 ──────────────────────
// 클릭 가능 카드 반환(타겟 지정용). 죽은 대상은 버튼 비활성.
static UCardButton* BuildBattlerCard(UVerticalBox* List, const FBattlerView& V, bool bEnemy, int32 Index)
{
    if (!List || !V.bValid) return nullptr;

    // 페르소나 톤 카드 배경: 행동중=골드 글로우, 조준중=크림슨, 사망=흐림, 평상=다크네이비 반투명
    FLinearColor Bg = FLinearColor(0.06f, 0.07f, 0.11f, 0.80f);
    if (V.bDead)          Bg = FLinearColor(0.07f, 0.07f, 0.09f, 0.66f);
    else if (V.bActive)   Bg = FLinearColor(0.24f, 0.18f, 0.05f, 0.94f); // 행동 중(골드)
    else if (V.bTargeted) Bg = FLinearColor(0.28f, 0.06f, 0.09f, 0.94f); // 조준 중(크림슨)

    UCardButton* Btn = nullptr;
    UVerticalBox* In = UUIRuntime::AddClickCard(List, Bg, Index, Btn, 4.f);
    if (!In) return nullptr;
    if (Btn && V.bDead) Btn->SetIsEnabled(false); // 죽은 대상은 클릭 불가

    // 1줄: ▶이름(좌) ── HP 수치(우). 컴팩트 — 별도 HP 텍스트 줄 제거.
    UHorizontalBox* Row = UUIRuntime::AddRow(In, 2.f);
    const FString NameStr = FString::Printf(TEXT("%s%s"), V.bActive ? TEXT("▶ ") : TEXT(""), *V.Name);
    UUIRuntime::RowText(Row, NameStr, V.bDead ? UIColor::Dim : (V.bActive ? UIColor::Accent : UIColor::Title),
                        15, ETextJustify::Left, true);
    if (V.bDead)
    {
        UUIRuntime::RowText(Row, TEXT("쓰러짐"), UIColor::Dim, 12, ETextJustify::Right, false);
        return Btn;
    }
    UUIRuntime::RowText(Row, FString::Printf(TEXT("%.0f/%.0f"), V.HP, V.MaxHP), UIColor::Sub, 11, ETextJustify::Right, false);

    // HP 바 (얇게, 비율색: 위험=빨강 / 주의=노랑 / 양호=초록)
    const float HpPct = V.HP / V.MaxHP;
    const FLinearColor HpCol = (HpPct < 0.3f) ? FLinearColor(0.92f, 0.27f, 0.27f, 1.f)
                            : (HpPct < 0.6f)  ? FLinearColor(0.93f, 0.78f, 0.28f, 1.f)
                                              : FLinearColor(0.32f, 0.82f, 0.42f, 1.f);
    UUIRuntime::AddBar(In, HpPct, HpCol, 8.f, (V.MaxSP > 0.f || !V.Tags.IsEmpty()) ? 2.f : 1.f);

    // 약점/상태 태그 (있을 때만, 작게)
    if (!V.Tags.IsEmpty())
        UUIRuntime::AddText(In, V.Tags, bEnemy ? UIColor::Accent : UIColor::Sub, 11, ETextJustify::Left, V.MaxSP > 0.f ? 2.f : 1.f);

    // SP 바 (아군만, 더 얇게)
    if (V.MaxSP > 0.f)
        UUIRuntime::AddBar(In, V.SP / V.MaxSP, FLinearColor(0.35f, 0.62f, 0.95f, 1.f), 5.f, 1.f);

    return Btn;
}

void UBattleHUDWidget::RebuildBattlerCards()
{
    if (!BattleManagerRef) return;

    if (List_Party)
    {
        UUIRuntime::Clear(List_Party);
        const int32 N = BattleManagerRef->GetAllyCount();
        for (int32 i = 0; i < N; ++i)
            if (UCardButton* B = BuildBattlerCard(List_Party, BattleManagerRef->GetAllyView(i), false, i))
                B->OnCardClicked.AddDynamic(this, &UBattleHUDWidget::OnAllyCardClicked);
    }
    if (List_Enemies)
    {
        UUIRuntime::Clear(List_Enemies);
        const int32 N = BattleManagerRef->GetEnemyCount();
        for (int32 i = 0; i < N; ++i)
            if (UCardButton* B = BuildBattlerCard(List_Enemies, BattleManagerRef->GetEnemyView(i), true, i))
                B->OnCardClicked.AddDynamic(this, &UBattleHUDWidget::OnEnemyCardClicked);
    }
}

// 적/아군 카드 직접 클릭 → 타겟 지정 (중앙 선택 패널을 대체하는 모던 방식)
void UBattleHUDWidget::OnEnemyCardClicked(int32 Index)
{
    if (BattleManagerRef) BattleManagerRef->SetEnemyTarget(Index);
    PlayUI(TEXT("ui_click"), 0.5f);
}
void UBattleHUDWidget::OnAllyCardClicked(int32 Index)
{
    if (BattleManagerRef) BattleManagerRef->SetAllyTarget(Index);
    PlayUI(TEXT("ui_click"), 0.5f);
}

void UBattleHUDWidget::UseSkillAndClose(int32 Index)
{
    if (BattleManagerRef) BattleManagerRef->PlayerUseSkill(Index);
    CloseSkillMenu();
}

// ── 스킬 메뉴 열기/닫기 ──────────────────────────────────

void UBattleHUDWidget::OpenSkillMenu()
{
    if (!BattleManagerRef) return;

    CloseItemMenu(); // 스킬·아이템 메뉴는 동시에 안 뜸

    const int32 Count = BattleManagerRef->GetCurrentSkillCount();

    auto SetupOption = [&](UButton* Btn, UTextBlock* Txt, int32 Idx)
    {
        if (!Btn) return;
        if (Idx < Count)
        {
            Btn->SetVisibility(ESlateVisibility::Visible);
            const FText Label = BattleManagerRef->GetSkillLabel(Idx);
            if (Txt) Txt->SetText(Label);
            // SP 부족 스킬은 비활성(회색) — GetSkillLabel이 "(SP부족)" 표기를 붙여줌(단일 소스).
            // 표시는 유지해 SP 모이면 무엇을 쓸 수 있는지 보이게 함(페르소나식).
            Btn->SetIsEnabled(!Label.ToString().Contains(TEXT("(SP부족)")));
        }
        else
        {
            Btn->SetVisibility(ESlateVisibility::Collapsed);
        }
    };

    SetupOption(Btn_SkillOpt0, Txt_SkillOpt0, 0);
    SetupOption(Btn_SkillOpt1, Txt_SkillOpt1, 1);
    SetupOption(Btn_SkillOpt2, Txt_SkillOpt2, 2);
    SetupOption(Btn_SkillOpt3, Txt_SkillOpt3, 3);
    SetupOption(Btn_SkillOpt4, Txt_SkillOpt4, 4);
    SetupOption(Btn_SkillOpt5, Txt_SkillOpt5, 5);

    if (Panel_Skills) Panel_Skills->SetVisibility(ESlateVisibility::Visible);
}

void UBattleHUDWidget::CloseSkillMenu()
{
    if (Panel_Skills) Panel_Skills->SetVisibility(ESlateVisibility::Collapsed);
}

// ── 아이템 메뉴 열기/닫기 ────────────────────────────────

void UBattleHUDWidget::OpenItemMenu()
{
    if (!BattleManagerRef) return;

    CloseSkillMenu(); // 스킬·아이템 메뉴는 동시에 안 뜸

    const int32 Count = BattleManagerRef->GetItemCount();

    auto SetupOption = [&](UButton* Btn, UTextBlock* Txt, int32 Idx)
    {
        if (!Btn) return;
        if (Idx < Count)
        {
            Btn->SetVisibility(ESlateVisibility::Visible);
            if (Txt) Txt->SetText(BattleManagerRef->GetItemLabel(Idx));
        }
        else
        {
            Btn->SetVisibility(ESlateVisibility::Collapsed);
        }
    };

    SetupOption(Btn_ItemOpt0, Txt_ItemOpt0, 0);
    SetupOption(Btn_ItemOpt1, Txt_ItemOpt1, 1);
    SetupOption(Btn_ItemOpt2, Txt_ItemOpt2, 2);
    SetupOption(Btn_ItemOpt3, Txt_ItemOpt3, 3);
    SetupOption(Btn_ItemOpt4, Txt_ItemOpt4, 4);
    SetupOption(Btn_ItemOpt5, Txt_ItemOpt5, 5);

    if (Panel_Items) Panel_Items->SetVisibility(ESlateVisibility::Visible);
}

void UBattleHUDWidget::CloseItemMenu()
{
    if (Panel_Items) Panel_Items->SetVisibility(ESlateVisibility::Collapsed);
}

void UBattleHUDWidget::UseItemAndClose(int32 Index)
{
    if (BattleManagerRef) BattleManagerRef->PlayerUseItem(Index);
    CloseItemMenu();
}
