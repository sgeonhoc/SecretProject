#include "MenuFlowWidget.h"
#include "MenuScenes.h"
#include "MenuActionButton.h"
#include "UIRuntime.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "StatComponent.h"
#include "TimeComponent.h"
#include "SecretSaveGame.h"
#include "EquipmentComponent.h"
#include "BankComponent.h"
#include "CraftingComponent.h"
#include "FastTravelPointActor.h"
#include "AssetResolver.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

namespace
{
    // 버튼 색(일반 액션 / 카테고리 / 와이프)
    const FLinearColor C_BtnN(0.16f, 0.19f, 0.30f, 1.f), C_BtnH(0.30f, 0.36f, 0.54f, 1.f), C_BtnP(0.10f, 0.12f, 0.20f, 1.f);
    const FLinearColor C_CatN(0.06f, 0.07f, 0.11f, 0.45f), C_CatH(0.86f, 0.18f, 0.24f, 0.95f), C_CatP(0.50f, 0.10f, 0.14f, 1.f);
    const FLinearColor C_Crimson(0.86f, 0.18f, 0.24f, 1.f), C_Dark(0.02f, 0.02f, 0.05f, 1.f);

    void TxtStyle(UTextBlock* T, FLinearColor Col, int32 Size, int32 Justify)
    {
        if (!T) return;
        T->SetColorAndOpacity(FSlateColor(Col));
        if (Size > 0) { FSlateFontInfo F = T->GetFont(); F.Size = Size; T->SetFont(F); }
        if (Justify >= 0) T->SetJustification(static_cast<ETextJustify::Type>(Justify));
        T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
        T->SetShadowOffset(FVector2D(1.f, 1.f));
    }
}

UMenuFlowWidget* UMenuFlowWidget::OpenMenu(APlayerController* PC, TSubclassOf<UMenuFlowWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UMenuFlowWidget* W = CreateWidget<UMenuFlowWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(100);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UMenuFlowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!bBuilt && RootCanvas)
    {
        BuildBackdrop();   // z0 배경 옵아트
        BuildHub();        // z1 우측 콘텐츠(허브)
        BuildStageShell(); // z1 우측 콘텐츠(장면)
        BuildWipes();      // z2 전환 와이프
        BuildCharSlot();   // z3 좌측 "상시" 캐릭터 무대(와이프 위 → 전환 중에도 보임, 모든 화면 공통)
        bBuilt = true;
    }
    bInScene = false;
    CurrentScene = NAME_None;
    bWiping = false;
    SysMsg.Reset();
    ShowWipes(false);
    ShowStage(false);
    ShowHub(true);
    RefreshHubInfo();
    HubSettle = 1.05f;            // 인트로(약 0.93s) 정착 동안 hover 슬라이드 보류
    HoveredCatIndex = -1;
    for (float& S : CatSlideCur) S = 0.f;
    StaggerIntro(HubCats);   // 카테고리 순차 등장
    ComposeScene(NAME_None);            // 허브 캐릭터 위치(좌)
    TriggerCharReaction(TEXT("intro")); // 열 때 캐릭터 반응
}

void UMenuFlowWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const float W = MyGeometry.GetLocalSize().X;
    if (W > 1.f) CachedScreenW = W;

    if (bWiping)
    {
        WipeElapsed += InDeltaTime;
        const float P = FMath::Clamp(WipeElapsed / FMath::Max(0.01f, WipeDur), 0.f, 1.f);
        UpdateWipe(P);
        if (!bSwapped && P >= 0.5f) { bSwapped = true; ApplyView(PendingScene); }
        if (P >= 1.f) { bWiping = false; ShowWipes(false); }
    }

    // 카테고리 hover 슬라이드(P5 시그니처: 강조 항목이 옆으로 튀어나옴) — 인트로 정착 후
    if (HubSettle > 0.f) HubSettle -= InDeltaTime;
    else if (!bInScene && !bWiping)
    {
        const float Speed = FMath::Clamp(InDeltaTime * 12.f, 0.f, 1.f);
        for (int32 i = 0; i < CatButtons.Num(); ++i)
        {
            if (!CatButtons[i] || !CatSlideCur.IsValidIndex(i)) continue;
            const float Target = (i == HoveredCatIndex) ? 26.f : 0.f;
            CatSlideCur[i] = FMath::Lerp(CatSlideCur[i], Target, Speed);
            CatButtons[i]->SetRenderTranslation(FVector2D(-CatSlideCur[i], 0.f));
        }
    }

    // 스테이지 진입 연출(제목 슬라이드인 + 밑줄 좌→우 와이프)
    if (bStageEntry)
    {
        StageEntryT += InDeltaTime;
        const float a = FMath::Clamp(StageEntryT / 0.30f, 0.f, 1.f);
        const float e = 1.f - FMath::Pow(1.f - a, 3.f);
        if (StageTitle)
        {
            StageTitle->SetRenderOpacity(e);
            StageTitle->SetRenderTranslation(FVector2D(-44.f * (1.f - e), 0.f));
        }
        if (StageAccentBar)
        {
            FWidgetTransform T; T.Translation = FVector2D::ZeroVector; T.Scale = FVector2D(e, 1.f); T.Shear = FVector2D::ZeroVector; T.Angle = 0.f;
            StageAccentBar->SetRenderTransform(T);
        }
        if (a >= 1.f) bStageEntry = false;
    }

    // 배경 옵아트 드리프트 + 캐릭터 존 아이들 스웨이(애니 들어올 자리도 살아있게)
    BgTime += InDeltaTime;
    const float Drift = FMath::Sin(BgTime * 0.55f) * 7.f;
    for (int32 i = 0; i < BgSlabs.Num(); ++i)
        if (BgSlabs[i]) BgSlabs[i]->SetRenderTranslation(FVector2D(Drift * ((i % 2) ? 1.f : -1.f), 0.f));
    // 상호작용 반응 펄스(플레이스홀더 — 실제 애니는 PlayCharReaction로 BP 연결) + 아이들 스웨이. 캐릭터 통째로.
    float CharScale = 1.f;
    if (CharReactT >= 0.f)
    {
        CharReactT += InDeltaTime;
        const float ra = FMath::Clamp(CharReactT / 0.35f, 0.f, 1.f);
        CharScale = 1.f + 0.07f * FMath::Sin(ra * PI);
        if (ra >= 1.f) CharReactT = -1.f;
    }
    if (CharStage)
    {
        const float Sway = FMath::Sin(BgTime * 0.9f) * 5.f;
        CharStage->SetRenderTranslation(FVector2D(0.f, Sway));
        CharStage->SetRenderScale(FVector2D(CharScale, CharScale));
    }
}

bool UMenuFlowWidget::HandleEscBack()
{
    if (bWiping) return true;                            // 전환 중엔 무시(소비)
    if (bInScene) { BeginWipe(NAME_None); return true; } // 장면 → 허브
    return false;                                        // 허브 → CloseTopEscWidget가 닫음
}

// ───────────────────────── 구성 ─────────────────────────
UCanvasPanelSlot* UMenuFlowWidget::PlaceFill(UWidget* W, float MinX, float MinY, float MaxX, float MaxY,
                                             float InL, float InT, float InR, float InB, float AlignX, float AlignY)
{
    if (!RootCanvas || !W) return nullptr;
    UCanvasPanelSlot* S = RootCanvas->AddChildToCanvas(W);
    if (!S) return nullptr;
    S->SetAnchors(FAnchors(MinX, MinY, MaxX, MaxY));
    S->SetOffsets(FMargin(InL, InT, InR, InB));
    S->SetAlignment(FVector2D(AlignX, AlignY));
    S->SetAutoSize(false);
    return S;
}

void UMenuFlowWidget::BuildHub()
{
    // 초대형 기울인 타이틀(좌상단, 위로 살짝 bleed)
    HubTitle = WidgetTree->ConstructWidget<UTextBlock>();
    HubTitle->SetText(FText::FromString(TEXT("MENU")));
    TxtStyle(HubTitle, FLinearColor(0.97f, 0.97f, 1.0f, 1.f), 96, 0);
    HubTitle->SetRenderTransformAngle(-6.f);
    PlaceFill(HubTitle, 0.04f, 0.015f, 0.66f, 0.21f);

    // 정보(작게, 좌하단 비대칭)
    HubInfo = WidgetTree->ConstructWidget<UVerticalBox>();
    HubInfo->SetRenderTransformAngle(-3.f);
    PlaceFill(HubInfo, 0.05f, 0.75f, 0.42f, 0.985f);

    // 카테고리(우측, 큰 글씨 + 계단식 사선 + 기울임)
    HubCats = WidgetTree->ConstructWidget<UVerticalBox>();
    PlaceFill(HubCats, 0.50f, 0.09f, 0.975f, 0.98f);

    static const struct { const TCHAR* Tag; const TCHAR* Label; } Cats[] = {
        { TEXT("BAG"), TEXT("가방") },        { TEXT("EQUIP"), TEXT("장비") },     { TEXT("STATUS"), TEXT("상태") },
        { TEXT("QUEST"), TEXT("퀘스트") },    { TEXT("SOCIAL"), TEXT("사회 스탯") },{ TEXT("BOND"), TEXT("인연") },
        { TEXT("BESTIARY"), TEXT("적 도감") },{ TEXT("DISCOVERY"), TEXT("지역") }, { TEXT("ACHIEVE"), TEXT("도전과제") },
        { TEXT("CRAFT"), TEXT("제작") },      { TEXT("BANK"), TEXT("은행") },      { TEXT("TRAVEL"), TEXT("빠른 이동") },
        { TEXT("SYSTEM"), TEXT("시스템") },   { TEXT("HELP"), TEXT("도움말") },
    };
    CatButtons.Reset();
    for (int32 i = 0; i < UE_ARRAY_COUNT(Cats); ++i)
    {
        UMenuActionButton* B = MakeCatButton(Cats[i].Label, i, FName(Cats[i].Tag));
        if (UVerticalBoxSlot* S = HubCats->AddChildToVerticalBox(B))
            S->SetPadding(FMargin(0.f, 2.f, float(i) * 9.f, 2.f));   // 아래로 갈수록 우측여백↑ = 사선 계단 배치
        B->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
        B->SetRenderTransformAngle(-4.f);                            // 살짝 기울임
        CatButtons.Add(B);
    }
    CatSlideCur.Init(0.f, CatButtons.Num());

    // 우상단 닫기(✕)
    UMenuActionButton* X = WidgetTree->ConstructWidget<UMenuActionButton>();
    {
        UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
        T->SetText(FText::FromString(TEXT("✕")));
        TxtStyle(T, FLinearColor(0.97f, 0.97f, 1.0f, 1.f), 26, 1);
        X->SetContent(T);
        if (UButtonSlot* BS = Cast<UButtonSlot>(T->Slot)) BS->SetPadding(FMargin(12.f, 6.f, 12.f, 6.f));
        StyleButton(X, FLinearColor(0.f, 0.f, 0.f, 0.f), C_Crimson, C_CatP);
        X->Init(0, TEXT("CLOSE"));
        X->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnCategoryClicked);
    }
    PlaceFill(X, 0.93f, 0.03f, 0.99f, 0.10f);

    HubWidgets.Add(HubTitle);
    HubWidgets.Add(HubInfo);
    HubWidgets.Add(HubCats);
    HubWidgets.Add(X);
}

void UMenuFlowWidget::BuildBackdrop()
{
    // 항상 보이는 옵아트 배경: 크림슨/흰/검정 큰 사선 슬랩 (가는 줄무늬 아님)
    BgSlabs.Reset();
    struct FSlab { float MinX, MinY, MaxX, MaxY, Angle; FLinearColor Col; };
    static const FSlab Slabs[] = {
        { 0.00f, 0.00f, 0.80f, 0.40f,  -20.f, FLinearColor(0.86f, 0.18f, 0.24f, 1.00f) }, // 큰 크림슨 슬랩(상)
        { 0.00f, 0.35f, 1.00f, 0.40f,  -20.f, FLinearColor(0.97f, 0.97f, 1.00f, 0.85f) }, // 흰 사선 라인
        { 0.00f, 0.43f, 1.00f, 0.82f,  -20.f, FLinearColor(0.045f,0.045f,0.07f, 1.00f) }, // 검정 밴드(중하)
        { 0.30f, 0.00f, 1.00f, 0.045f, -20.f, FLinearColor(0.86f, 0.18f, 0.24f, 1.00f) }, // 크림슨 얇은 띠(상)
        { 0.00f, 0.88f, 0.64f, 0.94f,  -20.f, FLinearColor(0.86f, 0.18f, 0.24f, 0.90f) }, // 크림슨 띠(하)
    };
    for (const FSlab& D : Slabs)
    {
        UBorder* B = WidgetTree->ConstructWidget<UBorder>();
        B->SetBrushColor(D.Col);
        PlaceFill(B, D.MinX, D.MinY, D.MaxX, D.MaxY);
        B->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
        B->SetRenderTransformAngle(D.Angle);
        BgSlabs.Add(B);
    }
}

void UMenuFlowWidget::BuildCharSlot()
{
    // 캐릭터 무대(한 덩어리 = CharStage) — 장면마다 PlaceChar로 통째 재배치(여러 위치). 화면 아래로 bleed.
    //   추후 /Game/Art/Illust(또는 Portraits)/T_MenuHub 넣으면 CharArt에 자동 표시.
    CharStage = WidgetTree->ConstructWidget<UOverlay>();
    CharStage->SetRenderTransformPivot(FVector2D(0.5f, 1.f));
    CharStageSlot = PlaceFill(CharStage, 0.0f, 0.10f, 0.46f, 1.0f, 0.f, 0.f, 0.f, -70.f);

    CharFrame = WidgetTree->ConstructWidget<UBorder>();
    CharFrame->SetBrushColor(FLinearColor(0.86f, 0.18f, 0.24f, 0.15f));   // 은은한 크림슨 존(아트 자리 암시)
    if (UOverlaySlot* OS = CharStage->AddChildToOverlay(CharFrame))
    { OS->SetHorizontalAlignment(HAlign_Fill); OS->SetVerticalAlignment(VAlign_Fill); OS->SetPadding(FMargin(14.f, 28.f, 14.f, 0.f)); }

    CharWatermark = WidgetTree->ConstructWidget<UTextBlock>();
    CharWatermark->SetText(FText::FromString(TEXT("CHARACTER")));
    TxtStyle(CharWatermark, FLinearColor(1.f, 1.f, 1.f, 0.05f), 56, 1);
    CharWatermark->SetRenderTransformAngle(-90.f);
    if (UOverlaySlot* OS = CharStage->AddChildToOverlay(CharWatermark))
    { OS->SetHorizontalAlignment(HAlign_Center); OS->SetVerticalAlignment(VAlign_Center); }

    CharArt = WidgetTree->ConstructWidget<UImage>();
    if (UOverlaySlot* OS = CharStage->AddChildToOverlay(CharArt))
    { OS->SetHorizontalAlignment(HAlign_Fill); OS->SetVerticalAlignment(VAlign_Fill); }
    UTexture2D* Tex = UAssetResolver::ResolveIllust(FName(TEXT("MenuHub")));
    if (!Tex) Tex = UAssetResolver::ResolvePortrait(FName(TEXT("MenuHub")));
    if (Tex) { CharArt->SetBrushFromTexture(Tex); CharArt->SetVisibility(ESlateVisibility::HitTestInvisible); }
    else CharArt->SetVisibility(ESlateVisibility::Collapsed);   // 없으면 워터마크만(추후 파일 넣으면 표시)

    // ★ 캐릭터 무대는 "상시"(허브/장면 모든 화면 공통) → HubWidgets/StageWidgets 어디에도 안 넣음(절대 안 숨김).
}

void UMenuFlowWidget::BuildStageShell()
{
    // 장면 가독성용 어둠막(볼드 배경 위 → 카드/글씨 읽힘, 단 배경 그래픽도 비치게 약하게).
    StageScrim = WidgetTree->ConstructWidget<UBorder>();
    StageScrim->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.05f, 0.46f));
    PlaceFill(StageScrim, 0.f, 0.f, 1.f, 1.f);
    StageWidgets.Add(StageScrim);

    // 상단 큰 사선 액센트 슬랩(장면마다 그 기능 색) — 장면별 볼드 정체성
    StageSlab = WidgetTree->ConstructWidget<UBorder>();
    StageSlab->SetBrushColor(C_Crimson);
    StageSlab->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    StageSlab->SetRenderTransformAngle(-20.f);
    PlaceFill(StageSlab, 0.0f, 0.0f, 0.72f, 0.205f);
    StageWidgets.Add(StageSlab);

    UMenuActionButton* Back = WidgetTree->ConstructWidget<UMenuActionButton>();
    {
        UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
        T->SetText(FText::FromString(TEXT("◀  뒤로")));
        TxtStyle(T, UIColor::Title, 20, 1);
        Back->SetContent(T);
        if (UButtonSlot* BS = Cast<UButtonSlot>(T->Slot)) BS->SetPadding(FMargin(16.f, 8.f, 16.f, 8.f));
        StyleButton(Back, C_BtnN, C_CatH, C_CatP);
        Back->Init(0, TEXT("BACK"));
        Back->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnCategoryClicked);
    }
    PlaceFill(Back, 0.02f, 0.04f, 0.125f, 0.105f);

    StageTitle = WidgetTree->ConstructWidget<UTextBlock>();
    TxtStyle(StageTitle, FLinearColor(0.97f, 0.97f, 1.0f, 1.f), 48, 0);   // 흰색, 초대형
    StageTitle->SetRenderTransformAngle(-5.f);

    StageAccentBar = WidgetTree->ConstructWidget<UBorder>();   // (현재 미사용 — 제목이 액센트 역할). 참조 보존 위해 생성만.
    StageAccentBar->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
    PlaceFill(StageAccentBar, 0.0f, 0.0f, 0.01f, 0.01f);
    StageAccentBar->SetVisibility(ESlateVisibility::Collapsed);

    StageHint = WidgetTree->ConstructWidget<UTextBlock>();
    StageHint->SetText(FText::FromString(TEXT("ESC · 뒤로")));
    TxtStyle(StageHint, UIColor::Dim, 14, 2);
    PlaceFill(StageHint, 0.70f, 0.045f, 0.975f, 0.105f);

    // 콘텐츠 묶음(제목 + 스크롤 = ContentBox) — 장면마다 PlaceContent로 통째 재배치(캐릭터 반대편)
    StageScroll = WidgetTree->ConstructWidget<UScrollBox>();
    StageList = WidgetTree->ConstructWidget<UVerticalBox>();
    StageScroll->AddChild(StageList);

    ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
    if (UVerticalBoxSlot* TitleS = ContentBox->AddChildToVerticalBox(StageTitle)) TitleS->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
    if (UVerticalBoxSlot* ScrollS = ContentBox->AddChildToVerticalBox(StageScroll))
    { FSlateChildSize Sz(ESlateSizeRule::Fill); Sz.Value = 1.f; ScrollS->SetSize(Sz); }
    ContentBoxSlot = PlaceFill(ContentBox, 0.49f, 0.13f, 0.975f, 0.96f);

    StageWidgets.Add(Back);
    StageWidgets.Add(StageHint);
    StageWidgets.Add(ContentBox);
}

void UMenuFlowWidget::BuildWipes()
{
    WipeBack = WidgetTree->ConstructWidget<UBorder>();
    WipeBack->SetBrushColor(C_Dark);
    PlaceFill(WipeBack, 0.f, 0.f, 1.f, 1.f);

    WipeFront = WidgetTree->ConstructWidget<UBorder>();
    WipeFront->SetBrushColor(C_Crimson);
    PlaceFill(WipeFront, 0.f, 0.f, 1.f, 1.f);

    WipeFlash = WidgetTree->ConstructWidget<UBorder>();   // 전환 중간점 화이트 플래시(임팩트)
    WipeFlash->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
    PlaceFill(WipeFlash, 0.f, 0.f, 1.f, 1.f);
}

void UMenuFlowWidget::RefreshHubInfo()
{
    if (!HubInfo) return;
    UUIRuntime::Clear(HubInfo);
    APawn* P = Player();
    UStatComponent* St = Stat();

    // 정보는 보조 — 작고 비대칭으로(페르소나 허브는 정보보다 그래픽이 주연)
    if (UTimeComponent* T = (P ? P->FindComponentByClass<UTimeComponent>() : nullptr))
        UUIRuntime::AddText(HubInfo, T->GetTimeLabel(), C_Crimson, 18, 0, 2.f);
    if (St)
    {
        UUIRuntime::AddText(HubInfo, FString::Printf(TEXT("Lv %d"), St->GetLevel()), FLinearColor(0.97f, 0.97f, 1.0f, 1.f), 42, 0, 0.f);
        UUIRuntime::AddText(HubInfo, FString::Printf(TEXT("¤ %d"), St->GetGold()), FLinearColor(0.88f, 0.90f, 0.94f, 1.f), 22, 0, 0.f);
    }
    UUIRuntime::AddText(HubInfo, USecretSaveGame::HasSave() ? TEXT("SAVE DATA") : TEXT("NO SAVE"), FLinearColor(0.6f, 0.62f, 0.68f, 1.f), 12, 0, 0.f);
}

// ───────────────────────── 전환 ─────────────────────────
void UMenuFlowWidget::BeginWipe(FName TargetScene)
{
    if (bWiping) return;
    PendingScene = TargetScene;
    PendingAccent = TargetScene.IsNone() ? C_Crimson : AccentFor(TargetScene);   // 목적지 테마색으로 쓸기
    bWiping = true;
    bSwapped = false;
    WipeElapsed = 0.f;
    ShowWipes(true);
    PlayUISound(TEXT("MenuTransition"));   // BP 훅(없으면 무동작)
    PlayUI(TEXT("ui_swipe"), 0.6f);        // P5 사선 와이프 스와이프음(C++ 직접)
}

void UMenuFlowWidget::UpdateWipe(float P)
{
    const float Sweep = CachedScreenW * 1.5f;
    auto Apply = [Sweep](UBorder* B, float pp)
    {
        if (!B) return;
        const float tx = FMath::Lerp(-Sweep, Sweep, FMath::Clamp(pp, 0.f, 1.f));
        FWidgetTransform T;
        T.Translation = FVector2D(tx, 0.f);
        T.Scale = FVector2D(1.9f, 1.9f);
        T.Shear = FVector2D::ZeroVector;
        T.Angle = 15.f;
        B->SetRenderTransform(T);
    };
    if (WipeFront) WipeFront->SetBrushColor(PendingAccent);   // 목적지 테마색 프리뷰
    Apply(WipeFront, P + 0.07f);   // 액센트가 살짝 앞서 쓸고
    Apply(WipeBack, P);            // 다크가 뒤따름
    if (WipeFlash)
    {
        const float fa = 1.f - FMath::Abs(P - 0.5f) * 2.f;   // 중간(0.5)에서 최대, 양끝 0
        WipeFlash->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, FMath::Clamp(fa, 0.f, 1.f) * 0.5f));
    }
}

void UMenuFlowWidget::ShowWipes(bool bShow)
{
    const ESlateVisibility V = bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
    if (WipeBack) WipeBack->SetVisibility(V);
    if (WipeFront) WipeFront->SetVisibility(V);
    if (WipeFlash) WipeFlash->SetVisibility(V);
    if (bShow) UpdateWipe(0.f);   // 첫 프레임에 화면 밖(좌측)으로 밀어 깜빡임 방지
}

void UMenuFlowWidget::ShowHub(bool bShow)
{
    const ESlateVisibility V = bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
    for (UWidget* W : HubWidgets) if (W) W->SetVisibility(V);
}

void UMenuFlowWidget::ShowStage(bool bShow)
{
    const ESlateVisibility V = bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
    for (UWidget* W : StageWidgets) if (W) W->SetVisibility(V);
}

void UMenuFlowWidget::ApplyView(FName Scene)
{
    if (Scene.IsNone())
    {
        bInScene = false;
        CurrentScene = NAME_None;
        ShowStage(false);
        ShowHub(true);
        RefreshHubInfo();
        HubSettle = 1.0f;
        HoveredCatIndex = -1;
        for (int32 i = 0; i < CatButtons.Num(); ++i)
        {
            if (CatButtons[i]) CatButtons[i]->SetRenderTranslation(FVector2D::ZeroVector);
            if (CatSlideCur.IsValidIndex(i)) CatSlideCur[i] = 0.f;
        }
        ComposeScene(NAME_None);             // 허브: 캐릭터 좌측
        TriggerCharReaction(TEXT("hub"));
        StaggerIntro(HubCats);
    }
    else
    {
        bInScene = true;
        CurrentScene = Scene;
        CurAccent = AccentFor(Scene);
        ShowHub(false);
        BuildScene(Scene);
        ShowStage(true);
        ComposeScene(Scene);                 // ★ 장면마다 캐릭터/콘텐츠 다른 위치
        TriggerCharReaction(Scene);          // ★ 장면마다 다른 반응 태그
        // 제목/밑줄에 그 기능 테마색 적용 + 진입 연출 시작값(0프레임 깜빡임 방지)
        if (StageTitle)
        {
            StageTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.97f, 0.97f, 1.0f, 1.f)));
            StageTitle->SetRenderOpacity(0.f);
            StageTitle->SetRenderTranslation(FVector2D(-44.f, 0.f));
        }
        if (StageSlab) StageSlab->SetBrushColor(FLinearColor(CurAccent.R, CurAccent.G, CurAccent.B, 0.92f));
        if (StageAccentBar)
        {
            StageAccentBar->SetBrushColor(CurAccent);
            FWidgetTransform T0; T0.Scale = FVector2D(0.f, 1.f); StageAccentBar->SetRenderTransform(T0);
        }
        bStageEntry = true;
        StageEntryT = 0.f;
        StaggerIntro(StageList);
    }
}

void UMenuFlowWidget::RebuildScene()
{
    if (!bInScene) return;
    BuildScene(CurrentScene);
    StaggerIntro(StageList);
    TriggerCharReaction(CurrentScene);   // 상호작용(장착/예치/제작 등)마다 캐릭터 반응
}

void UMenuFlowWidget::CloseMenu()
{
    RemoveFromParent();
    UPersonaWidgetBase::RefreshInputMode(this);
}

void UMenuFlowWidget::OnBack()
{
    if (bWiping) return;
    if (bInScene) BeginWipe(NAME_None);
    else CloseMenu();
}

void UMenuFlowWidget::OnCategoryClicked(int32 /*Index*/, FName Tag)
{
    if (bWiping) return;
    if (Tag == FName(TEXT("CLOSE"))) { CloseMenu(); return; }
    if (Tag == FName(TEXT("BACK")))  { OnBack();    return; }
    SysMsg.Reset();
    TriggerCharReaction(TEXT("select"));
    BeginWipe(Tag);
}

// ───────────────────────── 장면 ─────────────────────────
void UMenuFlowWidget::BuildScene(FName Scene)
{
    if (!StageList) return;
    UUIRuntime::Clear(StageList);
    APawn* P = Player();
    FString Title;

    if (Scene == FName(TEXT("BAG")))            Title = UMenuScenes::BuildBag(StageList, P);
    else if (Scene == FName(TEXT("STATUS")))    { SceneStatus(StageList); return; }
    else if (Scene == FName(TEXT("SOCIAL")))    Title = UMenuScenes::BuildSocial(StageList, P);
    else if (Scene == FName(TEXT("QUEST")))     Title = UMenuScenes::BuildQuests(StageList, P);
    else if (Scene == FName(TEXT("BOND")))      Title = UMenuScenes::BuildBond(StageList, P);
    else if (Scene == FName(TEXT("BESTIARY")))  Title = UMenuScenes::BuildBestiary(StageList, P);
    else if (Scene == FName(TEXT("ACHIEVE")))   Title = UMenuScenes::BuildAchievements(StageList, P);
    else if (Scene == FName(TEXT("DISCOVERY"))) Title = UMenuScenes::BuildDiscovery(StageList, P);
    else if (Scene == FName(TEXT("EQUIP")))     { SceneEquipment(StageList); return; }
    else if (Scene == FName(TEXT("BANK")))      { SceneBank(StageList);      return; }
    else if (Scene == FName(TEXT("CRAFT")))     { SceneCrafting(StageList);  return; }
    else if (Scene == FName(TEXT("TRAVEL")))    { SceneFastTravel(StageList);return; }
    else if (Scene == FName(TEXT("SYSTEM")))    { SceneSystem(StageList);    return; }
    else if (Scene == FName(TEXT("HELP")))      { SceneHelp(StageList);      return; }

    if (StageTitle) StageTitle->SetText(FText::FromString(Title));
}

void UMenuFlowWidget::SceneEquipment(UPanelWidget* Body)
{
    if (StageTitle) StageTitle->SetText(FText::FromString(TEXT("장비")));
    APawn* P = Player();
    UEquipmentComponent* Eq = P ? P->FindComponentByClass<UEquipmentComponent>() : nullptr;
    if (!Eq) { UUIRuntime::AddText(Body, TEXT("장비 정보를 찾을 수 없습니다."), UIColor::Sub, 18, 1, 0.f); return; }

    // ── 전용 레이아웃: 장착 슬롯 3칸 다이어그램(가로 균등, 큰 카드) ──
    const FLinearColor SlotBg(0.10f, 0.12f, 0.18f, 0.96f);
    const FLinearColor White(0.97f, 0.97f, 1.0f, 1.f);
    UHorizontalBox* SlotRow = UUIRuntime::AddRow(Body, 16.f);
    const struct { EEquipSlot Slot; const TCHAR* Name; } S3[] = {
        { EEquipSlot::Weapon, TEXT("무기") }, { EEquipSlot::Armor, TEXT("방어구") }, { EEquipSlot::Accessory, TEXT("장신구") } };
    for (const auto& E : S3)
    {
        UVerticalBox* SlotCard = UUIRuntime::AddCardFill(SlotRow, SlotBg);
        UUIRuntime::AddText(SlotCard, E.Name, UIColor::Accent, 15, 1, 8.f);
        const FName Cur = Eq->GetEquipped(E.Slot);
        UUIRuntime::AddText(SlotCard, Cur.IsNone() ? TEXT("— 비어있음 —") : Cur.ToString(),
                            Cur.IsNone() ? UIColor::Dim : White, 22, 1, 0.f);
    }

    // ── 보유 장비(클릭형) ──
    UUIRuntime::AddText(Body, TEXT("보유 장비 — 눌러서 장착"), UIColor::Sub, 15, 0, 6.f);
    for (int32 i = 0; i < Eq->Catalog.Num(); ++i)
    {
        const FEquipItem& It = Eq->Catalog[i];
        const bool bOn = (Eq->GetEquipped(It.Slot) == It.Id);
        const FString Lbl = FString::Printf(TEXT("%s%s   (공+%.0f 방+%.0f)"),
                                            bOn ? TEXT("● ") : TEXT(""), *It.Name, It.AtkBonus, It.DefBonus);
        UMenuActionButton* B = MakeActionButton(Body, Lbl, i, TEXT("EQUIP"));
        if (bOn) StyleButton(B, FLinearColor(0.42f, 0.12f, 0.16f, 1.f), C_Crimson, C_CatP);
        B->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnEquipClicked);
    }
}

void UMenuFlowWidget::SceneStatus(UPanelWidget* Body)
{
    if (StageTitle) StageTitle->SetText(FText::FromString(TEXT("상태")));
    UStatComponent* St = Stat();
    if (!St) { UUIRuntime::AddText(Body, TEXT("플레이어 정보 없음"), UIColor::Sub, 18, 1, 0.f); return; }

    const FLinearColor HpCol(0.92f, 0.34f, 0.32f, 1.f), SpCol(0.35f, 0.62f, 0.95f, 1.f), XpCol(0.98f, 0.86f, 0.5f, 1.f);
    const FLinearColor Sheet(0.10f, 0.12f, 0.18f, 0.96f);
    const FLinearColor White(0.97f, 0.97f, 1.0f, 1.f);

    // ── 전용 레이아웃: 좌(거대 LEVEL + HP/SP/EXP) | 우(공/방/골드) ──
    UHorizontalBox* Top = UUIRuntime::AddRow(Body, 14.f);
    UVerticalBox* L = UUIRuntime::AddCardFill(Top, Sheet);
    UUIRuntime::AddText(L, TEXT("LEVEL"), UIColor::Accent, 14, 0, 0.f);
    UUIRuntime::AddText(L, FString::Printf(TEXT("%d"), St->GetLevel()), White, 64, 0, 8.f);
    UUIRuntime::AddText(L, FString::Printf(TEXT("HP   %.0f / %.0f"), St->GetCurrentHP(), St->GetMaxHP()), UIColor::Sub, 14, 0, 2.f);
    UUIRuntime::AddBar(L, St->GetMaxHP() > 0 ? St->GetCurrentHP() / St->GetMaxHP() : 0.f, HpCol, 16.f, 8.f);
    UUIRuntime::AddText(L, FString::Printf(TEXT("SP   %.0f / %.0f"), St->GetCurrentSP(), St->GetMaxSP()), UIColor::Sub, 14, 0, 2.f);
    UUIRuntime::AddBar(L, St->GetMaxSP() > 0 ? St->GetCurrentSP() / St->GetMaxSP() : 0.f, SpCol, 16.f, 8.f);
    UUIRuntime::AddText(L, FString::Printf(TEXT("EXP  %d / %d"), St->GetCurrentXP(), St->GetXPToNext()), UIColor::Sub, 13, 0, 2.f);
    UUIRuntime::AddBar(L, St->GetXPToNext() > 0 ? (float)St->GetCurrentXP() / St->GetXPToNext() : 1.f, XpCol, 12.f, 0.f);

    UVerticalBox* R = UUIRuntime::AddCardFill(Top, Sheet);
    UUIRuntime::AddText(R, TEXT("공격"), UIColor::Sub, 14, 0, 0.f);
    UUIRuntime::AddText(R, FString::Printf(TEXT("%.0f"), St->GetAttack()), White, 34, 0, 10.f);
    UUIRuntime::AddText(R, TEXT("방어"), UIColor::Sub, 14, 0, 0.f);
    UUIRuntime::AddText(R, FString::Printf(TEXT("%.0f"), St->GetDefense()), White, 34, 0, 10.f);
    UUIRuntime::AddText(R, FString::Printf(TEXT("¤ %d"), St->GetGold()), UIColor::Accent, 18, 0, 0.f);

    // ── 세부 스탯(있으면) 전폭 바 ──
    if (St->GetSTR() > 0.f || St->GetMAG() > 0.f)
    {
        UVerticalBox* C = UUIRuntime::AddCard(Body, UIColor::Card, 0.f);
        UUIRuntime::AddText(C, TEXT("세부 스탯"), UIColor::Accent, 16, 0, 6.f);
        const float Vals[5] = { St->GetSTR(), St->GetMAG(), St->GetVIT(), St->GetAGI(), St->GetLUK() };
        const TCHAR* Names[5] = { TEXT("힘 STR"), TEXT("마력 MAG"), TEXT("체력 VIT"), TEXT("민첩 AGI"), TEXT("운 LUK") };
        float Mx = 1.f; for (float V : Vals) Mx = FMath::Max(Mx, V);
        for (int32 i = 0; i < 5; ++i)
        {
            UHorizontalBox* Rr = UUIRuntime::AddRow(C, 2.f);
            UUIRuntime::RowText(Rr, Names[i], UIColor::Sub, 15, 0, true);
            UUIRuntime::RowText(Rr, FString::Printf(TEXT("%.0f"), Vals[i]), White, 15, 2, false);
            UUIRuntime::AddBar(C, Vals[i] / Mx, UIColor::Fill, 10.f, 6.f);
        }
    }
}

void UMenuFlowWidget::OnEquipClicked(int32 Index, FName /*Tag*/)
{
    APawn* P = Player();
    if (UEquipmentComponent* Eq = (P ? P->FindComponentByClass<UEquipmentComponent>() : nullptr))
        if (Eq->Catalog.IsValidIndex(Index)) Eq->EquipById(Eq->Catalog[Index].Id);
    RebuildScene();
}

void UMenuFlowWidget::SceneBank(UPanelWidget* Body)
{
    if (StageTitle) StageTitle->SetText(FText::FromString(TEXT("은행")));
    APawn* P = Player();
    UBankComponent* Bank = P ? P->FindComponentByClass<UBankComponent>() : nullptr;
    UStatComponent* St = Stat();
    const int32 OnHand = St ? St->GetGold() : 0;
    const int32 Stored = Bank ? Bank->GetStoredGold() : 0;

    // 전용 레이아웃: 대형 잔액판(보유 | 예치금 큰 숫자 2단)
    const FLinearColor Sheet(0.10f, 0.12f, 0.18f, 0.96f);
    const FLinearColor White(0.97f, 0.97f, 1.0f, 1.f);
    UHorizontalBox* Top = UUIRuntime::AddRow(Body, 14.f);
    UVerticalBox* L = UUIRuntime::AddCardFill(Top, Sheet);
    UUIRuntime::AddText(L, TEXT("보유"), UIColor::Sub, 14, 1, 4.f);
    UUIRuntime::AddText(L, FString::Printf(TEXT("%d G"), OnHand), White, 36, 1, 0.f);
    UVerticalBox* R = UUIRuntime::AddCardFill(Top, Sheet);
    UUIRuntime::AddText(R, TEXT("예치금"), UIColor::Sub, 14, 1, 4.f);
    UUIRuntime::AddText(R, FString::Printf(TEXT("%d G"), Stored), UIColor::Accent, 36, 1, 0.f);

    UUIRuntime::AddText(Body, TEXT("입출금"), UIColor::Sub, 15, 0, 6.f);
    MakeActionButton(Body, TEXT("100 예치"),  0, TEXT("BANK"))->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnBankClicked);
    MakeActionButton(Body, TEXT("전액 예치"), 1, TEXT("BANK"))->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnBankClicked);
    MakeActionButton(Body, TEXT("100 인출"),  2, TEXT("BANK"))->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnBankClicked);
    MakeActionButton(Body, TEXT("전액 인출"), 3, TEXT("BANK"))->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnBankClicked);
}

void UMenuFlowWidget::OnBankClicked(int32 Index, FName /*Tag*/)
{
    APawn* P = Player();
    UBankComponent* Bank = P ? P->FindComponentByClass<UBankComponent>() : nullptr;
    UStatComponent* St = Stat();
    if (Bank)
    {
        switch (Index)
        {
        case 0: Bank->Deposit(100); break;
        case 1: if (St) Bank->Deposit(St->GetGold()); break;
        case 2: Bank->Withdraw(100); break;
        case 3: Bank->Withdraw(Bank->GetStoredGold()); break;
        default: break;
        }
    }
    RebuildScene();
}

void UMenuFlowWidget::SceneCrafting(UPanelWidget* Body)
{
    APawn* P = Player();
    UCraftingComponent* Craft = P ? P->FindComponentByClass<UCraftingComponent>() : nullptr;
    const int32 Num = Craft ? Craft->NumRecipes() : 0;
    if (StageTitle)
        StageTitle->SetText(FText::FromString(Num == 0 ? FString(TEXT("제작")) : FString::Printf(TEXT("제작 — 레시피 %d종"), Num)));
    if (Num == 0) { UUIRuntime::AddText(Body, TEXT("레시피가 없습니다."), UIColor::Sub, 18, 1, 0.f); return; }

    UUIRuntime::AddText(Body, TEXT("눌러서 제작 (재료 충분 시 활성)"), UIColor::Sub, 15, 0, 6.f);
    UHorizontalBox* GridRow = nullptr;
    for (int32 i = 0; i < Num; ++i)   // 2열 클릭 그리드
    {
        if (i % 2 == 0) GridRow = UUIRuntime::AddRow(Body, 6.f);
        const FString Lbl = Craft->GetRecipeLabel(i).ToString();
        UMenuActionButton* B = MakeActionButton(GridRow, Lbl, i, TEXT("CRAFT"), Craft->CanCraft(i));
        B->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnCraftClicked);
    }
}

void UMenuFlowWidget::OnCraftClicked(int32 Index, FName /*Tag*/)
{
    APawn* P = Player();
    if (UCraftingComponent* Craft = (P ? P->FindComponentByClass<UCraftingComponent>() : nullptr))
        Craft->Craft(Index);
    RebuildScene();
}

void UMenuFlowWidget::SceneFastTravel(UPanelWidget* Body)
{
    TravelDest.Reset();
    const FName CurLevel(*UGameplayStatics::GetCurrentLevelName(this, true));
    APawn* P = Player();
    const FVector PlayerLoc = P ? P->GetActorLocation() : FVector::ZeroVector;
    int32 Discovered = 0;
    UHorizontalBox* GridRow = nullptr;
    for (const FFastTravelPoint& Pt : AFastTravelPointActor::GetRegisteredPoints())
    {
        if (Pt.LevelName != CurLevel) continue;
        if (!USecretSaveGame::IsCollected(Pt.Id)) continue;
        const int32 Idx = TravelDest.Num();
        TravelDest.Add(Pt.Location);
        ++Discovered;
        const float Dist = P ? FVector::Dist(PlayerLoc, Pt.Location) : 0.f;
        const FString Lbl = FString::Printf(TEXT("%s   (%.0fm)"), *Pt.Name, Dist / 100.f);
        if (Idx % 2 == 0) GridRow = UUIRuntime::AddRow(Body, 6.f);   // 2열 클릭 그리드
        UMenuActionButton* B = MakeActionButton(GridRow, Lbl, Idx, TEXT("TRAVEL"));
        B->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnTravelClicked);
    }
    if (Discovered == 0)
        UUIRuntime::AddText(Body, TEXT("발견한 빠른 이동 지점이 없습니다."), UIColor::Sub, 18, 1, 0.f);
    if (StageTitle)
        StageTitle->SetText(FText::FromString(FString::Printf(TEXT("빠른 이동 — 목적지 %d곳"), TravelDest.Num())));
}

void UMenuFlowWidget::OnTravelClicked(int32 Index, FName /*Tag*/)
{
    if (!TravelDest.IsValidIndex(Index)) return;
    if (APawn* P = Player())
    {
        const FVector Target = TravelDest[Index] + FVector(0.f, 0.f, 50.f);
        P->SetActorLocation(Target, false, nullptr, ETeleportType::TeleportPhysics);
    }
    CloseMenu();   // 이동했으니 메뉴 닫기
}

void UMenuFlowWidget::SceneSystem(UPanelWidget* Body)
{
    if (StageTitle) StageTitle->SetText(FText::FromString(TEXT("시스템")));
    UStatComponent* St = Stat();
    const bool bHasSave = USecretSaveGame::HasSave();

    UVerticalBox* C = UUIRuntime::AddCard(Body, UIColor::Card, 12.f);
    UUIRuntime::AddText(C, FString::Printf(TEXT("Lv %d   골드 %d"), St ? St->GetLevel() : 0, St ? St->GetGold() : 0), UIColor::Title, 20, 0, 4.f);
    UUIRuntime::AddText(C, bHasSave ? TEXT("저장 데이터 있음") : TEXT("저장 데이터 없음"), UIColor::Sub, 14, 0, 0.f);
    if (!SysMsg.IsEmpty())
        UUIRuntime::AddText(Body, SysMsg, UIColor::Good, 16, 0, 6.f);

    MakeActionButton(Body, TEXT("저장"),     0, TEXT("SYS"))->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnSystemClicked);
    MakeActionButton(Body, TEXT("불러오기"), 1, TEXT("SYS"), bHasSave)->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnSystemClicked);
    MakeActionButton(Body, TEXT("새 게임"),  2, TEXT("SYS"))->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnSystemClicked);
}

void UMenuFlowWidget::OnSystemClicked(int32 Index, FName /*Tag*/)
{
    switch (Index)
    {
    case 0: // 저장
        if (UStatComponent* St = Stat()) { USecretSaveGame::SavePlayerProgression(St); SysMsg = TEXT("저장 완료"); }
        else SysMsg = TEXT("저장 실패: 플레이어를 찾을 수 없음");
        RebuildScene();
        break;
    case 1: // 불러오기
        if (USecretSaveGame::HasSave()) ReloadLevel();
        break;
    case 2: // 새 게임
        USecretSaveGame::DeleteSave();
        ReloadLevel();
        break;
    default: break;
    }
}

void UMenuFlowWidget::SceneHelp(UPanelWidget* Body)
{
    if (StageTitle) StageTitle->SetText(FText::FromString(TEXT("도움말")));
    UVerticalBox* C = UUIRuntime::AddCard(Body, UIColor::Card, 10.f);
    UUIRuntime::AddText(C, TEXT("조작"), UIColor::Accent, 20, 0, 8.f);
    UUIRuntime::AddText(C, TEXT("이동  WASD        시점  마우스        상호작용  E"), UIColor::Sub, 16, 0, 4.f, true);
    UUIRuntime::AddText(C, TEXT("메뉴 열기·닫기  M 또는 ESC"), UIColor::Sub, 16, 0, 4.f, true);
    UUIRuntime::AddText(C, TEXT("메뉴 안에서  ESC = 뒤로"), UIColor::Sub, 16, 0, 4.f, true);
    UUIRuntime::AddText(C, TEXT("전투  화면 지시에 따라 공격 / 스킬 / 방어"), UIColor::Sub, 16, 0, 0.f, true);
}

// ───────────────────────── 헬퍼 ─────────────────────────
UMenuActionButton* UMenuFlowWidget::MakeCatButton(const FString& Label, int32 Index, FName Tag)
{
    UMenuActionButton* B = WidgetTree->ConstructWidget<UMenuActionButton>();
    UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
    T->SetText(FText::FromString(Label));
    TxtStyle(T, FLinearColor(0.97f, 0.97f, 1.0f, 1.f), 28, 2);   // 흰색, 큼, 우측 정렬
    B->SetContent(T);
    if (UButtonSlot* BS = Cast<UButtonSlot>(T->Slot))
    {
        BS->SetPadding(FMargin(24.f, 7.f, 22.f, 7.f));
        BS->SetHorizontalAlignment(HAlign_Right);
        BS->SetVerticalAlignment(VAlign_Center);
    }
    // 평소 투명(흰 글씨만) → 호버=크림슨 슬랩(흰 글씨가 빨강 위로) = P5식
    StyleButton(B, FLinearColor(0.f, 0.f, 0.f, 0.f), C_Crimson, FLinearColor(0.55f, 0.10f, 0.14f, 1.f));
    B->Init(Index, Tag);
    B->OnMenuClicked.AddDynamic(this, &UMenuFlowWidget::OnCategoryClicked);
    B->OnMenuHover.AddDynamic(this, &UMenuFlowWidget::OnCatHover);
    return B;
}

UMenuActionButton* UMenuFlowWidget::MakeActionButton(UPanelWidget* Into, const FString& Label, int32 Index, FName Tag, bool bEnabled)
{
    UMenuActionButton* B = WidgetTree->ConstructWidget<UMenuActionButton>();
    UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
    T->SetText(FText::FromString(Label));
    TxtStyle(T, UIColor::Title, 18, 0);
    B->SetContent(T);
    if (UButtonSlot* BS = Cast<UButtonSlot>(T->Slot))
    {
        BS->SetPadding(FMargin(16.f, 10.f, 16.f, 10.f));
        BS->SetHorizontalAlignment(HAlign_Left);
        BS->SetVerticalAlignment(VAlign_Center);
    }
    StyleButton(B, C_BtnN, C_BtnH, C_BtnP);
    B->SetIsEnabled(bEnabled);
    B->Init(Index, Tag);
    if (Into)
    {
        if (UVerticalBox* VB = Cast<UVerticalBox>(Into))
        {
            if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(B)) S->SetPadding(FMargin(0.f, 3.f, 0.f, 3.f));
        }
        else if (UHorizontalBox* HB = Cast<UHorizontalBox>(Into))   // 2열 클릭 그리드 지원(균등 채움)
        {
            if (UHorizontalBoxSlot* S = HB->AddChildToHorizontalBox(B))
            {
                FSlateChildSize Sz(ESlateSizeRule::Fill); Sz.Value = 1.f; S->SetSize(Sz);
                S->SetPadding(FMargin(4.f, 3.f, 4.f, 3.f));
            }
        }
        else Into->AddChild(B);
    }
    return B;
}

void UMenuFlowWidget::StyleButton(UButton* B, FLinearColor Normal, FLinearColor Hover, FLinearColor Press)
{
    if (!B) return;
    FButtonStyle S = B->GetStyle();
    S.Normal.TintColor  = FSlateColor(Normal);
    S.Hovered.TintColor = FSlateColor(Hover);
    S.Pressed.TintColor = FSlateColor(Press);
    B->SetStyle(S);
}

FLinearColor UMenuFlowWidget::AccentFor(FName Tag) const
{
    if (Tag == FName(TEXT("BAG")))       return FLinearColor(0.45f, 0.85f, 0.55f, 1.f);  // 초록
    if (Tag == FName(TEXT("EQUIP")))     return FLinearColor(0.40f, 0.62f, 0.95f, 1.f);  // 파랑
    if (Tag == FName(TEXT("STATUS")))    return FLinearColor(0.98f, 0.82f, 0.40f, 1.f);  // 금
    if (Tag == FName(TEXT("QUEST")))     return FLinearColor(0.95f, 0.70f, 0.30f, 1.f);  // 호박
    if (Tag == FName(TEXT("SOCIAL")))    return FLinearColor(0.92f, 0.45f, 0.70f, 1.f);  // 분홍
    if (Tag == FName(TEXT("BOND")))      return FLinearColor(0.95f, 0.45f, 0.55f, 1.f);  // 장미
    if (Tag == FName(TEXT("BESTIARY")))  return FLinearColor(0.66f, 0.45f, 0.95f, 1.f);  // 보라
    if (Tag == FName(TEXT("DISCOVERY"))) return FLinearColor(0.35f, 0.85f, 0.80f, 1.f);  // 청록
    if (Tag == FName(TEXT("ACHIEVE")))   return FLinearColor(0.98f, 0.86f, 0.50f, 1.f);  // 금
    if (Tag == FName(TEXT("CRAFT")))     return FLinearColor(0.96f, 0.55f, 0.30f, 1.f);  // 주황
    if (Tag == FName(TEXT("BANK")))      return FLinearColor(0.70f, 0.82f, 0.40f, 1.f);  // 연두금
    if (Tag == FName(TEXT("TRAVEL")))    return FLinearColor(0.40f, 0.80f, 0.95f, 1.f);  // 하늘
    if (Tag == FName(TEXT("SYSTEM")))    return FLinearColor(0.65f, 0.70f, 0.80f, 1.f);  // 회청
    if (Tag == FName(TEXT("HELP")))      return FLinearColor(0.50f, 0.65f, 0.95f, 1.f);  // 파랑
    return C_Crimson;
}

void UMenuFlowWidget::OnCatHover(int32 Index, FName /*Tag*/, bool bHovered)
{
    if (bHovered) HoveredCatIndex = Index;
    else if (HoveredCatIndex == Index) HoveredCatIndex = -1;
}

APawn* UMenuFlowWidget::Player() const { return GetOwningPlayerPawn(); }

UStatComponent* UMenuFlowWidget::Stat() const
{
    APawn* P = Player();
    return P ? P->FindComponentByClass<UStatComponent>() : nullptr;
}

void UMenuFlowWidget::ReloadLevel()
{
    CloseMenu();
    const FName LevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
    UGameplayStatics::OpenLevel(this, LevelName);
}

void UMenuFlowWidget::TriggerCharReaction(FName Tag)
{
    CharReactT = 0.f;        // 플레이스홀더 반응 펄스 시작
    PlayCharReaction(Tag);   // BP 훅(실제 캐릭터 애니 — 미구현 시 무동작)
}

void UMenuFlowWidget::PlaceChar(float MinX, float MinY, float MaxX, float MaxY, float Angle)
{
    if (CharStageSlot)
    {
        CharStageSlot->SetAnchors(FAnchors(MinX, MinY, MaxX, MaxY));
        CharStageSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, -70.f));   // 아래로 bleed
    }
    if (CharStage) CharStage->SetRenderTransformAngle(Angle);
}

void UMenuFlowWidget::PlaceContent(float MinX, float MinY, float MaxX, float MaxY)
{
    if (ContentBoxSlot)
    {
        ContentBoxSlot->SetAnchors(FAnchors(MinX, MinY, MaxX, MaxY));
        ContentBoxSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
    }
}

void UMenuFlowWidget::ComposeScene(FName Scene)
{
    // ★ 장면마다 캐릭터/콘텐츠가 "다른 위치" — 여러 위치·여러 연출(일관 스타일 탈피)
    if (Scene.IsNone()) { PlaceChar(0.0f, 0.10f, 0.46f, 1.0f, -4.f); return; }   // 허브: 캐릭터 좌(콘텐츠=우측 카테고리 고정)

    // 기본: 캐릭터 좌 / 콘텐츠 우. 14종 각각 고유 위치(좌·우·좌하·우하 + 크기·각도 차등)
    float cx0 = 0.0f, cy0 = 0.16f, cx1 = 0.42f, cy1 = 1.0f, ang = -4.f;
    float tx0 = 0.44f, tx1 = 0.975f;
    const float ty0 = 0.13f, ty1 = 0.96f;
    if      (Scene == FName(TEXT("BAG")))       { cx0 = 0.00f; cy0 = 0.46f; cx1 = 0.30f; ang = -3.f; tx0 = 0.32f; tx1 = 0.975f; } // 좌하 작게 / 우 넓게
    else if (Scene == FName(TEXT("EQUIP")))     { cx0 = 0.00f; cy0 = 0.16f; cx1 = 0.40f; ang = -5.f; tx0 = 0.42f; tx1 = 0.975f; } // 좌
    else if (Scene == FName(TEXT("STATUS")))    { cx0 = 0.56f; cy0 = 0.12f; cx1 = 1.00f; ang =  4.f; tx0 = 0.03f; tx1 = 0.53f; }  // 우 / 콘텐츠 좌
    else if (Scene == FName(TEXT("QUEST")))     { cx0 = 0.64f; cy0 = 0.32f; cx1 = 1.00f; ang =  5.f; tx0 = 0.03f; tx1 = 0.61f; }  // 우하 / 좌
    else if (Scene == FName(TEXT("SOCIAL")))    { cx0 = 0.66f; cy0 = 0.30f; cx1 = 1.00f; ang =  3.f; tx0 = 0.03f; tx1 = 0.63f; }  // 우 / 좌
    else if (Scene == FName(TEXT("BOND")))      { cx0 = 0.00f; cy0 = 0.40f; cx1 = 0.30f; ang = -3.f; tx0 = 0.32f; tx1 = 0.975f; } // 좌하 / 우
    else if (Scene == FName(TEXT("BESTIARY")))  { cx0 = 0.58f; cy0 = 0.18f; cx1 = 1.00f; ang =  4.f; tx0 = 0.03f; tx1 = 0.55f; }  // 우 / 좌
    else if (Scene == FName(TEXT("DISCOVERY"))) { cx0 = 0.00f; cy0 = 0.22f; cx1 = 0.36f; ang = -4.f; tx0 = 0.38f; tx1 = 0.975f; } // 좌
    else if (Scene == FName(TEXT("ACHIEVE")))   { cx0 = 0.64f; cy0 = 0.34f; cx1 = 1.00f; ang =  5.f; tx0 = 0.03f; tx1 = 0.61f; }  // 우하 / 좌
    else if (Scene == FName(TEXT("CRAFT")))     { cx0 = 0.00f; cy0 = 0.46f; cx1 = 0.30f; ang = -3.f; tx0 = 0.32f; tx1 = 0.975f; } // 좌하 작게 / 우
    else if (Scene == FName(TEXT("BANK")))      { cx0 = 0.57f; cy0 = 0.18f; cx1 = 1.00f; ang =  4.f; tx0 = 0.03f; tx1 = 0.54f; }  // 우 / 좌
    else if (Scene == FName(TEXT("TRAVEL")))    { cx0 = 0.00f; cy0 = 0.20f; cx1 = 0.36f; ang = -4.f; tx0 = 0.38f; tx1 = 0.975f; } // 좌
    else if (Scene == FName(TEXT("SYSTEM")))    { cx0 = 0.60f; cy0 = 0.22f; cx1 = 1.00f; ang =  4.f; tx0 = 0.03f; tx1 = 0.57f; }  // 우 / 좌
    else if (Scene == FName(TEXT("HELP")))      { cx0 = 0.00f; cy0 = 0.42f; cx1 = 0.30f; ang = -3.f; tx0 = 0.32f; tx1 = 0.975f; } // 좌하 / 우
    PlaceChar(cx0, cy0, cx1, cy1, ang);
    PlaceContent(tx0, ty0, tx1, ty1);
}
