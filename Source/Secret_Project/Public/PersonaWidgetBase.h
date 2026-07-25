#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PersonaWidgetBase.generated.h"

class UWidgetAnimation;

/**
 * 페르소나식 연출 UI 베이스.
 * - 로직/타이밍은 C++, 실제 애니메이션(WidgetAnimation)·사운드·아트는 BP(디자이너)가 올림.
 * - 모든 메뉴/HUD 위젯을 이 클래스로 reparent하면 공통 등장/퇴장 연출 + 이름으로 애니 재생 가능.
 *
 * 사용법(디자이너):
 *   1) 위젯 BP를 이 클래스로 Reparent.
 *   2) 디자이너 탭에서 WidgetAnimation 만들기 (예: "Intro", "Outro", 강조 애니 등).
 *   3) 이벤트 그래프에서 PlayIntro/PlayOutro 구현 → 해당 애니 Play. (또는 PlayNamedAnimation로 C++가 트리거)
 */
UCLASS()
class SECRET_PROJECT_API UPersonaWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    // 등장 연출 — 디자이너가 BP에서 구현(WidgetAnimation 재생). C++가 열 때/NativeConstruct에서 호출.
    UFUNCTION(BlueprintImplementableEvent, Category = "Persona UI")
    void PlayIntro();

    // 퇴장 연출 — 닫기 직전 호출. 보통 애니 끝나고 RemoveFromParent.
    UFUNCTION(BlueprintImplementableEvent, Category = "Persona UI")
    void PlayOutro();

    // UI 효과음 훅 — 디자이너가 SoundId별로 사운드 재생(메뉴 이동/결정/취소 등).
    UFUNCTION(BlueprintImplementableEvent, Category = "Persona UI")
    void PlayUISound(FName SoundId);

    // ★ 메뉴 효과음을 C++가 직접 재생(/Game/Audio/SFX/SFX_<Name>). GameAudioSubsystem 경유.
    //   ui_click/ui_open/ui_close/ui_swipe/ui_error/ui_confirm 등. 디자이너 BP 없이 동작.
    UFUNCTION(BlueprintCallable, Category = "Persona UI")
    void PlayUI(FName Name, float Volume = 0.6f) const;

    // 이름으로 WidgetAnimation 재생 (BP에서 만든 애니를 C++ 로직이 트리거). 찾으면 true.
    UFUNCTION(BlueprintCallable, Category = "Persona UI")
    bool PlayNamedAnimation(FName AnimName, float StartAtTime = 0.f, int32 NumLoops = 1, float PlaybackSpeed = 1.f);

    // 닫기 연출 후 실제 제거 (Outro 재생 → DelaySeconds 후 RemoveFromParent)
    UFUNCTION(BlueprintCallable, Category = "Persona UI")
    void CloseWithOutro(float DelaySeconds = 0.35f);

    // ESC로 닫을 수 있는, 현재 화면에 열린 팝업을 하나 닫는다(닫았으면 true).
    // APlayerCharacter::OpenSystemMenu가 호출 → 열린 팝업 있으면 닫기(토글/뒤로), 없으면 메뉴 오픈.
    UFUNCTION(BlueprintCallable, Category = "Persona UI")
    static bool CloseTopEscWidget(UObject* WorldContext);

    // 입력 모드 재조정: 화면에 커서가 필요한 위젯(메뉴·대화·상점 등)이 하나라도 남아있으면
    // GameAndUI+커서, 없으면 GameOnly+커서off. 위젯 닫을 때 호출 → "상점 닫으니 대화 못 누름" 류 해결.
    UFUNCTION(BlueprintCallable, Category = "Persona UI")
    static void RefreshInputMode(UObject* WorldContext);

    // ★ 리스트 연출: 컨테이너의 자식 항목들을 순차(stagger) 페이드+슬라이드로 등장시킨다.
    //   동적 생성한 카드/게이지에 페르소나식 등장감을 부여. Refresh 끝에 호출.
    UFUNCTION(BlueprintCallable, Category = "Persona UI")
    void StaggerIntro(class UPanelWidget* Container, float PerItemDelay = 0.05f);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ESC/뒤로 키를 위젯이 자체 처리하면 true 반환 → CloseTopEscWidget가 위젯을 제거하지 않음.
    // 기본 false(=기존처럼 닫힘). 풀스크린 메뉴가 "장면→허브"로 되돌릴 때 override.
    virtual bool HandleEscBack() { return false; }

    // 켜면 NativeConstruct에서 자동으로 PlayIntro 호출(기본 on)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persona UI")
    bool bAutoPlayIntro = true;

    // ESC(시스템메뉴 키)로 닫히는 팝업인가. 기본 true(메뉴/인벤/상점 등 모달).
    // 상시 HUD(WorldHUD)·전투HUD·대화창은 false로 둬 ESC가 닫지 않게 함.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persona UI")
    bool bCloseOnEsc = true;

    // 이 위젯이 떠 있는 동안 마우스 커서/UI 입력이 필요한가. 기본 true(메뉴/대화/상점).
    // 상시 HUD(WorldHUD)는 false(표시 전용 → 게임 입력 막지 않음).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persona UI")
    bool bWantsCursor = true;

    // 켜면 NativeConstruct에서 모든 자식 버튼에 클릭음(SFX_ui_click)을 일괄 부여 + 팝업이면 열기음.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persona UI")
    bool bAutoUISound = true;

    // 모든 자식 UButton의 PressedSound를 SFX_ui_click으로 설정(클릭음 일괄). NativeConstruct에서 호출.
    void ApplyButtonClickSound();

    // C++ 등장 연출(페이드인 + 아래→제자리 슬라이드). WidgetAnimation 에셋 없이도 모든 위젯에 자동.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persona UI")
    bool bPlayEntrance = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persona UI")
    float EntranceDuration = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persona UI")
    float EntranceSlideY = 36.f;

    // 이름으로 UWidgetAnimation* 찾기(리플렉션 — 패키지 빌드에서도 동작)
    UWidgetAnimation* FindAnimByName(FName AnimName) const;

private:
    FTimerHandle OutroTimer;
    void RemoveSelf();

    // 등장 연출 상태
    bool  bEntranceInit = false;
    bool  bEntranceDone = false;
    float EntranceElapsed = 0.f;

    // 리스트 stagger 연출 상태
    TArray<TWeakObjectPtr<UWidget>> IntroItems;
    bool  bIntroActive = false;
    float IntroElapsed = 0.f;
    float IntroPerDelay = 0.05f;
    float IntroItemDur = 0.28f;
    float IntroSlideY = 22.f;
};
