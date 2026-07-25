// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BattleTypes.h"
#include "ABaseCharacter.generated.h"

// 전방 선언 (컴파일 속도 향상)
class UStatComponent;
class UCombatComponent;
class UAnimSequence;

UCLASS()
class SECRET_PROJECT_API AABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AABaseCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 1. 스탯을 관리할 컴포넌트 파일 등록
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStatComponent> StatComponent;

	// 2. 전투 로직을 관리할 컴포넌트 파일 등록
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatComponent> CombatComponent;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ABP에서 사용할 기본 공격 몽타주 목록 (BP에서 채워 넣기)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Montages")
	TArray<TObjectPtr<UAnimMontage>> BasicAttackMontages;

	// 피격 리액션 몽타주 목록 (BP에서 채워 넣기)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Montages")
	TArray<TObjectPtr<UAnimMontage>> HitReactionMontages;

	// ★ 시퀀스 기반(리타겟 패러곤 애니 등 — 몽타주 없이 AnimSequence 직접). 몽타주 비면 이걸로 폴백.
	//   다이내믹 슬롯몽타주로 재생되므로 ABP에 슬롯("DefaultSlot")만 있으면 됨.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Montages")
	TArray<TObjectPtr<UAnimSequence>> BasicAttackSequences;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Montages")
	TArray<TObjectPtr<UAnimSequence>> HitReactionSequences;

	// 단일 AnimSequence를 다이내믹 슬롯 몽타주로 재생(몽타주 에셋 없이). 슬롯명 기본 DefaultSlot.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlaySequenceDynamic(UAnimSequence* Seq, float PlayRate = 1.f);

	// 랜덤 기본 공격 몽타주 재생 (CombatComponent에서 호출). PreferIndex>=0이면 그 인덱스(모듈러) 선택 → 스킬별 차별 모션.
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayRandomBasicAttackMontage(int32 PreferIndex = -1);

	// 랜덤 피격 몽타주 재생
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayRandomHitReactionMontage();

	// 스킬 전용 몽타주 재생 (M이 지정되면 그걸, 비면 기본 공격 모션 폴백). MotionVariant>=0이면 폴백 시 모션 인덱스 고정(속성/스킬별 차별).
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlaySkillMontage(UAnimMontage* M, int32 MotionVariant = -1);

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UStatComponent* GetStatComponent() const { return StatComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComponent; }

	// ── 페르소나 전투: 스킬/속성 상성 ──────────────────────

	// 이 캐릭터가 쓸 수 있는 스킬 (BP 디테일에서 캐릭터별로 채움)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Skills")
	TArray<FSkillDef> Skills;

	// 레벨업 시 습득할 스킬 (Level 도달 시 Skills에 자동 추가)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Skills")
	TArray<FLevelUpSkill> LearnableSkills;

	// NewLevel까지 도달한 미보유 스킬을 Skills에 추가. 새로 배운 스킬 이름 반환
	UFUNCTION(BlueprintCallable, Category = "Battle|Skills")
	TArray<FString> LearnSkillsUpToLevel(int32 NewLevel);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Affinity")
	TArray<EBattleElement> WeakElements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Affinity")
	TArray<EBattleElement> ResistElements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Affinity")
	TArray<EBattleElement> NullElements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Affinity")
	TArray<EBattleElement> AbsorbElements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Affinity")
	TArray<EBattleElement> RepelElements;

	// 보스 여부 — HP 50% 미만으로 떨어지면 1회 분노(공격력↑ + 자기 상태이상 해제)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Boss")
	bool bIsBoss = false;

	// ★ 레이드 보스 레벨 — 0=일반. 150/200 등으로 설정하면 그 레벨에 맞춰 HP/공격력을 대폭 스케일
	//   (플레이어 상한 100을 '뚫는' 레이드 티어 적). BP의 적에 숫자만 넣으면 레이드 보스가 됨.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Boss", meta = (ClampMin = "0"))
	int32 BossLevel = 0;

	// 면역인 상태이상 (BP에서 캐릭터별 지정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Affinity")
	TArray<EAilment> AilmentImmune;

	// 물리 공격을 맞고 생존 시 시전자에게 되받아치는 확률 (0~1, 기본 0=없음). 페르소나식 Counter 패시브
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Counter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CounterChance = 0.f;

	// 궁지(Adversity): 자신 HP가 25% 미만일 때 가하는 데미지 증가율 (0=없음, 0.5=+50%). 빈사 역전 패시브
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Adversity", meta = (ClampMin = "0.0"))
	float LowHPDamageBonus = 0.f;

	// 처치 시 아이템 드롭 (전투 보상). DropItemId=인벤토리 카탈로그 Id(비면 드롭 없음), 확률 DropChance(0~1), 개수 DropCount.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Reward")
	FName DropItemId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Reward", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Reward", meta = (ClampMin = "1"))
	int32 DropCount = 1;

	// 이 캐릭터가 해당 상태이상에 저항(면역)하는가. 보스는 수면/빙결(하드CC) 자동 면역.
	UFUNCTION(BlueprintPure, Category = "Battle|Affinity")
	bool ResistsAilment(EAilment A) const
	{
		if (A == EAilment::None) return false;
		if (AilmentImmune.Contains(A)) return true;
		if (bIsBoss && (A == EAilment::Sleep || A == EAilment::Freeze)) return true;
		return false;
	}

	// 대상이 해당 속성에 대해 갖는 상성 반환
	UFUNCTION(BlueprintPure, Category = "Battle|Affinity")
	EAffinity GetAffinity(EBattleElement Element) const
	{
		if (NullElements.Contains(Element))   return EAffinity::Null;
		if (AbsorbElements.Contains(Element)) return EAffinity::Absorb;
		if (RepelElements.Contains(Element))  return EAffinity::Repel;
		if (WeakElements.Contains(Element))   return EAffinity::Weak;
		if (ResistElements.Contains(Element)) return EAffinity::Resist;
		return EAffinity::Normal;
	}

	// ABP에서 직접 읽을 수 있는 전투 상태 체크 (CombatComponent 체인 불필요)
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInCombat() const;

};
