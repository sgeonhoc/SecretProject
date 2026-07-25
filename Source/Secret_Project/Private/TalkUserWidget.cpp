#include "TalkUserWidget.h"
#include "APlayerCharacter.h"
#include "ANPCCharacter.h"
#include "CombatComponent.h"
#include "StatComponent.h"
#include "BattleManager.h"
#include "ShopWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Components/ProgressBar.h"
#include "TimerManager.h"
#include "Input/Reply.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

// ── 초기화 ───────────────────────────────────────────────

void UTalkUserWidget::NativeConstruct()
{
    Super::NativeConstruct();
    bCloseOnEsc = false;   // 대화창은 ESC로 닫지 않음(자체 흐름/전투 진행 보호)

    // 버튼 클릭 이벤트 바인딩 (없는 버튼은 무시)
    if (Btn_Dialogue)       Btn_Dialogue->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_Dialogue);
    if (Btn_PersonaBattle)  Btn_PersonaBattle->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_PersonaBattle);
    if (Btn_Shop)           Btn_Shop->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_Shop);
    if (Btn_CloseTalk)      Btn_CloseTalk->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_CloseTalk);
    if (Btn_TurnBased)  Btn_TurnBased->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_TurnBased);
    if (Btn_RealTime)   Btn_RealTime->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_RealTime);
    if (Btn_Boxing)     Btn_Boxing->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_Boxing);
    if (Btn_MenuSkill1) Btn_MenuSkill1->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_MenuSkill1);
    if (Btn_MenuSkill2) Btn_MenuSkill2->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_MenuSkill2);
    if (Btn_MenuSkill3) Btn_MenuSkill3->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_MenuSkill3);
    if (Btn_Defend)     Btn_Defend->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_Defend);
    if (Btn_Item)       Btn_Item->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_Item);
    if (Btn_Flee)       Btn_Flee->OnClicked.AddDynamic(this, &UTalkUserWidget::OnClick_Flee);
}

// 상점 열기 — 골드 사용처 (대화창의 상점 버튼)
void UTalkUserWidget::OnClick_Shop()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    if (!ShopWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Shop] ShopWidgetClass 미할당 — WBP_TalkUserWidget의 ShopWidgetClass에 WBP_Shop 지정 필요"));
        return;
    }
    UShopWidget::OpenShop(PC, ShopWidgetClass);
}

// ── NPC 상호작용 진입 (전투 포함) ─────────────────────────

void UTalkUserWidget::StartNPCInteraction(APlayerCharacter* Player, AANPCCharacter* NPC)
{
    OwnerPlayer = Player;
    TargetNPC = NPC;

    if (Player && Player->GetCombatComponent())
    {
        PlayerCombat = Player->GetCombatComponent();
        BindCombatDelegates();
    }

    if (CharacterNameText && NPC)
        CharacterNameText->SetText(FText::FromString(NPC->NPCName));

    // 전투 가능한 NPC → 바로 선택지 표시
    if (NPC && NPC->bCanEnterCombat)
    {
        ShowPanel(EDialogueState::CombatModeSelect);
        if (DialogueText)
            DialogueText->SetText(FText::FromString(TEXT("무슨 일이야?")));
        return;
    }

    // 전투 불가 NPC → 기존 대화 흐름 (시간대별 대사 반영)
    CachedLines = NPC ? NPC->GetContextualLines(Player) : TArray<FString>{TEXT("...")};
    LineIndex = 0;
    ShowPanel(EDialogueState::NormalDialogue);
    if (CachedLines.Num() > 0)
        StartTyping(CachedLines[0]);
}

void UTalkUserWidget::BindCombatDelegates()
{
    if (!PlayerCombat) return;
    PlayerCombat->OnPlayerTurnStarted.AddDynamic(this, &UTalkUserWidget::OnPlayerTurnStarted_Handler);
    PlayerCombat->OnEnemyTurnStarted.AddDynamic(this, &UTalkUserWidget::OnEnemyTurnStarted_Handler);
    PlayerCombat->OnComboCompleted.AddDynamic(this, &UTalkUserWidget::OnComboCompleted_Handler);
    PlayerCombat->OnDamageEvent.AddDynamic(this, &UTalkUserWidget::OnDamage_Handler);
    PlayerCombat->OnCombatTargetDied.AddDynamic(this, &UTalkUserWidget::OnCombatTargetDied_Handler);
    PlayerCombat->OnPlayerDied.AddDynamic(this, &UTalkUserWidget::OnPlayerDied_Handler);
    PlayerCombat->OnCombatEnded.AddDynamic(this, &UTalkUserWidget::OnCombatEnded_Handler);
}

// ── 기존 대화 전용 (호환 유지) ───────────────────────────

void UTalkUserWidget::StartDialogue(FString Name, TArray<FString> Lines)
{
    CachedLines = Lines;
    LineIndex = 0;
    if (CharacterNameText)
        CharacterNameText->SetText(FText::FromString(Name));
    ShowPanel(EDialogueState::NormalDialogue);
    if (CachedLines.Num() > 0)
        StartTyping(CachedLines[0]);
}

// ── 타이핑 효과 ──────────────────────────────────────────

void UTalkUserWidget::StartTyping(FString Content)
{
    FullDialogue = Content;
    CurrentCharIndex = 0;
    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);
    if (DialogueText)
    {
        DialogueText->SetText(FText::GetEmpty());
        GetWorld()->GetTimerManager().SetTimer(
            TypingTimerHandle, this,
            &UTalkUserWidget::DisplayNextChar, 0.05f, true);
    }
}

void UTalkUserWidget::DisplayNextChar()
{
    if (CurrentCharIndex < FullDialogue.Len())
    {
        CurrentCharIndex++;
        if (DialogueText)
            DialogueText->SetText(FText::FromString(FullDialogue.Left(CurrentCharIndex)));
    }
    else
    {
        if (GetWorld())
            GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);
    }
}

// ── 다음 대사 / 대화 종료 ────────────────────────────────

void UTalkUserWidget::ShowNextLine()
{
    LineIndex++;
    if (LineIndex < CachedLines.Num())
    {
        StartTyping(CachedLines[LineIndex]);
    }
    else
    {
        // 모든 대사가 끝남
        if (TargetNPC && TargetNPC->bCanEnterCombat)
        {
            // 전투 가능한 NPC → 모드 선택 화면으로
            ShowPanel(EDialogueState::CombatModeSelect);
            if (DialogueText)
                DialogueText->SetText(FText::FromString(TEXT("무슨 일이야?")));
        }
        else
        {
            FinishDialogue();
        }
    }
}

// ── 패널 전환 ────────────────────────────────────────────

void UTalkUserWidget::HideAllPanels()
{
    auto Hide = [](UWidget* W) { if (W) W->SetVisibility(ESlateVisibility::Collapsed); };
    Hide(Panel_Dialogue);
    Hide(Panel_CombatMode);
    Hide(Panel_StyleSelect);
    Hide(Panel_TurnBased);
    Hide(Panel_HPBars);
}

void UTalkUserWidget::ShowPanel(EDialogueState NewState)
{
    WidgetState = NewState;
    HideAllPanels();

    auto Show = [](UWidget* W) { if (W) W->SetVisibility(ESlateVisibility::Visible); };

    switch (NewState)
    {
    case EDialogueState::NormalDialogue:
        Show(Panel_Dialogue);
        break;
    case EDialogueState::CombatModeSelect:
        Show(Panel_Dialogue);
        Show(Panel_CombatMode);
        break;
    case EDialogueState::StyleSelect:
        Show(Panel_StyleSelect);
        break;
    case EDialogueState::TurnBasedCombat:
        Show(Panel_TurnBased);
        Show(Panel_HPBars);
        UpdateHPBars();
        break;
    default:
        break;
    }
}

// ── 버튼 콜백 ────────────────────────────────────────────

void UTalkUserWidget::OnClick_Dialogue()
{
    FinishDialogue();
}

void UTalkUserWidget::OnClick_PersonaBattle()
{
    if (!OwnerPlayer || !TargetNPC || !GetWorld()) return;

    // 레벨에 배치된 BattleManager 탐색
    ABattleManager* BM = Cast<ABattleManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ABattleManager::StaticClass()));
    if (!BM) return;

    // 플레이어 + 주변 아군 NPC vs talk한 NPC + 주변 적 NPC (자동 분류)
    TArray<AABaseCharacter*> Party = { OwnerPlayer };

    TArray<AABaseCharacter*> Enemies;
    Enemies.Add(TargetNPC);

    const float GroupRadius = 1500.f; // 전투 현장 주변 반경 내 NPC 합류
    TArray<AActor*> AllNPCs;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AANPCCharacter::StaticClass(), AllNPCs);
    for (AActor* A : AllNPCs)
    {
        AANPCCharacter* NPC = Cast<AANPCCharacter>(A);
        if (!NPC || NPC == TargetNPC) continue;
        if (FVector::Dist(NPC->GetActorLocation(), TargetNPC->GetActorLocation()) > GroupRadius) continue;

        if (NPC->bIsAlly)
            Party.Add(NPC);        // 아군으로 참전
        else if (NPC->bCanEnterCombat)
            Enemies.Add(NPC);      // 적으로 참전
    }

    FinishDialogue(); // 대화창 닫기
    BM->StartBattle(Party, Enemies);
}

void UTalkUserWidget::OnClick_CloseTalk()
{
    // 어느 상태에서든 대화 종료(상점 갔다가 돌아온 경우 등 탈출구). 진행 중 레거시 턴제 전투도 함께 종료.
    if (PlayerCombat && WidgetState == EDialogueState::TurnBasedCombat)
        PlayerCombat->UseFlee();
    FinishDialogue();
}

void UTalkUserWidget::OnClick_TurnBased()
{
    PendingCombatMode = ECombatMode::TurnBased;
    ShowPanel(EDialogueState::StyleSelect);
    if (DialogueText)
        DialogueText->SetText(FText::FromString(TEXT("전투 스타일을 선택하세요.")));
}

void UTalkUserWidget::OnClick_RealTime()
{
    PendingCombatMode = ECombatMode::RealTime;
    ShowPanel(EDialogueState::StyleSelect);
    if (DialogueText)
        DialogueText->SetText(FText::FromString(TEXT("전투 스타일을 선택하세요.")));
}

void UTalkUserWidget::OnClick_Boxing()
{
    if (!PlayerCombat || !TargetNPC) return;

    PlayerCombat->StartCombat(
        PendingCombatMode,
        ECombatStyle::Boxing,
        TargetNPC);

    if (PendingCombatMode == ECombatMode::TurnBased)
        ShowPanel(EDialogueState::TurnBasedCombat);
    else
    {
        HideAllPanels();
        if (Panel_HPBars) Panel_HPBars->SetVisibility(ESlateVisibility::Visible);
        UpdateHPBars();
        WidgetState = EDialogueState::TurnBasedCombat;
    }

    // SetInputMode를 이 프레임에 바로 호출하면 Slate가 버튼 클릭 처리 완료 후
    // 포커스를 버튼으로 되돌림 → 다음 프레임에 적용해야 안정적으로 동작
    bool bTurnBased = (PendingCombatMode == ECombatMode::TurnBased);
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,
            [this, bTurnBased]()
            {
                APlayerController* PC = GetOwningPlayer();
                if (!PC) return;
                if (bTurnBased)
                {
                    // 턴제: 마우스로 버튼 클릭해야 하므로 GameAndUI 모드
                    FInputModeGameAndUI InputMode;
                    PC->SetInputMode(InputMode);
                    PC->bShowMouseCursor = true;
                }
                else
                {
                    // 실시간: 게임 입력만 (UIJKL)
                    FInputModeGameOnly InputMode;
                    PC->SetInputMode(InputMode);
                    PC->bShowMouseCursor = false;
                }
            }));
    }
}

// ── 턴제 메뉴 버튼 ───────────────────────────────────────

void UTalkUserWidget::OnClick_MenuSkill1()
{
    if (PlayerCombat) PlayerCombat->UseMenuSkill(0);
}
void UTalkUserWidget::OnClick_MenuSkill2()
{
    if (PlayerCombat) PlayerCombat->UseMenuSkill(1);
}
void UTalkUserWidget::OnClick_MenuSkill3()
{
    if (PlayerCombat) PlayerCombat->UseMenuSkill(2);
}
void UTalkUserWidget::OnClick_Defend()
{
    if (PlayerCombat) PlayerCombat->UseDefend();
}
void UTalkUserWidget::OnClick_Item()
{
    // 아이템 시스템 추후 구현
    if (DialogueText)
        DialogueText->SetText(FText::FromString(TEXT("아이템이 없습니다.")));
}
void UTalkUserWidget::OnClick_Flee()
{
    if (PlayerCombat) PlayerCombat->UseFlee();
}

// ── CombatComponent 이벤트 수신 ──────────────────────────

void UTalkUserWidget::OnPlayerTurnStarted_Handler()
{
    if (TurnInfoText)
        TurnInfoText->SetText(FText::FromString(TEXT("플레이어 턴")));
    UpdateHPBars();
}

void UTalkUserWidget::OnEnemyTurnStarted_Handler()
{
    if (TurnInfoText)
        TurnInfoText->SetText(FText::FromString(TEXT("적의 턴")));
}

void UTalkUserWidget::OnComboCompleted_Handler(FString SkillName)
{
    if (DialogueText)
        DialogueText->SetText(FText::FromString(FString::Printf(TEXT("콤보! %s"), *SkillName)));
}

void UTalkUserWidget::OnDamage_Handler(float Damage, bool bToPlayer)
{
    UpdateHPBars();
    if (DialogueText)
    {
        FString Msg = bToPlayer
            ? FString::Printf(TEXT("-%0.0f 피격"), Damage)
            : FString::Printf(TEXT("-%0.0f 데미지"), Damage);
        DialogueText->SetText(FText::FromString(Msg));
    }
}

void UTalkUserWidget::OnCombatTargetDied_Handler()
{
    if (DialogueText)
        DialogueText->SetText(FText::FromString(TEXT("승리!")));
}

void UTalkUserWidget::OnPlayerDied_Handler()
{
    if (DialogueText)
        DialogueText->SetText(FText::FromString(TEXT("패배...")));
}

void UTalkUserWidget::OnCombatEnded_Handler()
{
    FinishDialogue();
}

// ── HP바 업데이트 ─────────────────────────────────────────

void UTalkUserWidget::UpdateHPBars()
{
    if (OwnerPlayer && OwnerPlayer->GetStatComponent())
    {
        UStatComponent* PS = OwnerPlayer->GetStatComponent();
        if (PlayerHPBar)
            PlayerHPBar->SetPercent(PS->GetCurrentHP() / PS->GetMaxHP());
    }
    if (TargetNPC && TargetNPC->GetStatComponent())
    {
        UStatComponent* ES = TargetNPC->GetStatComponent();
        if (EnemyHPBar)
            EnemyHPBar->SetPercent(ES->GetCurrentHP() / ES->GetMaxHP());
    }
}

// ── 종료 ─────────────────────────────────────────────────

void UTalkUserWidget::FinishDialogue()
{
    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);

    RemoveFromParent();
    // 남은 위젯(메뉴 등)에 맞춰 입력모드 재조정. 남은 게 없으면 게임모드.
    UPersonaWidgetBase::RefreshInputMode(this);
}

// ── 마우스 클릭 (타이핑 스킵 / 다음 줄) ─────────────────

FReply UTalkUserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // 전투 UI 상태에서는 마우스 클릭으로 대사 넘기지 않음 (버튼이 처리)
        if (WidgetState == EDialogueState::TurnBasedCombat ||
            WidgetState == EDialogueState::CombatModeSelect ||
            WidgetState == EDialogueState::StyleSelect)
        {
            return FReply::Unhandled();
        }

        if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(TypingTimerHandle))
        {
            GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);
            if (DialogueText)
                DialogueText->SetText(FText::FromString(FullDialogue));
            CurrentCharIndex = FullDialogue.Len();
        }
        else
        {
            ShowNextLine();
        }
        return FReply::Handled();
    }
    return FReply::Unhandled();
}
