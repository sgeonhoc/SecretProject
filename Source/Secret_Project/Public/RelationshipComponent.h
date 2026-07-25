#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RelationshipComponent.generated.h"

// NPC 1명과의 인연(소셜링크) 진행 상황. NPCName(FName)으로 식별. 런타임 + 세이브 공용.
USTRUCT(BlueprintType)
struct FRelationshipRecord
{
    GENERATED_BODY()

    UPROPERTY() FName NPCName;
    UPROPERTY() int32 Points = 0;
    UPROPERTY() int32 Rank = 0;
    // 마지막으로 대화로 호감도를 올린 게임 날짜 (하루 1회 제한용)
    UPROPERTY() int32 LastTalkedDay = 0;
    // 마지막으로 선물한 게임 날짜 (하루 1회 제한용)
    UPROPERTY() int32 LastGiftDay = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRelationshipChanged, FName, NPCName, int32, NewRank);

/**
 * 페르소나식 NPC 인연(소셜링크). 플레이어에 장착, 세이브 영구화.
 * 대화 시 하루 1회 호감도 상승, 일정 누적마다 랭크업 + 보상(골드). 로직 전부 C++.
 * 랭크는 BP/다른 시스템이 OnRelationshipChanged로 구독하거나 GetRank로 조회해 대사/이벤트 분기에 사용.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SECRET_PROJECT_API URelationshipComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URelationshipComponent();

    // 랭크 1단계에 필요한 호감도 포인트
    static constexpr int32 PointsPerRank = 10;
    // 최대 랭크
    static constexpr int32 MaxRank = 10;
    // 대화 1회(하루 1회)당 상승 포인트
    static constexpr int32 TalkPoints = 3;

    UPROPERTY(BlueprintAssignable, Category = "Relationship")
    FOnRelationshipChanged OnRelationshipChanged;

    // 대화로 호감도 상승 (같은 날 중복 호출은 무시 — 하루 1회). 월드 상호작용에서 호출.
    // 반환: 이번 호출에 실제로 호감도가 올랐는가(하루 첫 대화면 true) — 사회스탯 등 연동용.
    UFUNCTION(BlueprintCallable, Category = "Relationship")
    bool RegisterTalk(FName NPCName, int32 CurrentDay);

    // 임의 호감도 추가 (선물/이벤트 등). 하루 제한 없음.
    UFUNCTION(BlueprintCallable, Category = "Relationship")
    void AddAffinity(FName NPCName, int32 Amount);

    // 선물로 호감도 상승. 같은 날 이미 선물했으면 false(소모 안 되게 호출부에서 체크용). 줬으면 true.
    UFUNCTION(BlueprintCallable, Category = "Relationship")
    bool TryGift(FName NPCName, int32 CurrentDay, int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Relationship")
    int32 GetRank(FName NPCName) const;

    UFUNCTION(BlueprintPure, Category = "Relationship")
    int32 GetPoints(FName NPCName) const;

    // 현재 랭크에서 다음 랭크까지 누적/필요 포인트 (UI 게이지용). 최대 랭크면 둘 다 PointsPerRank.
    UFUNCTION(BlueprintPure, Category = "Relationship")
    void GetRankProgress(FName NPCName, int32& OutInto, int32& OutNeeded) const;

    // 최대 랭크에 도달한 인연 수 (상태/도감 화면용)
    UFUNCTION(BlueprintPure, Category = "Relationship")
    int32 GetMaxedCount() const;

    // 진행중 인연 정의 목록 (UI 표시용, by-value)
    UFUNCTION(BlueprintCallable, Category = "Relationship")
    void GetAllRelationships(TArray<FRelationshipRecord>& OutRecords) const;

    // 세이브/로드
    const TArray<FRelationshipRecord>& GetRecords() const { return Records; }
    void LoadRecords(const TArray<FRelationshipRecord>& InRecords) { Records = InRecords; }

private:
    UPROPERTY(VisibleAnywhere, Category = "Relationship")
    TArray<FRelationshipRecord> Records;

    FRelationshipRecord& FindOrAddRecord(FName NPCName);
    // 포인트를 더하고 랭크 재계산. 랭크가 올랐으면 보상+알림.
    void ApplyPoints(FRelationshipRecord& Rec, int32 Amount);
    void OnRankUp(const FRelationshipRecord& Rec);
    void PersistAndNotify(const FString& Message);
};
