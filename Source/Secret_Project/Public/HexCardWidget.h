#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HexTypes.h"
#include "HexCardWidget.generated.h"

class UBorder;
class UTextBlock;
class UHexBattleScreen;

/**
 * 손패 카드 한 장(UMG). C++로 트리를 짓고 NativeTick으로 연출한다:
 *  - 딜인(패에 들어오는 연출): 아래에서 스르륵 + 페이드, 인덱스별 stagger.
 *  - 호버(자세히 보면 자세히): 위로 떠오르며 확대 + 화면에 큰 상세 패널.
 *  - 선택 연출: 금빛 글로우 맥동.
 *  - 사용 가능(선택가능한 기술 부각): 살짝 떠오름 + 또렷 / 불가면 가라앉고 흐림.
 */
UCLASS()
class SECRET_PROJECT_API UHexCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetupCard(UHexBattleScreen* InOwner, int32 InCardIndex);
	void UpdateFromCard(const FSalanCard& Card, bool bInCastable, bool bInSelected);
	void SetReason(const FString& R); // 못 내는 이유 뱃지("칸 부족"/"재료 부족") — 빈 문자열이면 숨김(설계 §6)
	void PlayDeal(float Delay);
	void PlayReject(); // 못 내는 카드를 눌렀을 때 — 좌우로 짧게 흔들리고 붉은 테두리 한 번(설계 §5)
	void SetFanAngle(float Deg) { FanAngle = Deg; } // 손패 부채꼴 기울기
	void SetShove(float X) { ShoveTargetX = X; }    // 이웃 호버 시 옆으로 비켜서는 목표 오프셋

	int32 GetCardIndex() const { return CardIndex; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override; // Slate 빌드 직전에 트리 구성
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void BuildTree();

	UPROPERTY() TWeakObjectPtr<UHexBattleScreen> Owner;
	int32 CardIndex = -1;

	// 트리 참조
	UPROPERTY() TObjectPtr<UBorder> GlowBorder;
	UPROPERTY() TObjectPtr<UBorder> BodyBorder;
	UPROPERTY() TObjectPtr<UBorder> Stripe;
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UTextBlock> CostText;
	UPROPERTY() TObjectPtr<UTextBlock> MetaText;
	UPROPERTY() TObjectPtr<UTextBlock> EffectText;
	UPROPERTY() TObjectPtr<UTextBlock> PathText;

	// 상태
	bool bBuilt = false;
	bool bCastable = true;
	bool bSelected = false;
	bool bHover = false;
	FLinearColor PathCol = FLinearColor::White;

	// 연출 값
	float DealTimer = 0.0f;
	float DealDelay = 0.0f;
	bool bDealing = false;
	float CurY = 120.0f;
	float CurScale = 0.8f;
	float CurOpacity = 0.0f;
	float GlowPhase = 0.0f;
	float FanAngle = 0.0f;  // 손패에서 이 카드가 부챗살처럼 기운 목표 각(도)
	float CurAngle = 0.0f;  // 부드럽게 추종하는 현재 각
	FLinearColor TierCol = FLinearColor(0.35f, 0.38f, 0.42f); // 마디(T)→등급 테두리 색
	float ShoveTargetX = 0.0f; // 이웃 카드 호버 시 옆으로 비켜서는 목표 X
	float CurShoveX = 0.0f;    // 부드럽게 추종하는 현재 X
	float RejectTimer = 0.0f;  // >0이면 거부 흔들림 진행 중(초)
};
