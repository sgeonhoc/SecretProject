#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LoreLibrary.generated.h"

/**
 * 읽을거리 한 편(신문/괴담/일기/전단/비석). 환경 스토리텔링 = "데이터로 양산되는 월드빌딩".
 * LoreNoteActor에 LoreId만 지정하면 여기 본문이 자동으로 채워진다(메시/배치만 BP).
 * 항목 추가 = 스토리(세계관) 확장. 메인 스파인(A StoryManager)과 별개 레이어 — B 소유.
 */
USTRUCT(BlueprintType)
struct FLoreEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lore")
    FName LoreId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lore")
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lore", meta = (MultiLine = true))
    TArray<FString> Lines;
};

/**
 * 읽을거리 카탈로그 + 조회. LoreNoteActor가 LoreId로 본문을 꺼내 쓴다(NPC/전투 양산과 같은 패턴).
 */
UCLASS()
class SECRET_PROJECT_API ULoreLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // LoreId로 읽을거리 조회 (찾으면 true)
    UFUNCTION(BlueprintCallable, Category = "Lore")
    static bool FindLore(FName LoreId, FLoreEntry& OutEntry);

    // 등록된 LoreId 전체 (배치/툴링용)
    UFUNCTION(BlueprintCallable, Category = "Lore")
    static TArray<FName> GetAllLoreIds();

    // 전체 카탈로그 (C++ 전용)
    static const TArray<FLoreEntry>& GetCatalog();
};
