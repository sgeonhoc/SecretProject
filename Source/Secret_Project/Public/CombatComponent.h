#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatStyleBase.h"
#include "CombatComponent.generated.h"

class AABaseCharacter;
class UCombatStyleBase;

// ── 열거형 ──────────────────────────────────────────────

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
    Default UMETA(DisplayName = "Default/Exploration"),
    Combat  UMETA(DisplayName = "Combat")
};

UENUM(BlueprintType)
enum class ECombatMode : uint8
{
    None      UMETA(DisplayName = "None"),
    TurnBased UMETA(DisplayName = "TurnBased"),
    RealTime  UMETA(DisplayName = "RealTime")
};

UENUM(BlueprintType)
enum class ECombatStyle : uint8
{
    None   UMETA(DisplayName = "None"),
    Boxing UMETA(DisplayName = "Boxing")
};

UENUM(BlueprintType)
enum class ETurnState : uint8
{
    Idle       UMETA(DisplayName = "Idle"),
    PlayerTurn UMETA(DisplayName = "PlayerTurn"),
    EnemyTurn  UMETA(DisplayName = "EnemyTurn")
};

// ── 델리게이트 ─────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatStarted, ECombatMode, Mode, ECombatStyle, Style);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerTurnStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyTurnStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboCompleted, FString, SkillName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageEvent, float, Damage, bool, bDealtToPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatTargetDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ── 상태 변수 ────────────────────────────────────────

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    ECharacterState CurrentState = ECharacterState::Default;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    ECombatMode CurrentMode = ECombatMode::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    ECombatStyle CurrentStyle = ECombatStyle::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    ETurnState TurnState = ETurnState::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UCombatStyleBase> ActiveStyle;

    TWeakObjectPtr<AABaseCharacter> CombatTarget;

    // 이번 플레이어 턴에서 입력된 공격 시퀀스
    TArray<EBoxingInput> InputBuffer;

    // 이번 턴 누적 공격력 (기본 공격 합산)
    float AccumulatedAttackDamage = 0.f;

    int32 BasicAttackCount = 0;

    // 이번 적 턴에 방어 중인지
    bool bIsDefendingThisTurn = false;

    FTimerHandle EnemyTurnTimer;
    FTimerHandle RealTimeEnemyTimer;

public:
    // ── 델리게이트 ──────────────────────────────────────

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCombatStarted OnCombatStarted;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCombatEnded OnCombatEnded;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnPlayerTurnStarted OnPlayerTurnStarted;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnEnemyTurnStarted OnEnemyTurnStarted;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnComboCompleted OnComboCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnDamageEvent OnDamageEvent;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCombatTargetDied OnCombatTargetDied;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnPlayerDied OnPlayerDied;

    // ── 외부 호출 함수 ──────────────────────────────────

    // NPC 대화에서 전투 선택 후 호출
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartCombat(ECombatMode Mode, ECombatStyle Style, AABaseCharacter* Target);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void EndCombat();

    // 플레이어 공격 입력 (U/I/J/K/L 키)
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnAttackInput(EBoxingInput Input);

    // 턴제 메뉴 선택 (콤보 없이 순수 스킬 데미지만)
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void UseMenuSkill(int32 SkillIndex);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void UseDefend();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void UseFlee();

    // 게터
    UFUNCTION(BlueprintPure, Category = "Combat")
    FORCEINLINE ECharacterState GetCharacterState() const { return CurrentState; }
    UFUNCTION(BlueprintPure, Category = "Combat")
    FORCEINLINE ECombatMode GetCombatMode() const { return CurrentMode; }
    UFUNCTION(BlueprintPure, Category = "Combat")
    FORCEINLINE ETurnState GetTurnState() const { return TurnState; }
    UFUNCTION(BlueprintPure, Category = "Combat")
    FORCEINLINE bool IsInCombat() const { return CurrentMode != ECombatMode::None; }
    UFUNCTION(BlueprintPure, Category = "Combat")
    UCombatStyleBase* GetActiveStyle() const { return ActiveStyle; }

    // 기존 호환
    UFUNCTION(BlueprintCallable, Category = "State")
    void SetCharacterState(ECharacterState NewState);

private:
    void SetupStyle(ECombatStyle Style);
    void StartPlayerTurn();
    void StartEnemyTurn();
    void ExecuteEnemyAttack();
    void ExecuteRealTimeEnemyAttack();
    void FireComboSkill(int32 ComboIndex);
    void ResetTurnState();
    bool ShouldEndTurnAfterInput() const;
    float CalculateBasicDamage() const;
};
