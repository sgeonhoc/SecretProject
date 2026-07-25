#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "SettingsWidget.generated.h"

class UButton;
class UTextBlock;
class USlider;
class UCheckBox;
class APlayerController;
class USecretGameSettings;

/**
 * ★ 설정 화면 (메인메뉴 '설정' 버튼에서 열림). 로직/바인딩 전부 C++.
 * WBP는 레이아웃만(BindWidgetOptional — 만든 것만 동작):
 *   볼륨: Slider_Master / Slider_Bgm / Slider_Sfx (+ Txt_Master/Bgm/Sfx %표시)
 *   화면: Btn_WindowMode(+Txt_WindowMode 전체화면/창전체/창모드 순환) · Chk_VSync
 *         Btn_ResPrev / Btn_ResNext (+ Txt_Res 해상도 순환)
 *   기타: Btn_TextPrev / Btn_TextNext (+ Txt_TextSpeed 느림/보통/빠름) · Btn_Language(+Txt_Language KR/EN)
 *   하단: Btn_Apply / Btn_Reset / Btn_Close
 * 볼륨은 즉시 반영(미리듣기), 해상도/창모드는 Apply에서 반영. 닫기/적용 시 저장.
 */
UCLASS()
class SECRET_PROJECT_API USettingsWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static USettingsWidget* OpenSettings(APlayerController* PC, TSubclassOf<USettingsWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> Slider_Master;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> Slider_Bgm;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> Slider_Sfx;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Master;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Bgm;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Sfx;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_WindowMode;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_WindowMode;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Chk_VSync;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_ResPrev;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_ResNext;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Res;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_TextPrev;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_TextNext;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_TextSpeed;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Language;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Language;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Apply;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Reset;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

private:
    UPROPERTY() TObjectPtr<USecretGameSettings> Settings;
    int32 ResIndex = 2;   // 지원 해상도 목록 인덱스

    void RefreshLabels();
    void ApplyAudioLive();

    UFUNCTION() void OnMasterChanged(float V);
    UFUNCTION() void OnBgmChanged(float V);
    UFUNCTION() void OnSfxChanged(float V);
    UFUNCTION() void OnVSyncChanged(bool bChecked);
    UFUNCTION() void OnWindowMode();
    UFUNCTION() void OnResPrev();
    UFUNCTION() void OnResNext();
    UFUNCTION() void OnTextPrev();
    UFUNCTION() void OnTextNext();
    UFUNCTION() void OnLanguage();
    UFUNCTION() void OnApply();
    UFUNCTION() void OnReset();
    UFUNCTION() void OnClose();
};
