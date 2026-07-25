#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueData.generated.h"

class UTexture2D;

/**
 * 대화/텍스트 콘텐츠 한 줄. 위젯을 직접 고치지 않고 "데이터"로 콘텐츠 관리.
 * 파이썬으로 대량 추가/수정 가능(위젯트리와 달리 DataAsset은 파이썬 접근 OK).
 */
USTRUCT(BlueprintType)
struct FDialogueEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    FName Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    FText Speaker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = true))
    FText Line;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TSoftObjectPtr<UTexture2D> Portrait;
};

/**
 * 대화 묶음(DataAsset). 위젯/시스템은 FindById로 꺼내 표시.
 * 콘텐츠 추가 = Entries에 줄 추가(파이썬으로 가능).
 */
UCLASS(BlueprintType)
class SECRET_PROJECT_API UDialogueData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TArray<FDialogueEntry> Entries;

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    bool FindById(FName InId, FDialogueEntry& OutEntry) const
    {
        for (const FDialogueEntry& E : Entries)
        {
            if (E.Id == InId) { OutEntry = E; return true; }
        }
        return false;
    }

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    int32 NumEntries() const { return Entries.Num(); }
};
