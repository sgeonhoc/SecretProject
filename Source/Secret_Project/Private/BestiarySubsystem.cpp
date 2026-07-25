#include "BestiarySubsystem.h"
#include "SecretSaveGame.h"

void UBestiarySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // 이전 세션 도감 적재(세이브 없으면 무동작)
    USecretSaveGame::LoadBestiaryInto(this);
}

void UBestiarySubsystem::ImportRecord(FName EnemyId, int32 DefeatedCount, bool bEncountered, const TArray<EBattleElement>& KnownWeak)
{
    if (EnemyId.IsNone()) return;
    FBestiaryEntry& E = Entries.FindOrAdd(EnemyId);
    E.DefeatedCount = DefeatedCount;
    E.bEncountered = bEncountered;
    E.KnownWeak = KnownWeak;
}

void UBestiarySubsystem::RecordEncounter(FName EnemyId)
{
    if (EnemyId.IsNone()) return;
    Entries.FindOrAdd(EnemyId).bEncountered = true;
}

void UBestiarySubsystem::RecordDefeat(FName EnemyId)
{
    if (EnemyId.IsNone()) return;
    FBestiaryEntry& E = Entries.FindOrAdd(EnemyId);
    E.bEncountered = true;
    E.DefeatedCount++;
}

void UBestiarySubsystem::RecordWeakness(FName EnemyId, EBattleElement Element)
{
    if (EnemyId.IsNone()) return;
    Entries.FindOrAdd(EnemyId).KnownWeak.AddUnique(Element);
}

int32 UBestiarySubsystem::GetDefeatedCount(FName EnemyId) const
{
    const FBestiaryEntry* E = Entries.Find(EnemyId);
    return E ? E->DefeatedCount : 0;
}

bool UBestiarySubsystem::GetEntry(FName EnemyId, FBestiaryEntry& OutEntry) const
{
    if (const FBestiaryEntry* E = Entries.Find(EnemyId))
    {
        OutEntry = *E;
        return true;
    }
    return false;
}
