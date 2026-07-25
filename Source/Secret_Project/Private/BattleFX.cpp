#include "BattleFX.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Shakes/WaveOscillatorCameraShakePattern.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "TimerManager.h"
#include "Engine/World.h"

// ── 카메라 흔들림 ─────────────────────────────────────────
UBattleHitShake::UBattleHitShake(const FObjectInitializer& OI)
    : Super(OI)
{
    UWaveOscillatorCameraShakePattern* P =
        OI.CreateDefaultSubobject<UWaveOscillatorCameraShakePattern>(this, TEXT("HitShakePattern"));
    if (P)
    {
        P->Duration = 0.22f;
        P->BlendInTime = 0.02f;
        P->BlendOutTime = 0.12f;
        // 회전 흔들림(피치/요) — 빠르고 짧게
        P->Pitch.Amplitude = 1.6f;  P->Pitch.Frequency = 28.f;
        P->Yaw.Amplitude   = 1.4f;  P->Yaw.Frequency   = 24.f;
        P->Roll.Amplitude  = 0.8f;  P->Roll.Frequency  = 20.f;
        // 위치 흔들림(가벼운 좌우상하)
        P->X.Amplitude = 3.f;  P->X.Frequency = 26.f;
        P->Y.Amplitude = 3.f;  P->Y.Frequency = 22.f;
        SetRootShakePattern(P);
    }
}

// ── 데미지 숫자 ───────────────────────────────────────────
void UDamageNumberWidget::Init(FVector WorldLoc, const FString& Text, FLinearColor Color, bool bBig)
{
    bBigPop = bBig;
    Life = bBig ? 1.05f : 0.85f;
    RiseSpeed = bBig ? 120.f : 90.f;
    // 가로 분산: 같은 대상에 숫자가 연달아 뜰 때(연타/AoE) 좌우로 부채꼴로 퍼지게.
    DriftX = FMath::FRandRange(-55.f, 55.f);

    if (Txt_Number)
    {
        Txt_Number->SetText(FText::FromString(Text));
        Txt_Number->SetColorAndOpacity(FSlateColor(Color));
        FSlateFontInfo F = Txt_Number->GetFont();
        F.Size = bBig ? 48 : 30;
        Txt_Number->SetFont(F);
        Txt_Number->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
        Txt_Number->SetShadowOffset(FVector2D(2.f, 2.f));
    }

    if (APlayerController* PC = GetOwningPlayer())
    {
        FVector2D SP;
        if (UGameplayStatics::ProjectWorldToScreen(PC, WorldLoc, SP))
        {
            // DPI 보정 후 좌표로 변환
            const float DPI = UWidgetLayoutLibrary::GetViewportScale(this);
            ScreenPos = (DPI > 0.f) ? SP / DPI : SP;
            bPlaced = true;
        }
    }
    if (!bPlaced)
        ScreenPos = FVector2D(960.f, 400.f); // 폴백(중앙 상단)
    SetPositionInViewport(ScreenPos, false);
}

void UDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    Elapsed += InDeltaTime;
    const float a = (Life > 0.f) ? (Elapsed / Life) : 1.f;
    if (a >= 1.f)
    {
        RemoveFromParent();
        return;
    }
    // 위로 상승 + 가로 분산(초반에 빠르게 퍼지고 점차 감속 → 부채꼴 아크)
    ScreenPos.Y -= RiseSpeed * InDeltaTime;
    ScreenPos.X += DriftX * FMath::Max(0.f, 1.f - a) * InDeltaTime;
    SetPositionInViewport(ScreenPos, false);
    // 후반부 페이드아웃
    SetRenderOpacity(a < 0.55f ? 1.f
        : FMath::GetMappedRangeValueClamped(FVector2D(0.55f, 1.f), FVector2D(1.f, 0.f), a));
    // 등장 팝 스케일
    const float pop = bBigPop ? 1.7f : 1.3f;
    const float s = (a < 0.16f) ? FMath::Lerp(pop, 1.f, a / 0.16f) : 1.f;
    SetRenderScale(FVector2D(s, s));
}

// ── 페르소나식 플레어 배너 ────────────────────────────────
void UBattleFlairWidget::Init(const FString& Text, FLinearColor Accent, bool bInBig, float StackY)
{
    bBig = bInBig;
    Life = bBig ? 1.15f : 0.92f;
    StackOffsetY = StackY;

    if (Txt_Flair)
    {
        Txt_Flair->SetText(FText::FromString(Text));
        Txt_Flair->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        FSlateFontInfo F = Txt_Flair->GetFont();
        F.Size = bBig ? 56 : 42;
        Txt_Flair->SetFont(F);
        Txt_Flair->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.95f));
        Txt_Flair->SetShadowOffset(FVector2D(2.5f, 2.5f));
    }
    if (Bg_Flair)
        Bg_Flair->SetBrushColor(FLinearColor(Accent.R, Accent.G, Accent.B, 0.96f));

    SetRenderOpacity(0.f);
}

void UBattleFlairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    Elapsed += InDeltaTime;
    const float a = (Life > 0.f) ? (Elapsed / Life) : 1.f;
    if (a >= 1.f) { RemoveFromParent(); return; }

    // 슬라이드: 좌측 밖 → 정위치(0~0.16) → 유지(~0.72) → 우측 밖(0.72~1)
    float slideX, op, scale;
    if (a < 0.16f)            // 진입: 왼쪽에서 빠르게 + 팝
    {
        const float t = a / 0.16f;
        const float e = 1.f - FMath::Pow(1.f - t, 3.f);   // ease-out
        slideX = FMath::Lerp(-720.f, 0.f, e);
        op = FMath::Clamp(t * 1.4f, 0.f, 1.f);
        scale = FMath::Lerp(1.35f, 1.f, e);
    }
    else if (a < 0.72f)       // 유지
    {
        slideX = 0.f; op = 1.f; scale = 1.f;
    }
    else                      // 퇴장: 오른쪽으로 가속 + 페이드
    {
        const float t = (a - 0.72f) / 0.28f;
        const float e = t * t;
        slideX = FMath::Lerp(0.f, 900.f, e);
        op = FMath::Clamp(1.f - t, 0.f, 1.f);
        scale = 1.f;
    }

    FWidgetTransform T;
    T.Translation = FVector2D(slideX, StackOffsetY);
    T.Scale = FVector2D(scale, scale);
    T.Shear = FVector2D(-7.f, 0.f);    // 페르소나식 이탤릭 사선
    T.Angle = 0.f;
    SetRenderTransform(T);
    SetRenderOpacity(op);
}

// ── 정적 헬퍼 ─────────────────────────────────────────────
void UBattleFXLibrary::HitStop(UObject* WorldContext, float RealSeconds, float Dilation)
{
    UWorld* W = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!W) return;
    Dilation = FMath::Clamp(Dilation, 0.01f, 1.f);
    UGameplayStatics::SetGlobalTimeDilation(W, Dilation);
    // 타이머는 게임시간(딜레이션 적용)으로 흐르므로, RealSeconds*Dilation 만큼 게임초를 주면 실제 RealSeconds에 복원됨.
    FTimerHandle H;
    TWeakObjectPtr<UWorld> WW(W);
    W->GetTimerManager().SetTimer(H, [WW]()
    {
        if (WW.IsValid()) UGameplayStatics::SetGlobalTimeDilation(WW.Get(), 1.f);
    }, FMath::Max(0.001f, RealSeconds * Dilation), false);
}

void UBattleFXLibrary::ShakeCamera(UObject* WorldContext, float Scale)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContext, 0);
    if (PC && PC->PlayerCameraManager)
        PC->PlayerCameraManager->StartCameraShake(UBattleHitShake::StaticClass(), Scale);
}

void UBattleFXLibrary::FlashScreen(UObject* WorldContext, FLinearColor Color, float FromAlpha, float Duration)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContext, 0);
    if (PC && PC->PlayerCameraManager)
        PC->PlayerCameraManager->StartCameraFade(FromAlpha, 0.f, Duration, Color, false, false);
}
