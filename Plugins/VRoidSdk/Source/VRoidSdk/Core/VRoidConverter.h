// Copyright © 2024 pixiv Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
//#include "VRoidConverter.generated.h"

extern TAutoConsoleVariable<bool> CVarVRoidRuntimeMeshLoadVerboseLog;
enum class EVRMImportTextureCompressType : uint8;

class VROIDSDK_API VRoidConverter final
{
	const struct aiScene* AiScene = nullptr;
	bool IsVrm10Model = false;

	static FTexturePlatformData* CreatePlatformData(const int SizeX, const int SizeY, const uint8* Data, const int Size, const EPixelFormat Format);
	static FTextureResource* CreateResourceForRenderThread(UTexture2D* const Texture, const FTextureRHIRef& RHITexture);
	static FTextureRHIRef CreateRHITexture2D(UTexture2D* const Texture);
	void ConvertTexture(class UVrmAssetListObject* const VrmAssetList) const;
	void ConvertMaterial(const class VRMConverter* const Converter, UVrmAssetListObject* const VrmAssetList) const;

public:
	explicit VRoidConverter(const aiScene* const Scene, const bool IsVrm10);
	UTexture2D* CreateTextureWithRHI(const struct aiTexture& Texture, const bool IsNormal, TArray<EVRMImportTextureCompressType>* TextureCompressTypeArray) const;
	UTexture2D* CreateTexture(struct FVRoidImage& Image, const bool IsNormal, const bool IsCreateRHITexture, TArray<EVRMImportTextureCompressType>* TextureCompressTypeArray) const;
	void CreateAndBindRHITextureToResource(UTexture2D* Texture2D) const;
	bool ConvertTextureAndMaterial(const VRMConverter* const Converter, UVrmAssetListObject* const VrmAssetList) const;
	bool ConvertMorphTarget(const UVrmAssetListObject* const VrmAssetList) const;
	void ConvertMannequin(UVrmAssetListObject* const VrmAssetList) const;

	bool ConvertModel(UVrmAssetListObject* const VrmAssetList) const;
};
