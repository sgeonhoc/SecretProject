#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "SystemMenuWidget.generated.h"

class UButton;
class UTextBlock;
class APlayerController;
class UInventoryWidget;
class UQuestLogWidget;
class UStatusWidget;
class UDiscoveryWidget;
class UBestiaryWidget;
class UBondWidget;
class UFastTravelWidget;
class UBankWidget;
class UCraftingWidget;
class UAchievementWidget;
class USocialStatsWidget;
class UEquipmentWidget;
class UHelpWidget;

/**
 * 시스템 메뉴(저장/불러오기/새 게임/계속). 로직·UI 바인딩 전부 C++. WBP는 레이아웃만(reparent).
 * - 저장: 현재 플레이어 진행(레벨/스탯/골드/인벤토리)을 슬롯0에 기록(어디서든 수동 저장).
 * - 불러오기: 현재 레벨 재오픈 → BeginPlay에서 자동 로드(=마지막 저장 상태 복원).
 * - 새 게임: 세이브 삭제 후 레벨 재오픈(처음부터).
 * 버튼/텍스트는 BindWidgetOptional — BP에서 만든 것만 연결됨(이름 정확히 일치).
 */
UCLASS()
class SECRET_PROJECT_API USystemMenuWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    // 어디서든 시스템 메뉴 열기 (뷰포트 + 입력 UI 모드 + 커서). 생성된 위젯 반환.
    UFUNCTION(BlueprintCallable, Category = "SystemMenu")
    static USystemMenuWidget* OpenMenu(APlayerController* PC, TSubclassOf<USystemMenuWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Status;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Info;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Save;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Load;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_NewGame;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Resume;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Inventory;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Quests;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Status;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Discovery;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Bestiary;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Bond;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Travel;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Bank;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Craft;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Achievements;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Social;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Equipment;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Help;

    // 메뉴에서 열 하위 위젯 클래스 (BP에서 WBP_Inventory / WBP_QuestLog 할당)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UInventoryWidget> InventoryWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UQuestLogWidget> QuestLogWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UStatusWidget> StatusWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UDiscoveryWidget> DiscoveryWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UBestiaryWidget> BestiaryWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UBondWidget> BondWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UFastTravelWidget> FastTravelWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UBankWidget> BankWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UCraftingWidget> CraftingWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UAchievementWidget> AchievementWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<USocialStatsWidget> SocialStatsWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UEquipmentWidget> EquipmentWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SystemMenu")
    TSubclassOf<UHelpWidget> HelpWidgetClass;

private:
    void Refresh();
    void Close();
    void ReloadCurrentLevel();

    UFUNCTION() void OnSaveClicked();
    UFUNCTION() void OnLoadClicked();
    UFUNCTION() void OnNewGameClicked();
    UFUNCTION() void OnResumeClicked();
    UFUNCTION() void OnInventoryClicked();
    UFUNCTION() void OnQuestsClicked();
    UFUNCTION() void OnStatusClicked();
    UFUNCTION() void OnDiscoveryClicked();
    UFUNCTION() void OnBestiaryClicked();
    UFUNCTION() void OnBondClicked();
    UFUNCTION() void OnTravelClicked();
    UFUNCTION() void OnBankClicked();
    UFUNCTION() void OnCraftClicked();
    UFUNCTION() void OnAchievementsClicked();
    UFUNCTION() void OnSocialClicked();
    UFUNCTION() void OnEquipmentClicked();
    UFUNCTION() void OnHelpClicked();
};
