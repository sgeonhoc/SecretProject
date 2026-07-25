#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HexBattleHUD.generated.h"

class AHexGridManager;

// 화면 클릭 판정용 사각 영역(카드/버튼)
USTRUCT()
struct FHudHit
{
	GENERATED_BODY()
	FBox2D Rect = FBox2D(ForceInit);
	int32 Kind = 0;    // 0=카드, 1=턴넘기기, 2=새 판, 3=선택해제
	int32 Index = -1;  // 카드일 때 손패 인덱스
};

/**
 * 헥스 전투 화면 HUD(Canvas). HP·칸·손패·로그를 그린다. UMG 에셋 없이 Play 즉시 뜬다.
 * 카드/버튼 클릭은 컨트롤러가 HitTest로 물어봐서 그리드 API를 부른다(폰트 함정 회피: 엔진 폰트 사용).
 */
UCLASS()
class SECRET_PROJECT_API AHexBattleHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	// 화면 좌표가 어떤 카드/버튼 위인지. 맞으면 true + 종류/인덱스.
	bool HitTest(const FVector2D& P, int32& OutKind, int32& OutIndex) const;

private:
	UPROPERTY() TObjectPtr<AHexGridManager> Grid;
	AHexGridManager* GetGrid();

	TArray<FHudHit> Hits; // 매 DrawHUD마다 재구성

	void DrawUnitPanel(class AHexUnit* U, float X, float Y, float W);
	void DrawButton(const FString& Label, float X, float Y, float W, float H, int32 Kind, const FLinearColor& Col);
};
