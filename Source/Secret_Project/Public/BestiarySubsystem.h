#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BattleTypes.h"
#include "BestiarySubsystem.generated.h"

// 적 도감 한 항목
USTRUCT(BlueprintType)
struct FBestiaryEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Bestiary")
    int32 DefeatedCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Bestiary")
    TArray<EBattleElement> KnownWeak;   // 플레이어가 발견한 약점

    UPROPERTY(BlueprintReadOnly, Category = "Bestiary")
    bool bEncountered = false;          // 한 번이라도 조우(전투 진입)했는가
};

/**
 * 적 도감(Bestiary). 처치 수 + 발견 약점을 전역으로 기록.
 * - GameInstanceSubsystem이라 어디서든 GetSubsystem으로 접근.
 * - BattleManager가 처치/약점발견 시 기록. (영구 저장은 추후 — 현재 세션 내 유지)
 */
UCLASS()
class SECRET_PROJECT_API UBestiarySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // 게임 시작 시 세이브에서 도감 적재
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // 세이브 로드용 — 한 항목 적재(덮어쓰기). SecretSaveGame이 호출.
    void ImportRecord(FName EnemyId, int32 DefeatedCount, bool bEncountered, const TArray<EBattleElement>& KnownWeak);

    UFUNCTION(BlueprintCallable, Category = "Bestiary")
    void RecordEncounter(FName EnemyId);

    UFUNCTION(BlueprintCallable, Category = "Bestiary")
    void RecordDefeat(FName EnemyId);

    UFUNCTION(BlueprintCallable, Category = "Bestiary")
    void RecordWeakness(FName EnemyId, EBattleElement Element);

    UFUNCTION(BlueprintPure, Category = "Bestiary")
    int32 GetDefeatedCount(FName EnemyId) const;

    UFUNCTION(BlueprintPure, Category = "Bestiary")
    bool GetEntry(FName EnemyId, FBestiaryEntry& OutEntry) const;

    UFUNCTION(BlueprintPure, Category = "Bestiary")
    int32 NumKnownEnemies() const { return Entries.Num(); }

    // 저장/로드용(추후 세이브 연동)
    const TMap<FName, FBestiaryEntry>& GetAll() const { return Entries; }

private:
    UPROPERTY()
    TMap<FName, FBestiaryEntry> Entries;
};
