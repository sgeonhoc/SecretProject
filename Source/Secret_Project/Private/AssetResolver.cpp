#include "AssetResolver.h"
#include "Sound/SoundBase.h"
#include "Animation/AnimMontage.h"
#include "Engine/Texture2D.h"

TMap<FString, TWeakObjectPtr<UObject>> UAssetResolver::Cache;

UObject* UAssetResolver::ResolveByPath(const FString& PackagePath)
{
    if (PackagePath.IsEmpty()) return nullptr;

    // 캐시 히트
    if (TWeakObjectPtr<UObject>* Found = Cache.Find(PackagePath))
        if (Found->IsValid())
            return Found->Get();

    // /Game/Folder/AssetName  →  /Game/Folder/AssetName.AssetName 형태로 오브젝트 경로 구성
    FString ObjectPath = PackagePath;
    int32 SlashIdx;
    if (PackagePath.FindLastChar('/', SlashIdx))
    {
        const FString AssetName = PackagePath.Mid(SlashIdx + 1);
        ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
    }

    UObject* Obj = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
    Cache.Add(PackagePath, Obj);   // null도 캐시(없는 걸 반복 시도 안 하게)
    return Obj;
}

USoundBase* UAssetResolver::ResolveSFX(FName Name)
{
    if (Name.IsNone()) return nullptr;
    return Cast<USoundBase>(ResolveByPath(FString::Printf(TEXT("/Game/Audio/SFX/SFX_%s"), *Name.ToString())));
}

USoundBase* UAssetResolver::ResolveBGM(FName Name)
{
    if (Name.IsNone()) return nullptr;
    return Cast<USoundBase>(ResolveByPath(FString::Printf(TEXT("/Game/Audio/BGM/BGM_%s"), *Name.ToString())));
}

UAnimMontage* UAssetResolver::ResolveMontage(FName Name)
{
    if (Name.IsNone()) return nullptr;
    return Cast<UAnimMontage>(ResolveByPath(FString::Printf(TEXT("/Game/Anim/Montage/AM_%s"), *Name.ToString())));
}

UTexture2D* UAssetResolver::ResolvePortrait(FName Name)
{
    if (Name.IsNone()) return nullptr;
    return Cast<UTexture2D>(ResolveByPath(FString::Printf(TEXT("/Game/Art/Portraits/T_%s"), *Name.ToString())));
}

UTexture2D* UAssetResolver::ResolveIllust(FName Name)
{
    if (Name.IsNone()) return nullptr;
    return Cast<UTexture2D>(ResolveByPath(FString::Printf(TEXT("/Game/Art/Illust/T_%s"), *Name.ToString())));
}

UObject* UAssetResolver::ResolveVFX(FName Name)
{
    if (Name.IsNone()) return nullptr;
    return ResolveByPath(FString::Printf(TEXT("/Game/VFX/NS_%s"), *Name.ToString()));
}
