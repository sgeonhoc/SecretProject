#include "SettingsWidget.h"
#include "SecretGameSettings.h"
#include "GameAudioSubsystem.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/GameInstance.h"

// 지원 해상도 목록
static const FIntPoint GResolutions[] = {
    {1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3840, 2160}
};
static const int32 GResCount = 5;

// 텍스트 속도 프리셋(초/글자) + 라벨
static const float GTextIntervals[] = { 0.06f, 0.035f, 0.015f };
static const TCHAR* GTextLabels[] = { TEXT("느림"), TEXT("보통"), TEXT("빠름") };

USettingsWidget* USettingsWidget::OpenSettings(APlayerController* PC, TSubclassOf<USettingsWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    USettingsWidget* W = CreateWidget<USettingsWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(120);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void USettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    Settings = USecretGameSettings::Load();

    // 슬라이더 초기값 + 바인딩
    if (Slider_Master) { Slider_Master->SetValue(Settings->MasterVolume); Slider_Master->OnValueChanged.AddDynamic(this, &USettingsWidget::OnMasterChanged); }
    if (Slider_Bgm)    { Slider_Bgm->SetValue(Settings->BgmVolume);       Slider_Bgm->OnValueChanged.AddDynamic(this, &USettingsWidget::OnBgmChanged); }
    if (Slider_Sfx)    { Slider_Sfx->SetValue(Settings->SfxVolume);       Slider_Sfx->OnValueChanged.AddDynamic(this, &USettingsWidget::OnSfxChanged); }

    if (Chk_VSync) { Chk_VSync->SetIsChecked(Settings->bVSync); Chk_VSync->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnVSyncChanged); }

    if (Btn_WindowMode) Btn_WindowMode->OnClicked.AddDynamic(this, &USettingsWidget::OnWindowMode);
    if (Btn_ResPrev)    Btn_ResPrev->OnClicked.AddDynamic(this, &USettingsWidget::OnResPrev);
    if (Btn_ResNext)    Btn_ResNext->OnClicked.AddDynamic(this, &USettingsWidget::OnResNext);
    if (Btn_TextPrev)   Btn_TextPrev->OnClicked.AddDynamic(this, &USettingsWidget::OnTextPrev);
    if (Btn_TextNext)   Btn_TextNext->OnClicked.AddDynamic(this, &USettingsWidget::OnTextNext);
    if (Btn_Language)   Btn_Language->OnClicked.AddDynamic(this, &USettingsWidget::OnLanguage);
    if (Btn_Apply)      Btn_Apply->OnClicked.AddDynamic(this, &USettingsWidget::OnApply);
    if (Btn_Reset)      Btn_Reset->OnClicked.AddDynamic(this, &USettingsWidget::OnReset);
    if (Btn_Close)      Btn_Close->OnClicked.AddDynamic(this, &USettingsWidget::OnClose);

    // 현재 해상도에 맞는 인덱스 찾기
    ResIndex = 2;
    for (int32 i = 0; i < GResCount; ++i)
        if (GResolutions[i].X == Settings->ResX && GResolutions[i].Y == Settings->ResY) { ResIndex = i; break; }

    RefreshLabels();
}

void USettingsWidget::RefreshLabels()
{
    if (Txt_Master) Txt_Master->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Settings->MasterVolume * 100))));
    if (Txt_Bgm)    Txt_Bgm->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Settings->BgmVolume * 100))));
    if (Txt_Sfx)    Txt_Sfx->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Settings->SfxVolume * 100))));

    if (Txt_Res) Txt_Res->SetText(FText::FromString(FString::Printf(TEXT("%d x %d"), Settings->ResX, Settings->ResY)));

    if (Txt_WindowMode)
    {
        const TCHAR* Modes[] = { TEXT("전체화면"), TEXT("창 전체화면"), TEXT("창모드") };
        Txt_WindowMode->SetText(FText::FromString(Modes[FMath::Clamp(Settings->WindowMode, 0, 2)]));
    }

    if (Txt_TextSpeed)
    {
        int32 Ti = 1;
        for (int32 i = 0; i < 3; ++i) if (FMath::IsNearlyEqual(GTextIntervals[i], Settings->TextInterval, 0.005f)) { Ti = i; break; }
        Txt_TextSpeed->SetText(FText::FromString(GTextLabels[Ti]));
    }

    if (Txt_Language) Txt_Language->SetText(FText::FromString(Settings->Language == 1 ? TEXT("English") : TEXT("한국어")));
}

void USettingsWidget::ApplyAudioLive()
{
    if (UGameInstance* GI = GetGameInstance())
        if (UGameAudioSubsystem* Audio = GI->GetSubsystem<UGameAudioSubsystem>())
            Audio->SetVolumes(Settings->MasterVolume, Settings->BgmVolume, Settings->SfxVolume);
}

void USettingsWidget::OnMasterChanged(float V) { Settings->MasterVolume = V; ApplyAudioLive(); RefreshLabels(); }
void USettingsWidget::OnBgmChanged(float V)    { Settings->BgmVolume = V;    ApplyAudioLive(); RefreshLabels(); }
void USettingsWidget::OnSfxChanged(float V)    { Settings->SfxVolume = V;    ApplyAudioLive(); RefreshLabels(); }

void USettingsWidget::OnVSyncChanged(bool bChecked) { Settings->bVSync = bChecked; }

void USettingsWidget::OnWindowMode()
{
    Settings->WindowMode = (Settings->WindowMode + 1) % 3;
    RefreshLabels();
}

void USettingsWidget::OnResPrev()
{
    ResIndex = (ResIndex + GResCount - 1) % GResCount;
    Settings->ResX = GResolutions[ResIndex].X; Settings->ResY = GResolutions[ResIndex].Y;
    RefreshLabels();
}

void USettingsWidget::OnResNext()
{
    ResIndex = (ResIndex + 1) % GResCount;
    Settings->ResX = GResolutions[ResIndex].X; Settings->ResY = GResolutions[ResIndex].Y;
    RefreshLabels();
}

void USettingsWidget::OnTextPrev()
{
    int32 Ti = 1;
    for (int32 i = 0; i < 3; ++i) if (FMath::IsNearlyEqual(GTextIntervals[i], Settings->TextInterval, 0.005f)) { Ti = i; break; }
    Ti = (Ti + 2) % 3;
    Settings->TextInterval = GTextIntervals[Ti];
    RefreshLabels();
}

void USettingsWidget::OnTextNext()
{
    int32 Ti = 1;
    for (int32 i = 0; i < 3; ++i) if (FMath::IsNearlyEqual(GTextIntervals[i], Settings->TextInterval, 0.005f)) { Ti = i; break; }
    Ti = (Ti + 1) % 3;
    Settings->TextInterval = GTextIntervals[Ti];
    RefreshLabels();
}

void USettingsWidget::OnLanguage()
{
    Settings->Language = (Settings->Language == 0) ? 1 : 0;
    RefreshLabels();
}

void USettingsWidget::OnApply()
{
    Settings->ApplySystem();          // 해상도/창모드/V싱크 반영
    ApplyAudioLive();
    Settings->Save();
}

void USettingsWidget::OnReset()
{
    Settings->MasterVolume = 1.0f; Settings->BgmVolume = 0.8f; Settings->SfxVolume = 1.0f;
    Settings->ResX = 1920; Settings->ResY = 1080; Settings->WindowMode = 0; Settings->bVSync = true;
    Settings->TextInterval = 0.035f; Settings->Language = 0;
    ResIndex = 2;
    if (Slider_Master) Slider_Master->SetValue(Settings->MasterVolume);
    if (Slider_Bgm)    Slider_Bgm->SetValue(Settings->BgmVolume);
    if (Slider_Sfx)    Slider_Sfx->SetValue(Settings->SfxVolume);
    if (Chk_VSync)     Chk_VSync->SetIsChecked(Settings->bVSync);
    ApplyAudioLive();
    RefreshLabels();
}

void USettingsWidget::OnClose()
{
    Settings->Save();   // 닫을 때 현재 값 저장(미적용 해상도는 다음 Apply/시작 때 반영)
    // 설정은 주로 메인메뉴(타이틀, UIOnly) 위에 열림 → 닫아도 뒤의 메뉴가 계속 클릭되게
    // GameAndUI + 커서 유지(메인메뉴 버튼은 UI 모드에서 클릭됨). 인게임에선 시스템메뉴가 별도 처리.
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, nullptr, EMouseLockMode::DoNotLock, false);
        PC->bShowMouseCursor = true;
    }
    RemoveFromParent();
}
