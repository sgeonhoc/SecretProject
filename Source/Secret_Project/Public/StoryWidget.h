#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "StoryManager.h"   // FStoryBeat / FStoryLine
#include "StoryWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class APlayerController;
class UStoryDirectorComponent;

/**
 * 스토리 표시 위젯 — 한 비트(FStoryBeat)의 대사 줄을 순서대로 출력하고,
 * 다 보면 StoryDirector.CompleteAndContinue를 호출해 보상/플래그/저장 + 다음 비트 연쇄.
 *
 * ★ 로직/바인딩/진행 전부 C++ (그래프 작업 0). WBP는 레이아웃만:
 *   - Txt_Speaker(화자) / Txt_Line(대사, MultiLine) / Txt_Title(장면 제목, 선택)
 *   - Btn_Next(다음 줄/완료) / Btn_Skip(장면 즉시 끝내기, 선택)
 *   - Txt_Progress("3 / 8" 진행, 선택)
 * BindWidgetOptional이라 만든 것만 연결됨. UPersonaWidgetBase 상속 → 등장 시 PlayIntro 자동.
 *
 * 일반 흐름: StoryDirector가 다음 비트를 찾으면 자동으로 ShowBeat 생성(StoryDirector.bAutoShowWidget).
 * 사용자 BP 작업 불필요 — StoryDirector에 StoryWidgetClass(=WBP_Story)만 지정.
 */
UCLASS()
class SECRET_PROJECT_API UStoryWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    // 비트를 화면에 띄움. Director는 완료 콜백 대상(없어도 표시만은 됨). 생성된 위젯 반환.
    UFUNCTION(BlueprintCallable, Category = "Story")
    static UStoryWidget* ShowBeat(APlayerController* PC, TSubclassOf<UStoryWidget> WidgetClass,
                                  const FStoryBeat& Beat, UStoryDirectorComponent* Director);

protected:
    virtual void NativeConstruct() override;
    // 화면 아무 곳이나 클릭해도 다음 줄(버튼이 처리한 클릭은 여기로 안 옴 → 중복 없음).
    // → WBP에 Btn_Next가 없어도 스토리가 막히지 않음(블로킹 위젯 안전장치).
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Speaker;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Line;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Progress;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Next;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Skip;

    // 화자 초상화(/Game/Art/Portraits/T_<화자이름>) · 장면 일러스트(/Game/Art/Illust/T_<비트Id>)
    // 파일 있으면 자동 표시, 없으면 숨김. 사용자는 이름 맞춰 이미지만 넣으면 됨.
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_Portrait;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_Illust;

    // 타자기 효과 글자당 간격(초). 0 이하면 즉시 전체 표시(타이핑 없음).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    float TypingInterval = 0.035f;

private:
    UPROPERTY() FStoryBeat ActiveBeat;
    UPROPERTY() TWeakObjectPtr<UStoryDirectorComponent> OwningDirector;
    int32 LineIndex = 0;

    // 타자기 상태
    FString CurrentFullLine;
    int32 CharIndex = 0;
    bool bLineComplete = false;
    FTimerHandle TypeTimer;

    void SetBeat(const FStoryBeat& Beat, UStoryDirectorComponent* Director);
    void ShowCurrentLine();
    void Advance();          // 다음 줄 or 완료
    void CompleteLineInstant(); // 현재 줄 즉시 전체 표시(타이핑 스킵)
    void TypeNextChar();     // 타이머 콜백(한 글자씩)
    void Finish();           // 비트 완료 처리 + 닫기 (+ Director 연쇄)
    void CloseAndRestoreInput();

    UFUNCTION() void OnNextClicked();
    UFUNCTION() void OnSkipClicked();
};
