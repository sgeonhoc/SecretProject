#include "StoryDirectorComponent.h"
#include "StoryWidget.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UStoryDirectorComponent::UStoryDirectorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UTimeComponent* UStoryDirectorComponent::FindTimeComp() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<UTimeComponent>() : nullptr;
}

UStoryManagerSubsystem* UStoryDirectorComponent::GetStory() const
{
    if (UWorld* W = GetWorld())
        if (UGameInstance* GI = W->GetGameInstance())
            return GI->GetSubsystem<UStoryManagerSubsystem>();
    return nullptr;
}

int32 UStoryDirectorComponent::CurrentDay() const
{
    if (UTimeComponent* T = FindTimeComp()) return T->GetDay();
    return 0;
}

void UStoryDirectorComponent::BeginPlay()
{
    Super::BeginPlay();

    if (UTimeComponent* T = FindTimeComp())
    {
        T->OnTimeChanged.RemoveDynamic(this, &UStoryDirectorComponent::HandleTimeChanged);
        T->OnTimeChanged.AddDynamic(this, &UStoryDirectorComponent::HandleTimeChanged);
        LastCheckedDay = T->GetDay();
    }

    // 플레이어/서브시스템 스폰 보장 후 1회 초기 체크(프롤로그 등 첫 비트 자동 등장)
    if (bAutoCheckOnDayChange && GetWorld())
        GetWorld()->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &UStoryDirectorComponent::HandleTimeChanged, CurrentDay(), EDayPhase::Morning));
}

void UStoryDirectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UTimeComponent* T = FindTimeComp())
        T->OnTimeChanged.RemoveDynamic(this, &UStoryDirectorComponent::HandleTimeChanged);
    Super::EndPlay(EndPlayReason);
}

void UStoryDirectorComponent::HandleTimeChanged(int32 Day, EDayPhase /*Phase*/)
{
    if (!bAutoCheckOnDayChange) return;
    // 시간대마다 체크하거나(기본), 날이 바뀐 경우에만
    if (!bCheckEveryPhase && Day == LastCheckedDay) return;
    LastCheckedDay = Day;
    CheckForStory();
}

bool UStoryDirectorComponent::CheckForStory()
{
    if (UStoryManagerSubsystem* Story = GetStory())
    {
        FStoryBeat Beat;
        if (Story->GetNextAvailableBeat(CurrentDay(), Beat))
        {
            OnStoryBeatReady.Broadcast(Beat);   // 연출/추가 리스너용(BP가 직접 표시하려면 bAutoShowWidget=false)

            // C++ 자체 표시(기본): 별도 BP 그래프 작업 없이 스토리 위젯이 떠서 진행 → 완료 콜백
            if (bAutoShowWidget && StoryWidgetClass)
            {
                APlayerController* PC = nullptr;
                if (APawn* Pawn = Cast<APawn>(GetOwner()))
                    PC = Cast<APlayerController>(Pawn->GetController());
                if (!PC && GetWorld())
                    PC = GetWorld()->GetFirstPlayerController();
                if (PC)
                    UStoryWidget::ShowBeat(PC, StoryWidgetClass, Beat, this);
            }
            return true;
        }
    }
    return false;
}

void UStoryDirectorComponent::CompleteAndContinue(FName BeatId)
{
    if (UStoryManagerSubsystem* Story = GetStory())
    {
        Story->CompleteBeat(BeatId);
        // 같은 타이밍에 연달아 해금된 비트가 있으면 이어서 방송(예: 인터루드 직후 다음 장면)
        CheckForStory();
    }
}
