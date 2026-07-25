#include "HexUnit.h"
#include "ABaseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetMathLibrary.h"

AHexUnit::AHexUnit()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylFinder.Succeeded())
	{
		MeshComp->SetStaticMesh(CylFinder.Object);
	}
	// 유닛은 캐릭터처럼 서 있는 기둥꼴: 얇고 키 크게(스탠딩 느낌)
	MeshComp->SetRelativeScale3D(FVector(0.5f, 0.5f, 2.4f));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
}

void AHexUnit::Setup(bool bInPlayer, const FString& InName, int32 InMaxHP)
{
	bPlayerSide = bInPlayer;
	DisplayName = InName;
	MaxHP = InMaxHP;
	HP = InMaxHP;

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base) { Base = MeshComp->GetMaterial(0); }
	if (Base)
	{
		MID = UMaterialInstanceDynamic::Create(Base, this);
		MeshComp->SetMaterial(0, MID);
	}
	RefreshVisual();
}

void AHexUnit::MoveToWorld(const FVector& GroundLoc)
{
	// 원기둥(플레이스홀더)은 지면 위에 서게
	SetActorLocation(GroundLoc + FVector(0, 0, 120));
	// 실제 캐릭터가 있으면 지면에 세운다(캡슐 반높이 ~90)
	if (CharacterActor) { CharacterActor->SetActorLocation(GroundLoc + FVector(0, 0, 90)); }
}

void AHexUnit::SetCharacter(AActor* InChar)
{
	CharacterActor = InChar;
	if (CharacterActor)
	{
		// 실제 캐릭터를 쓰면 원기둥 표식은 숨김(클릭 콜리전은 유지)
		MeshComp->SetVisibility(false);
	}
}

void AHexUnit::FaceToward(const FVector& WorldTarget)
{
	AActor* Body = CharacterActor ? CharacterActor.Get() : (AActor*)this;
	FVector From = Body->GetActorLocation();
	FRotator Look = UKismetMathLibrary::FindLookAtRotation(FVector(From.X, From.Y, 0), FVector(WorldTarget.X, WorldTarget.Y, 0));
	Body->SetActorRotation(FRotator(0.0f, Look.Yaw, 0.0f));
}

void AHexUnit::PlayCastAnim()
{
	if (AABaseCharacter* C = Cast<AABaseCharacter>(CharacterActor)) { C->PlayRandomBasicAttackMontage(); }
}

void AHexUnit::PlayHitAnim()
{
	if (AABaseCharacter* C = Cast<AABaseCharacter>(CharacterActor)) { C->PlayRandomHitReactionMontage(); }
}

void AHexUnit::RefreshVisual()
{
	FLinearColor C = bPlayerSide ? FLinearColor(0.23f, 0.53f, 0.88f) : FLinearColor(0.88f, 0.34f, 0.29f);
	FLinearColor Emissive = FLinearColor::Black;
	// 상태를 판 위에서 한눈에: 결빙=얼음빛, 둔화=탁한 잿빛, 영창 무방비=금빛(강타 기회, 발광으로 또렷).
	if (Frozen > 0)       { C = C * 0.45f + FLinearColor(0.55f, 0.85f, 1.00f) * 0.55f; }
	else if (Slow > 0)    { C = C * 0.70f + FLinearColor(0.50f, 0.55f, 0.60f) * 0.30f; }
	if (bExposed)
	{
		C = C * 0.6f + FLinearColor(1.0f, 0.83f, 0.30f) * 0.4f;
		Emissive = FLinearColor(1.0f, 0.83f, 0.30f) * 0.35f; // "지금 쳐라" 신호
	}
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), C);
		MID->SetVectorParameterValue(TEXT("BaseColor"), C);
		MID->SetVectorParameterValue(TEXT("Emissive"), Emissive);
		MID->SetVectorParameterValue(TEXT("EmissiveColor"), Emissive);
	}
	OnUnitVisualUpdate(HP, MaxHP, bExposed, Slow, Frozen);
}
