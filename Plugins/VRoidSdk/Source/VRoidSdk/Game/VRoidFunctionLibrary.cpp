// Copyright © 2023 pixiv Inc. All rights reserved.

#include "VRoidFunctionLibrary.h"

#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Animation/NodeMappingContainer.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Components/Button.h"
#include "Styling/SlateBrush.h"
#include "Engine/Texture2DDynamic.h"
#if WITH_EDITOR
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Character.h"
#endif // WITH_EDITOR

#include "VrmMetaObject.h"
#include "VrmAssetListObject.h"
#include "LoaderBPFunctionLibrary.h"
#include "VrmLicenseObject.h"
#include "Vrm1LicenseObject.h"

#include "Core/VRoidDefinitions.h"
#include "Core/VRoidDataTypes.h"
#include "Library/VRoidSdk.h"
#include "Core/VRoidConverter.h"
#include "Core/VRoidAsyncLoadAction.h"
#include "Character/VRoidPlayerStateBase.h"
#include "Core/VRoidLogger.h"
#include "Core/VRoidCharacterModelLicense.h"
#include "Core/VRoidSdkCoreFunctionLibrary.h"

namespace VRoid
{
	constexpr auto ConvertText = [](const FString& Text, const bool IsAdditionalLicense = false)
	{
		const auto IsMatchText = [&Text](const FString& SubStr, const bool IsContains = false)
		{
			return (IsContains) ? Text.Contains(SubStr) : Text.Equals(SubStr, ESearchCase::IgnoreCase);
		};

		if (IsAdditionalLicense)
		{
			if (IsMatchText(TEXT("personalProfit")) ||
				IsMatchText(TEXT("allowModificationRedistribution")))
			{
				return FString(TEXT("OK"));
			}
			if (IsMatchText(TEXT("allowModification")))
			{
				return FString(TEXT("NG"));
			}
		}

		if (IsMatchText(TEXT("false")) ||
			IsMatchText(TEXT("onlyauthor")) ||
			IsMatchText(TEXT("onlySeparatelyLicensedPerson")) ||
			IsMatchText(TEXT("prohibited")) ||
			IsMatchText(TEXT("disallow")) ||
			IsMatchText(TEXT("personal"), true))
		{
			return FString(TEXT("NG"));
		}
		if (IsMatchText(TEXT("everyone")) ||
			IsMatchText(TEXT("true")) ||
			IsMatchText(TEXT("corporation")) ||
			IsMatchText(TEXT("allow"), true))
		{
			return FString(TEXT("OK"));
		}
#define LOCTEXT_NAMESPACE "VRoidFunctionLibrary"
		if (IsMatchText(TEXT("necessary")) ||
			IsMatchText(TEXT("required")))
		{
			return LOCTEXT("NecessaryKey", "必要").ToString();
		}
		if (IsMatchText(TEXT("unnecessary")))
		{
			return LOCTEXT("UnnecessaryKey", "不要").ToString();
		}
		if (IsMatchText(TEXT("profit")))
		{
			return LOCTEXT("ProfitKey", "OK - ギフティング").ToString();
		}
		if (IsMatchText(TEXT("nonprofit")))
		{
			return LOCTEXT("NonprofitKey", "OK - 同人").ToString();
		}
		if (IsMatchText(TEXT("未設定")))
		{
			return LOCTEXT("DefaultKey", "未設定").ToString();
		}
#undef LOCTEXT_NAMESPACE
		return Text;
	};

	static std::vector<UINT8> ReadBinaryFromSdk(const UObject* WorldContextObject, const FString& FilePath, const int64 FileSize)
	{
		std::vector<UINT8> Binary;
		const std::string AccessToken = UVRoidSdkCoreFunctionLibrary::LoadStoredAccessToken(WorldContextObject);
		if (AccessToken.empty())
		{
			VROID_ERROR(TEXT("The model could not be Loaded because the access token failed to load."));
			return Binary;
		}
		if (const TUniquePtr<vroid::VRoidSdk> Sdk = MakeUnique<vroid::VRoidSdk>())
		{
			if (Sdk->InitializeApi(AccessToken))
			{
				Binary.reserve(FileSize);
				Binary = Sdk->ReadVRMBinary(FStringToString(FilePath));
			}
		}
		return Binary;
	}
} // namespace VRoid

bool UVRoidFunctionLibrary::LoadVrmFileFromMemory(const UVrmAssetListObject* InVrmAsset, UVrmAssetListObject*& OutVrmAsset, const FString& FilePath, const uint8* Data, const size_t Size)
{
	if (InVrmAsset == nullptr)
	{
		VROID_ERROR(TEXT("InVrmAsset has not valid."));
		return false;
	}
	if (FilePath.IsEmpty())
	{
		VROID_ERROR(TEXT("FilePath(%s) has not valid."), *FilePath);
		return false;
	}
	if (Data == nullptr || Size <= 0 || Size != IFileManager::Get().FileSize(*FilePath))
	{
		VROID_ERROR(TEXT("Failed read vrm from File(%s)."), *FilePath);
		return false;
	}

	bool IsVrm10Model = false;
	const FString& Extension(FPaths::GetExtension(FilePath).ToLower());
	if (Extension.Compare("vrm") == 0 || Extension.Compare("glb") == 0 || Extension.Compare("gltf") == 0)
	{
		if (IsVrm10Model = IsVrm10(Data, Size);
			IsVrm10Model)
		{
			VRMConverter::Options::Get().SetVRM10Model(true);
		}
		else
		{
			VRMConverter::Options::Get().SetVRM0Model(true);
		}
	}

	const TSharedPtr<Assimp::Importer, ESPMode::ThreadSafe> Importer(MakeShared<Assimp::Importer, ESPMode::ThreadSafe>());
	Importer->SetPropertyBool(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, false);
	constexpr unsigned int Flags = (aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes | aiProcess_PopulateArmatureData);
	if (Importer->ReadFileFromMemory(Data, Size, Flags, UVRoidSdkCoreFunctionLibrary::FStringToStdString(Extension). c_str()) == nullptr)
	{
		Importer->ReadFile(UVRoidSdkCoreFunctionLibrary::FStringToStdString(FilePath), Flags);
	}
	const aiScene* ScenePtr = Importer->GetScene();
	if (ScenePtr == nullptr)
	{
		VROID_ERROR(TEXT("Failed Read File(%s)."), *FilePath);
		return false;
	}

	if (OutVrmAsset == nullptr)
	{
#if 0
		//OutVrmAsset = NewObject<UVrmAssetListObject>(GetTransientPackage(), InVrmAsset->GetClass(), NAME_None, RF_StrongRefOnFrame);
		OutVrmAsset = NewObject<UVrmAssetListObject>(GetTransientPackage());
		OutVrmAsset->SetFlags(InVrmAsset->GetFlags());
		InVrmAsset->CopyMember(OutVrmAsset);
#else
		OutVrmAsset = Cast<UVrmAssetListObject>(StaticDuplicateObject(InVrmAsset, GetTransientPackage(), NAME_None));
	}
	if (OutVrmAsset == nullptr)
	{
		VROID_ERROR(TEXT("Failed initialize OutVrmAsset."));
		return false;
#endif
	}
	const FString& BaseFileName(FPaths::GetBaseFilename(FilePath));
	OutVrmAsset->FileFullPathName = FilePath;
	OutVrmAsset->OrigFileName = BaseFileName;
	OutVrmAsset->BaseFileName = VRMConverter::NormalizeFileName(BaseFileName);
	OutVrmAsset->Package = GetTransientPackage();

	const TSharedPtr<VRMConverter, ESPMode::ThreadSafe> VrmConverter = MakeShared<VRMConverter, ESPMode::ThreadSafe>();
	const TSharedPtr<VRoidConverter, ESPMode::ThreadSafe> VroidConverter = MakeShared<VRoidConverter, ESPMode::ThreadSafe>(ScenePtr, IsVrm10Model);

	bool bRet = true;
	constexpr int32 VrmEncryptedHeaderSize = 44;
	bRet &= VrmConverter->Init(Data, Size - VrmEncryptedHeaderSize, ScenePtr);
	bRet &= VrmConverter->ConvertVrmFirst(OutVrmAsset, Data, Size);
	bRet &= VrmConverter->NormalizeBoneName(ScenePtr);
	bRet &= VroidConverter->ConvertTextureAndMaterial(VrmConverter.Get(), OutVrmAsset);
	const bool r = VrmConverter->ConvertVrmMeta(OutVrmAsset, ScenePtr, Data, Size);
	if (VRMConverter::Options::Get().IsVRMModel())
	{
		bRet &= r;
	}
	// bRet &= VrmConverter->ConvertModel(OutVrmAsset);
	bRet &= VroidConverter->ConvertModel(OutVrmAsset);
	bRet &= ConvertVrmMetaRenamedWrapper(VrmConverter.Get(), OutVrmAsset, ScenePtr, Data, Size);
	bRet &= VrmConverter->ConvertRig(OutVrmAsset);
	bRet &= VrmConverter->ConvertIKRig(OutVrmAsset);
	if (OutVrmAsset->bSkipMorphTarget == false)
	{
		// bRet &= VrmConverter->ConvertMorphTarget(OutVrmAsset);
		bRet &= VroidConverter->ConvertMorphTarget(OutVrmAsset);
	}
	bRet &= VrmConverter->ConvertPose(OutVrmAsset);
	bRet &= VrmConverter->ConvertHumanoid(OutVrmAsset);

	OutVrmAsset->MeshReturnedData = nullptr;
	if (bRet == false)
	{
		VROID_ERROR(TEXT("Failed Load VRM."));
		return false;
	}
#if 0
#if UE_NEWER_5_7
	// UE5.7 requires CreateMeshDescription to be called only after PostLoad.
	const auto GameThreadConvertTextureAndMaterialTask = MakeShared<TFunction<void()>, ESPMode::ThreadSafe>([&]
	{
		OutVrmAsset->SkeletalMesh->PostLoad(); 
	});
	RunAndWaitForGameThreadTask(GameThreadConvertTextureAndMaterialTask);
#endif // UE_NEWER_5_7
#endif
	OutVrmAsset->VrmMetaObject->SkeletalMesh = OutVrmAsset->SkeletalMesh;
	OutVrmAsset->VrmMetaObject->SkeletalMesh->SetPhysicsAsset(nullptr);
	if (OutVrmAsset->bSkipMorphTarget == false && OutVrmAsset->SkeletalMesh)
	{
		OutVrmAsset->SkeletalMesh->InitMorphTargetsAndRebuildRenderData();
	}
	return true;
}

UVrmMetaObject* UVRoidFunctionLibrary::GetVrmMetaObject(const UVrmAssetListObject* const VrmAsset, const EVRMType VrmType)
{
	if (VrmAsset == nullptr)
	{
		return nullptr;
	}

	switch (VrmType)
	{
	case EVRMType::VRM:
		if (VrmAsset->VrmMetaObject == nullptr)
		{
			VROID_ERROR(TEXT("VrmMetaObject has not found."));
			return nullptr;
		}
		return VrmAsset->VrmMetaObject;
	case EVRMType::Humanoid:
		if (VrmAsset->VrmHumanoidMetaObject == nullptr)
		{
			VROID_ERROR(TEXT("VrmHumanoidMetaObject has not found."));
			return nullptr;
		}
		return VrmAsset->VrmHumanoidMetaObject;
	case EVRMType::Mannequin:
		if (VrmAsset->VrmMannequinMetaObject == nullptr)
		{
			VROID_ERROR(TEXT("VrmMannequinMetaObject has not found."));
			return nullptr;
		}
		return VrmAsset->VrmMannequinMetaObject;
	default:
		return nullptr;
	}
}

USkeletalMesh* UVRoidFunctionLibrary::GetVrmSkeletalMesh(const UVrmAssetListObject* const VrmAsset, const EVRMType VrmType)
{
	if (VrmAsset == nullptr)
	{
		return nullptr;
	}

	if (const auto MetaObject = GetVrmMetaObject(VrmAsset, VrmType))
	{
		return MetaObject->SkeletalMesh;
	}
	return nullptr;
}

USkeleton* UVRoidFunctionLibrary::GetVrmSkeleton(const UVrmAssetListObject* const VrmAsset, const EVRMType VrmType)
{
	if (VrmAsset == nullptr)
	{
		return nullptr;
	}

	if (const auto MetaObject = GetVrmMetaObject(VrmAsset, VrmType))
	{
		return MetaObject->SkeletalMesh->GetSkeleton();
	}
	return nullptr;
}

bool UVRoidFunctionLibrary::AddVirtualBone(const USkeletalMeshComponent* const Mesh, const FName SourceBoneName, const FName TargetBoneName, const FName ReNameBoneName)
{
	const auto SkeletalMesh = Mesh->GetSkeletalMeshAsset();
	if (SkeletalMesh == nullptr)
	{
		return false;
	}
	const auto Skeleton = SkeletalMesh->GetSkeleton();
	if (Skeleton == nullptr)
	{
		return false;
	}
	for (const auto Bone : Skeleton->GetVirtualBones())
	{
		if (Bone.VirtualBoneName == ReNameBoneName)
		{
			return false;
		}
	}
	FName BoneName;
	if (Skeleton->AddNewVirtualBone(SourceBoneName, TargetBoneName, BoneName))
	{
		if (ReNameBoneName.IsValid())
		{
			Skeleton->RenameVirtualBone(BoneName, ReNameBoneName);
		}
		return true;
	}
	return false;
}

APlayerController* UVRoidFunctionLibrary::GetFirstLocalPlayerController(const UObject* const WorldContextObject)
{
	if (const auto World = WorldContextObject->GetWorld())
	{
		return GEngine->GetFirstLocalPlayerController(World);
	}
	return nullptr;
}

FString UVRoidFunctionLibrary::GetFirstLocalPlayerUniqueNetId(const UObject* const WorldContextObject)
{
	if (const auto PC = GetFirstLocalPlayerController(WorldContextObject))
	{
		if (const auto PS = PC->PlayerState)
		{
			return PS->GetUniqueId()->ToString();
		}
	}
	return TEXT("");
}

FString UVRoidFunctionLibrary::ConvertUniqueNetIdToString(const FUniqueNetIdRepl& UniqueNetId)
{
	return UniqueNetId.ToString();
}

bool UVRoidFunctionLibrary::IsAllClientsSelectedModel(const UObject* const WorldContextObject)
{
	const auto World = WorldContextObject->GetWorld();
	if (World == nullptr)
	{
		return false;
	}
	const auto GS = World->GetGameState();
	if (GS == nullptr)
	{
		return false;
	}
	for (const auto Player : GS->PlayerArray)
	{
		const auto VRoidPS = Cast<AVRoidPlayerStateBase>(Player);
		if (VRoidPS == nullptr)
		{
			continue;
		}
		if (VRoidPS->GetIsModelSelected() == false)
		{
			return false;
		}
	}
	return true;
}

bool UVRoidFunctionLibrary::ClientDestroySession(const UObject* const WorldContextObject)
{
	const auto World = WorldContextObject->GetWorld();
	if (World == nullptr)
	{
		return false;
	}
	if (const IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(World);
		OnlineSubsystem)
	{
		if (const IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
			SessionInterface.IsValid())
		{
			return SessionInterface->DestroySession(NAME_GameSession);
		}
	}
	return false;
}

bool UVRoidFunctionLibrary::LoadEncodedVRM(const UVrmAssetListObject* InVrmAsset, UVrmAssetListObject*& OutVrmAsset, const FString& FilePath, const FImportOptionData& OptionForRuntimeLoad, const bool IsFastConverter)
{
	if (TArray<uint8> Plain; ReadVrmFileFromPath(InVrmAsset, FilePath, Plain))
	{
		VRMConverter::Options::Get().SetVrmOption(&OptionForRuntimeLoad);
		if (IsFastConverter)
		{
			return LoadVrmFileFromMemory(InVrmAsset, OutVrmAsset, FilePath, Plain.GetData(), Plain.Num());
		}
		return ULoaderBPFunctionLibrary::LoadVRMFileFromMemory(InVrmAsset, OutVrmAsset, FilePath, Plain.GetData(), Plain.Num());
	}
	return false;
}

void UVRoidFunctionLibrary::AsyncLoadEncodedVRM(const UObject* WorldContextObject, const UVrmAssetListObject* InVrmAsset, UVrmAssetListObject*& OutVrmAsset, const FString& FilePath, const FImportOptionData& OptionForRuntimeLoad, const FLatentActionInfo LatentInfo, const bool IsFastConverter)
{
	if (IsFastConverter == false && FilePath.Contains(".enc") == false)
	{
		ULoaderBPFunctionLibrary::LoadVRMFileAsync(WorldContextObject, InVrmAsset, OutVrmAsset, FilePath, OptionForRuntimeLoad, LatentInfo);
		return;
	}
	VRMConverter::Options::Get().SetVrmOption(&OptionForRuntimeLoad);
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr || WorldContextObject == nullptr)
	{
		VROID_ERROR(TEXT("Failed get world from context object."));
		return;
	}
	FLatentActionManager& LatentActionManager(World->GetLatentActionManager());
	if (LatentActionManager.FindExistingAction<FVRoidAsyncLoadAction>(LatentInfo.CallbackTarget, LatentInfo.UUID))
	{
#if 0
		VROID_WARNING(TEXT("LatentAction is already in progress for this Action."));
		return;
#else
		VROID_WARNING(TEXT("LatentAction is already in progress for this Action, and the existing action will be discarded and re-registered."));
		LatentActionManager.RemoveActionsForObject(LatentInfo.CallbackTarget);
#endif
	}
	const FVRoidAsyncLoadActionParam Param(InVrmAsset, OutVrmAsset, FilePath, IsFastConverter);
	LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FVRoidAsyncLoadAction(LatentInfo, Param));
}

FVRMLicense UVRoidFunctionLibrary::ConvertLicenseText(FVRMLicense License)
{
	License.Modification = VRoid::ConvertText(License.Modification);
	License.Redistribution = VRoid::ConvertText(License.Redistribution);
	License.Credit = VRoid::ConvertText(License.Credit);
	License.CharacterizationAllowedUser = VRoid::ConvertText(License.CharacterizationAllowedUser);
	License.SexualExpression = VRoid::ConvertText(License.SexualExpression);
	License.ViolentExpression = VRoid::ConvertText(License.ViolentExpression);
	License.CorporateCommercialUse = VRoid::ConvertText(License.CorporateCommercialUse);
	License.PersonalCommercialUse = VRoid::ConvertText(License.PersonalCommercialUse);

	return License;
}

FVRMLicense10 UVRoidFunctionLibrary::ConvertLicenseText10(FVRMLicense10 License)
{
	License.AntisocialOrHateUse = VRoid::ConvertText(License.AntisocialOrHateUse);
	License.ExcessivelySexualUse = VRoid::ConvertText(License.ExcessivelySexualUse);
	License.ExcessivelyViolentUse = VRoid::ConvertText(License.ExcessivelyViolentUse);
	License.PoliticalOrReligiousUse = VRoid::ConvertText(License.PoliticalOrReligiousUse);
	License.Redistribution = VRoid::ConvertText(License.Redistribution);
	License.AvatarPermission = VRoid::ConvertText(License.AvatarPermission);
	License.CorporateCommercialUse = VRoid::ConvertText(License.CorporateCommercialUse);
	License.CommercialUse = VRoid::ConvertText(License.CommercialUse, true);
	License.Credit = VRoid::ConvertText(License.Credit);
	License.Modification = VRoid::ConvertText(License.Modification);
	License.ModificationRedistribution = VRoid::ConvertText(License.ModificationRedistribution, true);

	return License;
}

void UVRoidFunctionLibrary::UpdateButtonImageOrClear(UButton* Button, UTexture2DDynamic* DynamicImage, const ESlateBrushDrawType::Type DrawType)
{
	if (Button == nullptr)
	{
		return;
	}
	FButtonStyle Style = Button->GetStyle();
	// Sets the button's image; clears it if DynamicImage is null.
	Style.Normal.SetResourceObject(DynamicImage);
	Style.Hovered.SetResourceObject(DynamicImage);
	Style.Pressed.SetResourceObject(DynamicImage);

	const ESlateBrushDrawType::Type BrushDrawType = (DynamicImage == nullptr) ? ESlateBrushDrawType::NoDrawType : DrawType;
	Style.Normal.DrawAs = BrushDrawType;
	Style.Hovered.DrawAs = BrushDrawType;
	Style.Pressed.DrawAs = BrushDrawType;
	Button->SetStyle(Style);
}

void UVRoidFunctionLibrary::UpdateBoneName(USkeletalMesh* const TargetMesh, const FName& OldBoneName, const FName& NewBoneName)
{
#if WITH_EDITOR
	USkeleton* const targetSkeleton = TargetMesh->GetSkeleton();
	if (targetSkeleton == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("target Skeleton is NULL."));
		return;
	}
	auto& refSkeleton = const_cast<FReferenceSkeleton&>(targetSkeleton->GetReferenceSkeleton());
	auto& rawRefBoneInfo = const_cast<TArray<FMeshBoneInfo>&>(refSkeleton.GetRawRefBoneInfo());
	if (const int32 BoneIndex = refSkeleton.FindBoneIndex(OldBoneName); BoneIndex != INDEX_NONE)
	{
		rawRefBoneInfo[BoneIndex].Name = NewBoneName;
#if WITH_EDITORONLY_DATA
		rawRefBoneInfo[BoneIndex].ExportName = NewBoneName.ToString();
#endif // WITH_EDITORONLY_DATA
	}
	refSkeleton.RebuildRefSkeleton(targetSkeleton, true);
	TargetMesh->SetRefSkeleton(refSkeleton);

	const bool Dirty(targetSkeleton->MarkPackageDirty());
	(void)Dirty;
	targetSkeleton->PostEditChange();
#endif // WITH_EDITOR
}

bool UVRoidFunctionLibrary::CopyVrmSkeletonSockets(const USkeletalMeshComponent* SrcSkeletalMeshComponent, const UVrmAssetListObject* const VrmAsset)
{
	if (SrcSkeletalMeshComponent == nullptr ||
		SrcSkeletalMeshComponent->GetSkeletalMeshAsset() == nullptr ||
		SrcSkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton() == nullptr)
	{
		VROID_ERROR(TEXT("Socket copy failed because the SrcSkeleton is NULL."));
		return false;
	}
	USkeletalMesh* const DstSkeletalMesh = GetVrmSkeletalMesh(VrmAsset, EVRMType::VRM);
	if (DstSkeletalMesh == nullptr || DstSkeletalMesh->GetSkeleton() == nullptr)
	{
		VROID_ERROR(TEXT("Socket copy failed, SkeletalMesh or Skeleton is NULL (VRMType: VRM)."));
		return false;
	}
	const auto SrcSockets(SrcSkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton()->Sockets);
	const int32 SocketNum = SrcSockets.Num();
	if (SocketNum <= 0)
	{
		VROID_LOG(TEXT("The number of sockets is 0, so the copy process is skipped."));
		return true;
	}

#if 1
	TArray<USkeletalMeshSocket*> NewSockets;
	NewSockets.Reserve(SocketNum);
	USkeleton* const DstSkeleton = DstSkeletalMesh->GetSkeleton();
	const auto DstRefSkeleton = DstSkeleton->GetReferenceSkeleton();
	for (const auto& Socket : SrcSockets)
	{
		if (DstRefSkeleton.FindBoneIndex(Socket->BoneName) != INDEX_NONE)
		{
#if 0
			NewSockets.Add(Socket);
#else
			USkeletalMeshSocket* const DstSocket = DuplicateObject<USkeletalMeshSocket>(Socket, DstSkeleton);
			NewSockets.Add(DstSocket);
#endif
		}
	}
#else
	// TArray<FMeshBoneInfo> to TSet<FName> (only BoneName is extracted).
	USkeleton* const DstSkeleton = DstSkeletalMesh->GetSkeleton();
	TSet<FName> DstBoneNames;
	Algo::Transform(DstSkeleton->GetReferenceSkeleton().GetRefBoneInfo(), DstBoneNames,
	                [](const FMeshBoneInfo& BoneInfo)
	                {
		                return BoneInfo.Name;
	                }
	);

	// Add the socket to be copied to the temporary list.
	TArray<USkeletalMeshSocket*> NewSockets;
	NewSockets.Reserve(SocketNum);
	for (const auto& Socket : SrcSockets)
	{
		if (DstBoneNames.Contains(Socket->BoneName))
		{
			NewSockets.Add(Socket);
		}
	}
#endif
	const bool IsUpdateSocket(NewSockets.Num() > 0);
	const bool IsAllUpdateSocket(IsUpdateSocket && (SocketNum == NewSockets.Num()));
	if (IsUpdateSocket)
	{
		DstSkeleton->Sockets.Append(NewSockets);
#if !WITH_EDITOR
		DstSkeletalMesh->RebuildSocketMap();
#endif // !WITH_EDITOR
	}
	if (IsAllUpdateSocket == false)
	{
		// TODO: Find compatible bone/socket relocations.
	}
	return IsUpdateSocket;
}

bool UVRoidFunctionLibrary::RenameBonesForHumanoid(const UVrmAssetListObject* VrmAsset)
{
	bool IsRename = false;
	const auto SkeletalMesh(VrmAsset->SkeletalMesh);
	if (SkeletalMesh == nullptr)
	{
		VROID_ERROR(TEXT("Failed to rename, Skeletal Mesh is NULL."));
		return IsRename;
	}
	USkeleton* Skeleton(SkeletalMesh->GetSkeleton());
	if (Skeleton == nullptr)
	{
		VROID_ERROR(TEXT("Failed to rename, Skeleton is NULL."));
		return IsRename;
	}
	FReferenceSkeleton& ReferenceSkeleton = const_cast<FReferenceSkeleton&>(Skeleton->GetReferenceSkeleton());
	FReferenceSkeletonModifier ReferenceSkeletonModifier(ReferenceSkeleton, Skeleton);

	for (const auto& [HumanoidNodeName, BoneName] : VrmAsset->VrmMetaObject->humanoidBoneTable)
	{
		if (BoneName.Contains(TEXT("J_Bip")) || BoneName.Contains(TEXT("J_Sec")) || BoneName.Contains(TEXT("J_Adj")))
		{
			continue;
		}
		const auto* MappedBone = VRoid::VrmHumanoidBoneMap.Find(HumanoidNodeName);
		if (MappedBone == nullptr)
		{
			continue;
		}
		const FString NewBoneName(*MappedBone);
		// VROID_DISPLAY(TEXT("Rename Bone: (%s) to (%s)"), *BoneName, *NewBoneName);
		ReferenceSkeletonModifier.Rename(*BoneName, *NewBoneName);
		VrmAsset->VrmMetaObject->humanoidBoneTable[HumanoidNodeName] = NewBoneName;
		IsRename = true;
	}

	ReferenceSkeleton.RebuildRefSkeleton(Skeleton, true);
	Skeleton->ClearCacheData();
	Skeleton->MergeAllBonesToBoneTree(SkeletalMesh);
#if WITH_EDITOR
	// Skeleton->Modify();
	// Skeleton->MarkPackageDirty();
	Skeleton->PostEditChange();
#endif // WITH_EDITOR
	SkeletalMesh->SetSkeleton(Skeleton);
	SkeletalMesh->SetRefSkeleton(ReferenceSkeleton);

	return IsRename;
}

void UVRoidFunctionLibrary::DestroyAssetListObject(UVrmAssetListObject* VrmAssetListObject)
{
	if (VrmAssetListObject == nullptr)
	{
		return;
	}
	for (const auto& Texture : VrmAssetListObject->Textures)
	{
		Texture->ReleaseResource();
	}
#if WITH_EDITOR
	const auto IsAssetOpenInEditor = [&VrmAssetListObject]
	{
		if (GEditor == nullptr)
		{
			return false;
		}
		const auto AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem == nullptr)
		{
			return false;
		}
		const TArray<UObject*> CheckAssets =
		{
			VrmAssetListObject,
			VrmAssetListObject->VrmMetaObject ? VrmAssetListObject->VrmMetaObject : nullptr,
			VrmAssetListObject->VrmHumanoidMetaObject ? VrmAssetListObject->VrmHumanoidMetaObject : nullptr,
			VrmAssetListObject->VrmMannequinMetaObject ? VrmAssetListObject->VrmMannequinMetaObject : nullptr,
			VrmAssetListObject->SkeletalMesh ? VrmAssetListObject->SkeletalMesh : nullptr,
			VrmAssetListObject->SkeletalMesh
				? (VrmAssetListObject->SkeletalMesh->GetSkeleton()
					   ? VrmAssetListObject->SkeletalMesh->GetSkeleton()
					   : nullptr)
				: nullptr,
		};
		for (const auto Object : CheckAssets)
		{
			if (Object == nullptr)
			{
				continue;
			}
			if (AssetEditorSubsystem->FindEditorForAsset(Object, false) != nullptr)
			{
				return true;
			}
		}
		return false;
	};
	if (IsAssetOpenInEditor())
	{
		return;
	}

	const auto BeginDestroyObject = [](UObject* const Object)
	{
		if (Object == nullptr)
		{
			return;
		}
		Object->ClearFlags(RF_Standalone);
		Object->SetFlags(RF_Public | RF_Transient);
		Object->RemoveFromRoot();
		Object->ConditionalBeginDestroy();
	};
	for (const auto& Texture : VrmAssetListObject->Textures)
	{
		BeginDestroyObject(Texture);
	}
	for (const auto& MaterialInterface : VrmAssetListObject->Materials)
	{
		BeginDestroyObject(MaterialInterface);
	}
	for (const auto& MaterialInterface : VrmAssetListObject->OutlineMaterials)
	{
		BeginDestroyObject(MaterialInterface);
	}
	if (VrmAssetListObject->SkeletalMesh)
	{
		BeginDestroyObject(VrmAssetListObject->SkeletalMesh->GetSkeleton());
		BeginDestroyObject(VrmAssetListObject->SkeletalMesh->GetPhysicsAsset());
		BeginDestroyObject(VrmAssetListObject->SkeletalMesh);
	}
	BeginDestroyObject(VrmAssetListObject->VrmMetaObject);
	BeginDestroyObject(VrmAssetListObject->VrmLicenseObject);
	BeginDestroyObject(VrmAssetListObject->Vrm1LicenseObject);
	BeginDestroyObject(VrmAssetListObject->HumanoidSkeletalMesh);
	BeginDestroyObject(VrmAssetListObject->HumanoidRig);
	BeginDestroyObject(VrmAssetListObject);
	VrmAssetListObject = nullptr;
#endif // WITH_EDITOR
}

bool UVRoidFunctionLibrary::DoesThirdPersonLevelExist(const FName& LevelName)
{
	const FString Level = LevelName.ToString();
	const TArray SearchPaths = {
		FString::Printf(TEXT("/Game/ThirdPerson/Maps/%s"), *Level),
		FString::Printf(TEXT("/Game/ThirdPerson/%s"), *Level)
	};
	for (const FString& Path : SearchPaths)
	{
		if (FPackageName::DoesPackageExist(Path))
		{
			return true;
		}
	}
	return false;
}

void UVRoidFunctionLibrary::EditorInitializeMeshFromThirdPersonIfEmpty(USkeletalMeshComponent* SkeletalMeshComponent, const FString& TryLoadMeshPath, const FString& TryLoadAnimPath)
{
#if WITH_EDITOR
	if (SkeletalMeshComponent == nullptr ||
		SkeletalMeshComponent->GetSkeletalMeshAsset() != nullptr &&
		SkeletalMeshComponent->GetAnimationMode() == EAnimationMode::AnimationBlueprint &&
		SkeletalMeshComponent->GetAnimClass() != nullptr)
	{
		return;
	}
	UObject* MeshObject = nullptr;
	UObject* AnimObject = nullptr;
	if (SkeletalMeshComponent->GetSkeletalMeshAsset() == nullptr)
	{
		FSoftObjectPath MeshSoftPath(TryLoadMeshPath.IsEmpty()
			                             ? TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn")
			                             : TryLoadMeshPath);
		MeshObject = MeshSoftPath.TryLoad();
		if (MeshObject == nullptr)
		{
			MeshSoftPath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple");
			MeshObject = MeshSoftPath.TryLoad();
		}
	}
	if (SkeletalMeshComponent->GetAnimClass() == nullptr)
	{
		FSoftObjectPath AnimSoftPath(TryLoadAnimPath.IsEmpty()
										 ? TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn")
										 : TryLoadAnimPath);
		AnimObject = AnimSoftPath.TryLoad();
		if (AnimObject == nullptr)
		{
			AnimSoftPath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed");
			AnimObject = AnimSoftPath.TryLoad();
		}
	}
	
	UBlueprintGeneratedClass* BPGeneratedClass = Cast<UBlueprintGeneratedClass>(SkeletalMeshComponent->GetOwner()->GetClass());
	const UBlueprint* OwnerBP = Cast<UBlueprint>(BPGeneratedClass->ClassGeneratedBy);
	if (BPGeneratedClass == nullptr || OwnerBP == nullptr || OwnerBP->SimpleConstructionScript == nullptr)
	{
		return;
	}
	for (const USCS_Node* AttachComponent : OwnerBP->SimpleConstructionScript->GetAllNodes())
	{
		if (AttachComponent == nullptr || AttachComponent->GetVariableName() != SkeletalMeshComponent->GetName())
		{
			continue;
		}
		if (USkeletalMeshComponent* ComponentTemplate = Cast<USkeletalMeshComponent>(AttachComponent->GetActualComponentTemplate(BPGeneratedClass)))
		{
			if (MeshObject && MeshObject->IsA<USkeletalMesh>())
			{
				USkeletalMesh* const MeshAsset = Cast<USkeletalMesh>(MeshObject);
				ComponentTemplate->SetSkeletalMeshAsset(MeshAsset);
				SkeletalMeshComponent->SetSkeletalMeshAsset(MeshAsset);
			}
			if (AnimObject && AnimObject->IsA<UAnimBlueprint>())
			{
				if (const UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(AnimObject))
				{
					if (UClass* AnimClass = AnimBP->GeneratedClass;
						AnimClass != nullptr)
					{
						ComponentTemplate->SetAnimInstanceClass(AnimClass);
						SkeletalMeshComponent->SetAnimInstanceClass(AnimClass);
					}
				}
			}
		}
		return;
	}
	// NOTE: If the SkeletalMeshComponent was added in the Blueprint, it will be updated up to this point.
	// However, in the case of a Mesh defined in C++, it won't be updated, so the following processing is required.
	const ACharacter* ClassDefaultCharacter = Cast<ACharacter>(BPGeneratedClass->GetDefaultObject());
	if (ClassDefaultCharacter == nullptr)
	{
		return;
	}
	if (USkeletalMeshComponent* CharacterMeshComponent = ClassDefaultCharacter->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (MeshObject && MeshObject->IsA<USkeletalMesh>())
		{
			USkeletalMesh* const MeshAsset = Cast<USkeletalMesh>(MeshObject);
			CharacterMeshComponent->SetSkeletalMeshAsset(MeshAsset);
			SkeletalMeshComponent->SetSkeletalMeshAsset(MeshAsset);
		}
		if (AnimObject && AnimObject->IsA<UAnimBlueprint>())
		{
			if (const UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(AnimObject))
			{
				if (UClass* AnimClass = AnimBP->GeneratedClass;
					AnimClass != nullptr)
				{
					CharacterMeshComponent->SetAnimInstanceClass(AnimClass);
					SkeletalMeshComponent->SetAnimInstanceClass(AnimClass);
				}
			}
		}
	}
#endif // WITH_EDITOR
}

bool UVRoidFunctionLibrary::ApplyVrmBlendShape(const UVrmAssetListObject* VrmAssetListObject, USkeletalMeshComponent* const SkeletalMeshComponent, const EVRMBlendShapeGroup BlendShapeType, const FString OverrideBlendShapeName, const float Value)
{
	if (VrmAssetListObject == nullptr || VrmAssetListObject->VrmMetaObject == nullptr || SkeletalMeshComponent == nullptr)
	{
		return false;
	}

	FString TargetBlendShapeName = OverrideBlendShapeName;
	if (TargetBlendShapeName.IsEmpty())
	{
		const UEnum* Enum = StaticEnum<EVRMBlendShapeGroup>();
		if (Enum == nullptr)
		{
			return false;
		}

		const FString DisplayName = Enum->GetDisplayNameTextByValue(static_cast<int64>(BlendShapeType)).ToString();
		FString Prefix;
		if (DisplayName.Split(TEXT("_"), &Prefix, &TargetBlendShapeName) == false)
		{
			return false;
		}
	}
	bool IsApplied = false; 
	for (const auto& BlendShapeGroup : VrmAssetListObject->VrmMetaObject->BlendShapeGroup)
	{
		if (BlendShapeGroup.name != TargetBlendShapeName) 
		{
			continue;
		}
		for (const auto& BlendShape : BlendShapeGroup.BlendShape)
		{
			SkeletalMeshComponent->SetMorphTarget(*BlendShape.morphTargetName, Value, true);
			IsApplied = true;
		}
	}
	return IsApplied;
}

bool UVRoidFunctionLibrary::IsVrm10(const uint8_t* Data, const size_t Size)
{
#if 0
	constexpr int FourCCSize = 4;
	if (Size < FourCCSize || Data == nullptr)
	{
		return false;
	}

	size_t c_start = 16;
	for (const std::vector JsonFourCC = {'J', 'S', 'O', 'N'}; c_start < Size - JsonFourCC.size(); ++c_start)
	{
		bool bFound = false;

		for (int i = 0; i < FourCCSize; ++i)
		{
			if (JsonFourCC[i] != Data[c_start + i])
			{
				break;
			}
			bFound = true;
		}
		if (bFound)
		{
			c_start += FourCCSize;
			break;
		}
	}

	int c = 0;
	size_t c_end = c_start;
	for (; c_end < Size; ++c_end)
	{
		if (Data[c_end] == '{')
		{
			c++;
		}
		if (Data[c_end] == '}')
		{
			c--;
		}
		if (c == 0)
		{
			++c_end;
			break;
		}
	}

	std::vector<char> v;
	v.resize(c_end - c_start);
	memcpy(&v[0], Data + c_start, c_end - c_start);
	v.push_back(0);

	RAPIDJSON_NAMESPACE::Document doc;
	doc.Parse(&v[0]);

	if (doc.HasMember("extensions"))
	{
		if (doc["extensions"].HasMember("VRMC_vrm"))
		{
			return true;
		}
		if (doc["extensions"].HasMember("VRMC_vrm_animation"))
		{
			return true;
		}
	}
	return false;
#else
	// Read 4 bytes from the current offset in little-endian order, then advance the offset.
	const auto ReadData = [](const uint8* InData, int32& OutOffset)
	{
		const uint32_t Value =
			 static_cast<uint32_t>(InData[OutOffset]) |
			(static_cast<uint32_t>(InData[OutOffset + 1]) << 8) |
			(static_cast<uint32_t>(InData[OutOffset + 2]) << 16) |
			(static_cast<uint32_t>(InData[OutOffset + 3]) << 24);
		OutOffset += 4;
		return Value;
	};
	// Read and validate the FourCC magic (should be 'glTF').
	constexpr int32 glTFChunkSize = 20;
	int32 ReadOffset = 0;
	const uint32_t FourCC = ReadData(Data, ReadOffset);
	if (Size < glTFChunkSize || FourCC != 0x46546C67) // 'glTF' in little endian.
	{
		VROID_WARNING(TEXT("Invalid glTF header: size too small (%d) or magic mismatch."), Size);
		return false;
	}
	// Read the glTF version (expected: 1 or 2) and total file size from the header.
	const uint32_t glTFVersion = ReadData(Data, ReadOffset);
	const uint32_t TotalLength = ReadData(Data, ReadOffset);
	if (TotalLength > Size)
	{
		VROID_ERROR(TEXT("glTF TotalLength (%u) exceeds actual loaded size (%d)"), TotalLength, Size);
		return false;
	}
	// Acceptable sizes:
	// - TotalLength (plain glTF size)
	// - TotalLength + VrmEncryptedHeaderSize (encrypted vrm with extra header)
	constexpr int32 VrmEncryptedHeaderSize = 44;
	if (TotalLength != Size && (TotalLength + VrmEncryptedHeaderSize) != Size)
	{
		VROID_WARNING(TEXT("glTF TotalLength (%u) does not match loaded size (%d); expected either exact match or +%d bytes (encrypted header)."), TotalLength, Size, VrmEncryptedHeaderSize);
		return false;
	}
	uint32_t JsonSize = 0;
	if (glTFVersion == 2)
	{
		const uint32_t ChunkLength = ReadData(Data, ReadOffset);
		const uint32_t ChunkType = ReadData(Data, ReadOffset);
		if (ChunkType != 0x4E4F534A) // 'JSON' in little endian.
		{
			VROID_WARNING(TEXT("Invalid chunk type: the first chunk is not JSON (expected 'JSON' at byte 16)"));
			return false;
		}
		if (glTFChunkSize + ChunkLength > Size)
		{
			VROID_WARNING(TEXT("Invalid chunk length: JSON chunk size exceeds total file size"));
			return false;
		}
		JsonSize = ChunkLength;
	}
	else
	{
		// glTF 1.0 GLB: not used in VRM, but handled for compatibility.
		if (glTFVersion == 1)
		{
			const uint32_t ContentLength = ReadData(Data, ReadOffset);
			const uint32_t ContentFormat = ReadData(Data, ReadOffset);
			if (ContentFormat != 0)
			{
				VROID_WARNING(TEXT("Unsupported content format in glTF 1.0: expected 0 (JSON)"));
				return false;
			}
			if (glTFChunkSize + ContentLength > Size)
			{
				VROID_WARNING(TEXT("Invalid content length: content size exceeds total file size (glTF 1.0)"));
				return false;
			}
			JsonSize = ContentLength;
		}
		else
		{
			VROID_ERROR(TEXT("Unsupported glTF version: %u"), glTFVersion);
			return false;
		}
	}
	// Parse json.
	std::vector<char> JsonBuffer(JsonSize + 1);
	memcpy(&JsonBuffer[0], Data + glTFChunkSize, JsonSize);
	JsonBuffer[JsonSize] = 0;
	RAPIDJSON_NAMESPACE::Document JsonDoc;
	JsonDoc.Parse(&JsonBuffer[0]);
	// Verify that the glTF file uses the VRM 1.0 format (by checking VRMC_vrm or VRMC_vrm_animation extensions).
	if (JsonDoc.HasMember("extensions"))
	{
		if (JsonDoc["extensions"].HasMember("VRMC_vrm"))
		{
			return true;
		}
		if (JsonDoc["extensions"].HasMember("VRMC_vrm_animation"))
		{
			return true;
		}
	}
	return false;
#endif
}

bool UVRoidFunctionLibrary::ReadVrmFileFromPath(const UObject* WorldContextObject, const FString& FilePath, TArray<uint8>& Result)
{
	const int64 FileSize(IFileManager::Get().FileSize(*FilePath));
	if (FileSize <= 0 || FilePath.IsEmpty())
	{
		VROID_ERROR(TEXT("Invalid file size for VRM binary (FilePath:%s)."), *FilePath);
		return false;
	}
	Result.Reserve(FileSize);

	bool IsLoad = false;
	if (FilePath.Contains(".enc") == false)
	{
		IsLoad = FFileHelper::LoadFileToArray(Result, *FilePath);
	}
	else
	{
		if (const TCHAR* Extension = TEXT(".enc.vrm");
			FilePath.EndsWith(Extension) &&
			FilePath.Len() > FCString::Strlen(Extension))
		{
			const auto& Binary(VRoid::ReadBinaryFromSdk(WorldContextObject, FilePath, FileSize));
			if (const int32 BinarySize = Binary.size();
				0 < BinarySize)
			{
				// It's assumed that for sizes less than 10MB, using Append is more efficient.
				if (constexpr int32 MaxSmallDataSize = 10 * 1024 * 1024; // 10MB
					BinarySize < MaxSmallDataSize)
				{
					Result.Append(Binary.data(), BinarySize);
				}
				else
				{
					Result.SetNumUninitialized(BinarySize);
					FMemory::Memcpy(Result.GetData(), Binary.data(), BinarySize);
				}
				IsLoad = true;
			}
		}
	}

	if (IsLoad == false)
	{
		VROID_ERROR(TEXT("Failed to read VRM binary (FilePath:%s)."), *FilePath);
	}
	return IsLoad;
}

bool UVRoidFunctionLibrary::ConvertVrmMetaRenamedWrapper(VRMConverter* VrmConverter, UVrmAssetListObject*& OutVrmAsset, const aiScene* ScenePtr, const uint8* Data, const size_t Size)
{
#if 0 // After VRM4U_5_3_20240506, ConvertVrmMetaRenamed is obsolete and replaced by ConvertVrmMetaPost.
	return VrmConverter->ConvertVrmMetaRenamed(OutVrmAsset, ScenePtr, Data, Size);
#else
	return VrmConverter->ConvertVrmMetaPost(OutVrmAsset, ScenePtr, Data, Size);
#endif
}

UTexture2D* UVRoidFunctionLibrary::CreateTextureFromImageWrapper(const FString& TextureName, const aiTexture& Texture, const bool bGenerateMips, const bool bIsNormal)
{
#if 0	// After VRM4U_5_3_20240402, VRMUtil is obsolete and replaced by VRMLoaderUtil.
	return VRMUtil::CreateTextureFromImage(TextureName, GetTransientPackage(), Texture.pcData, Texture.mWidth, bGenerateMips, bIsNormal); // bRuntimeMode);
#else
	return VRMLoaderUtil::CreateTextureFromImage(TextureName, GetTransientPackage(), Texture.pcData, Texture.mWidth, bGenerateMips, bIsNormal);
#endif
}

void UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(const TSharedRef<TFunction<void()>>& Function)
{
	if (IsInGameThread())
	{
		(*Function)();
		return;
	}
#if 0
	const auto TaskRef = FFunctionGraphTask::CreateAndDispatchWhenReady
	(
		[&Function]
		{
			(*Function)();
		}, TStatId(), nullptr, ENamedThreads::GameThread
	);
	TaskRef->Wait();
#else
	TSharedRef<TFunction<void()>> LocalFunction = Function;
	const FGraphEventRef TaskRef = FFunctionGraphTask::CreateAndDispatchWhenReady
	(
		[Local = MoveTemp(LocalFunction)]
		{
			(*Local)();
		}, TStatId(), nullptr, ENamedThreads::GameThread
	);
	FTaskGraphInterface::Get().WaitUntilTaskCompletes(TaskRef);
#endif
}

/**
 * Example:
 * 
 * // Initializing the task.
 * const auto Tasks = MakeShared<FGraphEventRef, ESPMode::ThreadSafe>(FFunctionGraphTask::CreateAndDispatchWhenReady([]()
 * {
 *     UE_LOG(LogTemp, Log, TEXT("Task is running."));
 * }, TStatId(), nullptr, ENamedThreads::AnyBackgroundThreadNormalTask));
 * 
 * // 
 * TSharedPtr<TFunction<void()>> Callback = MakeShared<TFunction<void()>>([]()
 * {
 *     UE_LOG(LogTemp, Log, TEXT("Task completed!"));
 * });
 */
void UVRoidFunctionLibrary::PollTaskWithTimeout(FTimerManager& Timer, const TSharedPtr<FGraphEventRef>& Task, const TSharedPtr<TFunction<void()>>& Callback,
                                                  const double StartTime, const float Timeout)
{
	if (Timeout < (FPlatformTime::Seconds() - StartTime))
	{
		VROID_ERROR(TEXT("Task polling exceeded the allowed timeout. Aborting task execution."));
		return;
	}
	if (Task.IsValid() && Task->IsValid() && Task->GetReference()->IsComplete())
	{
		if (Callback.IsValid())
		{
			(*Callback)();
		}
		else
		{
			VROID_WARNING(TEXT("Callback is not valid."));
		}
		return;
	}
	Timer.SetTimerForNextTick([&Timer, Task, Callback, StartTime, Timeout]
	{
		PollTaskWithTimeout(Timer, Task, Callback, StartTime, Timeout);
	});
}
void UVRoidFunctionLibrary::PollTaskWithTimeout(const TSharedPtr<FGraphEventRef>& Task, const TSharedPtr<TFunction<void()>>& Callback, const ENamedThreads::Type ThreadType,
                                                  const float Timeout, const float SleepTime)
{
	const double StartTime(FPlatformTime::Seconds());
	AsyncTask(ThreadType, [Task, Callback, StartTime, Timeout, SleepTime]
	{
		while (Task.IsValid() == false || Task->IsValid() == false || Task->GetReference()->IsComplete() == false)
		{
			if (Timeout < (FPlatformTime::Seconds() - StartTime))
			{
				VROID_ERROR(TEXT("Task polling exceeded the allowed timeout. Aborting task execution."));
				return;
			}
			FPlatformProcess::Sleep(SleepTime);
		}

		if (Callback.IsValid())
		{
			(*Callback)();
		}
		else
		{
			VROID_WARNING(TEXT("Callback is not valid."));
		}
	});
}