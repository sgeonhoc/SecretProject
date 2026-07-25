#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LockedGateActor.generated.h"

class UStaticMeshComponent;

/**
 * 잠긴 문/관문. 플레이어를 물리적으로 막고, E(Interact)로 열쇠 아이템 보유 시 열린다.
 * 열린 상태는 영구화(GateId). 탐험↔인벤토리 연결. 로직 C++, 메시/배치는 BP·에디터.
 */
UCLASS()
class SECRET_PROJECT_API ALockedGateActor : public AActor
{
    GENERATED_BODY()

public:
    ALockedGateActor();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    // 열기 위해 필요한 인벤토리 아이템 Id (InventoryComponent 카탈로그 Id, 예: OldKey)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
    FName RequiredItemId = TEXT("OldKey");

    // 열 때 열쇠를 소모할지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
    bool bConsumeKey = true;

    // 세이브 키(배치마다 고유). 한 번 열면 재방문 시 열린 상태.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate")
    FName GateId;

    // ── 스토리 상태 게이트 ──────────────────────────────
    // RequiredFlag 지정 시 그 플래그가 서야 이 관문이 존재한다. ForbiddenFlag 지정 시 그 플래그가 서면 사라진다(치웠음).
    // 열쇠(RequiredItemId) 게이트와 별개 — 이건 "이 상태에 이 문이 있는가"를 가른다. 둘 다 비면 항상 존재.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FName RequiredFlag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FName ForbiddenFlag;

    bool bOpen = false;

    void ApplyOpenVisual();

public:
    // 상호작용으로 호출. 열쇠 있으면 열고 true, 없으면 안내 후 false.
    UFUNCTION(BlueprintCallable, Category = "Gate")
    bool TryOpen(AActor* Opener);

    UFUNCTION(BlueprintPure, Category = "Gate")
    FORCEINLINE bool IsOpen() const { return bOpen; }
};
