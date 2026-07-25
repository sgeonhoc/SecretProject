#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class APlayerController;

/**
 * 시작 화면 차림표 — Slate로 직접 그린다.
 *
 * 왜 위젯 에셋(WBP)을 안 쓰나: 지금 WBP_MainMenu는 옛 시험판의 이름표(REVERIE·환영의 도시)와
 * 화면을 가로지르는 분홍 사선을 달고 있어, 뒤에 깔린 밤거리를 덮어 버린다. 글자와 칸만 있으면 되는
 * 화면이라 코드로 그리는 편이 고치기도 쉽고 에셋 의존이 0이 된다.
 *
 * 뒤 배경은 레벨(밤 아랫장터)이 그대로 보인다 — 왼쪽에만 어두운 천을 깔아 글자가 읽히게 한다.
 *
 * ★이름표는 아직 정해진 제목이 아니다. 세계의 도시 이름(라셀)을 임시로 쓴다 — 제목은 사용자 몫.
 */
class SECRET_PROJECT_API STitleMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STitleMenu) {}
        SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Player)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    /** 화면에 올린다. 이어하기는 저장된 진행이 있을 때만 눌린다. */
    static void Show(APlayerController* PC);

private:
    /** 화면에서 걷어낸다(레벨을 옮기기 전에 반드시 — 안 그러면 다음 화면 위에 그대로 남는다). */
    void Dismiss();

    TSharedRef<SWidget> MakeItem(const FText& Label, int32 Index, bool bEnabled);
    void OnItemClicked(int32 Index);

    TWeakObjectPtr<APlayerController> Player;
    int32 HoverIndex = -1;
    bool bContinueEnabled = false;
    bool bTaken = false;   // 한 번 고르면 더 안 받는다(두 번 눌러 두 번 떠나는 일 방지)
};
