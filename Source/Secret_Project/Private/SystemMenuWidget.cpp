#include "SystemMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "StatComponent.h"
#include "SecretSaveGame.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryWidget.h"
#include "QuestLogWidget.h"
#include "StatusWidget.h"
#include "DiscoveryWidget.h"
#include "BestiaryWidget.h"
#include "BondWidget.h"
#include "FastTravelWidget.h"
#include "BankWidget.h"
#include "CraftingWidget.h"
#include "AchievementWidget.h"
#include "SocialStatsWidget.h"
#include "EquipmentWidget.h"
#include "HelpWidget.h"

USystemMenuWidget* USystemMenuWidget::OpenMenu(APlayerController* PC, TSubclassOf<USystemMenuWidget> WidgetClass)
{
    if (!PC || !WidgetClass) return nullptr;
    USystemMenuWidget* W = CreateWidget<USystemMenuWidget>(PC, WidgetClass);
    if (!W) return nullptr;
    W->AddToViewport(100);
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, W, EMouseLockMode::DoNotLock, false);
    PC->bShowMouseCursor = true;
    return W;
}

void USystemMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Save)    Btn_Save->OnClicked.AddDynamic(this, &USystemMenuWidget::OnSaveClicked);
    if (Btn_Load)    Btn_Load->OnClicked.AddDynamic(this, &USystemMenuWidget::OnLoadClicked);
    if (Btn_NewGame) Btn_NewGame->OnClicked.AddDynamic(this, &USystemMenuWidget::OnNewGameClicked);
    if (Btn_Resume)  Btn_Resume->OnClicked.AddDynamic(this, &USystemMenuWidget::OnResumeClicked);
    if (Btn_Inventory) Btn_Inventory->OnClicked.AddDynamic(this, &USystemMenuWidget::OnInventoryClicked);
    if (Btn_Quests)    Btn_Quests->OnClicked.AddDynamic(this, &USystemMenuWidget::OnQuestsClicked);
    if (Btn_Status)    Btn_Status->OnClicked.AddDynamic(this, &USystemMenuWidget::OnStatusClicked);
    if (Btn_Discovery) Btn_Discovery->OnClicked.AddDynamic(this, &USystemMenuWidget::OnDiscoveryClicked);
    if (Btn_Bestiary) Btn_Bestiary->OnClicked.AddDynamic(this, &USystemMenuWidget::OnBestiaryClicked);
    if (Btn_Bond)     Btn_Bond->OnClicked.AddDynamic(this, &USystemMenuWidget::OnBondClicked);
    if (Btn_Travel)   Btn_Travel->OnClicked.AddDynamic(this, &USystemMenuWidget::OnTravelClicked);
    if (Btn_Bank)     Btn_Bank->OnClicked.AddDynamic(this, &USystemMenuWidget::OnBankClicked);
    if (Btn_Craft)    Btn_Craft->OnClicked.AddDynamic(this, &USystemMenuWidget::OnCraftClicked);
    if (Btn_Achievements) Btn_Achievements->OnClicked.AddDynamic(this, &USystemMenuWidget::OnAchievementsClicked);
    if (Btn_Social)   Btn_Social->OnClicked.AddDynamic(this, &USystemMenuWidget::OnSocialClicked);
    if (Btn_Equipment) Btn_Equipment->OnClicked.AddDynamic(this, &USystemMenuWidget::OnEquipmentClicked);
    if (Btn_Help)      Btn_Help->OnClicked.AddDynamic(this, &USystemMenuWidget::OnHelpClicked);

    Refresh();
}

void USystemMenuWidget::Refresh()
{
    const bool bHasSave = USecretSaveGame::HasSave();

    // 세이브 없으면 불러오기 비활성
    if (Btn_Load) Btn_Load->SetIsEnabled(bHasSave);

    if (Txt_Info)
    {
        FString Info;
        if (APawn* P = GetOwningPlayerPawn())
        {
            if (UStatComponent* Stat = P->FindComponentByClass<UStatComponent>())
                Info = FString::Printf(TEXT("Lv %d   골드 %d"), Stat->GetLevel(), Stat->GetGold());
        }
        Info += bHasSave ? TEXT("\n저장 데이터 있음") : TEXT("\n저장 데이터 없음");
        Txt_Info->SetText(FText::FromString(Info));
    }
}

void USystemMenuWidget::OnSaveClicked()
{
    if (APawn* P = GetOwningPlayerPawn())
    {
        if (UStatComponent* Stat = P->FindComponentByClass<UStatComponent>())
        {
            USecretSaveGame::SavePlayerProgression(Stat);
            if (Txt_Status) Txt_Status->SetText(FText::FromString(TEXT("저장 완료")));
            Refresh();
            return;
        }
    }
    if (Txt_Status) Txt_Status->SetText(FText::FromString(TEXT("저장 실패: 플레이어를 찾을 수 없음")));
}

void USystemMenuWidget::OnLoadClicked()
{
    if (!USecretSaveGame::HasSave())
    {
        if (Txt_Status) Txt_Status->SetText(FText::FromString(TEXT("저장 데이터가 없습니다")));
        return;
    }
    // 현재 레벨 재오픈 → BeginPlay에서 세이브 자동 로드(마지막 저장 상태 복원)
    ReloadCurrentLevel();
}

void USystemMenuWidget::OnNewGameClicked()
{
    USecretSaveGame::DeleteSave();
    // 레벨 재오픈 → 세이브 없으니 기본값으로 시작
    ReloadCurrentLevel();
}

void USystemMenuWidget::OnResumeClicked()
{
    Close();
}

void USystemMenuWidget::OnInventoryClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && InventoryWidgetClass)
        UInventoryWidget::OpenInventory(PC, InventoryWidgetClass);
    RemoveFromParent(); // 하위 위젯이 입력모드/커서 유지
}

void USystemMenuWidget::OnQuestsClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && QuestLogWidgetClass)
        UQuestLogWidget::OpenQuestLog(PC, QuestLogWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::OnStatusClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && StatusWidgetClass)
        UStatusWidget::OpenStatus(PC, StatusWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::OnDiscoveryClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && DiscoveryWidgetClass)
        UDiscoveryWidget::OpenDiscovery(PC, DiscoveryWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::OnBestiaryClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && BestiaryWidgetClass)
        UBestiaryWidget::OpenBestiary(PC, BestiaryWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::OnBondClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && BondWidgetClass)
        UBondWidget::OpenBond(PC, BondWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::OnTravelClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && FastTravelWidgetClass)
        UFastTravelWidget::OpenFastTravel(PC, FastTravelWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::OnBankClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && BankWidgetClass)
        UBankWidget::OpenBank(PC, BankWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::OnCraftClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && CraftingWidgetClass)
        UCraftingWidget::OpenCrafting(PC, CraftingWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::OnEquipmentClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && EquipmentWidgetClass)
        UEquipmentWidget::OpenEquipment(PC, EquipmentWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::OnHelpClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && HelpWidgetClass)
        UHelpWidget::OpenHelp(PC, HelpWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::OnAchievementsClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && AchievementWidgetClass)
        UAchievementWidget::OpenAchievements(PC, AchievementWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::OnSocialClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && SocialStatsWidgetClass)
        USocialStatsWidget::OpenSocialStats(PC, SocialStatsWidgetClass);
    RemoveFromParent();
}

void USystemMenuWidget::ReloadCurrentLevel()
{
    Close();
    const FName LevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
    UGameplayStatics::OpenLevel(this, LevelName);
}

void USystemMenuWidget::Close()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC);
        PC->bShowMouseCursor = false;
    }
    RemoveFromParent();
}
