#include "TitleGameMode.h"
#include "MainMenuWidget.h"
#include "TitleMenu.h"
#include "SecretGameSettings.h"
#include "GameAudioSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/DefaultPawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

ATitleGameMode::ATitleGameMode()
{
    PrimaryActorTick.bCanEverTick = true;   // 시작 화면 카메라 흐름

    // 타이틀에선 전투 캐릭터 대신 기본 폰(메뉴만 보여줄 거라 조작 불필요)
    DefaultPawnClass = ADefaultPawn::StaticClass();

    // WBP_MainMenu가 /Game/UI에 있으면 기본 메뉴 클래스로 자동 연결 → 에디터 할당 불필요.
    // (build_all_ui.py가 만들어 둠. 없으면 경고만 뜨고 BeginPlay에서 재시도.)
    static ConstructorHelpers::FClassFinder<UMainMenuWidget> MM(TEXT("/Game/UI/WBP_MainMenu"));
    if (MM.Succeeded()) MainMenuClass = MM.Class;
}

void ATitleGameMode::BeginPlay()
{
    Super::BeginPlay();

    // 저장된 설정 적용(볼륨/해상도/창모드) + 타이틀 BGM(/Game/Audio/BGM/BGM_title 있으면)
    if (UGameInstance* GI = GetGameInstance())
    {
        USecretGameSettings::Load()->ApplyAll(GI);
        if (UGameAudioSubsystem* Audio = GI->GetSubsystem<UGameAudioSubsystem>())
            Audio->PlayBGM(TEXT("title"));
    }

    // 생성자 시점에 위젯이 아직 없었으면(첫 빌드 등) 런타임에 한 번 더 로드 시도
    if (!MainMenuClass)
    {
        if (UClass* Loaded = LoadClass<UMainMenuWidget>(nullptr, TEXT("/Game/UI/WBP_MainMenu.WBP_MainMenu_C")))
            MainMenuClass = Loaded;
    }

    UWorld* W = GetWorld();
    if (!W) return;

    // 시작 화면 카메라 — 레벨에 세워 둔 CameraActor를 그대로 쓴다(맵이 정본, 코드는 좌표를 안 박는다).
    for (TActorIterator<ACameraActor> It(W); It; ++It)
    {
        TitleCam = *It;
        break;
    }
    if (APlayerController* PC = W->GetFirstPlayerController())
    {
        if (TitleCam)
        {
            CamBaseLoc = TitleCam->GetActorLocation();
            CamBaseRot = TitleCam->GetActorRotation();
            PC->SetViewTarget(TitleCam);
        }
        // ★차림표는 코드로 그린다(STitleMenu). 옛 WBP_MainMenu는 시험판 이름표(REVERIE)와
        //   화면을 가로지르는 분홍 사선을 달고 있어 밤거리를 덮었다. 위젯 에셋 의존 0.
        STitleMenu::Show(PC);
    }
}

/**
 * 카메라를 아주 느리게 민다 — 24초에 한 번 오가는 흐름.
 * 큰 움직임은 메뉴를 읽는 데 방해가 되므로, 거리 30·각도 0.8°만 쓴다(멈춘 그림이 아니게만).
 */
void ATitleGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!TitleCam) return;

    CamTime += DeltaSeconds;
    const float S = FMath::Sin(CamTime * 0.26f);           // 주기 ≈ 24초
    const FVector Fwd = CamBaseRot.Vector();
    TitleCam->SetActorLocation(CamBaseLoc + Fwd * (30.f * S) + FVector(0, 0, 6.f * S));
    TitleCam->SetActorRotation(CamBaseRot + FRotator(0.f, 0.8f * S, 0.f));
}
