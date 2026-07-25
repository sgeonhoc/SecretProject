#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexTypes.h"
#include "HexUnit.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * 전투 유닛(능력자) 하나. HP·좌표·상태(둔화/결빙/방패/영창 무방비)를 갖는다.
 * 실제 캐릭터 메시·애니는 나중에 BP로 교체. 지금은 원기둥 표식으로 위치만 실감나게.
 */
UCLASS()
class SECRET_PROJECT_API AHexUnit : public AActor
{
	GENERATED_BODY()

public:
	AHexUnit();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(BlueprintReadOnly, Category = "Unit") FHexCoord Coord;
	UPROPERTY(BlueprintReadOnly, Category = "Unit") bool bPlayerSide = true;
	UPROPERTY(BlueprintReadOnly, Category = "Unit") FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Unit") int32 HP = 4000;
	UPROPERTY(BlueprintReadOnly, Category = "Unit") int32 MaxHP = 4000;
	UPROPERTY(BlueprintReadOnly, Category = "Unit") int32 Shield = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Unit") int32 Slow = 0;    // 둔화 남은 턴
	UPROPERTY(BlueprintReadOnly, Category = "Unit") int32 Frozen = 0;  // 결빙 남은 턴
	UPROPERTY(BlueprintReadOnly, Category = "Unit") bool bExposed = false; // 영창 무방비(자리 값)

	bool IsAlive() const { return HP > 0; }

	void Setup(bool bInPlayer, const FString& InName, int32 InMaxHP);
	void RefreshVisual();

	// 실제 캐릭터(AABaseCharacter BP)를 이 유닛의 몸으로 삼는다. 성공하면 원기둥은 숨김.
	UPROPERTY() TObjectPtr<AActor> CharacterActor;
	void SetCharacter(AActor* InChar);
	void FaceToward(const FVector& WorldTarget);
	void PlayCastAnim();
	void PlayHitAnim();

	// 좌표가 바뀌면 월드 위치도 갱신. GroundLoc = 타일 지면 위치(그리드 매니저가 WorldOfCoord로 전달)
	void MoveToWorld(const FVector& GroundLoc);

	UFUNCTION(BlueprintImplementableEvent, Category = "Unit")
	void OnUnitVisualUpdate(int32 InHP, int32 InMaxHP, bool bInExposed, int32 InSlow, int32 InFrozen);

private:
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MID;
};
