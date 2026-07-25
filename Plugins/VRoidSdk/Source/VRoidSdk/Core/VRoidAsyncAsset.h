// Copyright © 2025 pixiv Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "VRoidImage.h"

class UVrmAssetListObject;

UENUM()
enum class EVRoidConvertSequence : uint8
{
	Init = 0,
	ConvertTextureAndMaterial,
	ConvertModel,
	ConvertMetaPost,
	ConvertMorphTarget,
	ConvertPose,
	ConvertHumanoid,
	Finish,
};

class VRoidAsyncAsset final
{
private:
	TSharedPtr<Assimp::Importer, ESPMode::ThreadSafe> Importer = nullptr;
	TSharedPtr<aiScene, ESPMode::ThreadSafe> AiScene = nullptr;
	TSharedPtr<class VRMConverter, ESPMode::ThreadSafe> VrmConverter = nullptr;
	TSharedPtr<class VRoidConverter, ESPMode::ThreadSafe> VroidConverter = nullptr;
	bool IsVrm10Model = false;

	EVRoidConvertSequence ConvertSequenceStatus = EVRoidConvertSequence::Init;
	TArray<bool> NormalBoolTable;
	TArray<FVRoidImage> Images;

private:
	bool ConvertVrmFirst(UVrmAssetListObject*& Out) const;
	bool ConvertVrm0Meta(const uint8* const Data, const size_t Size, UVrmAssetListObject*& Out) const;
	bool ConvertVrm10Meta(const uint8* const Data, const size_t Size, UVrmAssetListObject*& Out) const;

public:
	VRoidAsyncAsset();
	~VRoidAsyncAsset();

	VRoidConverter* GetVroidConverter() const { return VroidConverter.Get(); }
	FString GetSequenceName() const;
	unsigned int GetTextureNum() const { return AiScene ? AiScene->mNumTextures : 0; }
	const aiTexture* GetTexture(const int Index) const;
	bool IsSequenceFinished() const { return ConvertSequenceStatus == EVRoidConvertSequence::Finish; }
	bool UpdateImage(const int Index, const FVRoidImage& Image);
	bool IsValidScene() const { return AiScene != nullptr; }
	bool IsVrm10() const { return IsVrm10Model; }

	void Reset();
	void ReadVrmFromMemory(const FString& FilePath, const uint8* const Data, const size_t Size, UVrmAssetListObject* const VrmAssetList, TFuture<bool>& AsyncFileReadTask);
	void ConvertTexture(const int TexCount, const int SubCount, UVrmAssetListObject* const VrmAssetList, const bool IsFastConverter);
	bool ConvertVrm(const uint8* const Data, const size_t Size, UVrmAssetListObject*& Out);
};
