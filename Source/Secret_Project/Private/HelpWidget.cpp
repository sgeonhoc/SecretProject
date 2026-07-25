#include "HelpWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UHelpWidget* UHelpWidget::OpenHelp(APlayerController* PC, TSubclassOf<UHelpWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    UHelpWidget* W = CreateWidget<UHelpWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(60);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void UHelpWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Btn_Close) Btn_Close->OnClicked.AddDynamic(this, &UHelpWidget::OnCloseClicked);

    // BP에서 Txt_Help를 채워두지 않았으면 기본 안내문 표시
    if (Txt_Help && Txt_Help->GetText().IsEmpty())
    {
        const FString Help =
            TEXT("[ 조작 안내 ]\n")
            TEXT("이동: WASD / 점프: Space / 달리기: Shift\n")
            TEXT("상호작용: E — NPC 대화·상점·영입, 보물상자, 표지판·메모(읽을거리), 잠긴문(열쇠), 채집물, 소원의 샘\n")
            TEXT("시스템 메뉴: 인벤토리/장비/퀘스트/상태/인연/도감/발견/도전과제/사회스탯/은행/제작/빠른이동/저장·불러오기\n")
            TEXT("전투(턴제 페르소나): 약점을 찌르면 1 More! / 속성·상태이상·각성기를 활용하세요\n")
            TEXT("휴식(세이브 포인트): 진행 저장 + 회복, 취침형은 다음날로\n")
            TEXT("\n[ 한울시 ]\n")
            TEXT("낮=일상·인연·준비 / 밤=이면·전투. 밤엔 적이 강해지고 일부 상점은 문을 닫습니다.\n")
            TEXT("거리의 표지판·신문·메모를 읽으면 '한울시'의 진실에 다가갑니다. 인연을 깊게 할수록 동료가 강해지고 이야기가 열립니다.\n")
            TEXT("힘들 땐, 혼자 짊어지지 마세요. — 약함은, 서로 기대는 이유니까.");
        Txt_Help->SetText(FText::FromString(Help));
    }
}

void UHelpWidget::OnCloseClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
