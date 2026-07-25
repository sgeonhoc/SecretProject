#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/Button.h"
#include "UIRuntime.generated.h"

class UPanelWidget;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class UBorder;

// 카드 클릭 시 자기 인덱스를 알려주는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardIndexClicked, int32, Index);

/**
 * 인덱스를 들고 다니는 클릭 카드 버튼. 런타임에 NewObject로 만들어 List 컨테이너에 채운다.
 * 내부 OnClicked(무인자)을 받아 자기 CardIndex를 OnCardClicked로 브로드캐스트 →
 * 고정 슬롯(Btn_Item0..N + OnItem0..N) 없이 가변 길이 목록을 클릭형으로 처리.
 */
UCLASS()
class SECRET_PROJECT_API UCardButton : public UButton
{
    GENERATED_BODY()

public:
    UPROPERTY() int32 CardIndex = 0;

    UPROPERTY(BlueprintAssignable, Category = "UIRuntime")
    FOnCardIndexClicked OnCardClicked;

    // 인덱스 지정 + 내부 OnClicked 바인딩(생성 직후 1회 호출)
    void Init(int32 InIndex);

private:
    UFUNCTION() void HandleClicked();
};

// 메뉴 전용 UI 공용 색 팔레트(카드/텍스트/게이지)
namespace UIColor
{
    inline const FLinearColor Card  (0.12f, 0.14f, 0.20f, 0.96f); // 카드 배경
    inline const FLinearColor Title (0.97f, 0.97f, 1.00f, 1.00f); // 항목 제목(흰)
    inline const FLinearColor Sub   (0.72f, 0.76f, 0.84f, 1.00f); // 보조 텍스트(회)
    inline const FLinearColor Accent(0.98f, 0.86f, 0.50f, 1.00f); // 강조(금)
    inline const FLinearColor Good  (0.45f, 0.85f, 0.55f, 1.00f); // 달성/긍정(초록)
    inline const FLinearColor Dim   (0.45f, 0.48f, 0.55f, 1.00f); // 비활성/미발견(흐림)
    inline const FLinearColor Fill  (0.35f, 0.62f, 0.95f, 1.00f); // 게이지 기본(파랑)
}

/**
 * 런타임 UI 조립 툴킷 (패키징 포함 동작 — UIBuilderLibrary와 별개).
 * 각 메뉴 위젯이 C++에서 데이터를 읽어 카드/행/게이지/텍스트를 동적으로 만들어 컨테이너에 채운다.
 * BindWidget 고정 슬롯의 한계(항목 수 가변) 탈피 → "기능별 전용 UI"의 기반.
 */
UCLASS()
class SECRET_PROJECT_API UUIRuntime : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // 컨테이너 비우기(Refresh 시 먼저 호출)
    UFUNCTION(BlueprintCallable, Category = "UIRuntime")
    static void Clear(UPanelWidget* Panel);

    // 텍스트 추가(색/크기/정렬 + 그림자). 반환 = 만든 TextBlock.
    static UTextBlock* AddText(UPanelWidget* Parent, const FString& Text,
                               FLinearColor Color, int32 Size, int32 Justify = 0,
                               float GapBelow = 0.f, bool bAutoWrap = false);

    // 가로 행 추가. 반환 = HBox(여기에 자식을 채워 좌우 배치).
    static UHorizontalBox* AddRow(UPanelWidget* Parent, float GapBelow = 0.f);

    // 카드(둥근 배경 패널) 추가. 반환 = 카드 내부 VerticalBox(여기에 내용 채움).
    static UVerticalBox* AddCard(UPanelWidget* Parent, FLinearColor Bg, float GapBelow = 8.f);

    // HBox 자식으로 카드 추가(남은 폭 균등 채움) — 열/그리드/슬롯 레이아웃용. 반환 = 카드 내부 VBox.
    static UVerticalBox* AddCardFill(UHorizontalBox* Row, FLinearColor Bg);

    // 비례 게이지 바(트랙 + 채움). Percent 0~1. 높이 Height.
    static void AddBar(UPanelWidget* Parent, float Percent, FLinearColor Fill,
                       float Height = 16.f, float GapBelow = 6.f);

    // HBox 자식으로 텍스트 추가(좌/우 분리 레이아웃용). bFill=true면 남은 공간 차지.
    static UTextBlock* RowText(UHorizontalBox* Row, const FString& Text,
                               FLinearColor Color, int32 Size, int32 Justify, bool bFill);

    // 클릭 가능한 카드. 반환 = 카드 내부 VBox(내용 채움). OutButton = 카드 버튼
    // (OutButton->OnCardClicked 바인딩, 필요 시 SetIsEnabled로 비활성). 클릭 시 Index 보고.
    static UVerticalBox* AddClickCard(UPanelWidget* Parent, FLinearColor Bg, int32 Index,
                                      UCardButton*& OutButton, float GapBelow = 8.f);
};
