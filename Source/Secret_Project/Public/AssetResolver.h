#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AssetResolver.generated.h"

class USoundBase;
class UAnimMontage;
class UTexture2D;
class UUserWidget;

/**
 * ★ 이름 규칙 기반 에셋 해석기 (convention-over-configuration).
 *
 * 사용자가 **정해진 폴더에 정해진 이름**으로 파일만 넣으면 C++가 런타임에 자동으로 찾아 호출.
 * 에디터에서 일일이 할당할 필요 없음. 파일이 없으면 null 반환(조용히 폴백 — 게임 안 깨짐).
 *
 * 폴더/이름 규칙 (전부 /Game 아래):
 *   - 효과음(SFX)   : /Game/Audio/SFX/SFX_<name>          예) SFX_skill_fire
 *   - 배경음(BGM)   : /Game/Audio/BGM/BGM_<name>          예) BGM_battle
 *   - 몽타주(Anim)  : /Game/Anim/Montage/AM_<name>        예) AM_skill_fire
 *   - 이펙트(VFX)   : /Game/VFX/NS_<name>                 (Niagara System)
 *   - 초상/일러스트 : /Game/Art/Portraits/T_<name>  ·  /Game/Art/Illust/T_<name>
 *
 * 패키징 시에도 잡히도록 위 폴더들을 DefaultGame.ini의 DirectoriesToAlwaysCook에 등록(A가 처리).
 * PIE/에디터에선 존재하는 에셋이면 바로 로드됨.
 */
UCLASS()
class SECRET_PROJECT_API UAssetResolver : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // 이름으로 효과음 로드 (/Game/Audio/SFX/SFX_<Name>). 없으면 nullptr.
    UFUNCTION(BlueprintCallable, Category = "Assets")
    static USoundBase* ResolveSFX(FName Name);

    // 이름으로 배경음 로드 (/Game/Audio/BGM/BGM_<Name>). 없으면 nullptr.
    UFUNCTION(BlueprintCallable, Category = "Assets")
    static USoundBase* ResolveBGM(FName Name);

    // 이름으로 몽타주 로드 (/Game/Anim/Montage/AM_<Name>). 없으면 nullptr.
    UFUNCTION(BlueprintCallable, Category = "Assets")
    static UAnimMontage* ResolveMontage(FName Name);

    // 이름으로 초상화 로드 (/Game/Art/Portraits/T_<Name>). 없으면 nullptr.
    UFUNCTION(BlueprintCallable, Category = "Assets")
    static UTexture2D* ResolvePortrait(FName Name);

    // 이름으로 일러스트(컷인/이벤트씬) 로드 (/Game/Art/Illust/T_<Name>). 없으면 nullptr.
    UFUNCTION(BlueprintCallable, Category = "Assets")
    static UTexture2D* ResolveIllust(FName Name);

    // 이름으로 VFX(Niagara) 클래스 경로 반환용 — UObject로 로드(스폰은 호출측). 없으면 nullptr.
    UFUNCTION(BlueprintCallable, Category = "Assets")
    static UObject* ResolveVFX(FName Name);

    // 범용: 임의 클래스/경로 조합으로 로드(캐시). 내부 공용.
    static UObject* ResolveByPath(const FString& PackagePath);

private:
    // 이미 로드한 것 캐시 (경로→오브젝트). 반복 로드 방지.
    static TMap<FString, TWeakObjectPtr<UObject>> Cache;
};
