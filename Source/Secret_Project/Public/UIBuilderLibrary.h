#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UIBuilderLibrary.generated.h"

class UWidget;
class UWidgetBlueprint;

/**
 * ★ 위젯 블루프린트 트리 빌더 (에디터 전용) — build_all_ui.py가 호출.
 * UE5.7이 Python에서 WidgetTree/RootWidget 접근을 protected로 막아 자식 자동배치가 불가해짐.
 * → C++에서 트리를 조작해 위젯을 생성/부착/루트설정/컴파일. 파이썬은 이 함수들만 호출.
 * 에디터 빌드에서만 동작(런타임/패키징에선 무동작 스텁).
 */
UCLASS()
class SECRET_PROJECT_API UUIBuilderLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // BP의 WidgetTree에 WidgetClass 위젯을 Name으로 생성(변수로 표시 → BindWidget 매칭). 생성된 위젯 반환.
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static UWidget* AddWidget(UWidgetBlueprint* BP, UClass* WidgetClass, FName Name);

    // Parent(패널)에 Child 부착.
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void AddChild(UWidget* Parent, UWidget* Child);

    // 트리 루트 위젯 설정.
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetRoot(UWidgetBlueprint* BP, UWidget* Root);

    // 이미 루트가 있는가(=이미 디자인된 위젯 → 자동배치 생략 판단).
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static bool HasRoot(UWidgetBlueprint* BP);

    // TextBlock/Button 라벨 텍스트 설정(Button이면 자식 TextBlock 생성해 넣음).
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetWidgetText(UWidget* W, const FString& Text);

    // 위젯 BP 컴파일(변경 반영).
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void CompileBP(UWidgetBlueprint* BP);

    // 캔버스 자식의 슬롯 위치/크기/앵커/정렬 설정 (레이아웃). Anchor: 0~1 비율.
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetCanvasSlot(UWidget* Child, FVector2D Pos, FVector2D Size,
                              FVector2D AnchorMin, FVector2D AnchorMax, FVector2D Alignment);

    // 기본 가시성: true면 Collapsed(숨김 — 전투 서브패널 등), false면 Visible.
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetCollapsed(UWidget* W, bool bCollapsed);

    // 트리 전체 비우기(루트/모든 위젯 제거) — 깨끗한 재생성용.
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void ClearTree(UWidgetBlueprint* BP);

    // Border/Image/Button 등의 색 설정. Border=배경브러시색, Image=틴트, Button=배경색.
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetColor(UWidget* W, FLinearColor Color);

    // TextBlock 스타일: 글자색 + 폰트크기(<=0이면 유지) + 정렬(ETextJustify: 0=Left,1=Center,2=Right,<0이면 유지).
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetTextStyle(UWidget* W, FLinearColor Color, int32 FontSize, int32 Justify);

    // Border 안쪽 여백(패딩).
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetBorderPadding(UWidget* W, float Left, float Top, float Right, float Bottom);

    // 캔버스 자식을 전체화면으로(앵커 0~1, 오프셋 0) — 딤/배경용.
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetCanvasFullScreen(UWidget* W);

    // 입력 통과(상시 HUD가 게임 클릭을 막지 않게). bChildrenInteractive=true면
    // SelfHitTestInvisible(자식 버튼은 클릭 가능), false면 HitTestInvisible(전체 통과).
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetHitTestInvisible(UWidget* W, bool bChildrenInteractive);

    // TextBlock 그림자(가독성). 어떤 배경에서도 글자가 또렷해짐.
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetTextShadow(UWidget* W, FLinearColor ShadowColor, FVector2D Offset);

    // Button 3상태 틴트(Normal/Hovered/Pressed) — 마우스 반응감.
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetButtonColors(UWidget* W, FLinearColor Normal, FLinearColor Hovered, FLinearColor Pressed);

    // 자식 슬롯의 패딩(여백) — 박스/스크롤/버튼 슬롯 공통. 간격·버튼높이 조정.
    UFUNCTION(BlueprintCallable, Category = "UIBuilder")
    static void SetSlotPadding(UWidget* Child, float Left, float Top, float Right, float Bottom);
};
