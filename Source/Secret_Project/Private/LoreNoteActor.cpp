#include "LoreNoteActor.h"
#include "Components/StaticMeshComponent.h"
#include "LoreLibrary.h"
#include "StoryManager.h" // 스토리 플래그 게이트

ALoreNoteActor::ALoreNoteActor()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = MeshComp;
    // 라인트레이스(가시성) 상호작용용 → Visibility 블록
    MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void ALoreNoteActor::BeginPlay()
{
    Super::BeginPlay();

    // LoreId 지정 + 본문 수동입력 없을 때만 카탈로그에서 자동 채움(수동 배치 보존).
    if (!LoreId.IsNone() && Lines.Num() == 0)
    {
        FLoreEntry Entry;
        if (ULoreLibrary::FindLore(LoreId, Entry))
        {
            Title = Entry.Title;
            Lines = Entry.Lines;
        }
    }

    // 스토리 상태 게이트: 조건 안 맞으면 이 상태에선 없음(숨김+충돌끔).
    if (!UStoryManagerSubsystem::PassesFlagGate(this, RequiredFlag, ForbiddenFlag))
    {
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
    }
}
