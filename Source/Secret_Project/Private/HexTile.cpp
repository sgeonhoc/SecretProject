#include "HexTile.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AHexTile::AHexTile()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);

	// 엔진 기본 실린더를 낮게 눌러 원반형 타일로. 커스텀 헥스 메시는 나중에 BP로 교체 가능.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylFinder.Succeeded())
	{
		MeshComp->SetStaticMesh(CylFinder.Object);
	}
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
}

void AHexTile::Setup(const FHexCoord& InCoord)
{
	Coord = InCoord;

	// 머티리얼 인스턴스(틴트 시도용). BasicShapeMaterial에 Color 파라미터가 없으면 무해하게 무시됨.
	UMaterialInterface* Base = MeshComp->GetMaterial(0);
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (Base) { MeshComp->SetMaterial(0, Base); }
	}
	if (Base)
	{
		MID = UMaterialInstanceDynamic::Create(Base, this);
		MeshComp->SetMaterial(0, MID);
	}
	RefreshVisual();
}

void AHexTile::SetSurface(ESurface In)
{
	Surface = In;
	RefreshVisual();
}

void AHexTile::SetHighlight(bool bOn, bool bBlocked)
{
	bHighlight = bOn;
	bHighlightBlocked = bBlocked;
	RefreshVisual();
}

FLinearColor AHexTile::SurfaceColor(ESurface S)
{
	switch (S)
	{
	case ESurface::Wet:        return FLinearColor(0.30f, 0.52f, 0.77f);
	case ESurface::Fire:       return FLinearColor(1.00f, 0.48f, 0.33f);
	case ESurface::Frost:      return FLinearColor(0.50f, 0.83f, 1.00f);
	case ESurface::Ice:        return FLinearColor(0.75f, 0.91f, 1.00f);
	case ESurface::Dust:       return FLinearColor(0.72f, 0.59f, 0.41f);
	case ESurface::SpreadFire: return FLinearColor(1.00f, 0.35f, 0.16f);
	default:                   return FLinearColor(0.07f, 0.09f, 0.12f);
	}
}

void AHexTile::RefreshVisual()
{
	FLinearColor C = SurfaceColor(Surface);
	if (bWall) { C = FLinearColor(0.35f, 0.42f, 0.48f); }
	FLinearColor Emissive = FLinearColor::Black;
	if (bHighlight)
	{
		// 유효 대상=금빛, 막힘(시야 없음)=붉은. 40% 은은한 색조는 전장에서 잘 안 보여 → 55% 블렌드 + 발광으로 또렷하게.
		const FLinearColor H = bHighlightBlocked ? FLinearColor(0.72f, 0.15f, 0.13f) : FLinearColor(0.85f, 0.72f, 0.28f);
		C = C * 0.45f + H * 0.55f;
		Emissive = H * (bHighlightBlocked ? 0.25f : 0.45f); // 그림자 속에서도 칸이 살짝 빛나 보이게
	}
	if (MID)
	{
		// 흔한 파라미터 이름들에 모두 시도(있는 것만 반영).
		MID->SetVectorParameterValue(TEXT("Color"), C);
		MID->SetVectorParameterValue(TEXT("BaseColor"), C);
		MID->SetVectorParameterValue(TEXT("Emissive"), Emissive);
		MID->SetVectorParameterValue(TEXT("EmissiveColor"), Emissive);
	}
	OnVisualUpdate(Surface, bWall, bJin, bHighlight);
}
