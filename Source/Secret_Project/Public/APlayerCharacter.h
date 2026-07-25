#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ABaseCharacter.h"
#include "CombatComponent.h"
#include "InputAction.h"
#include "APlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UTalkUserWidget;
class UInventoryComponent;
class UQuestComponent;
class UTimeComponent;
class URelationshipComponent;
class UCalendarComponent;
class UWeatherComponent;
class UBankComponent;
class UAchievementComponent;
class USocialStatsComponent;
class UEquipmentComponent;
enum class EDayPhase : uint8;

UCLASS()
class SECRET_PROJECT_API APlayerCharacter : public AABaseCharacter
{
    GENERATED_BODY()

public:
    APlayerCharacter();

    virtual void PostInitializeComponents() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
    class UCapsuleComponent* MyCapsule;

    // 소비아이템 보관함 (상점 구매분 적립 + 세이브 영구화)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<UInventoryComponent> InventoryComp;

    // 퀘스트 추적 (대화/상자열기 목표 + 세이브 영구화)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    TObjectPtr<UQuestComponent> QuestComp;

    // 날짜/시간대 (휴식·이벤트로 진행 + 세이브 영구화)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time")
    TObjectPtr<UTimeComponent> TimeComp;

    // NPC 인연(소셜링크) — 대화 시 호감도 상승 + 세이브 영구화
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Relationship")
    TObjectPtr<URelationshipComponent> RelationComp;

    // 캘린더 이벤트 — 특정 날짜/시간대 축제·보상 (TimeComponent 구독)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Calendar")
    TObjectPtr<UCalendarComponent> CalendarComp;

    // 날씨 — 날짜 바뀌면 변화 (TimeComponent 구독 + 세이브 영구화)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather")
    TObjectPtr<UWeatherComponent> WeatherComp;

    // 은행 — 골드 예치/인출 + 일일 이자 (세이브 영구화)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bank")
    TObjectPtr<UBankComponent> BankComp;

    // 도전과제 — 기존 시스템 데이터 읽어 마일스톤 해금
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Achievement")
    TObjectPtr<UAchievementComponent> AchievementComp;

    // 사회 스탯(지식/매력/용기/친절/숙련) — 활동으로 성장, 자체 세이브 슬롯 영구화
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Social")
    TObjectPtr<USocialStatsComponent> SocialComp;

    // 장비(무기/방어구 등) — 장착 시 StatComponent에 보너스 합산. EquipmentWidget이 이 컴포넌트를 읽음.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
    TObjectPtr<UEquipmentComponent> EquipmentComp;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // ── 카메라 ───────────────────────────────────────────

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> SpringArmComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> CameraComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    int32 ViewMode = 0;

    void ToggleView();
    bool bIsFirstPerson = false;

    UPROPERTY(EditAnywhere, Category = "Camera")
    FVector FirstPersonCameraLocation = FVector(20.0f, 0.0f, 60.0f);

    // ── 이동 ─────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float NormalSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeedMultiplier;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    float SprintSpeed;

    UFUNCTION() void Move(const FInputActionValue& value);
    UFUNCTION() void StartJump(const FInputActionValue& value);
    UFUNCTION() void StopJump(const FInputActionValue& value);
    UFUNCTION() void Look(const FInputActionValue& value);
    UFUNCTION() void StartSprint(const FInputActionValue& value);
    UFUNCTION() void StopSprint(const FInputActionValue& value);

    // ── 상호작용 ─────────────────────────────────────────

    void Interact();

    UPROPERTY(EditAnywhere, Category = "Interaction")
    float InteractDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> ToggleViewAction;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class UTalkUserWidget> DialogueWidgetClass;

    // 시스템 메뉴 위젯 클래스 (BP에서 WBP_SystemMenu 할당) — 레거시 폴백
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class USystemMenuWidget> SystemMenuWidgetClass;

    // ★ 풀스크린 M 메뉴(P5식 사선 와이프 takeover) — 있으면 이게 우선 열림. BP에서 WBP_MenuFlow 할당.
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class UMenuFlowWidget> MenuFlowWidgetClass;

    // 상시 월드 HUD 클래스 (BP에서 WBP_WorldHUD 할당). 비면 표시 안 함(옵트인).
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class UWorldHUDWidget> WorldHUDClass;

    // 생성된 월드 HUD 인스턴스
    UPROPERTY(Transient)
    TObjectPtr<class UWorldHUDWidget> WorldHUD;

public:
    // 상시 월드 HUD 감추기/보이기.
    // 스토리 장면이 도는 동안 옛 기능 시험용 표시(날짜·소지금·퀘스트 알림)가 겹쳐 뜨는 것을 막으려고
    // 진행 담당(ASecretProjectGameMode)이 부른다. 위젯 자체는 그대로 두고 보임만 끈다.
    // 아직 HUD 위젯이 안 만들어졌으면 false를 준다 — 부른 쪽이 다음 프레임에 다시 시도하라는 뜻.
    UFUNCTION(BlueprintCallable, Category = "UI")
    bool SetWorldHudHidden(bool bHide);

protected:

    // 시스템 메뉴 열기 (SystemMenuAction 입력에 바인딩)
    void OpenSystemMenu();

    // T 키 — 목적지 이동 맵(페르소나식 구역 선택) 토글
    void OpenDestinationMap();

    // 시간(날짜) 변경 시 퀘스트 ReachDay 목표 평가
    UFUNCTION() void OnTimeChanged_Handler(int32 Day, EDayPhase Phase);

    // ── 복싱 전투 입력 핸들러 ─────────────────────────────

    UFUNCTION() void OnBoxingLeftJab(const FInputActionValue& Value);
    UFUNCTION() void OnBoxingRightJab(const FInputActionValue& Value);
    UFUNCTION() void OnBoxingLeftHook(const FInputActionValue& Value);
    UFUNCTION() void OnBoxingRightHook(const FInputActionValue& Value);
    UFUNCTION() void OnBoxingUppercut(const FInputActionValue& Value);

    // ── 콤보 윈도우 시스템 ────────────────────────────────

    void ExecuteBoxingInput(EBoxingInput Input);

    UFUNCTION()
    void OnMontageNotifyBegin_Handler(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

    UFUNCTION()
    void OnMontageBlendingOut_Handler(UAnimMontage* Montage, bool bInterrupted);

    EBoxingInput PendingBoxingInput = EBoxingInput::None;
    bool bComboWindowOpen = false;

    // ── CombatComponent 델리게이트 핸들러 ──────────────

    UFUNCTION() void OnCombatStarted_Handler(ECombatMode Mode, ECombatStyle Style);
    UFUNCTION() void OnCombatEnded_Handler();

    void AddCombatIMC();
    void RemoveCombatIMC();
};
