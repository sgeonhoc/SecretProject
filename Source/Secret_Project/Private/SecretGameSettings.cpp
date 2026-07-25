#include "SecretGameSettings.h"
#include "GameAudioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/GameInstance.h"

USecretGameSettings* USecretGameSettings::Load()
{
    if (UGameplayStatics::DoesSaveGameExist(SlotName(), 0))
        if (USecretGameSettings* S = Cast<USecretGameSettings>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0)))
            return S;
    // 없으면 기본값 새로 생성
    return Cast<USecretGameSettings>(UGameplayStatics::CreateSaveGameObject(USecretGameSettings::StaticClass()));
}

void USecretGameSettings::Save()
{
    UGameplayStatics::SaveGameToSlot(this, SlotName(), 0);
}

void USecretGameSettings::ApplyAudio(UGameInstance* GI) const
{
    if (!GI) return;
    if (UGameAudioSubsystem* Audio = GI->GetSubsystem<UGameAudioSubsystem>())
        Audio->SetVolumes(MasterVolume, BgmVolume, SfxVolume);
}

void USecretGameSettings::ApplySystem() const
{
    UGameUserSettings* GUS = UGameUserSettings::GetGameUserSettings();
    if (!GUS) return;

    GUS->SetScreenResolution(FIntPoint(ResX, ResY));

    EWindowMode::Type Mode = EWindowMode::Fullscreen;
    switch (WindowMode)
    {
        case 1: Mode = EWindowMode::WindowedFullscreen; break;
        case 2: Mode = EWindowMode::Windowed; break;
        default: Mode = EWindowMode::Fullscreen; break;
    }
    GUS->SetFullscreenMode(Mode);
    GUS->SetVSyncEnabled(bVSync);
    GUS->ApplySettings(false);
}

void USecretGameSettings::ApplyAll(UGameInstance* GI) const
{
    ApplyAudio(GI);
    ApplySystem();
}

float USecretGameSettings::GetTextInterval()
{
    // 가볍게: 슬롯 존재 시 로드해 값만 반환(설정 위젯이 자주 호출 안 함 — 위젯 생성 시 1회)
    if (UGameplayStatics::DoesSaveGameExist(SlotName(), 0))
        if (USecretGameSettings* S = Cast<USecretGameSettings>(UGameplayStatics::LoadGameFromSlot(SlotName(), 0)))
            return S->TextInterval;
    return 0.035f;
}
