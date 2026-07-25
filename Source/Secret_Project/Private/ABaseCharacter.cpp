#include "ABaseCharacter.h"
#include "StatComponent.h"
#include "CombatComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"

// Sets default values
AABaseCharacter::AABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

    StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
    CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
}

// Called when the game starts or when spawned
void AABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AABaseCharacter::PlaySequenceDynamic(UAnimSequence* Seq, float PlayRate)
{
	if (!Seq) return;
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;

	if (UAnimInstance* Anim = MeshComp->GetAnimInstance())
	{
		// ABP가 있으면 다이내믹 슬롯 몽타주("DefaultSlot")로 재생.
		// ※ ABP AnimGraph에 "DefaultSlot" 슬롯 노드가 있어야 화면에 보임(없으면 재생되나 출력 포즈에 안 섞임).
		Anim->PlaySlotAnimationAsDynamicMontage(Seq, FName("DefaultSlot"), 0.1f, 0.15f, PlayRate, 1, -1.f, 0.f);
	}
	else
	{
		// ABP 자체가 없는 메시 → 단일노드 모드로 시퀀스 직접 재생(애니가 아예 안 나오던 폴백).
		MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		MeshComp->SetAnimation(Seq);
		MeshComp->SetPlayRate(PlayRate);
		MeshComp->Play(false);
	}
}

void AABaseCharacter::PlayRandomBasicAttackMontage(int32 PreferIndex)
{
	// 몽타주 우선, 없으면 시퀀스(리타겟 패러곤 애니) 다이내믹 재생.
	// PreferIndex>=0이면 그 인덱스를 모듈러로 골라 스킬/속성별로 다른 모션이 나오게(차별).
	if (BasicAttackMontages.Num() > 0)
	{
		int32 Idx = (PreferIndex >= 0) ? (PreferIndex % BasicAttackMontages.Num())
		                               : FMath::RandRange(0, BasicAttackMontages.Num() - 1);
		if (BasicAttackMontages[Idx]) { PlayAnimMontage(BasicAttackMontages[Idx]); return; }
	}
	if (BasicAttackSequences.Num() > 0)
	{
		int32 Idx = (PreferIndex >= 0) ? (PreferIndex % BasicAttackSequences.Num())
		                               : FMath::RandRange(0, BasicAttackSequences.Num() - 1);
		PlaySequenceDynamic(BasicAttackSequences[Idx]);
	}
}

void AABaseCharacter::PlayRandomHitReactionMontage()
{
	if (HitReactionMontages.Num() > 0)
	{
		int32 Idx = FMath::RandRange(0, HitReactionMontages.Num() - 1);
		if (HitReactionMontages[Idx]) { PlayAnimMontage(HitReactionMontages[Idx]); return; }
	}
	if (HitReactionSequences.Num() > 0)
	{
		int32 Idx = FMath::RandRange(0, HitReactionSequences.Num() - 1);
		PlaySequenceDynamic(HitReactionSequences[Idx]);
	}
}

void AABaseCharacter::PlaySkillMontage(UAnimMontage* M, int32 MotionVariant)
{
	// 스킬에 전용 몽타주가 지정돼 있으면 그걸, 아니면 기본 공격 모션으로 폴백.
	// MotionVariant(속성/스킬 유도)로 폴백 모션을 고정 → 물리/마법/속성별로 다른 동작.
	if (M) PlayAnimMontage(M);
	else   PlayRandomBasicAttackMontage(MotionVariant);
}

bool AABaseCharacter::IsInCombat() const
{
	return CombatComponent && CombatComponent->IsInCombat();
}

TArray<FString> AABaseCharacter::LearnSkillsUpToLevel(int32 NewLevel)
{
	TArray<FString> Learned;
	for (const FLevelUpSkill& LU : LearnableSkills)
	{
		if (LU.Level > NewLevel) continue;

		// 같은 이름 스킬을 이미 보유했으면 건너뜀(중복 습득 방지)
		bool bAlready = false;
		for (const FSkillDef& S : Skills)
			if (S.SkillName == LU.Skill.SkillName) { bAlready = true; break; }

		if (!bAlready)
		{
			Skills.Add(LU.Skill);
			Learned.Add(LU.Skill.SkillName);
		}
	}
	return Learned;
}



