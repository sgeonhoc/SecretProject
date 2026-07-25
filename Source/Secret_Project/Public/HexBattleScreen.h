#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "HexBattleScreen.generated.h"

class AHexGridManager;
class UHexCardWidget;
class UCanvasPanel;
class UHorizontalBox;
class UScrollBox;
class UProgressBar;
class UTextBlock;
class UBorder;
class UButton;

/**
 * 헥스 전투 메인 화면(UMG, 3D 판 위 오버레이). 순수 C++로 트리를 짓는다:
 *  - 좌상단 유닛 패널(HP 게이지 부드럽게 채워짐) · 칸 도트 · 턴/영창 배너 · 전투 기록
 *  - 발현방식 필터 탭(전체/소리길/몸길/그림길/매개길) — 누르면 그 길 기술만 패에 딜인
 *  - 하단 손패: 카드가 아래에서 스르륵 들어오고(딜인), 호버하면 떠오르며 우측에 상세 카드
 *  - 사용 가능한 기술은 떠오르고 또렷, 불가는 가라앉고 흐림 · 선택 카드는 금빛 맥동
 * 빈 영역 클릭은 아래 3D 판으로 통과(SelfHitTestInvisible), 카드/버튼만 입력을 먹는다.
 */
UCLASS()
class SECRET_PROJECT_API UHexBattleScreen : public UPersonaWidgetBase
{
	GENERATED_BODY()

public:
	void Init(AHexGridManager* InGrid);

	// 카드 위젯이 부른다
	void OnCardClicked(int32 CardIndex);
	void ShowInspect(int32 CardIndex);
	void OnCardHover(int32 CardIndex); // -1=없음. 이웃 카드를 옆으로 밀어 가림 방지(하스스톤식)
	void ToggleSalan(int32 PoolIndex); // 살란 카드가 부른다 — 조합에 넣고 뺀다

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override; // Slate 빌드 직전에 트리 구성(핵심)
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY() TObjectPtr<AHexGridManager> Grid;

	// 지속 위젯
	UPROPERTY() TObjectPtr<UCanvasPanel> Root;
	UPROPERTY() TObjectPtr<UProgressBar> EnemyHp;
	UPROPERTY() TObjectPtr<UProgressBar> PlayerHp;
	UPROPERTY() TObjectPtr<UTextBlock> EnemyName;
	UPROPERTY() TObjectPtr<UTextBlock> PlayerName;
	UPROPERTY() TObjectPtr<UTextBlock> EnemyHpText;
	UPROPERTY() TObjectPtr<UTextBlock> PlayerHpText;
	UPROPERTY() TObjectPtr<UTextBlock> EnemyStatus;
	UPROPERTY() TObjectPtr<UTextBlock> PlayerStatus;
	UPROPERTY() TObjectPtr<UTextBlock> TurnText;
	UPROPERTY() TObjectPtr<UTextBlock> TurnBanner; // 새 라운드에 크게 떴다 위로 사라지는 배너
	UPROPERTY() TObjectPtr<UTextBlock> LogHeadText; // 가장 최근 사건 한 줄(밝게 강조)
	UPROPERTY() TObjectPtr<UTextBlock> LogText;
	UPROPERTY() TObjectPtr<UHorizontalBox> KanBox;
	UPROPERTY() TObjectPtr<UTextBlock> KanText;
	UPROPERTY() TObjectPtr<UProgressBar> ChargeBar; // 대주문 영창 진행(무방비 창) 시각화
	UPROPERTY() TArray<TObjectPtr<UBorder>> KanDots;
	UPROPERTY() TObjectPtr<UButton> EndBtn;      // 턴 넘기기 — 내 턴에만 활성
	UPROPERTY() TObjectPtr<UTextBlock> EndBtnText; // 적 턴엔 "적이 두는 중…"으로 바뀜
	UPROPERTY() TObjectPtr<UButton> NewBtn;      // 새 판 — 판 끝나면 맥동으로 강조
	UPROPERTY() TObjectPtr<UHorizontalBox> FilterBox;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> FilterLabels; // 활성 탭 강조용
	UPROPERTY() TObjectPtr<UScrollBox> HandScroll;
	UPROPERTY() TObjectPtr<UHorizontalBox> HandBox;
	UPROPERTY() TArray<TObjectPtr<UHexCardWidget>> CardWidgets;

	// 상세(호버) 패널
	UPROPERTY() TObjectPtr<UBorder> InspectBorder;
	UPROPERTY() TObjectPtr<UTextBlock> InspectTitle;
	UPROPERTY() TObjectPtr<UTextBlock> InspectBody;

	// 살란 조합대(우하단) — 조각 카드를 부챗살로 펴고, 조합하면 언령 기술이 활성화된다
	UPROPERTY() TObjectPtr<class UHorizontalBox> SalanBox;
	UPROPERTY() TArray<TObjectPtr<class USalanCardWidget>> SalanCards;
	UPROPERTY() TObjectPtr<UTextBlock> ComboText;
	TArray<FString> SalanPool;                 // 손에 든 살란 조각들
	TArray<FString> Combo;                     // 지금 짠 조합
	TMap<FString, FString> RecipeByName;        // 기술명 → 필요한 살란 토큰(공백 구분)

	void BuildSalan();
	void BuildSalanCards();                     // 우하단 살란 조각 카드(부챗살)를 런타임에 채운다
	bool IsActivated(int32 CardIndex) const;    // 언령=레시피⊆조합, 그 외=항상 참
	FString ActivatedNames() const;             // 지금 조합이 켜는 언령 기술 이름들(", " 이음)
	void RefreshCombo();                        // 조합 표시 + 기술 활성 갱신 + 칩 하이라이트
	void FlashCast(const FString& TechName);    // 발현 성공 연출 시작(칩이 기술로 합쳐짐)
	UFUNCTION() void HandleSalanChip(int32 PoolIndex);
	UFUNCTION() void ClearComboClicked();

	// 상태
	bool bBuilt = false;
	bool bBound = false;
	int32 FilterIndex = 0; // 0=전체,1=소리,2=몸,3=그림,4=매개
	float EnemyHpShown = 1.0f;
	float PlayerHpShown = 1.0f;
	float HpPulsePhase = 0.0f; // 위급 HP(30% 아래) 붉은 맥동 위상
	float BannerTimer = 0.0f;  // 새 라운드 배너 남은 시간(초)
	float InspectTargetOpacity = 0.0f;
	float InspectCurOpacity = 0.0f;
	int32 LastRoundSeen = -1; // 새 턴 감지 → 드로우 연출
	int32 HoverCardIndex = -1; // 지금 호버한 카드(칸 소모 미리보기용)
	float CastFlash = 0.0f;   // >0이면 발현 성공 연출 진행 중(초 단위로 감쇠)
	FString CastFlashName;    // 방금 발현한 기술 이름

	void EnsureGrid();
	void BuildTree();
	void RebuildHand();
	void RefreshStates();
	bool CardPassesFilter(int32 CardIndex) const;

	UFUNCTION() void HandleChanged();
	UFUNCTION() void HandleFilter(int32 Index);
	UFUNCTION() void EndTurnClicked();
	UFUNCTION() void NewClicked();
	UFUNCTION() void DeselectClicked();

	UButton* MakeTextButton(const FString& Label, const FLinearColor& Col);
	UTextBlock* MakeText(int32 FontSize, const FLinearColor& Col, bool bWrap = false);
};
