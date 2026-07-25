#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "MenuFlowWidget.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UVerticalBox;
class UHorizontalBox;
class UScrollBox;
class UBorder;
class UTextBlock;
class UButton;
class UWidget;
class UPanelWidget;
class UMenuActionButton;
class UImage;
class UOverlay;
class UStatComponent;
class APlayerController;

/**
 * ★ 풀스크린 M 메뉴 — P5식 사선 와이프 takeover.
 *  - 허브(화면 전체): 좌측 정보(날짜/골드/Lv/HP·SP·EXP) + 우측 큰 카테고리 리스트.
 *  - 카테고리 클릭 → 붉은 사선 와이프가 화면을 쓸고 → 그 기능 "전용 풀스크린 장면"으로 전환.
 *  - ESC/뒤로: 장면→허브, 허브에서 한 번 더 → 닫기.
 *  WBP는 RootCanvas + 불투명 배경 + 사선 줄무늬만 제공. 허브/스테이지/와이프는 전부 C++ 런타임 조립.
 *  데이터 카드는 UMenuScenes(표시형) / 본 클래스 멤버(상호작용형)로 채움. 등장 연출 = StaggerIntro.
 */
UCLASS()
class SECRET_PROJECT_API UMenuFlowWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Menu")
    static UMenuFlowWidget* OpenMenu(APlayerController* PC, TSubclassOf<UMenuFlowWidget> WidgetClass);

    // ★ 상호작용마다 캐릭터 애니 재생 — 디자이너가 BP에서 실제 애니/몽타주 연결(태그=intro/select/장면 태그 등).
    //   지금은 빈 훅(아트 없음) + C++ 플레이스홀더 반응 펄스. 캐릭터 아트/애니 들어오면 여기에 얹음.
    UFUNCTION(BlueprintImplementableEvent, Category = "Menu")
    void PlayCharReaction(FName Tag);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual bool HandleEscBack() override;

    // WBP가 제공(루트 캔버스). 나머지는 C++가 이 위에 붙인다.
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCanvasPanel> RootCanvas;

private:
    // 구성(최초 1회)
    void BuildHub();
    void BuildStageShell();
    void BuildWipes();
    void BuildBackdrop();
    void BuildCharSlot();
    void RefreshHubInfo();
    void TriggerCharReaction(FName Tag);
    // 장면마다 캐릭터/콘텐츠를 다른 위치로(여러 위치·여러 연출). 캐릭터 무대를 한 덩어리로 재배치.
    void PlaceChar(float MinX, float MinY, float MaxX, float MaxY, float Angle);
    void PlaceContent(float MinX, float MinY, float MaxX, float MaxY);
    void ComposeScene(FName Scene);

    // 입력 핸들러 (UMenuActionButton::OnMenuClicked → 인덱스/태그)
    UFUNCTION() void OnCategoryClicked(int32 Index, FName Tag);
    UFUNCTION() void OnEquipClicked(int32 Index, FName Tag);
    UFUNCTION() void OnBankClicked(int32 Index, FName Tag);
    UFUNCTION() void OnCraftClicked(int32 Index, FName Tag);
    UFUNCTION() void OnTravelClicked(int32 Index, FName Tag);
    UFUNCTION() void OnSystemClicked(int32 Index, FName Tag);
    UFUNCTION() void OnCatHover(int32 Index, FName Tag, bool bHovered);

    // 전환(사선 와이프)
    void BeginWipe(FName TargetScene);
    void UpdateWipe(float P);
    void ShowWipes(bool bShow);
    void ApplyView(FName Scene);   // 와이프 중간점에서 허브<->장면 스왑
    void ShowHub(bool bShow);
    void ShowStage(bool bShow);
    void CloseMenu();
    void OnBack();
    void RebuildScene();           // 같은 장면 내용만 갱신(액션 후)

    // 장면 빌드
    void BuildScene(FName Scene);
    void SceneEquipment(UPanelWidget* Body);
    void SceneStatus(UPanelWidget* Body);
    void SceneBank(UPanelWidget* Body);
    void SceneCrafting(UPanelWidget* Body);
    void SceneFastTravel(UPanelWidget* Body);
    void SceneSystem(UPanelWidget* Body);
    void SceneHelp(UPanelWidget* Body);

    // 헬퍼
    UCanvasPanelSlot* PlaceFill(UWidget* W, float MinX, float MinY, float MaxX, float MaxY,
                                float InL = 0.f, float InT = 0.f, float InR = 0.f, float InB = 0.f,
                                float AlignX = 0.f, float AlignY = 0.f);
    UMenuActionButton* MakeCatButton(const FString& Label, int32 Index, FName Tag);
    UMenuActionButton* MakeActionButton(UPanelWidget* Into, const FString& Label, int32 Index, FName Tag, bool bEnabled = true);
    void StyleButton(UButton* B, FLinearColor Normal, FLinearColor Hover, FLinearColor Press);
    FLinearColor AccentFor(FName Tag) const;
    APawn* Player() const;
    UStatComponent* Stat() const;
    void ReloadLevel();

    // 위젯 참조
    UPROPERTY() TObjectPtr<UTextBlock> HubTitle;
    UPROPERTY() TObjectPtr<UVerticalBox> HubInfo;
    UPROPERTY() TObjectPtr<UVerticalBox> HubCats;
    UPROPERTY() TObjectPtr<UTextBlock> StageTitle;
    UPROPERTY() TObjectPtr<UTextBlock> StageHint;
    UPROPERTY() TObjectPtr<UScrollBox> StageScroll;
    UPROPERTY() TObjectPtr<UVerticalBox> StageList;
    UPROPERTY() TObjectPtr<UBorder> WipeBack;
    UPROPERTY() TObjectPtr<UBorder> WipeFront;
    UPROPERTY() TObjectPtr<UBorder> WipeFlash;
    UPROPERTY() TObjectPtr<UBorder> StageAccentBar;
    UPROPERTY() TObjectPtr<UBorder> StageSlab;
    UPROPERTY() TObjectPtr<UBorder> StageScrim;
    UPROPERTY() TObjectPtr<UImage> CharArt;
    UPROPERTY() TObjectPtr<UBorder> CharFrame;
    UPROPERTY() TObjectPtr<UTextBlock> CharWatermark;
    UPROPERTY() TObjectPtr<UOverlay> CharStage;            // 캐릭터 묶음(장면마다 통째로 재배치)
    UPROPERTY() TObjectPtr<UVerticalBox> ContentBox;       // 장면 콘텐츠 묶음(제목+스크롤)
    UPROPERTY() TObjectPtr<class UCanvasPanelSlot> CharStageSlot;
    UPROPERTY() TObjectPtr<class UCanvasPanelSlot> ContentBoxSlot;
    UPROPERTY() TArray<TObjectPtr<UBorder>> BgSlabs;
    UPROPERTY() TArray<TObjectPtr<UMenuActionButton>> CatButtons;
    UPROPERTY() TArray<TObjectPtr<UWidget>> HubWidgets;
    UPROPERTY() TArray<TObjectPtr<UWidget>> StageWidgets;

    // 상태
    bool  bBuilt = false;
    // 추가 연출 상태(카테고리 hover 슬라이드 / 스테이지 진입 / 전환 테마색)
    TArray<float> CatSlideCur;
    int32 HoveredCatIndex = -1;
    float HubSettle = 0.f;
    bool  bStageEntry = false;
    float StageEntryT = 0.f;
    float BgTime = 0.f;
    float CharReactT = -1.f;   // -1=대기, >=0 진행중(상호작용 반응 펄스)
    FLinearColor PendingAccent = FLinearColor(0.86f, 0.18f, 0.24f, 1.f);
    FLinearColor CurAccent = FLinearColor(0.86f, 0.18f, 0.24f, 1.f);
    bool  bInScene = false;
    FName CurrentScene;
    bool  bWiping = false;
    bool  bSwapped = false;
    float WipeElapsed = 0.f;
    float WipeDur = 0.5f;
    FName PendingScene;
    float CachedScreenW = 2200.f;
    FString SysMsg;
    TArray<FVector> TravelDest;
};
