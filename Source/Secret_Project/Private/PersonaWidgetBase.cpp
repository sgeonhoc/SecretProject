#include "PersonaWidgetBase.h"
#include "Animation/WidgetAnimation.h"
#include "TimerManager.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"            // 버튼 클릭음 일괄 부여
#include "Blueprint/WidgetTree.h"
#include "Sound/SoundBase.h"
#include "GameAudioSubsystem.h"
#include "Engine/GameInstance.h"

void UPersonaWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();
    // 재오픈 시마다 등장 연출 다시 재생 + 첫 프레임 깜빡임 방지(즉시 투명/이동 세팅)
    bEntranceInit = false;
    bEntranceDone = false;
    EntranceElapsed = 0.f;
    if (bPlayEntrance)
    {
        SetRenderOpacity(0.f);
        SetRenderTranslation(FVector2D(0.f, EntranceSlideY));
    }
    if (bAutoPlayIntro)
        PlayIntro(); // BP 구현(없으면 무동작)

    // ★ 메뉴 사운드: 모든 자식 버튼에 클릭음 + (팝업류면) 열기음. 디자이너 BP 없이 동작.
    if (bAutoUISound)
    {
        ApplyButtonClickSound();
        if (bCloseOnEsc)              // 팝업/메뉴(상시 HUD·전투HUD·대화창 제외)만 열기음
            PlayUI(TEXT("ui_open"), 0.55f);
    }
}

void UPersonaWidgetBase::PlayUI(FName Name, float Volume) const
{
    if (Name.IsNone()) return;
    if (UGameInstance* GI = GetGameInstance())
        if (UGameAudioSubsystem* A = GI->GetSubsystem<UGameAudioSubsystem>())
            A->PlaySFX(Name, Volume);
}

void UPersonaWidgetBase::ApplyButtonClickSound()
{
    if (!WidgetTree) return;
    static const TCHAR* ClickPath = TEXT("/Game/Audio/SFX/SFX_ui_click");
    USoundBase* Click = LoadObject<USoundBase>(nullptr, ClickPath);
    if (!Click) return;   // 사운드 미배선이면 조용히 무시
    WidgetTree->ForEachWidget([Click](UWidget* W)
    {
        if (UButton* B = Cast<UButton>(W))
        {
            FButtonStyle S = B->WidgetStyle;
            S.PressedSlateSound.SetResourceObject(Click);   // 누를 때 클릭음(SButton 자동 재생)
            B->SetStyle(S);
        }
    });
}

void UPersonaWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // ① 위젯 자체 등장(페이드+슬라이드)
    if (bPlayEntrance && !bEntranceDone)
    {
        if (!bEntranceInit) { bEntranceInit = true; EntranceElapsed = 0.f; }
        EntranceElapsed += InDeltaTime;
        const float D = FMath::Max(0.01f, EntranceDuration);
        const float a = FMath::Clamp(EntranceElapsed / D, 0.f, 1.f);
        const float e = 1.f - FMath::Pow(1.f - a, 3.f);
        SetRenderOpacity(e);
        SetRenderTranslation(FVector2D(0.f, EntranceSlideY * (1.f - e)));
        if (a >= 1.f) { bEntranceDone = true; SetRenderOpacity(1.f); SetRenderTranslation(FVector2D::ZeroVector); }
    }

    // ② 리스트 항목 순차 등장(stagger)
    if (bIntroActive)
    {
        IntroElapsed += InDeltaTime;
        bool bAllDone = true;
        for (int32 i = 0; i < IntroItems.Num(); ++i)
        {
            UWidget* W = IntroItems[i].Get();
            if (!W) continue;
            const float Start = i * IntroPerDelay;
            const float a = FMath::Clamp((IntroElapsed - Start) / IntroItemDur, 0.f, 1.f);
            const float e = 1.f - FMath::Pow(1.f - a, 3.f);
            W->SetRenderOpacity(e);
            W->SetRenderTranslation(FVector2D(0.f, IntroSlideY * (1.f - e)));
            if (a < 1.f) bAllDone = false;
        }
        if (bAllDone) { bIntroActive = false; IntroItems.Reset(); }
    }
}

void UPersonaWidgetBase::StaggerIntro(UPanelWidget* Container, float PerItemDelay)
{
    IntroItems.Reset();
    if (!Container) { bIntroActive = false; return; }
    const int32 N = Container->GetChildrenCount();
    for (int32 i = 0; i < N; ++i)
    {
        if (UWidget* W = Container->GetChildAt(i))
        {
            W->SetRenderOpacity(0.f);   // 시작은 투명(첫 프레임 깜빡임 방지)
            IntroItems.Add(W);
        }
    }
    IntroPerDelay = FMath::Max(0.f, PerItemDelay);
    IntroElapsed = 0.f;
    bIntroActive = IntroItems.Num() > 0;
}

UWidgetAnimation* UPersonaWidgetBase::FindAnimByName(FName AnimName) const
{
    // WidgetAnimation은 생성된 위젯 클래스에 같은 이름의 Transient 오브젝트 프로퍼티로 존재
    if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(GetClass(), AnimName))
        return Cast<UWidgetAnimation>(Prop->GetObjectPropertyValue_InContainer(this));
    return nullptr;
}

bool UPersonaWidgetBase::PlayNamedAnimation(FName AnimName, float StartAtTime, int32 NumLoops, float PlaybackSpeed)
{
    if (UWidgetAnimation* Anim = FindAnimByName(AnimName))
    {
        PlayAnimation(Anim, StartAtTime, NumLoops, EUMGSequencePlayMode::Forward, PlaybackSpeed);
        return true;
    }
    return false;
}

void UPersonaWidgetBase::CloseWithOutro(float DelaySeconds)
{
    PlayUI(TEXT("ui_close"), 0.55f);   // 닫기음
    PlayOutro(); // BP가 퇴장 애니 재생
    if (UWorld* W = GetWorld())
        W->GetTimerManager().SetTimer(OutroTimer, this, &UPersonaWidgetBase::RemoveSelf, FMath::Max(0.01f, DelaySeconds), false);
    else
        RemoveSelf();
}

void UPersonaWidgetBase::RemoveSelf()
{
    RemoveFromParent();
}

bool UPersonaWidgetBase::CloseTopEscWidget(UObject* WorldContext)
{
    if (!WorldContext) return false;
    TArray<UUserWidget*> Found;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(WorldContext, Found, UPersonaWidgetBase::StaticClass(), false);
    for (UUserWidget* W : Found)
    {
        UPersonaWidgetBase* P = Cast<UPersonaWidgetBase>(W);
        if (P && P->bCloseOnEsc && P->IsInViewport())
        {
            if (P->HandleEscBack())   // 위젯이 자체 처리(예: 장면→허브) → 닫지 않음
                return true;
            P->RemoveFromParent();
            RefreshInputMode(WorldContext);   // 남은 위젯에 맞춰 입력모드 재조정
            return true;
        }
    }
    return false;
}

void UPersonaWidgetBase::RefreshInputMode(UObject* WorldContext)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContext, 0);
    if (!PC) return;
    TArray<UUserWidget*> Found;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(WorldContext, Found, UPersonaWidgetBase::StaticClass(), false);
    bool bNeedCursor = false;
    for (UUserWidget* W : Found)
    {
        UPersonaWidgetBase* P = Cast<UPersonaWidgetBase>(W);
        if (P && P->bWantsCursor && P->IsInViewport()) { bNeedCursor = true; break; }
    }
    if (bNeedCursor)
        UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, nullptr, EMouseLockMode::DoNotLock, false);
    else
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
    PC->bShowMouseCursor = bNeedCursor;
}
