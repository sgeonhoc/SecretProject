#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HexBattlePlayerController.generated.h"

class AHexGridManager;
class UHexBattleScreen;

/**
 * 헥스 전투 입력. 마우스 클릭으로 타일/유닛 선택·이동·시전, 숫자키로 카드 선택, Space=턴 넘기기, Esc=선택 해제.
 * HUD(카드 손패·HP·칸)는 나중에 UMG 위젯을 붙인다 — 지금은 그리드 로그가 화면에 뜬다.
 */
UCLASS()
class SECRET_PROJECT_API AHexBattlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHexBattlePlayerController();

	// UMG 전투 화면이 떠 있는가 — 캔버스 HUD가 같은 UI를 중복으로 그리지 않게 하는 판정용.
	bool HasBattleScreen() const { return Screen != nullptr; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	UPROPERTY() TObjectPtr<AHexGridManager> Grid;
	UPROPERTY() TObjectPtr<UHexBattleScreen> Screen;

	AHexGridManager* GetGrid();

	void OnClick();
	void OnEndTurn();
	void OnDeselect();
	void SelectCardKey(int32 Index);

	// 숫자키 카드 선택(1~9,0). 람다 바인딩 대신 확실한 개별 핸들러.
	void Key1(); void Key2(); void Key3(); void Key4(); void Key5();
	void Key6(); void Key7(); void Key8(); void Key9(); void Key0();
};
