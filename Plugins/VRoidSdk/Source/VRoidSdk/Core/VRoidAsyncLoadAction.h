// Copyright © 2023 pixiv Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/Task.h"

extern TAutoConsoleVariable<bool> CVarVRoidRuntimeLoadVerboseLog;
class UVrmAssetListObject;

enum class EVRoidSequence : uint8
{
	Init = 0,
	ReadFile,
	ReadFileMemory,
	ReadFileLoop,
	LoadImage,
	TextureLoop,
	CreateAsset,
	ConvertAsset,
	CreateAndBindRHITexture,
	Finish,
};

class FVRoidAsyncLoadActionParam
{
public:
	const UVrmAssetListObject* InVrmAsset;
	UVrmAssetListObject*& OutVrmAsset;
	const FString FilePath;
	TArray<uint8> Plain;
	bool IsFastConverter;

	FVRoidAsyncLoadActionParam() = delete;
	FVRoidAsyncLoadActionParam(const UVrmAssetListObject* In, UVrmAssetListObject*& Out, const FString& Path, const bool InFastConverter)
		: InVrmAsset(In), OutVrmAsset(Out), FilePath(Path), Plain(TArray<uint8>()), IsFastConverter(InFastConverter)
	{
	}
};

class FVRoidAsyncLoadAction final : public FPendingLatentAction
{
	FLatentActionInfo LatentActionInfo;
	FVRoidAsyncLoadActionParam Param;
	EVRoidSequence SequenceStatus = EVRoidSequence::Init;
	FGraphEventRef TaskRef = nullptr;
	TFuture<bool> AsyncFileReadTask;
	FThreadSafeCounter ImageCounter;
	FThreadSafeCounter TexCounter;
	TArray<UE::Tasks::TTask<void>> ImageTaskArray;
	TArray<UE::Tasks::TTask<void>> TextureTaskArray;
	TSharedPtr<class VRoidAsyncAsset, ESPMode::ThreadSafe> AsyncAsset = nullptr;
	float ElapsedTime = 0.0f;

public:
	FVRoidAsyncLoadAction(const FLatentActionInfo& LatentInfo, const FVRoidAsyncLoadActionParam& InParam);
	virtual void UpdateOperation(FLatentResponse& Response) override;
	/** 
	* VRM バージョンの同期を待機し、必要に応じて切り替える 
	* ロード中の VRM アセットと現在の設定バージョンが異なる場合、一定時間経過後に自動的に切り替えます
	* Waits to synchronize the VRM version and switches if necessary.
	* Automatically switches to the appropriate version after a certain duration
	* when the currently loaded VRM asset version differs from the current setting.
	*/
	bool WaitAndSyncVrmVersion(const FLatentResponse& Response);

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		return TEXT("");
	}
#endif // WITH_EDITOR
};
