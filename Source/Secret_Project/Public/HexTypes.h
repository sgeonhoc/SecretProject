#pragma once

#include "CoreMinimal.h"
#include "HexTypes.generated.h"

/**
 * 헥스 칸/살란 전투 — 공용 타입 (웹 시뮬 전투시뮬.html v0.2를 UE로 이식하는 정본 골격).
 * 계열=부름말(서리/불/물/바람/돌/몸/주술), 길=발현방식(소리/몸/그림/매개).
 * 코스트 단위는 오직 "칸"(행동 슬롯). 큰 영창은 여러 칸을 부어 완성하며 그동안 무방비.
 * 로직은 전부 C++, 타일·유닛 비주얼(머티리얼/메시)은 BP로 다듬는다(프로젝트 컨벤션).
 */

// 계열(부름말) — 살란 정본 §5. 페르소나식 EBattleElement와 별개(어휘가 다름).
UENUM(BlueprintType)
enum class ESalanElement : uint8
{
	Frost UMETA(DisplayName = "서리"),
	Fire  UMETA(DisplayName = "불"),
	Water UMETA(DisplayName = "물"),
	Wind  UMETA(DisplayName = "바람"),
	Stone UMETA(DisplayName = "돌"),
	Body  UMETA(DisplayName = "몸"),
	Curse UMETA(DisplayName = "주술")
};

// 길(발현방식) — 회로 정본 §2. 길마다 문법(정체성)이 다르다.
UENUM(BlueprintType)
enum class ESalanPath : uint8
{
	Sori UMETA(DisplayName = "소리길(언령)"),   // 과녁=눈(LoS 필요) · 영창은 말문 막기로 끊김
	Mom  UMETA(DisplayName = "몸길(연공)"),      // 근접 · 즉발 · 안 끊김(잠들지 않음)
	Grim UMETA(DisplayName = "그림길(진·술식)"), // 선새김(진을 깔고 밟으면 발동) · 설치
	Mae  UMETA(DisplayName = "매개길(주술)")     // 인연으로 벽 뒤 우회(LoS 무시) · 안 끊김
};

// 표면(설계 캐논) — 타일 하나에 하나. 상호작용: 젖음+서리→얼음판 / 불+젖음→소화 / 불+바람→번짐불.
UENUM(BlueprintType)
enum class ESurface : uint8
{
	None       UMETA(DisplayName = "없음"),
	Wet        UMETA(DisplayName = "젖음"),
	Fire       UMETA(DisplayName = "불"),
	Frost      UMETA(DisplayName = "서리"),
	Ice        UMETA(DisplayName = "얼음판"),
	Dust       UMETA(DisplayName = "돌먼지"),
	SpreadFire UMETA(DisplayName = "번짐불")
};

// 카드 종류 — 웹 시뮬 카드 type 매핑.
UENUM(BlueprintType)
enum class ECardType : uint8
{
	Strike     UMETA(DisplayName = "직격"),
	Control    UMETA(DisplayName = "제어"),
	Surface    UMETA(DisplayName = "표면"),
	Move       UMETA(DisplayName = "이동"),
	Wall       UMETA(DisplayName = "벽"),
	JinTrap    UMETA(DisplayName = "선새김진"),
	Disarm     UMETA(DisplayName = "무장떨굼"),
	Push       UMETA(DisplayName = "밀침"),
	WindSweep  UMETA(DisplayName = "바람"),
	Defend     UMETA(DisplayName = "방어"),
	GreatSpell UMETA(DisplayName = "대주문"),
	Interrupt  UMETA(DisplayName = "말문막기")
};

// 축 좌표(axial). pointy-top. TMap 키로 쓰기 위해 GetTypeHash/== 제공.
USTRUCT(BlueprintType)
struct FHexCoord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Hex")
	int32 Q = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Hex")
	int32 R = 0;

	FHexCoord() {}
	FHexCoord(int32 InQ, int32 InR) : Q(InQ), R(InR) {}

	bool operator==(const FHexCoord& O) const { return Q == O.Q && R == O.R; }
	bool operator!=(const FHexCoord& O) const { return !(*this == O); }
};

FORCEINLINE uint32 GetTypeHash(const FHexCoord& C)
{
	return HashCombine(::GetTypeHash(C.Q), ::GetTypeHash(C.R));
}

// 카드 정의 — "기술=곧 카드"(정본 §5). 값 단위는 칸.
USTRUCT(BlueprintType)
struct FSalanCard
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Card") FName Id;
	UPROPERTY(BlueprintReadOnly, Category = "Card") FString Name;       // 정본 기술 이름(얼음화살 등)
	UPROPERTY(BlueprintReadOnly, Category = "Card") ESalanElement Element = ESalanElement::Frost;
	UPROPERTY(BlueprintReadOnly, Category = "Card") ESalanPath Path = ESalanPath::Sori;
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 Tier = 3;      // 그 바탕 길의 T
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 Cast = 1;      // 완성 칸(1자잘/2주력/3대형/4~대주문)
	UPROPERTY(BlueprintReadOnly, Category = "Card") ECardType Type = ECardType::Strike;
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 Range = 5;
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 Damage = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 ComboBonus = 0;   // 합 시 대체 데미지
	UPROPERTY(BlueprintReadOnly, Category = "Card") TArray<ESurface> ComboSurf; // 이 표면 위면 합
	UPROPERTY(BlueprintReadOnly, Category = "Card") ESurface Leaves = ESurface::None; // 남기는 표면
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 Aoe = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 MoveRange = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 Shield = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 Knock = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 WallDur = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Card") bool bWallRing = false;  // 둘레 벽
	UPROPERTY(BlueprintReadOnly, Category = "Card") bool bThrough = false;   // 벽 뒤 우회(매개길)
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 SlowDur = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 FreezeDur = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 TrapDamage = 0;    // 선새김 진 발동 데미지
	UPROPERTY(BlueprintReadOnly, Category = "Card") int32 GreatKind = 0;     // 0=직격 대주문, 1=겨울
};

// 레벨 지형 표면 한 칸(프리필) — 젖음/돌먼지/불 등을 미리 깔아 무대를 만든다.
USTRUCT()
struct FHexSurfaceCell
{
	GENERATED_BODY()
	UPROPERTY() FHexCoord Coord;
	UPROPERTY() ESurface Surf = ESurface::None;
	FHexSurfaceCell() {}
	FHexSurfaceCell(FHexCoord InC, ESurface InS) : Coord(InC), Surf(InS) {}
};

// 전투 레벨 하나(라셀 현대 배경). 정본=게임_전투레벨_정본.md. 웹 LEVELS와 같은 표.
USTRUCT()
struct FHexLevel
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() FString Stage;         // 무대 이름
	UPROPERTY() FString Flavor;        // 한 줄 배경
	UPROPERTY() FString Objective;     // 목표
	UPROPERTY() int32 Radius = 4;      // 필드 반경(3 좁음/4 보통/5 트임)
	UPROPERTY() FHexCoord PlayerStart = FHexCoord(-3, 3);
	UPROPERTY() FString PlayerName = TEXT("아군");
	UPROPERTY() int32 PlayerHP = 4000;
	UPROPERTY() FHexCoord EnemyStart = FHexCoord(3, -3);
	UPROPERTY() FString EnemyName = TEXT("적");
	UPROPERTY() int32 EnemyHP = 4000;
	UPROPERTY() FName EnemyStrike = TEXT("불덩이");  // 적 AI 대표 직격 카드(계열이 여기서 갈림)
	UPROPERTY() TArray<FHexSurfaceCell> Surfaces;    // 지형 표면 프리필
	UPROPERTY() TArray<FHexCoord> Walls;             // 지형 벽(엄폐·병목) — 안 삭음
};

// ─────────────────────────────────────────────────────────────
// 헥스 수학 (axial, pointy-top) — 웹 시뮬과 동일 공식
// ─────────────────────────────────────────────────────────────
namespace SalanHex
{
	// 이웃 여섯 방향
	static const FHexCoord DIRS[6] = {
		{1,0},{1,-1},{0,-1},{-1,0},{-1,1},{0,1}
	};

	FORCEINLINE int32 Dist(const FHexCoord& A, const FHexCoord& B)
	{
		return (FMath::Abs(A.Q - B.Q) + FMath::Abs(A.Q + A.R - B.Q - B.R) + FMath::Abs(A.R - B.R)) / 2;
	}

	// 헥스 좌표 → 평면 X/Y (uu). Size = 헥스 반지름(uu).
	FORCEINLINE FVector2D ToWorld2D(const FHexCoord& C, float Size)
	{
		const float X = Size * FMath::Sqrt(3.0f) * (C.Q + C.R * 0.5f);
		const float Y = Size * 1.5f * C.R;
		return FVector2D(X, Y);
	}

	// 큐브 반올림(라인 드로용)
	FORCEINLINE FHexCoord CubeRound(float x, float y, float z)
	{
		int32 rx = FMath::RoundToInt(x), ry = FMath::RoundToInt(y), rz = FMath::RoundToInt(z);
		const float dx = FMath::Abs(rx - x), dy = FMath::Abs(ry - y), dz = FMath::Abs(rz - z);
		if (dx > dy && dx > dz) rx = -ry - rz;
		else if (dy > dz)       ry = -rx - rz;
		else                    rz = -rx - ry;
		return FHexCoord(rx, rz);
	}

	// 두 헥스 사이 직선상의 칸들 (과녁=눈 LoS 판정용)
	FORCEINLINE void Line(const FHexCoord& A, const FHexCoord& B, TArray<FHexCoord>& Out)
	{
		Out.Reset();
		const int32 N = Dist(A, B);
		if (N == 0) { Out.Add(A); return; }
		const float ay = float(-A.Q - A.R), by = float(-B.Q - B.R);
		for (int32 i = 0; i <= N; ++i)
		{
			const float t = float(i) / float(N);
			const float x = A.Q + (B.Q - A.Q) * t;
			const float z = A.R + (B.R - A.R) * t;
			const float y = ay + (by - ay) * t;
			Out.Add(CubeRound(x, y, z));
		}
	}
}
