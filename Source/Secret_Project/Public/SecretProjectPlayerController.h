#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SecretProjectPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class SECRET_PROJECT_API ASecretProjectPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ASecretProjectPlayerController();

    // ── 기본 이동 IMC / IA ──────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputMappingContext> InputMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> ToggleViewAction;

    // 시스템 메뉴(저장/불러오기/새 게임) 열기 — 예: ESC/M 키에 IA 할당
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> SystemMenuAction;

    // ── 복싱 전투 IMC / IA (U/I/J/K/L) ────────────────
    // 전투 진입 시 AddMappingContext로 활성화, 이동 키와 독립

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Combat|Boxing")
    TObjectPtr<UInputMappingContext> CombatBoxingIMC;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Combat|Boxing")
    TObjectPtr<UInputAction> BoxingLeftJabAction;   // U

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Combat|Boxing")
    TObjectPtr<UInputAction> BoxingRightJabAction;  // I

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Combat|Boxing")
    TObjectPtr<UInputAction> BoxingLeftHookAction;  // J

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Combat|Boxing")
    TObjectPtr<UInputAction> BoxingRightHookAction; // K

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Combat|Boxing")
    TObjectPtr<UInputAction> BoxingUppercutAction;  // L

    // IMC 전환 함수 (APlayerCharacter에서 호출)
    void AddCombatBoxingIMC();
    void RemoveCombatBoxingIMC();

    virtual void BeginPlay() override;
};
