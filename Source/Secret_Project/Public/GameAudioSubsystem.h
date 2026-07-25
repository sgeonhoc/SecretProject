#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameAudioSubsystem.generated.h"

class USoundBase;
class UAudioComponent;

/**
 * ★ 이름으로 효과음/배경음을 재생하는 오디오 매니저 (GameInstance 서브시스템).
 * - PlaySFX(FName): /Game/Audio/SFX/SFX_<name> 자동 로드 후 재생. 없으면 조용히 무시.
 * - PlayBGM(FName): /Game/Audio/BGM/BGM_<name> 루프 재생(이전 BGM 정지). 같은 곡이면 유지.
 * - 볼륨: 마스터/BGM/SFX 분리. 설정(USecretGameSettings)이 값을 넣어줌. 세이브는 설정이 담당.
 *
 * 어디서든: GetGameInstance()->GetSubsystem<UGameAudioSubsystem>()->PlaySFX("weak").
 * 전투 연출(Flair)·UI·스토리가 이걸 호출 → 사용자는 SFX_<name> 파일만 폴더에 넣으면 소리가 남.
 */
UCLASS()
class SECRET_PROJECT_API UGameAudioSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // 이름으로 효과음 1회 재생 (2D). VolumeScale은 추가 배율(기본 1).
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void PlaySFX(FName Name, float VolumeScale = 1.f);

    // 이름으로 배경음 루프 재생 (같은 곡이면 무시, 다르면 교체). None이면 정지.
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void PlayBGM(FName Name);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void StopBGM();

    // 볼륨 설정 (0~1). 설정 위젯/세이브가 호출.
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetVolumes(float Master, float Bgm, float Sfx);

    UFUNCTION(BlueprintPure, Category = "Audio") float GetMasterVolume() const { return MasterVolume; }
    UFUNCTION(BlueprintPure, Category = "Audio") float GetBgmVolume() const { return BgmVolume; }
    UFUNCTION(BlueprintPure, Category = "Audio") float GetSfxVolume() const { return SfxVolume; }

private:
    UPROPERTY() float MasterVolume = 1.f;
    UPROPERTY() float BgmVolume = 1.f;
    UPROPERTY() float SfxVolume = 1.f;

    UPROPERTY(Transient) TObjectPtr<UAudioComponent> BgmComp = nullptr;
    FName CurrentBGM = NAME_None;
};
