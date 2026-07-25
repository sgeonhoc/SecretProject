#include "SecretProjectGameMode.h"
#include "APlayerCharacter.h"
#include "SecretProjectPlayerController.h" // PlayerController 클래스를 사용
#include "SecretProjectGameState.h"
#include "SecretGameSettings.h"
#include "GameAudioSubsystem.h"
#include "GameFlowSubsystem.h"
#include "GameOpeningDirector.h"
#include "StageObjectiveActor.h"
#include "StageHudActor.h"
#include "StorySceneDirector.h"
#include "PortalActor.h"
#include "ANPCCharacter.h"
#include "StoryDirectorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"

ASecretProjectGameMode::ASecretProjectGameMode()
{
    PrimaryActorTick.bCanEverTick = true;   // 떨어진 캐릭터 건져 올리기(아래 Tick)

    DefaultPawnClass = APlayerCharacter::StaticClass();
    PlayerControllerClass = ASecretProjectPlayerController::StaticClass();
    GameStateClass = ASecretProjectGameState::StaticClass();

    OpeningDirectorClass = AGameOpeningDirector::StaticClass();
}

/**
 * 어느 문으로 들어왔느냐에 따라 설 자리가 달라진다.
 * 레벨에 PlayerStart를 여러 개 두고 각각 PlayerStartTag를 붙여 두면(예: FromStreet / FromAlley),
 * 그 문으로 들어온 이동이 바로 그 자리에 사람을 세운다. 태그가 안 맞으면 엔진 기본 규칙으로 물러난다.
 */
AActor* ASecretProjectGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    UGameFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGameFlowSubsystem>() : nullptr;
    const FName Tag = Flow ? Flow->ConsumePendingEntryTag() : NAME_None;

    if (!Tag.IsNone() && GetWorld())
    {
        for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
        {
            if (It->PlayerStartTag == Tag)
                return *It;
        }
        // 지금 라셀 레벨들은 PlayerStart에 태그가 안 붙어 있다(맵마다 하나뿐).
        // 진행표의 시작 자리 덮어쓰기가 뒤에서 자리를 바로잡으므로 여기선 조용히 물러난다.
    }

    return Super::ChoosePlayerStart_Implementation(Player);
}

/**
 * 안 보이는 임시 바닥 — 레벨 메시에 충돌이 없을 때만 깔린다.
 * 근본 해결은 레벨 쪽 메시에 충돌을 넣는 것이고, 이건 그때까지 게임이 돌게 하는 받침일 뿐이다.
 */
void ASecretProjectGameMode::SpawnFallbackFloor(float TopZ)
{
    UWorld* W = GetWorld();
    if (!W || bFallbackFloorSpawned) return;
    bFallbackFloorSpawned = true;

    const FVector Center(2400.f, 0.f, TopZ - 100.f);   // 두께 200, 윗면이 TopZ
    AActor* Floor = W->SpawnActor<AActor>(AActor::StaticClass(), Center, FRotator::ZeroRotator);
    if (!Floor) return;
#if WITH_EDITOR
    Floor->SetActorLabel(TEXT("[임시] 바닥 받침 — 레벨 충돌 생기면 안 깔림"));
#endif

    UBoxComponent* Box = NewObject<UBoxComponent>(Floor, TEXT("FallbackFloor"));
    Box->SetBoxExtent(FVector(6000.f, 6000.f, 100.f));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->SetHiddenInGame(true);
    Box->RegisterComponent();
    Floor->SetRootComponent(Box);
    Box->SetWorldLocation(Center);
}

void ASecretProjectGameMode::BeginPlay()
{
    Super::BeginPlay();

    // 저장된 설정 적용 + 필드 BGM(/Game/Audio/BGM/BGM_field 있으면). 타이틀 안 거쳐도 안전.
    UGameInstance* GI = GetGameInstance();
    if (GI)
    {
        USecretGameSettings::Load()->ApplyAll(GI);
        if (UGameAudioSubsystem* Audio = GI->GetSubsystem<UGameAudioSubsystem>())
            Audio->PlayBGM(TEXT("field"));
    }

    UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr;
    UWorld* W = GetWorld();
    if (!Flow || !W) return;

    // 엔진이 화면 왼쪽 위에 노란 글씨로 띄우는 경고(지향광 둘이 겹쳤다 등)를 끈다.
    // 플레이 화면에 개발용 글씨가 얹히면 그것부터 눈에 들어온다 — 경고는 로그로 계속 남는다.
#if !UE_BUILD_SHIPPING
    GAreScreenMessagesEnabled = false;
#endif

    // 진행 표시줄 — 스테이지가 바뀔 때마다 여기에 목표가 얹힌다.
    StageHud = W->SpawnActor<AStageHudActor>(AStageHudActor::StaticClass());

    // 옛 시험판 표시(월드 HUD·옛 스토리 창)를 여기서 먼저 재운다 — StoryDirector가 첫 틱에 창을 띄우므로
    // Tick까지 기다리면 늦는다.
    QuietLegacyOverlays();

    // ── 특정 칸부터 열어 보기 ────────────────────────────
    // 실행 인자로 스테이지를 지정하면 그 칸부터 시작한다. 예:
    //   UnrealEditor.exe <proj> /Game/Maps/Rasel/L22_Yoa_Room?stage=A0_Roof -game
    // 콘솔(`FlowGoStage`)이 안 열리는 창모드에서도 장면·전투를 바로 확인할 수 있는 길이다
    // (레벨·스토리 담당도 제 장면만 열어 볼 수 있다).
    const FString WantStage = UGameplayStatics::ParseOption(OptionsString, TEXT("stage"));
    if (!WantStage.IsEmpty())
    {
        FGameStage Probe;
        if (Flow->GetStage(FName(*WantStage), Probe))
        {
            UE_LOG(LogTemp, Log, TEXT("[GameFlow] 실행 인자로 %s 칸부터 시작한다."), *WantStage);
            Flow->GoToStage(FName(*WantStage));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameFlow] 실행 인자의 칸 '%s'이 진행표에 없다."), *WantStage);
        }
    }

    // ① 먼저 이 레벨을 진행에 기록한다. 여기서 스테이지가 정해지거나(레벨을 바로 재생한 경우),
    //    문으로 한 칸 넘어가거나, 다른 무대로 옮겨 갈 수 있다.
    //    ★이때 오는 OnStageChanged 방송은 아직 안 듣는다 — 들으면 "레벨에 막 들어온 참"이 아닌
    //      차림(bFreshLevel=false)이 먼저 돌아, 시작 자리 세우기를 건너뛰고 캐릭터가 PlayerStart에 박힌다.
    const FName Here = FName(*W->GetOutermost()->GetName());
    Flow->NotifyLevelEntered(Here);

    // ② 이제 듣는다(같은 레벨 안에서 칸이 넘어갈 때 다음 장면을 차리기 위해).
    Flow->OnStageChanged.AddDynamic(this, &ASecretProjectGameMode::HandleStageChanged);

    // ③ 차림은 늘 돈다. 지금 레벨이 현재 스테이지의 무대면 그 스테이지대로(자리·장면·목표),
    //    아니면(목적지 이동·문으로 딴 구역에 온 것) 자유 탐험 허브로(상주 사람 + "라셀" 표시).
    //    판별은 SetupCurrentStage 안의 bStageIsHere가 한다 — 예전엔 여기서 return 해 버려 허브가 텅 비었다.
    SetupCurrentStage(true);
}

void ASecretProjectGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGameFlowSubsystem* Flow = GI->GetSubsystem<UGameFlowSubsystem>())
            Flow->OnStageChanged.RemoveDynamic(this, &ASecretProjectGameMode::HandleStageChanged);
    }
    Super::EndPlay(Reason);
}

/**
 * 스테이지가 바뀌었다.
 * 무대가 이 레벨이면 그 자리에서 다음 장면을 차리고, 다른 레벨이면 곧 그 레벨이 열리므로 여기선 아무것도 안 한다
 * (그쪽 레벨의 게임모드가 제 차례에 차린다).
 */
void ASecretProjectGameMode::HandleStageChanged(FName /*StageId*/)
{
    UGameInstance* GI = GetGameInstance();
    UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr;
    UWorld* W = GetWorld();
    if (!Flow || !W) return;

    FGameStage St;
    if (!Flow->GetStage(Flow->GetCurrentStageId(), St)) return;

    const FName Here = FName(*W->GetOutermost()->GetName());
    if (!UGameFlowSubsystem::LevelNameEquals(St.LevelPath, Here))
        return;   // 다른 무대로 옮겨 가는 중

    SetupCurrentStage(false);
}

/**
 * 이 스테이지를 차린다.
 * bFreshLevel = 레벨을 막 열고 들어온 참(자리 세우기·오프닝이 여기서만 일어난다).
 * 같은 레벨 안에서 칸만 넘어간 경우엔 사람을 옮기지 않는다 — 서 있던 자리에서 다음 장면이 이어진다.
 */
void ASecretProjectGameMode::SetupCurrentStage(bool bFreshLevel)
{
    bStageSetupDone = true;

    UGameInstance* GI = GetGameInstance();
    UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr;
    UWorld* W = GetWorld();
    if (!Flow || !W) return;

    ClearStageActors();

    FGameStage Stage;
    const bool bHasStage = Flow->GetStage(Flow->GetCurrentStageId(), Stage);

    // ── 지금 발을 딛은 레벨이 "현재 스토리 스테이지의 무대"인가? ──
    // 목적지 이동 맵으로 딴 구역에 들어와 있을 수 있다. 그때는 이 스테이지의 시작 자리·장면·목표를
    // 적용하면 안 된다(스토리 스폰으로 순간이동되거나, 여기에 없는 목표가 뜬다). 그저 자유롭게 논다.
    const FName ThisLevel = FName(*W->GetOutermost()->GetName());
    bStageIsHere = bHasStage && UGameFlowSubsystem::LevelNameEquals(Stage.LevelPath, ThisLevel);

    // ── 시작 자리 바로잡기 — 레벨의 PlayerStart가 쓸 자리에 없을 때 진행표가 이긴다 ──
    if (bFreshLevel && bStageIsHere && Stage.bOverrideSpawn)
    {
        if (APlayerController* PC = W->GetFirstPlayerController())
        {
            if (APawn* P = PC->GetPawn())
            {
                // 발이 닿는 자리에 세운다. 스폰 위에서 아래로 훑어 실제 바닥 높이를 찾는다.
                // 레벨 바닥 높이는 레벨 담당이 계속 고치므로 좌표를 박아 두지 않는다.
                FVector Loc = Stage.SpawnLocation;
                const float HalfHeight = P->GetSimpleCollisionHalfHeight();

                float GroundZ;
                FHitResult Hit;
                FCollisionQueryParams Q(SCENE_QUERY_STAT(FlowSpawnGround), false, P);
                // ★훑기는 요청한 높이 바로 위(150)에서 시작한다.
                //   위에서(500) 훑으면 실내에서 **천장·옥상 슬래브**가 먼저 걸려, 방 안에 세우려던 사람이
                //   지붕 위에 선다(L22 셋방에서 실제로 그랬다 — 지붕 z=324가 먼저 잡혔다).
                const FVector From = Loc + FVector(0, 0, 150.f);
                const FVector To   = Loc - FVector(0, 0, 3000.f);
                if (W->LineTraceSingleByChannel(Hit, From, To, ECC_WorldStatic, Q))
                {
                    GroundZ = Hit.ImpactPoint.Z;   // 진짜 충돌이 있으면 그 위에 선다
                }
                else
                {
                    // 발밑에 충돌이 없다(레벨 메시가 단순 충돌 없이 구워짐 — 메시는 레벨 담당 소관).
                    // 진행표의 스폰 z를 "이 레벨의 지면 높이"로 보고 그 높이에 안 보이는 임시 바닥을 깐다.
                    // ★레벨에 진짜 충돌이 붙는 순간 위 훑기가 성공해 이 임시 바닥은 저절로 안 깔린다.
                    GroundZ = Stage.SpawnLocation.Z;
                    UE_LOG(LogTemp, Warning,
                        TEXT("[GameFlow] %s 아래에 충돌 바닥이 없다 — 지면 z=%.0f에 임시 바닥을 깐다(레벨 충돌 필요)."),
                        *Stage.SpawnLocation.ToCompactString(), GroundZ);
                    SpawnFallbackFloor(GroundZ);
                }

                Loc.Z = GroundZ + HalfHeight + 2.f;   // 발이 지면에 닿게
                StageGroundZ  = GroundZ;              // 떨어짐 판정 기준
                StageSpawnLoc = Loc;
                bHasStageSpawn = true;
                P->SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
                P->SetActorRotation(FRotator(0.f, Stage.SpawnYaw, 0.f));
                // 카메라를 내려다보게 시작(-16°) — 높은 3인칭에서 앞 거리가 넓게 보이게.
                PC->SetControlRotation(FRotator(-16.f, Stage.SpawnYaw, 0.f));
                UE_LOG(LogTemp, Log, TEXT("[GameFlow] 시작 자리: %s (요청 %s · %s)"),
                    *Loc.ToCompactString(), *Stage.SpawnLocation.ToCompactString(), *P->GetName());
            }
        }
    }

    PlaceResidentNpcs();                   // 구역 상주 사람은 늘 세운다(거리를 살아 있게)
    if (bStageIsHere) PlaceSceneNpcs();    // 장면용 사람은 그 스테이지의 무대에서만 세운다

    // ── 새 게임이면 먼저 오프닝, 그다음에 도착 장면 ──
    if (bFreshLevel && Flow->ConsumeOpeningPending() && OpeningDirectorClass)
    {
        if (AGameOpeningDirector* Dir = W->SpawnActor<AGameOpeningDirector>(OpeningDirectorClass))
        {
            if (bHasStage && !Stage.ObjectiveLabel.IsEmpty())
                Dir->ObjectiveText = Stage.ObjectiveLabel;

            // 도착 장면이 뒤따르면 오프닝의 "첫 목표 한 줄"은 접는다.
            // 안 접으면 조작을 돌려준 뒤 4~5초 동안 플레이어가 걸어 다니다가 갑자기 대사에 붙들린다
            // (목표는 어차피 진행 표시줄에 계속 떠 있다).
            if (bHasStage && Stage.ArrivalScene.Num() > 0)
            {
                Dir->ObjectiveText = FText::GetEmpty();
                Dir->ObjectiveHold = 0.15f;
            }
            if (bHasStage && !Stage.TransitionCard.IsEmpty())
            {
                Dir->PlaceCardText = Stage.TransitionCard;   // 때·자리는 진행표가 정한다
                bCardShownByOpening = true;
            }

            Dir->OnFinished = FSimpleDelegate::CreateUObject(this, &ASecretProjectGameMode::ProceedAfterEntry);
            if (StageHud) StageHud->SetHidden(true);
            Dir->Begin(W->GetFirstPlayerController());
            return;
        }
    }

    ProceedAfterEntry();
}

/**
 * 진입(과 오프닝)이 끝난 뒤 갈림길 — 페르소나 기본 구조의 분기점.
 *   - Episode 칸(+ArrivalScene 있음) : 컷신을 튼다(현행 스크립트 흐름).
 *   - Hub 칸(기본)                    : 대사를 강제하지 않는다. 바로 자유 탐험 — 목표는 소프트 힌트,
 *                                       사람에게 말 걸고 거리를 돈다. 목표 자리에 닿으면 그때 에피소드가 열린다.
 */
void ASecretProjectGameMode::ProceedAfterEntry()
{
    UGameInstance* GI = GetGameInstance();
    UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr;
    FGameStage Stage;
    const bool bHasStage = Flow && Flow->GetStage(Flow->GetCurrentStageId(), Stage);

    // 컷신은 "그 스테이지의 무대에 실제로 서 있을 때"만 튼다. 딴 구역에 자유로 와 있으면 자유 탐험.
    if (bStageIsHere && bHasStage && Stage.Kind == EStageKind::Episode && Stage.ArrivalScene.Num() > 0)
    {
        PlayArrivalScene();   // 컷신
    }
    else
    {
        EnterHubExplore();    // 자유 탐험 (대사 강제 없음)
    }
}

/** 허브 진입 — 대사 없이 곧바로 조작. */
void ASecretProjectGameMode::EnterHubExplore()
{
    if (StageHud) StageHud->SetHidden(false);

    if (bStageIsHere)
    {
        SpawnObjective();   // 지금 이 구역이 이야기의 무대다 — 목표를 세운다.
        return;
    }

    // 딴 구역에 자유로 들어와 있다 — 여기엔 목표 표식이 없다. 화살표 없이 "지금 이야기가 부르는 곳"만 떠올려 준다.
    UGameInstance* GI = GetGameInstance();
    UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr;
    FGameStage Stage;
    if (StageHud && Flow && Flow->GetStage(Flow->GetCurrentStageId(), Stage))
        StageHud->SetObjective(FText::FromString(TEXT("라셀")), Stage.ObjectiveLabel, false, FVector::ZeroVector);
}

/** 사람 한 묶음을 세운다(장면용·상주 공용). 레벨 바닥을 훑어 발을 맞춘다. */
void ASecretProjectGameMode::SpawnNpcRoster(const TArray<FStageNpc>& Roster, const TCHAR* LabelTag)
{
    UWorld* W = GetWorld();
    if (!W) return;

    for (const FStageNpc& N : Roster)
    {
        const FString Path = N.BlueprintPath.IsEmpty() ? DefaultNpcBlueprintPath : N.BlueprintPath;
        UClass* Cls = !Path.IsEmpty() ? LoadClass<AActor>(nullptr, *Path) : nullptr;
        if (!Cls) Cls = AANPCCharacter::StaticClass();   // 몸을 못 찾으면 기본 NPC로

        // 바닥을 훑어 발을 맞춘다(레벨마다 바닥 높이가 다르다).
        FVector Loc = N.Location;
        FHitResult Hit;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(StageNpcGround), false);
        if (W->LineTraceSingleByChannel(Hit, Loc + FVector(0, 0, 400.f), Loc - FVector(0, 0, 2000.f), ECC_WorldStatic, Q))
            Loc.Z = Hit.ImpactPoint.Z + 90.f;

        FActorSpawnParameters Sp;
        Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (AActor* A = W->SpawnActor<AActor>(Cls, Loc, FRotator(0.f, N.Yaw, 0.f), Sp))
        {
#if WITH_EDITOR
            A->SetActorLabel(FString::Printf(TEXT("[%s] %s"), LabelTag, *N.Name));
#endif
            if (AANPCCharacter* Npc = Cast<AANPCCharacter>(A))
            {
                Npc->NPCName = N.Name;
                if (N.Lines.Num() > 0)
                    Npc->DialogueLines = N.Lines;   // 말 걸면 이 대사(소문·정보)
            }
            StageNpcs.Add(A);
        }
    }
}

/** 진행표(현재 스테이지)에 적힌 장면용 사람을 세운다. */
void ASecretProjectGameMode::PlaceSceneNpcs()
{
    UGameInstance* GI = GetGameInstance();
    UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr;
    if (!Flow) return;

    FGameStage Stage;
    if (!Flow->GetStage(Flow->GetCurrentStageId(), Stage)) return;

    SpawnNpcRoster(Stage.SceneNpcs, TEXT("장면"));
}

/** 이 구역에 늘 서 있는 상주 사람들을 세운다(스테이지와 무관 — 거리를 살아 있게). */
void ASecretProjectGameMode::PlaceResidentNpcs()
{
    UWorld* W = GetWorld();
    if (!W) return;

    const FName ThisLevel = FName(*W->GetOutermost()->GetName());
    SpawnNpcRoster(UGameFlowSubsystem::GetResidentNpcs(ThisLevel), TEXT("상주"));
}

/** 도착 장면 — 때·자리 카드 + 대사. 끝나면 목표가 선다. */
void ASecretProjectGameMode::PlayArrivalScene()
{
    UGameInstance* GI = GetGameInstance();
    UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr;
    UWorld* W = GetWorld();
    if (!Flow || !W) { OnArrivalSceneDone(); return; }

    FGameStage Stage;
    if (!Flow->GetStage(Flow->GetCurrentStageId(), Stage)) { OnArrivalSceneDone(); return; }

    if (StageHud) StageHud->SetHidden(true);

    // 오프닝이 이미 그 자막을 띄웠으면 같은 카드를 두 번 보여 주지 않는다.
    const FText Card = bCardShownByOpening ? FText::GetEmpty() : Stage.TransitionCard;
    bCardShownByOpening = false;

    AStorySceneDirector::Play(W, W->GetFirstPlayerController(), Card, Stage.ArrivalScene,
        FSimpleDelegate::CreateUObject(this, &ASecretProjectGameMode::OnArrivalSceneDone));
}

void ASecretProjectGameMode::OnArrivalSceneDone()
{
    if (StageHud) StageHud->SetHidden(false);
    SpawnObjective();
}

/**
 * 목표를 세운다.
 *  - ReachSpot  : 진행표의 자리에 표식을 세운다.
 *  - EnterLevel : 그 레벨로 가는 문(포탈)을 이 레벨에서 찾아 그 앞을 목표로 삼는다.
 *                 (문 좌표를 진행표에 박지 않는다 — 레벨이 문을 옮기면 저절로 따라간다.)
 *  - SceneOnly  : 목표 없이 바로 다음 칸.
 */
void ASecretProjectGameMode::SpawnObjective()
{
    UGameInstance* GI = GetGameInstance();
    UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr;
    UWorld* W = GetWorld();
    if (!Flow || !W) return;

    FGameStage Stage;
    if (!Flow->GetStage(Flow->GetCurrentStageId(), Stage)) return;

    if (Stage.Goal == EStageGoal::SceneOnly)
    {
        if (StageHud)
            StageHud->SetObjective(FText::FromString(Stage.DisplayName), FText::GetEmpty(), false, FVector::ZeroVector);

        // 장면만 있는 칸 — 볼 것을 다 봤으면 다음 칸으로. 다음 칸이 없으면 여기가 지금 지어진 끝이다.
        if (Flow->HasNextStage())
            Flow->AdvanceStage();
        else
            UE_LOG(LogTemp, Log, TEXT("[GameFlow] 진행표의 마지막 칸이다 — 여기까지."));
        return;
    }

    bool bHasSpot = false;
    FVector Spot = FVector::ZeroVector;

    if (Stage.Goal == EStageGoal::EnterLevel && !Stage.GoalLevel.IsNone())
    {
        for (TActorIterator<APortalActor> It(W); It; ++It)
        {
            if (UGameFlowSubsystem::LevelNameEquals(It->GetTargetLevelName(), Stage.GoalLevel))
            {
                Spot = It->GetActorLocation();
                bHasSpot = true;
                break;
            }
        }
        if (!bHasSpot)
            UE_LOG(LogTemp, Warning, TEXT("[GameFlow] 이 레벨엔 %s로 가는 문이 없다 — 목표 표식 없이 진행."),
                *Stage.GoalLevel.ToString());
    }
    else if (Stage.bHasObjective)
    {
        Spot = Stage.ObjectiveLocation;
        bHasSpot = true;

        FActorSpawnParameters Sp;
        Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ObjectiveActor = W->SpawnActor<AStageObjectiveActor>(
            AStageObjectiveActor::StaticClass(), Spot, FRotator::ZeroRotator, Sp);
        if (ObjectiveActor)
            ObjectiveActor->Setup(Stage.ObjectiveRadius, Stage.ObjectiveLabel);
    }

    if (StageHud)
        StageHud->SetObjective(FText::FromString(Stage.DisplayName), Stage.ObjectiveLabel, bHasSpot, Spot);
}

void ASecretProjectGameMode::NotifyObjectiveReached()
{
    UGameInstance* GI = GetGameInstance();
    UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr;
    UWorld* W = GetWorld();
    if (!Flow || !W) return;

    FGameStage Stage;
    if (!Flow->GetStage(Flow->GetCurrentStageId(), Stage)) return;

    if (StageHud) StageHud->SetHidden(true);

    // 목표 자리의 장면 → 끝나면 한 칸.
    AStorySceneDirector::Play(W, W->GetFirstPlayerController(), FText::GetEmpty(), Stage.ObjectiveScene,
        FSimpleDelegate::CreateUObject(this, &ASecretProjectGameMode::OnObjectiveSceneDone));
}

void ASecretProjectGameMode::OnObjectiveSceneDone()
{
    if (StageHud) StageHud->SetHidden(false);

    UGameInstance* GI = GetGameInstance();
    if (UGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UGameFlowSubsystem>() : nullptr)
    {
        if (!Flow->AdvanceStage())
            UE_LOG(LogTemp, Log, TEXT("[GameFlow] 마지막 칸을 마쳤다 — 진행표에 다음이 없다."));
    }
}

/**
 * 떨어진 캐릭터 건져 올리기.
 * 레벨 메시에 충돌이 없는 자리가 남아 있어 걷다 보면 바닥이 끊긴다. 그대로 두면 캐릭터가
 * 한없이 떨어져 화면에 하늘밖에 안 남는다(=게임이 멈춘 것과 같다). 지면보다 1,500 아래로
 * 내려가면 이 스테이지의 시작 자리로 되돌리고, 몇 번 건졌는지 로그에 남긴다
 * (그 숫자가 곧 레벨 담당에게 넘길 "충돌 없는 자리"의 증거다).
 */
void ASecretProjectGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    FallCheckTime += DeltaSeconds;
    if (FallCheckTime < 0.5f) return;
    FallCheckTime = 0.f;

    UWorld* W = GetWorld();
    APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
    APawn* P = PC ? PC->GetPawn() : nullptr;
    if (!P) return;

    QuietLegacyOverlays();   // 놓친 프레임이 있을 때를 위한 되짚기

    if (!bHasStageSpawn) return;

    const FVector Loc = P->GetActorLocation();
    if (Loc.Z > StageGroundZ - 1500.f) return;

    ++RescueCount;
    UE_LOG(LogTemp, Warning,
        TEXT("[GameFlow] 캐릭터가 %s까지 떨어졌다(지면 z=%.0f) — 시작 자리로 되돌린다(%d번째). 레벨에 충돌 없는 자리가 있다."),
        *Loc.ToCompactString(), StageGroundZ, RescueCount);

    P->SetActorLocation(StageSpawnLoc, false, nullptr, ETeleportType::TeleportPhysics);
    if (ACharacter* C = Cast<ACharacter>(P))
    {
        if (UCharacterMovementComponent* M = C->GetCharacterMovement())
            M->StopMovementImmediately();
    }
}

/**
 * 옛 시험판 표시 잠재우기.
 *
 * 스토리 진행 중에 저절로 뜨는 옛것 둘:
 *   ①상시 월드 HUD(1일차 아침·0G·"새 퀘스트: 첫 보물 사냥")
 *   ②옛 스토리 카탈로그 창(「괴담 — 거울 너머의 나」) — 캐릭터의 StoryDirector가 **첫 틱에** 띄운다.
 * 둘 다 다른 담당의 시스템이라 **지우지 않는다.** 저절로 뜨는 것만 끄고, 손으로 여는 길은 남긴다.
 * ★게임모드 BeginPlay에서 한 번 부르고, 혹시 놓친 프레임을 위해 Tick에서도 한 번 더 부른다.
 */
void ASecretProjectGameMode::QuietLegacyOverlays()
{
    if (bHidLegacyHud) return;

    UWorld* W = GetWorld();
    APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
    APawn* P = PC ? PC->GetPawn() : nullptr;
    if (!P) return;

    // 옛 스토리 창은 첫 틱에 뜨므로 여기서 곧바로 막는다(플래그 쓰기는 여러 번 해도 무해).
    if (UStoryDirectorComponent* Dir = P->FindComponentByClass<UStoryDirectorComponent>())
    {
        Dir->bAutoShowWidget = false;
        Dir->bAutoCheckOnDayChange = false;
    }

    // ★상시 월드 HUD는 캐릭터가 제 BeginPlay에서 만든다 — 게임모드가 먼저 돌면 아직 없다.
    //   실제로 껐을 때만 "다 했다"로 치고, 아니면 다음 틱에 다시 시도한다.
    if (APlayerCharacter* PCh = Cast<APlayerCharacter>(P))
    {
        if (PCh->SetWorldHudHidden(true))
            bHidLegacyHud = true;
    }
    else
    {
        bHidLegacyHud = true;   // 우리 캐릭터가 아니면 더 볼 것이 없다
    }
}

/** 앞 칸이 세워 둔 것 치우기 — 목표 표식과 장면용 사람. */
void ASecretProjectGameMode::ClearStageActors()
{
    if (ObjectiveActor) { ObjectiveActor->Destroy(); ObjectiveActor = nullptr; }
    for (TObjectPtr<AActor>& A : StageNpcs)
        if (A) A->Destroy();
    StageNpcs.Reset();
}
