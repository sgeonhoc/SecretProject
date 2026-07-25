#include "HexGridManager.h"
#include "HexTile.h"
#include "HexUnit.h"
#include "ABaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

AHexGridManager::AHexGridManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHexGridManager::BeginPlay()
{
	Super::BeginPlay();
	StartBattle();
}

void AHexGridManager::StartBattle()
{
	Kan = KanMax;
	BuildLevels();
	if (Levels.Num() == 0) { return; }
	LevelIndex = FMath::Clamp(LevelIndex, 0, Levels.Num() - 1);
	const FHexLevel& L = Levels[LevelIndex];
	GridRadius = L.Radius;   // 레벨 반경으로 필드 크기 결정

	BuildCards();
	BuildGrid();
	SpawnUnits();

	// 무대 지형 프리필(라셀 배경) — 정본=게임_전투레벨_정본.md
	for (const FHexSurfaceCell& Cell : L.Surfaces) { ApplySurface(Cell.Coord, Cell.Surf); }
	for (const FHexCoord& Wc : L.Walls) { if (InField(Wc) && !UnitAt(Wc)) { Walls.Add(Wc, 999999); } } // 지형 벽=안 삭음
	RefreshTileFlags();

	Log(FString::Printf(TEXT("【%s】 %s — 목표: %s"), *L.Stage, *L.Flavor, *L.Objective));
	Log(TEXT("개전. 칸=행동 슬롯(완성 예산). 큰 영창은 여러 칸·완성까지 무방비. 언령=과녁은 눈, 주술=벽 뒤 우회."));
	Changed();
}

const FHexLevel& AHexGridManager::CurrentLevel() const
{
	static const FHexLevel Fallback;
	return Levels.IsValidIndex(LevelIndex) ? Levels[LevelIndex] : Fallback;
}

// 라셀 현대 배경 레벨 표 — 웹 LEVELS(전투시뮬.html)와 같은 데이터. 정본=게임_전투레벨_정본.md
void AHexGridManager::BuildLevels()
{
	if (Levels.Num() > 0) { return; }
	auto SC = [](int32 q, int32 r, ESurface s) { return FHexSurfaceCell(FHexCoord(q, r), s); };

	{ FHexLevel L; L.Id = TEXT("L1_폐선지하수로"); L.Stage = TEXT("폐선 지하수로 · 첫 물길");
	  L.Flavor = TEXT("삼각주 아래 고인 물, 좁은 관. 물길이 판을 반으로 가른다 — 젖음 위에 서리를 얹으면 언다.");
	  L.Objective = TEXT("첫 마주침에서 살아 나가기");
	  L.Radius = 3; L.PlayerStart = FHexCoord(-3, 3); L.PlayerName = TEXT("아군 (빙·언령)"); L.PlayerHP = 3400;
	  L.EnemyStart = FHexCoord(3, -3); L.EnemyName = TEXT("불 초심자"); L.EnemyHP = 2600; L.EnemyStrike = TEXT("불덩이");
	  L.Surfaces = { SC(-2,2,ESurface::Wet),SC(-1,1,ESurface::Wet),SC(0,0,ESurface::Wet),SC(1,-1,ESurface::Wet),SC(2,-2,ESurface::Wet),SC(1,1,ESurface::Dust),SC(-1,-1,ESurface::Dust) };
	  L.Walls = { FHexCoord(2,1),FHexCoord(-2,-1) };
	  Levels.Add(L); }

	{ FHexLevel L; L.Id = TEXT("L2_아랫장터뒷골목"); L.Stage = TEXT("아랫장터 뒷골목 · 닫힌 골목");
	  L.Flavor = TEXT("장터 뒤 좁은 골목. 벽 두 줄이 지그재그 병목을 만든다 — 언령은 시야가 막히고 주술은 돌아간다.");
	  L.Objective = TEXT("갇히기 전에 제압하고 빠져나온다");
	  L.Radius = 4; L.PlayerStart = FHexCoord(-3, 3); L.PlayerName = TEXT("아군 (물·언령)"); L.PlayerHP = 4000;
	  L.EnemyStart = FHexCoord(3, -3); L.EnemyName = TEXT("돌·진 술자"); L.EnemyHP = 4000; L.EnemyStrike = TEXT("돌팔매");
	  L.Surfaces = { SC(0,0,ESurface::Wet),SC(0,1,ESurface::Wet) };
	  L.Walls = { FHexCoord(1,-2),FHexCoord(1,-1),FHexCoord(1,0),FHexCoord(-1,0),FHexCoord(-1,1),FHexCoord(-1,2) };
	  Levels.Add(L); }

	{ FHexLevel L; L.Id = TEXT("L3_신항굴착갱"); L.Stage = TEXT("신항 굴착 갱 · 내려가는 자들");
	  L.Flavor = TEXT("지하 열여덟 자, 옛 석축이 드러난 방. 바닥 가운데가 통째로 돌먼지 — 돌 계열 합이 상시로 선다.");
	  L.Objective = TEXT("물건을 안고 갱을 빠져나온다");
	  L.Radius = 3; L.PlayerStart = FHexCoord(-3, 3); L.PlayerName = TEXT("아군 (돌·언령)"); L.PlayerHP = 4000;
	  L.EnemyStart = FHexCoord(3, -3); L.EnemyName = TEXT("불 언령 술자"); L.EnemyHP = 3800; L.EnemyStrike = TEXT("불덩이");
	  L.Surfaces = { SC(0,0,ESurface::Dust),SC(1,0,ESurface::Dust),SC(-1,0,ESurface::Dust),SC(0,1,ESurface::Dust),SC(0,-1,ESurface::Dust),SC(1,-1,ESurface::Dust),SC(-1,1,ESurface::Dust) };
	  L.Walls = { FHexCoord(2,-1),FHexCoord(-2,1) };
	  Levels.Add(L); }

	{ FHexLevel L; L.Id = TEXT("L4_학당가강둑"); L.Stage = TEXT("학당가 강둑 · 물과 불");
	  L.Flavor = TEXT("한쪽 변이 통째로 강이다. 강 쪽으로 밀면 젖고, 젖으면 언다 — 밀침과 이동이 값을 하는 판.");
	  L.Objective = TEXT("추격을 따돌린다");
	  L.Radius = 4; L.PlayerStart = FHexCoord(-3, 3); L.PlayerName = TEXT("아군 (불·언령)"); L.PlayerHP = 4000;
	  L.EnemyStart = FHexCoord(3, -3); L.EnemyName = TEXT("물 언령 술자"); L.EnemyHP = 4000; L.EnemyStrike = TEXT("물송곳");
	  L.Surfaces = { SC(2,1,ESurface::Wet),SC(1,2,ESurface::Wet),SC(0,3,ESurface::Wet),SC(-1,3,ESurface::Wet) };
	  L.Walls = { FHexCoord(2,-1),FHexCoord(-2,1) };
	  Levels.Add(L); }

	{ FHexLevel L; L.Id = TEXT("L5_유리탑로비"); L.Stage = TEXT("강 건너 유리탑 로비 · 깨끗한 얼굴");
	  L.Flavor = TEXT("매끈한 대리석 로비. 깔린 표면이 하나도 없다 — 대칭 기둥 여섯이 만드는 순수 시야 싸움.");
	  L.Objective = TEXT("로비를 돌파한다");
	  L.Radius = 5; L.PlayerStart = FHexCoord(-4, 4); L.PlayerName = TEXT("아군 (빙·언령)"); L.PlayerHP = 4200;
	  L.EnemyStart = FHexCoord(4, -4); L.EnemyName = TEXT("불 고수"); L.EnemyHP = 4800; L.EnemyStrike = TEXT("불비");
	  L.Surfaces = {};
	  L.Walls = { FHexCoord(2,0),FHexCoord(-2,0),FHexCoord(0,2),FHexCoord(0,-2),FHexCoord(2,-2),FHexCoord(-2,2) };
	  Levels.Add(L); }

	{ FHexLevel L; L.Id = TEXT("L6_옛도읍폐허"); L.Stage = TEXT("옛 도읍 폐허 · 주인 없는 물건");
	  L.Flavor = TEXT("강 상류 폐허. 무너진 기둥 여덟이 막힌 길과 뚫린 길을 뒤섞고, 물웅덩이와 돌먼지가 함께 깔렸다.");
	  L.Objective = TEXT("폐허의 물건을 회수한다");
	  L.Radius = 5; L.PlayerStart = FHexCoord(-4, 4); L.PlayerName = TEXT("아군 (빙·언령)"); L.PlayerHP = 4500;
	  L.EnemyStart = FHexCoord(4, -4); L.EnemyName = TEXT("고대 잔존 (불·돌 복합)"); L.EnemyHP = 6500; L.EnemyStrike = TEXT("돌창");
	  L.Surfaces = { SC(0,0,ESurface::Dust),SC(1,-1,ESurface::Dust),SC(-1,1,ESurface::Dust),SC(2,-2,ESurface::Dust),SC(-2,2,ESurface::Dust),SC(1,1,ESurface::Wet),SC(-1,-1,ESurface::Wet) };
	  L.Walls = { FHexCoord(2,0),FHexCoord(-2,0),FHexCoord(0,3),FHexCoord(0,-3),FHexCoord(3,-2),FHexCoord(-3,2),FHexCoord(1,2),FHexCoord(-1,-2) };
	  Levels.Add(L); }

	{ FHexLevel L; L.Id = TEXT("L7_화물창지붕"); L.Stage = TEXT("강변 화물창 지붕 · 바람 부는 판");
	  L.Flavor = TEXT("사방이 트인 지붕, 비 그친 직후. 벽은 환기탑 둘뿐이라 숨을 곳이 없고, 젖은 자리가 흩어져 어디든 얼 수 있다.");
	  L.Objective = TEXT("지붕을 건너 반대편으로 빠진다");
	  L.Radius = 5; L.PlayerStart = FHexCoord(-4, 4); L.PlayerName = TEXT("아군 (돌·언령)"); L.PlayerHP = 4300;
	  L.EnemyStart = FHexCoord(4, -4); L.EnemyName = TEXT("빙·언령 술자"); L.EnemyHP = 4400; L.EnemyStrike = TEXT("얼음화살");
	  L.Surfaces = { SC(0,0,ESurface::Wet),SC(2,-1,ESurface::Wet),SC(-2,1,ESurface::Wet),SC(1,2,ESurface::Wet),SC(-1,-2,ESurface::Wet),SC(3,-3,ESurface::Wet) };
	  L.Walls = { FHexCoord(1,-1),FHexCoord(-1,1) };
	  Levels.Add(L); }

	{ FHexLevel L; L.Id = TEXT("L8_아래층회랑"); L.Stage = TEXT("아래층 회랑 · 도시 규모의 켜");
	  L.Flavor = TEXT("갱이 뚫고 내려간 아래층. 기둥이 두 줄로 서서 복도 셋을 만든다 — 어느 복도로 드느냐가 곧 교전 거리다.");
	  L.Objective = TEXT("회랑 끝까지 간다");
	  L.Radius = 4; L.PlayerStart = FHexCoord(-3, 3); L.PlayerName = TEXT("아군 (물·언령)"); L.PlayerHP = 4400;
	  L.EnemyStart = FHexCoord(3, -3); L.EnemyName = TEXT("돌 고수"); L.EnemyHP = 5000; L.EnemyStrike = TEXT("돌창");
	  L.Surfaces = { SC(0,0,ESurface::Dust),SC(0,1,ESurface::Dust),SC(0,-1,ESurface::Dust),SC(0,2,ESurface::Dust),SC(0,-2,ESurface::Dust),SC(2,-2,ESurface::Dust),SC(-2,2,ESurface::Dust),SC(2,-1,ESurface::Dust),SC(-2,1,ESurface::Dust) };
	  L.Walls = { FHexCoord(1,-2),FHexCoord(1,-1),FHexCoord(1,0),FHexCoord(1,1),FHexCoord(-1,-1),FHexCoord(-1,0),FHexCoord(-1,1),FHexCoord(-1,2) };
	  Levels.Add(L); }

	{ FHexLevel L; L.Id = TEXT("L9_침수승강장"); L.Stage = TEXT("침수된 승강장 · 무릎까지 물");
	  L.Flavor = TEXT("판 전체가 물이다. 서리 한 번이면 판이 통째로 언다 — 내가 깐 것에 내가 걸리는 양날의 판.");
	  L.Objective = TEXT("승강장을 장악한다");
	  L.Radius = 4; L.PlayerStart = FHexCoord(-3, 3); L.PlayerName = TEXT("아군 (불·언령)"); L.PlayerHP = 4600;
	  L.EnemyStart = FHexCoord(3, -3); L.EnemyName = TEXT("물·빙 복합 술자"); L.EnemyHP = 5200; L.EnemyStrike = TEXT("물송곳");
	  L.Surfaces = { SC(0,0,ESurface::Wet),SC(1,0,ESurface::Wet),SC(-1,0,ESurface::Wet),SC(0,1,ESurface::Wet),SC(0,-1,ESurface::Wet),SC(1,-1,ESurface::Wet),SC(-1,1,ESurface::Wet),SC(2,-1,ESurface::Wet),SC(-2,1,ESurface::Wet),SC(1,1,ESurface::Wet),SC(-1,-1,ESurface::Wet),SC(2,-2,ESurface::Wet),SC(-2,2,ESurface::Wet),SC(0,2,ESurface::Wet),SC(0,-2,ESurface::Wet),SC(2,0,ESurface::Wet),SC(-2,0,ESurface::Wet),SC(1,-2,ESurface::Wet),SC(-1,2,ESurface::Wet) };
	  L.Walls = { FHexCoord(3,-1),FHexCoord(-3,1),FHexCoord(0,3) };
	  Levels.Add(L); }

	{ FHexLevel L; L.Id = TEXT("L10_무너진성문"); L.Stage = TEXT("무너진 성문 앞뜰 · 옛 뼈의 문턱");
	  L.Flavor = TEXT("성문 잔해가 가운데를 가로질러 판을 두 마당으로 나눈다. 뚫린 틈은 양끝 둘뿐 — 언제 들어가느냐가 전부다.");
	  L.Objective = TEXT("문턱을 넘는다");
	  L.Radius = 5; L.PlayerStart = FHexCoord(-4, 4); L.PlayerName = TEXT("아군 (빙·언령)"); L.PlayerHP = 4800;
	  L.EnemyStart = FHexCoord(4, -4); L.EnemyName = TEXT("고대 잔존 (불·돌·바람 복합)"); L.EnemyHP = 7200; L.EnemyStrike = TEXT("불비");
	  L.Surfaces = { SC(2,-2,ESurface::Fire),SC(-2,2,ESurface::Fire),SC(0,0,ESurface::Dust),SC(1,-1,ESurface::Dust),SC(-1,1,ESurface::Dust),SC(3,-3,ESurface::Dust) };
	  L.Walls = { FHexCoord(-3,4),FHexCoord(-2,3),FHexCoord(-1,2),FHexCoord(0,1),FHexCoord(1,0),FHexCoord(2,-1),FHexCoord(3,-2),FHexCoord(4,-3) };
	  Levels.Add(L); }
}

// ─────────────────────────────── 빌드 ───────────────────────────────
void AHexGridManager::BuildGrid()
{
	UWorld* W = GetWorld();
	if (!W) { return; }
	const int32 Rr = GridRadius;
	for (int32 q = -Rr; q <= Rr; ++q)
	{
		for (int32 r = -Rr; r <= Rr; ++r)
		{
			if (FMath::Max3(FMath::Abs(q), FMath::Abs(r), FMath::Abs(q + r)) > Rr) { continue; }
			const FHexCoord C(q, r);
			const FVector Loc = WorldOfCoord(C);
			FActorSpawnParameters Sp;
			Sp.Owner = this;
			AHexTile* T = W->SpawnActor<AHexTile>(AHexTile::StaticClass(), Loc, FRotator::ZeroRotator, Sp);
			if (T)
			{
				// 타일은 낮은 원반: 지름 ~ 헥스, 두께 얇게
				const float Diam = HexSize * 1.7f / 100.0f; // 실린더 기본 반지름 50uu 기준 스케일
				T->MeshComp->SetRelativeScale3D(FVector(Diam, Diam, 0.12f));
				T->Setup(C);
				Tiles.Add(C, T);
			}
		}
	}
}

// 실제 캐릭터 BP를 유닛 몸으로 스폰(AI 미소유·움직임 정지·클릭 콜리전 통과)
void AHexGridManager::SpawnCharacterFor(AHexUnit* Unit, const TCHAR* BPPath)
{
	if (!Unit) { return; }
	UWorld* W = GetWorld();
	UClass* BP = LoadClass<AABaseCharacter>(nullptr, BPPath);
	if (!W || !BP) { return; } // 못 찾으면 원기둥 플레이스홀더 유지

	const FVector Ground = WorldOfCoord(Unit->Coord);
	const FTransform T(FRotator::ZeroRotator, Ground + FVector(0, 0, 90));
	AABaseCharacter* C = W->SpawnActorDeferred<AABaseCharacter>(BP, T, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!C) { return; }
	C->AutoPossessAI = EAutoPossessAI::Disabled; // AI가 안 돌아다니게
	UGameplayStatics::FinishSpawningActor(C, T);
	if (UCharacterMovementComponent* Mv = C->GetCharacterMovement()) { Mv->StopMovementImmediately(); Mv->DisableMovement(); Mv->GravityScale = 0.0f; }
	if (UCapsuleComponent* Cap = C->GetCapsuleComponent()) { Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore); }
	if (USkeletalMeshComponent* Mesh = C->GetMesh()) { Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore); }
	Unit->SetCharacter(C);
}

void AHexGridManager::SpawnUnits()
{
	UWorld* W = GetWorld();
	if (!W) { return; }
	FActorSpawnParameters Sp; Sp.Owner = this;

	const FHexLevel& L = CurrentLevel();
	const FHexCoord PC = L.PlayerStart, EC = L.EnemyStart;
	PlayerUnit = W->SpawnActor<AHexUnit>(AHexUnit::StaticClass(), WorldOfCoord(PC), FRotator::ZeroRotator, Sp);
	if (PlayerUnit) { PlayerUnit->Coord = PC; PlayerUnit->Setup(true, L.PlayerName, L.PlayerHP); }

	EnemyUnit = W->SpawnActor<AHexUnit>(AHexUnit::StaticClass(), WorldOfCoord(EC), FRotator::ZeroRotator, Sp);
	if (EnemyUnit) { EnemyUnit->Coord = EC; EnemyUnit->Setup(false, L.EnemyName, L.EnemyHP); }

	// 실제 캐릭터 몸 연결(우리가 뽑은 캐릭터). 실패 시 원기둥 유지.
	SpawnCharacterFor(PlayerUnit, TEXT("/Game/BP_Characters/BP_PlayerCharacter.BP_PlayerCharacter_C"));
	SpawnCharacterFor(EnemyUnit, TEXT("/Game/BP_Characters/BP_ANPCCharacter1.BP_ANPCCharacter1_C"));

	// 위치 확정 후 서로 마주보게
	RefreshUnitWorld(PlayerUnit);
	RefreshUnitWorld(EnemyUnit);
	if (PlayerUnit && EnemyUnit)
	{
		PlayerUnit->FaceToward(EnemyUnit->GetActorLocation());
		EnemyUnit->FaceToward(PlayerUnit->GetActorLocation());
	}
}

// 카드값 셋업 헬퍼(가독)
static FSalanCard MakeCard(const TCHAR* Name, ESalanElement El, ESalanPath Path, int32 T, int32 Cast, ECardType Type)
{
	FSalanCard C;
	C.Name = Name; C.Id = FName(Name); C.Element = El; C.Path = Path; C.Tier = T; C.Cast = Cast; C.Type = Type;
	return C;
}

void AHexGridManager::BuildCards()
{
	Cards.Reset();
	auto Add = [&](FSalanCard C) { Cards.Add(C); };

	// ── 소리길(언령): 과녁=눈, 영창은 끊긴다 ──
	{ FSalanCard c = MakeCard(TEXT("얼음화살"), ESalanElement::Frost, ESalanPath::Sori, 3, 1, ECardType::Strike);
	  c.Range = 5; c.Damage = 55; c.ComboBonus = 120; c.ComboSurf = { ESurface::Wet, ESurface::Frost, ESurface::Ice }; c.Leaves = ESurface::Frost; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("무릎 서리"), ESalanElement::Frost, ESalanPath::Sori, 4, 1, ECardType::Control);
	  c.Range = 5; c.SlowDur = 2; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("손목 서리"), ESalanElement::Frost, ESalanPath::Sori, 3, 1, ECardType::Disarm);
	  c.Range = 5; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("고드름 비"), ESalanElement::Frost, ESalanPath::Sori, 5, 2, ECardType::Strike);
	  c.Range = 5; c.Damage = 95; c.Aoe = 1; c.ComboBonus = 175; c.ComboSurf = { ESurface::Wet }; c.Leaves = ESurface::Frost; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("얼음 성벽 세우기"), ESalanElement::Frost, ESalanPath::Sori, 7, 3, ECardType::Wall);
	  c.bWallRing = true; c.WallDur = 3; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("내려앉는 겨울"), ESalanElement::Frost, ESalanPath::Sori, 10, 5, ECardType::GreatSpell);
	  c.Range = 3; c.Aoe = 2; c.GreatKind = 1; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("불덩이"), ESalanElement::Fire, ESalanPath::Sori, 3, 1, ECardType::Strike);
	  c.Range = 5; c.Damage = 60; c.ComboBonus = 130; c.ComboSurf = { ESurface::Fire, ESurface::SpreadFire }; c.Leaves = ESurface::Fire; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("불살"), ESalanElement::Fire, ESalanPath::Sori, 3, 1, ECardType::Strike);
	  c.Range = 5; c.Damage = 60; c.Leaves = ESurface::Fire; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("불씨 뿌리기"), ESalanElement::Fire, ESalanPath::Sori, 1, 1, ECardType::Surface);
	  c.Range = 4; c.Leaves = ESurface::Fire; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("불비"), ESalanElement::Fire, ESalanPath::Sori, 5, 2, ECardType::Strike);
	  c.Range = 5; c.Damage = 110; c.Aoe = 1; c.Leaves = ESurface::Fire; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("불지옥"), ESalanElement::Fire, ESalanPath::Sori, 6, 4, ECardType::GreatSpell);
	  c.Range = 4; c.Damage = 90; c.Aoe = 1; c.GreatKind = 0; c.ComboBonus = 320; c.ComboSurf = { ESurface::Fire, ESurface::SpreadFire }; c.Leaves = ESurface::Fire; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("물송곳"), ESalanElement::Water, ESalanPath::Sori, 3, 1, ECardType::Strike);
	  c.Range = 5; c.Damage = 50; c.ComboBonus = 110; c.ComboSurf = { ESurface::Wet }; c.Leaves = ESurface::Wet; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("젖은 바닥"), ESalanElement::Water, ESalanPath::Sori, 2, 1, ECardType::Surface);
	  c.Range = 4; c.Leaves = ESurface::Wet; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("바람칼"), ESalanElement::Wind, ESalanPath::Sori, 3, 1, ECardType::Strike);
	  c.Range = 5; c.Damage = 45; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("몰이 바람"), ESalanElement::Wind, ESalanPath::Sori, 4, 1, ECardType::Push);
	  c.Range = 4; c.Knock = 2; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("바람 부치기"), ESalanElement::Wind, ESalanPath::Sori, 2, 1, ECardType::WindSweep);
	  c.Range = 4; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("돌팔매"), ESalanElement::Stone, ESalanPath::Sori, 3, 1, ECardType::Strike);
	  c.Range = 5; c.Damage = 55; c.ComboBonus = 100; c.ComboSurf = { ESurface::Dust }; c.Leaves = ESurface::Dust; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("돌창"), ESalanElement::Stone, ESalanPath::Sori, 4, 2, ECardType::Strike);
	  c.Range = 5; c.Damage = 85; c.Leaves = ESurface::Dust; Add(c); }

	// ── 몸길(연공): 근접·즉발·안 끊김 ──
	{ FSalanCard c = MakeCard(TEXT("스무 걸음"), ESalanElement::Body, ESalanPath::Mom, 4, 1, ECardType::Move);
	  c.MoveRange = 5; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("강철 정강이"), ESalanElement::Body, ESalanPath::Mom, 3, 1, ECardType::Strike);
	  c.Range = 1; c.Damage = 65; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("굳은 살갗"), ESalanElement::Body, ESalanPath::Mom, 4, 1, ECardType::Defend);
	  c.Shield = 80; Add(c); }

	// ── 그림길(진): 선새김·설치 ──
	{ FSalanCard c = MakeCard(TEXT("서리 사슬 진"), ESalanElement::Frost, ESalanPath::Grim, 4, 2, ECardType::JinTrap);
	  c.Range = 3; c.FreezeDur = 1; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("무게 진"), ESalanElement::Stone, ESalanPath::Grim, 4, 2, ECardType::JinTrap);
	  c.Range = 3; c.SlowDur = 2; c.TrapDamage = 40; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("돌벽 진"), ESalanElement::Stone, ESalanPath::Grim, 4, 2, ECardType::Wall);
	  c.Range = 3; c.WallDur = 3; Add(c); }

	// ── 매개길(주술): 벽 뒤 우회·안 끊김 ──
	{ FSalanCard c = MakeCard(TEXT("실 건 바늘"), ESalanElement::Curse, ESalanPath::Mae, 5, 2, ECardType::Strike);
	  c.Range = 9; c.Damage = 70; c.bThrough = true; Add(c); }
	{ FSalanCard c = MakeCard(TEXT("묶은 발"), ESalanElement::Curse, ESalanPath::Mae, 4, 2, ECardType::Control);
	  c.Range = 9; c.bThrough = true; c.SlowDur = 2; Add(c); }

	// ── 칸 방해(말문 막기) ──
	{ FSalanCard c = MakeCard(TEXT("말문 막기"), ESalanElement::Wind, ESalanPath::Sori, 4, 1, ECardType::Interrupt);
	  c.Range = 6; Add(c); }
}

// ─────────────────────────────── 좌표/월드 ───────────────────────────────
FVector AHexGridManager::WorldOfCoord(const FHexCoord& C) const
{
	const FVector2D P = SalanHex::ToWorld2D(C, HexSize);
	return GetActorLocation() + FVector(P.X, P.Y, 0.0f);
}

AHexTile* AHexGridManager::TileAt(const FHexCoord& C) const
{
	const TObjectPtr<AHexTile>* Found = Tiles.Find(C);
	return Found ? Found->Get() : nullptr;
}

bool AHexGridManager::InField(const FHexCoord& C) const
{
	return FMath::Max3(FMath::Abs(C.Q), FMath::Abs(C.R), FMath::Abs(C.Q + C.R)) <= GridRadius;
}

AHexUnit* AHexGridManager::UnitAt(const FHexCoord& C) const
{
	if (PlayerUnit && PlayerUnit->IsAlive() && PlayerUnit->Coord == C) { return PlayerUnit; }
	if (EnemyUnit && EnemyUnit->IsAlive() && EnemyUnit->Coord == C) { return EnemyUnit; }
	return nullptr;
}

bool AHexGridManager::HasLoS(const FHexCoord& A, const FHexCoord& B) const
{
	TArray<FHexCoord> LineArr;
	SalanHex::Line(A, B, LineArr);
	for (int32 i = 1; i < LineArr.Num() - 1; ++i)
	{
		if (Walls.Contains(LineArr[i])) { return false; }
	}
	return true;
}

// ─────────────────────────────── 규칙 ───────────────────────────────
void AHexGridManager::Log(const FString& Msg)
{
	LogLines.Insert(Msg, 0);
	if (LogLines.Num() > 60) { LogLines.SetNum(60); }
	OnBattleLog.Broadcast(Msg);
	UE_LOG(LogTemp, Log, TEXT("[Salan] %s"), *Msg);
}

bool AHexGridManager::CanStartCardIndex(int32 Index) const
{
	return Cards.IsValidIndex(Index) && CanStartCard(Cards[Index]);
}

FString AHexGridManager::ChargeText() const
{
	if (!Charge.bActive) { return FString(); }
	const FString Who = Charge.bWhoPlayer ? TEXT("내") : TEXT("적");
	const FString Nm = Cards.IsValidIndex(Charge.CardIndex) ? Cards[Charge.CardIndex].Name : TEXT("?");
	return FString::Printf(TEXT("%s 영창 %s %d/%d"), *Who, *Nm, Charge.Acc, Charge.Need);
}

void AHexGridManager::Restart()
{
	GetWorldTimerManager().ClearTimer(AITimer);
	for (const auto& Pair : Tiles) { if (Pair.Value) { Pair.Value->Destroy(); } }
	Tiles.Empty(); Surf.Empty(); Walls.Empty(); Jin.Empty();
	if (PlayerUnit) { PlayerUnit->Destroy(); }
	if (EnemyUnit) { EnemyUnit->Destroy(); }
	PlayerUnit = nullptr; EnemyUnit = nullptr;
	Charge = FChargeState();
	bOver = false; Round = 1; SelectedCard = -1; bMoveMode = false;
	Refund = 0; bHasLastElement = false; AIGuard = 0;
	LogLines.Empty();
	StartBattle();
}

FString AHexGridManager::Short(const AHexUnit* U)
{
	return (U && U->bPlayerSide) ? TEXT("아군") : TEXT("적");
}

FString AHexGridManager::ElementLabel(ESalanElement E) const
{
	switch (E)
	{
	case ESalanElement::Frost: return TEXT("서리");
	case ESalanElement::Fire:  return TEXT("불");
	case ESalanElement::Water: return TEXT("물");
	case ESalanElement::Wind:  return TEXT("바람");
	case ESalanElement::Stone: return TEXT("돌");
	case ESalanElement::Body:  return TEXT("몸");
	case ESalanElement::Curse: return TEXT("주술");
	default: return TEXT("?");
	}
}

void AHexGridManager::ApplySurface(const FHexCoord& C, ESurface S)
{
	const ESurface Cur = Surf.Contains(C) ? Surf[C] : ESurface::None;

	// 상호작용(설계 캐논)
	if (S == ESurface::Frost && Cur == ESurface::Wet)
	{
		Surf.Add(C, ESurface::Ice);
		Log(FString::Printf(TEXT("젖음+서리 → 얼음판 (%d,%d)"), C.Q, C.R));
		if (AHexTile* T = TileAt(C)) { T->SetSurface(ESurface::Ice); }
		FreezeIfOn(C);
		return;
	}
	if (S == ESurface::Fire && (Cur == ESurface::Wet || Cur == ESurface::Ice))
	{
		Surf.Remove(C);
		Log(TEXT("불이 젖은 자리에서 꺼짐(소화)"));
		if (AHexTile* T = TileAt(C)) { T->SetSurface(ESurface::None); }
		return;
	}
	if (Cur == ESurface::Fire && S == ESurface::Wet)
	{
		Surf.Remove(C);
		Log(TEXT("물이 불을 끔"));
		if (AHexTile* T = TileAt(C)) { T->SetSurface(ESurface::None); }
		return;
	}

	Surf.Add(C, S);
	if (AHexTile* T = TileAt(C)) { T->SetSurface(S); }
}

void AHexGridManager::FreezeIfOn(const FHexCoord& C)
{
	if (AHexUnit* U = UnitAt(C))
	{
		U->Frozen = FMath::Max(U->Frozen, 1);
		U->RefreshVisual();
		Log(FString::Printf(TEXT("%s 발이 얼음판에 얼어붙음"), *Short(U)));
	}
}

void AHexGridManager::DealDamage(AHexUnit* U, int32 Amount, const FString& Tag)
{
	if (!U) { return; }
	int32 A = Amount;
	if (U->bExposed) { A = FMath::RoundToInt(A * 1.5f); }
	if (U->Shield > 0)
	{
		const int32 Ab = FMath::Min(U->Shield, A);
		U->Shield -= Ab; A -= Ab;
		if (Ab > 0) { Log(FString::Printf(TEXT("%s 방패가 %d 막음"), *Short(U), Ab)); }
	}
	U->HP = FMath::Max(0, U->HP - A);
	Log(FString::Printf(TEXT("%s에 %d 피해 %s%s → HP %d"), *Short(U), A, *Tag, U->bExposed ? TEXT(" (영창 무방비!)") : TEXT(""), U->HP));
	if (A > 0) { U->PlayHitAnim(); } // 피격 반응 연출
	U->RefreshVisual();
	if (U->HP <= 0)
	{
		bOver = true;
		Log(FString::Printf(TEXT("★ %s 쓰러짐 — %s"), *Short(U), U->bPlayerSide ? TEXT("패배") : TEXT("승리")));
	}
}

void AHexGridManager::ApplyCC(AHexUnit* U, int32 SlowDur, int32 FreezeDur)
{
	if (!U) { return; }
	if (SlowDur > 0) { U->Slow = FMath::Max(U->Slow, SlowDur); Log(FString::Printf(TEXT("%s 걸음 느려짐(%d턴)"), *Short(U), SlowDur)); }
	if (FreezeDur > 0) { U->Frozen = FMath::Max(U->Frozen, FreezeDur); Log(FString::Printf(TEXT("%s 얼어붙음(%d턴)"), *Short(U), FreezeDur)); }
	U->RefreshVisual();
}

void AHexGridManager::CheckJin(AHexUnit* U)
{
	if (!U) { return; }
	FJinData* J = Jin.Find(U->Coord);
	if (J && J->bOwnerPlayer != U->bPlayerSide)
	{
		Log(FString::Printf(TEXT("★ %s이(가) %s을(를) 밟아 진이 깼다"), *Short(U), *J->Name));
		if (J->TrapDamage > 0) { DealDamage(U, J->TrapDamage, FString::Printf(TEXT("[%s]"), *J->Name)); }
		ApplyCC(U, J->SlowDur, J->FreezeDur);
		Jin.Remove(U->Coord);
		if (AHexTile* T = TileAt(U->Coord)) { T->bJin = false; T->RefreshVisual(); }
	}
}

bool AHexGridManager::IsChainCard(const FSalanCard& Card) const
{
	const bool bElemental = Card.Element != ESalanElement::Body && Card.Element != ESalanElement::Curse;
	return bElemental && (Card.Type == ECardType::Strike || Card.Type == ECardType::Surface);
}

void AHexGridManager::ChainRefund(ESalanElement El, bool bPlayer)
{
	if (bHasLastElement && El == LastElement && Refund < 4)
	{
		Kan++; Refund++;
		if (bPlayer) { Log(FString::Printf(TEXT("연쇄 환급 +1 칸 (%d/4) — 같은 계열 이어 감"), Refund)); }
	}
	LastElement = El; bHasLastElement = true;
}

bool AHexGridManager::CardNeedsTarget(const FSalanCard& Card)
{
	switch (Card.Type)
	{
	case ECardType::Strike: case ECardType::GreatSpell: case ECardType::Surface:
	case ECardType::Push: case ECardType::WindSweep: case ECardType::Control:
	case ECardType::Move: case ECardType::Disarm: case ECardType::JinTrap:
		return true;
	case ECardType::Wall:
		return !Card.bWallRing; // 둘레 벽은 자기중심
	default:
		return false;
	}
}

bool AHexGridManager::CanStartCard(const FSalanCard& Card) const
{
	if (bOver || !bPlayerTurn || (PlayerUnit && PlayerUnit->Frozen > 0)) { return false; }
	if (Charge.bActive) { return false; }
	if (Card.Cast >= 4) { return Kan > 0; } // 대주문=얹기 시작
	return Kan >= Card.Cast;
}

bool AHexGridManager::TargetOK(const FSalanCard& Card, const FHexCoord& Target, const FHexCoord& From, FString& OutMsg) const
{
	const int32 Rng = (Card.Type == ECardType::Move) ? Card.MoveRange : Card.Range;
	if (SalanHex::Dist(From, Target) > Rng) { OutMsg = TEXT("사거리 밖"); return false; }

	// 과녁=눈: 소리길 직격/제어는 시야 필요. 매개길(through)·몸길은 무시.
	const bool bLosCard = (Card.Type == ECardType::Strike || Card.Type == ECardType::GreatSpell || Card.Type == ECardType::Control);
	if (bLosCard && Card.Path == ESalanPath::Sori && !Card.bThrough)
	{
		if (!HasLoS(From, Target)) { OutMsg = TEXT("벽에 시야가 막힘 — 언령은 안 보이면 못 건다 (주술은 우회)"); return false; }
	}
	OutMsg.Reset();
	return true;
}

void AHexGridManager::ResolveCard(const FSalanCard& Card, const FHexCoord& Target, bool bActorPlayer)
{
	AHexUnit* Me = bActorPlayer ? PlayerUnit : EnemyUnit;
	AHexUnit* Foe = bActorPlayer ? EnemyUnit : PlayerUnit;
	if (!Me) { return; }
	Me->PlayCastAnim(); // 시전 동작(살란 발현) 연출

	switch (Card.Type)
	{
	case ECardType::Move:
	{
		Me->Coord = Target; RefreshUnitWorld(Me);
		Log(FString::Printf(TEXT("%s 스무 걸음 이동"), *Short(Me)));
		if (Surf.Contains(Target) && Surf[Target] == ESurface::Ice) { FreezeIfOn(Target); }
		CheckJin(Me);
		return;
	}
	case ECardType::Defend:
	{
		Me->Shield += Card.Shield; Me->RefreshVisual();
		Log(FString::Printf(TEXT("%s 굳은 살갗 방패 +%d"), *Short(Me), Card.Shield));
		return;
	}
	case ECardType::Interrupt:
	{
		if (Charge.bActive && Charge.bWhoPlayer != bActorPlayer)
		{
			const FSalanCard& CC = Cards[Charge.CardIndex];
			const bool bInterruptible = (CC.Path == ESalanPath::Sori || CC.Path == ESalanPath::Grim);
			if (bInterruptible)
			{
				Log(FString::Printf(TEXT("★ 말문 막기! %s의 %s 영창이 끊겼다 (칸 %d 날림)"), *Short(Foe), *CC.Name, Charge.Acc));
				if (Foe) { Foe->bExposed = false; Foe->RefreshVisual(); }
				Charge = FChargeState();
			}
			else { Log(FString::Printf(TEXT("%s은(는) 그 길이라 말로 못 끊는다"), *CC.Name)); }
		}
		else { Log(TEXT("끊을 영창이 없다")); }
		return;
	}
	case ECardType::Wall:
	{
		if (Card.bWallRing)
		{
			for (const FHexCoord& D : SalanHex::DIRS)
			{
				const FHexCoord P(Me->Coord.Q + D.Q, Me->Coord.R + D.R);
				if (InField(P) && !UnitAt(P)) { Walls.Add(P, Card.WallDur); }
			}
			Log(TEXT("얼음 성벽: 둘레에 벽"));
		}
		else
		{
			Walls.Add(Target, Card.WallDur);
			Log(FString::Printf(TEXT("%s 설치 (%d,%d)"), Card.Path == ESalanPath::Grim ? TEXT("돌벽 진") : TEXT("돌벽"), Target.Q, Target.R));
		}
		RefreshTileFlags();
		return;
	}
	case ECardType::JinTrap:
	{
		FJinData J; J.bOwnerPlayer = bActorPlayer; J.FreezeDur = Card.FreezeDur; J.SlowDur = Card.SlowDur; J.TrapDamage = Card.TrapDamage; J.Name = Card.Name;
		Jin.Add(Target, J);
		Log(FString::Printf(TEXT("%s %s 선새김 — (%d,%d) 잠든 진. 적이 밟으면 깬다"), *Short(Me), *Card.Name, Target.Q, Target.R));
		RefreshTileFlags();
		return;
	}
	case ECardType::WindSweep:
	{
		const ESurface Cur = Surf.Contains(Target) ? Surf[Target] : ESurface::None;
		if (Cur == ESurface::Fire) { Surf.Add(Target, ESurface::SpreadFire); if (AHexTile* T = TileAt(Target)) { T->SetSurface(ESurface::SpreadFire); } Log(TEXT("불+바람 → 번짐불(확산)")); }
		else if (Cur != ESurface::None) { Surf.Remove(Target); if (AHexTile* T = TileAt(Target)) { T->SetSurface(ESurface::None); } Log(TEXT("바람이 표면을 걷음")); }
		else { Log(TEXT("바람 — 걷을 표면 없음")); }
		return;
	}
	case ECardType::Push:
	{
		AHexUnit* Tgt = UnitAt(Target);
		if (Tgt)
		{
			const int32 dq = FMath::Sign(Tgt->Coord.Q - Me->Coord.Q);
			const int32 dr = FMath::Sign(Tgt->Coord.R - Me->Coord.R);
			FHexCoord N = Tgt->Coord;
			for (int32 i = 0; i < Card.Knock; ++i)
			{
				const FHexCoord Cand(Tgt->Coord.Q + dq * (i + 1), Tgt->Coord.R + dr * (i + 1));
				if (InField(Cand) && !UnitAt(Cand) && !Walls.Contains(Cand)) { N = Cand; }
			}
			Tgt->Coord = N; RefreshUnitWorld(Tgt);
			Log(FString::Printf(TEXT("몰이 바람: %s 밀려남"), *Short(Tgt)));
			CheckJin(Tgt);
			if (Surf.Contains(N) && Surf[N] == ESurface::Ice) { FreezeIfOn(N); }
		}
		return;
	}
	case ECardType::Disarm:
	{
		AHexUnit* U = UnitAt(Target);
		if (U && U->bPlayerSide != bActorPlayer) { Log(FString::Printf(TEXT("%s 무장 떨굼 — 다음 근접 무력"), *Short(U))); }
		return;
	}
	case ECardType::Surface:
	{
		TArray<FHexCoord> Ts;
		if (Card.Aoe > 0) { for (const auto& Pair : Tiles) { if (SalanHex::Dist(Target, Pair.Key) <= Card.Aoe) { Ts.Add(Pair.Key); } } }
		else { Ts.Add(Target); }
		for (const FHexCoord& Tc : Ts) { ApplySurface(Tc, Card.Leaves); }
		return;
	}
	case ECardType::Control:
	{
		AHexUnit* U = UnitAt(Target);
		if (U && U->bPlayerSide != bActorPlayer) { ApplyCC(U, Card.SlowDur, Card.FreezeDur); }
		return;
	}
	case ECardType::Strike:
	{
		TArray<FHexCoord> Ts;
		if (Card.Aoe > 0) { for (const auto& Pair : Tiles) { if (SalanHex::Dist(Target, Pair.Key) <= Card.Aoe) { Ts.Add(Pair.Key); } } }
		else { Ts.Add(Target); }
		for (const FHexCoord& Tc : Ts)
		{
			AHexUnit* U = UnitAt(Tc);
			int32 Dmg = Card.Damage;
			if (U && U->bPlayerSide != bActorPlayer)
			{
				bool bHit = false;
				const ESurface Here = Surf.Contains(Tc) ? Surf[Tc] : ESurface::None;
				if (Card.ComboBonus > 0 && Card.ComboSurf.Contains(Here)) { Dmg = Card.ComboBonus; bHit = true; }
				DealDamage(U, Dmg, FString::Printf(TEXT("[%s]%s"), *Card.Name, bHit ? TEXT(" 합!") : TEXT("")));
			}
			if (Card.Leaves != ESurface::None) { ApplySurface(Tc, Card.Leaves); }
		}
		return;
	}
	case ECardType::GreatSpell:
	{
		if (Card.GreatKind == 1) // 내려앉는 겨울
		{
			for (const auto& Pair : Tiles)
			{
				if (SalanHex::Dist(Target, Pair.Key) > Card.Aoe) { continue; }
				const FHexCoord Tc = Pair.Key;
				const ESurface Cur = Surf.Contains(Tc) ? Surf[Tc] : ESurface::None;
				if (Cur == ESurface::Wet || Cur == ESurface::Ice) { ApplySurface(Tc, ESurface::Frost); }
				else if (Cur == ESurface::Fire || Cur == ESurface::SpreadFire) { Surf.Remove(Tc); if (AHexTile* T = TileAt(Tc)) { T->SetSurface(ESurface::None); } }
				else { ApplySurface(Tc, ESurface::Frost); }
				AHexUnit* U = UnitAt(Tc);
				if (U && U->bPlayerSide != bActorPlayer) { ApplyCC(U, 2, 0); DealDamage(U, 60, TEXT("[내려앉는 겨울]")); }
			}
			Log(TEXT("★ 내려앉는 겨울 — 판에 겨울이 앉았다(서리·얼음판·둔화)"));
		}
		else // 직격 대주문(불지옥)
		{
			TArray<FHexCoord> Ts;
			if (Card.Aoe > 0) { for (const auto& Pair : Tiles) { if (SalanHex::Dist(Target, Pair.Key) <= Card.Aoe) { Ts.Add(Pair.Key); } } }
			else { Ts.Add(Target); }
			for (const FHexCoord& Tc : Ts)
			{
				AHexUnit* U = UnitAt(Tc);
				int32 Dmg = Card.Damage;
				if (U && U->bPlayerSide != bActorPlayer)
				{
					bool bHit = false;
					const ESurface Here = Surf.Contains(Tc) ? Surf[Tc] : ESurface::None;
					if (Card.ComboBonus > 0 && Card.ComboSurf.Contains(Here)) { Dmg = Card.ComboBonus; bHit = true; }
					DealDamage(U, Dmg, FString::Printf(TEXT("[%s]%s"), *Card.Name, bHit ? TEXT(" 합!") : TEXT("")));
				}
				if (Card.Leaves != ESurface::None) { ApplySurface(Tc, Card.Leaves); }
			}
		}
		return;
	}
	default: return;
	}
}

void AHexGridManager::PlayCard(int32 Index, const FHexCoord& Target)
{
	if (!Cards.IsValidIndex(Index) || !PlayerUnit) { return; }
	const FSalanCard& Card = Cards[Index];

	FString Msg;
	if (!TargetOK(Card, Target, PlayerUnit->Coord, Msg)) { Log(Msg); return; }

	if (Card.Cast >= 4) // 대주문 = 얹기 시작
	{
		const int32 Put = Kan; Kan = 0;
		Charge = FChargeState(); Charge.bActive = true; Charge.bWhoPlayer = true; Charge.CardIndex = Index; Charge.Target = Target; Charge.Acc = Put; Charge.Need = Card.Cast;
		PlayerUnit->bExposed = true; PlayerUnit->RefreshVisual();
		Log(FString::Printf(TEXT("%s 영창 시작 — 칸 %d/%d 얹음. 완성까지 무방비(자리 값). 적이 말문 막기로 끊을 수 있다"), *Card.Name, Put, Charge.Need));
		Deselect();
		return;
	}
	if (Kan < Card.Cast) { Log(TEXT("칸 부족")); return; }
	Kan -= Card.Cast;
	if (IsChainCard(Card)) { ChainRefund(Card.Element, true); }
	ResolveCard(Card, Target, true);
	SelectedCard = -1; bMoveMode = false;
	if (Kan <= 0) { Log(TEXT("칸 소진 — 턴을 넘기세요")); }
	UpdateHighlights();
	Changed();
}

// ─────────────────────────────── 입력 API ───────────────────────────────
void AHexGridManager::SelectCard(int32 Index)
{
	if (!Cards.IsValidIndex(Index)) { return; }
	const FSalanCard& Card = Cards[Index];
	if (!CanStartCard(Card)) { Log(TEXT("칸이 부족하거나 지금 낼 수 없음")); return; }
	SelectedCard = Index; bMoveMode = false;
	if (!CardNeedsTarget(Card) && PlayerUnit)
	{
		PlayCard(Index, PlayerUnit->Coord); // 자기중심(둘레 벽·방어·자원 등)
	}
	UpdateHighlights();
	Changed();
}

void AHexGridManager::EnterMoveMode()
{
	if (bOver || !bPlayerTurn) { return; }
	bMoveMode = true; SelectedCard = -1;
	UpdateHighlights();
	Changed();
}

void AHexGridManager::Deselect()
{
	SelectedCard = -1; bMoveMode = false;
	UpdateHighlights();
	Changed();
}

void AHexGridManager::ClickCoord(const FHexCoord& C)
{
	if (bOver || !bPlayerTurn) { return; }

	// 자기 유닛 클릭 → 이동 모드
	if (PlayerUnit && C == PlayerUnit->Coord && SelectedCard < 0)
	{
		EnterMoveMode();
		return;
	}

	if (SelectedCard >= 0)
	{
		const FSalanCard& Card = Cards[SelectedCard];
		if (Card.Type == ECardType::Move)
		{
			if (SalanHex::Dist(PlayerUnit->Coord, C) <= Card.MoveRange && !UnitAt(C) && !Walls.Contains(C)) { PlayCard(SelectedCard, C); }
			else { Log(TEXT("이동 불가")); }
			return;
		}
		PlayCard(SelectedCard, C);
		return;
	}

	if (bMoveMode && PlayerUnit)
	{
		if (Kan < 1) { Log(TEXT("이동도 칸 1이 든다 — 칸 부족")); return; }
		if (SalanHex::Dist(PlayerUnit->Coord, C) <= 2 && !UnitAt(C) && !Walls.Contains(C) && PlayerUnit->Frozen <= 0)
		{
			const int32 D = SalanHex::Dist(PlayerUnit->Coord, C);
			PlayerUnit->Coord = C; RefreshUnitWorld(PlayerUnit); Kan -= 1;
			Log(FString::Printf(TEXT("아군 이동(%d칸 거리, 칸 1)"), D));
			CheckJin(PlayerUnit);
			bMoveMode = false; UpdateHighlights(); Changed();
		}
		else { Log(TEXT("이동 불가")); }
		return;
	}
}

void AHexGridManager::RequestEndTurn()
{
	if (bOver || !bPlayerTurn) { return; }
	EndPlayerTurn();
}

// ─────────────────────────────── 턴 진행 ───────────────────────────────
void AHexGridManager::StartTurn(bool bPlayer)
{
	bPlayerTurn = bPlayer;
	Kan = KanMax; Refund = 0; bHasLastElement = false;
	AHexUnit* U = bPlayer ? PlayerUnit : EnemyUnit;
	if (U)
	{
		if (U->Slow > 0) { Kan = FMath::Max(1, KanMax - 2); U->Slow--; }
		if (U->Frozen > 0) { U->Frozen--; }
		U->RefreshVisual();
	}
}

void AHexGridManager::ResolveOwnCharge(bool bPlayer)
{
	if (!Charge.bActive || Charge.bWhoPlayer != bPlayer) { return; }
	AHexUnit* Me = bPlayer ? PlayerUnit : EnemyUnit;
	const int32 Put = FMath::Min(Kan, Charge.Need - Charge.Acc);
	Kan -= Put; Charge.Acc += Put;
	if (Charge.Acc >= Charge.Need)
	{
		if (Me) { Me->bExposed = false; Me->RefreshVisual(); }
		const FSalanCard& Card = Cards[Charge.CardIndex];
		Log(FString::Printf(TEXT("★ %s 영창 완성!"), *Card.Name));
		FString Msg;
		if (Me && TargetOK(Card, Charge.Target, Me->Coord, Msg)) { ResolveCard(Card, Charge.Target, bPlayer); }
		else { Log(TEXT("완성했으나 과녁을 잃음(이동/시야)")); }
		Charge = FChargeState();
	}
	else
	{
		const FSalanCard& Card = Cards[Charge.CardIndex];
		Log(FString::Printf(TEXT("%s 계속 얹는 중 %d/%d (여전히 무방비)"), *Card.Name, Charge.Acc, Charge.Need));
	}
}

void AHexGridManager::SurfaceTick()
{
	// 번짐불 확산 + 도트
	TArray<FHexCoord> Spread;
	TArray<FHexCoord> Keys; Surf.GetKeys(Keys);
	for (const FHexCoord& K : Keys)
	{
		if (Surf[K] == ESurface::SpreadFire)
		{
			if (AHexUnit* U = UnitAt(K)) { DealDamage(U, 40, TEXT("[번짐불 도트]")); }
			for (const FHexCoord& D : SalanHex::DIRS)
			{
				const FHexCoord P(K.Q + D.Q, K.R + D.R);
				if (InField(P) && !Surf.Contains(P) && FMath::FRand() < 0.5f) { Spread.Add(P); }
			}
		}
	}
	for (const FHexCoord& K : Spread) { ApplySurface(K, ESurface::SpreadFire); }

	// 벽 수명
	TArray<FHexCoord> WKeys; Walls.GetKeys(WKeys);
	for (const FHexCoord& K : WKeys)
	{
		Walls[K]--;
		if (Walls[K] <= 0) { Walls.Remove(K); }
	}
	RefreshTileFlags();
}

void AHexGridManager::StartPlayerTurn()
{
	Round++;
	StartTurn(true);
	ResolveOwnCharge(true);
	UpdateHighlights();
	Changed();
}

void AHexGridManager::EndPlayerTurn()
{
	SurfaceTick();
	if (bOver) { Changed(); return; }
	AIBegin();
}

void AHexGridManager::AIBegin()
{
	StartTurn(false);
	ResolveOwnCharge(false);
	AIGuard = 0;
	Changed();
	if (!EnemyUnit || !PlayerUnit || bOver) { FinishAI(); return; }
	ScheduleAIStep();
}

void AHexGridManager::ScheduleAIStep()
{
	GetWorldTimerManager().SetTimer(AITimer, this, &AHexGridManager::AIStep, 0.55f, false);
}

void AHexGridManager::AIStep()
{
	if (bOver) { Changed(); return; }
	if (!EnemyUnit || !PlayerUnit) { FinishAI(); return; }

	AIGuard++;
	bool bActed = false;
	if (Kan > 0 && AIGuard < 14) { bActed = DoOneEnemyAction(); }
	Changed();

	if (bOver) { return; }
	if (bActed && Kan > 0 && AIGuard < 14) { ScheduleAIStep(); }
	else { FinishAI(); }
}

// 적의 한 수(행동 하나). 했으면 true, 할 게 없으면 false. 웹 v0.2 AI 로직과 동일.
bool AHexGridManager::DoOneEnemyAction()
{
	// 1) 플레이어가 언령/진 영창 무방비면: 말문 막기 우선
	if (Charge.bActive && Charge.bWhoPlayer)
	{
		const FSalanCard& CC = Cards[Charge.CardIndex];
		const bool bInterruptible = (CC.Path == ESalanPath::Sori || CC.Path == ESalanPath::Grim);
		if (bInterruptible)
		{
			int32 PIdx = INDEX_NONE;
			for (int32 i = 0; i < Cards.Num(); ++i) { if (Cards[i].Type == ECardType::Interrupt) { PIdx = i; break; } }
			if (PIdx != INDEX_NONE && SalanHex::Dist(EnemyUnit->Coord, PlayerUnit->Coord) <= Cards[PIdx].Range && HasLoS(EnemyUnit->Coord, PlayerUnit->Coord))
			{
				Kan -= 1; ResolveCard(Cards[PIdx], PlayerUnit->Coord, false);
				return true;
			}
		}
	}

	const int32 D = SalanHex::Dist(EnemyUnit->Coord, PlayerUnit->Coord);

	// 2) 접근(칸 1)
	if (D > 5)
	{
		FHexCoord Best = EnemyUnit->Coord; int32 BestD = D; bool bFound = false;
		for (const FHexCoord& Dir : SalanHex::DIRS)
		{
			const FHexCoord P(EnemyUnit->Coord.Q + Dir.Q, EnemyUnit->Coord.R + Dir.R);
			if (InField(P) && !UnitAt(P) && !Walls.Contains(P))
			{
				const int32 Nd = SalanHex::Dist(P, PlayerUnit->Coord);
				if (Nd < BestD) { BestD = Nd; Best = P; bFound = true; }
			}
		}
		if (bFound)
		{
			EnemyUnit->Coord = Best; RefreshUnitWorld(EnemyUnit); Kan -= 1; CheckJin(EnemyUnit);
			Log(TEXT("적 접근 (칸 1)"));
			return true;
		}
	}

	// 3) 적 직격(시야 있을 때) — 레벨별 대표 계열 카드(불덩이/돌팔매/물송곳/불비/돌창…)
	if (D <= 5 && HasLoS(EnemyUnit->Coord, PlayerUnit->Coord))
	{
		const FName StrikeId = CurrentLevel().EnemyStrike;
		int32 FIdx = INDEX_NONE;
		for (int32 i = 0; i < Cards.Num(); ++i) { if (Cards[i].Id == StrikeId) { FIdx = i; break; } }
		if (FIdx == INDEX_NONE) { for (int32 i = 0; i < Cards.Num(); ++i) { if (Cards[i].Name == TEXT("불덩이")) { FIdx = i; break; } } }
		if (FIdx != INDEX_NONE && Kan >= Cards[FIdx].Cast)
		{
			Kan -= Cards[FIdx].Cast;
			if (IsChainCard(Cards[FIdx])) { ChainRefund(Cards[FIdx].Element, false); }
			ResolveCard(Cards[FIdx], PlayerUnit->Coord, false);
			return true;
		}
	}

	if (D <= 5 && !HasLoS(EnemyUnit->Coord, PlayerUnit->Coord)) { Log(TEXT("적: 시야 막힘 — 돌아갈 칸 없음")); }
	return false;
}

void AHexGridManager::FinishAI()
{
	SurfaceTick();
	if (bOver) { Changed(); return; }
	StartPlayerTurn();
}

// ─────────────────────────────── 비주얼 ───────────────────────────────
void AHexGridManager::RefreshUnitWorld(AHexUnit* U)
{
	if (U) { U->MoveToWorld(WorldOfCoord(U->Coord)); } // 지면 위치 전달(유닛이 오프셋 처리)
}

void AHexGridManager::RefreshTileFlags()
{
	for (const auto& Pair : Tiles)
	{
		AHexTile* T = Pair.Value;
		if (!T) { continue; }
		const bool bW = Walls.Contains(Pair.Key);
		const bool bJ = Jin.Contains(Pair.Key);
		if (T->bWall != bW || T->bJin != bJ) { T->bWall = bW; T->bJin = bJ; T->RefreshVisual(); }
	}
}

void AHexGridManager::UpdateHighlights()
{
	const bool bSel = (SelectedCard >= 0 || bMoveMode) && !bOver && PlayerUnit;
	for (const auto& Pair : Tiles)
	{
		AHexTile* T = Pair.Value;
		if (!T) { continue; }
		bool bOn = false, bBlocked = false;
		if (bSel)
		{
			if (SelectedCard >= 0)
			{
				const FSalanCard& C = Cards[SelectedCard];
				const int32 Rng = (C.Type == ECardType::Move) ? C.MoveRange : C.Range;
				if (Rng > 0 && SalanHex::Dist(PlayerUnit->Coord, Pair.Key) <= Rng)
				{
					bOn = true;
					const bool bLosCard = (C.Type == ECardType::Strike || C.Type == ECardType::GreatSpell || C.Type == ECardType::Control);
					if (bLosCard && C.Path == ESalanPath::Sori && !C.bThrough && !HasLoS(PlayerUnit->Coord, Pair.Key)) { bBlocked = true; }
				}
			}
			else if (bMoveMode)
			{
				if (SalanHex::Dist(PlayerUnit->Coord, Pair.Key) <= 2 && !UnitAt(Pair.Key)) { bOn = true; }
			}
		}
		T->SetHighlight(bOn, bBlocked);
	}
}

void AHexGridManager::Changed()
{
	OnChanged.Broadcast();
}
