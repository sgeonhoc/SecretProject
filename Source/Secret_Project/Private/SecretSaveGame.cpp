#include "SecretSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "StatComponent.h"
#include "InventoryComponent.h"
#include "QuestComponent.h"
#include "TimeComponent.h"
#include "RelationshipComponent.h"
#include "WeatherComponent.h"
#include "BankComponent.h"
#include "EquipmentComponent.h"
#include "BestiarySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void USecretSaveGame::SavePlayerProgression(UStatComponent* PlayerStat)
{
    if (!PlayerStat) return;

    // 기존 세이브 로드(아군 진행 보존), 없으면 새로 생성
    USecretSaveGame* Save = nullptr;
    if (UGameplayStatics::DoesSaveGameExist(TEXT("PlayerSave"), 0))
        Save = Cast<USecretSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0));
    if (!Save)
        Save = Cast<USecretSaveGame>(UGameplayStatics::CreateSaveGameObject(USecretSaveGame::StaticClass()));
    if (!Save) return;

    int32 Lv = 1, XP = 0;
    float MHP = 100.f, Atk = 15.f, Def = 5.f, MSP = 50.f;
    PlayerStat->GetProgression(Lv, XP, MHP, Atk, Def, MSP);

    Save->bHasData = true;
    Save->PlayerLevel = Lv;
    Save->PlayerXP = XP;
    Save->PlayerMaxHP = MHP;
    Save->PlayerAttack = Atk;
    Save->PlayerDefense = Def;
    Save->PlayerMaxSP = MSP;
    Save->PlayerGold = PlayerStat->GetGold();
    // Save->Allies 는 로드된 값 그대로 유지 (덮어쓰지 않음)

    // 인벤토리/퀘스트: 플레이어 액터에서 컴포넌트 찾아 저장 (있을 때만 갱신)
    if (AActor* Owner = PlayerStat->GetOwner())
    {
        if (UInventoryComponent* Inv = Owner->FindComponentByClass<UInventoryComponent>())
            Save->Inventory = Inv->GetStacks();
        if (UQuestComponent* Quest = Owner->FindComponentByClass<UQuestComponent>())
            Save->Quests = Quest->GetRecords();
        if (UTimeComponent* Time = Owner->FindComponentByClass<UTimeComponent>())
        {
            Save->Day = Time->GetDay();
            Save->TimePhase = Time->GetPhase();
        }
        if (URelationshipComponent* Rel = Owner->FindComponentByClass<URelationshipComponent>())
            Save->Relationships = Rel->GetRecords();
        if (UWeatherComponent* Weather = Owner->FindComponentByClass<UWeatherComponent>())
            Save->Weather = Weather->GetWeather();
        if (UBankComponent* Bank = Owner->FindComponentByClass<UBankComponent>())
            Save->StoredGold = Bank->GetStoredGold();
        if (UEquipmentComponent* Equip = Owner->FindComponentByClass<UEquipmentComponent>())
            Equip->GetEquippedIds(Save->EquippedIds);

        // 적 도감(GameInstance 서브시스템) 스냅샷
        if (UWorld* W = Owner->GetWorld())
            if (UGameInstance* GI = W->GetGameInstance())
                if (UBestiarySubsystem* Bst = GI->GetSubsystem<UBestiarySubsystem>())
                {
                    Save->Bestiary.Reset();
                    for (const TPair<FName, FBestiaryEntry>& P : Bst->GetAll())
                    {
                        FBestiaryRecord R;
                        R.Id            = P.Key;
                        R.DefeatedCount = P.Value.DefeatedCount;
                        R.bEncountered  = P.Value.bEncountered;
                        R.KnownWeak     = P.Value.KnownWeak;
                        Save->Bestiary.Add(R);
                    }
                }
    }

    UGameplayStatics::SaveGameToSlot(Save, TEXT("PlayerSave"), 0);
}

bool USecretSaveGame::IsCollected(FName Id)
{
    if (Id.IsNone()) return false;
    if (!UGameplayStatics::DoesSaveGameExist(TEXT("PlayerSave"), 0)) return false;
    if (USecretSaveGame* Save = Cast<USecretSaveGame>(
            UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0)))
        return Save->CollectedIds.Contains(Id);
    return false;
}

void USecretSaveGame::MarkCollected(FName Id)
{
    if (Id.IsNone()) return;

    USecretSaveGame* Save = nullptr;
    if (UGameplayStatics::DoesSaveGameExist(TEXT("PlayerSave"), 0))
        Save = Cast<USecretSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0));
    if (!Save)
        Save = Cast<USecretSaveGame>(UGameplayStatics::CreateSaveGameObject(USecretSaveGame::StaticClass()));
    if (!Save) return;

    Save->bHasData = true;
    Save->CollectedIds.AddUnique(Id);
    UGameplayStatics::SaveGameToSlot(Save, TEXT("PlayerSave"), 0);
}

bool USecretSaveGame::HasSave()
{
    return UGameplayStatics::DoesSaveGameExist(TEXT("PlayerSave"), 0);
}

void USecretSaveGame::DeleteSave()
{
    if (UGameplayStatics::DoesSaveGameExist(TEXT("PlayerSave"), 0))
        UGameplayStatics::DeleteGameInSlot(TEXT("PlayerSave"), 0);
}

void USecretSaveGame::LoadBestiaryInto(UBestiarySubsystem* Sub)
{
    if (!Sub) return;
    if (!UGameplayStatics::DoesSaveGameExist(TEXT("PlayerSave"), 0)) return;
    if (USecretSaveGame* Save = Cast<USecretSaveGame>(
            UGameplayStatics::LoadGameFromSlot(TEXT("PlayerSave"), 0)))
    {
        for (const FBestiaryRecord& R : Save->Bestiary)
            Sub->ImportRecord(R.Id, R.DefeatedCount, R.bEncountered, R.KnownWeak);
    }
}
