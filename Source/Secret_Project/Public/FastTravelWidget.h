#pragma once

#include "CoreMinimal.h"
#include "PersonaWidgetBase.h"
#include "FastTravelWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class APlayerController;

/**
 * 빠른 이동 화면. 발견한(IsCollected) 현재 레벨의 여행 지점을 동적 클릭 카드 목록으로 보여주고, 선택 시 텔레포트.
 * 지점 개수 무제한(List_Dests 컨테이너에 C++가 동적 생성).
 * 로직·바인딩 C++. WBP는 Txt_Title + List_Dests 컨테이너 + Btn_Close(헤더).
 */
UCLASS()
class SECRET_PROJECT_API UFastTravelWidget : public UPersonaWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "FastTravel")
    static UFastTravelWidget* OpenFastTravel(APlayerController* PC, TSubclassOf<UFastTravelWidget> WidgetClass);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Title;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Btn_Close;
    // 전용 UI: C++가 목적지 카드를 동적 생성해 채우는 컨테이너
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> List_Dests;

private:
    // 현재 표시중인 목적지 위치(카드 인덱스 = 이 배열 인덱스)
    TArray<FVector> DestLocations;

    void Refresh();
    void TravelToIndex(int32 Index);

    UFUNCTION() void OnCardClicked(int32 Index);
    UFUNCTION() void OnCloseClicked();
};
