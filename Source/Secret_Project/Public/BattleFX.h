#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraShakeBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BattleFX.generated.h"

class UTextBlock;
class UBorder;

/**
 * 전투 타격감 연출 모듈 (코드 전용, 아트 에셋 불필요).
 *  - UBattleHitShake : 타격 시 카메라 흔들림(웨이브 오실레이터, 에셋 없이 C++ 생성)
 *  - UDamageNumberWidget : 적 위로 솟아올라 사라지는 데미지 숫자 (WBP_DamageNumber로 생성)
 *  - UBattleFlairWidget : WEAK!/CRITICAL!/1 MORE! 등 페르소나식 화면 배너 (WBP_BattleFlair)
 *  - UBattleFXLibrary : 히트스톱/카메라셰이크/화면플래시 정적 헬퍼
 */

// ── 카메라 흔들림 ─────────────────────────────────────────
UCLASS()
class SECRET_PROJECT_API UBattleHitShake : public UCameraShakeBase
{
    GENERATED_BODY()
public:
    UBattleHitShake(const FObjectInitializer& OI);
};

// ── 데미지 숫자 (떠오르며 페이드) ─────────────────────────
UCLASS()
class SECRET_PROJECT_API UDamageNumberWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    // 대상 월드 위치 위로 숫자를 띄운다. Big=크리티컬/약점 강조(크게+팝).
    void Init(FVector WorldLoc, const FString& Text, FLinearColor Color, bool bBig);

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_Number;

private:
    FVector2D ScreenPos = FVector2D::ZeroVector;
    float Elapsed = 0.f;
    float Life = 0.9f;
    float RiseSpeed = 90.f;   // 픽셀/초
    float DriftX = 0.f;       // 가로 분산 속도(픽셀/초) — 연타/AoE 숫자가 겹치지 않게 좌우로 퍼짐
    bool  bBigPop = false;
    bool  bPlaced = false;
};

// ── 페르소나식 플레어 배너 (WEAK!/CRITICAL!/1 MORE! 슬라이드 인) ───────────
UCLASS()
class SECRET_PROJECT_API UBattleFlairWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    // 배너 텍스트 + 강조색. bBig=총공격/즉사 등 초강조(더 크게/길게).
    //  StackY=세로 누적 오프셋(같은 순간 여러 배너가 뜨면 겹치지 않게 아래로 밀어줌).
    void Init(const FString& Text, FLinearColor Accent, bool bBig, float StackY = 0.f);

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Flair;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder>    Bg_Flair;

private:
    float Elapsed = 0.f;
    float Life = 1.0f;
    float StackOffsetY = 0.f;  // 다발 배너 세로 분리
    bool  bBig = false;
};

// ── 정적 헬퍼 ─────────────────────────────────────────────
UCLASS()
class SECRET_PROJECT_API UBattleFXLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // 타격 순간 시간을 잠깐 멈춤(글로벌 타임딜레이션). RealSeconds=실제 멈춤 시간.
    UFUNCTION(BlueprintCallable, Category = "BattleFX")
    static void HitStop(UObject* WorldContext, float RealSeconds = 0.05f, float Dilation = 0.04f);

    // 카메라 흔들림. Scale 1=기본, 크게 줄수록 강함.
    UFUNCTION(BlueprintCallable, Category = "BattleFX")
    static void ShakeCamera(UObject* WorldContext, float Scale = 1.f);

    // 화면 컬러 플래시(카메라 페이드, 위젯 불필요). FromAlpha→0으로 Duration 동안.
    UFUNCTION(BlueprintCallable, Category = "BattleFX")
    static void FlashScreen(UObject* WorldContext, FLinearColor Color, float FromAlpha = 0.55f, float Duration = 0.25f);
};
