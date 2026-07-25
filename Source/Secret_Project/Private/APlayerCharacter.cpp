#include "APlayerCharacter.h"
#include "ANPCCharacter.h"
#include "TalkUserWidget.h"
#include "SecretProjectPlayerController.h"
#include "CombatComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "StatComponent.h"
#include "SecretSaveGame.h"
#include "InventoryComponent.h"
#include "QuestComponent.h"
#include "EquipmentComponent.h"
#include "TimeComponent.h"
#include "RelationshipComponent.h"
#include "CalendarComponent.h"
#include "WeatherComponent.h"
#include "BankComponent.h"
#include "AchievementComponent.h"
#include "SocialStatsComponent.h"
#include "NPCArchetype.h"
#include "WorldHUDWidget.h"
#include "TreasureChest.h"
#include "LoreNoteActor.h"
#include "LockedGateActor.h"
#include "HarvestNodeActor.h"
#include "WishingWellActor.h"
#include "SystemMenuWidget.h"
#include "MenuFlowWidget.h"
#include "DestinationMenu.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ShopWidget.h"



APlayerCharacter::APlayerCharacter()
{
    // Tick 함수는 우선 꺼둡니다.
    PrimaryActorTick.bCanEverTick = false;

    // (1) 스프링 암 생성
    SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    // 스프링 암을 루트 컴포넌트 (CapsuleComponent)에 부착
    SpringArmComp->SetupAttachment(RootComponent);
    // 캐릭터와 카메라 사이의 거리 기본값 300으로 설정
    SpringArmComp->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
    // 컨트롤러 회전에 따라 스프링 암도 회전하도록 설정
    SpringArmComp->TargetArmLength = 300.0f;
    SpringArmComp->bUsePawnControlRotation = true;

    // (1-b) 소비아이템 보관함
    InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

    // (1-c) 퀘스트 추적
    QuestComp = CreateDefaultSubobject<UQuestComponent>(TEXT("Quest"));

    // (1-d) 날짜/시간대
    TimeComp = CreateDefaultSubobject<UTimeComponent>(TEXT("Time"));

    // (1-e) NPC 인연(소셜링크)
    RelationComp = CreateDefaultSubobject<URelationshipComponent>(TEXT("Relationship"));

    // (1-f) 캘린더 이벤트
    CalendarComp = CreateDefaultSubobject<UCalendarComponent>(TEXT("Calendar"));

    // (1-g) 날씨
    WeatherComp = CreateDefaultSubobject<UWeatherComponent>(TEXT("Weather"));

    // (1-h) 은행
    BankComp = CreateDefaultSubobject<UBankComponent>(TEXT("Bank"));

    // (1-i) 도전과제
    AchievementComp = CreateDefaultSubobject<UAchievementComponent>(TEXT("Achievements"));

    // (1-j) 사회 스탯 (자체 세이브 슬롯 자가 로드 — 별도 로드 코드 불필요)
    SocialComp = CreateDefaultSubobject<USocialStatsComponent>(TEXT("SocialStats"));

    // (1-k) 장비 — 장착 시 StatComponent 보너스 합산. EquipmentWidget이 이걸 읽음(없으면 빈 화면이라 장착).
    EquipmentComp = CreateDefaultSubobject<UEquipmentComponent>(TEXT("Equipment"));

    // (2) 카메라 컴포넌트 생성
    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    // 스프링 암의 소켓 위치에 카메라를 부착
    CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
    CameraComp->SetRelativeLocation(FVector::ZeroVector);

    // 카메라는 스프링 암의 회전을 따르므로 PawnControlRotation은 꺼둠
    CameraComp->bUsePawnControlRotation = false;


    // 1. 캐릭터가 이동 방향으로 몸을 자동으로 회전하도록 설정
    GetCharacterMovement()->bOrientRotationToMovement = true;

    // 2. 회전 속도 설정 (값이 클수록 홱홱 돕니다. 보통 540 정도가 적당합니다)
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

    // 3. 컨트롤러(카메라)의 회전값이 캐릭터의 회전에 영향을 주지 않도록 모두 끔
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    SpringArmComp->bUsePawnControlRotation = true; // 카메라는 컨트롤러 회전을 따름


    NormalSpeed = 600.0f;
    SprintSpeedMultiplier = 1.5f;
    SprintSpeed = NormalSpeed * SprintSpeedMultiplier;

    GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;

    // 캡슐 컴포넌트의 수정을 허용하도록 설정 (잠겨있는 경우 강제 해제)
    if (GetCapsuleComponent())
    {
        GetCapsuleComponent()->SetCanEverAffectNavigation(true);
        // 특별히 생성자에서 아무 값도 InitCapsuleSize로 건드리지 않으면 
        // 블루프린트의 기본값이 우선권을 갖게 됩니다.
    }

    // ★탐험 기본 = 3인칭 (사용자 지시 2026-07-24: "그대로 3인칭으로 해줘").
    //   카메라가 캐릭터 뒤 위쪽에서 내려다보는 시점. 몸은 이동 방향으로 돈다.
    ViewMode = 0;

    SpringArmComp->SetUsingAbsoluteRotation(false);
    // 시점을 더 높이 올린다 — 사용자 지적 "시점 더 올려". 카메라를 위로 많이 얹고 위에서 내려다보게.
    SpringArmComp->TargetArmLength = 520.0f;
    SpringArmComp->SocketOffset = FVector(0.0f, 0.0f, 90.0f);       // 카메라를 훨씬 위로 → 내려다보는 각
    SpringArmComp->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f)); // 회전 축을 머리 높이로

    SpringArmComp->bUsePawnControlRotation = true;   // 카메라 팔이 컨트롤러(마우스) 회전을 따름
    // 위치 지연만 약하게(걸을 때 부드럽게). ★회전 지연은 끈다 — 마우스 돌릴 때 굼떠서 답답한 원인.
    SpringArmComp->bEnableCameraLag = true;
    SpringArmComp->CameraLagSpeed = 14.0f;
    SpringArmComp->bEnableCameraRotationLag = false;
    CameraComp->bUsePawnControlRotation = false;

    bUseControllerRotationYaw = false;                // 마우스로 카메라만 돌고, 몸은 안 따라 돌음
    GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향으로 몸이 돈다
    GetMesh()->SetOwnerNoSee(false);                  // 내 캐릭터가 화면에 보인다(3인칭)
}



void APlayerCharacter::ToggleView()
{
    ViewMode = (ViewMode + 1) % 2;

    if (ViewMode == 0)
    {
        // [3인칭: 장소 고정(CCTV) 느낌 모드]
        SpringArmComp->TargetArmLength = 300.0f;
        SpringArmComp->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));

        // 핵심 1: 카메라가 캐릭터의 회전을 전혀 따르지 않게 설정 (절대 좌표 사용)
        // 이렇게 하면 캐릭터가 뱅글뱅글 돌아도 카메라는 장소만 비춥니다.
        SpringArmComp->SetUsingAbsoluteRotation(true);

        // 핵심 2: 마우스로 카메라를 돌려도 캐릭터 몸은 전혀 반응하지 않음
        bUseControllerRotationYaw = false;
        GetCharacterMovement()->bOrientRotationToMovement = true;

        // 카메라 자체 회전은 마우스로 가능하게 유지
        SpringArmComp->bUsePawnControlRotation = true;
        CameraComp->bUsePawnControlRotation = false;

        GetMesh()->SetOwnerNoSee(false);
    }
    else
    {
        // [1인칭: 제자리 공회전 모드 - 그대로 유지]
        // 1인칭으로 올 때는 다시 절대 좌표를 꺼줘야 정상 작동합니다.
        SpringArmComp->SetUsingAbsoluteRotation(false);

        SpringArmComp->TargetArmLength = 0.0f;
        SpringArmComp->SocketOffset = FVector(20.0f, 0.0f, 0.0f);
        SpringArmComp->SetRelativeLocation(FVector(0.0f, 0.0f, 35.0f));

        SpringArmComp->bUsePawnControlRotation = false;
        CameraComp->bUsePawnControlRotation = true;

        bUseControllerRotationYaw = true;
        GetCharacterMovement()->bOrientRotationToMovement = false;
        GetMesh()->SetOwnerNoSee(true);
    }
}










void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 장비 카탈로그 기본값 주입(BP에서 안 채웠을 때만) — 장비창이 빈 화면이 안 되도록. 현대 배경 기어.
    if (EquipmentComp && EquipmentComp->Catalog.Num() == 0)
    {
        auto Eq = [](FName Id, const FString& Name, EEquipSlot Slot,
                     float Atk, float Def, float HP, float SP) -> FEquipItem
        {
            FEquipItem E; E.Id = Id; E.Name = Name; E.Slot = Slot;
            E.AtkBonus = Atk; E.DefBonus = Def; E.HPBonus = HP; E.SPBonus = SP;
            return E;
        };
        EquipmentComp->Catalog = {
            // 무기
            Eq(TEXT("wpn_woodsword"), TEXT("목검"),       EEquipSlot::Weapon, 5.f,  0.f, 0.f,  0.f),
            Eq(TEXT("wpn_bat"),       TEXT("금속 배트"),   EEquipSlot::Weapon, 12.f, 0.f, 0.f,  0.f),
            Eq(TEXT("wpn_baton"),     TEXT("특수 경봉"),   EEquipSlot::Weapon, 20.f, 2.f, 0.f,  0.f),
            // 방어구
            Eq(TEXT("arm_gym"),       TEXT("체육복"),       EEquipSlot::Armor,  0.f,  5.f,  10.f, 0.f),
            Eq(TEXT("arm_jacket"),    TEXT("가죽 자켓"),    EEquipSlot::Armor,  0.f,  10.f, 20.f, 0.f),
            Eq(TEXT("arm_vest"),      TEXT("방검복"),       EEquipSlot::Armor,  0.f,  18.f, 40.f, 0.f),
            // 장신구 (장비창 슬롯이 8개라 카탈로그도 8종으로 맞춤 — 전부 표시되도록)
            Eq(TEXT("acc_charm"),     TEXT("행운의 부적"),  EEquipSlot::Accessory, 0.f, 3.f, 15.f, 10.f),
            Eq(TEXT("acc_ring"),      TEXT("각성의 반지"),  EEquipSlot::Accessory, 5.f, 5.f, 0.f,  15.f),
        };
    }

    if (CombatComponent)
    {
        CombatComponent->OnCombatStarted.AddDynamic(this, &APlayerCharacter::OnCombatStarted_Handler);
        CombatComponent->OnCombatEnded.AddDynamic(this, &APlayerCharacter::OnCombatEnded_Handler);
    }

    // 콤보 윈도우: AnimInstance 이벤트 바인딩
    if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
    {
        AnimInst->OnPlayMontageNotifyBegin.AddDynamic(this, &APlayerCharacter::OnMontageNotifyBegin_Handler);
        AnimInst->OnMontageBlendingOut.AddDynamic(this, &APlayerCharacter::OnMontageBlendingOut_Handler);
    }

    // 세이브 데이터 로드 (플레이어 레벨/경험치/성장 복원)
    if (UGameplayStatics::DoesSaveGameExist(TEXT("PlayerSave"), 0))
    {
        if (USecretSaveGame* Save = Cast<USecretSaveGame>(
            UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0)))
        {
            if (Save->bHasData && GetStatComponent())
            {
                GetStatComponent()->SetProgression(
                    Save->PlayerLevel, Save->PlayerXP, Save->PlayerMaxHP,
                    Save->PlayerAttack, Save->PlayerDefense, Save->PlayerMaxSP);
                GetStatComponent()->SetGold(Save->PlayerGold);

                // 소비아이템 보관함 복원
                if (InventoryComp)
                    InventoryComp->LoadStacks(Save->Inventory);

                // 퀘스트 진행 복원 (저장된 기록이 있을 때만 — 구버전 세이브는 자동시작 퀘스트 유지)
                if (QuestComp && Save->Quests.Num() > 0)
                    QuestComp->LoadRecords(Save->Quests);

                // 장비 장착 복원 (저장분 있을 때만 — 구버전 세이브는 무시) + 스탯 보너스 재적용
                if (EquipmentComp && Save->EquippedIds.Num() > 0)
                {
                    EquipmentComp->LoadEquippedIds(Save->EquippedIds);
                    EquipmentComp->RecalcAndApply();
                }

                // 날짜/시간대 복원
                if (TimeComp)
                    TimeComp->LoadTime(Save->Day, Save->TimePhase);

                // NPC 인연(소셜링크) 복원
                if (RelationComp && Save->Relationships.Num() > 0)
                    RelationComp->LoadRecords(Save->Relationships);

                // 날씨 복원
                if (WeatherComp)
                    WeatherComp->LoadWeather(Save->Weather);

                // 은행 예치금 복원
                if (BankComp)
                    BankComp->LoadStoredGold(Save->StoredGold);
            }
        }
    }

    // 시간 변경 → 퀘스트 ReachDay 연동 (이후 변경 구독 + 현재 날짜 1회 평가)
    if (TimeComp)
    {
        TimeComp->OnTimeChanged.AddDynamic(this, &APlayerCharacter::OnTimeChanged_Handler);
        if (QuestComp)
            QuestComp->NotifyDayReached(TimeComp->GetDay());
    }

    // 상시 월드 HUD (옵트인 — WorldHUDClass 할당됐을 때만)
    if (WorldHUDClass)
    {
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            WorldHUD = CreateWidget<UWorldHUDWidget>(PC, WorldHUDClass);
            if (WorldHUD)
                WorldHUD->AddToViewport(0);
        }
    }
}

bool APlayerCharacter::SetWorldHudHidden(bool bHide)
{
    if (!WorldHUD) return false;   // 아직 안 만들어졌다 — 부른 쪽이 다시 시도해야 한다
    WorldHUD->SetVisibility(bHide ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    return true;
}

void APlayerCharacter::OnTimeChanged_Handler(int32 Day, EDayPhase /*Phase*/)
{
    if (QuestComp)
        QuestComp->NotifyDayReached(Day);
}

void APlayerCharacter::OnCombatStarted_Handler(ECombatMode Mode, ECombatStyle Style)
{
    if (Style == ECombatStyle::Boxing)
    {
        AddCombatIMC();

        // 전투는 항상 3인칭으로 강제 전환 (복싱 자세가 보여야 함)
        ViewMode = 0;
        SpringArmComp->SetUsingAbsoluteRotation(false);
        SpringArmComp->TargetArmLength = 300.0f;
        SpringArmComp->SocketOffset = FVector::ZeroVector;
        SpringArmComp->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
        SpringArmComp->bUsePawnControlRotation = true;
        CameraComp->bUsePawnControlRotation = false;
        GetMesh()->SetOwnerNoSee(false);

        // 복싱: strafe 이동 (카메라 방향 기준 8방향)
        GetCharacterMovement()->bOrientRotationToMovement = false;
        bUseControllerRotationYaw = true;
    }

    // 턴제에서는 이동 비활성화
    if (Mode == ECombatMode::TurnBased)
        GetCharacterMovement()->SetMovementMode(MOVE_None);
}

void APlayerCharacter::OnCombatEnded_Handler()
{
    RemoveCombatIMC();
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    GetCharacterMovement()->bOrientRotationToMovement = true;
    bUseControllerRotationYaw = false;

    // 콤보 상태 초기화
    bComboWindowOpen = false;
    PendingBoxingInput = EBoxingInput::None;
}

void APlayerCharacter::AddCombatIMC()
{
    if (ASecretProjectPlayerController* PC = Cast<ASecretProjectPlayerController>(GetController()))
        PC->AddCombatBoxingIMC();
}

void APlayerCharacter::RemoveCombatIMC()
{
    if (ASecretProjectPlayerController* PC = Cast<ASecretProjectPlayerController>(GetController()))
        PC->RemoveCombatBoxingIMC();
}

// ── 복싱 입력 핸들러 ──────────────────────────────────────

void APlayerCharacter::OnBoxingLeftJab(const FInputActionValue& Value)   { ExecuteBoxingInput(EBoxingInput::LeftJab); }
void APlayerCharacter::OnBoxingRightJab(const FInputActionValue& Value)  { ExecuteBoxingInput(EBoxingInput::RightJab); }
void APlayerCharacter::OnBoxingLeftHook(const FInputActionValue& Value)  { ExecuteBoxingInput(EBoxingInput::LeftHook); }
void APlayerCharacter::OnBoxingRightHook(const FInputActionValue& Value) { ExecuteBoxingInput(EBoxingInput::RightHook); }
void APlayerCharacter::OnBoxingUppercut(const FInputActionValue& Value)  { ExecuteBoxingInput(EBoxingInput::Uppercut); }

void APlayerCharacter::ExecuteBoxingInput(EBoxingInput Input)
{
    if (!CombatComponent || !CombatComponent->IsInCombat()) return;

    UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
    const bool bMontagePlaying = AnimInst && AnimInst->IsAnyMontagePlaying();

    if (bMontagePlaying)
    {
        if (bComboWindowOpen)
        {
            // 윈도우 열려있으면 즉시 다음 공격 실행
            bComboWindowOpen = false;
            PendingBoxingInput = EBoxingInput::None;
            CombatComponent->OnAttackInput(Input);
        }
        else
        {
            // 윈도우 닫혔으면 큐에 저장 (마지막 입력만 유지)
            PendingBoxingInput = Input;
        }
    }
    else
    {
        // 재생 중인 몽타주 없으면 즉시 실행
        CombatComponent->OnAttackInput(Input);
    }
}

void APlayerCharacter::OnMontageNotifyBegin_Handler(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
    if (NotifyName != FName("ComboWindow")) return;

    if (PendingBoxingInput != EBoxingInput::None)
    {
        // 윈도우 열리는 시점에 이미 입력이 대기 중이면 즉시 처리
        EBoxingInput Input = PendingBoxingInput;
        PendingBoxingInput = EBoxingInput::None;
        bComboWindowOpen = false;
        CombatComponent->OnAttackInput(Input);
    }
    else
    {
        bComboWindowOpen = true;
    }
}

void APlayerCharacter::OnMontageBlendingOut_Handler(UAnimMontage* Montage, bool bInterrupted)
{
    // 공격 몽타주에만 반응
    bool bIsAttackMontage = false;
    for (const auto& M : BasicAttackMontages)
        if (M == Montage) { bIsAttackMontage = true; break; }
    if (!bIsAttackMontage) return;

    bComboWindowOpen = false;

    if (!bInterrupted && PendingBoxingInput != EBoxingInput::None)
    {
        // 몽타주 자연 종료 후 대기 중인 입력 처리 (윈도우를 놓친 입력)
        EBoxingInput Input = PendingBoxingInput;
        PendingBoxingInput = EBoxingInput::None;
        CombatComponent->OnAttackInput(Input);
    }
    else
    {
        PendingBoxingInput = EBoxingInput::None;
    }
}

void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Enhanced InputComponent로 캐스팅
    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // IA를 가져오기 위해 현재 소유 중인 Controller를 ASecretProjectPlayerController로 캐스팅
        if (ASecretProjectPlayerController* PlayerController = Cast<ASecretProjectPlayerController>(GetController()))
        {
            if (PlayerController->MoveAction)
            {
                // IA_Move 액션 키를 "키를 누르고 있는 동안" Move() 호출
                EnhancedInput->BindAction(
                    PlayerController->MoveAction,
                    ETriggerEvent::Triggered,
                    this,
                    &APlayerCharacter::Move
                );
            }

            if (PlayerController->JumpAction)
            {
                // IA_Jump 액션 키를 "키를 누르고 있는 동안" StartJump() 호출
                EnhancedInput->BindAction(
                    PlayerController->JumpAction,
                    ETriggerEvent::Triggered,
                    this,
                    &APlayerCharacter::StartJump
                );

                // IA_Jump 액션 키에서 "손을 뗀 순간" StopJump() 호출
                EnhancedInput->BindAction(
                    PlayerController->JumpAction,
                    ETriggerEvent::Completed,
                    this,
                    &APlayerCharacter::StopJump
                );
            }

            if (PlayerController->LookAction)
            {
                // IA_Look 액션 마우스가 "움직일 때" Look() 호출
                EnhancedInput->BindAction(
                    PlayerController->LookAction,
                    ETriggerEvent::Triggered,
                    this,
                    &APlayerCharacter::Look
                );
            }

            if (PlayerController->SprintAction)
            {
                // IA_Sprint 액션 키를 "누르고 있는 동안" StartSprint() 호출
                EnhancedInput->BindAction(
                    PlayerController->SprintAction,
                    ETriggerEvent::Triggered,
                    this,
                    &APlayerCharacter::StartSprint
                );
                // IA_Sprint 액션 키에서 "손을 뗀 순간" StopSprint() 호출
                EnhancedInput->BindAction(
                    PlayerController->SprintAction,
                    ETriggerEvent::Completed,
                    this,
                    &APlayerCharacter::StopSprint
                );
            }
            if (PlayerController->InteractAction)
            {
                EnhancedInput->BindAction(
                    PlayerController->InteractAction,
                    ETriggerEvent::Started, // 누른 순간 바로 실행
                    this,
                    &APlayerCharacter::Interact
                );
            }
            if (PlayerController->ToggleViewAction)
            {
                EnhancedInput->BindAction(
                    PlayerController->ToggleViewAction,
                    ETriggerEvent::Started,
                    this,
                    &APlayerCharacter::ToggleView
                );
            }
            if (PlayerController->SystemMenuAction)
            {
                EnhancedInput->BindAction(
                    PlayerController->SystemMenuAction,
                    ETriggerEvent::Started,
                    this,
                    &APlayerCharacter::OpenSystemMenu
                );
            }

            // ★ ESC/M 직접 키 바인딩 — IMC/IA 에셋 없이도 시스템 메뉴 열림(C++ 폴백).
            //   SystemMenuAction이 따로 매핑돼 있어도 중복 무해. 에디터 입력설정 없이 바로 동작.
            PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &APlayerCharacter::OpenSystemMenu);
            PlayerInputComponent->BindKey(EKeys::M,      IE_Pressed, this, &APlayerCharacter::OpenSystemMenu);

            // ★ T 직접 키 — 목적지 이동 맵(페르소나식 구역 선택) 토글. 에셋 없이 바로 동작.
            PlayerInputComponent->BindKey(EKeys::T,      IE_Pressed, this, &APlayerCharacter::OpenDestinationMap);

            // ── 복싱 공격 키 바인딩 (U/I/J/K/L) ─────────────
            // CombatBoxingIMC가 활성화된 동안에만 실제로 동작함

            if (PlayerController->BoxingLeftJabAction)
                EnhancedInput->BindAction(PlayerController->BoxingLeftJabAction,
                    ETriggerEvent::Started, this, &APlayerCharacter::OnBoxingLeftJab);

            if (PlayerController->BoxingRightJabAction)
                EnhancedInput->BindAction(PlayerController->BoxingRightJabAction,
                    ETriggerEvent::Started, this, &APlayerCharacter::OnBoxingRightJab);

            if (PlayerController->BoxingLeftHookAction)
                EnhancedInput->BindAction(PlayerController->BoxingLeftHookAction,
                    ETriggerEvent::Started, this, &APlayerCharacter::OnBoxingLeftHook);

            if (PlayerController->BoxingRightHookAction)
                EnhancedInput->BindAction(PlayerController->BoxingRightHookAction,
                    ETriggerEvent::Started, this, &APlayerCharacter::OnBoxingRightHook);

            if (PlayerController->BoxingUppercutAction)
                EnhancedInput->BindAction(PlayerController->BoxingUppercutAction,
                    ETriggerEvent::Started, this, &APlayerCharacter::OnBoxingUppercut);
        }
    }
}
    


void APlayerCharacter::Move(const FInputActionValue& value)
{
    // 컨트롤러가 있어야 방향 계산이 가능
    if (!Controller) return;

    // Value는 Axis2D로 설정된 IA_Move의 입력값 (WASD)을 담고 있음
// 예) (X=1, Y=0) → 전진 / (X=-1, Y=0) → 후진 / (X=0, Y=1) → 오른쪽 / (X=0, Y=-1) → 왼쪽
    const FVector2D MoveInput = value.Get<FVector2D>();

    // 1. 컨트롤러(카메라)의 회전값을 가져옵니다.
    const FRotator Rotation = Controller->GetControlRotation();
    // 2. 캐릭터는 바닥에서만 움직이므로 Yaw(좌우) 값만 남기고 Pitch, Roll은 0으로 만듭니다.
    const FRotator YawRotation(0, Rotation.Yaw, 0);

    // 3. 카메라가 바라보는 방향을 기준으로 '전진'과 '오른쪽' 벡터를 새로 계산합니다.
    // 기존의 GetActorForwardVector() 대신 카메라 기준 벡터를 사용하는 것이 핵심입니다.
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    if (!FMath::IsNearlyZero(MoveInput.X))
    {
        // 아까는 GetActorForwardVector()였지만, 이제는 계산한 ForwardDirection을 씁니다.
        AddMovementInput(ForwardDirection, MoveInput.X);
    }

    if (!FMath::IsNearlyZero(MoveInput.Y))
    {
        // 아까는 GetActorRightVector()였지만, 이제는 계산한 RightDirection을 씁니다.
        AddMovementInput(RightDirection, MoveInput.Y);
    }
}

void APlayerCharacter::StartJump(const FInputActionValue& value)
{
    // Jump 함수는 Character가 기본 제공
    if (value.Get<bool>())
    {
        Jump();
    }
}

void APlayerCharacter::StopJump(const FInputActionValue& value)
{
    // StopJumping 함수도 Character가 기본 제공
    if (!value.Get<bool>())
    {
        StopJumping();
    }
}

void APlayerCharacter::Look(const FInputActionValue& value)
{
    // 마우스의 X, Y 움직임을 2D 축으로 가져옴
    FVector2D LookInput = value.Get<FVector2D>();

    // X는 좌우 회전 (Yaw), Y는 상하 회전 (Pitch)
    // 감도. 0.6은 굼떠서 답답했다(사용자 지적) → 0.85로 올려 반응 살림.
    const float LookSensitivity = 0.85f;
    AddControllerYawInput(LookInput.X * LookSensitivity);
    AddControllerPitchInput(LookInput.Y * LookSensitivity);
}

void APlayerCharacter::StartSprint(const FInputActionValue& value)
{
    // Shift 키를 누른 순간 이 함수가 호출된다고 가정
// 스프린트 속도를 적용
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
    }
}

void APlayerCharacter::StopSprint(const FInputActionValue& value)
{
    // Shift 키를 뗀 순간 이 함수가 호출
// 평상시 속도로 복귀
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
    }
}

void APlayerCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
}


// APlayerCharacter.cpp (상호작용 키 입력 시 호출되는 함수 예시)
// APlayerCharacter.cpp 내부 상호작용 함수 예시
void APlayerCharacter::OpenSystemMenu()
{
    // 토글/뒤로: 이미 열린 팝업(메뉴·인벤 등)이 있으면 그걸 닫는다(중복 스택 방지).
    //   ★ MenuFlow(풀스크린)는 장면→허브→닫기로 단계 처리(HandleEscBack). 허브에서 ESC면 여기서 제거.
    if (UPersonaWidgetBase::CloseTopEscWidget(this))
        return;
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;
    // 풀스크린 M 메뉴 우선(P5식 takeover). 미할당 시 레거시 시스템 메뉴로 폴백.
    if (MenuFlowWidgetClass)
        UMenuFlowWidget::OpenMenu(PC, MenuFlowWidgetClass);
    else if (SystemMenuWidgetClass)
        USystemMenuWidget::OpenMenu(PC, SystemMenuWidgetClass);
}

// T 키 — 목적지 이동 맵을 연다/닫는다(페르소나식 구역 선택). 대화창이 떠 있으면 무시.
void APlayerCharacter::OpenDestinationMap()
{
    if (UPersonaWidgetBase::CloseTopEscWidget(this)) return;   // 다른 팝업이 먼저 닫히게
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
        SDestinationMenu::Toggle(PC);
}

void APlayerCharacter::Interact()
{
    // E 연타 대화 중첩 방지: 이미 대화창이 떠 있으면 새 상호작용 무시(대화 진행은 마우스 클릭).
    {
        TArray<UUserWidget*> Talks;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Talks, UTalkUserWidget::StaticClass(), false);
        for (UUserWidget* W : Talks)
            if (W && W->IsInViewport()) return;
    }

    // ── [임시 진단] E키 = 복싱 전투 강제 시작/종료 토글 ──────
    static bool bTestMode = false; // 복싱 실시간 전투 진단용 토글 (NPC 대화 테스트 시 false 유지)
    if (bTestMode)
    {
        if (CombatComponent && !CombatComponent->IsInCombat())
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("[TEST] StartCombat ON - ABP/UIJKL 확인"));
            // StartCombat 호출: CurrentMode=RealTime 세팅 + OnCombatStarted 브로드캐스트(→ AddCombatIMC)
            // 자기 자신을 타겟으로 넘겨도 NPC캐스트 실패로 AI는 안 켜짐, IsInCombat()만 true됨
            CombatComponent->StartCombat(ECombatMode::RealTime, ECombatStyle::Boxing, Cast<AABaseCharacter>(this));
        }
        else if (CombatComponent)
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("[TEST] EndCombat OFF"));
            CombatComponent->EndCombat();
        }
        return;
    }
    // ── [임시 진단 끝] ────────────────────────────────────

    // 2. 시야(카메라)의 위치를 시작점으로, 바라보는 방향으로 1000유닛(10미터)만큼 앞을 끝점으로 설정합니다.
    FVector Start = GetPawnViewLocation();
    FVector End = Start + (GetViewRotation().Vector() * InteractDistance);


    // 4. 충돌 테스트(LineTrace)를 설정합니다. 자기 자신(플레이어)은 충돌 대상에서 제외합니다.
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    // 5. 설정한 시작점(Start)부터 끝점(End)까지 가시성(Visibility) 채널을 이용해 선을 쏩니다.
    if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        // 6. 무언가에 부딪혔다면, 그 대상의 이름을 화면에 초록색으로 출력합니다.
        FString HitName = Hit.GetActor()->GetName();
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("Hit: %s"), *HitName));

        // 7. 부딪힌 대상이 우리가 대화하고자 하는 'AANPCCharacter' 클래스인지 확인(Casting)합니다.
        AANPCCharacter* TargetNPC = Cast<AANPCCharacter>(Hit.GetActor());
        if (TargetNPC)
        {
            // 8. NPC를 성공적으로 확인했다면 하늘색 메시지를 띄웁니다.
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("NPC Cast Success!"));

            // 8-a. 퀘스트: 이 NPC와 대화 목표 진행 (상점 NPC 포함)
            // NPCName은 FString → 퀘스트 키(FName)로 변환
            if (QuestComp)
            {
                QuestComp->NotifyTalkedTo(FName(*TargetNPC->NPCName));
                // 이 NPC가 주는 퀘스트가 있으면 시작 (이미 진행/완료면 내부에서 무시)
                if (!TargetNPC->GrantsQuestId.IsNone())
                    QuestComp->StartQuest(TargetNPC->GrantsQuestId);
            }

            // 8-a2. 인연(소셜링크): 이 NPC와 대화 → 하루 1회 호감도 상승 (영입 게이트보다 먼저 — 오늘 대화분 반영)
            if (RelationComp)
            {
                const int32 TalkDay = TimeComp ? TimeComp->GetDay() : 1;
                const bool bFirstTalkToday = RelationComp->RegisterTalk(FName(*TargetNPC->NPCName), TalkDay);

                // 성격 → 사회 스탯 연동(페르소나식 "어울리기"): 하루 첫 대화면 NPC 성격에 맞는 사회 스탯 +2.
                // ArchetypeId가 있는(양산) NPC만. 수동 NPC는 성격 미정 → 스킵.
                if (bFirstTalkToday && SocialComp && !TargetNPC->ArchetypeId.IsNone())
                {
                    FNPCSocialProfile Prof;
                    if (UNPCArchetypeLibrary::FindSocialProfile(TargetNPC->ArchetypeId, Prof))
                    {
                        ESocialStat Stat = ESocialStat::Charm;
                        switch (Prof.Personality)
                        {
                        case ENPCPersonality::Cheerful:     Stat = ESocialStat::Charm;     break;
                        case ENPCPersonality::Shy:          Stat = ESocialStat::Knowledge; break;
                        case ENPCPersonality::Intellectual: Stat = ESocialStat::Knowledge; break;
                        case ENPCPersonality::Tough:        Stat = ESocialStat::Guts;      break;
                        case ENPCPersonality::Kind:         Stat = ESocialStat::Kindness;  break;
                        case ENPCPersonality::Cool:         Stat = ESocialStat::Guts;      break;
                        }
                        SocialComp->AddPoints(Stat, 2);
                    }
                }
            }

            // 8-a2b. 선물: NPC 선호 아이템을 들고 있으면 하루 1회 증정(1개 소모) + 큰 호감도
            if (RelationComp && InventoryComp && !TargetNPC->FavoriteGiftId.IsNone()
                && InventoryComp->GetCount(TargetNPC->FavoriteGiftId) > 0)
            {
                const int32 GiftDay = TimeComp ? TimeComp->GetDay() : 1;
                if (RelationComp->TryGift(FName(*TargetNPC->NPCName), GiftDay, TargetNPC->GiftAffinity))
                {
                    InventoryComp->RemoveItem(TargetNPC->FavoriteGiftId, 1);

                    FConsumableDef GiftDef;
                    const FString GiftName = UInventoryComponent::FindDef(TargetNPC->FavoriteGiftId, GiftDef)
                        ? GiftDef.Name : TargetNPC->FavoriteGiftId.ToString();
                    if (GEngine)
                    {
                        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Magenta,
                            FString::Printf(TEXT("%s에게 [%s] 선물!"), *TargetNPC->NPCName, *GiftName));
                        // 인물별 선물 반응 대사(있으면)
                        if (!TargetNPC->GiftReactionLine.IsEmpty())
                            GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan,
                                FString::Printf(TEXT("%s: %s"), *TargetNPC->NPCName, *TargetNPC->GiftReactionLine));
                    }
                }
            }

            // 8-a3. 영입 가능한 NPC면 아군으로 영입 (가능 여부는 내부 판단 — 인연 게이트 포함). recruiter 전달.
            TargetNPC->TryRecruit(this);

            // 8-b. 상점 NPC면 대화 대신 상점을 바로 연다 (야간 영업종료 체크)
            if (TargetNPC->bIsShopkeeper && TargetNPC->ShopWidgetClass)
            {
                const bool bNight = TimeComp && TimeComp->GetPhase() == EDayPhase::Night;
                if (TargetNPC->bClosedAtNight && bNight)
                {
                    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, TEXT("영업 종료 — 낮에 다시 오세요"));
                    return;
                }
                if (APlayerController* ShopPC = Cast<APlayerController>(GetController()))
                {
                    UShopWidget::OpenShop(ShopPC, TargetNPC->ShopWidgetClass, TargetNPC->ShopKind);
                    return;
                }
            }

            // NPC가 플레이어 쪽을 향하는 방향 계산
            FVector Direction = GetActorLocation() - TargetNPC->GetActorLocation();
            Direction.Z = 0.0f;
            FRotator NewRotation = Direction.Rotation();

            // 방법 A: 액터 회전 직접 설정 (가장 기본)
            TargetNPC->SetActorRotation(NewRotation);
            UE_LOG(LogTemp, Warning, TEXT("NPC New Rotation: %s"), *NewRotation.ToString());

            // 방법 B: 만약 NPC가 컨트롤러를 가지고 있다면 컨트롤러 회전도 맞춰줍니다.
            if (TargetNPC->GetController())
            {
                TargetNPC->GetController()->SetControlRotation(NewRotation);
            }

            // 9. 위젯을 생성하고 모드를 변경하기 위해 플레이어 컨트롤러를 가져옵니다.
            APlayerController* PC = Cast<APlayerController>(GetController());
            if (PC && DialogueWidgetClass)
            {
                // 10. 설정된 위젯 클래스(DialogueWidgetClass)를 바탕으로 대화 UI 위젯을 생성합니다.
                UTalkUserWidget* TalkUI = CreateWidget<UTalkUserWidget>(PC, DialogueWidgetClass);
                if (TalkUI)
                {
                    TalkUI->AddToViewport();

                    // 전투 선택지 포함 NPC 상호작용 시작
                    TalkUI->StartNPCInteraction(this, TargetNPC);

                    // SetWidgetToFocus 사용 안 함 → 위젯이 키보드 포커스를 가져가지 않음
                    // 대화 넘기기는 마우스 클릭으로 처리되므로 문제 없음
                    // 전투 시작 후 U/I/J/K/L이 Enhanced Input으로 바로 전달됨
                    FInputModeGameAndUI InputMode;
                    PC->SetInputMode(InputMode);
                    PC->bShowMouseCursor = true;
                }
            }
        }
        else if (ATreasureChest* Chest = Cast<ATreasureChest>(Hit.GetActor()))
        {
            // 7-b. 보물상자면 열기 (골드/아이템 지급 + 영구화는 상자 내부에서 처리)
            Chest->Open(this);
        }
        else if (ALockedGateActor* Gate = Cast<ALockedGateActor>(Hit.GetActor()))
        {
            // 7-b2. 잠긴 문이면 열쇠로 열기 시도 (성공/실패 메시지는 내부 처리)
            Gate->TryOpen(this);
        }
        else if (AHarvestNodeActor* Node = Cast<AHarvestNodeActor>(Hit.GetActor()))
        {
            // 7-b3. 채집물이면 채집 (하루 1회, 메시지 내부 처리)
            Node->Harvest(this);
        }
        else if (AWishingWellActor* Well = Cast<AWishingWellActor>(Hit.GetActor()))
        {
            // 7-b4. 소원의 샘이면 소원 빌기 (골드 소비→확률 보상)
            Well->MakeWish(this);
        }
        else if (ALoreNoteActor* Note = Cast<ALoreNoteActor>(Hit.GetActor()))
        {
            // 7-c-1. 표지판을 처음 읽으면 사회 스탯 [지식] 상승 + 읽을거리 수집 퀘스트 진행
            if (!Note->bAlreadyStudied)
            {
                Note->bAlreadyStudied = true;                       // 세션 내 반복 파밍 방지
                if (SocialComp)
                    SocialComp->AddPoints(ESocialStat::Knowledge, 5);

                // 읽을거리 수집(ReadLore) 카운트: LoreId 있으면 영구 1회만(CollectedIds), 없으면 세션 1회
                bool bCountForQuest = true;
                if (!Note->LoreId.IsNone())
                {
                    if (USecretSaveGame::IsCollected(Note->LoreId)) bCountForQuest = false;
                    else USecretSaveGame::MarkCollected(Note->LoreId);
                }
                if (bCountForQuest && QuestComp)
                    QuestComp->NotifyLoreRead(1);
            }

            // 7-c. 표지판/메모면 대화창에 텍스트 표시 (대화 위젯 재사용)
            if (APlayerController* NotePC = Cast<APlayerController>(GetController()))
            {
                if (DialogueWidgetClass)
                {
                    if (UTalkUserWidget* TalkUI = CreateWidget<UTalkUserWidget>(NotePC, DialogueWidgetClass))
                    {
                        TalkUI->AddToViewport();
                        TalkUI->StartDialogue(Note->Title, Note->Lines);
                        FInputModeGameAndUI InputMode;
                        NotePC->SetInputMode(InputMode);
                        NotePC->bShowMouseCursor = true;
                    }
                }
            }
        }
        else
        {
            // 14. 무언가 맞긴 했지만, NPC/상자/표지판이 아닐 경우 노란색 경고를 띄웁니다.
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Cast Failed: Not Interactable"));
        }
    }
}

