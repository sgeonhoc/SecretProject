#include "TitleMenu.h"
#include "GameFlowSubsystem.h"
#include "SettingsWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
    // 차림표 칸 — 순서가 곧 Index다.
    enum : int32 { ITEM_NEW = 0, ITEM_CONTINUE, ITEM_SETTINGS, ITEM_QUIT };
}

void STitleMenu::Construct(const FArguments& InArgs)
{
    Player = InArgs._Player;

    // 이어할 진행이 있나 — 없으면 "이어하기"는 회색으로 두고 안 받는다.
    if (APlayerController* PC = Player.Get())
    {
        if (UGameInstance* GI = PC->GetGameInstance())
        {
            if (UGameFlowSubsystem* Flow = GI->GetSubsystem<UGameFlowSubsystem>())
                bContinueEnabled = Flow->HasSavedRun();
        }
    }

    // 한글 — 엔진 Roboto(CJK 폴백). 프로젝트 폰트는 Slate에서 네모로 뜬다(2026-07-24 확인).
    FSlateFontInfo MarkFont = FCoreStyle::GetDefaultFontStyle("Regular", 76);
    FSlateFontInfo SubFont  = FCoreStyle::GetDefaultFontStyle("Regular", 18);
    FSlateFontInfo FootFont = FCoreStyle::GetDefaultFontStyle("Regular", 13);
    if (UObject* Roboto = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto")))
    {
        MarkFont = FSlateFontInfo(Roboto, 76);
        SubFont  = FSlateFontInfo(Roboto, 18);
        FootFont = FSlateFontInfo(Roboto, 13);
    }

    ChildSlot
    [
        SNew(SOverlay)

        // 왼쪽에 드리운 어두운 천 — 밤거리 위에서도 글자가 읽히게(오른쪽 거리는 그대로 보인다).
        + SOverlay::Slot().HAlign(HAlign_Left)
        [
            SNew(SBox).WidthOverride(620.f)
            [
                SNew(SImage)
                .Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .ColorAndOpacity(FLinearColor(0.015f, 0.02f, 0.03f, 0.72f))
            ]
        ]

        // 이름표 + 차림표
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(FMargin(86, 0, 0, 0))
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock).Font(MarkFont)
                .ColorAndOpacity(FSlateColor(FLinearColor(0.97f, 0.86f, 0.55f, 1.f)))
                .Text(FText::FromString(TEXT("라 셀")))
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(4, 6, 0, 0))
            [
                SNew(STextBlock).Font(SubFont)
                .ColorAndOpacity(FSlateColor(FLinearColor(0.66f, 0.68f, 0.72f, 1.f)))
                .Text(FText::FromString(TEXT("검은물이 지나간 뒤의 도시")))
            ]

            // 가는 금 — 이름표와 차림표를 가른다
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(6, 26, 0, 22))
            [
                SNew(SBox).WidthOverride(300.f).HeightOverride(1.f)
                [
                    SNew(SImage).Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .ColorAndOpacity(FLinearColor(0.75f, 0.66f, 0.42f, 0.55f))
                ]
            ]

            + SVerticalBox::Slot().AutoHeight()[ MakeItem(FText::FromString(TEXT("새 게임")),  ITEM_NEW,      true) ]
            + SVerticalBox::Slot().AutoHeight()[ MakeItem(FText::FromString(TEXT("이어하기")), ITEM_CONTINUE, bContinueEnabled) ]
            + SVerticalBox::Slot().AutoHeight()[ MakeItem(FText::FromString(TEXT("설정")),     ITEM_SETTINGS, true) ]
            + SVerticalBox::Slot().AutoHeight()[ MakeItem(FText::FromString(TEXT("나가기")),   ITEM_QUIT,     true) ]
        ]

        // 아래 구석 — 조작 안내
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(FMargin(90, 0, 0, 44))
        [
            SNew(STextBlock).Font(FootFont)
            .ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.56f, 0.58f, 0.85f)))
            .Text(FText::FromString(TEXT("마우스로 고른다")))
        ]
    ];
}

TSharedRef<SWidget> STitleMenu::MakeItem(const FText& Label, int32 Index, bool bEnabled)
{
    FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 26);
    if (UObject* Roboto = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto")))
        Font = FSlateFontInfo(Roboto, 26);

    // 색은 매 프레임 읽어 간다 — 얹은 칸은 밝은 금, 못 누르는 칸은 어둡게.
    auto Tint = [this, Index, bEnabled]()
    {
        if (!bEnabled) return FSlateColor(FLinearColor(0.42f, 0.42f, 0.44f, 1.f));
        return (HoverIndex == Index)
            ? FSlateColor(FLinearColor(1.0f, 0.92f, 0.66f, 1.f))
            : FSlateColor(FLinearColor(0.86f, 0.86f, 0.84f, 1.f));
    };
    auto TickTint = [this, Index]()
    {
        return FSlateColor(FLinearColor(0.98f, 0.80f, 0.36f, HoverIndex == Index ? 1.f : 0.f));
    };

    return SNew(SBox).Padding(FMargin(0, 7, 0, 7))
    [
        SNew(SButton)
        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
        .ContentPadding(FMargin(0))
        .IsEnabled(bEnabled)
        .OnHovered_Lambda([this, Index]() { HoverIndex = Index; })
        .OnUnhovered_Lambda([this, Index]() { if (HoverIndex == Index) HoverIndex = -1; })
        .OnClicked_Lambda([this, Index]() { OnItemClicked(Index); return FReply::Handled(); })
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(0, 0, 12, 0))
            [
                SNew(STextBlock).Font(Font).ColorAndOpacity_Lambda(TickTint)
                .Text(FText::FromString(TEXT("—")))
            ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SNew(STextBlock).Font(Font).ColorAndOpacity_Lambda(Tint).Text(Label)
            ]
        ]
    ];
}

/**
 * 화면에서 걷어낸다.
 * ★뷰포트에 얹은 위젯은 레벨을 옮겨도 그대로 남는다 — 안 걷으면 셋방 장면 위에
 *   시작 차림표가 유령처럼 겹쳐 뜬다(2026-07-24 실기에서 확인).
 */
void STitleMenu::Dismiss()
{
    if (GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(SharedThis(this));

    if (APlayerController* PC = Player.Get())
    {
        FInputModeGameOnly Mode;
        PC->SetInputMode(Mode);
        PC->bShowMouseCursor = false;
    }
}

void STitleMenu::OnItemClicked(int32 Index)
{
    if (bTaken && Index != ITEM_SETTINGS) return;

    APlayerController* PC = Player.Get();
    UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
    UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr;

    switch (Index)
    {
    case ITEM_NEW:
        if (Flow) { bTaken = true; Dismiss(); Flow->StartNewGame(); }
        break;

    case ITEM_CONTINUE:
        if (Flow && bContinueEnabled) { bTaken = true; Dismiss(); Flow->ContinueGame(); }
        break;

    case ITEM_SETTINGS:
        if (PC)
        {
            if (UClass* Loaded = LoadClass<USettingsWidget>(nullptr, TEXT("/Game/UI/WBP_Settings.WBP_Settings_C")))
                USettingsWidget::OpenSettings(PC, Loaded);
        }
        break;

    case ITEM_QUIT:
        UKismetSystemLibrary::QuitGame(PC, PC, EQuitPreference::Quit, false);
        break;

    default:
        break;
    }
}

void STitleMenu::Show(APlayerController* PC)
{
    if (!PC || !GEngine || !GEngine->GameViewport) return;

    TSharedRef<STitleMenu> Menu = SNew(STitleMenu).Player(PC);
    GEngine->GameViewport->AddViewportWidgetContent(Menu, 20);

    // 마우스로 고르는 화면 — 커서를 띄우고 UI가 입력을 받게 한다.
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(Menu);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor = true;
}
