#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SecretGameSettings.generated.h"

class UGameInstance;

/**
 * ★ 게임 설정 저장 (자체 슬롯 "Settings"). 메인메뉴 설정 화면이 조절, 게임 시작 시 자동 적용.
 * - 오디오: 마스터/BGM/SFX 볼륨 → UGameAudioSubsystem 반영.
 * - 화면: 해상도/창모드/수직동기 → UGameUserSettings 반영.
 * - 텍스트 속도: 대화·스토리 타자기 글자 간격(초).
 * - 언어: 0=한국어, 1=English (콘텐츠 분기용 — 현재는 값 보관).
 * 전부 C++ 적용. 사용자는 설정 위젯 버튼만 배치하면 동작.
 */
UCLASS()
class SECRET_PROJECT_API USecretGameSettings : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY() float MasterVolume = 1.0f;
    UPROPERTY() float BgmVolume = 0.8f;
    UPROPERTY() float SfxVolume = 1.0f;

    UPROPERTY() int32 ResX = 1920;
    UPROPERTY() int32 ResY = 1080;
    // ★기본값 = 창모드(2). 전체화면(0)이면 알탭·스크린샷 때마다 검은 화면 깜빡임이 나서 개발이 느려진다.
    //   -windowed로 켜도 GameMode::BeginPlay의 ApplyAll이 이 값으로 되돌리므로 여기가 실질 기본값이다.
    UPROPERTY() int32 WindowMode = 2;   // 0=Fullscreen, 1=WindowedFullscreen, 2=Windowed
    UPROPERTY() bool bVSync = true;

    UPROPERTY() float TextInterval = 0.035f;   // 타자기 글자당 초 (작을수록 빠름)
    UPROPERTY() int32 Language = 0;            // 0=KR, 1=EN

    static const TCHAR* SlotName() { return TEXT("Settings"); }

    // 로드(없으면 기본값 새로 생성). 항상 유효한 객체 반환.
    static USecretGameSettings* Load();
    void Save();

    // 오디오 서브시스템에 볼륨 반영
    void ApplyAudio(UGameInstance* GI) const;
    // UGameUserSettings에 해상도/창모드/수직동기 반영 + Apply
    void ApplySystem() const;
    // 둘 다 적용 (게임 시작 시 호출)
    void ApplyAll(UGameInstance* GI) const;

    // 현재 적용된 텍스트 타자기 간격(스토리/대화 위젯이 읽음). 설정 없으면 기본 0.035.
    static float GetTextInterval();
};
