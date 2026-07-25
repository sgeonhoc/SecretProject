#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexTypes.h"
#include "HexTile.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * 헥스 타일 하나. 좌표·표면 상태를 갖고, 클릭 히트 대상이 된다.
 * 로직(표면/좌표)은 C++. 색·머티리얼은 best-effort 틴트 + BP 훅(OnVisualUpdate)으로 다듬는다.
 */
UCLASS()
class SECRET_PROJECT_API AHexTile : public AActor
{
	GENERATED_BODY()

public:
	AHexTile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(BlueprintReadOnly, Category = "Hex")
	FHexCoord Coord;

	UPROPERTY(BlueprintReadOnly, Category = "Hex")
	ESurface Surface = ESurface::None;

	// 벽/잠든 진 표시 여부(그리드 매니저가 갱신)
	UPROPERTY(BlueprintReadOnly, Category = "Hex")
	bool bWall = false;

	UPROPERTY(BlueprintReadOnly, Category = "Hex")
	bool bJin = false;

	void Setup(const FHexCoord& InCoord);
	void SetSurface(ESurface In);
	void SetHighlight(bool bOn, bool bBlocked);
	void RefreshVisual();

	// 표면 색(비주얼 훅에서 참고)
	static FLinearColor SurfaceColor(ESurface S);

	// BP에서 실제 머티리얼/이펙트로 다듬을 수 있는 훅(로직 C++/비주얼 BP 컨벤션)
	UFUNCTION(BlueprintImplementableEvent, Category = "Hex")
	void OnVisualUpdate(ESurface InSurface, bool bInWall, bool bInJin, bool bInHighlight);

private:
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MID;
	bool bHighlight = false;
	bool bHighlightBlocked = false;
};
