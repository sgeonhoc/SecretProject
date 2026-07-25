#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * 레벨 이동 포탈/문. 플레이어가 오버랩하면 진행상황 저장 후 목표 레벨로 이동.
 * - 이동 전 SavePlayerProgression 호출 → 골드/인벤토리/퀘스트가 다음 레벨로 이어짐
 *   (도착 레벨 APlayerCharacter::BeginPlay에서 자동 로드).
 * - 로직 전부 C++. 메시/머티리얼/배치는 BP·에디터(사용자). TargetLevelName만 지정하면 됨.
 */
UCLASS()
class SECRET_PROJECT_API APortalActor : public AActor
{
    GENERATED_BODY()

public:
    APortalActor();

    // 이 문이 어디로 나가나 — 진행(GameFlow)이 "저 레벨로 가는 문"을 찾아 목표로 삼을 때 쓴다.
    UFUNCTION(BlueprintPure, Category = "Portal")
    FName GetTargetLevelName() const { return TargetLevelName; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
    TObjectPtr<USphereComponent> Trigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    // 이동할 목표 레벨 이름 (에디터 맵 이름). 비면 동작 안 함.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
    FName TargetLevelName;

    // 도착 레벨에서 설 자리 — 그 레벨 PlayerStart의 PlayerStartTag와 같은 값을 넣는다.
    // (예: 큰길→골동상 문에 "FromStreet" → 골동상 안 문간 PlayerStart의 태그도 "FromStreet")
    // 비우면 그 레벨의 아무 PlayerStart에 선다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
    FName ArrivalEntryTag;

    // 이동 직전 진행상황 저장 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
    bool bSaveBeforeTravel = true;

    // ── 스토리 상태 게이트 ──────────────────────────────
    // RequiredFlag 지정 시 그 플래그가 서야 이 문/포탈이 열려 있다. ForbiddenFlag 지정 시 그 플래그가 서면 닫힌다(숨김+충돌끔).
    // 둘 다 비면 항상 열림(기존 동작). 잠긴 문의 "다른 길은 이미 있다" 원칙과 함께 쓴다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FName RequiredFlag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FName ForbiddenFlag;

    bool bTraveling = false;

    UFUNCTION()
    void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

public:
    // 목표 레벨로 이동 (저장 후 OpenLevel). 직접 호출도 가능(BP 버튼/연출 등).
    UFUNCTION(BlueprintCallable, Category = "Portal")
    void TravelTo(AActor* Traveler);
};
