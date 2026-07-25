#include "GameAudioSubsystem.h"
#include "AssetResolver.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

void UGameAudioSubsystem::PlaySFX(FName Name, float VolumeScale)
{
    USoundBase* S = UAssetResolver::ResolveSFX(Name);
    if (!S) return;   // 파일 없으면 조용히 무시
    const float Vol = FMath::Clamp(MasterVolume * SfxVolume * VolumeScale, 0.f, 2.f);
    if (Vol <= 0.f) return;
    UGameplayStatics::PlaySound2D(this, S, Vol);
}

void UGameAudioSubsystem::PlayBGM(FName Name)
{
    if (Name == CurrentBGM && BgmComp && BgmComp->IsPlaying()) return;   // 같은 곡 재생 중 → 유지

    if (Name.IsNone()) { StopBGM(); return; }

    USoundBase* S = UAssetResolver::ResolveBGM(Name);
    if (!S) return;

    StopBGM();
    const float Vol = FMath::Clamp(MasterVolume * BgmVolume, 0.f, 2.f);
    BgmComp = UGameplayStatics::SpawnSound2D(this, S, Vol, 1.f, 0.f, nullptr, true /*bPersistAcrossLevel*/, false);
    if (BgmComp)
    {
        BgmComp->bAutoDestroy = false;
        CurrentBGM = Name;
    }
}

void UGameAudioSubsystem::StopBGM()
{
    if (BgmComp)
    {
        BgmComp->Stop();
        BgmComp = nullptr;
    }
    CurrentBGM = NAME_None;
}

void UGameAudioSubsystem::SetVolumes(float Master, float Bgm, float Sfx)
{
    MasterVolume = FMath::Clamp(Master, 0.f, 1.f);
    BgmVolume    = FMath::Clamp(Bgm, 0.f, 1.f);
    SfxVolume    = FMath::Clamp(Sfx, 0.f, 1.f);

    // 재생 중인 BGM 볼륨 즉시 반영
    if (BgmComp)
        BgmComp->SetVolumeMultiplier(FMath::Clamp(MasterVolume * BgmVolume, 0.f, 2.f));
}
