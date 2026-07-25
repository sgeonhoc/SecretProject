#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SalanCardWidget.generated.h"

class UBorder;
class UTextBlock;
class UHexBattleScreen;

/**
 * 살란 조각 한 장(UMG 카드). 기술 손패와 같은 결의 카드로, 우하단에 부챗살로 펼쳐진다:
 *  - 큰 살란 글자 + 뜻(불/서리/여럿/이루라 …) + 계열 색띠.
 *  - 누르면 조합에 넣고 뺀다(토글). 조합에 든 조각은 떠오르고 초록빛으로 켜진다.
 *  - 딜인·호버·부채 기울기는 기술 카드와 같은 방식(NativeTick render transform).
 */
UCLASS()
class SECRET_PROJECT_API USalanCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UHexBattleScreen* InOwner, int32 InPoolIndex, const FString& Token, const FString& Meaning, const FLinearColor& InAccent);
	void SetPicked(bool bInPicked);      // 조합에 들어있는지
	void SetFanAngle(float Deg) { FanAngle = Deg; }
	void PlayDeal(float Delay);
	int32 GetPoolIndex() const { return PoolIndex; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void BuildTree();

	UPROPERTY() TWeakObjectPtr<UHexBattleScreen> Owner;
	int32 PoolIndex = -1;
	FString TokenStr;
	FString MeaningStr;
	FLinearColor Accent = FLinearColor::White;

	UPROPERTY() TObjectPtr<UBorder> GlowBorder;
	UPROPERTY() TObjectPtr<UBorder> BodyBorder;
	UPROPERTY() TObjectPtr<UBorder> Stripe;
	UPROPERTY() TObjectPtr<UTextBlock> TokenText;
	UPROPERTY() TObjectPtr<UTextBlock> MeaningText;

	bool bBuilt = false;
	bool bPicked = false;
	bool bHover = false;

	float FanAngle = 0.0f;
	float CurAngle = 0.0f;
	float CurY = 120.0f;
	float CurScale = 0.8f;
	float CurOpacity = 0.0f;
	float GlowPhase = 0.0f;
	float DealTimer = 0.0f;
	float DealDelay = 0.0f;
	bool bDealing = false;
};
