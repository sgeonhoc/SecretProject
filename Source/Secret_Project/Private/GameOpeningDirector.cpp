#include "GameOpeningDirector.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Font.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Framework/Application/SlateApplication.h"

AGameOpeningDirector::AGameOpeningDirector()
{
    PrimaryActorTick.bCanEverTick = true;

    // 프롤로그 — 이 세계가 지금 어떤 꼴인지만 세운다.
    // (검은물이 세상을 훑고 간 뒤 / 파헤친 자리에서 삭지 않은 판이 올라오는 개막 — 현대 정본 §1·§3)
    PrologueLines = {
        FText::FromString(TEXT("검은물이 세상을 한 번 훑고 간 뒤로,\n사람들은 마력을 옛이야기로 배웠다.")),
        FText::FromString(TEXT("그리고 올해, 땅을 파헤친 자리마다\n삭지 않은 판이 올라오기 시작했다.")),
    };
    PlaceCardText = FText::FromString(TEXT("라셀 · 아랫장터\n비 오는 밤"));
    ObjectiveText = FText::FromString(TEXT("저잣거리에 도는 그림을 쫓아라"));
}

void AGameOpeningDirector::BeginPlay()
{
    Super::BeginPlay();
}

void AGameOpeningDirector::EndPlay(const EEndPlayReason::Type Reason)
{
    RemoveOverlay();
    Super::EndPlay(Reason);
}

void AGameOpeningDirector::Begin(APlayerController* PC)
{
    if (!PC) { Destroy(); return; }
    Player = PC;

    // 연출 동안 조작을 막는다 — 검은 화면 뒤에서 캐릭터가 걸어다니면 시작이 아니라 사고다.
    if (APawn* P = PC->GetPawn())
        P->DisableInput(PC);
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);
    PC->bShowMouseCursor = false;

    BuildOverlay();
    SetupSweepCamera();
    EnterPhase(EPhase::Black);
}

/**
 * 화면 위에 얹는 것 — 검은 막 · 가운데 글 · 아래쪽 목표 한 줄.
 * 위젯 에셋(WBP)에 기대지 않는다. 한글이 필요하므로 프로젝트 폰트(NotoKR)를 쓰고, 없으면 엔진 기본으로 물러난다.
 */
void AGameOpeningDirector::BuildOverlay()
{
    if (Overlay.IsValid() || !GEngine || !GEngine->GameViewport) return;

    // 한글 폰트. 엔진 기본(Roboto)에는 한글 글자가 없어 그대로 두면 네모(□)만 뜬다.
    // ①프로젝트가 만들어 둔 Font 에셋 → ②원본 FontFace 에셋 순으로 잡고, 둘 다 없으면 경고를 남긴다.
    FSlateFontInfo BodyFont = FCoreStyle::GetDefaultFontStyle("Regular", 30);
    FSlateFontInfo ObjFont  = FCoreStyle::GetDefaultFontStyle("Regular", 22);

    // ★엔진 Roboto를 쓴다. 이 합성 폰트에는 CJK 폴백이 붙어 있어 한글이 실제로 그려진다.
    //   프로젝트 Font_NotoKR(build_all_ui.py 생성)은 UMG 위젯에선 잘 뜨지만, 여기 Slate에서
    //   FSlateFontInfo로 물리면 타입페이스 이름을 줘도 전부 네모(□)로 나온다 — 실기로 확인함(2026-07-24).
    if (UObject* Roboto = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto")))
    {
        BodyFont = FSlateFontInfo(Roboto, 30);
        ObjFont  = FSlateFontInfo(Roboto, 22);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Opening] 폰트를 못 찾았다 — 프롤로그가 네모로 뜬다."));
    }

    TWeakObjectPtr<AGameOpeningDirector> Self(this);
    auto BlackTint = [Self]() -> FSlateColor
    {
        return FSlateColor(FLinearColor(0.f, 0.f, 0.f, Self.IsValid() ? Self->BlackAlpha : 1.f));
    };
    auto TextTint = [Self]() -> FSlateColor
    {
        const float A = Self.IsValid() ? Self->TextAlpha : 0.f;
        return FSlateColor(FLinearColor(0.94f, 0.92f, 0.86f, A));
    };
    auto ObjTint = [Self]() -> FSlateColor
    {
        const float A = Self.IsValid() ? Self->ObjAlpha : 0.f;
        return FSlateColor(FLinearColor(0.98f, 0.84f, 0.42f, A));
    };
    auto BodyText = [Self]() -> FText
    {
        return Self.IsValid() ? Self->CurrentText : FText::GetEmpty();
    };
    auto ObjText = [Self]() -> FText
    {
        return Self.IsValid() ? Self->ObjectiveText : FText::GetEmpty();
    };

    Overlay = SNew(SOverlay)

        // 검은 막
        + SOverlay::Slot()
        [
            SNew(SImage)
            .Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .ColorAndOpacity_Lambda(BlackTint)
        ]

        // 가운데 — 프롤로그/장소 자막
        + SOverlay::Slot()
        .HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Font(BodyFont)
            .Justification(ETextJustify::Center)
            .LineHeightPercentage(1.45f)
            .ColorAndOpacity_Lambda(TextTint)
            .Text_Lambda(BodyText)
        ]

        // 아래 — 첫 목표 한 줄
        + SOverlay::Slot()
        .HAlign(HAlign_Center).VAlign(VAlign_Bottom)
        .Padding(FMargin(0.f, 0.f, 0.f, 110.f))
        [
            SNew(STextBlock)
            .Font(ObjFont)
            .Justification(ETextJustify::Center)
            .ColorAndOpacity_Lambda(ObjTint)
            .Text_Lambda(ObjText)
        ];

    // ZOrder를 낮게 둬서 시스템 메뉴·대화창이 위로 올라올 수 있게 한다.
    GEngine->GameViewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 5);
}

void AGameOpeningDirector::RemoveOverlay()
{
    if (Overlay.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
    Overlay.Reset();
}

/**
 * 거리를 훑는 카메라를 세운다.
 * 플레이어가 선 자리를 기준으로 잡으므로 레벨의 PlayerStart가 어디로 옮겨져도 따라간다
 * (레벨은 레벨 담당이 계속 고치는 중이라 좌표를 박아 두면 곧 어긋난다).
 */
void AGameOpeningDirector::SetupSweepCamera()
{
    UWorld* W = GetWorld();
    if (!W || !Player) return;

    APawn* P = Player->GetPawn();
    const FVector Anchor = P ? P->GetActorLocation() : FVector::ZeroVector;
    const FRotator Face  = P ? P->GetActorRotation() : FRotator::ZeroRotator;
    const FVector Fwd    = Face.Vector();
    const FVector Right  = FRotationMatrix(Face).GetUnitAxis(EAxis::Y);

    // ① 젖은 포석 가까이, 등롱 쪽을 비스듬히 — 바닥에서 시작해
    SweepStartLoc = Anchor + Fwd * 620.f + Right * 240.f + FVector(0, 0, 55.f);
    SweepStartRot = (Anchor + Fwd * 200.f + FVector(0, 0, 120.f) - SweepStartLoc).Rotation();

    // ② 차양 높이까지 올라와 거리를 훑고, 플레이어 뒤쪽으로 물러난다
    SweepEndLoc = Anchor - Fwd * 320.f + FVector(0, 0, 240.f);
    SweepEndRot = (Anchor + Fwd * 400.f + FVector(0, 0, 90.f) - SweepEndLoc).Rotation();

    FActorSpawnParameters Sp;
    Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SweepCam = W->SpawnActor<ACameraActor>(SweepStartLoc, SweepStartRot, Sp);
    if (SweepCam)
    {
        if (UCameraComponent* Cam = SweepCam->GetCameraComponent())
            Cam->SetFieldOfView(72.f);
        Player->SetViewTarget(SweepCam);
    }
}

void AGameOpeningDirector::EnterPhase(EPhase Next)
{
    Phase = Next;
    PhaseTime = 0.f;

    switch (Phase)
    {
    case EPhase::Prologue:
        LineIndex = 0;
        CurrentText = PrologueLines.IsValidIndex(0) ? PrologueLines[0] : PlaceCardText;
        break;

    case EPhase::Sweep:
        // 장소 자막을 띄운 채로 화면을 연다 — 글이 사라지는 동안 거리가 드러난다.
        CurrentText = PlaceCardText;
        break;

    case EPhase::Handoff:
        HandControlBackToPlayer();
        break;

    case EPhase::Objective:
        CurrentText = FText::GetEmpty();
        break;

    case EPhase::Done:
    {
        RemoveOverlay();
        SetActorTickEnabled(false);
        if (SweepCam) { SweepCam->Destroy(); SweepCam = nullptr; }

        // 오프닝이 끝나야 그 자리의 장면(대사)이 시작된다 — 두 연출이 겹치지 않게.
        FSimpleDelegate Next = OnFinished;
        OnFinished.Unbind();
        Destroy();
        Next.ExecuteIfBound();
        break;
    }

    default:
        break;
    }
}

void AGameOpeningDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    PhaseTime += DeltaSeconds;

    switch (Phase)
    {
    case EPhase::Black:
    {
        BlackAlpha = 1.f;
        TextAlpha = 0.f;
        if (PhaseTime >= BlackHold) EnterPhase(EPhase::Prologue);
        break;
    }

    case EPhase::Prologue:
    {
        // 한 줄이 떴다가 진다. 마지막 줄까지 끝나면 화면을 연다.
        const float In  = FMath::Clamp(PhaseTime / LineFade, 0.f, 1.f);
        const float Out = FMath::Clamp((LineDuration - PhaseTime) / LineFade, 0.f, 1.f);
        TextAlpha = FMath::Min(In, Out);
        BlackAlpha = 1.f;

        if (PhaseTime >= LineDuration)
        {
            ++LineIndex;
            if (PrologueLines.IsValidIndex(LineIndex))
            {
                CurrentText = PrologueLines[LineIndex];
                PhaseTime = 0.f;
            }
            else
            {
                EnterPhase(EPhase::Sweep);
            }
        }
        break;
    }

    case EPhase::Sweep:
    {
        const float T = FMath::Clamp(PhaseTime / FMath::Max(SweepDuration, 0.01f), 0.f, 1.f);
        const float Eased = FMath::InterpEaseInOut(0.f, 1.f, T, 2.f);

        if (SweepCam)
        {
            SweepCam->SetActorLocation(FMath::Lerp(SweepStartLoc, SweepEndLoc, Eased));
            SweepCam->SetActorRotation(FMath::Lerp(SweepStartRot, SweepEndRot, Eased));
        }

        // 검은 막이 걷히는 동안 장소 자막이 같이 사라진다.
        BlackAlpha = FMath::Clamp(1.f - PhaseTime / 2.2f, 0.f, 1.f);
        TextAlpha  = FMath::Clamp(FMath::Min(PhaseTime / LineFade, (2.6f - PhaseTime) / LineFade), 0.f, 1.f);

        if (T >= 1.f) EnterPhase(EPhase::Handoff);
        break;
    }

    case EPhase::Handoff:
    {
        BlackAlpha = 0.f;
        TextAlpha = 0.f;
        ObjAlpha = FMath::Clamp(PhaseTime / 0.8f, 0.f, 1.f);
        if (PhaseTime >= HandoffBlend) EnterPhase(EPhase::Objective);
        break;
    }

    case EPhase::Objective:
    {
        ObjAlpha = FMath::Clamp(FMath::Min(1.f, (ObjectiveHold - PhaseTime) / 1.0f), 0.f, 1.f);
        if (PhaseTime >= ObjectiveHold) EnterPhase(EPhase::Done);
        break;
    }

    default:
        break;
    }
}

void AGameOpeningDirector::HandControlBackToPlayer()
{
    if (!Player) return;

    // 카메라를 플레이어에게 부드럽게 넘긴다 — 뚝 끊기면 연출이 아니라 로딩처럼 보인다.
    // 캐릭터가 3인칭이라 폰 카메라(뒤 300)로 블렌드되며 그대로 3인칭이 이어진다(1인칭 스냅 없음).
    if (APawn* P = Player->GetPawn())
    {
        UE_LOG(LogTemp, Log, TEXT("[Opening] 조작 인계 — 플레이어 %s 위치 %s / 회전 %s"),
            *P->GetName(), *P->GetActorLocation().ToCompactString(), *P->GetActorRotation().ToCompactString());
        Player->SetViewTargetWithBlend(P, HandoffBlend, EViewTargetBlendFunction::VTBlend_Cubic, 2.f);
        P->EnableInput(Player);
    }
    Player->SetIgnoreMoveInput(false);
    Player->SetIgnoreLookInput(false);

    // ★조작이 실제로 먹게 만드는 두 줄.
    //   ①게임 전용 입력 모드로 되돌린다(오프닝 전에 UI 모드였을 수 있다).
    //   ②게임 뷰포트로 키보드 포커스를 강제한다 — 에디터 PIE에선 오버레이를 붙였다 떼면
    //     뷰포트가 포커스를 잃어, 이걸 안 하면 인트로 뒤에 키를 눌러도 캐릭터가 안 움직인다.
    FInputModeGameOnly Mode;
    Player->SetInputMode(Mode);
    Player->bShowMouseCursor = false;
    FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

void AGameOpeningDirector::FinishNow()
{
    if (Phase == EPhase::Done) return;
    HandControlBackToPlayer();
    EnterPhase(EPhase::Done);
}
