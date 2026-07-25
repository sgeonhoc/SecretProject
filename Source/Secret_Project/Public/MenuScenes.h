#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MenuScenes.generated.h"

class UPanelWidget;
class APawn;

/**
 * M 메뉴의 "표시형" 장면(가방/상태/사회/퀘스트/인연/도감/도전과제/지역)을
 * 컨테이너(VerticalBox 등)에 카드/게이지로 채우는 정적 빌더.
 * 반환 = 해당 장면 제목(카운트 포함).
 *
 * ★ 풀스크린 허브(UMenuFlowWidget)와 (필요 시)기존 단독 위젯이 같은 로직을 공유 → 중복 제거.
 *   등장 연출(StaggerIntro)은 호출자가 담당(여긴 컨텐츠만 채움).
 */
UCLASS()
class SECRET_PROJECT_API UMenuScenes : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    static FString BuildBag(UPanelWidget* Into, APawn* P);
    static FString BuildStatus(UPanelWidget* Into, APawn* P);
    static FString BuildSocial(UPanelWidget* Into, APawn* P);
    static FString BuildQuests(UPanelWidget* Into, APawn* P);
    static FString BuildBond(UPanelWidget* Into, APawn* P);
    static FString BuildBestiary(UPanelWidget* Into, APawn* P);
    static FString BuildAchievements(UPanelWidget* Into, APawn* P);
    static FString BuildDiscovery(UPanelWidget* Into, APawn* P);
};
