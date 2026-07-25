#include "BattleManager.h"
#include "BattleHUDWidget.h"
#include "ABaseCharacter.h"
#include "ANPCCharacter.h"
#include "StatComponent.h"
#include "InventoryComponent.h"
#include "QuestComponent.h"
#include "TimeComponent.h"
#include "RelationshipComponent.h" // 유대 전투 보너스(호출만)
#include "BestiarySubsystem.h"
#include "StoryManager.h"
#include "Engine/GameInstance.h"
#include "GameAudioSubsystem.h"   // 연출 효과음 자동 재생
#include "AssetResolver.h"        // 이름으로 VFX 로드
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"            // 스폰 컴포넌트 업캐스트(USceneComponent)
#include "Particles/ParticleSystem.h"   // FXVarietyPack(Cascade) 이펙트 스폰
#include "Particles/ParticleSystemComponent.h"
#include "SecretSaveGame.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "BattleFX.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"        // 발바닥 = 캡슐 중심 - half-height
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"

ABattleManager::ABattleManager()
{
    PrimaryActorTick.bCanEverTick = true; // 전투 시작 연출(점프 배치) 보간용 — 평상시엔 무동작
}

// ── 도감(Bestiary) 헬퍼 ──
static UBestiarySubsystem* GetBestiary(UWorld* W)
{
    if (W && W->GetGameInstance())
        return W->GetGameInstance()->GetSubsystem<UBestiarySubsystem>();
    return nullptr;
}
static FName EnemyIdOf(AABaseCharacter* C)
{
    if (AANPCCharacter* N = Cast<AANPCCharacter>(C))
        return FName(*N->NPCName);
    return NAME_None;
}

// ── 전투 시작 ────────────────────────────────────────────

void ABattleManager::StartBattle(const TArray<AABaseCharacter*>& InParty, const TArray<AABaseCharacter*>& InEnemies, EBattleInitiative Initiative)
{
    PlayerParty = InParty;
    Enemies = InEnemies;

    // 페르소나식 인원 제한: 주변 NPC를 전부 끌어모으는 진입부(OnClick_PersonaBattle) 보정.
    // 플레이어(파티 0번)·talk한 적(적 0번)은 앞쪽이라 항상 유지됨. 적 캡은 단일타겟 UI(4슬롯)와도 정합.
    if (MaxPartySize > 0 && PlayerParty.Num() > MaxPartySize)
        PlayerParty.SetNum(MaxPartySize);
    if (MaxEnemies > 0 && Enemies.Num() > MaxEnemies)
        Enemies.SetNum(MaxEnemies);

    CurrentTurnIndex = 0;
    CurrentEnemyTarget = 0;
    CurrentAllyTarget = 0;
    bBattleActive = true;
    bAllOutReady = false;
    DownedEnemies.Empty();
    KnownWeaknesses.Empty();
    EnragedBosses.Empty();
    BossPhase.Empty();
    BatonStack = 0;

    PlayBGM(TEXT("battle"));   // 전투 BGM (BGM_battle 있으면). 종료 시 RemoveHUD에서 field 복귀.
    ResultMessage.Empty();
    ActionFeedback.Empty();

    // 전투 한정 1회성 상태(근성 등) 리셋 — 직전 전투에서 발동했어도 새 전투엔 다시 가능
    for (AABaseCharacter* C : PlayerParty)
        if (C && C->GetStatComponent()) C->GetStatComponent()->ResetBattleOnce();
    for (AABaseCharacter* C : Enemies)
        if (C && C->GetStatComponent()) C->GetStatComponent()->ResetBattleOnce();

    // ★ 현재 레벨까지의 트리 스킬을 즉시 보유 보장 — 영입/사전레벨업 유닛이 시작스킬(2~3개)만 갖던 문제 해결.
    //   LearnSkillsUpToLevel은 중복 습득을 막으므로 안전. 레벨이 높을수록 스킬 메뉴가 풍성해짐.
    for (AABaseCharacter* C : PlayerParty)
        if (C && C->GetStatComponent())
            C->LearnSkillsUpToLevel(C->GetStatComponent()->GetLevel());

    // 적 스케일링: 난이도(레벨) 보정 + 시간대(밤) 보정 — 둘 다 옵트인, 곱연산
    if (bScaleEnemiesToParty || bNightEnemiesStronger)
    {
        // 파티 평균 레벨
        int32 LevelSum = 0, LevelCount = 0;
        for (AABaseCharacter* P : PlayerParty)
            if (P && P->GetStatComponent())
            {
                LevelSum += P->GetStatComponent()->GetLevel();
                ++LevelCount;
            }
        const float PartyAvgLevel = (LevelCount > 0) ? (float)LevelSum / LevelCount : 1.f;

        // 시간대 배율 (B TimeComponent 연동): 저녁 ×1.1, 밤 ×1.25
        float NightFactor = 1.f;
        if (bNightEnemiesStronger)
        {
            EDayPhase Phase = EDayPhase::Day;
            if (UWorld* W = GetWorld())
                if (APlayerController* PC = W->GetFirstPlayerController())
                    if (APawn* Pawn = PC->GetPawn())
                        if (UTimeComponent* TC = Pawn->FindComponentByClass<UTimeComponent>())
                            Phase = TC->GetPhase();
            if (Phase == EDayPhase::Evening)   NightFactor = 1.1f;
            else if (Phase == EDayPhase::Night) NightFactor = 1.25f;
        }

        for (AABaseCharacter* E : Enemies)
            if (E && E->GetStatComponent())
            {
                if (E->BossLevel > 0) continue; // 레이드 보스는 아래 전용 블록에서 스케일(이중 적용 방지)

                float PowerMult = 1.f, HPMult = 1.f;
                if (bScaleEnemiesToParty)
                {
                    // ★단방향 레벨 추적: 파티가 적보다 높을 때만 적을 끌어올림(공/방 + HP).
                    //   → 만렙(100)까지 잡몹이 도전 유지. 파티가 약하면 적은 카탈로그 원본 그대로(과보호 방지).
                    const int32 EnemyLevel = E->GetStatComponent()->GetLevel();
                    const float Gap = PartyAvgLevel - (float)EnemyLevel;
                    if (Gap > 0.f)
                    {
                        PowerMult += Gap * ScalePerLevel;        // 공/방
                        HPMult    += Gap * EnemyHPScalePerLevel; // HP(한 방에 안 죽게)
                    }
                    if (E->bIsBoss) { PowerMult *= 1.3f; HPMult *= 1.5f; } // 보스는 더 강하게
                }
                PowerMult *= NightFactor; // 시간대 보정은 공/방에만(밤 압박감)
                PowerMult = FMath::Clamp(PowerMult, 0.5f, MaxEnemyPowerScale);
                HPMult    = FMath::Clamp(HPMult, 1.f, MaxEnemyHPScale);
                // ApplyRaidScale = MaxHP×HPMult + 공/방 BattleScale(둘 중 큰 쪽). 레이드 스케일과 동일 경로.
                E->GetStatComponent()->ApplyRaidScale(HPMult, PowerMult);
            }
    }

    // ★ 레이드 보스 스케일링: BossLevel>0이면 그 레벨에 맞춰 HP/공격력 대폭 강화(플레이어 상한 100을 넘는 레이드 티어).
    //   HPMult = Lv/12 (Lv150≈12.5배 HP, Lv200≈16.7배), PowerMult = 1 + (Lv-100)*1.5%p (Lv150≈1.75배, Lv200≈2.5배).
    for (AABaseCharacter* E : Enemies)
        if (E && E->BossLevel > 0 && E->GetStatComponent())
        {
            const float L = (float)E->BossLevel;
            const float HPMult = FMath::Max(1.f, L / 12.f);
            const float PowerMult = 1.f + FMath::Max(0.f, L - 100.f) * 0.015f;
            E->GetStatComponent()->ApplyRaidScale(HPMult, PowerMult);
        }

    // 유대(코프) 전투 보너스: 영입 아군은 플레이어와의 인연 랭크만큼 강해진다(페르소나식). 아군 BattleScale에 반영(아군은 난이도 스케일 미사용이라 충돌 0).
    if (BondCombatBonusPerRank > 0.f)
        if (UWorld* W = GetWorld())
            if (APlayerController* PC = W->GetFirstPlayerController())
                if (APawn* Pawn = PC->GetPawn())
                    if (URelationshipComponent* Rel = Pawn->FindComponentByClass<URelationshipComponent>())
                        for (AABaseCharacter* P : PlayerParty)
                            if (AANPCCharacter* Ally = Cast<AANPCCharacter>(P))
                                if (Ally->GetStatComponent() && !Ally->NPCName.IsEmpty())
                                {
                                    const int32 Rank = Rel->GetRank(FName(*Ally->NPCName));
                                    if (Rank > 0)
                                        Ally->GetStatComponent()->SetBattleScale(1.f + Rank * BondCombatBonusPerRank);
                                    // 유대 근성 각성: 높은 인연 랭크 아군은 근성(전투당 1회 치명타 버팀) 획득
                                    if (BondEndureRank > 0 && Rank >= BondEndureRank)
                                        Ally->GetStatComponent()->SetEndureOnce(true);
                                }

    // 도감: 전투 진입 = 조우 기록(처치/약점발견 전이라도 ??? 항목으로 등장)
    if (UBestiarySubsystem* Bst = GetBestiary(GetWorld()))
        for (AABaseCharacter* E : Enemies)
            if (E) Bst->RecordEncounter(EnemyIdOf(E));

    CreateHUD();

    // 선제/기습: HUD 생성 후 적용해야 어드밴티지 연출(Flair)을 HUD가 구독 상태로 받음.
    // CurrentTurnIndex(선공 진영)를 정하므로 반드시 NextTurn 이전에 호출.
    ApplyInitiative(Initiative);

    OnBattleStarted.Broadcast();

    // ★ 페르소나식 전투 시작 연출: 진형으로 점프 배치 + 시네마틱 카메라. 끝나면 NextTurn.
    if (bEnableBattleIntro) BeginBattleIntro();
    else NextTurn();
}

// ── 전투 시작 연출 (진형 점프 배치 + 시네마틱 카메라) ──────
void ABattleManager::BeginBattleIntro()
{
    IntroSlots.Reset();
    UWorld* W = GetWorld();
    if (!W) { FinishBattleIntro(); return; }

    TArray<AABaseCharacter*> Party, Foe;
    for (AABaseCharacter* C : PlayerParty) if (C) Party.Add(C);
    for (AABaseCharacter* C : Enemies)     if (C) Foe.Add(C);
    if (Party.Num() == 0 || Foe.Num() == 0) { FinishBattleIntro(); return; }

    auto Centroid = [](const TArray<AABaseCharacter*>& A) -> FVector
    {
        FVector S = FVector::ZeroVector;
        for (AABaseCharacter* C : A) S += C->GetActorLocation();
        return A.Num() ? S / A.Num() : FVector::ZeroVector;
    };
    const FVector PCen = Centroid(Party);
    const FVector ECen = Centroid(Foe);
    const FVector Center = (PCen + ECen) * 0.5f;

    FVector Axis = ECen - PCen; Axis.Z = 0.f;
    if (Axis.IsNearlyZero()) Axis = FVector(1.f, 0.f, 0.f);
    Axis.Normalize();
    const FVector Right = FVector::CrossProduct(FVector::UpVector, Axis).GetSafeNormal();

    // 한 진영 배치: 중앙±Depth 라인 + 좌우 spread, 상대 진영 바라봄
    auto Layout = [&](const TArray<AABaseCharacter*>& Side, float DepthSign, const FRotator& Face)
    {
        const int32 N = Side.Num();
        const float Span = (N - 1) * FormationSpacing;
        for (int32 i = 0; i < N; ++i)
        {
            AABaseCharacter* C = Side[i];
            const float Off = (i * FormationSpacing) - Span * 0.5f;
            FVector T = Center + Axis * (DepthSign * FormationDepth) + Right * Off;
            T.Z = C->GetActorLocation().Z;  // 지면 높이 유지(낙하 방지)
            FIntroSlot Slot;
            Slot.Actor = C; Slot.Start = C->GetActorLocation(); Slot.Target = T;
            Slot.StartRot = C->GetActorRotation(); Slot.TargetRot = Face;
            IntroSlots.Add(Slot);
        }
    };
    Layout(Party, -1.f, Axis.Rotation());      // 아군: 뒤쪽(-Axis), 적을 바라봄
    Layout(Foe,   +1.f, (-Axis).Rotation());   // 적: 앞쪽(+Axis), 아군을 바라봄

    // 시네마틱 카메라: 아군 뒤·측면·위에서 중앙을 바라봄(¾ 영웅 구도)
    const FVector CamPos = Center - Axis * CamBackOffset + Right * CamSideOffset + FVector(0.f, 0.f, CamHeight);
    const FVector LookAt = Center + FVector(0.f, 0.f, 90.f);
    const FRotator CamRot = (LookAt - CamPos).Rotation();
    if (APlayerController* PCtrl = W->GetFirstPlayerController())
    {
        SavedViewTarget = PCtrl->GetViewTarget();
        FActorSpawnParameters Sp; Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        BattleCamera = W->SpawnActor<ACameraActor>(CamPos, CamRot, Sp);
        if (BattleCamera)
            PCtrl->SetViewTargetWithBlend(BattleCamera, CamBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
    }

    bIntroPlaying = true;
    IntroElapsed = 0.f;

    PlaySFX(TEXT("battlestart"));   // 전투 시작 연출 사운드(SFX_battlestart 있으면)
}

void ABattleManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bIntroPlaying)
    {
        // 턴 추종 카메라: BattleCamera를 현재 턴 유닛 뒤 목표 포즈로 부드럽게 보간(매 턴 컷 전환).
        if (bCamFollow && BattleCamera)
        {
            const FVector L = FMath::VInterpTo(BattleCamera->GetActorLocation(), CamTargetLoc, DeltaSeconds, TurnCamFollowSpeed);
            const FRotator R = FMath::RInterpTo(BattleCamera->GetActorRotation(), CamTargetRot, DeltaSeconds, TurnCamFollowSpeed);
            BattleCamera->SetActorLocationAndRotation(L, R);
        }
        return;
    }

    IntroElapsed += DeltaSeconds;
    const float A = (IntroDuration > 0.f) ? FMath::Clamp(IntroElapsed / IntroDuration, 0.f, 1.f) : 1.f;
    const float Ease = 1.f - FMath::Pow(1.f - A, 3.f);           // ease-out cubic
    const float Arc  = FMath::Sin(A * PI) * IntroJumpHeight;      // 점프 아치(올라갔다 착지)

    for (FIntroSlot& S : IntroSlots)
    {
        AABaseCharacter* C = S.Actor.Get();
        if (!C) continue;
        FVector P = FMath::Lerp(S.Start, S.Target, Ease);
        P.Z += Arc;
        C->SetActorLocation(P, false, nullptr, ETeleportType::TeleportPhysics);
        C->SetActorRotation(FMath::RInterpTo(C->GetActorRotation(), S.TargetRot, DeltaSeconds, 12.f));
    }

    if (A >= 1.f) FinishBattleIntro();
}

void ABattleManager::FinishBattleIntro()
{
    bIntroPlaying = false;
    for (FIntroSlot& S : IntroSlots)
        if (AABaseCharacter* C = S.Actor.Get())
        {
            C->SetActorLocation(S.Target, false, nullptr, ETeleportType::TeleportPhysics);
            C->SetActorRotation(S.TargetRot);
        }
    IntroSlots.Reset();
    NextTurn();   // 연출 끝 → 실제 전투 시작
}

void ABattleManager::RestoreFieldCamera()
{
    bIntroPlaying = false;
    if (UWorld* W = GetWorld())
        if (APlayerController* PCtrl = W->GetFirstPlayerController())
        {
            AActor* Back = SavedViewTarget.Get();
            if (!Back) Back = PCtrl->GetPawn();
            if (Back) PCtrl->SetViewTargetWithBlend(Back, CamBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
        }
    if (BattleCamera) { BattleCamera->Destroy(); BattleCamera = nullptr; }
    bCamFollow = false;
    UpdateTurnMarker(nullptr);   // 전투 종료 → 턴 마커 제거
}

void ABattleManager::FrameCameraOnActor(AABaseCharacter* Actor)
{
    if (!bTurnCamera || !Actor) return;
    UWorld* W = GetWorld();
    if (!W) return;

    // 상대 진영: 행동 유닛이 아군이면 적 전체, 적이면 아군 전체(그 중심을 향해 봄)
    const bool bActorIsAlly = PlayerParty.Contains(Actor);
    const TArray<AABaseCharacter*>& Foes = bActorIsAlly ? Enemies : PlayerParty;

    FVector FoeCen = FVector::ZeroVector; int32 FoeN = 0;
    for (AABaseCharacter* C : Foes)
        if (C && C->GetStatComponent() && !C->GetStatComponent()->IsDead()) { FoeCen += C->GetActorLocation(); ++FoeN; }
    if (FoeN == 0)   // 전멸 직전 폴백: 사망 포함 평균
        for (AABaseCharacter* C : Foes) if (C) { FoeCen += C->GetActorLocation(); ++FoeN; }
    FoeCen = (FoeN > 0) ? (FoeCen / FoeN) : (Actor->GetActorLocation() + Actor->GetActorForwardVector() * 300.f);

    const FVector UnitLoc = Actor->GetActorLocation();
    FVector ToFoe = FoeCen - UnitLoc; ToFoe.Z = 0.f;
    if (ToFoe.IsNearlyZero()) ToFoe = Actor->GetActorForwardVector();
    ToFoe.Normalize();
    const FVector Right = FVector::CrossProduct(FVector::UpVector, ToFoe).GetSafeNormal();

    // 유닛 뒤·위·어깨측면 → 유닛 뒷모습이 전경, 상대 진영이 프레임 안에. 시선은 유닛~상대중심 사이.
    CamTargetLoc = UnitLoc - ToFoe * TurnCamBack + FVector(0.f, 0.f, TurnCamHeight) + Right * TurnCamSide;
    const FVector LookAt = FMath::Lerp(UnitLoc, FoeCen, TurnCamLookBias) + FVector(0.f, 0.f, TurnCamLookUp);
    CamTargetRot = (LookAt - CamTargetLoc).Rotation();
    bCamFollow = true;

    // 카메라 없으면 생성 + 뷰타겟 전환(블렌드). 있으면 매 턴 컷은 Tick 보간이 부드럽게 처리.
    if (!BattleCamera)
    {
        if (APlayerController* PCtrl = W->GetFirstPlayerController())
        {
            if (!SavedViewTarget.IsValid()) SavedViewTarget = PCtrl->GetViewTarget();
            FActorSpawnParameters Sp; Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            BattleCamera = W->SpawnActor<ACameraActor>(CamTargetLoc, CamTargetRot, Sp);
            if (BattleCamera) PCtrl->SetViewTargetWithBlend(BattleCamera, CamBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
        }
    }
}

void ABattleManager::UpdateTurnMarker(AABaseCharacter* Actor)
{
    // 이전 마커 제거(누가 차례인지 1개만 유지)
    if (TurnMarkerComp) { TurnMarkerComp->DestroyComponent(); TurnMarkerComp = nullptr; }
    if (!Actor || !bBattleActive) return;

    UObject* Asset = UAssetResolver::ResolveVFX(TEXT("turnmark"));
    if (!Asset) return;
    USceneComponent* Root = Actor->GetRootComponent();
    if (!Root) return;

    // 발밑(캡슐 바닥) 상대 오프셋
    float FeetZ = 6.f;
    if (const ACharacter* Ch = Cast<ACharacter>(Actor))
        if (const UCapsuleComponent* Cap = Ch->GetCapsuleComponent())
            FeetZ = -Cap->GetScaledCapsuleHalfHeight() + 6.f;
    const FVector Off(0.f, 0.f, FeetZ);

    USceneComponent* Spawned = nullptr;
    if (UNiagaraSystem* NS = Cast<UNiagaraSystem>(Asset))
        Spawned = UNiagaraFunctionLibrary::SpawnSystemAttached(NS, Root, NAME_None, Off, FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset, false /*bAutoDestroy*/, true);
    else if (UParticleSystem* PS = Cast<UParticleSystem>(Asset))
        Spawned = UGameplayStatics::SpawnEmitterAttached(PS, Root, NAME_None, Off, FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset, false /*bAutoDestroy*/);

    if (Spawned)
    {
        Spawned->SetRelativeScale3D(FVector(TurnMarkerScale));
        TurnMarkerComp = Spawned;
    }
}

// ── 선제/기습 (Initiative) ───────────────────────────────

void ABattleManager::BuildTurnOrder(int32 PreferSide)
{
    const int32 Total = PlayerParty.Num() + Enemies.Num();
    TurnOrder.Reset();
    for (int32 i = 0; i < Total; ++i) TurnOrder.Add(i);

    // 효과 AGI(민첩) = 속도. AGI 미설정(0, 예: 5스탯 안 박힌 적)은 중립 기준값 10으로 폴백
    //   (캐릭터 AGI 범위 7~16의 중앙) → 적이 무조건 꼴찌 되는 것 방지, AGI 박힌 캐릭만 빠르고/느림.
    //   전원 미설정이면 모두 10 동속 → 안정정렬이 기존 [아군→적] 순서 유지(완전 하위호환).
    auto SpeedOf = [this](int32 Idx) -> float
    {
        AABaseCharacter* C = (Idx < PlayerParty.Num()) ? PlayerParty[Idx] : Enemies[Idx - PlayerParty.Num()];
        if (!C || !C->GetStatComponent()) return 0.f;
        const float Agi = C->GetStatComponent()->GetAGI();
        return Agi > 0.f ? Agi : 10.f;
    };

    // 속도 내림차순(안정정렬: 동속이면 원래 순서 유지)
    TurnOrder.StableSort([&SpeedOf](const int32& A, const int32& B) { return SpeedOf(A) > SpeedOf(B); });

    // 선제/기습: 라운드1 한정 해당 진영을 선두로(진영 내부 속도순은 보존 = 안정 분할)
    if (PreferSide == 1 || PreferSide == 2)
    {
        const bool bPlayerFront = (PreferSide == 1);
        const int32 NumParty = PlayerParty.Num();
        TurnOrder.StableSort([NumParty, bPlayerFront](const int32& A, const int32& B)
        {
            const bool aPlayer = (A < NumParty);
            const bool bPlayer = (B < NumParty);
            if (aPlayer != bPlayer) return bPlayerFront ? aPlayer : bPlayer; // 선두 진영 우선
            return false; // 같은 진영: 기존(속도) 순서 유지
        });
    }

    TurnPos = 0;
    CurrentTurnIndex = (TurnOrder.Num() > 0) ? TurnOrder[0] : 0;
}

void ABattleManager::ApplyInitiative(EBattleInitiative Initiative)
{
    int32 PreferSide = 0; // 0=속도순수
    if (Initiative == EBattleInitiative::Player)
    {
        PreferSide = 1; // 아군 선두
        // Turns=2: 자기 턴 시작의 TickStatMods가 먼저 1 감소시키므로, 첫 행동 때 버프 유지하려면 2 필요
        for (AABaseCharacter* P : PlayerParty)
            if (P && P->GetStatComponent() && !P->GetStatComponent()->IsDead())
                P->GetStatComponent()->ApplyAttackMod(AmbushAttackMult, 2);

        ActionFeedback = TEXT("★ 선제공격! 기습 성공 — 아군 공격력 상승 ★");
        Flair(EBattleFlair::Ambush, TEXT("AMBUSH!"));
    }
    else if (Initiative == EBattleInitiative::Enemy)
    {
        PreferSide = 2; // 적 선두
        for (AABaseCharacter* E : Enemies)
            if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead())
                E->GetStatComponent()->ApplyAttackMod(AmbushAttackMult, 2);

        ActionFeedback = TEXT("★ 기습당했다! — 적 공격력 상승 ★");
        Flair(EBattleFlair::Ambushed, TEXT("AMBUSHED!"));
    }

    // 턴 순서 구성(속도순 + 선제 진영 선두). Normal도 여기서 속도순으로 빌드.
    BuildTurnOrder(PreferSide);
}

// ── HUD 관리 ─────────────────────────────────────────────

void ABattleManager::CreateHUD()
{
    if (!BattleHUDClass || !GetWorld()) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    BattleHUDInstance = CreateWidget<UUserWidget>(PC, BattleHUDClass);
    if (BattleHUDInstance)
    {
        // BattleManagerRef 주입 — WBP_BattleHUD 버튼이 이 변수로 PlayerAttack 등을 호출.
        // 이름으로 찾아 주입하므로 reparent 여부와 무관(C++/BP 변수 둘 다 커버).
        if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(
            BattleHUDInstance->GetClass(), FName("BattleManagerRef")))
        {
            Prop->SetObjectPropertyValue_InContainer(BattleHUDInstance, this);
        }

        // 부모가 UBattleHUDWidget이면(reparent 완료) 턴 변경 이벤트 구독까지 연결
        if (UBattleHUDWidget* HUD = Cast<UBattleHUDWidget>(BattleHUDInstance))
            HUD->SetBattleManager(this);

        BattleHUDInstance->AddToViewport();

        FInputModeGameAndUI InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
    }
}

void ABattleManager::RemoveHUD()
{
    if (BattleHUDInstance)
    {
        BattleHUDInstance->RemoveFromParent();
        BattleHUDInstance = nullptr;
    }

    RestoreFieldCamera();     // 전투 종료 → 시네마틱 카메라 해제, 플레이어 카메라 복원

    PlayBGM(TEXT("field"));   // 전투 종료 → 필드 BGM 복귀 (승리/패배/도망 공통 정리 지점)

    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (PC)
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;
    }
}

// ── 턴 진행 ──────────────────────────────────────────────

void ABattleManager::NextTurn()
{
    if (!bBattleActive) return;

    int32 Total = PlayerParty.Num() + Enemies.Num();
    if (Total == 0) return;
    if (TurnOrder.Num() != Total) BuildTurnOrder(); // 안전: 인원 변동 시 재구성

    // 턴 순서(속도순)를 따라 살아있는 전투원 탐색
    for (int32 Tries = 0; Tries < Total; ++Tries)
    {
        if (TurnPos >= TurnOrder.Num()) { BuildTurnOrder(); } // 라운드 끝 → 새 라운드 속도순 재정렬
        const int32 Idx = TurnOrder[TurnPos];
        CurrentTurnIndex = Idx; // 미리보기/피드백 동기화

        AABaseCharacter* Candidate = (Idx < PlayerParty.Num())
            ? PlayerParty[Idx]
            : Enemies[Idx - PlayerParty.Num()];

        bool bCandidateIsPlayer = (Idx < PlayerParty.Num());

        if (Candidate && Candidate->GetStatComponent() && !Candidate->GetStatComponent()->IsDead())
        {
            UStatComponent* CS = Candidate->GetStatComponent();
            CS->TickStatMods();                          // 버프/디버프 지속 턴 감소
            const EAilment DotAil = CS->GetAilment();    // 틱 전 상태(도트 표시용)
            const float DotDmg = CS->OnTurnStartAilment(); // 독/화상 도트뎀 + 상태이상 지속 감소

            // 독/화상 도트로 깎였으면 피드백 + 연출(조용히 깎이지 않게)
            if (DotDmg > 0.f)
            {
                const int32 Num = bCandidateIsPlayer ? (Idx + 1) : (Idx - PlayerParty.Num() + 1);
                ActionFeedback = FString::Printf(TEXT("%s%d %s 데미지 %.0f"),
                    bCandidateIsPlayer ? TEXT("아군") : TEXT("적"), Num, *LexAilment(DotAil), DotDmg);
                Flair(EBattleFlair::Ailment, FString::Printf(TEXT("%s! %.0f"), *LexAilment(DotAil), DotDmg));
            }

            // 독 등으로 턴 시작에 사망 → 이번 턴 건너뜀
            if (CS->IsDead())
            {
                CheckBattleEnd();
                if (!bBattleActive) return;
                ++TurnPos;
                continue;
            }

            CurrentActor = Candidate;
            CS->SetDefending(false);   // 자기 턴 오면 방어 해제
            bIsPlayerTurn = bCandidateIsPlayer;
            if (!bIsPlayerTurn)
            {
                // 적 턴이 오면 다운 상태 회복 (총공격 기회 사라짐)
                DownedEnemies.Empty();
                bAllOutReady = false;
            }

            // 카메라는 아군 턴에만 그 아군 뒤로 전환(적 턴엔 직전 구도 유지 → 매 턴 휙휙 도는 어지러움 방지).
            // 아군 턴 시작 = 직전 행동이 끝난 시점 → "행동 끝나고 전환"되는 흐름.
            if (bIsPlayerTurn) FrameCameraOnActor(CurrentActor);
            UpdateTurnMarker(CurrentActor);   // 누구 차례인지 발밑 마커(아군·적 모두)

            // 수면/마비 → 이번 턴 행동 불가
            const bool bIncap = CS->IsIncapacitated();
            OnTurnStarted.Broadcast(CurrentActor, bIsPlayerTurn && !bIncap);

            if (bIncap)
            {
                const FString IncName = LexAilment(CS->GetAilment());
                ActionFeedback = FString::Printf(TEXT("%s %s! 행동 불가"),
                    bIsPlayerTurn ? TEXT("아군") : TEXT("적"),
                    IncName.IsEmpty() ? TEXT("") : *IncName);
                Flair(EBattleFlair::Ailment, IncName.IsEmpty() ? TEXT("행동 불가") : *IncName);
                if (GetWorld())
                    GetWorld()->GetTimerManager().SetTimer(
                        EnemyTurnTimer, this, &ABattleManager::EndTurn, 1.2f, false);
                return;
            }

            if (!bIsPlayerTurn && GetWorld())
            {
                GetWorld()->GetTimerManager().SetTimer(
                    EnemyTurnTimer,
                    this, &ABattleManager::ExecuteEnemyTurn,
                    1.5f, false);
            }
            return;
        }

        ++TurnPos; // 죽은/빈 전투원 → 턴순서 다음으로
    }

    // 살아있는 전투원 없음 — 전투 종료 확인
    CheckBattleEnd();
}

void ABattleManager::EndTurn()
{
    if (!bBattleActive) return;

    CheckBattleEnd();
    if (!bBattleActive) return;

    BatonStack = 0;   // 턴이 실제로 끝남 = One More/바톤 체인 종료 → 누적 리셋
    ++TurnPos;        // 턴순서 다음 전투원(라운드 끝이면 NextTurn이 재정렬)
    NextTurn();
}

// ── 전투 종료 판정 ────────────────────────────────────────

void ABattleManager::CheckBattleEnd()
{
    // 모든 적 사망 → 승리
    bool bAllEnemiesDead = true;
    for (AABaseCharacter* Enemy : Enemies)
    {
        if (Enemy && Enemy->GetStatComponent() && !Enemy->GetStatComponent()->IsDead())
        {
            bAllEnemiesDead = false;
            break;
        }
    }
    if (bAllEnemiesDead)
    {
        bBattleActive = false;

        // 적 도감: 쓰러뜨린 적 기록
        if (UBestiarySubsystem* Bst = GetBestiary(GetWorld()))
            for (AABaseCharacter* E : Enemies)
                if (E && E->GetStatComponent() && E->GetStatComponent()->IsDead())
                    Bst->RecordDefeat(EnemyIdOf(E));

        // 경험치 보상: 쓰러뜨린 적들의 XPReward 합산
        int32 TotalXP = 0;
        for (AABaseCharacter* E : Enemies)
            if (E && E->GetStatComponent())
                TotalXP += E->GetStatComponent()->GetXPReward();

        // 생존 아군에게 각자 지급 + 레벨업 표시
        FString LevelUps;
        for (int32 i = 0; i < PlayerParty.Num(); ++i)
        {
            AABaseCharacter* P = PlayerParty[i];
            if (P && P->GetStatComponent() && !P->GetStatComponent()->IsDead())
            {
                int32 Gained = P->GetStatComponent()->GainXP(TotalXP);
                if (Gained > 0)
                {
                    const int32 NewLv = P->GetStatComponent()->GetLevel();
                    LevelUps += FString::Printf(TEXT("  아군%d→Lv%d!"), i + 1, NewLv);

                    // 레벨업으로 새 스킬 습득
                    for (const FString& SkillName : P->LearnSkillsUpToLevel(NewLv))
                        LevelUps += FString::Printf(TEXT(" [%s 습득!]"), *SkillName);
                }
            }
        }

        // 골드 보상: 적 GoldReward 합산 → 플레이어(아군1)에게 누적
        int32 TotalGold = 0;
        for (AABaseCharacter* E : Enemies)
            if (E && E->GetStatComponent())
                TotalGold += E->GetStatComponent()->GetGoldReward();
        int32 CurGold = 0;
        if (PlayerParty.Num() > 0 && PlayerParty[0] && PlayerParty[0]->GetStatComponent())
        {
            PlayerParty[0]->GetStatComponent()->AddGold(TotalGold);
            CurGold = PlayerParty[0]->GetStatComponent()->GetGold();
        }

        // 아이템 드롭: 쓰러뜨린 적별 DropChance 굴려 플레이어 인벤토리에 적립 (B InventoryComponent 호출만)
        FString DropMsg;
        if (UInventoryComponent* Inv = GetPlayerInventory())
        {
            // 같은 아이템 합산해 "이름 xN" 한 번에 표기
            TMap<FName, int32> Gained;
            for (AABaseCharacter* E : Enemies)
            {
                if (!E || !E->GetStatComponent() || !E->GetStatComponent()->IsDead()) continue;
                if (E->DropItemId.IsNone() || E->DropChance <= 0.f) continue;
                if (FMath::FRand() < E->DropChance)
                {
                    const int32 Cnt = FMath::Max(1, E->DropCount);
                    Inv->AddItem(E->DropItemId, Cnt);
                    Gained.FindOrAdd(E->DropItemId) += Cnt;
                }
            }
            for (const TPair<FName, int32>& G : Gained)
            {
                FConsumableDef Def;
                const FString Nm = UInventoryComponent::FindDef(G.Key, Def) ? Def.Name : G.Key.ToString();
                DropMsg += FString::Printf(TEXT("  %s x%d"), *Nm, G.Value);
            }
            if (!DropMsg.IsEmpty()) DropMsg = TEXT("  획득:") + DropMsg;
        }

        ResultMessage = FString::Printf(TEXT("승리!  +%d EXP  +%d골드 (총 %d)%s%s"), TotalXP, TotalGold, CurGold, *LevelUps, *DropMsg);

        // 진행 저장 — 기존 세이브 로드 후 갱신(아군 누적 보존), 없으면 새로 생성
        {
            USecretSaveGame* Save = nullptr;
            if (UGameplayStatics::DoesSaveGameExist(TEXT("PlayerSave"), 0))
                Save = Cast<USecretSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0));
            if (!Save)
                Save = Cast<USecretSaveGame>(UGameplayStatics::CreateSaveGameObject(USecretSaveGame::StaticClass()));

            if (Save)
            {
                Save->bHasData = true;

                // 플레이어(아군1)
                if (PlayerParty.Num() > 0 && PlayerParty[0] && PlayerParty[0]->GetStatComponent())
                {
                    PlayerParty[0]->GetStatComponent()->GetProgression(
                        Save->PlayerLevel, Save->PlayerXP, Save->PlayerMaxHP,
                        Save->PlayerAttack, Save->PlayerDefense, Save->PlayerMaxSP);
                    Save->PlayerGold = PlayerParty[0]->GetStatComponent()->GetGold();
                }

                // 아군 NPC들 (NPCName 기준 갱신/추가)
                for (int32 pi = 1; pi < PlayerParty.Num(); ++pi)
                {
                    AANPCCharacter* Ally = Cast<AANPCCharacter>(PlayerParty[pi]);
                    if (!Ally || !Ally->GetStatComponent()) continue;

                    FAllyProgress AP;
                    AP.Name = Ally->NPCName;
                    Ally->GetStatComponent()->GetProgression(
                        AP.Level, AP.XP, AP.MaxHP, AP.Attack, AP.Defense, AP.MaxSP);

                    bool bFound = false;
                    for (FAllyProgress& E : Save->Allies)
                        if (E.Name == AP.Name) { E = AP; bFound = true; break; }
                    if (!bFound) Save->Allies.Add(AP);
                }

                UGameplayStatics::SaveGameToSlot(Save, TEXT("PlayerSave"), 0);
            }
        }

        // 처치 퀘스트 연동(B): 플레이어 QuestComponent에 쓰러뜨린 적 수 알림
        if (UWorld* W = GetWorld())
            if (APlayerController* PC = W->GetFirstPlayerController())
                if (APawn* Pawn = PC->GetPawn())
                    if (UQuestComponent* Q = Pawn->FindComponentByClass<UQuestComponent>())
                    {
                        int32 KillCount = 0;
                        for (AABaseCharacter* E : Enemies)
                            if (E && E->GetStatComponent() && E->GetStatComponent()->IsDead())
                                ++KillCount;
                        if (KillCount > 0) Q->NotifyEnemyDefeated(KillCount);
                    }

        // 보스 처치 → 메인 스토리 플래그(ArchetypeId 기준). 다음 챕터 해금 = 전투↔스토리 연결.
        if (UWorld* W = GetWorld())
            if (UGameInstance* GI = W->GetGameInstance())
                if (UStoryManagerSubsystem* Story = GI->GetSubsystem<UStoryManagerSubsystem>())
                    for (AABaseCharacter* E : Enemies)
                        if (E && E->bIsBoss && E->GetStatComponent() && E->GetStatComponent()->IsDead())
                            if (AANPCCharacter* N = Cast<AANPCCharacter>(E))
                                if (!N->ArchetypeId.IsNone())
                                    Story->SetFlag(N->ArchetypeId);

        Flair(EBattleFlair::Victory, TEXT("VICTORY"));
        OnBattleVictory.Broadcast();
        if (GetWorld())
            GetWorld()->GetTimerManager().SetTimer(
                EndBattleTimer, this, &ABattleManager::RemoveHUD, 3.0f, false);
        return;
    }

    // 모든 아군 사망 → 패배
    bool bAllPlayersDead = true;
    for (AABaseCharacter* Player : PlayerParty)
    {
        if (Player && Player->GetStatComponent() && !Player->GetStatComponent()->IsDead())
        {
            bAllPlayersDead = false;
            break;
        }
    }
    if (bAllPlayersDead)
    {
        bBattleActive = false;
        ResultMessage = TEXT("패배...");

        // 패배 복구(임시): 파티 HP/SP 복구해서 월드에서 죽은 채 멈추지 않게
        for (AABaseCharacter* P : PlayerParty)
            if (P && P->GetStatComponent())
                P->GetStatComponent()->ResetHP();

        Flair(EBattleFlair::Defeat, TEXT("DEFEAT"));
        OnBattleDefeat.Broadcast();
        if (GetWorld())
            GetWorld()->GetTimerManager().SetTimer(
                EndBattleTimer, this, &ABattleManager::RemoveHUD, 2.0f, false);
    }
}

// ── 플레이어 액션 ─────────────────────────────────────────

void ABattleManager::PlayerAttack()
{
    if (!bBattleActive || !bIsPlayerTurn || !CurrentActor) return;

    AABaseCharacter* Target = GetSelectedEnemy();
    if (!Target) return;

    const float ChargeFactor = ConsumeCharge(CurrentActor);
    PerformAttack(CurrentActor, Target, ChargeFactor, EBattleElement::Physical);
    if (ChargeFactor > 1.f) ActionFeedback = TEXT("차지! ") + ActionFeedback;
    if (bLastHitWeakness) MarkDownedAndCheckAllOut(Target);
    EndPlayerActionOrOneMore();
}

void ABattleManager::PlayerUseSkill(int32 SkillIndex)
{
    if (!bBattleActive || !bIsPlayerTurn || !CurrentActor) return;
    if (!CurrentActor->Skills.IsValidIndex(SkillIndex)) return;

    const FSkillDef Skill = CurrentActor->Skills[SkillIndex];

    UStatComponent* SelfStat = CurrentActor->GetStatComponent();
    if (!SelfStat || !SelfStat->SpendSP(static_cast<float>(Skill.SPCost)))
    {
        ActionFeedback = TEXT("SP 부족!");
        return; // SP 부족 → 턴 소모 없음
    }

    // 시전음: 범용 시전 효과음(SFX_cast) — 데미지 스킬은 PerformAttack이 속성 임팩트음을 별도 재생(중복 방지).
    // 회복/버프 등 비-공격 스킬도 시전음으로 청각 피드백 확보.
    PlaySFX(TEXT("cast"), 0.6f);
    // 시전 연출: 시전자 발밑 마법진(NS_cast)만. 타격 VFX/속성음은 PerformAttack이 대상에 스폰(화면 과밀 방지).
    SpawnVFX(TEXT("cast"), CurrentActor, VFXScaleCast, 6.f);

    switch (Skill.SkillType)
    {
    case ESkillType::DamageOne:
    {
        AABaseCharacter* Target = GetSelectedEnemy();
        if (Target && TryInstantKill(Target, Skill))
        {
            // 즉사 발동 — 연타/상태이상/추가타 없이 종료(차지는 소모하지 않음)
            bLastHitWeakness = false;
        }
        else if (Target)
        {
            const int32 Hits = FMath::Max(1, Skill.HitCount);
            const float ChargeFactor = ConsumeCharge(CurrentActor);
            bool bAnyWeak = false;
            for (int32 h = 0; h < Hits; ++h)
            {
                if (!Target->GetStatComponent() || Target->GetStatComponent()->IsDead()) break;
                PerformAttack(CurrentActor, Target, Skill.PowerMultiplier * ChargeFactor, Skill.Element, ResolveSkillMontage(Skill));
                if (bLastHitWeakness) bAnyWeak = true;
            }
            if (Hits > 1)
                ActionFeedback = FString::Printf(TEXT("%d연타! "), Hits) + ActionFeedback;
            if (ChargeFactor > 1.f) ActionFeedback = TEXT("차지! ") + ActionFeedback;
            bLastHitWeakness = bAnyWeak;
            if (bAnyWeak) MarkDownedAndCheckAllOut(Target);
            TryInflictAilment(Target, Skill);
            // 대상 타격 VFX는 PerformAttack이 속성별로 스폰(중복 방지)
        }
        break;
    }
    case ESkillType::DamageAll:
    {
        const int32 Hits = FMath::Max(1, Skill.HitCount);
        const float ChargeFactor = ConsumeCharge(CurrentActor);
        bool bAnyWeak = false;
        for (int32 h = 0; h < Hits; ++h)
            for (AABaseCharacter* E : Enemies)
            {
                if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead())
                {
                    if (h == 0 && TryInstantKill(E, Skill)) continue; // 즉사한 적은 일반타 스킵
                    PerformAttack(CurrentActor, E, Skill.PowerMultiplier * ChargeFactor, Skill.Element, ResolveSkillMontage(Skill));
                    if (bLastHitWeakness) { MarkDownedAndCheckAllOut(E); bAnyWeak = true; }
                    TryInflictAilment(E, Skill);
                    // 대상 타격 VFX는 PerformAttack이 속성별로 스폰(중복 방지)
                }
            }
        if (ChargeFactor > 1.f) ActionFeedback = TEXT("차지! ") + ActionFeedback;
        bLastHitWeakness = bAnyWeak; // 전체기는 하나라도 약점이면 One More
        break;
    }
    case ESkillType::HealSelf:
    {
        const float HealAmount = Skill.PowerMultiplier * 20.f;
        SelfStat->Heal(HealAmount);
        SpawnVFX(TEXT("heal"), CurrentActor, VFXScaleSupport, VFXSpawnHeight);
        ShowFloatingNumber(CurrentActor, FString::Printf(TEXT("+%.0f"), HealAmount), FLinearColor(0.36f, 0.95f, 0.45f, 1.f));
        ActionFeedback = FString::Printf(TEXT("회복 +%.0f"), HealAmount);
        bLastHitWeakness = false;
        break;
    }
    case ESkillType::HealOne:
    {
        // 선택한 아군 회복(미선택/사망 시 최저HP 아군으로 폴백)
        AABaseCharacter* Ally = GetSelectedAlly();
        const float HealAmount = Skill.PowerMultiplier * 20.f;
        if (Ally && Ally->GetStatComponent())
        {
            Ally->GetStatComponent()->Heal(HealAmount);
            SpawnVFX(TEXT("heal"), Ally, VFXScaleSupport, VFXSpawnHeight);
            ShowFloatingNumber(Ally, FString::Printf(TEXT("+%.0f"), HealAmount), FLinearColor(0.36f, 0.95f, 0.45f, 1.f));
        }
        ActionFeedback = FString::Printf(TEXT("회복 +%.0f"), HealAmount);
        bLastHitWeakness = false;
        break;
    }
    case ESkillType::HealAll:
    {
        const float HealAmount = Skill.PowerMultiplier * 20.f;
        for (AABaseCharacter* A : PlayerParty)
            if (A && A->GetStatComponent() && !A->GetStatComponent()->IsDead())
            {
                A->GetStatComponent()->Heal(HealAmount);
                SpawnVFX(TEXT("heal"), A, VFXScaleSupport, VFXSpawnHeight);
                ShowFloatingNumber(A, FString::Printf(TEXT("+%.0f"), HealAmount), FLinearColor(0.36f, 0.95f, 0.45f, 1.f));
            }
        ActionFeedback = FString::Printf(TEXT("아군 전체 회복 +%.0f"), HealAmount);
        bLastHitWeakness = false;
        break;
    }
    case ESkillType::BuffAttack:
    {
        for (AABaseCharacter* A : PlayerParty)
            if (A && A->GetStatComponent() && !A->GetStatComponent()->IsDead())
            {
                A->GetStatComponent()->ApplyAttackMod(Skill.PowerMultiplier, 3);
                SpawnVFX(TEXT("buff"), A, VFXScaleSupport, VFXSpawnHeight);
            }
        ActionFeedback = TEXT("아군 공격력 상승!");
        bLastHitWeakness = false;
        break;
    }
    case ESkillType::BuffDefense:
    {
        for (AABaseCharacter* A : PlayerParty)
            if (A && A->GetStatComponent() && !A->GetStatComponent()->IsDead())
            {
                A->GetStatComponent()->ApplyDefenseMod(Skill.PowerMultiplier, 3);
                SpawnVFX(TEXT("buff"), A, VFXScaleSupport, VFXSpawnHeight);
            }
        ActionFeedback = TEXT("아군 방어력 상승!");
        bLastHitWeakness = false;
        break;
    }
    case ESkillType::DebuffAttack:
    {
        if (AABaseCharacter* T = GetSelectedEnemy())
            if (T->GetStatComponent())
            {
                T->GetStatComponent()->ApplyAttackMod(Skill.PowerMultiplier, 3);
                SpawnVFX(TEXT("debuff"), T, VFXScaleSupport, VFXSpawnHeight);
            }
        ActionFeedback = TEXT("적 공격력 하락!");
        bLastHitWeakness = false;
        break;
    }
    case ESkillType::DebuffDefense:
    {
        if (AABaseCharacter* T = GetSelectedEnemy())
            if (T->GetStatComponent())
            {
                T->GetStatComponent()->ApplyDefenseMod(Skill.PowerMultiplier, 3);
                SpawnVFX(TEXT("debuff"), T, VFXScaleSupport, VFXSpawnHeight);
            }
        ActionFeedback = TEXT("적 방어력 하락!");
        bLastHitWeakness = false;
        break;
    }
    case ESkillType::Analyze:
    {
        // 생존 적 전체의 약점을 즉시 공개(KnownWeaknesses 등록 → 라벨/상태창에 표시)
        int32 Revealed = 0;
        for (AABaseCharacter* E : Enemies)
            if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead())
                for (EBattleElement W : E->WeakElements)
                {
                    KnownWeaknesses.FindOrAdd(E).Add(W);
                    if (UBestiarySubsystem* Bst = GetBestiary(GetWorld())) Bst->RecordWeakness(EnemyIdOf(E), W);
                    ++Revealed;
                }
        ActionFeedback = Revealed > 0 ? TEXT("분석 완료! 적 약점이 드러났다") : TEXT("분석했지만 약점이 없다");
        bLastHitWeakness = false;
        break;
    }
    case ESkillType::Cure:
    {
        // 선택한 아군의 상태이상 치료(미선택/사망 시 최저HP 아군으로 폴백)
        AABaseCharacter* Ally = GetSelectedAlly();
        if (Ally && Ally->GetStatComponent() && Ally->GetStatComponent()->GetAilment() != EAilment::None)
        {
            Ally->GetStatComponent()->CureAilment();
            SpawnVFX(TEXT("heal"), Ally, VFXScaleSupport, VFXSpawnHeight);
            ActionFeedback = TEXT("상태이상 회복!");
        }
        else ActionFeedback = TEXT("치료할 상태이상이 없다");
        bLastHitWeakness = false;
        break;
    }
    case ESkillType::Revive:
    {
        // 죽은 아군 부활(첫 사망 아군). 회복량 = 배율×20
        AABaseCharacter* Dead = nullptr;
        for (AABaseCharacter* A : PlayerParty)
            if (A && A->GetStatComponent() && A->GetStatComponent()->IsDead()) { Dead = A; break; }
        if (Dead)
        {
            Dead->GetStatComponent()->Revive(Skill.PowerMultiplier * 20.f);
            SpawnVFX(TEXT("heal"), Dead, VFXScaleSupport, VFXSpawnHeight);
            ShowFloatingNumber(Dead, TEXT("부활!"), FLinearColor(0.45f, 1.f, 0.65f, 1.f), true);
            ActionFeedback = TEXT("부활!");
        }
        else ActionFeedback = TEXT("부활 대상이 없다");
        bLastHitWeakness = false;
        break;
    }
    }

    EndPlayerActionOrOneMore();
}

void ABattleManager::PlayerDefend()
{
    if (!bBattleActive || !bIsPlayerTurn || !CurrentActor) return;
    if (UStatComponent* CS = CurrentActor->GetStatComponent())
    {
        CS->SetDefending(true);
        CS->RestoreSP(5.f); // 가드 시 SP 소량 회복(페르소나식)
    }
    EndTurn();
}

void ABattleManager::PlayerCharge()
{
    if (!bBattleActive || !bIsPlayerTurn || !CurrentActor) return;
    UStatComponent* CS = CurrentActor->GetStatComponent();
    if (!CS) return;
    CS->SetCharged(true);
    ActionFeedback = TEXT("차지! 다음 공격이 강해진다");
    EndTurn(); // 방어처럼 턴 소모
}

void ABattleManager::PlayerBatonPass()
{
    if (!bBattleActive || !bIsPlayerTurn || !CurrentActor) return;
    if (!bLastHitWeakness) return; // One More(약점/치명타/테크니컬) 상태에서만 가능

    // 선택된 아군(자기 자신/사망이면 무효)
    AABaseCharacter* Ally = nullptr;
    if (PlayerParty.IsValidIndex(CurrentAllyTarget))
    {
        AABaseCharacter* A = PlayerParty[CurrentAllyTarget];
        if (A && A != CurrentActor && A->GetStatComponent() && !A->GetStatComponent()->IsDead())
            Ally = A;
    }
    if (!Ally)
    {
        ActionFeedback = TEXT("바톤 받을 아군을 먼저 선택하세요");
        return; // One More 유지(턴 안 넘김)
    }

    // ★ 페르소나5식 누적 강화: 체인에서 패스를 거듭할수록 버프↑ (1패스 ×1.5 / 2패스 ×2.0 / 3패스+ ×2.5)
    ++BatonStack;
    const float BatonMult = FMath::Min(1.5f + 0.5f * (BatonStack - 1), 2.5f);

    // 바톤 버프 + 행동권을 아군에게 (턴 인덱스는 그대로 → 인덱스 손상/이중턴 없음)
    UStatComponent* AS = Ally->GetStatComponent();
    AS->ApplyAttackMod(BatonMult, 2);
    AS->RestoreSP(5.f * BatonStack); // 바톤 받으면 소폭 SP 회복(연계 지속력 — 패스 길수록↑)
    CurrentActor = Ally;
    bLastHitWeakness = false; // One More 소비(받은 아군이 다시 약점 맞히면 또 One More→바톤 가능)
    ActionFeedback = FString::Printf(TEXT("★ 바톤 터치 %d연쇄! 공격력 ×%.1f ★"), BatonStack, BatonMult);
    Flair(EBattleFlair::BatonPass, TEXT("BATON PASS!"));
    FrameCameraOnActor(CurrentActor);            // 바톤 받은 아군 뒤로 카메라 전환
    UpdateTurnMarker(CurrentActor);              // 마커도 새 행동자로
    OnTurnStarted.Broadcast(CurrentActor, true); // HUD를 새 행동자로 갱신
}

bool ABattleManager::GetCanBatonPass() const
{
    if (!bBattleActive || !bIsPlayerTurn || !bLastHitWeakness) return false;
    for (AABaseCharacter* A : PlayerParty)
        if (A && A != CurrentActor && A->GetStatComponent() && !A->GetStatComponent()->IsDead())
            return true;
    return false;
}

void ABattleManager::PlayerFlee()
{
    if (!bBattleActive || !bIsPlayerTurn) return;

    // 도망 성공 확률(튜닝 가능) — 실패하면 턴만 소모
    if (FMath::FRand() < FleeChance)
    {
        bBattleActive = false;
        if (GetWorld())
            GetWorld()->GetTimerManager().ClearTimer(EnemyTurnTimer);
        RemoveHUD();
        OnBattleFled.Broadcast();
    }
    else
    {
        ActionFeedback = TEXT("도망 실패!");
        EndTurn();
    }
}

void ABattleManager::PlayerAllOutAttack()
{
    if (!bBattleActive || !bIsPlayerTurn || !bAllOutReady || !CurrentActor) return;

    CurrentActor->PlayRandomBasicAttackMontage(); // 돌진 모션

    // 모든 생존 적에게 상성 무시 대형 데미지 (공격력 3배)
    for (AABaseCharacter* E : Enemies)
    {
        if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead())
        {
            float Dmg = CalculateDamage(CurrentActor, E, AllOutMult);
            E->GetStatComponent()->ApplyDamage(Dmg);
            E->PlayRandomHitReactionMontage();
        }
    }

    ActionFeedback = TEXT("★★ 총공격! ★★");
    Flair(EBattleFlair::AllOut, TEXT("ALL-OUT ATTACK!"));
    DownedEnemies.Empty();
    bAllOutReady = false;
    EndTurn();
}

// ── 적 턴 자동 실행 ──────────────────────────────────────

void ABattleManager::ExecuteEnemyTurn()
{
    if (!bBattleActive || !CurrentActor) return;

    // 타겟(기본): 가장 HP 낮은 생존 플레이어 (집중 공격 AI). 데미지 행동은 약점 인지로 재선택
    AABaseCharacter* PlayerTarget = FindLowestHPAlly();
    if (!PlayerTarget)
    {
        EndTurn();
        return;
    }

    bLastHitWeakness = false;

    // 대상이 방어 중이면 데미지 0.5배 (캐릭터별)
    auto DefMultFor = [](AABaseCharacter* C) -> float
    {
        return (C && C->GetStatComponent() && C->GetStatComponent()->IsDefending()) ? 0.5f : 1.f;
    };

    // 적 차지 AI: HP 여유 있고 다음 턴 강공 가능하면 가끔 힘을 모음(이번 턴 스킵 → 다음 턴 ×2.5)
    {
        UStatComponent* MyStat = CurrentActor->GetStatComponent();
        bool bHasDamage = (CurrentActor->Skills.Num() == 0); // 스킬 없으면 물리=데미지 가능
        for (const FSkillDef& S : CurrentActor->Skills)
            if (S.SkillType == ESkillType::DamageOne || S.SkillType == ESkillType::DamageAll) { bHasDamage = true; break; }

        if (MyStat && !MyStat->IsCharged() && bHasDamage
            && MyStat->GetCurrentHP() >= MyStat->GetMaxHP() * 0.5f
            && FMath::FRand() < 0.22f)
        {
            MyStat->SetCharged(true);
            ActionFeedback = TEXT("적이 힘을 모은다...");
            EndTurn();
            return;
        }
    }

    // 보스 페이즈/패턴: 페이즈 전환 시 형태변화+광역 일격, 평상시 확률적 광역. 발동하면 이번 턴 소모.
    if (CurrentActor->bIsBoss && TryBossPattern(CurrentActor))
        return;

    if (CurrentActor->Skills.Num() == 0)
    {
        // 스킬 없으면 물리 기본 공격 (차지 보유 시 ×2.5 소모). 물리 약점 우선 타겟
        PlayerTarget = PickPlayerTargetFor(EBattleElement::Physical);
        const float EC = ConsumeCharge(CurrentActor);
        PerformAttack(CurrentActor, PlayerTarget, EC * DefMultFor(PlayerTarget), EBattleElement::Physical);
        if (EC > 1.f) ActionFeedback = TEXT("차지! ") + ActionFeedback;
    }
    else
    {
        // 똑똑한 AI 우선순위: ①자기 위급 회복 → ①-b 아군 위급 지원회복 → ②상태이상 치료 → ③유효 스킬 랜덤
        UStatComponent* SelfStat = CurrentActor->GetStatComponent();
        FSkillDef Skill;
        bool bChose = false;

        // ① 자신 위급(HP<35%)하면 회복 스킬 우선 (자기회복 포함)
        if (SelfStat && SelfStat->GetCurrentHP() < SelfStat->GetMaxHP() * 0.35f)
        {
            for (const FSkillDef& S : CurrentActor->Skills)
                if (S.SkillType == ESkillType::HealSelf || S.SkillType == ESkillType::HealAll || S.SkillType == ESkillType::HealOne)
                { Skill = S; bChose = true; break; }
        }

        // ①-b 아군(다른 적)이 위급(HP<50%)하면 광역/단일 회복 우선 — 후방 지원형 적("힐러부터 잡아라" 전술 유발).
        //      HealSelf는 아군을 못 살리므로 제외. 자기 위급(①)이 우선.
        if (!bChose)
        {
            bool bAllyHurt = false;
            for (AABaseCharacter* E : Enemies)
                if (E && E != CurrentActor && E->GetStatComponent() && !E->GetStatComponent()->IsDead()
                    && E->GetStatComponent()->GetCurrentHP() < E->GetStatComponent()->GetMaxHP() * 0.5f)
                { bAllyHurt = true; break; }
            if (bAllyHurt)
                for (const FSkillDef& S : CurrentActor->Skills)
                    if (S.SkillType == ESkillType::HealAll || S.SkillType == ESkillType::HealOne)
                    { Skill = S; bChose = true; break; }
        }

        // 적 진영에 상태이상 걸린 아군이 있는지 (Cure 케이스와 동일 기준 — 치료 대상 유무)
        bool bAnyEnemyAiling = false;
        for (AABaseCharacter* E : Enemies)
            if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead()
                && E->GetStatComponent()->GetAilment() != EAilment::None)
            { bAnyEnemyAiling = true; break; }

        // ② 상태이상(독/화상 도트·행동불가 등) 있고 치료 스킬 보유 시 치료 우선
        if (!bChose && bAnyEnemyAiling)
        {
            for (const FSkillDef& S : CurrentActor->Skills)
                if (S.SkillType == ESkillType::Cure) { Skill = S; bChose = true; break; }
        }

        // ③ 유효 스킬 랜덤 — 효과 없는 스킬(Analyze/대상없는 Cure·Revive)은 풀에서 제외(턴 낭비 방지)
        if (!bChose)
        {
            bool bAnyDeadAlly = false;
            for (AABaseCharacter* E : Enemies)
                if (E && E->GetStatComponent() && E->GetStatComponent()->IsDead()) { bAnyDeadAlly = true; break; }

            TArray<FSkillDef> Usable;
            for (const FSkillDef& S : CurrentActor->Skills)
            {
                if (S.SkillType == ESkillType::Analyze) continue;                  // 적이 쓰면 효과 없음
                if (S.SkillType == ESkillType::Cure   && !bAnyEnemyAiling) continue; // 치료 대상 없음
                if (S.SkillType == ESkillType::Revive && !bAnyDeadAlly)    continue; // 부활 대상 없음
                Usable.Add(S);
            }
            if (Usable.Num() > 0)
                Skill = Usable[FMath::RandRange(0, Usable.Num() - 1)];
            // Usable이 비면 Skill은 기본값(DamageOne)
        }

        // 단일 데미지기는 약점 인지로 타겟 재선택(없으면 최저HP 유지)
        if (Skill.SkillType == ESkillType::DamageOne)
            PlayerTarget = PickPlayerTargetFor(Skill.Element);

        // 적 기준: 데미지/디버프는 플레이어에게, 회복/버프는 적 진영에게
        switch (Skill.SkillType)
        {
        case ESkillType::DamageOne:
        {
            if (TryInstantKill(PlayerTarget, Skill))
            {
                bLastHitWeakness = false;
                break; // 즉사 — 추가 처리 없음
            }
            const int32 Hits = FMath::Max(1, Skill.HitCount);
            const float EC = ConsumeCharge(CurrentActor);
            bool bAnyWeak = false;
            for (int32 h = 0; h < Hits; ++h)
            {
                if (!PlayerTarget->GetStatComponent() || PlayerTarget->GetStatComponent()->IsDead()) break;
                PerformAttack(CurrentActor, PlayerTarget, Skill.PowerMultiplier * EC * DefMultFor(PlayerTarget), Skill.Element, ResolveSkillMontage(Skill));
                if (bLastHitWeakness) bAnyWeak = true;
            }
            if (Hits > 1) ActionFeedback = FString::Printf(TEXT("%d연타! "), Hits) + ActionFeedback;
            if (EC > 1.f)  ActionFeedback = TEXT("차지! ") + ActionFeedback;
            bLastHitWeakness = bAnyWeak;
            TryInflictAilment(PlayerTarget, Skill);
            break;
        }

        case ESkillType::DamageAll:
        {
            const float EC = ConsumeCharge(CurrentActor);
            for (AABaseCharacter* P : PlayerParty)
                if (P && P->GetStatComponent() && !P->GetStatComponent()->IsDead())
                {
                    if (TryInstantKill(P, Skill)) continue; // 즉사한 아군은 일반타 스킵
                    PerformAttack(CurrentActor, P, Skill.PowerMultiplier * EC * DefMultFor(P), Skill.Element, ResolveSkillMontage(Skill));
                    TryInflictAilment(P, Skill);
                }
            if (EC > 1.f) ActionFeedback = TEXT("차지! ") + ActionFeedback;
            break;
        }

        case ESkillType::HealSelf:
            if (SelfStat) SelfStat->Heal(Skill.PowerMultiplier * 20.f);
            SpawnVFX(TEXT("heal"), CurrentActor, VFXScaleSupport, VFXSpawnHeight);
            ShowFloatingNumber(CurrentActor, FString::Printf(TEXT("+%.0f"), Skill.PowerMultiplier * 20.f), FLinearColor(0.36f, 0.95f, 0.45f, 1.f));
            ActionFeedback = TEXT("적이 회복했다");
            break;

        case ESkillType::HealOne:
            // 가장 HP 낮은 적(아군) 회복
            if (AABaseCharacter* Hurt = FindLowestHPEnemy())
                if (Hurt->GetStatComponent())
                {
                    Hurt->GetStatComponent()->Heal(Skill.PowerMultiplier * 20.f);
                    SpawnVFX(TEXT("heal"), Hurt, VFXScaleSupport, VFXSpawnHeight);
                    ShowFloatingNumber(Hurt, FString::Printf(TEXT("+%.0f"), Skill.PowerMultiplier * 20.f), FLinearColor(0.36f, 0.95f, 0.45f, 1.f));
                }
            ActionFeedback = TEXT("적이 회복했다");
            break;

        case ESkillType::HealAll:
            for (AABaseCharacter* E : Enemies)
                if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead())
                {
                    E->GetStatComponent()->Heal(Skill.PowerMultiplier * 20.f);
                    SpawnVFX(TEXT("heal"), E, VFXScaleSupport, VFXSpawnHeight);
                    ShowFloatingNumber(E, FString::Printf(TEXT("+%.0f"), Skill.PowerMultiplier * 20.f), FLinearColor(0.36f, 0.95f, 0.45f, 1.f));
                }
            ActionFeedback = TEXT("적 전체가 회복했다");
            break;

        case ESkillType::BuffAttack:
            for (AABaseCharacter* E : Enemies)
                if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead())
                    E->GetStatComponent()->ApplyAttackMod(Skill.PowerMultiplier, 3);
            ActionFeedback = TEXT("적이 공격력을 높였다!");
            break;

        case ESkillType::BuffDefense:
            for (AABaseCharacter* E : Enemies)
                if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead())
                    E->GetStatComponent()->ApplyDefenseMod(Skill.PowerMultiplier, 3);
            ActionFeedback = TEXT("적이 방어력을 높였다!");
            break;

        case ESkillType::DebuffAttack:
            if (PlayerTarget->GetStatComponent())
                PlayerTarget->GetStatComponent()->ApplyAttackMod(Skill.PowerMultiplier, 3);
            ActionFeedback = TEXT("아군 공격력이 떨어졌다...");
            break;

        case ESkillType::DebuffDefense:
            if (PlayerTarget->GetStatComponent())
                PlayerTarget->GetStatComponent()->ApplyDefenseMod(Skill.PowerMultiplier, 3);
            ActionFeedback = TEXT("아군 방어력이 떨어졌다...");
            break;

        case ESkillType::Revive:
        {
            // 적이 죽은 아군(적 진영)을 부활
            for (AABaseCharacter* E : Enemies)
                if (E && E->GetStatComponent() && E->GetStatComponent()->IsDead())
                {
                    E->GetStatComponent()->Revive(Skill.PowerMultiplier * 20.f);
                    ActionFeedback = TEXT("적이 동료를 부활시켰다!");
                    break;
                }
            break;
        }

        case ESkillType::Cure:
        {
            // 상태이상 걸린 적(아군 진영) 치료
            AABaseCharacter* Sick = nullptr;
            for (AABaseCharacter* E : Enemies)
                if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead()
                    && E->GetStatComponent()->GetAilment() != EAilment::None)
                { Sick = E; break; }
            if (Sick) { Sick->GetStatComponent()->CureAilment(); ActionFeedback = TEXT("적이 상태이상을 회복했다"); }
            else ActionFeedback = TEXT("적이 치료했지만 효과가 없었다");
            break;
        }

        default: break;
        }
    }

    // 적도 약점을 맞히면 한 번 더 (데미지 스킬에서만 bLastHitWeakness가 true)
    if (bLastHitWeakness && FindFirstLivingPlayer() != nullptr && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            EnemyTurnTimer, this, &ABattleManager::ExecuteEnemyTurn, 1.0f, false);
        return;
    }

    EndTurn();
}

// ── HUD 상태 텍스트 ──────────────────────────────────────

// 텍스트 HP/SP 바 — [██████░░░░] 형태로 한눈에 잔량 표시(폰트 무관 블록문자).
static FString MakeBar(float Cur, float Max, int32 Seg = 12)
{
    const float R = (Max > 0.f) ? FMath::Clamp(Cur / Max, 0.f, 1.f) : 0.f;
    const int32 Filled = FMath::RoundToInt(R * Seg);
    FString Bar = TEXT("[");
    for (int32 i = 0; i < Seg; ++i) Bar += (i < Filled) ? TEXT("█") : TEXT("░");
    Bar += TEXT("]");
    return Bar;
}

FText ABattleManager::GetBattleStatusText() const
{
    FString Result;

    if (!ResultMessage.IsEmpty())
        Result += FString::Printf(TEXT("=== %s ===\n"), *ResultMessage);

    // One More / 바톤 연쇄 배너 — 플레이어가 "한 번 더" 가능 상태와 바톤 누적을 명확히 인지
    if (bBattleActive && bIsPlayerTurn && bLastHitWeakness)
    {
        Result += TEXT("★ 1 MORE! — 다시 행동");
        if (GetCanBatonPass())
            Result += FString::Printf(TEXT(" 또는 바톤 터치(현재 %d연쇄 → 다음 ×%.1f)"),
                BatonStack, FMath::Min(1.5f + 0.5f * BatonStack, 2.5f));
        Result += TEXT(" ★\n");
    }

    for (int32 i = 0; i < PlayerParty.Num(); ++i)
    {
        AABaseCharacter* C = PlayerParty[i];
        if (C && C->GetStatComponent())
        {
            UStatComponent* St = C->GetStatComponent();
            FString Mods;
            if (St->GetAttackMod()  > 1.f) Mods += TEXT(" 공↑");
            if (St->GetAttackMod()  < 1.f) Mods += TEXT(" 공↓");
            if (St->GetDefenseMod() > 1.f) Mods += TEXT(" 방↑");
            if (St->GetDefenseMod() < 1.f) Mods += TEXT(" 방↓");
            if (St->GetAilment() != EAilment::None)
                Mods += FString::Printf(TEXT(" [%s]"), *LexAilment(St->GetAilment()));
            if (St->IsCharged()) Mods += TEXT(" [차지]");
            const TCHAR* GuardTag = St->IsDefending() ? TEXT("  (방어중)") : TEXT("");
            const TCHAR* TurnMark = (C == CurrentActor) ? TEXT("▶ ") : TEXT("   ");
            Result += FString::Printf(TEXT("%s아군%d Lv%d  HP %s %.0f/%.0f   SP %.0f/%.0f%s%s\n"),
                TurnMark, i + 1, St->GetLevel(),
                *MakeBar(St->GetCurrentHP(), St->GetMaxHP()), St->GetCurrentHP(), St->GetMaxHP(),
                St->GetCurrentSP(), St->GetMaxSP(), GuardTag, *Mods);
        }
    }

    for (int32 i = 0; i < Enemies.Num(); ++i)
    {
        AABaseCharacter* C = Enemies[i];
        if (C && C->GetStatComponent())
        {
            UStatComponent* St = C->GetStatComponent();
            FString EMods;
            if (St->GetAttackMod()  > 1.f) EMods += TEXT(" 공↑");
            if (St->GetAttackMod()  < 1.f) EMods += TEXT(" 공↓");
            if (St->GetDefenseMod() > 1.f) EMods += TEXT(" 방↑");
            if (St->GetDefenseMod() < 1.f) EMods += TEXT(" 방↓");
            if (St->GetAilment() != EAilment::None)
                EMods += FString::Printf(TEXT(" [%s]"), *LexAilment(St->GetAilment()));
            if (St->IsCharged()) EMods += TEXT(" [차지]");
            if (C->bIsBoss)
            {
                FString BossTag = TEXT(" [보스");
                const int32* Ph = BossPhase.Find(C);
                if (Ph && *Ph > 0) BossTag += FString::Printf(TEXT("·P%d"), *Ph + 1); // 페이즈 표시
                if (EnragedBosses.Contains(C)) BossTag += TEXT("·분노");
                BossTag += TEXT("]");
                EMods += BossTag;
            }
            EMods += GetKnownWeaknessTag(C);
            const TCHAR* TurnMark = (C == CurrentActor) ? TEXT("▶ ") : TEXT("   ");
            const bool bDead = St->IsDead();
            Result += FString::Printf(TEXT("%s적%d  %s %.0f/%.0f%s%s\n"),
                TurnMark, i + 1,
                *MakeBar(St->GetCurrentHP(), St->GetMaxHP()), St->GetCurrentHP(), St->GetMaxHP(),
                bDead ? TEXT("  (쓰러짐)") : TEXT(""), *EMods);
        }
    }

    if (!ActionFeedback.IsEmpty())
        Result += FString::Printf(TEXT("\n> %s"), *ActionFeedback);

    return FText::FromString(Result);
}

FText ABattleManager::GetTurnOrderPreview() const
{
    const int32 Total = PlayerParty.Num() + Enemies.Num();
    if (Total == 0 || !bBattleActive) return FText::GetEmpty();

    // 턴순서(속도순) 큐를 현재 위치부터 따라가며 미리보기. 큐가 비었으면 인덱스 순 폴백.
    const int32 OrderNum = TurnOrder.Num();

    FString Result = TEXT("턴: ");
    int32 Shown = 0;
    for (int32 Step = 0; Step < Total && Shown < 6; ++Step)
    {
        const int32 Idx = (OrderNum == Total)
            ? TurnOrder[(TurnPos + Step) % Total]      // 속도순 큐
            : (CurrentTurnIndex + Step) % Total;       // 폴백
        const bool bPlayer = (Idx < PlayerParty.Num());
        AABaseCharacter* C = bPlayer ? PlayerParty[Idx] : Enemies[Idx - PlayerParty.Num()];
        if (!C || !C->GetStatComponent() || C->GetStatComponent()->IsDead()) continue;

        const int32 Num = bPlayer ? (Idx + 1) : (Idx - PlayerParty.Num() + 1);
        if (Shown > 0) Result += TEXT(" → ");
        if (Shown == 0) Result += TEXT("▶");
        Result += FString::Printf(TEXT("%s%d"), bPlayer ? TEXT("아군") : TEXT("적"), Num);
        ++Shown;
    }
    return FText::FromString(Result);
}

int32 ABattleManager::GetCurrentSkillCount() const
{
    return CurrentActor ? CurrentActor->Skills.Num() : 0;
}

FText ABattleManager::GetSkillLabel(int32 Index) const
{
    if (!CurrentActor || !CurrentActor->Skills.IsValidIndex(Index))
        return FText::GetEmpty();
    const FSkillDef& S = CurrentActor->Skills[Index];

    // 비데미지 스킬 타입 태그 (데미지기는 아래 ElemTag가 속성으로 대체)
    const TCHAR* TypeTag = TEXT("");
    switch (S.SkillType)
    {
    case ESkillType::HealSelf:      TypeTag = TEXT(" [힐]");     break;
    case ESkillType::HealOne:       TypeTag = TEXT(" [단일힐]"); break;
    case ESkillType::HealAll:       TypeTag = TEXT(" [전체힐]"); break;
    case ESkillType::BuffAttack:    TypeTag = TEXT(" [공↑]");   break;
    case ESkillType::BuffDefense:   TypeTag = TEXT(" [방↑]");   break;
    case ESkillType::DebuffAttack:  TypeTag = TEXT(" [적공↓]"); break;
    case ESkillType::DebuffDefense: TypeTag = TEXT(" [적방↓]"); break;
    case ESkillType::Analyze:       TypeTag = TEXT(" [분석]");   break;
    case ESkillType::Cure:          TypeTag = TEXT(" [치료]");   break;
    case ESkillType::Revive:        TypeTag = TEXT(" [부활]");   break;
    default: break;
    }

    const bool bDamage = (S.SkillType == ESkillType::DamageOne || S.SkillType == ESkillType::DamageAll);

    // 데미지 스킬: 속성 표시(약점 공략의 핵심 정보). 전체기는 [전체·속성].
    FString ElemTag;
    if (S.SkillType == ESkillType::DamageOne)
        ElemTag = FString::Printf(TEXT(" [%s]"), *LexBattleElement(S.Element));
    else if (S.SkillType == ESkillType::DamageAll)
        ElemTag = FString::Printf(TEXT(" [전체·%s]"), *LexBattleElement(S.Element));

    // 연타 / 부여 상태이상 / 즉사 확률 (데미지기 한정)
    FString HitTag, AilTag, InstaTag;
    if (bDamage && S.HitCount > 1)
        HitTag = FString::Printf(TEXT(" x%d"), S.HitCount);
    if (bDamage && S.InflictAilment != EAilment::None)
        AilTag = FString::Printf(TEXT(" (%s)"), *LexAilment(S.InflictAilment));
    if (bDamage && S.InstaKillChance > 0.f)
        InstaTag = FString::Printf(TEXT(" [즉사%d%%]"), FMath::RoundToInt(S.InstaKillChance * 100.f));

    // SP 부족 표시 — ※ HUD가 "(SP부족)" 문자열로 버튼 비활성 판정 → 항상 맨 끝 유지
    const TCHAR* CostTag = TEXT("");
    if (CurrentActor->GetStatComponent() &&
        CurrentActor->GetStatComponent()->GetCurrentSP() < static_cast<float>(S.SPCost))
        CostTag = TEXT(" (SP부족)");

    return FText::FromString(FString::Printf(TEXT("%s (SP %d)%s%s%s%s%s%s"),
        *S.SkillName, S.SPCost, *ElemTag, TypeTag, *HitTag, *InstaTag, *AilTag, CostTag));
}

static FString ElementName(EBattleElement E)
{
    // 공용 단일 소스(BattleTypes.h) 위임 — 도감 위젯과 동일 라벨
    return LexBattleElement(E);
}

FString ABattleManager::GetKnownWeaknessTag(AABaseCharacter* Enemy) const
{
    const TSet<EBattleElement>* Found = KnownWeaknesses.Find(Enemy);
    if (!Found || Found->Num() == 0) return FString();

    TArray<FString> Names;
    for (EBattleElement El : *Found) Names.Add(ElementName(El));
    return FString::Printf(TEXT("  (약점:%s)"), *FString::Join(Names, TEXT(",")));
}

int32 ABattleManager::GetEnemyCount() const
{
    return Enemies.Num();
}

FText ABattleManager::GetEnemyLabel(int32 Index) const
{
    if (!Enemies.IsValidIndex(Index)) return FText::GetEmpty();
    AABaseCharacter* E = Enemies[Index];
    if (!E || !E->GetStatComponent()) return FText::GetEmpty();

    UStatComponent* St = E->GetStatComponent();
    const TCHAR* Marker = (Index == CurrentEnemyTarget) ? TEXT("▶ ") : TEXT("   ");
    if (St->IsDead())
        return FText::FromString(FString::Printf(TEXT("%s적%d (쓰러짐)"), Marker, Index + 1));
    return FText::FromString(FString::Printf(TEXT("%s적%d  %s %.0f/%.0f%s"),
        Marker, Index + 1, *MakeBar(St->GetCurrentHP(), St->GetMaxHP(), 8),
        St->GetCurrentHP(), St->GetMaxHP(), *GetKnownWeaknessTag(E)));
}

void ABattleManager::SetEnemyTarget(int32 Index)
{
    if (!Enemies.IsValidIndex(Index)) return;
    AABaseCharacter* E = Enemies[Index];
    if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead())
        CurrentEnemyTarget = Index;
}

int32 ABattleManager::GetAllyCount() const
{
    return PlayerParty.Num();
}

FText ABattleManager::GetAllyLabel(int32 Index) const
{
    if (!PlayerParty.IsValidIndex(Index)) return FText::GetEmpty();
    AABaseCharacter* A = PlayerParty[Index];
    if (!A || !A->GetStatComponent()) return FText::GetEmpty();

    UStatComponent* St = A->GetStatComponent();
    const TCHAR* Marker = (Index == CurrentAllyTarget) ? TEXT("▶ ") : TEXT("   ");
    if (St->IsDead())
        return FText::FromString(FString::Printf(TEXT("%s아군%d (쓰러짐)"), Marker, Index + 1));
    return FText::FromString(FString::Printf(TEXT("%s아군%d  HP: %.0f/%.0f"),
        Marker, Index + 1, St->GetCurrentHP(), St->GetMaxHP()));
}

static FString BattlerDisplayName(AABaseCharacter* C, int32 Index, bool bEnemy)
{
    if (AANPCCharacter* N = Cast<AANPCCharacter>(C))
        if (!N->NPCName.IsEmpty()) return N->NPCName;
    if (bEnemy) return FString::Printf(TEXT("적 %d"), Index + 1);
    return (Index == 0) ? FString(TEXT("주인공")) : FString::Printf(TEXT("아군 %d"), Index + 1);
}

FBattlerView ABattleManager::GetAllyView(int32 Index) const
{
    FBattlerView V;
    if (!PlayerParty.IsValidIndex(Index)) return V;
    AABaseCharacter* C = PlayerParty[Index];
    if (!C || !C->GetStatComponent()) return V;
    UStatComponent* St = C->GetStatComponent();
    V.bValid = true;
    V.Name = BattlerDisplayName(C, Index, false);
    V.HP = St->GetCurrentHP(); V.MaxHP = FMath::Max(1.f, St->GetMaxHP());
    V.SP = St->GetCurrentSP(); V.MaxSP = St->GetMaxSP();
    V.bDead = St->IsDead();
    V.bActive = (C == CurrentActor);
    V.bTargeted = (Index == CurrentAllyTarget);

    FString T;
    if (St->GetAttackMod()  > 1.f) T += TEXT(" 공↑"); else if (St->GetAttackMod()  < 1.f) T += TEXT(" 공↓");
    if (St->GetDefenseMod() > 1.f) T += TEXT(" 방↑"); else if (St->GetDefenseMod() < 1.f) T += TEXT(" 방↓");
    if (St->GetAilment() != EAilment::None) T += FString::Printf(TEXT(" [%s]"), *LexAilment(St->GetAilment()));
    if (St->IsCharged())  T += TEXT(" [차지]");
    if (St->IsDefending()) T += TEXT(" [방어]");
    V.Tags = T.TrimStartAndEnd();
    return V;
}

FBattlerView ABattleManager::GetEnemyView(int32 Index) const
{
    FBattlerView V;
    if (!Enemies.IsValidIndex(Index)) return V;
    AABaseCharacter* C = Enemies[Index];
    if (!C || !C->GetStatComponent()) return V;
    UStatComponent* St = C->GetStatComponent();
    V.bValid = true;
    V.Name = BattlerDisplayName(C, Index, true);
    V.HP = St->GetCurrentHP(); V.MaxHP = FMath::Max(1.f, St->GetMaxHP());
    V.SP = 0.f; V.MaxSP = 0.f;   // 적은 SP 바 생략
    V.bDead = St->IsDead();
    V.bActive = (C == CurrentActor);
    V.bTargeted = (Index == CurrentEnemyTarget);

    FString T;
    if (St->GetAttackMod()  > 1.f) T += TEXT(" 공↑"); else if (St->GetAttackMod()  < 1.f) T += TEXT(" 공↓");
    if (St->GetDefenseMod() > 1.f) T += TEXT(" 방↑"); else if (St->GetDefenseMod() < 1.f) T += TEXT(" 방↓");
    if (St->GetAilment() != EAilment::None) T += FString::Printf(TEXT(" [%s]"), *LexAilment(St->GetAilment()));
    if (St->IsCharged()) T += TEXT(" [차지]");
    if (C->bIsBoss)
    {
        FString B = TEXT(" [보스");
        const int32* Ph = BossPhase.Find(C);
        if (Ph && *Ph > 0) B += FString::Printf(TEXT("·P%d"), *Ph + 1);
        if (EnragedBosses.Contains(C)) B += TEXT("·분노");
        B += TEXT("]");
        T += B;
    }
    T += GetKnownWeaknessTag(C); // "  (약점:빙결)" — 발견 시
    V.Tags = T.TrimStartAndEnd();
    return V;
}

void ABattleManager::SetAllyTarget(int32 Index)
{
    if (!PlayerParty.IsValidIndex(Index)) return;
    AABaseCharacter* A = PlayerParty[Index];
    if (A && A->GetStatComponent() && !A->GetStatComponent()->IsDead())
        CurrentAllyTarget = Index;
}

AABaseCharacter* ABattleManager::GetSelectedAlly() const
{
    if (PlayerParty.IsValidIndex(CurrentAllyTarget))
    {
        AABaseCharacter* A = PlayerParty[CurrentAllyTarget];
        if (A && A->GetStatComponent() && !A->GetStatComponent()->IsDead())
            return A;
    }
    return FindLowestHPAlly(); // 지정 아군이 죽었거나 없으면 최저HP 아군
}

// ── 아이템 (B의 InventoryComponent 연동) ──────────────────

// 보유 소비아이템(개수>0)만 필터 — 메뉴 라벨과 사용 인덱스의 일관성 유지
static TArray<FItemStack> GetOwnedItems(UInventoryComponent* Inv)
{
    TArray<FItemStack> Out;
    if (Inv)
        for (const FItemStack& S : Inv->GetStacks())
        {
            if (S.Count <= 0) continue;
            // 전투에서 못 쓰는 키/열쇠 아이템은 메뉴에서 제외(선택 시 턴 낭비·키 소모 방지).
            // GetItemCount/GetItemLabel/PlayerUseItem 전부 이 함수로 인덱싱 → 필터해도 인덱스 일관.
            FConsumableDef Def;
            if (UInventoryComponent::FindDef(S.Id, Def) && Def.Effect == EConsumableEffect::KeyItem)
                continue;
            Out.Add(S);
        }
    return Out;
}

UInventoryComponent* ABattleManager::GetPlayerInventory() const
{
    if (UWorld* W = GetWorld())
        if (APlayerController* PC = W->GetFirstPlayerController())
            if (APawn* Pawn = PC->GetPawn())
                return Pawn->FindComponentByClass<UInventoryComponent>();
    return nullptr;
}

int32 ABattleManager::GetItemCount() const
{
    return GetOwnedItems(GetPlayerInventory()).Num();
}

FText ABattleManager::GetItemLabel(int32 Index) const
{
    const TArray<FItemStack> Owned = GetOwnedItems(GetPlayerInventory());
    if (!Owned.IsValidIndex(Index)) return FText::GetEmpty();
    const FItemStack& S = Owned[Index];
    FConsumableDef Def;
    if (!UInventoryComponent::FindDef(S.Id, Def))
        return FText::FromString(FString::Printf(TEXT("%s x%d"), *S.Id.ToString(), S.Count));

    // 효과 요약을 함께 표시(전투 중 무엇인지 바로 보이게). 플레이버 Description은 인벤/상점 툴팁용으로 남김.
    FString Eff;
    switch (Def.Effect)
    {
        case EConsumableEffect::HealHP:      Eff = FString::Printf(TEXT("HP+%.0f"), Def.Magnitude); break;
        case EConsumableEffect::HealSP:      Eff = FString::Printf(TEXT("SP+%.0f"), Def.Magnitude); break;
        case EConsumableEffect::FullHeal:    Eff = TEXT("완전회복"); break;
        case EConsumableEffect::CureAilment: Eff = TEXT("상태이상 치료"); break;
        default: break;
    }
    return FText::FromString(Eff.IsEmpty()
        ? FString::Printf(TEXT("%s x%d"), *Def.Name, S.Count)
        : FString::Printf(TEXT("%s x%d  (%s)"), *Def.Name, S.Count, *Eff));
}

void ABattleManager::PlayerUseItem(int32 Index)
{
    if (!bBattleActive || !bIsPlayerTurn || !CurrentActor) return;

    UInventoryComponent* Inv = GetPlayerInventory();
    if (!Inv) return;

    const TArray<FItemStack> Owned = GetOwnedItems(Inv);
    if (!Owned.IsValidIndex(Index)) return;

    const FName Id = Owned[Index].Id;
    // 대상: 선택한 아군(미선택/솔로면 자기 자신으로 폴백)
    AABaseCharacter* TargetAlly = GetSelectedAlly();
    if (!TargetAlly) TargetAlly = CurrentActor;
    UStatComponent* TargetStat = TargetAlly->GetStatComponent();
    if (Inv->UseItemOn(TargetStat, Id))
    {
        FConsumableDef Def;
        const FString Nm = UInventoryComponent::FindDef(Id, Def) ? Def.Name : Id.ToString();
        ActionFeedback = FString::Printf(TEXT("%s 사용!"), *Nm);
        bLastHitWeakness = false;
        EndTurn(); // 아이템 = 턴 소모 (One More 없음)
    }
    else
    {
        ActionFeedback = TEXT("아이템을 쓸 수 없다");
        // 실패 시 턴 소모 안 함
    }
}

// ── 헬퍼 ─────────────────────────────────────────────────

float ABattleManager::CalculateDamage(AABaseCharacter* Attacker, AABaseCharacter* Target, float Multiplier, EBattleElement Element) const
{
    // Defense 차감은 ApplyDamage가 담당(이중 차감 방지). 여기선 공격력*배율만 산출.
    (void)Target;
    if (!Attacker || !Attacker->GetStatComponent()) return 0.f;

    UStatComponent* AS = Attacker->GetStatComponent();
    // ★ 물리 스킬은 힘(STR), 마법 스킬은 마력(MAG)을 원천으로 (세부 스탯 미설정이면 기존 Attack 폴백)
    const float Power = (Element == EBattleElement::Physical) ? AS->GetAttack() : AS->GetMagPower();
    float Dmg = Power * Multiplier;

    // 궁지(Adversity): 시전자 HP가 25% 미만이면 데미지 증가 (빈사 역전 패시브 — 기본 0=off)
    if (Attacker->LowHPDamageBonus > 0.f && AS->GetMaxHP() > 0.f
        && AS->GetCurrentHP() < AS->GetMaxHP() * 0.25f)
    {
        Dmg *= (1.f + Attacker->LowHPDamageBonus);
    }

    return FMath::Max(1.f, Dmg);
}

// 테크니컬 배율: 상태이상×속성 조합이 좋으면 더 큼 (페르소나식)
static float TechnicalMult(EAilment Ail, EBattleElement Elem)
{
    if (Ail == EAilment::None) return 1.f;
    if (Ail == EAilment::Freeze && Elem == EBattleElement::Physical) return 2.0f; // 빙결+물리 = 박살
    if (Ail == EAilment::Burn   && Elem == EBattleElement::Wind)     return 1.8f; // 화상+질풍
    if (Ail == EAilment::Shock  && Elem == EBattleElement::Physical) return 1.8f; // 감전+물리
    if (Ail == EAilment::Sleep)                                       return 1.8f; // 수면 대상 일격
    return 1.5f; // 그 외 상태이상 = 기본 테크니컬
}

void ABattleManager::PerformAttack(AABaseCharacter* Attacker, AABaseCharacter* Target, float Multiplier, EBattleElement Element, UAnimMontage* OverrideMontage)
{
    bLastHitWeakness = false;
    if (!Attacker || !Target || !Target->GetStatComponent()) return;

    // 스킬 전용 몽타주가 있으면 그걸, 없으면 기본 공격 모션 (배열 비면 무시).
    // 폴백 시 속성을 모션 변종으로 전달 → 물리/화염/빙결 등 스킬마다 다른 동작이 나오게(차별 모션).
    Attacker->PlaySkillMontage(OverrideMontage, static_cast<int32>(Element));

    // 명중 판정 (92%) — 빗나가면 데미지 없음. 단 수면/빙결 대상은 회피 불가(반드시 명중)
    const EAilment TargetAilment = Target->GetStatComponent()->GetAilment();
    const bool bCannotDodge = (TargetAilment == EAilment::Sleep || TargetAilment == EAilment::Freeze);
    // ★ 명중 = 기본 + (시전자 민첩 - 대상 민첩)*0.4%p, 0.4~1.0 클램프 (세부 스탯 미설정이면 0차→기존값)
    float EffHit = HitChance;
    if (Attacker->GetStatComponent())
        EffHit += (Attacker->GetStatComponent()->GetAGI() - Target->GetStatComponent()->GetAGI()) * 0.004f;
    EffHit = FMath::Clamp(EffHit, 0.4f, 1.f);
    if (!bCannotDodge && FMath::FRand() > EffHit)
    {
        ActionFeedback = TEXT("빗나감!");
        Flair(EBattleFlair::Miss, TEXT("MISS"));
        return;
    }

    float Base = CalculateDamage(Attacker, Target, Multiplier, Element);
    EAffinity Aff = Target->GetAffinity(Element);

    // 흡수: 대상이 데미지만큼 오히려 회복하고 종료
    if (Aff == EAffinity::Absorb)
    {
        Target->GetStatComponent()->Heal(Base);
        ActionFeedback = FString::Printf(TEXT("흡수! 적이 %.0f 회복"), Base);
        Flair(EBattleFlair::Absorb, TEXT("ABSORB"));
        return;
    }
    // 반사: 공격이 시전자에게 되돌아감
    if (Aff == EAffinity::Repel)
    {
        if (Attacker->GetStatComponent())
            Attacker->GetStatComponent()->ApplyDamage(Base);
        ActionFeedback = TEXT("반사! 공격이 되돌아왔다");
        Flair(EBattleFlair::Repel, TEXT("REPEL"));
        return;
    }

    float AffMult = 1.f;
    FString Tag;
    switch (Aff)
    {
    case EAffinity::Weak:
        AffMult = WeaknessMult; Tag = TEXT("약점! "); bLastHitWeakness = true;
        KnownWeaknesses.FindOrAdd(Target).Add(Element);
        if (UBestiarySubsystem* Bst = GetBestiary(GetWorld())) Bst->RecordWeakness(EnemyIdOf(Target), Element);
        Flair(EBattleFlair::Weakness, TEXT("WEAK!"));
        break;
    case EAffinity::Resist: AffMult = ResistMult;  Tag = TEXT("내성 ");  break;
    case EAffinity::Null:   AffMult = 0.f;   Tag = TEXT("무효! ");  break;
    default: break;
    }

    if (AffMult <= 0.f)
    {
        ActionFeedback = TEXT("무효! 데미지 없음");
        return; // 무효는 ApplyDamage 호출 안 함(최소 1 데미지 방지)
    }

    // 치명타 판정 — 기본 + 운(LUK) 보정. 약점처럼 넉다운/One More 유발
    float CritMult = 1.f;
    float EffCrit = CritChance;
    if (Attacker->GetStatComponent()) EffCrit += Attacker->GetStatComponent()->GetCritBonus();
    if (FMath::FRand() < EffCrit)
    {
        CritMult = CritMultiplier;
        bLastHitWeakness = true; // 치명타도 다운 취급 → One More/총공격 기여
        Tag = TEXT("치명타! ") + Tag;
        Flair(EBattleFlair::Critical, TEXT("CRITICAL!"));
    }

    // 테크니컬: 상태이상 걸린 대상 공격 시 보너스 + 넉다운 (속성×상태 조합으로 배율 차등)
    float TechMult = 1.f;
    {
        const EAilment TargetAil = Target->GetStatComponent()->GetAilment();
        if (TargetAil != EAilment::None)
        {
            TechMult = TechnicalMult(TargetAil, Element);
            bLastHitWeakness = true; // 테크니컬도 다운 취급
            Tag = TEXT("테크니컬! ") + Tag;
            Flair(EBattleFlair::Technical, TEXT("TECHNICAL!"));
        }
    }

    float Variance = FMath::FRandRange(0.9f, 1.1f); // ±10% 데미지 변동
    float Dealt = Target->GetStatComponent()->ApplyDamage(Base * AffMult * CritMult * TechMult * Variance);
    Target->PlayRandomHitReactionMontage(); // 피격 모션 (배열 비면 무시)
    ActionFeedback = FString::Printf(TEXT("%s%.0f 데미지"), *Tag, Dealt);

    // ── 속성 타격 연출: 모든 데미지 행동(기본공격·스킬)의 단일 지점 ──
    // 대상 가슴높이에 속성 VFX(NS_<속성>) + 약점/치명타는 키워서 강조 → "맞았다"가 확실히 보이게.
    // 속성 임팩트 효과음도 여기서 재생(기본공격은 지금까지 무음이었음).
    {
        const bool bImpactFx = (CritMult > 1.f) || (AffMult > 1.f);
        const FName ElemId = BattleElementId(Element);
        SpawnVFX(ElemId, Target, bImpactFx ? VFXScaleImpact : VFXScaleHit, VFXSpawnHeight);
        PlaySFX(ElemId, bImpactFx ? 0.75f : 0.55f);
    }

    // ── 타격감 연출 (데미지 숫자 + 히트스톱 + 카메라 흔들림) ──
    {
        const bool bImpact = (CritMult > 1.f) || (AffMult > 1.f); // 치명타/약점 = 강조
        {
            FLinearColor NumCol(0.97f, 0.97f, 1.f, 1.f);          // 일반=흰색
            if (CritMult > 1.f)      NumCol = FLinearColor(1.f, 0.85f, 0.20f, 1.f); // 치명타=금색
            else if (AffMult > 1.f)  NumCol = FLinearColor(1.f, 0.45f, 0.20f, 1.f); // 약점=주황
            ShowFloatingNumber(Target, FString::Printf(TEXT("%.0f"), Dealt), NumCol, bImpact);
        }
        UBattleFXLibrary::HitStop(this, bImpact ? 0.075f : 0.04f);
        UBattleFXLibrary::ShakeCamera(this, bImpact ? 1.25f : 0.55f);
    }

    // 근성(Endure): 대상이 치명적 데미지를 HP 1로 버텼으면 표시 + 연출
    if (Target->GetStatComponent()->ConsumeJustEndured())
    {
        ActionFeedback += TEXT("   ▶ 버텼다! (근성)");
        Flair(EBattleFlair::Endure, TEXT("ENDURE!"));
    }

    CheckBossEnrage(Target); // 보스가 HP 절반 밑으로 떨어지면 분노

    // 처치 보상: 플레이어가 적을 쓰러뜨리면 SP 소량 회복(공격 보상 + SP 수급)
    if (Target->GetStatComponent()->IsDead() && PlayerParty.Contains(Attacker) && Attacker->GetStatComponent())
        Attacker->GetStatComponent()->RestoreSP(5.f);

    // 반격(Counter): 물리 공격을 맞고 생존한 대상이 CounterChance로 시전자에게 되받아침.
    // 되받아치기는 직접 ApplyDamage(추가 반격/약점/명중 판정 없음 → 무한루프 방지). 수면/빙결이면 반격 불가.
    {
        UStatComponent* TS = Target->GetStatComponent();
        UStatComponent* AS = Attacker->GetStatComponent();
        if (Element == EBattleElement::Physical
            && TS && !TS->IsDead() && Target->CounterChance > 0.f
            && AS && !AS->IsDead()
            && TS->GetAilment() != EAilment::Sleep && TS->GetAilment() != EAilment::Freeze
            && FMath::FRand() < Target->CounterChance)
        {
            const float CtrRaw = CalculateDamage(Target, Attacker, 0.5f); // 반격 = 절반 위력 물리
            const float CtrDealt = AS->ApplyDamage(CtrRaw);
            Attacker->PlayRandomHitReactionMontage();
            ActionFeedback += FString::Printf(TEXT("  ▶ 반격! %.0f 데미지"), CtrDealt);
            Flair(EBattleFlair::Counter, TEXT("COUNTER!"));
            CheckBossEnrage(Attacker); // 반격으로 보스가 분노 조건에 들어갈 수도
        }
    }
}

void ABattleManager::Flair(EBattleFlair Kind, const FString& Label)
{
    OnBattleFlair.Broadcast(Kind, Label);

    // ★ 연출 효과음 자동 재생 — SFX_<name> 파일이 폴더에 있으면 소리남(없으면 조용히 무시).
    //   사용자는 /Game/Audio/SFX/ 에 SFX_weak, SFX_critical 등 파일만 넣으면 됨.
    static const TMap<EBattleFlair, FName> FlairSFX = {
        { EBattleFlair::Weakness,  TEXT("weak") },      { EBattleFlair::Critical,  TEXT("critical") },
        { EBattleFlair::Technical, TEXT("technical") }, { EBattleFlair::OneMore,   TEXT("onemore") },
        { EBattleFlair::BatonPass, TEXT("baton") },     { EBattleFlair::AllOut,    TEXT("allout") },
        { EBattleFlair::Miss,      TEXT("miss") },      { EBattleFlair::Repel,     TEXT("repel") },
        { EBattleFlair::Absorb,    TEXT("absorb") },    { EBattleFlair::Counter,   TEXT("counter") },
        { EBattleFlair::Ailment,   TEXT("ailment") },   { EBattleFlair::Endure,    TEXT("endure") },
        { EBattleFlair::Instakill, TEXT("instakill") }, { EBattleFlair::Ambush,    TEXT("ambush") },
        { EBattleFlair::Ambushed,  TEXT("ambushed") },  { EBattleFlair::Victory,   TEXT("victory") },
        { EBattleFlair::Defeat,    TEXT("defeat") },
    };
    if (const FName* N = FlairSFX.Find(Kind))
        PlaySFX(*N);

    // ★ 극적 순간 화면 플래시 + 추가 카메라 흔들림 (위젯 불필요, 카메라 페이드)
    switch (Kind)
    {
    case EBattleFlair::Critical:
        UBattleFXLibrary::FlashScreen(this, FLinearColor(1.f, 1.f, 1.f, 1.f), 0.5f, 0.22f);
        UBattleFXLibrary::ShakeCamera(this, 1.3f); break;
    case EBattleFlair::Weakness:
        UBattleFXLibrary::FlashScreen(this, FLinearColor(1.f, 0.8f, 0.2f, 1.f), 0.45f, 0.22f);
        UBattleFXLibrary::ShakeCamera(this, 1.2f); break;
    case EBattleFlair::Technical:
        UBattleFXLibrary::FlashScreen(this, FLinearColor(0.3f, 0.8f, 1.f, 1.f), 0.45f, 0.22f); break;
    case EBattleFlair::AllOut:
        UBattleFXLibrary::FlashScreen(this, FLinearColor(0.9f, 0.1f, 0.15f, 1.f), 0.6f, 0.35f);
        UBattleFXLibrary::ShakeCamera(this, 2.0f); break;
    case EBattleFlair::Instakill:
        UBattleFXLibrary::FlashScreen(this, FLinearColor(0.6f, 0.1f, 0.7f, 1.f), 0.6f, 0.3f);
        UBattleFXLibrary::ShakeCamera(this, 1.6f); break;
    case EBattleFlair::Repel:
    case EBattleFlair::Counter:
        UBattleFXLibrary::ShakeCamera(this, 1.1f); break;
    default: break;
    }

    // 페르소나식 화면 배너(WEAK!/1 MORE! 등) — C++ 구동
    ShowFlairBanner(Kind, Label);
}

UClass* ABattleManager::ResolveFlairClass()
{
    if (FlairWidgetClass) return FlairWidgetClass;
    if (!CachedFlairClass)
        CachedFlairClass = LoadClass<UBattleFlairWidget>(nullptr, TEXT("/Game/UI/WBP_BattleFlair.WBP_BattleFlair_C"));
    return CachedFlairClass;
}

void ABattleManager::ShowFlairBanner(EBattleFlair Kind, const FString& Label)
{
    // 표시할 가치가 있는 극적 순간만 + 강조색/초강조 결정
    FLinearColor Accent; bool bBig = false;
    switch (Kind)
    {
    case EBattleFlair::Weakness:  Accent = FLinearColor(0.98f, 0.74f, 0.12f, 1.f); break;            // 골드
    case EBattleFlair::Critical:  Accent = FLinearColor(0.96f, 0.85f, 0.20f, 1.f); bBig = true; break;// 옐로
    case EBattleFlair::Technical: Accent = FLinearColor(0.25f, 0.70f, 0.98f, 1.f); break;            // 블루
    case EBattleFlair::OneMore:   Accent = FLinearColor(0.96f, 0.28f, 0.46f, 1.f); bBig = true; break;// 핫핑크
    case EBattleFlair::BatonPass: Accent = FLinearColor(0.30f, 0.85f, 0.55f, 1.f); break;            // 그린
    case EBattleFlair::AllOut:    Accent = FLinearColor(0.90f, 0.10f, 0.16f, 1.f); bBig = true; break;// 크림슨
    case EBattleFlair::Counter:   Accent = FLinearColor(0.96f, 0.52f, 0.18f, 1.f); break;            // 오렌지
    case EBattleFlair::Repel:     Accent = FLinearColor(0.50f, 0.72f, 0.96f, 1.f); break;
    case EBattleFlair::Absorb:    Accent = FLinearColor(0.32f, 0.82f, 0.52f, 1.f); break;
    case EBattleFlair::Ailment:   Accent = FLinearColor(0.62f, 0.32f, 0.72f, 1.f); break;
    case EBattleFlair::Endure:    Accent = FLinearColor(0.72f, 0.72f, 0.78f, 1.f); break;
    case EBattleFlair::Instakill: Accent = FLinearColor(0.60f, 0.14f, 0.76f, 1.f); bBig = true; break;
    case EBattleFlair::Ambush:    Accent = FLinearColor(0.30f, 0.85f, 0.50f, 1.f); break;
    case EBattleFlair::Ambushed:  Accent = FLinearColor(0.85f, 0.30f, 0.20f, 1.f); break;
    case EBattleFlair::Victory:   Accent = FLinearColor(0.98f, 0.80f, 0.30f, 1.f); bBig = true; break;
    case EBattleFlair::Defeat:    Accent = FLinearColor(0.55f, 0.16f, 0.22f, 1.f); bBig = true; break;
    default: return;   // Miss/None 등은 배너 생략(데미지넘버·피드백으로 충분)
    }

    UClass* Cls = ResolveFlairClass();
    if (!Cls) return;
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    // 다발 배너 세로 스태거: 직전 배너와 0.55초 이내면 한 칸씩 아래로(겹침 방지), 아니면 리셋.
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (Now - LastFlairBannerTime < 0.55)
        FlairBannerStack = FMath::Min(FlairBannerStack + 1, 3);
    else
        FlairBannerStack = 0;
    LastFlairBannerTime = Now;
    const float StackY = FlairBannerStack * 74.f;

    if (UBattleFlairWidget* W = CreateWidget<UBattleFlairWidget>(PC, Cls))
    {
        W->Init(Label, Accent, bBig, StackY);
        W->AddToViewport(600);
    }
}

void ABattleManager::ShowFloatingNumber(AABaseCharacter* Target, const FString& Text, FLinearColor Color, bool bBig) const
{
    if (!Target || !DamageNumberClass) return;
    APlayerController* FXPC = UGameplayStatics::GetPlayerController(this, 0);
    if (!FXPC) return;
    if (UDamageNumberWidget* DN = CreateWidget<UDamageNumberWidget>(FXPC, DamageNumberClass))
    {
        DN->Init(Target->GetActorLocation() + FVector(0, 0, 95.f), Text, Color, bBig);
        DN->AddToViewport(500);
    }
}

void ABattleManager::PlaySFX(FName Name, float VolumeScale) const
{
    if (Name.IsNone()) return;
    if (UWorld* W = GetWorld())
        if (UGameInstance* GI = W->GetGameInstance())
            if (UGameAudioSubsystem* Audio = GI->GetSubsystem<UGameAudioSubsystem>())
                Audio->PlaySFX(Name, FMath::Max(0.f, BattleSFXVolume) * VolumeScale);
}

void ABattleManager::PlayBGM(FName Name) const
{
    if (UWorld* W = GetWorld())
        if (UGameInstance* GI = W->GetGameInstance())
            if (UGameAudioSubsystem* Audio = GI->GetSubsystem<UGameAudioSubsystem>())
                Audio->PlayBGM(Name);
}

void ABattleManager::SpawnVFX(FName Name, AActor* At, float Scale, float ZOffset) const
{
    if (Name.IsNone() || !At) return;
    UObject* Asset = UAssetResolver::ResolveVFX(Name);
    if (!Asset) return;

    // ★ ActorLocation은 캡슐 '중심'(몸통 높이)이라 그대로 쓰면 가슴에서 나옴.
    //   캡슐 half-height만큼 내려 실제 '발바닥'을 구한 뒤 ZOffset 적용 → 발치에서 솟음.
    FVector Loc = At->GetActorLocation();
    if (const ACharacter* Ch = Cast<ACharacter>(At))
    {
        if (const UCapsuleComponent* Cap = Ch->GetCapsuleComponent())
            Loc.Z -= Cap->GetScaledCapsuleHalfHeight();
    }
    Loc.Z += ZOffset;
    const FVector ScaleV(Scale);

    USceneComponent* Spawned = nullptr;
    if (UNiagaraSystem* NS = Cast<UNiagaraSystem>(Asset))
    {
        Spawned = UNiagaraFunctionLibrary::SpawnSystemAtLocation(At->GetWorld(), NS, Loc,
            FRotator::ZeroRotator, ScaleV, true, true, ENCPoolMethod::None, true);
    }
    else if (UParticleSystem* PS = Cast<UParticleSystem>(Asset))   // FXVarietyPack 등 Cascade 지원
    {
        Spawned = UGameplayStatics::SpawnEmitterAtLocation(At->GetWorld(), PS, Loc,
            FRotator::ZeroRotator, ScaleV, true);
    }

    // 안전장치: 루프형/장시간 이펙트가 화면에 안 남도록 N초 뒤 비활성(emission 중지 → 남은 파티클 자연 소멸 후 bAutoDestroy로 정리).
    if (Spawned && VFXLifeSeconds > 0.f)
    {
        if (UWorld* W = GetWorld())
        {
            TWeakObjectPtr<USceneComponent> Weak(Spawned);
            FTimerHandle Tmp;
            W->GetTimerManager().SetTimer(Tmp, FTimerDelegate::CreateLambda([Weak]()
            {
                if (USceneComponent* C = Weak.Get()) C->Deactivate();
            }), VFXLifeSeconds, false);
        }
    }
}

UAnimMontage* ABattleManager::ResolveSkillMontage(const FSkillDef& S) const
{
    if (S.SkillMontage) return S.SkillMontage;                            // 에셋 직접 지정 우선
    if (!S.SkillAnimName.IsNone()) return UAssetResolver::ResolveMontage(S.SkillAnimName); // 이름 폴백(AM_<이름>)
    return nullptr;                                                       // PerformAttack이 기본공격 모션으로 폴백
}

void ABattleManager::CheckBossEnrage(AABaseCharacter* Boss)
{
    if (!Boss || !Boss->bIsBoss || !Boss->GetStatComponent()) return;
    if (EnragedBosses.Contains(Boss)) return;

    UStatComponent* St = Boss->GetStatComponent();
    if (St->IsDead()) return;

    // HP 50% 미만 첫 도달 → 분노(전투 내내 공격력 1.5배 + 상태이상 해제)
    if (St->GetCurrentHP() < St->GetMaxHP() * 0.5f)
    {
        EnragedBosses.Add(Boss);
        St->ApplyAttackMod(1.5f, 99);
        St->CureAilment();
        // 데미지 피드백을 덮지 않고 덧붙임(플레이어가 타격+분노 둘 다 봄)
        ActionFeedback += TEXT("   ★ 보스 분노! 공격력 상승 ★");
    }
}

void ABattleManager::BossAoEStrike(AABaseCharacter* Boss, float Mult)
{
    if (!Boss) return;
    auto DefMult = [](AABaseCharacter* C) -> float
    {
        return (C && C->GetStatComponent() && C->GetStatComponent()->IsDefending()) ? 0.5f : 1.f;
    };
    bLastHitWeakness = false;
    for (AABaseCharacter* P : PlayerParty)
        if (P && P->GetStatComponent() && !P->GetStatComponent()->IsDead())
            PerformAttack(Boss, P, Mult * DefMult(P), EBattleElement::Almighty); // 만능=상성무시 광역
}

bool ABattleManager::TryBossPattern(AABaseCharacter* Boss)
{
    if (!Boss || !Boss->GetStatComponent()) return false;
    UStatComponent* BS = Boss->GetStatComponent();
    const float MaxHP = BS->GetMaxHP();
    const float HPPct = (MaxHP > 0.f) ? BS->GetCurrentHP() / MaxHP : 1.f;

    // HP 구간 → 페이즈(0:>75% / 1:>50% / 2:>25% / 3:그 이하)
    const int32 Phase = (HPPct > 0.75f) ? 0 : (HPPct > 0.5f) ? 1 : (HPPct > 0.25f) ? 2 : 3;
    int32& Tracked = BossPhase.FindOrAdd(Boss); // 기본 0

    // ① 페이즈 진입(HP 구간 하락) → 형태 전환: 누적 공격 강화 + 광역 일격(필살기)
    if (Phase > Tracked)
    {
        Tracked = Phase;
        BS->MultiplyBattleScale(1.15f);           // 페이즈마다 누적 공/방↑(영속, 분노 AttackMod와 독립 곱연산)
        BS->SetDefending(false);
        ActionFeedback = FString::Printf(TEXT("★ 보스 페이즈 %d! 형태가 바뀐다 — 광역 일격! ★"), Phase + 1);
        Flair(EBattleFlair::AllOut, FString::Printf(TEXT("PHASE %d"), Phase + 1));
        BossAoEStrike(Boss, 0.9f + 0.1f * Phase); // 페이즈 깊을수록 강한 광역
        EndTurn();
        return true;
    }

    // ② 평상시: 낮은 페이즈일수록 광역 일격 확률↑(페이즈1=base, 2=×2, 3=×3)
    if (Phase >= 1 && BossNovaChancePerPhase > 0.f
        && FMath::FRand() < BossNovaChancePerPhase * Phase)
    {
        ActionFeedback = TEXT("보스의 광역 일격!");
        Flair(EBattleFlair::Critical, TEXT("NOVA"));
        BossAoEStrike(Boss, 0.9f);
        EndTurn();
        return true;
    }

    return false; // 특수 패턴 미발동 → 일반 AI로 진행
}

void ABattleManager::TryInflictAilment(AABaseCharacter* Target, const FSkillDef& Skill)
{
    if (!Target || !Target->GetStatComponent()) return;
    if (Skill.InflictAilment == EAilment::None || Skill.AilmentChance <= 0.f) return;
    if (Target->GetStatComponent()->IsDead()) return;

    const FString AilName = LexAilment(Skill.InflictAilment);
    if (Target->ResistsAilment(Skill.InflictAilment)) // 면역이면 안 걸림
    {
        ActionFeedback += FString::Printf(TEXT("  → %s 무효!"), *AilName);
        return;
    }

    // 방어 중이면 상태이상 확률 절반(가드가 상태이상도 일부 막음)
    float EffChance = Skill.AilmentChance;
    if (Target->GetStatComponent()->IsDefending()) EffChance *= 0.5f;
    if (FMath::FRand() <= EffChance)
    {
        Target->GetStatComponent()->ApplyAilment(Skill.InflictAilment, 3);
        ActionFeedback += FString::Printf(TEXT("  → %s!"), *AilName);
        Flair(EBattleFlair::Ailment, AilName);
    }
}

bool ABattleManager::TryInstantKill(AABaseCharacter* Target, const FSkillDef& Skill)
{
    if (Skill.InstaKillChance <= 0.f || !Target || !Target->GetStatComponent()) return false;
    UStatComponent* TS = Target->GetStatComponent();
    if (TS->IsDead()) return false;
    if (Target->bIsBoss) return false; // 보스는 즉사 면역

    // 무효/흡수/반사 속성이면 즉사 안 통함(데미지 로직이 흡수/반사를 처리하게 둠)
    const EAffinity Aff = Target->GetAffinity(Skill.Element);
    if (Aff == EAffinity::Null || Aff == EAffinity::Absorb || Aff == EAffinity::Repel)
        return false;

    float Chance = Skill.InstaKillChance;
    if (Aff == EAffinity::Weak)        Chance *= 1.5f; // 약점이면 즉사율↑
    else if (Aff == EAffinity::Resist) Chance *= 0.5f; // 내성이면 즉사율↓

    if (FMath::FRand() < Chance)
    {
        TS->Kill();
        ActionFeedback = TEXT("즉사!");
        Flair(EBattleFlair::Instakill, TEXT("INSTA-KILL!"));
        return true;
    }
    return false;
}

float ABattleManager::ConsumeCharge(AABaseCharacter* Actor)
{
    if (Actor && Actor->GetStatComponent() && Actor->GetStatComponent()->IsCharged())
    {
        Actor->GetStatComponent()->SetCharged(false);
        return ChargeMult;
    }
    return 1.f;
}

void ABattleManager::EndPlayerActionOrOneMore()
{
    // 약점을 맞혔고 아직 살아있는 적이 있으면 턴을 넘기지 않음(One More)
    if (bLastHitWeakness && FindFirstLivingEnemy() != nullptr)
    {
        ActionFeedback += TEXT("   ★ 한 번 더!");
        Flair(EBattleFlair::OneMore, TEXT("1 MORE!"));
        return;
    }
    EndTurn();
}

void ABattleManager::MarkDownedAndCheckAllOut(AABaseCharacter* Enemy)
{
    if (Enemy) DownedEnemies.Add(Enemy);

    // 살아있는 적이 전부 다운 상태면 총공격 가능
    bool bAnyAlive = false;
    bool bAllDowned = true;
    for (AABaseCharacter* E : Enemies)
    {
        if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead())
        {
            bAnyAlive = true;
            if (!DownedEnemies.Contains(E)) { bAllDowned = false; break; }
        }
    }
    bAllOutReady = bAnyAlive && bAllDowned;
}

AABaseCharacter* ABattleManager::FindFirstLivingEnemy() const
{
    for (AABaseCharacter* Enemy : Enemies)
    {
        if (Enemy && Enemy->GetStatComponent() && !Enemy->GetStatComponent()->IsDead())
            return Enemy;
    }
    return nullptr;
}

AABaseCharacter* ABattleManager::GetSelectedEnemy() const
{
    if (Enemies.IsValidIndex(CurrentEnemyTarget))
    {
        AABaseCharacter* E = Enemies[CurrentEnemyTarget];
        if (E && E->GetStatComponent() && !E->GetStatComponent()->IsDead())
            return E;
    }
    return FindFirstLivingEnemy(); // 지정 타겟이 죽었거나 없으면 첫 생존 적
}

AABaseCharacter* ABattleManager::FindFirstLivingPlayer() const
{
    for (AABaseCharacter* Player : PlayerParty)
    {
        if (Player && Player->GetStatComponent() && !Player->GetStatComponent()->IsDead())
            return Player;
    }
    return nullptr;
}

// 배열에서 가장 HP 낮은 생존자 반환
static AABaseCharacter* LowestHPIn(const TArray<AABaseCharacter*>& Group)
{
    AABaseCharacter* Best = nullptr;
    float Lowest = TNumericLimits<float>::Max();
    for (AABaseCharacter* C : Group)
        if (C && C->GetStatComponent() && !C->GetStatComponent()->IsDead())
        {
            const float HP = C->GetStatComponent()->GetCurrentHP();
            if (HP < Lowest) { Lowest = HP; Best = C; }
        }
    return Best;
}

AABaseCharacter* ABattleManager::FindLowestHPAlly() const  { return LowestHPIn(PlayerParty); }
AABaseCharacter* ABattleManager::FindLowestHPEnemy() const { return LowestHPIn(Enemies); }

AABaseCharacter* ABattleManager::PickPlayerTargetFor(EBattleElement Elem) const
{
    // 약점 인지 발동: 이 속성에 약점인 생존 플레이어 중 최저HP 우선
    if (FMath::FRand() < EnemyWeaknessAware)
    {
        AABaseCharacter* Best = nullptr;
        float Low = TNumericLimits<float>::Max();
        for (AABaseCharacter* P : PlayerParty)
            if (P && P->GetStatComponent() && !P->GetStatComponent()->IsDead()
                && P->GetAffinity(Elem) == EAffinity::Weak)
            {
                const float HP = P->GetStatComponent()->GetCurrentHP();
                if (HP < Low) { Low = HP; Best = P; }
            }
        if (Best) return Best; // 약점 보유자 있으면 그쪽
    }
    return FindLowestHPAlly(); // 폴백: 최저HP 플레이어
}
