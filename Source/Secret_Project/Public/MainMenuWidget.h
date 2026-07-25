#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * 메인 화면(타이틀) UI. 게임 시작 시 표시.
 * - 로직은 C++(New/Continue/Settings/Quit), 연출(등장 애니·배경·사운드)은 BP(디자이너).
 * - UPersonaWidgetBase 상속 → 등장 시 PlayIntro 자동 호출(디자이너가 애니 구현).
 *
 * 사용법:
 *   1) WBP_MainMenu 위젯 BP 만들고 Parent = UMainMenuWidget.
 *   2) 버튼 배치: Btn_NewGame / Btn_Continue / Btn_Settings / Btn_Quit (Is Variable+이름정확).
 *   3) 타이틀 레벨의 Level BP BeginPlay → ShowMainMenu(PlayerController, WBP_MainMenu).
 *   4) GameplayLevelName을 본편 레벨 이름으로 설정.
 */
UCLASS()
class SECRET_PROJECT_API UMainMenuWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    // 타이틀 레벨 BP에서 호출 → 메인메뉴 생성+표시(+UI 입력모드/커서)
    UFUNCTION(BlueprintCallable, Category = "Main Menu")
    static UMainMenuWidget* ShowMainMenu(APlayerController* PC, TSubclassOf<UMainMenuWidget> WidgetClass);

    // 설정 화면 열기 — 디자이너가 BP에서 구현(설정 패널/위젯 표시)
    UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
    void OpenSettings();

protected:
    virtual void NativeConstruct() override;
    // 타이틀 로고 "살아있는" 연출 — 숨쉬는 펄스/골드 시머/부제 글로우(C++ 구동, BP 불필요).
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // 게임플레이(본편) 레벨 이름 — New/Continue 시 이동
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu")
    FName GameplayLevelName = TEXT("MainLevel");

    // 세이브 슬롯 이름 (B 세이브 시스템과 동일: "PlayerSave")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu")
    FString SaveSlotName = TEXT("PlayerSave");

    // New Game 시 기존 세이브 삭제 (기본 off=안전. 켜면 진짜 새 게임)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu")
    bool bResetSaveOnNewGame = false;

    // 설정 화면 위젯 클래스 (비면 /Game/UI/WBP_Settings 자동 로드). 설정 버튼이 이걸 띄움.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu")
    TSubclassOf<class USettingsWidget> SettingsWidgetClass;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_NewGame;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Continue;  // 세이브 없으면 자동 비활성
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Settings;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Quit;

    // 로고 연출 대상(있으면 애니, 없으면 무동작 — 하위호환)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Subtitle;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Foot;

private:
    UFUNCTION() void OnNewGameClicked();
    UFUNCTION() void OnContinueClicked();
    UFUNCTION() void OnSettingsClicked();
    UFUNCTION() void OnQuitClicked();

    void StartGame();

    float TitleTime = 0.f;   // 로고 연출 누적 시간
};
