#include "DestinationMenu.h"
#include "GameFlowSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"

namespace
{
    // 지금 화면에 떠 있는 이동 맵(하나만). T 토글·중복 방지에 쓴다.
    TWeakPtr<SDestinationMenu> GOpenMenu;

    FSlateFontInfo Roboto(int32 Size)
    {
        if (UObject* R = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto")))
            return FSlateFontInfo(R, Size);
        return FCoreStyle::GetDefaultFontStyle("Regular", Size);
    }
}

void SDestinationMenu::Construct(const FArguments& InArgs)
{
    Player    = InArgs._Player;
    Districts = UGameFlowSubsystem::GetTravelDistricts();

    TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
    for (int32 i = 0; i < Districts.Num(); ++i)
        List->AddSlot().AutoHeight()[ MakeRow(Districts[i], i) ];

    ChildSlot
    [
        SNew(SOverlay)

        // 왼쪽 어두운 천 — 뒤 거리 위에서 글자가 읽히게(오른쪽 거리는 그대로 보인다).
        + SOverlay::Slot().HAlign(HAlign_Left)
        [
            SNew(SBox).WidthOverride(660.f)
            [
                SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .ColorAndOpacity(FLinearColor(0.015f, 0.02f, 0.03f, 0.78f))
            ]
        ]

        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill).Padding(FMargin(86, 64, 0, 56))
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock).Font(Roboto(40))
                .ColorAndOpacity(FSlateColor(FLinearColor(0.97f, 0.86f, 0.55f, 1.f)))
                .Text(FText::FromString(TEXT("어디로 갈까")))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(4, 6, 0, 0))
            [
                SNew(STextBlock).Font(Roboto(15))
                .ColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.64f, 0.68f, 1.f)))
                .Text(FText::FromString(TEXT("라셀 · 오갈 수 있는 구역")))
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(6, 20, 0, 16))
            [
                SNew(SBox).WidthOverride(360.f).HeightOverride(1.f)
                [
                    SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .ColorAndOpacity(FLinearColor(0.75f, 0.66f, 0.42f, 0.5f))
                ]
            ]

            // 구역 목록 — 많아질 수 있으니 스크롤 안에 둔다.
            + SVerticalBox::Slot().FillHeight(1.f)
            [
                SNew(SScrollBox) + SScrollBox::Slot()[ List ]
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(2, 18, 0, 0))
            [
                SNew(STextBlock).Font(Roboto(13))
                .ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.56f, 0.58f, 0.85f)))
                .Text(FText::FromString(TEXT("마우스로 고른다 · T / Esc 로 닫기")))
            ]
        ]
    ];
}

TSharedRef<SWidget> SDestinationMenu::MakeRow(const FTravelDistrict& D, int32 Index)
{
    auto NameTint = [this, Index]()
    {
        return (HoverIndex == Index)
            ? FSlateColor(FLinearColor(1.0f, 0.92f, 0.66f, 1.f))
            : FSlateColor(FLinearColor(0.88f, 0.88f, 0.86f, 1.f));
    };
    auto TickTint = [this, Index]()
    {
        return FSlateColor(FLinearColor(0.98f, 0.80f, 0.36f, HoverIndex == Index ? 1.f : 0.f));
    };

    return SNew(SBox).Padding(FMargin(0, 6, 0, 6))
    [
        SNew(SButton)
        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
        .ContentPadding(FMargin(0))
        .OnHovered_Lambda([this, Index]() { HoverIndex = Index; })
        .OnUnhovered_Lambda([this, Index]() { if (HoverIndex == Index) HoverIndex = -1; })
        .OnClicked_Lambda([this, Index]() { OnRowClicked(Index); return FReply::Handled(); })
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(0, 0, 12, 0))
            [
                SNew(STextBlock).Font(Roboto(22)).ColorAndOpacity_Lambda(TickTint)
                .Text(FText::FromString(TEXT("—")))
            ]
            + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Font(Roboto(22)).ColorAndOpacity_Lambda(NameTint)
                    .Text(FText::FromString(D.Name))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 1, 0, 0))
                [
                    SNew(STextBlock).Font(Roboto(12))
                    .ColorAndOpacity(FSlateColor(FLinearColor(0.56f, 0.58f, 0.62f, 1.f)))
                    .Text(FText::FromString(D.Note))
                ]
            ]
        ]
    ];
}

void SDestinationMenu::OnRowClicked(int32 Index)
{
    if (bTaken || !Districts.IsValidIndex(Index)) return;

    APlayerController* PC = Player.Get();
    UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
    UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr;
    if (!Flow) return;

    const FTravelDistrict& D = Districts[Index];
    bTaken = true;
    Dismiss();
    Flow->TravelToLevel(D.LevelPath, D.EntryTag);   // 장소만 옮긴다(진행은 그대로)
}

FReply SDestinationMenu::OnKeyDown(const FGeometry& Geo, const FKeyEvent& Key)
{
    const FKey K = Key.GetKey();
    if (K == EKeys::Escape || K == EKeys::T)
    {
        Dismiss();
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

void SDestinationMenu::Dismiss()
{
    if (GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(SharedThis(this));

    if (APlayerController* PC = Player.Get())
    {
        FInputModeGameOnly Mode;
        PC->SetInputMode(Mode);
        PC->bShowMouseCursor = false;
    }
    GOpenMenu.Reset();
}

bool SDestinationMenu::IsOpen()
{
    return GOpenMenu.IsValid();
}

void SDestinationMenu::Toggle(APlayerController* PC)
{
    // 이미 떠 있으면 걷어낸다(T 토글).
    if (TSharedPtr<SDestinationMenu> Open = GOpenMenu.Pin())
    {
        Open->Dismiss();
        return;
    }

    if (!PC || !GEngine || !GEngine->GameViewport) return;

    TSharedRef<SDestinationMenu> Menu = SNew(SDestinationMenu).Player(PC);
    GOpenMenu = Menu;
    GEngine->GameViewport->AddViewportWidgetContent(Menu, 22);

    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(Menu);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor = true;
}
