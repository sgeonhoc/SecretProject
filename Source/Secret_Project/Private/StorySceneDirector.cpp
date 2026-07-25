#include "StorySceneDirector.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"

AStorySceneDirector::AStorySceneDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

AStorySceneDirector* AStorySceneDirector::Play(UWorld* World, APlayerController* PC,
    const FText& Card, const TArray<FStoryLine>& InLines, FSimpleDelegate OnDone)
{
    if (!World || !PC)
    {
        OnDone.ExecuteIfBound();
        return nullptr;
    }

    // 카드도 대사도 없으면 장면이 아니다 — 그냥 끝났다고 알린다.
    if (Card.IsEmpty() && InLines.Num() == 0)
    {
        OnDone.ExecuteIfBound();
        return nullptr;
    }

    FActorSpawnParameters Sp;
    Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AStorySceneDirector* D = World->SpawnActor<AStorySceneDirector>(AStorySceneDirector::StaticClass(), Sp);
    if (!D)
    {
        OnDone.ExecuteIfBound();
        return nullptr;
    }

    D->Player = PC;
    D->CardText = Card;
    D->Lines = InLines;
    D->OnFinished = OnDone;

    // 장면 동안은 걷지도 둘러보지도 않는다 — 대사 중에 캐릭터가 걸어 나가면 장면이 아니라 사고다.
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);

    D->BuildOverlay();
    D->Phase = Card.IsEmpty() ? EPhase::CardOut : EPhase::Card;
    D->BlackAlpha = Card.IsEmpty() ? 0.f : 1.f;
    D->CardVisible = Card;
    if (Card.IsEmpty()) D->NextLine();

    return D;
}

void AStorySceneDirector::BuildOverlay()
{
    if (Overlay.IsValid() || !GEngine || !GEngine->GameViewport) return;

    // 한글 — 엔진 Roboto(합성 폰트에 CJK 폴백이 붙어 있다). 프로젝트 Font_NotoKR은
    // C++ Slate에서 물리면 전부 네모로 나온다(2026-07-24 실기 확인).
    FSlateFontInfo CardFont = FCoreStyle::GetDefaultFontStyle("Regular", 34);
    FSlateFontInfo BodyFont = FCoreStyle::GetDefaultFontStyle("Regular", 24);
    FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Regular", 20);
    FSlateFontInfo HintFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
    if (UObject* Roboto = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto")))
    {
        CardFont = FSlateFontInfo(Roboto, 34);
        BodyFont = FSlateFontInfo(Roboto, 24);
        NameFont = FSlateFontInfo(Roboto, 20);
        HintFont = FSlateFontInfo(Roboto, 14);
    }

    TWeakObjectPtr<AStorySceneDirector> Self(this);
    auto Black = [Self]() { return FSlateColor(FLinearColor(0, 0, 0, Self.IsValid() ? Self->BlackAlpha : 0.f)); };
    auto Bar = [Self]() { return FSlateColor(FLinearColor(0, 0, 0, Self.IsValid() ? Self->LetterboxAlpha : 0.f)); };
    auto BoxTint = [Self]() { return FSlateColor(FLinearColor(0.02f, 0.02f, 0.03f, Self.IsValid() ? Self->BoxAlpha * 0.86f : 0.f)); };
    auto CardTint = [Self]()
    {
        const float A = Self.IsValid() ? Self->BlackAlpha : 0.f;
        return FSlateColor(FLinearColor(0.95f, 0.93f, 0.87f, A));
    };
    auto BodyTint = [Self]() { return FSlateColor(FLinearColor(0.94f, 0.93f, 0.90f, Self.IsValid() ? Self->BoxAlpha : 0.f)); };
    auto NameTint = [Self]() { return FSlateColor(FLinearColor(0.98f, 0.82f, 0.40f, Self.IsValid() ? Self->BoxAlpha : 0.f)); };
    auto HintTint = [Self]()
    {
        // 다 드러난 줄에서만 "▸ Space" 힌트가 뜬다 — 아직 흐르는 중이면 안 뜬다.
        const bool bReady = Self.IsValid() && Self->Revealed >= 1.f;
        return FSlateColor(FLinearColor(0.75f, 0.72f, 0.66f, (bReady && Self.IsValid()) ? Self->BoxAlpha * 0.75f : 0.f));
    };

    Overlay = SNew(SOverlay)

        // 위아래 검은 띠 — 지금은 조작이 아니라 장면이라는 표시
        + SOverlay::Slot().VAlign(VAlign_Top)
        [
            SNew(SBox).HeightOverride(78.f)
            [
                SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity_Lambda(Bar)
            ]
        ]
        + SOverlay::Slot().VAlign(VAlign_Bottom)
        [
            SNew(SBox).HeightOverride(78.f)
            [
                SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity_Lambda(Bar)
            ]
        ]

        // 대사 상자 (아래쪽)
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(0, 0, 0, 96))
        [
            SNew(SBox).WidthOverride(1080.f)
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                    SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity_Lambda(BoxTint)
                ]
                + SOverlay::Slot().Padding(FMargin(30, 20, 30, 20))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 0, 0, 8))
                    [
                        SNew(STextBlock).Font(NameFont).ColorAndOpacity_Lambda(NameTint)
                        .Text_Lambda([Self]() { return Self.IsValid() ? Self->VisibleSpeaker : FText::GetEmpty(); })
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(STextBlock).Font(BodyFont).ColorAndOpacity_Lambda(BodyTint)
                        .AutoWrapText(true).LineHeightPercentage(1.35f)
                        .Text_Lambda([Self]() { return Self.IsValid() ? Self->VisibleText : FText::GetEmpty(); })
                    ]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(FMargin(0, 10, 0, 0))
                    [
                        SNew(STextBlock).Font(HintFont).ColorAndOpacity_Lambda(HintTint)
                        .Text(FText::FromString(TEXT("▸  Space")))
                    ]
                ]
            ]
        ]

        // 자막 카드 — 검은 막 + 가운데 글(때·자리)
        + SOverlay::Slot()
        [
            SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush")).ColorAndOpacity_Lambda(Black)
        ]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(STextBlock).Font(CardFont).Justification(ETextJustify::Center).LineHeightPercentage(1.4f)
            .ColorAndOpacity_Lambda(CardTint)
            .Text_Lambda([Self]() { return Self.IsValid() ? Self->CardVisible : FText::GetEmpty(); })
        ];

    GEngine->GameViewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 6);
}

void AStorySceneDirector::RemoveOverlay()
{
    if (Overlay.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
    Overlay.Reset();
}

void AStorySceneDirector::NextLine()
{
    ++LineIndex;
    if (!Lines.IsValidIndex(LineIndex))
    {
        Finish();
        return;
    }

    Phase = EPhase::Line;
    PhaseTime = 0.f;
    Revealed = 0.f;
    FullLine = Lines[LineIndex].Line;
    SpeakerName = Lines[LineIndex].Speaker;
    VisibleSpeaker = SpeakerName.IsEmpty() ? FText::GetEmpty() : FText::FromString(SpeakerName);
    VisibleText = FText::GetEmpty();
}

void AStorySceneDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    PhaseTime += DeltaSeconds;

    // 넘기기 입력 — Space / 좌클릭 / Enter. 흐르는 중이면 한 줄을 즉시 다 드러내고,
    // 다 드러났으면 다음 줄로.
    bool bAdvance = false;
    if (Player)
    {
        bAdvance = Player->WasInputKeyJustPressed(EKeys::SpaceBar)
            || Player->WasInputKeyJustPressed(EKeys::LeftMouseButton)
            || Player->WasInputKeyJustPressed(EKeys::Enter)
            || Player->WasInputKeyJustPressed(EKeys::E);
    }

    switch (Phase)
    {
    case EPhase::Card:
    {
        BlackAlpha = 1.f;
        LetterboxAlpha = 0.f;
        if (PhaseTime >= CardHold || bAdvance)
        {
            Phase = EPhase::CardOut;
            PhaseTime = 0.f;
        }
        break;
    }

    case EPhase::CardOut:
    {
        // 카드가 걷히며 화면이 열리고, 위아래 띠가 들어온다.
        BlackAlpha = FMath::Clamp(1.f - PhaseTime / CardFade, 0.f, 1.f);
        LetterboxAlpha = FMath::Clamp(PhaseTime / CardFade, 0.f, 1.f) * 0.92f;
        if (PhaseTime >= CardFade)
        {
            BlackAlpha = 0.f;
            NextLine();
        }
        break;
    }

    case EPhase::Line:
    {
        BoxAlpha = FMath::Clamp(PhaseTime / 0.25f, 0.f, 1.f);
        LetterboxAlpha = 0.92f;

        const int32 Total = FullLine.Len();
        const int32 Show = FMath::Min(Total, FMath::FloorToInt(PhaseTime * CharsPerSec));
        Revealed = (Total <= 0) ? 1.f : (float)Show / (float)Total;

        if (bAdvance && Revealed < 1.f)
        {
            // 흐르는 중에 누르면 그 줄을 다 드러낸다(빨리 읽는 사람이 기다리지 않게).
            Revealed = 1.f;
            VisibleText = FText::FromString(FullLine);
            PhaseTime = Total / CharsPerSec;
            break;
        }

        VisibleText = FText::FromString(FullLine.Left(Revealed >= 1.f ? Total : Show));

        if (bAdvance && Revealed >= 1.f)
            NextLine();
        break;
    }

    default:
        break;
    }
}

void AStorySceneDirector::Finish()
{
    if (Phase == EPhase::Done) return;
    Phase = EPhase::Done;

    // 조작을 돌려준다. 오프닝과 같은 두 줄이 필요하다 — 오버레이를 떼면 에디터 PIE에서
    // 뷰포트가 키보드 포커스를 잃어, 이걸 안 하면 장면 뒤에 캐릭터가 안 움직인다.
    if (Player)
    {
        Player->SetIgnoreMoveInput(false);
        Player->SetIgnoreLookInput(false);
        FInputModeGameOnly Mode;
        Player->SetInputMode(Mode);
        Player->bShowMouseCursor = false;
    }
    if (FSlateApplication::IsInitialized())
        FSlateApplication::Get().SetAllUserFocusToGameViewport();

    RemoveOverlay();
    SetActorTickEnabled(false);

    OnFinished.ExecuteIfBound();
    Destroy();
}

void AStorySceneDirector::EndPlay(const EEndPlayReason::Type Reason)
{
    RemoveOverlay();
    Super::EndPlay(Reason);
}
