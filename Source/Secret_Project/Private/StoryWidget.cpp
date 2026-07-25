#include "StoryWidget.h"
#include "StoryDirectorComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "SecretGameSettings.h"   // 텍스트 속도 설정 연동
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "AssetResolver.h"        // 초상/일러스트 이름 로드

UStoryWidget* UStoryWidget::ShowBeat(APlayerController* PC, TSubclassOf<UStoryWidget> WidgetClass,
                                     const FStoryBeat& Beat, UStoryDirectorComponent* Director)
{
    if (!PC || !WidgetClass) return nullptr;
    UStoryWidget* W = CreateWidget<UStoryWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->SetBeat(Beat, Director);
    W->AddToViewport(100);   // 다른 메뉴/HUD보다 위
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UStoryWidget::SetBeat(const FStoryBeat& Beat, UStoryDirectorComponent* Director)
{
    ActiveBeat = Beat;
    OwningDirector = Director;
    LineIndex = 0;
}

void UStoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 설정의 텍스트 속도(느림/보통/빠름)를 타자기 간격에 반영
    TypingInterval = USecretGameSettings::GetTextInterval();

    if (Btn_Next) Btn_Next->OnClicked.AddDynamic(this, &UStoryWidget::OnNextClicked);
    if (Btn_Skip) Btn_Skip->OnClicked.AddDynamic(this, &UStoryWidget::OnSkipClicked);

    if (Txt_Title)
        Txt_Title->SetText(FText::FromString(ActiveBeat.Title));

    // 장면 일러스트 (/Game/Art/Illust/T_<비트Id> 있으면 표시, 없으면 숨김)
    if (Img_Illust)
    {
        if (UTexture2D* Tex = UAssetResolver::ResolveIllust(ActiveBeat.BeatId))
        { Img_Illust->SetBrushFromTexture(Tex); Img_Illust->SetVisibility(ESlateVisibility::HitTestInvisible); }
        else Img_Illust->SetVisibility(ESlateVisibility::Collapsed);
    }

    ShowCurrentLine();
}

void UStoryWidget::ShowCurrentLine()
{
    const TArray<FStoryLine>& Lines = ActiveBeat.Lines;

    // 대사가 아예 없는 비트(플래그용 등) → 곧바로 완료 처리
    if (Lines.Num() == 0)
    {
        Finish();
        return;
    }

    LineIndex = FMath::Clamp(LineIndex, 0, Lines.Num() - 1);
    const FStoryLine& L = Lines[LineIndex];

    if (Txt_Speaker) Txt_Speaker->SetText(FText::FromString(L.Speaker));

    // 화자 초상화 (/Game/Art/Portraits/T_<화자이름> 있으면 표시, 없으면 숨김)
    if (Img_Portrait)
    {
        if (UTexture2D* Tex = UAssetResolver::ResolvePortrait(FName(*L.Speaker)))
        { Img_Portrait->SetBrushFromTexture(Tex); Img_Portrait->SetVisibility(ESlateVisibility::HitTestInvisible); }
        else Img_Portrait->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (Txt_Progress)
        Txt_Progress->SetText(FText::FromString(
            FString::Printf(TEXT("%d / %d"), LineIndex + 1, Lines.Num())));

    // 타자기 효과로 대사 한 글자씩 표시
    CurrentFullLine = L.Line;
    CharIndex = 0;
    bLineComplete = false;
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(TypeTimer);

    if (!Txt_Line || TypingInterval <= 0.f || CurrentFullLine.IsEmpty())
    {
        CompleteLineInstant();   // 타이핑 끄거나 빈 줄이면 즉시 완성
        return;
    }
    Txt_Line->SetText(FText::GetEmpty());
    if (GetWorld())
        GetWorld()->GetTimerManager().SetTimer(
            TypeTimer, this, &UStoryWidget::TypeNextChar, TypingInterval, true);
}

void UStoryWidget::TypeNextChar()
{
    if (CharIndex < CurrentFullLine.Len())
    {
        ++CharIndex;
        if (Txt_Line) Txt_Line->SetText(FText::FromString(CurrentFullLine.Left(CharIndex)));
    }
    else
    {
        if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(TypeTimer);
        bLineComplete = true;
    }
}

void UStoryWidget::CompleteLineInstant()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(TypeTimer);
    CharIndex = CurrentFullLine.Len();
    if (Txt_Line) Txt_Line->SetText(FText::FromString(CurrentFullLine));
    bLineComplete = true;
}

void UStoryWidget::Advance()
{
    const int32 Last = ActiveBeat.Lines.Num() - 1;
    if (LineIndex >= Last)
    {
        Finish();   // 마지막 줄 → 완료
        return;
    }
    ++LineIndex;
    ShowCurrentLine();
}

FReply UStoryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 버튼 위 클릭은 버튼이 소비하므로 여기 안 옴 → 빈 영역 클릭만 진행으로 처리
    OnNextClicked();
    return FReply::Handled();
}

void UStoryWidget::OnNextClicked()
{
    PlayUISound(TEXT("Confirm"));

    // VN 표준 UX: 타이핑 중이면 첫 입력은 즉시 전체 표시, 완성됐으면 다음 줄로.
    if (!bLineComplete) { CompleteLineInstant(); return; }
    Advance();
}

void UStoryWidget::OnSkipClicked()
{
    PlayUISound(TEXT("Cancel"));
    Finish();   // 장면 즉시 끝내기(완료 처리는 동일)
}

void UStoryWidget::Finish()
{
    const FName BeatId = ActiveBeat.BeatId;
    UStoryDirectorComponent* Director = OwningDirector.Get();

    // 먼저 이 위젯을 닫는다 → Director가 다음 비트를 자동 표시해도 중첩되지 않음
    CloseAndRestoreInput();

    if (Director)
        Director->CompleteAndContinue(BeatId);   // 보상/플래그/저장 + 다음 비트 연쇄(자동 재표시)
}

void UStoryWidget::CloseAndRestoreInput()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(TypeTimer);   // 타이핑 타이머 정리
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
