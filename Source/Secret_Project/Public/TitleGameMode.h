#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TitleGameMode.generated.h"

class UMainMenuWidget;

/**
 * 타이틀(메인 화면) 게임모드 — 이 게임모드를 쓰는 레벨을 열면 자동으로 메인 메뉴를 띄운다.
 * 사용자 에디터 작업 최소화:
 *   - MainMenuClass는 생성자에서 /Game/UI/WBP_MainMenu 를 FClassFinder로 자동 연결(있으면).
 *     → BP에서 따로 할당할 필요 없음(에디터에서 바꾸고 싶을 때만 덮어쓰기).
 *   - 빈 타이틀 레벨 하나 만들고 World Settings의 GameMode Override를 이 클래스로 지정 + 그 레벨을
 *     Project Settings의 Game Default Map으로(또는 A가 DefaultEngine.ini에 설정) → 실행 시 메인 화면.
 * 로직(New/Continue/Settings/Quit)은 UMainMenuWidget(C++). 연출만 BP.
 */
UCLASS()
class SECRET_PROJECT_API ATitleGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ATitleGameMode();

    // 띄울 메인 메뉴 위젯 클래스 (기본 = WBP_MainMenu 자동연결). 비면 BeginPlay에서 한 번 더 로드 시도.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Title")
    TSubclassOf<UMainMenuWidget> MainMenuClass;

protected:
    virtual void BeginPlay() override;

    // 시작 화면 카메라를 아주 느리게 움직인다 — 멈춘 그림이 아니라 밤거리가 살아 있게.
    virtual void Tick(float DeltaSeconds) override;

private:
    UPROPERTY() TObjectPtr<class ACameraActor> TitleCam;
    FVector  CamBaseLoc = FVector::ZeroVector;
    FRotator CamBaseRot = FRotator::ZeroRotator;
    float    CamTime = 0.f;
};
