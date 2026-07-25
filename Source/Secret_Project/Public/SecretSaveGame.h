#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InventoryComponent.h" // FItemStack
#include "QuestComponent.h"      // FQuestRecord
#include "TimeComponent.h"       // EDayPhase
#include "RelationshipComponent.h" // FRelationshipRecord
#include "WeatherComponent.h"     // EWeather
#include "BattleTypes.h"         // EBattleElement (도감 약점)
#include "SecretSaveGame.generated.h"

class UBestiarySubsystem;

// 적 도감 1종의 영구 기록 (서브시스템 ↔ 세이브 직렬화용 미러)
USTRUCT()
struct FBestiaryRecord
{
    GENERATED_BODY()

    UPROPERTY() FName Id;
    UPROPERTY() int32 DefeatedCount = 0;
    UPROPERTY() bool bEncountered = false;
    UPROPERTY() TArray<EBattleElement> KnownWeak;
};

// 아군 NPC 1명의 진행 상황 (이름으로 식별)
USTRUCT()
struct FAllyProgress
{
    GENERATED_BODY()

    UPROPERTY() FString Name;
    UPROPERTY() int32 Level = 1;
    UPROPERTY() int32 XP = 0;
    UPROPERTY() float MaxHP = 80.f;
    UPROPERTY() float Attack = 12.f;
    UPROPERTY() float Defense = 5.f;
    UPROPERTY() float MaxSP = 50.f;
};

// 진행 상황 저장 (플레이어 + 아군 NPC들)
UCLASS()
class SECRET_PROJECT_API USecretSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY() bool bHasData = false;

    UPROPERTY() int32 PlayerLevel = 1;
    UPROPERTY() int32 PlayerXP = 0;
    UPROPERTY() float PlayerMaxHP = 100.f;
    UPROPERTY() float PlayerAttack = 15.f;
    UPROPERTY() float PlayerDefense = 5.f;
    UPROPERTY() float PlayerMaxSP = 50.f;
    UPROPERTY() int32 PlayerGold = 0;

    // 아군 NPC들 (NPCName 기준)
    UPROPERTY() TArray<FAllyProgress> Allies;

    // 플레이어 소비아이템 보관함 (Id+개수)
    UPROPERTY() TArray<FItemStack> Inventory;

    // 1회성 월드 수집물(보물상자/획득물)의 고유 Id 목록 — 재방문 시 다시 안 나오게
    UPROPERTY() TArray<FName> CollectedIds;

    // 퀘스트 진행 기록
    UPROPERTY() TArray<FQuestRecord> Quests;

    // 날짜/시간대
    UPROPERTY() int32 Day = 1;
    UPROPERTY() EDayPhase TimePhase = EDayPhase::Morning;

    // 적 도감(처치수/조우/발견약점) 영구 기록
    UPROPERTY() TArray<FBestiaryRecord> Bestiary;

    // NPC 인연(소셜링크) 진행 기록
    UPROPERTY() TArray<FRelationshipRecord> Relationships;

    // 현재 날씨
    UPROPERTY() EWeather Weather = EWeather::Clear;

    // 은행 예치금
    UPROPERTY() int32 StoredGold = 0;

    // 장착 중인 장비 Id 목록(슬롯별) — EquipmentComponent 직렬화
    UPROPERTY() TArray<FName> EquippedIds;

    // 플레이어 진행(레벨/스탯/골드)을 "PlayerSave" 슬롯0에 저장. 기존 아군 데이터는 보존.
    // 상점 구매·전투 승리 등에서 공용으로 사용 (저장 시퀀스 중복 방지).
    static void SavePlayerProgression(class UStatComponent* PlayerStat);

    // ── 월드 수집물 영구화 (보물상자/획득물 공용) ──
    // 슬롯0 세이브에서 이 Id가 이미 수집됐는지. 세이브 없으면 false.
    static bool IsCollected(FName Id);
    // 이 Id를 수집됨으로 기록(기존 세이브 로드→추가→저장, 나머지 데이터 보존).
    static void MarkCollected(FName Id);

    // ── 세이브 슬롯 관리 (시스템 메뉴용) ──
    // "PlayerSave" 슬롯0 존재 여부
    static bool HasSave();
    // "PlayerSave" 슬롯0 삭제 (새 게임용)
    static void DeleteSave();

    // ── 적 도감 영구화 ──
    // 슬롯0의 저장된 도감 기록을 서브시스템에 적재(게임 시작 시 1회). 세이브 없으면 무동작.
    static void LoadBestiaryInto(UBestiarySubsystem* Sub);
};
