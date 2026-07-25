#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "BattleManager.h"
#include "BattleHUDWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;
class AABaseCharacter;

UCLASS()
class SECRET_PROJECT_API UBattleHUDWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    // BattleManager가 HUD 생성 직후 호출 (CreateHUD에서)
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Battle")
    void SetBattleManager(ABattleManager* Manager);
    virtual void SetBattleManager_Implementation(ABattleManager* Manager);

    // 연출 이벤트 — 디자이너가 BP에서 구현(WEAK!/CRITICAL!/1 MORE! 등 페르소나식 팝업·사운드 재생)
    UFUNCTION(BlueprintImplementableEvent, Category = "Battle|Flair")
    void OnFlair(EBattleFlair Kind, const FText& Label);

    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    TObjectPtr<ABattleManager> BattleManagerRef;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ── 메인 액션 버튼 (BP 위젯 이름과 정확히 일치해야 바인딩됨) ──
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Attack;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Skill;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Guard;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Charge;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Item;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Escape;

    // 총공격 버튼 (모든 적 다운 시에만 표시)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_AllOut;

    // 바톤 터치 버튼 (One More 중 + 아군 선택 가능 시에만 표시)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Baton;

    // ── 상태 텍스트 (레거시 폴백 — List_Party/Enemies 있으면 미사용) ──
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Status;

    // ── 모던 HUD: C++가 게이지 카드를 동적 생성하는 컨테이너 ──
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<class UVerticalBox> List_Party;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<class UVerticalBox> List_Enemies;

    // 턴 순서 미리보기 (옵트인 — 만들면 자동 표시)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_TurnOrder;

    // ── 스킬 선택 서브메뉴 (최대 6개 슬롯, 만든 버튼만 바인딩됨) ──
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> Panel_Skills;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_SkillOpt0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_SkillOpt1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_SkillOpt2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_SkillOpt3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_SkillOpt4;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_SkillOpt5;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_SkillOpt0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_SkillOpt1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_SkillOpt2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_SkillOpt3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_SkillOpt4;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_SkillOpt5;

    // ── 아이템 선택 서브메뉴 (보유 소비아이템, 최대 6슬롯) ──
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> Panel_Items;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_ItemOpt0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_ItemOpt1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_ItemOpt2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_ItemOpt3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_ItemOpt4;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_ItemOpt5;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_ItemOpt0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_ItemOpt1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_ItemOpt2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_ItemOpt3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_ItemOpt4;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_ItemOpt5;

    // ── 타겟 선택 (적 2명 이상일 때만 표시, 최대 4슬롯) ──
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> Panel_Targets;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_TargetOpt0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_TargetOpt1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_TargetOpt2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_TargetOpt3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_TargetOpt0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_TargetOpt1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_TargetOpt2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_TargetOpt3;

    // ── 아군 타겟 선택 (파티 2명 이상일 때만 표시, 힐/아이템 대상, 최대 4슬롯) ──
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> Panel_Allies;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_AllyOpt0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_AllyOpt1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_AllyOpt2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_AllyOpt3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_AllyOpt0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_AllyOpt1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_AllyOpt2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_AllyOpt3;

private:
    UFUNCTION() void OnAttackClicked();
    UFUNCTION() void OnSkillClicked();
    UFUNCTION() void OnGuardClicked();
    UFUNCTION() void OnChargeClicked();
    UFUNCTION() void OnEscapeClicked();
    UFUNCTION() void OnSkillOpt0Clicked();
    UFUNCTION() void OnSkillOpt1Clicked();
    UFUNCTION() void OnSkillOpt2Clicked();
    UFUNCTION() void OnSkillOpt3Clicked();
    UFUNCTION() void OnSkillOpt4Clicked();
    UFUNCTION() void OnSkillOpt5Clicked();

    UFUNCTION() void OnTargetOpt0Clicked();
    UFUNCTION() void OnTargetOpt1Clicked();
    UFUNCTION() void OnTargetOpt2Clicked();
    UFUNCTION() void OnTargetOpt3Clicked();

    UFUNCTION() void OnAllyOpt0Clicked();
    UFUNCTION() void OnAllyOpt1Clicked();
    UFUNCTION() void OnAllyOpt2Clicked();
    UFUNCTION() void OnAllyOpt3Clicked();

    UFUNCTION() void OnAllOutClicked();
    UFUNCTION() void OnBatonClicked();

    UFUNCTION() void OnItemClicked();
    UFUNCTION() void OnItemOpt0Clicked();
    UFUNCTION() void OnItemOpt1Clicked();
    UFUNCTION() void OnItemOpt2Clicked();
    UFUNCTION() void OnItemOpt3Clicked();
    UFUNCTION() void OnItemOpt4Clicked();
    UFUNCTION() void OnItemOpt5Clicked();

    UFUNCTION() void OnTurnStarted_Handler(AABaseCharacter* CurrentActor, bool bIsPlayerTurn);
    UFUNCTION() void HandleBattleFlair(EBattleFlair Kind, const FString& Label);

    // 모던 타겟팅: 적/아군 게이지 카드를 직접 클릭 → 타겟 지정 (중앙 선택 패널 대체)
    UFUNCTION() void OnEnemyCardClicked(int32 Index);
    UFUNCTION() void OnAllyCardClicked(int32 Index);

    void OpenSkillMenu();
    void CloseSkillMenu();
    void UseSkillAndClose(int32 Index);
    void OpenItemMenu();
    void CloseItemMenu();
    void UseItemAndClose(int32 Index);
    void RefreshTargets();
    void RefreshAllies();

    // 모던 HUD: 아군/적 게이지 카드를 동적 재생성 (상태 변할 때만). LastCardSig로 변경 감지.
    void RebuildBattlerCards();
    FString LastCardSig;
};
