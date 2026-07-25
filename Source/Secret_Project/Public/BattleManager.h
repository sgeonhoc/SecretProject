#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "BattleTypes.h"
#include "BattleManager.generated.h"

class AABaseCharacter;
class UInventoryComponent;
class UDamageNumberWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTurnStarted, AABaseCharacter*, CurrentActor, bool, bIsPlayerTurn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleVictory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleDefeat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleFled);
// 연출 이벤트: 극적 순간을 HUD/연출 위젯에 알림 (페르소나식 팝업/사운드용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBattleFlair, EBattleFlair, Kind, const FString&, Label);

// 전투 HUD 카드용 1인 표시 데이터 (이름 + HP/SP 비율 + 상태태그). 모던 HUD가 이걸로 게이지 카드 동적 생성.
USTRUCT(BlueprintType)
struct FBattlerView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Battle") FString Name;
    UPROPERTY(BlueprintReadOnly, Category = "Battle") float HP = 0.f;
    UPROPERTY(BlueprintReadOnly, Category = "Battle") float MaxHP = 1.f;
    UPROPERTY(BlueprintReadOnly, Category = "Battle") float SP = 0.f;
    UPROPERTY(BlueprintReadOnly, Category = "Battle") float MaxSP = 0.f;
    UPROPERTY(BlueprintReadOnly, Category = "Battle") FString Tags;     // [독] [차지] [보스·P2] (약점:빙결)
    UPROPERTY(BlueprintReadOnly, Category = "Battle") bool bDead = false;
    UPROPERTY(BlueprintReadOnly, Category = "Battle") bool bActive = false; // 현재 행동 중
    UPROPERTY(BlueprintReadOnly, Category = "Battle") bool bTargeted = false; // 플레이어가 조준 중
    UPROPERTY(BlueprintReadOnly, Category = "Battle") bool bValid = false;
};

UCLASS()
class SECRET_PROJECT_API ABattleManager : public AActor
{
    GENERATED_BODY()

public:
    ABattleManager();

    // Initiative: 조우 방식에 따른 선공권(기본 Normal). 선제=플레이어 선공+첫 라운드 공격↑, 기습=적 선공+적 공격↑.
    // B의 전투 진입부(TalkUserWidget)는 2인자 호출 유지 가능(세 번째 기본값) — 어드밴티지를 주려면 enum 전달.
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void StartBattle(const TArray<AABaseCharacter*>& InParty, const TArray<AABaseCharacter*>& InEnemies, EBattleInitiative Initiative = EBattleInitiative::Normal);

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void PlayerAttack();

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void PlayerUseSkill(int32 SkillIndex);

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void PlayerDefend();

    // 차지: 이번 턴을 소모해 다음 데미지 행동을 ×2.5 강화
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void PlayerCharge();

    // 바톤 터치: One More 상태에서 선택한 아군에게 행동을 넘김(+공격 버프). 턴 순번은 그대로
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void PlayerBatonPass();

    // 바톤 가능 여부(One More 중 + 자기 외 생존 아군 존재) — HUD 버튼 표시용
    UFUNCTION(BlueprintPure, Category = "Battle")
    bool GetCanBatonPass() const;

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void PlayerFlee();

    // 모든 적이 다운됐을 때 발동 (HUD의 총공격 버튼)
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void PlayerAllOutAttack();

    UFUNCTION(BlueprintPure, Category = "Battle")
    bool GetIsAllOutReady() const { return bAllOutReady; }

    UFUNCTION(BlueprintPure, Category = "Battle")
    AABaseCharacter* GetCurrentActor() const { return CurrentActor; }

    UFUNCTION(BlueprintPure, Category = "Battle")
    bool GetIsPlayerTurn() const { return bIsPlayerTurn; }

    UFUNCTION(BlueprintPure, Category = "Battle")
    const TArray<AABaseCharacter*>& GetPlayerParty() const { return PlayerParty; }

    UFUNCTION(BlueprintPure, Category = "Battle")
    const TArray<AABaseCharacter*>& GetEnemies() const { return Enemies; }

    UFUNCTION(BlueprintPure, Category = "Battle")
    bool IsBattleActive() const { return bBattleActive; }

    // 전투 결과 메시지("승리! +N EXP..." / "패배...") — 결과 화면 바인딩용
    UFUNCTION(BlueprintPure, Category = "Battle")
    FText GetResultMessage() const { return FText::FromString(ResultMessage); }

    // HUD 텍스트 바인딩용 — 아군/적 HP 현황을 실시간 문자열로 반환
    UFUNCTION(BlueprintPure, Category = "Battle")
    FText GetBattleStatusText() const;

    // 턴 순서 미리보기 ("턴: ▶아군1 → 적1 → 아군2 ...") — 옵트인 텍스트블록용
    UFUNCTION(BlueprintPure, Category = "Battle")
    FText GetTurnOrderPreview() const;

    // 스킬 선택 메뉴 UI용 — 현재 행동 캐릭터의 스킬 개수 / 라벨("이름 (SP n)")
    UFUNCTION(BlueprintPure, Category = "Battle")
    int32 GetCurrentSkillCount() const;

    UFUNCTION(BlueprintPure, Category = "Battle")
    FText GetSkillLabel(int32 Index) const;

    // 타겟 선택 UI용 — 적 수 / 라벨(선택된 적은 ▶ 표시, 죽은 적은 쓰러짐) / 타겟 지정
    UFUNCTION(BlueprintPure, Category = "Battle")
    int32 GetEnemyCount() const;

    UFUNCTION(BlueprintPure, Category = "Battle")
    FText GetEnemyLabel(int32 Index) const;

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void SetEnemyTarget(int32 Index);

    // 아군 타겟 선택 UI용 (힐/아이템 대상) — 아군 수 / 라벨 / 타겟 지정
    UFUNCTION(BlueprintPure, Category = "Battle")
    int32 GetAllyCount() const;

    UFUNCTION(BlueprintPure, Category = "Battle")
    FText GetAllyLabel(int32 Index) const;

    // ── 모던 HUD용 1인 표시 데이터 (게이지 카드 동적 생성) ──
    UFUNCTION(BlueprintPure, Category = "Battle") FBattlerView GetAllyView(int32 Index) const;
    UFUNCTION(BlueprintPure, Category = "Battle") FBattlerView GetEnemyView(int32 Index) const;

    UFUNCTION(BlueprintCallable, Category = "Battle")
    void SetAllyTarget(int32 Index);

    // 아이템 메뉴 UI용 — 플레이어 보유 소비아이템(개수>0) 수 / 라벨("이름 x개수") / 사용
    UFUNCTION(BlueprintPure, Category = "Battle")
    int32 GetItemCount() const;

    UFUNCTION(BlueprintPure, Category = "Battle")
    FText GetItemLabel(int32 Index) const;

    // Index번째 보유 아이템을 현재 행동 캐릭터에게 사용(효과+1개 소모). 성공 시 턴 소모
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void PlayerUseItem(int32 Index);

    // BP 에디터에서 WBP_BattleHUD 클래스 할당
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> BattleHUDClass;

    // 데미지 숫자 위젯 클래스(WBP_DamageNumber). 타격 시 적 위로 숫자가 솟아오름. build_all_ui가 자동 배선.
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UDamageNumberWidget> DamageNumberClass;

    // 플레어 배너 위젯(WBP_BattleFlair). WEAK!/1 MORE! 등 페르소나 화면 배너. 미배선 시 경로 폴백 로드.
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class UBattleFlairWidget> FlairWidgetClass;

    // 난이도 스케일링: 켜면 전투 시작 시 적을 파티 평균 레벨에 맞춰 상향(공/방 + HP). 만렙(100)까지 도전 유지.
    //   ★단방향: 파티가 적보다 높을 때만 적을 끌어올림(파티가 낮으면 적은 카탈로그 원본 그대로 — 과보호 방지).
    //   기본 off — 사용자 설정 밸런스 보존. 켜면 잡몹이 후반에도 한 방에 안 죽고 압박을 유지.
    UPROPERTY(EditAnywhere, Category = "Battle|Difficulty")
    bool bScaleEnemiesToParty = false;

    // 레벨차 1당 공/방 증가폭 (0.05 = 레벨차 1당 +5%)
    UPROPERTY(EditAnywhere, Category = "Battle|Difficulty")
    float ScalePerLevel = 0.05f;

    // 레벨차 1당 적 HP 증가폭 (0.06 = 레벨차 1당 +6%). 잡몹이 후반에 한 방에 죽는 갭 해소.
    UPROPERTY(EditAnywhere, Category = "Battle|Difficulty")
    float EnemyHPScalePerLevel = 0.06f;

    // ── 전투 이펙트/사운드 튜닝(BP에서 재빌드 없이 조정) ──
    // FXVariety 등은 scale 1.0이 이미 거대 → 캐릭터 크기로 줄임. 일반 타격/약점·치명타/시전 마법진 크기.
    UPROPERTY(EditAnywhere, Category = "Battle|FX")
    float VFXScaleHit = 0.30f;
    UPROPERTY(EditAnywhere, Category = "Battle|FX")
    float VFXScaleImpact = 0.45f;
    UPROPERTY(EditAnywhere, Category = "Battle|FX")
    float VFXScaleCast = 0.40f;
    // 힐/버프/디버프 오라(Free_Spells류 — 캐릭터를 감싸는 효과, 보통 1.0 기준).
    UPROPERTY(EditAnywhere, Category = "Battle|FX")
    float VFXScaleSupport = 1.0f;
    // 스폰한 이펙트를 N초 뒤 강제 비활성(루프형이 화면에 안 남도록). 0 이하면 비활성 안 함.
    UPROPERTY(EditAnywhere, Category = "Battle|FX")
    float VFXLifeSeconds = 1.2f;
    // 타격/지원 이펙트 스폰 높이(발밑=캡슐 바닥 기준 오프셋). 작을수록 발치에서 솟음.
    UPROPERTY(EditAnywhere, Category = "Battle|FX")
    float VFXSpawnHeight = 14.f;
    // 현재 턴 유닛 발밑 지면 마커(NS_turnmark) 스케일.
    UPROPERTY(EditAnywhere, Category = "Battle|FX")
    float TurnMarkerScale = 0.6f;
    // 전투 효과음 전체 볼륨 배율(너무 크면 낮춤). 음원이 핫해서 보수적으로 낮게.
    UPROPERTY(EditAnywhere, Category = "Battle|FX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BattleSFXVolume = 0.3f;

    // 스케일 상한: 잡몹이 보스화되지 않게 캡(공/방·HP 각각). 만렙 도전은 유지하되 과도한 스펀지 방지.
    UPROPERTY(EditAnywhere, Category = "Battle|Difficulty")
    float MaxEnemyPowerScale = 6.f;
    UPROPERTY(EditAnywhere, Category = "Battle|Difficulty")
    float MaxEnemyHPScale = 12.f;

    // 시간대 난이도(옵트인): 켜면 저녁/밤 전투에서 적 강화 (B TimeComponent 연동, 기본 off)
    UPROPERTY(EditAnywhere, Category = "Battle|Difficulty")
    bool bNightEnemiesStronger = false;

    // ── 전투 수치 튜닝 (BP_BattleManager 인스턴스에서 밸런싱) ──
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning") float HitChance = 0.92f;     // 명중 확률
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning") float CritChance = 0.1f;     // 치명타 확률
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning") float CritMultiplier = 1.5f; // 치명타 배율
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning") float WeaknessMult = 1.75f;  // 약점 배율
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning") float ResistMult = 0.5f;     // 내성 배율
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning") float ChargeMult = 2.5f;     // 차지 배율
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning") float AllOutMult = 3.0f;     // 총공격 배율
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning") float FleeChance = 0.55f;    // 도주 성공 확률
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning") float AmbushAttackMult = 1.3f; // 선제/기습 시 선공 진영 첫 라운드 공격 배율
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EnemyWeaknessAware = 0.5f; // 적이 데미지 행동 시 플레이어 약점을 노릴 확률(0=항상 최저HP, 1=항상 약점 우선)

    // 전투 진입 인원 제한 (페르소나식). 플레이어(0번)·talk한 적(0번)은 항상 유지하고 앞에서부터 자름. 0이하=무제한.
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning", meta = (ClampMin = "0"))
    int32 MaxPartySize = 4;     // 활성 파티 최대 (플레이어 포함)
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning", meta = (ClampMin = "0"))
    int32 MaxEnemies = 4;       // 동시 등장 적 최대 (단일타겟 UI 버튼 4개와 정합)

    // 유대(코프) 전투 보너스: 영입 아군은 플레이어와의 인연 랭크 1당 공/방 +이 비율(페르소나식, 기본 3%/랭크 → 랭크10=+30%).
    // 0이면 끔. B RelationshipComponent 랭크를 전투 시작 시 읽어 아군 BattleScale에 반영.
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning", meta = (ClampMin = "0.0"))
    float BondCombatBonusPerRank = 0.03f;

    // 유대 근성 각성: 이 인연 랭크 이상인 영입 아군은 전투당 1회 근성(치명타 버팀) 획득(페르소나 코프 패시브). 0이면 끔.
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning", meta = (ClampMin = "0"))
    int32 BondEndureRank = 7;

    // 보스 페이즈: 페이즈당 광역 일격(전체공격) 확률 (페이즈1=이값, 2=×2, 3=×3). 0이면 평상시 광역 안 씀(페이즈 전환 시엔 항상 발동).
    UPROPERTY(EditAnywhere, Category = "Battle|Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BossNovaChancePerPhase = 0.12f;

    // ── 전투 시작 연출 (페르소나식: 진형으로 점프 배치 + 시네마틱 카메라) ──
    UPROPERTY(EditAnywhere, Category = "Battle|Intro") bool  bEnableBattleIntro = true;
    UPROPERTY(EditAnywhere, Category = "Battle|Intro") float IntroDuration   = 0.75f; // 점프 배치 시간
    UPROPERTY(EditAnywhere, Category = "Battle|Intro") float IntroJumpHeight = 140.f; // 점프 아치 높이
    UPROPERTY(EditAnywhere, Category = "Battle|Intro") float FormationSpacing = 190.f; // 같은 진영 좌우 간격
    UPROPERTY(EditAnywhere, Category = "Battle|Intro") float FormationDepth   = 340.f; // 중앙에서 각 진영까지 거리
    UPROPERTY(EditAnywhere, Category = "Battle|Intro") float CamSideOffset    = 680.f; // 카메라 측면 오프셋
    UPROPERTY(EditAnywhere, Category = "Battle|Intro") float CamBackOffset    = 320.f; // 카메라 후방 오프셋(아군 뒤)
    UPROPERTY(EditAnywhere, Category = "Battle|Intro") float CamHeight        = 300.f; // 카메라 높이
    UPROPERTY(EditAnywhere, Category = "Battle|Intro") float CamBlendTime     = 0.55f; // 카메라 전환 블렌드

    // ── 턴 추종 카메라 (현재 턴 유닛 뒤 → 상대 진영 전체가 보이는 오버숄더) ──
    UPROPERTY(EditAnywhere, Category = "Battle|Camera") bool  bTurnCamera     = true;  // 매 턴 현재 유닛 뒤로 카메라 전환
    UPROPERTY(EditAnywhere, Category = "Battle|Camera") float TurnCamBack     = 380.f; // 유닛 뒤 거리(클수록 멀리서)
    UPROPERTY(EditAnywhere, Category = "Battle|Camera") float TurnCamHeight   = 120.f; // 유닛 어깨 높이(낮을수록 눈높이)
    UPROPERTY(EditAnywhere, Category = "Battle|Camera") float TurnCamSide     = 85.f;  // 어깨 측면 오프셋(오버숄더)
    UPROPERTY(EditAnywhere, Category = "Battle|Camera") float TurnCamLookBias = 0.72f; // 시선: 0=유닛 1=상대중심(클수록 적 중앙)
    UPROPERTY(EditAnywhere, Category = "Battle|Camera") float TurnCamLookUp   = 55.f;  // 시선 목표 높이(가슴 — 낮을수록 수평)
    UPROPERTY(EditAnywhere, Category = "Battle|Camera") float TurnCamFollowSpeed = 7.f; // 전환 속도(클수록 빠른 컷)

    UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
    FOnBattleStarted OnBattleStarted;

    UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
    FOnTurnStarted OnTurnStarted;

    UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
    FOnBattleVictory OnBattleVictory;

    UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
    FOnBattleDefeat OnBattleDefeat;

    UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
    FOnBattleFled OnBattleFled;

    // 연출 이벤트 — HUD/연출 위젯이 구독해 페르소나식 팝업·사운드·아트 재생
    UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
    FOnBattleFlair OnBattleFlair;

private:
    UPROPERTY()
    TArray<AABaseCharacter*> PlayerParty;

    UPROPERTY()
    TArray<AABaseCharacter*> Enemies;

    // 약점에 맞아 다운된 적들 (총공격 판정용)
    UPROPERTY()
    TSet<TObjectPtr<AABaseCharacter>> DownedEnemies;

    // 플레이어가 공격으로 발견한 적별 약점 속성 (분석 표시용)
    TMap<TObjectPtr<AABaseCharacter>, TSet<EBattleElement>> KnownWeaknesses;

    // 이미 분노한 보스들 (분노 1회 제한)
    UPROPERTY()
    TSet<TObjectPtr<AABaseCharacter>> EnragedBosses;

    // 보스별 현재 페이즈(HP 구간). HP가 75/50/25% 밑으로 내려갈 때마다 페이즈 상승 → 전환 연출+강화+광역 일격.
    UPROPERTY()
    TMap<TObjectPtr<AABaseCharacter>, int32> BossPhase;

    UPROPERTY()
    TObjectPtr<AABaseCharacter> CurrentActor;

    int32 CurrentTurnIndex = 0;
    int32 CurrentEnemyTarget = 0; // 플레이어가 지정한 적 인덱스(Enemies 기준)
    int32 CurrentAllyTarget = 0;  // 플레이어가 지정한 아군 인덱스(PlayerParty 기준, 힐/아이템 대상)
    bool bIsPlayerTurn = false;
    bool bBattleActive = false;
    bool bLastHitWeakness = false; // 직전 PerformAttack이 약점을 맞혔는가 (One More 판정용)
    int32 BatonStack = 0;          // 현재 One More 체인에서 바톤 터치 누적 횟수(패스할수록 버프↑, 체인 끝나면 리셋)
    bool bAllOutReady = false;     // 모든 적 다운 → 총공격 가능

    FTimerHandle EnemyTurnTimer;
    FTimerHandle EndBattleTimer;

    // 전투 종료 메시지("승리!"/"패배...") — HUD 닫기 전 잠깐 표시
    FString ResultMessage;

    // 최근 행동 결과 피드백("약점! 45 데미지" 등) — 상태창 하단 표시
    FString ActionFeedback;

    UPROPERTY()
    TObjectPtr<UUserWidget> BattleHUDInstance;

    void CreateHUD();
    void RemoveHUD();

    // 선제/기습: 선공 진영을 정하고(턴순서 선두) 첫 라운드 공격 버프 + 연출 브로드캐스트
    void ApplyInitiative(EBattleInitiative Initiative);

    // ── AGI 기반 턴 순서 (페르소나/SMT식 속도 턴제) ──
    // 라운드마다 전투원을 효과 AGI 내림차순으로 정렬한 인덱스 큐. AGI 미설정(0)이면 안정정렬로 기존 [아군→적] 순서 유지(하위호환).
    UPROPERTY() TArray<int32> TurnOrder; // [아군0..,적0..] 통합 인덱스를 속도순으로 정렬
    int32 TurnPos = 0;                   // TurnOrder 내 현재 위치
    // 턴 순서 재구성. PreferSide: 0=속도순수, 1=아군 선두(선제), 2=적 선두(기습) — 선두 진영은 라운드1 한정.
    void BuildTurnOrder(int32 PreferSide = 0);

    // ── 전투 시작 연출 ──
    virtual void Tick(float DeltaSeconds) override;
    void BeginBattleIntro();   // 진형 계산 + 카메라 세팅 + 점프 애니 시작
    void FinishBattleIntro();  // 진형 확정 + NextTurn 시작
    void RestoreFieldCamera(); // 전투 종료 시 카메라 복원
    // 현재 턴 유닛 뒤로 카메라를 잡음(상대 진영 전체 + 유닛 뒷모습). 매 턴 시작 시 호출. 없으면 BattleCamera 생성.
    void FrameCameraOnActor(AABaseCharacter* Actor);

    // 현재 턴 유닛 발밑에 지면 마커(NS_turnmark) 부착 — 누구 차례인지 3D로 표시. nullptr/전투종료 시 제거.
    void UpdateTurnMarker(AABaseCharacter* Actor);
    UPROPERTY(Transient) TObjectPtr<USceneComponent> TurnMarkerComp;

    // 연출 중 1인 보간 데이터
    struct FIntroSlot { TWeakObjectPtr<AABaseCharacter> Actor; FVector Start; FVector Target; FRotator StartRot; FRotator TargetRot; };
    TArray<FIntroSlot> IntroSlots;
    bool  bIntroPlaying = false;
    float IntroElapsed  = 0.f;
    UPROPERTY() TObjectPtr<class ACameraActor> BattleCamera;
    UPROPERTY() TWeakObjectPtr<AActor> SavedViewTarget;

    // 턴 추종 카메라 보간 상태 (Tick에서 BattleCamera를 목표 포즈로 부드럽게 이동)
    bool     bCamFollow = false;
    FVector  CamTargetLoc = FVector::ZeroVector;
    FRotator CamTargetRot = FRotator::ZeroRotator;

    void NextTurn();
    void EndTurn();
    void CheckBattleEnd();
    void ExecuteEnemyTurn();

    // 플레이어 행동 후: 약점이면 턴 유지(One More), 아니면 EndTurn
    void EndPlayerActionOrOneMore();

    // 적을 다운 처리하고 전원 다운이면 bAllOutReady 세팅
    void MarkDownedAndCheckAllOut(AABaseCharacter* Enemy);

    // Element: 물리면 STR(GetAttack), 그 외면 MAG(GetMagPower)를 데미지 원천으로. 기본 물리.
    float CalculateDamage(AABaseCharacter* Attacker, AABaseCharacter* Target, float Multiplier = 1.f, EBattleElement Element = EBattleElement::Physical) const;

    // 데미지 적용 + 속성 상성 반영 + ActionFeedback 갱신 (공격/스킬/적턴 공통)
    // OverrideMontage: 스킬 전용 애니메이션(있으면 기본 공격 모션 대신 재생). 기본 nullptr=기본 모션.
    void PerformAttack(AABaseCharacter* Attacker, AABaseCharacter* Target, float Multiplier, EBattleElement Element, UAnimMontage* OverrideMontage = nullptr);

    // 스킬의 상태이상을 확률로 대상에게 부여
    void TryInflictAilment(AABaseCharacter* Target, const FSkillDef& Skill);

    // 스킬의 즉사를 판정 — 발동 시 대상을 즉시 처치하고 true 반환(일반 데미지 스킵).
    // 무효/흡수/반사 속성·보스는 면역, 약점 ×1.5 / 내성 ×0.5.
    bool TryInstantKill(AABaseCharacter* Target, const FSkillDef& Skill);

    // 보스가 HP 50% 미만으로 떨어지면 1회 분노 발동 (공격력↑ + 상태이상 해제)
    void CheckBossEnrage(AABaseCharacter* Boss);

    // 보스 페이즈/패턴: 페이즈 진입(HP 구간 하락) 시 전환 연출+누적강화+광역 일격, 평상시 확률적 광역 일격.
    // 이번 턴을 특수 패턴으로 소모했으면 true(EndTurn까지 내부 처리) → 호출부는 즉시 return.
    bool TryBossPattern(AABaseCharacter* Boss);
    // 보스가 살아있는 전 아군에게 만능 광역 공격(상성 무시). Mult=배율.
    void BossAoEStrike(AABaseCharacter* Boss, float Mult);

    // 연출 이벤트 브로드캐스트 (극적 순간 → BP 페르소나 연출)
    void Flair(EBattleFlair Kind, const FString& Label);

    // 극적 순간 화면 배너(WEAK!/1 MORE! 등) 표시. C++ 구동 — BP 불필요. Miss 등 사소한 건 생략.
    void ShowFlairBanner(EBattleFlair Kind, const FString& Label);

    // 대상 머리 위로 떠오르는 플로팅 숫자(데미지/회복/SP 등). DamageNumberClass 재사용.
    //  Text=표시 문자열, Color=색, bBig=강조(크게+팝). 회복/버프 등 비-데미지 피드백 통일용.
    void ShowFloatingNumber(AABaseCharacter* Target, const FString& Text, FLinearColor Color, bool bBig = false) const;
    // 플레어 위젯 클래스 해석: FlairWidgetClass 우선, 없으면 경로 폴백(캐시).
    UClass* ResolveFlairClass();
    UPROPERTY(Transient) TObjectPtr<UClass> CachedFlairClass;

    // 다발 배너 세로 스태거: 짧은 시간 안에 여러 배너가 뜨면(약점+치명+테크니컬) 겹치지 않게 누적 오프셋.
    double LastFlairBannerTime = -10.0;
    int32  FlairBannerStack = 0;

    // 이름으로 효과음 재생(/Game/Audio/SFX/SFX_<Name>). BattleSFXVolume×VolumeScale 볼륨. 없으면 무음.
    void PlaySFX(FName Name, float VolumeScale = 1.f) const;

    // 이름으로 배경음 재생(/Game/Audio/BGM/BGM_<Name>). 전투 시작=battle, 종료=field 복귀.
    void PlayBGM(FName Name) const;

    // 이름으로 이펙트(/Game/VFX/NS_<Name>) 스폰. Niagara·Cascade 둘 다 지원.
    //  Scale=크기 배율(타격이펙트는 1.6~2.2 권장), ZOffset=발밑 기준 높이(가슴=90, 마법진=8).
    void SpawnVFX(FName Name, AActor* At, float Scale = 1.8f, float ZOffset = 90.f) const;

    // 스킬 몽타주 결정: SkillMontage(에셋) 우선 → 없으면 AM_<SkillAnimName> 로드 → 그것도 없으면 null(기본공격 폴백)
    UAnimMontage* ResolveSkillMontage(const struct FSkillDef& S) const;

    // 차지 상태면 ×2.5 반환하고 소모, 아니면 1.0 (데미지 행동 직전 호출)
    float ConsumeCharge(AABaseCharacter* Actor);

    AABaseCharacter* FindFirstLivingEnemy() const;
    AABaseCharacter* FindFirstLivingPlayer() const;

    // 가장 HP 낮은 생존자 (단일 회복 타겟용)
    AABaseCharacter* FindLowestHPAlly() const;
    AABaseCharacter* FindLowestHPEnemy() const;

    // 적 데미지 행동 타겟 선택: EnemyWeaknessAware 확률로 해당 속성 약점 플레이어(최저HP) 우선, 아니면 최저HP 폴백
    AABaseCharacter* PickPlayerTargetFor(EBattleElement Elem) const;

    // 플레이어가 지정한 적(죽었거나 무효면 첫 생존 적으로 폴백)
    AABaseCharacter* GetSelectedEnemy() const;

    // 플레이어가 지정한 아군(죽었거나 무효면 최저HP 아군으로 폴백)
    AABaseCharacter* GetSelectedAlly() const;

    // 플레이어 폰의 인벤토리 컴포넌트 (없으면 nullptr)
    UInventoryComponent* GetPlayerInventory() const;

    // 발견된 적 약점을 "  (약점:화염,빙결)" 형태로 반환(없으면 빈 문자열)
    FString GetKnownWeaknessTag(AABaseCharacter* Enemy) const;
};
