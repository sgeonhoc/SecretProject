#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "StoryJournalWidget.generated.h"

class UButton;
class UTextBlock;
class APlayerController;
class UStoryWidget;

/**
 * 회상 저널 — 지금까지 본 스토리 비트(메인/인터루드/미스터리/인연)를 모아 다시보기.
 * UStoryManagerSubsystem.GetCompletedBeatIds()(카탈로그 순) 순회 → 제목 목록 + 다시보기 버튼.
 * 다시보기는 StoryWidget을 Director=nullptr로 띄움 → 완료/보상 재처리 없이 표시만(읽기 전용).
 *
 * 로직/바인딩 C++. WBP는 레이아웃만(BindWidgetOptional):
 *   - Txt_Title(헤더 "회상 N/M") / Txt_Entries(본 장면 목록)
 *   - Btn_Beat0~7 + Txt_Beat0~7 (다시보기 버튼, 본 비트 앞에서부터 최대 8개)
 *   - Btn_Close
 * 진입점은 시스템메뉴 Btn_Journal(B 파일) — 없으면 build_all_ui.py가 못 붙임(B에 요청).
 */
UCLASS()
class SECRET_PROJECT_API UStoryJournalWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Story")
    static UStoryJournalWidget* OpenJournal(APlayerController* PC, TSubclassOf<UStoryJournalWidget> WidgetClass);

    // 다시보기에 쓸 스토리 위젯 클래스(BP에서 WBP_Story 할당 — build_all_ui.py가 자동 배선)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
    TSubclassOf<UStoryWidget> StoryWidgetClass;

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Entries;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Beat0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Beat1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Beat2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Beat3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Beat4;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Beat5;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Beat6;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Beat7;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Beat0;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Beat1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Beat2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Beat3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Beat4;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Beat5;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Beat6;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Beat7;

private:
    // 다시보기 버튼(0~7)에 매핑된 비트 Id
    UPROPERTY() TArray<FName> ReplayIds;

    void Refresh();
    void Replay(int32 ButtonIndex);
    void CloseAndRestoreInput();

    UFUNCTION() void OnCloseClicked();
    UFUNCTION() void OnBeat0(); UFUNCTION() void OnBeat1(); UFUNCTION() void OnBeat2(); UFUNCTION() void OnBeat3();
    UFUNCTION() void OnBeat4(); UFUNCTION() void OnBeat5(); UFUNCTION() void OnBeat6(); UFUNCTION() void OnBeat7();
};
