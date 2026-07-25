#include "MainMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "SettingsWidget.h"
#include "GameFlowSubsystem.h"
#include "Engine/GameInstance.h"

UMainMenuWidget* UMainMenuWidget::ShowMainMenu(APlayerController* PC, TSubclassOf<UMainMenuWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;

    UMainMenuWidget* W = CreateWidget<UMainMenuWidget>(PC, WidgetClass);
    if (W)
    {
        W->AddToViewport(100);
        FInputModeUIOnly Mode;
        Mode.SetWidgetToFocus(W->TakeWidget());
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode);
        PC->bShowMouseCursor = true;
    }
    return W;
}

void UMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct(); // 부모: PlayIntro 자동(디자이너 등장 애니)

    if (Btn_NewGame)  Btn_NewGame->OnClicked.AddDynamic(this, &UMainMenuWidget::OnNewGameClicked);
    if (Btn_Continue) Btn_Continue->OnClicked.AddDynamic(this, &UMainMenuWidget::OnContinueClicked);
    if (Btn_Settings) Btn_Settings->OnClicked.AddDynamic(this, &UMainMenuWidget::OnSettingsClicked);
    if (Btn_Quit)     Btn_Quit->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);

    // 이어할 진행이 없으면 Continue 비활성화 (진행 세이브가 기준 — 스탯 세이브만 남아 있는 경우와 구분한다)
    bool bHasRun = UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0);
    if (UGameInstance* GI = GetGameInstance())
        if (UGameFlowSubsystem* Flow = GI->GetSubsystem<UGameFlowSubsystem>())
            bHasRun = Flow->HasSavedRun();
    if (Btn_Continue) Btn_Continue->SetIsEnabled(bHasRun);

    // 로고 피벗을 중앙으로(펄스가 중심 기준으로 커지게)
    if (Txt_Title)    Txt_Title->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    if (Txt_Subtitle) Txt_Subtitle->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
}

void UMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    TitleTime += InDeltaTime;

    // ── 로고: 천천히 숨쉬는 펄스 + 골드 시머(밝기 일렁임) ──
    if (Txt_Title)
    {
        const float Breathe = FMath::Sin(TitleTime * 1.5f);          // -1..1, 주기 ~4.2s
        const float Scale = 1.f + 0.025f * Breathe;                   // ±2.5% 스케일
        Txt_Title->SetRenderScale(FVector2D(Scale, Scale));
        // 골드(0.98,0.80,0.32) ↔ 밝은 골드(1.0,0.92,0.55) 사이 시머
        const float Shimmer = 0.5f + 0.5f * FMath::Sin(TitleTime * 2.1f);
        const FLinearColor Lo(0.96f, 0.78f, 0.28f, 1.f);
        const FLinearColor Hi(1.00f, 0.93f, 0.58f, 1.f);
        Txt_Title->SetColorAndOpacity(FSlateColor(FMath::Lerp(Lo, Hi, Shimmer)));
    }

    // ── 부제: 은은한 글로우(투명도 일렁임) ──
    if (Txt_Subtitle)
        Txt_Subtitle->SetRenderOpacity(0.62f + 0.30f * (0.5f + 0.5f * FMath::Sin(TitleTime * 1.1f + 1.0f)));

    // ── 푸터 "마우스로 선택" 힌트: 느린 깜빡임 ──
    if (Txt_Foot)
        Txt_Foot->SetRenderOpacity(0.45f + 0.35f * (0.5f + 0.5f * FMath::Sin(TitleTime * 2.6f)));
}

void UMainMenuWidget::OnNewGameClicked()
{
    if (bResetSaveOnNewGame && UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
        UGameplayStatics::DeleteGameInSlot(SaveSlotName, 0);

    // 진행 담당(GameFlow)에게 넘긴다 — 첫 무대와 오프닝이 거기서 결정된다.
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGameFlowSubsystem* Flow = GI->GetSubsystem<UGameFlowSubsystem>())
        {
            Flow->StartNewGame();
            return;
        }
    }
    StartGame(); // GameFlow가 없을 때의 옛 경로(레벨 이름 직행)
}

void UMainMenuWidget::OnContinueClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGameFlowSubsystem* Flow = GI->GetSubsystem<UGameFlowSubsystem>())
        {
            Flow->ContinueGame();   // 저장된 레벨·도착 지점으로 (스탯/인벤은 도착 레벨에서 로드)
            return;
        }
    }
    StartGame();
}

void UMainMenuWidget::OnSettingsClicked()
{
    // C++ 설정 화면 열기 (클래스 미지정 시 /Game/UI/WBP_Settings 자동 로드)
    if (!SettingsWidgetClass)
    {
        if (UClass* Loaded = LoadClass<USettingsWidget>(nullptr, TEXT("/Game/UI/WBP_Settings.WBP_Settings_C")))
            SettingsWidgetClass = Loaded;
    }
    if (SettingsWidgetClass)
        USettingsWidget::OpenSettings(GetOwningPlayer(), SettingsWidgetClass);

    OpenSettings(); // BP 추가 연출(선택) — 구현 없으면 무동작
}

void UMainMenuWidget::OnQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UMainMenuWidget::StartGame()
{
    if (!GameplayLevelName.IsNone())
        UGameplayStatics::OpenLevel(this, GameplayLevelName);
}
