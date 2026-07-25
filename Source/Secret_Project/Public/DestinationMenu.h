#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "GameFlowSubsystem.h"   // FTravelDistrict

class APlayerController;

/**
 * 목적지 이동 맵 — 페르소나식 구역 선택 화면. Slate로 직접 그린다(위젯 에셋 0).
 *
 * 라셀의 갈 수 있는 구역을 목록으로 보여주고, 고르면 그 구역으로 옮긴다(GameFlow->TravelToLevel).
 * 스토리 진행은 건드리지 않는다 — 그냥 장소만 옮긴다(딴 구역에 서면 GameMode가 자유 탐험으로 맞이한다).
 *
 * T 키로 열고, 다시 T나 Esc·"닫기"로 닫는다. STitleMenu와 같은 결(뒤 거리는 그대로 보이고 왼쪽에 어두운 천).
 */
class SECRET_PROJECT_API SDestinationMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDestinationMenu) {}
        SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Player)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    /** 화면에 올린다. 이미 떠 있으면 걷어낸다(T 토글). */
    static void Toggle(APlayerController* PC);

    /** 지금 떠 있나 — 입력 토글 판단용. */
    static bool IsOpen();

protected:
    virtual FReply OnKeyDown(const FGeometry& Geo, const FKeyEvent& Key) override;
    virtual bool SupportsKeyboardFocus() const override { return true; }

private:
    void Dismiss();
    TSharedRef<SWidget> MakeRow(const FTravelDistrict& D, int32 Index);
    void OnRowClicked(int32 Index);

    TWeakObjectPtr<APlayerController> Player;
    TArray<FTravelDistrict> Districts;
    int32 HoverIndex = -1;
    bool bTaken = false;
};
