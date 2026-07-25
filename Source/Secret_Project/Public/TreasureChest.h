#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TreasureChest.generated.h"

class UStaticMeshComponent;

/**
 * 월드에 배치하는 보물상자. 플레이어가 E(Interact)로 열면 골드/소비아이템 지급 후 영구히 열린 상태.
 * - 로직 전부 C++. 메시/머티리얼은 BP/에디터에서 지정(사용자 작업).
 * - ChestId는 배치된 상자마다 **고유**해야 함(세이브 키). 같은 Id면 하나만 열려도 둘 다 열린 처리됨.
 */
UCLASS()
class SECRET_PROJECT_API ATreasureChest : public AActor
{
    GENERATED_BODY()

public:
    ATreasureChest();

protected:
    virtual void BeginPlay() override;

    // 외형 + 라인트레이스 충돌용 (Visibility 채널 블록 → Interact 트레이스가 맞힘)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    // ── 세이브 키 (배치마다 고유하게) ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
    FName ChestId;

    // ── 내용물 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
    int32 GoldContents = 50;

    // 지급할 소비아이템 Id (InventoryComponent 카탈로그 Id). None이면 골드만.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
    FName ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
    int32 ItemCount = 1;

    // 열렸을 때 액터를 숨길지 (false면 BP의 OnOpened에서 메시 교체 등 직접 처리)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
    bool bHideWhenOpened = true;

    bool bOpened = false;

    // 열림 처리 후 시각 갱신 (숨김/콜리전 off)
    void ApplyOpenedVisual();

public:
    // 플레이어가 상호작용으로 호출. 이미 열렸으면 무시.
    UFUNCTION(BlueprintCallable, Category = "Chest")
    void Open(AActor* Opener);

    UFUNCTION(BlueprintPure, Category = "Chest")
    FORCEINLINE bool IsOpened() const { return bOpened; }

    // BP에서 FX/메시 교체 등 연출용 (열릴 때 1회 호출됨)
    UFUNCTION(BlueprintImplementableEvent, Category = "Chest")
    void OnOpened();
};
