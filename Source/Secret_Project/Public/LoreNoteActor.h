#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoreNoteActor.generated.h"

class UStaticMeshComponent;

/**
 * 읽을 수 있는 월드 오브젝트(표지판/비석/메모). 플레이어가 E로 상호작용하면
 * 기존 대화창(TalkUserWidget)에 제목+본문을 띄운다. 환경 스토리텔링용.
 * 로직 C++, 표시는 대화 위젯 재사용. 메시/배치/텍스트는 BP·에디터.
 */
UCLASS()
class SECRET_PROJECT_API ALoreNoteActor : public AActor
{
    GENERATED_BODY()

public:
    ALoreNoteActor();

protected:
    virtual void BeginPlay() override;

public:
    // 읽을거리 카탈로그(LoreLibrary) Id. 지정 시 BeginPlay에서 Title/Lines를 자동으로 채운다.
    // (Lines를 직접 채워두면 그게 우선 — 수동 배치 보존). None이면 아래 Title/Lines 수동 사용.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lore")
    FName LoreId;

    // 표시용 제목 (대화창 이름칸)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lore")
    FString Title = TEXT("표지판");

    // 본문 (여러 줄 = 클릭으로 넘김, TalkUserWidget이 처리)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lore", meta = (MultiLine = true))
    TArray<FString> Lines;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lore")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    // ── 스토리 상태 게이트(레벨 표정 스왑) ──────────────
    // RequiredFlag 지정 시 그 스토리 플래그가 서 있어야 이 조사물이 나타난다(예: 사건 발생 후 소문판).
    // ForbiddenFlag 지정 시 그 플래그가 서면 사라진다. 둘 다 비면 항상 등장(기존 동작). 레벨 진입 시 판정.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FName RequiredFlag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    FName ForbiddenFlag;

    // 사회 스탯(지식) 1회 지급 처리 여부 — 같은 표지판 반복 읽기로 파밍 방지(세션 한정 런타임)
    bool bAlreadyStudied = false;
};
