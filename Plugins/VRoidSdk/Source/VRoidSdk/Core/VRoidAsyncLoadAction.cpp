// Copyright © 2023 pixiv Inc. All rights reserved.

#include "VroidAsyncLoadAction.h"
#include "Rendering/SkeletalMeshRenderData.h"

#include "LoaderBPFunctionLibrary.h"
#include "VrmAssetListObject.h"

#include "VRoidAsyncAsset.h"
#include "VRoidConverter.h"
#include "Game/VRoidFunctionLibrary.h"
#include "Core/VRoidDefinitions.h"
#include "Core/VRoidLogger.h"

FVRoidAsyncLoadAction::FVRoidAsyncLoadAction(const FLatentActionInfo& LatentInfo, const FVRoidAsyncLoadActionParam& InParam)
	: LatentActionInfo(LatentInfo),
	  Param(InParam),
	  AsyncFileReadTask(Async(EAsyncExecution::ThreadPool, [] { return true; })),
	  AsyncAsset(MakeShared<VRoidAsyncAsset, ESPMode::ThreadSafe>())
{
}

void FVRoidAsyncLoadAction::UpdateOperation(FLatentResponse& Response)
{
	static double StartTime = 0.f;
	static double DeltaTime = 0.f;
	constexpr auto Log = [](const FString& Str)
	{
		if (CVarVRoidRuntimeLoadVerboseLog.GetValueOnAnyThread() == false)
		{
			return;
		}
		const double MsLoadTime = (FPlatformTime::Seconds() - StartTime) * 1000.0;
		const double Delta = MsLoadTime - DeltaTime < 0.0 ? 0.0 : MsLoadTime - DeltaTime;
		VROID_LOG(TEXT("VRoid async load time=%02.2lf ms (delta: %05.2lf ms), ESequence::%s"), MsLoadTime, Delta, *Str);
		DeltaTime = MsLoadTime;
	};
	const bool TaskCompleted(TaskRef.IsValid() && TaskRef->IsComplete());
	const int TexCountValue = TexCounter.GetValue();
	const int ImageCountValue = ImageCounter.GetValue();
	static int TexCount = 0;
	static int SubCount = 0;

	// Waits to synchronize the VRM version and switches if necessary.
	if (WaitAndSyncVrmVersion(Response))
	{
		return;
	}
	// async file load
	switch (SequenceStatus)
	{
	case EVRoidSequence::Init:
		TexCounter.Reset();
		ImageCounter.Reset();
		ElapsedTime = 0.0f;
		TexCount = 0;
		SubCount = 0;
		ImageTaskArray.Empty();
		TextureTaskArray.Empty();
		StartTime = FPlatformTime::Seconds();
		DeltaTime = FPlatformTime::Seconds();
		Log(TEXT("Init."));
		TaskRef = FFunctionGraphTask::CreateAndDispatchWhenReady
		(
			[this]
			{
				Param.OutVrmAsset = Cast<UVrmAssetListObject>(StaticDuplicateObject(Param.InVrmAsset, GetTransientPackage(), NAME_None));
			}, TStatId(), nullptr, ENamedThreads::GameThread
		);
		SequenceStatus = EVRoidSequence::ReadFile;
		return;

	case EVRoidSequence::ReadFile:
		if (TaskCompleted)
		{
			Log(TEXT("ReadFile."));
			TaskRef = FFunctionGraphTask::CreateAndDispatchWhenReady
			(
				[this]
				{
					UVRoidFunctionLibrary::ReadVrmFileFromPath(Param.InVrmAsset, Param.FilePath, Param.Plain);
				}, TStatId(), nullptr, ENamedThreads::AnyBackgroundHiPriTask
			);
			SequenceStatus = EVRoidSequence::ReadFileMemory;
		}
		return;

	case EVRoidSequence::ReadFileMemory:
		if (TaskCompleted)
		{
			Log(TEXT("ReadFileMemory."));
			if (Param.Plain.IsEmpty() || Param.Plain.Num() <= 0)
			{
				SequenceStatus = EVRoidSequence::Finish;
				return;
			}
			TaskRef = FFunctionGraphTask::CreateAndDispatchWhenReady
			(
				[this]
				{
					AsyncAsset->ReadVrmFromMemory(Param.FilePath, Param.Plain.GetData(), Param.Plain.Num(), Param.OutVrmAsset, AsyncFileReadTask);
				}, TStatId(), nullptr, ENamedThreads::AnyBackgroundHiPriTask
			);
			SequenceStatus = EVRoidSequence::ReadFileLoop;
		}
		return;

	case EVRoidSequence::ReadFileLoop:
		if (TaskCompleted == false || AsyncAsset->IsValidScene() == false)
		{
			return;
		}
		if (AsyncFileReadTask.IsValid())
		{
			if (AsyncFileReadTask.IsReady() == false)
			{
				return;
			}
		}
		ImageTaskArray.SetNum(AsyncAsset->GetTextureNum());
		TextureTaskArray.SetNum(AsyncAsset->GetTextureNum());
		TaskRef = nullptr;
		AsyncFileReadTask = {};
		SequenceStatus = Param.IsFastConverter ? EVRoidSequence::LoadImage : EVRoidSequence::TextureLoop;
		return;

	case EVRoidSequence::LoadImage:
		if (Param.IsFastConverter && ImageCountValue < static_cast<int>(AsyncAsset->GetTextureNum()))
		{
			if (ImageTaskArray[ImageCountValue].IsValid() == false)
			{
				Log(FString::Printf(TEXT("LoadImage : ImageCount=%d."), 1 + ImageCountValue));
				ImageTaskArray[ImageCountValue] = UE::Tasks::Launch(
					TEXT("VRoidLoadImageFromMemoryTasks"), [this, ImageCountValue]
					{
						if (const auto& AITexture = AsyncAsset->GetTexture(ImageCountValue))
						{
							if (FVRoidImage Image; FVRoidImage::LoadImageFromMemory(AITexture->pcData, AITexture->mWidth, Image))
							{
								AsyncAsset->UpdateImage(ImageCountValue, Image);
							}
							else if (VRMUtil::FImportImage VrmImage; VRMLoaderUtil::LoadImageFromMemory(AITexture->pcData, AITexture->mWidth, VrmImage))
							{
								Image.Init(VrmImage.SizeX, VrmImage.SizeY, VrmImage.Format, VrmImage.RawData.GetData());
								AsyncAsset->UpdateImage(ImageCountValue, Image);
							}
						}
						ImageCounter.Increment();
					}, LowLevelTasks::ETaskPriority::BackgroundHigh
				);
			}
			return;
		}
		SequenceStatus = EVRoidSequence::TextureLoop;
		return;

	case EVRoidSequence::TextureLoop:
		// FIXME: Starting from UE5.4, the flow for parallel rendering and bindless rendering has changed, so the generation of RHITexture has been temporarily disabled.
		// Once the bug fixes are completed, it will be made available for use in UE5.4 and later.
		if (Param.IsFastConverter)
		{
			for (const auto Task : ImageTaskArray)
			{
				if (Task.IsValid() == false || Task.IsCompleted() == false)
				{
					Log(TEXT("LoadImageLoop."));
					return;
				}
			}
			if (TexCountValue < static_cast<int>(AsyncAsset->GetTextureNum()))
			{
				if (TextureTaskArray[TexCountValue].IsValid() == false)
				{
					Log(FString::Printf(TEXT("TextureLoop : TextureCount=%d."), 1 + TexCountValue));
#if UE_OLDER_5_4
					TextureTaskArray[TexCountValue] = UE::Tasks::Launch(
						TEXT("VRoidConvertTextureTasks"), [this, TexCountValue]
						{
							AsyncAsset->ConvertTexture(TexCountValue, 0, Param.OutVrmAsset, true);
							TexCounter.Increment();
						}, LowLevelTasks::ETaskPriority::BackgroundNormal
					);
#else
					AsyncAsset->ConvertTexture(TexCountValue, 0, Param.OutVrmAsset, true);
					TexCounter.Increment();
#endif // UE_OLDER_5_4
				}
				return;
			}
		}
		else
		{
			if (TexCount < static_cast<int>(AsyncAsset->GetTextureNum()))
			{
				if (SubCount == 0)
				{
					AsyncAsset->ConvertTexture(TexCount, SubCount, Param.OutVrmAsset, false);
				}
				if (SubCount == 2)
				{
					AsyncAsset->ConvertTexture(TexCount, 1, Param.OutVrmAsset, false);
				}
				++SubCount;

				if (SubCount >= 4)
				{
					Log(FString::Printf(TEXT("TextureLoop : TextureCount=%d."), 1 + TexCount));
					++TexCount;
					SubCount = 0;
				}
				return;
			}
		}
#if UE_OLDER_5_4
		if (Param.IsFastConverter)
		{
			for (const auto Task : TextureTaskArray)
			{
				if (Task.IsValid() == false || Task.IsCompleted() == false)
				{
					return;
				}
			}
		}
#endif // UE_OLDER_5_4
		SequenceStatus = Param.IsFastConverter ? EVRoidSequence::ConvertAsset : EVRoidSequence::CreateAsset;
		return;

	case EVRoidSequence::CreateAsset:
		Log(TEXT("CreateAsset."));
		ULoaderBPFunctionLibrary::LoadVRMFileFromMemory(Param.InVrmAsset, Param.OutVrmAsset, Param.FilePath, Param.Plain.GetData(), Param.Plain.Num());
		SequenceStatus = EVRoidSequence::Finish;
		return;

	case EVRoidSequence::ConvertAsset:
		Log(FString::Printf(TEXT("ConvertVrm, ConvertSequence::%s"), *AsyncAsset->GetSequenceName()));
		if (TaskRef.IsValid() == false)
		{
			TaskRef = FFunctionGraphTask::CreateAndDispatchWhenReady
			(
				[this]
				{
					AsyncAsset->ConvertVrm(Param.Plain.GetData(), Param.Plain.Num(), Param.OutVrmAsset);
				},
				TStatId(), nullptr, ENamedThreads::AnyBackgroundHiPriTask
			);
			return;
		}
		if (TaskCompleted == false)
		{
			return;
		}
		if (AsyncAsset->IsSequenceFinished())
		{
			TexCounter.Reset();
			TextureTaskArray.Reset();
			TextureTaskArray.SetNum(AsyncAsset->GetTextureNum());
			SequenceStatus = EVRoidSequence::CreateAndBindRHITexture;
			return;
		}
		TaskRef = nullptr;
		return;

	case EVRoidSequence::CreateAndBindRHITexture:
		if (Param.OutVrmAsset == nullptr || Param.OutVrmAsset->Textures.Num() <= 0)
		{
			SequenceStatus = EVRoidSequence::Finish;
			return;
		}
		if (TexCountValue < static_cast<int>(AsyncAsset->GetTextureNum()))
		{
			if (TextureTaskArray[TexCountValue].IsValid() == false && Param.OutVrmAsset->Textures[TexCountValue] != nullptr)
			{
				Log(FString::Printf(TEXT("CreateAndBindRHITexture : TextureCount=%d."), 1 + TexCountValue));
				TextureTaskArray[TexCountValue] = UE::Tasks::Launch(
					TEXT("VRoidCreateAndBindRHITextureTasks"), [this, TexCountValue]
					{
						if (const auto Converter = AsyncAsset->GetVroidConverter())
						{
							Converter->CreateAndBindRHITextureToResource(Param.OutVrmAsset->Textures[TexCountValue]);
						}
						TexCounter.Increment();
					}, LowLevelTasks::ETaskPriority::BackgroundNormal
				);
			}
			return;
		}
		for (const auto Task : TextureTaskArray)
		{
			if (Task.IsValid() == false || Task.IsCompleted() == false)
			{
				return;
			}
		}
		SequenceStatus = EVRoidSequence::Finish;
		return;

	case EVRoidSequence::Finish:
	default:
		if (const auto SkeletalMesh = Param.OutVrmAsset->SkeletalMesh; SkeletalMesh)
		{
			if (const auto RenderingResource = SkeletalMesh->GetResourceForRendering();
				RenderingResource && RenderingResource->LODRenderData.Num() > 0)
			{
				if (auto& Lod0 = RenderingResource->LODRenderData[0]; Lod0.BuffersSize <= 0)
				{
					VROID_WARNING(TEXT("Failed to initialize the SkeletalMesh, leaving it incomplete."));
#if 1
					SkeletalMesh->PostLoad();
#else
#if WITH_EDITOR
					SkeletalMesh->Build();
#else
					for (auto& RenderSection : Lod0.RenderSections)
					{
						RenderSection.DuplicatedVerticesBuffer.DupVertData.SetNum(1);
					}
#if	UE_OLDER_5_4
					Lod0.InitResources(false, 0, SkeletalMesh->GetMorphTargets(), SkeletalMesh);
#else
                    TArray<UMorphTarget*> MorphTargets;
                    for (auto& MorphTarget : SkeletalMesh->GetMorphTargets())
                    {
                    	MorphTargets.Add(MorphTarget);
                    }
#if UE_OLDER_5_7
					Lod0.InitResources(false, 0, MorphTargets, SkeletalMesh);
#else
					Lod0.InitResources(false, 0, SkeletalMesh);
#endif // UE_OLDER_5_7
#endif // UE_OLDER_5_4
#endif // WITH_EDITOR
#endif
				}
			}
		}

		Log(TEXT("Finish."));
		TaskRef = nullptr;
		Response.FinishAndTriggerIf(true, LatentActionInfo.ExecutionFunction, LatentActionInfo.Linkage, LatentActionInfo.CallbackTarget);
		AsyncAsset->Reset();
		break;
	}
}

bool FVRoidAsyncLoadAction::WaitAndSyncVrmVersion(const FLatentResponse& Response)
{
	ElapsedTime += Response.ElapsedTime();
	// VROID_ERROR(TEXT("経過Time=%05.2lf (delta: %05.2lf ms)"), ElapsedTime, Response.ElapsedTime());
	
	// Check if the file read for the VRM we want to load has been completed.
	if (AsyncAsset == nullptr || AsyncAsset->IsValidScene() == false)
	{
		return false;
	}
	// Skip version check until texture creation since there is no significant difference between VRM0 and VRM10 at this stage.
	if (SequenceStatus < EVRoidSequence::CreateAsset)
	{
		return false;
	}
	if (AsyncAsset->IsVrm10() != VRMConverter::Options::Get().IsVRM10Model())
	{
		// FIXME: Currently hardcoded to wait for 5 seconds, but ideally the VRM version should be updated and resumed as soon as other latent actions are completed.
		if (constexpr float Duration = 5.0f;
			ElapsedTime > Duration)
		{
			ElapsedTime = 0.0f;
			// Update the VRM version only if the loaded VRM version differs from the current one.
			if (AsyncAsset->IsVrm10())
			{
				VRMConverter::Options::Get().SetVRM10Model(true);
			}
			else
			{
				VRMConverter::Options::Get().SetVRM0Model(true);
			}
			return false;
		}
		return true;
	}
	return false;
}
