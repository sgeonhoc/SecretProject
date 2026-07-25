#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "CombatComponent.h"
#include "TalkUserWidget.generated.h"

class UTextBlock;
class UButton;
class UWidget;
class UProgressBar;
class APlayerCharacter;
class AANPCCharacter;
class UCombatComponent;
class ABattleManager;
class UShopWidget;

UENUM()
enum class EDialogueState : uint8
{
    Hidden,
    NormalDialogue,
    CombatModeSelect,
    StyleSelect,
    TurnBasedCombat,
    CombatResult,
    PartyBattle
};

UCLASS()
class SECRET_PROJECT_API UTalkUserWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

protected:
    // ── 필수 BindWidget (BP에서 이름 반드시 일치) ──────────

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DialogueText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CharacterNameText;

    // ── 선택 BindWidget (BP에서 이름 일치 시 자동 연결) ────
    // 없어도 컴파일 오류 없음. 전투 UI 추가할 때 하나씩 만들어서 연결하면 됨

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_Dialogue;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_CombatMode;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_StyleSelect;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_TurnBased;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Panel_HPBars;

    // 모드 선택
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Dialogue;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_TurnBased;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_RealTime;

    // 페르소나 파티 전투
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_PersonaBattle;

    // 상점 (골드 사용처) — 클릭 시 ShopWidgetClass 위젯 열기
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Shop;

    // 대화창 닫기(항상 표시) — 어느 상태에서든 대화 종료. 직관적 탈출.
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_CloseTalk;

    // 열 상점 위젯 클래스 (BP에서 WBP_Shop 할당)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    TSubclassOf<UShopWidget> ShopWidgetClass;

    // 스타일 선택
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Boxing;

    // 턴제 메뉴
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_MenuSkill1;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_MenuSkill2;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_MenuSkill3;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Defend;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Item;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Flee;

    // 상태 표시
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TurnInfoText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> PlayerHPBar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> EnemyHPBar;

    // ── 내부 상태 ────────────────────────────────────────

    FTimerHandle TypingTimerHandle;
    FString FullDialogue;
    int32 CurrentCharIndex = 0;
    TArray<FString> CachedLines;
    int32 LineIndex = 0;

    EDialogueState WidgetState = EDialogueState::Hidden;
    ECombatMode PendingCombatMode = ECombatMode::None;

    UPROPERTY()
    TObjectPtr<APlayerCharacter> OwnerPlayer;

    UPROPERTY()
    TObjectPtr<AANPCCharacter> TargetNPC;

    UPROPERTY()
    TObjectPtr<UCombatComponent> PlayerCombat;

    // ── 내부 함수 ────────────────────────────────────────

    void DisplayNextChar();
    void StartTyping(FString Content);
    void ShowPanel(EDialogueState NewState);
    void HideAllPanels();
    void UpdateHPBars();
    void BindCombatDelegates();

    UFUNCTION() void OnClick_Dialogue();
    UFUNCTION() void OnClick_PersonaBattle();
    UFUNCTION() void OnClick_Shop();
    UFUNCTION() void OnClick_CloseTalk();
    UFUNCTION() void OnClick_TurnBased();
    UFUNCTION() void OnClick_RealTime();
    UFUNCTION() void OnClick_Boxing();
    UFUNCTION() void OnClick_MenuSkill1();
    UFUNCTION() void OnClick_MenuSkill2();
    UFUNCTION() void OnClick_MenuSkill3();
    UFUNCTION() void OnClick_Defend();
    UFUNCTION() void OnClick_Item();
    UFUNCTION() void OnClick_Flee();

    UFUNCTION() void OnPlayerTurnStarted_Handler();
    UFUNCTION() void OnEnemyTurnStarted_Handler();
    UFUNCTION() void OnComboCompleted_Handler(FString SkillName);
    UFUNCTION() void OnDamage_Handler(float Damage, bool bToPlayer);
    UFUNCTION() void OnCombatTargetDied_Handler();
    UFUNCTION() void OnPlayerDied_Handler();
    UFUNCTION() void OnCombatEnded_Handler();

public:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // 기존 대화 전용 진입점 (호환 유지)
    void StartDialogue(FString Name, TArray<FString> Lines);

    // NPC 상호작용 진입점 (전투 선택지 포함)
    void StartNPCInteraction(APlayerCharacter* Player, AANPCCharacter* NPC);

    void ShowNextLine();
    void FinishDialogue();
};
