#include "SecretProjectPlayerController.h"
#include "EnhancedInputSubsystems.h"

ASecretProjectPlayerController::ASecretProjectPlayerController()
    : InputMappingContext(nullptr)
    , MoveAction(nullptr)
    , JumpAction(nullptr)
    , LookAction(nullptr)
    , SprintAction(nullptr)
    , CombatBoxingIMC(nullptr)
    , BoxingLeftJabAction(nullptr)
    , BoxingRightJabAction(nullptr)
    , BoxingLeftHookAction(nullptr)
    , BoxingRightHookAction(nullptr)
    , BoxingUppercutAction(nullptr)
{
}

void ASecretProjectPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (InputMappingContext)
                Subsystem->AddMappingContext(InputMappingContext, 0);
        }
    }
}

void ASecretProjectPlayerController::AddCombatBoxingIMC()
{
    if (!CombatBoxingIMC) return;
    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Sub =
            LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            // 우선순위 1: 기본 IMC(0)보다 높아서 U/I/J/K/L 충돌 시 전투 IMC가 우선
            Sub->AddMappingContext(CombatBoxingIMC, 1);
        }
    }
}

void ASecretProjectPlayerController::RemoveCombatBoxingIMC()
{
    if (!CombatBoxingIMC) return;
    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Sub =
            LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            Sub->RemoveMappingContext(CombatBoxingIMC);
        }
    }
}
