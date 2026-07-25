#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "MenuActionButton.generated.h"

// 클릭 시 자기 인덱스/태그를 알리는 신호(동적 개수 리스트용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMenuActionClicked, int32, Index, FName, Tag);
// 호버 진입/이탈 신호(인덱스+태그+호버여부) — 허브 카테고리 슬라이드 연출용
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMenuActionHovered, int32, Index, FName, Tag, bool, bHovered);

/**
 * 인덱스+태그를 들고 클릭/호버를 브로드캐스트하는 버튼.
 * 장비/제작/빠른이동/은행처럼 항목 수가 가변인 클릭형 리스트에서, 각 항목 버튼이
 * 자기 인덱스를 알리도록 한다(고정 BindWidget 핸들러의 한계 회피).
 * 사용법: NewObject/ConstructWidget로 생성 → Init(index, tag) → OnMenuClicked/OnMenuHover에 핸들러 바인딩.
 */
UCLASS()
class SECRET_PROJECT_API UMenuActionButton : public UButton
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Menu")
    FMenuActionClicked OnMenuClicked;

    UPROPERTY(BlueprintAssignable, Category = "Menu")
    FMenuActionHovered OnMenuHover;

    UPROPERTY() int32 ActionIndex = 0;
    UPROPERTY() FName  ActionTag;

    // 인덱스/태그 세팅 + 내부 OnClicked/OnHovered/OnUnhovered 바인딩(최초 1회)
    void Init(int32 InIndex, FName InTag);

private:
    UFUNCTION() void HandleClicked();
    UFUNCTION() void HandleHovered();
    UFUNCTION() void HandleUnhovered();
    bool bBound = false;
};
