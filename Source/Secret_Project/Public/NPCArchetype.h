#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TimeComponent.h" // EDayPhase
#include "NPCArchetype.generated.h"

class AANPCCharacter;

// 캐릭터 성격 — 대사 톤/콘텐츠 자동 생성의 기준 (양산용)
UENUM(BlueprintType)
enum class ENPCPersonality : uint8
{
    Cheerful     UMETA(DisplayName = "활발"),
    Shy          UMETA(DisplayName = "내성적"),
    Intellectual UMETA(DisplayName = "지적"),
    Tough        UMETA(DisplayName = "거친"),
    Kind         UMETA(DisplayName = "상냥"),
    Cool         UMETA(DisplayName = "냉정")
};

FORCEINLINE FString LexPersonality(ENPCPersonality P)
{
    switch (P)
    {
    case ENPCPersonality::Cheerful:     return TEXT("활발");
    case ENPCPersonality::Shy:          return TEXT("내성적");
    case ENPCPersonality::Intellectual: return TEXT("지적");
    case ENPCPersonality::Tough:        return TEXT("거친");
    case ENPCPersonality::Kind:         return TEXT("상냥");
    case ENPCPersonality::Cool:         return TEXT("냉정");
    default:                            return TEXT("?");
    }
}

/**
 * 캐릭터 소셜 프로필 — 한 NPC의 비전투 콘텐츠 전체(성격/대화/선물/스케줄/상점/퀘스트/영입).
 * 카탈로그(UNPCArchetypeLibrary)에 항목을 추가하는 것 = NPC 1명 "양산". 전투(스탯/스킬/약점)는 A의 전투 프로필 담당.
 * 대사 배열을 비워두면 Personality 기반으로 자동 생성 → 최소 입력으로 캐릭터가 굴러감.
 */
USTRUCT(BlueprintType)
struct FNPCSocialProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    FName ArchetypeId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    FString DisplayName = TEXT("이름");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    ENPCPersonality Personality = ENPCPersonality::Cheerful;

    // 비우면 Personality 기반 자동 생성
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    TArray<FString> DialogueLines;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    TArray<FString> EveningDialogueLines;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    TArray<FString> BondDialogueLines;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    int32 BondLineMinRank = 3;

    // 선호 선물 아이템 Id (인벤토리 카탈로그). 비면 선물 안 받음.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    FName FavoriteGiftId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    int32 GiftAffinity = 8;

    // 선물 받았을 때 전용 반응 대사(비면 성격 기반 자동 생성)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    FString GiftReactionLine;

    // 출현 시간대 (비면 항상 등장)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    TArray<EDayPhase> ActivePhases;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    bool bIsShopkeeper = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    bool bClosedAtNight = false;

    // 상점 테마(convenience/gear/cafe/bar/flower/manhwa). 비면 종합.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    FName ShopKind;

    // 대화 시 줄 퀘스트 Id (None이면 안 줌)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    FName GrantsQuestId;

    // 인연 게이트 영입 (bond 랭크로 아군 영입)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    bool bCanBeRecruited = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    int32 RecruitMinBondRank = 0;

    // 영입 성공 순간 전용 대사(비면 기본 메시지만)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archetype")
    FString RecruitLine;
};

/**
 * NPC 양산 카탈로그 + 자동 적용. ArchetypeId로 프로필을 찾아 ANPCCharacter의 기존 필드를 채운다.
 * 사용자는 BP에서 ArchetypeId만 지정(+메시/애니) → 나머지 소셜 콘텐츠 자동.
 */
UCLASS()
class SECRET_PROJECT_API UNPCArchetypeLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ArchetypeId로 소셜 프로필 조회 (찾으면 true)
    UFUNCTION(BlueprintCallable, Category = "Archetype")
    static bool FindSocialProfile(FName ArchetypeId, FNPCSocialProfile& OutProfile);

    // 표시 이름(DisplayName)으로 소셜 프로필 조회 (인연 랭크업 연출 등 — 이름만 아는 곳에서). 찾으면 true.
    UFUNCTION(BlueprintCallable, Category = "Archetype")
    static bool FindProfileByName(const FString& DisplayName, FNPCSocialProfile& OutProfile);

    // NPC->ArchetypeId 기준으로 소셜 프로필 자동 적용. ArchetypeId None이면 무동작(수동 BP 모드 보존).
    static void ApplySocialProfile(AANPCCharacter* NPC);

    // 성격 기반 기본 대사 생성 (프로필에 명시 대사 없을 때)
    UFUNCTION(BlueprintPure, Category = "Archetype")
    static TArray<FString> GenerateDialogue(ENPCPersonality Personality, const FString& Name);

    UFUNCTION(BlueprintPure, Category = "Archetype")
    static TArray<FString> GenerateEveningDialogue(ENPCPersonality Personality, const FString& Name);

    UFUNCTION(BlueprintPure, Category = "Archetype")
    static TArray<FString> GenerateBondDialogue(ENPCPersonality Personality, const FString& Name);

    // 카탈로그에 등록된 ArchetypeId 목록 (양산 현황/툴링용)
    UFUNCTION(BlueprintCallable, Category = "Archetype")
    static TArray<FName> GetAllArchetypeIds();

    // 전체 카탈로그 (C++ 전용)
    static const TArray<FNPCSocialProfile>& GetCatalog();
};
