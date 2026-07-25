#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexTypes.h"
#include "HexGridManager.generated.h"

class AHexTile;
class AHexUnit;

// 잠든 진(선새김) 데이터
USTRUCT()
struct FJinData
{
	GENERATED_BODY()
	bool bOwnerPlayer = true;
	int32 FreezeDur = 0;
	int32 SlowDur = 0;
	int32 TrapDamage = 0;
	FString Name;
};

// 얹는 중인 대주문(영창 차징)
USTRUCT()
struct FChargeState
{
	GENERATED_BODY()
	bool bActive = false;
	bool bWhoPlayer = true;
	int32 CardIndex = -1;
	FHexCoord Target;
	int32 Acc = 0;   // 지금까지 얹은 칸
	int32 Need = 0;  // 완성 칸
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHexBattleLog, const FString&, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHexBattleChanged);

/**
 * 헥스 칸/살란 전투의 심장. 그리드 절차 생성 + 표면 물리 + 과녁=눈(LoS) + 칸 턴 루프 + 카드 해결.
 * 웹 시뮬 전투시뮬.html v0.2를 그대로 이식한다(신규 규칙 창작 없음).
 * GameMode가 스폰·초기화하고, PlayerController가 클릭 입력을 이 API로 넘긴다.
 */
UCLASS()
class SECRET_PROJECT_API AHexGridManager : public AActor
{
	GENERATED_BODY()

public:
	AHexGridManager();

	// ── 설정 ──
	UPROPERTY(EditAnywhere, Category = "Hex") int32 GridRadius = 4;  // StartBattle에서 레벨 반경으로 덮임
	UPROPERTY(EditAnywhere, Category = "Hex") float HexSize = 110.0f; // uu, 헥스 반지름
	UPROPERTY(EditAnywhere, Category = "Hex") int32 KanMax = 6;
	// 어느 레벨을 띄울지(GameMode가 맵 이름 접미사로 정해 넣는다). 정본=게임_전투레벨_정본.md
	UPROPERTY(EditAnywhere, Category = "Hex") int32 LevelIndex = 0;

	// ── 상태(HUD/컨트롤러가 읽음) ──
	UPROPERTY(BlueprintReadOnly, Category = "Hex") int32 Round = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Hex") bool bPlayerTurn = true;
	UPROPERTY(BlueprintReadOnly, Category = "Hex") int32 Kan = 6;
	UPROPERTY(BlueprintReadOnly, Category = "Hex") bool bOver = false;

	UPROPERTY(BlueprintReadOnly, Category = "Hex") TObjectPtr<AHexUnit> PlayerUnit;
	UPROPERTY(BlueprintReadOnly, Category = "Hex") TObjectPtr<AHexUnit> EnemyUnit;

	UPROPERTY(BlueprintReadOnly, Category = "Hex") TArray<FSalanCard> Cards;

	UPROPERTY(BlueprintAssignable, Category = "Hex") FHexBattleLog OnBattleLog;
	UPROPERTY(BlueprintAssignable, Category = "Hex") FHexBattleChanged OnChanged;

	// ── 라이프사이클 ──
	virtual void BeginPlay() override;
	void StartBattle();

	// ── 좌표/월드 ──
	FVector WorldOfCoord(const FHexCoord& C) const;
	AHexTile* TileAt(const FHexCoord& C) const;
	bool InField(const FHexCoord& C) const;
	AHexUnit* UnitAt(const FHexCoord& C) const;
	bool HasLoS(const FHexCoord& A, const FHexCoord& B) const;

	// ── 컨트롤러가 부르는 입력 API ──
	UFUNCTION(BlueprintCallable, Category = "Hex") void SelectCard(int32 Index);
	UFUNCTION(BlueprintCallable, Category = "Hex") void EnterMoveMode();
	UFUNCTION(BlueprintCallable, Category = "Hex") void Deselect();
	UFUNCTION(BlueprintCallable, Category = "Hex") void ClickCoord(const FHexCoord& C);
	UFUNCTION(BlueprintCallable, Category = "Hex") void RequestEndTurn();

	// 선택 상태(컨트롤러 힌트용)
	int32 GetSelectedCard() const { return SelectedCard; }
	bool IsMoveMode() const { return bMoveMode; }
	static bool CardNeedsTarget(const FSalanCard& Card); // 이 기술이 판에서 대상 칸을 고르게 하는가(UI 안내용)

	// ── HUD용 읽기 API ──
	const TArray<FString>& GetLogLines() const { return LogLines; }
	bool CanStartCardIndex(int32 Index) const;
	bool IsCharging() const { return Charge.bActive; }
	bool IsPlayerCharging() const { return Charge.bActive && Charge.bWhoPlayer; } // 영창 주체(색 구분용)
	float ChargeFraction() const { return Charge.Need > 0 ? (float)Charge.Acc / (float)Charge.Need : 0.0f; } // 진행 바용
	FString ChargeText() const;
	FString ElementLabelPub(ESalanElement E) const { return ElementLabel(E); }

	// 새 판(리셋) — 타일/유닛 파괴 후 재생성
	UFUNCTION(BlueprintCallable, Category = "Hex") void Restart();

private:
	// ── 내부 상태 ──
	UPROPERTY() TMap<FHexCoord, TObjectPtr<AHexTile>> Tiles;
	TMap<FHexCoord, ESurface> Surf;
	TMap<FHexCoord, int32> Walls;      // 값=수명 턴
	TMap<FHexCoord, FJinData> Jin;
	FChargeState Charge;

	int32 Refund = 0;
	ESalanElement LastElement = ESalanElement::Body;
	bool bHasLastElement = false;

	int32 SelectedCard = -1;
	bool bMoveMode = false;

	// 로그 링버퍼(HUD 표시용, 최근 우선)
	TArray<FString> LogLines;

	// 연출형 AI 턴(타이머 스텝)
	FTimerHandle AITimer;
	int32 AIGuard = 0;

	// ── 레벨 ──
	UPROPERTY() TArray<FHexLevel> Levels;
	void BuildLevels();               // 라셀 현대 배경 레벨 표를 채운다
	const FHexLevel& CurrentLevel() const;

	// ── 빌드 ──
	void BuildGrid();
	void BuildCards();
	void SpawnUnits();
	void SpawnCharacterFor(AHexUnit* Unit, const TCHAR* BPPath); // 실제 캐릭터 BP를 유닛 몸으로 스폰

	// ── 규칙 ──
	void Log(const FString& Msg);
	void ApplySurface(const FHexCoord& C, ESurface S);
	void FreezeIfOn(const FHexCoord& C);
	void DealDamage(AHexUnit* U, int32 Amount, const FString& Tag);
	void ApplyCC(AHexUnit* U, int32 SlowDur, int32 FreezeDur);
	void CheckJin(AHexUnit* U);
	bool IsChainCard(const FSalanCard& Card) const;
	void ChainRefund(ESalanElement El, bool bPlayer);
	bool CanStartCard(const FSalanCard& Card) const;
	bool TargetOK(const FSalanCard& Card, const FHexCoord& Target, const FHexCoord& From, FString& OutMsg) const;
	void ResolveCard(const FSalanCard& Card, const FHexCoord& Target, bool bActorPlayer);
	void PlayCard(int32 Index, const FHexCoord& Target);

	// ── 턴 진행 ──
	void StartTurn(bool bPlayer);
	void ResolveOwnCharge(bool bPlayer);
	void SurfaceTick();
	void StartPlayerTurn();
	void EndPlayerTurn();
	void AIBegin();
	void ScheduleAIStep();
	void AIStep();
	bool DoOneEnemyAction();
	void FinishAI();

	// ── 비주얼 ──
	void RefreshUnitWorld(AHexUnit* U);
	void RefreshTileFlags();
	void UpdateHighlights();
	void Changed();

	static FString Short(const AHexUnit* U);
	FString ElementLabel(ESalanElement E) const;
};
