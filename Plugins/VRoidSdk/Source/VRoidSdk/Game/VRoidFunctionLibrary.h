// Copyright © 2023 pixiv Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VRoidFunctionLibrary.generated.h"

enum class EVRMBlendShapeGroup : uint8;
class UVrmAssetListObject;

namespace VRoid
{
	const TMap<FString, FString> VrmHumanoidBoneMap = {
		{TEXT("hips"), TEXT("J_Bip_C_Hips")},
		{TEXT("spine"), TEXT("J_Bip_C_Spine")},
		{TEXT("chest"), TEXT("J_Bip_C_Chest")},
		{TEXT("upperChest"), TEXT("J_Bip_C_UpperChest")},
		{TEXT("neck"), TEXT("J_Bip_C_Neck")},
		{TEXT("head"), TEXT("J_Bip_C_Head")},
		{TEXT("leftEye"), TEXT("J_Adj_L_FaceEye")},
		{TEXT("rightEye"), TEXT("J_Adj_R_FaceEye")},
		// {TEXT("jaw"), TEXT("")},

		{TEXT("leftShoulder"), TEXT("J_Bip_L_Shoulder")},
		{TEXT("leftUpperArm"), TEXT("J_Bip_L_UpperArm")},
		{TEXT("leftLowerArm"), TEXT("J_Bip_L_LowerArm")},
		{TEXT("leftHand"), TEXT("J_Bip_L_Hand")},
		{TEXT("leftIndexProximal"), TEXT("J_Bip_L_Index1")},
		{TEXT("leftIndexIntermediate"), TEXT("J_Bip_L_Index2")},
		{TEXT("leftIndexDistal"), TEXT("J_Bip_L_Index3")},
		{TEXT("leftMiddleProximal"), TEXT("J_Bip_L_Middle1")},
		{TEXT("leftMiddleIntermediate"), TEXT("J_Bip_L_Middle2")},
		{TEXT("leftMiddleDistal"), TEXT("J_Bip_L_Middle3")},
		{TEXT("leftRingProximal"), TEXT("J_Bip_L_Ring1")},
		{TEXT("leftRingIntermediate"), TEXT("J_Bip_L_Ring2")},
		{TEXT("leftRingDistal"), TEXT("J_Bip_L_Ring3")},
		{TEXT("leftLittleProximal"), TEXT("J_Bip_L_Little1")},
		{TEXT("leftLittleIntermediate"), TEXT("J_Bip_L_Little2")},
		{TEXT("leftLittleDistal"), TEXT("J_Bip_L_Little3")},
		{TEXT("leftThumbProximal"), TEXT("J_Bip_L_Thumb1")},
		{TEXT("leftThumbIntermediate"), TEXT("J_Bip_L_Thumb2")},
		{TEXT("leftThumbDistal"), TEXT("J_Bip_L_Thumb3")},

		{TEXT("rightShoulder"), TEXT("J_Bip_R_Shoulder")},
		{TEXT("rightUpperArm"), TEXT("J_Bip_R_UpperArm")},
		{TEXT("rightLowerArm"), TEXT("J_Bip_R_LowerArm")},
		{TEXT("rightHand"), TEXT("J_Bip_R_Hand")},
		{TEXT("rightIndexProximal"), TEXT("J_Bip_R_Index1")},
		{TEXT("rightIndexIntermediate"), TEXT("J_Bip_R_Index2")},
		{TEXT("rightIndexDistal"), TEXT("J_Bip_R_Index3")},
		{TEXT("rightMiddleProximal"), TEXT("J_Bip_R_Middle1")},
		{TEXT("rightMiddleIntermediate"), TEXT("J_Bip_R_Middle2")},
		{TEXT("rightMiddleDistal"), TEXT("J_Bip_R_Middle3")},
		{TEXT("rightRingProximal"), TEXT("J_Bip_R_Ring1")},
		{TEXT("rightRingIntermediate"), TEXT("J_Bip_R_Ring2")},
		{TEXT("rightRingDistal"), TEXT("J_Bip_R_Ring3")},
		{TEXT("rightLittleProximal"), TEXT("J_Bip_R_Little1")},
		{TEXT("rightLittleIntermediate"), TEXT("J_Bip_R_Little2")},
		{TEXT("rightLittleDistal"), TEXT("J_Bip_R_Little3")},
		{TEXT("rightThumbProximal"), TEXT("J_Bip_R_Thumb1")},
		{TEXT("rightThumbIntermediate"), TEXT("J_Bip_R_Thumb2")},
		{TEXT("rightThumbDistal"), TEXT("J_Bip_R_Thumb3")},

		{TEXT("leftUpperLeg"), TEXT("J_Bip_L_UpperLeg")},
		{TEXT("leftLowerLeg"), TEXT("J_Bip_L_LowerLeg")},
		{TEXT("leftFoot"), TEXT("J_Bip_L_Foot")},
		{TEXT("leftToes"), TEXT("J_Bip_L_ToeBase")},
		{TEXT("rightUpperLeg"), TEXT("J_Bip_R_UpperLeg")},
		{TEXT("rightLowerLeg"), TEXT("J_Bip_R_LowerLeg")},
		{TEXT("rightFoot"), TEXT("J_Bip_R_Foot")},
		{TEXT("rightToes"), TEXT("J_Bip_R_ToeBase")},
	};
} // namespace VRoid

UCLASS()
class VROIDSDK_API UVRoidFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	static bool LoadVrmFileFromMemory(const UVrmAssetListObject* InVrmAsset, UVrmAssetListObject*& OutVrmAsset, const FString& FilePath, const uint8* Data, const size_t Size);

public:
	UFUNCTION(BlueprintPure, Category="VRoid")
	static class UVrmMetaObject* GetVrmMetaObject(const UVrmAssetListObject* const VrmAsset, const EVRMType VrmType);
	UFUNCTION(BlueprintPure, Category="VRoid")
	static USkeletalMesh* GetVrmSkeletalMesh(const UVrmAssetListObject* const VrmAsset, const EVRMType VrmType);
	UFUNCTION(BlueprintPure, Category="VRoid")
	static USkeleton* GetVrmSkeleton(const UVrmAssetListObject* const VrmAsset, const EVRMType VrmType);
	UFUNCTION(BlueprintCallable, Category="VRoid")
	static bool AddVirtualBone(const USkeletalMeshComponent* const Mesh, const FName SourceBoneName, const FName TargetBoneName, const FName ReNameBoneName);

	UFUNCTION(BlueprintPure, Category="VRoid|Multiplay", meta = (WorldContext = "WorldContextObject"))
	static APlayerController* GetFirstLocalPlayerController(const UObject* const WorldContextObject);
	UFUNCTION(BlueprintPure, Category="VRoid|Multiplay", meta = (WorldContext = "WorldContextObject"))
	static FString GetFirstLocalPlayerUniqueNetId(const UObject* const WorldContextObject);
	UFUNCTION(BlueprintPure, Category="VRoid|Multiplay", meta=(DisplayName = "To String (UniqueNetId)", CompactNodeTitle = "->", BlueprintAutocast))
	static FString ConvertUniqueNetIdToString(const FUniqueNetIdRepl& UniqueNetId);
	UFUNCTION(BlueprintPure, Category="VRoid|Multiplay", meta=(WorldContext = "WorldContextObject"))
	static bool IsAllClientsSelectedModel(const UObject* const WorldContextObject);
	/**
	 * ClientのSessionが残っていればSessionを破棄しますが、OnlineSubsystemNULL前提です。
	 * If a Session remains, the Session is destroyed, but the OnlineSubsystemNULL assumption is made.
	*/
	UFUNCTION(BlueprintCallable, Category="VRoid|Multiplay", meta=(WorldContext = "WorldContextObject"))
	static bool ClientDestroySession(const UObject* const WorldContextObject);

	UFUNCTION(BlueprintCallable,Category="VRoid", meta = (DynamicOutputParam = "OutVrmAsset"))
	static bool LoadEncodedVRM(const UVrmAssetListObject* InVrmAsset, UVrmAssetListObject*& OutVrmAsset, const FString& FilePath, const struct FImportOptionData& OptionForRuntimeLoad, const bool IsFastConverter = true);
	UFUNCTION(BlueprintCallable, Category="VRoid", meta = (Latent, DynamicOutputParam = "OutVrmAsset", WorldContext = "WorldContextObject", LatentInfo = "LatentInfo"))
	static void AsyncLoadEncodedVRM(const UObject* WorldContextObject, const UVrmAssetListObject* InVrmAsset, UVrmAssetListObject*& OutVrmAsset, const FString& FilePath, const FImportOptionData& OptionForRuntimeLoad, const FLatentActionInfo LatentInfo, const bool  IsFastConverter = true);

	UFUNCTION(BlueprintPure, Category="VRoid|UserInterface")
	static FVRMLicense ConvertLicenseText(FVRMLicense License);
	UFUNCTION(BlueprintPure, Category="VRoid|UserInterface")
	static FVRMLicense10 ConvertLicenseText10(FVRMLicense10 License);
	UFUNCTION(BlueprintCallable, Category="VRoid|UserInterface", meta = (AdvancedDisplay = "2"))
	static void UpdateButtonImageOrClear(class UButton* Button, UTexture2DDynamic* DynamicImage = nullptr, const ESlateBrushDrawType::Type DrawType = ESlateBrushDrawType::RoundedBox);

	UFUNCTION(BlueprintCallable, Category="VRoid")
	static void UpdateBoneName(USkeletalMesh* TargetMesh, const FName& OldBoneName, const FName& NewBoneName);
	UFUNCTION(BlueprintCallable, Category="VRoid")
	static bool CopyVrmSkeletonSockets(const USkeletalMeshComponent* SrcSkeletalMeshComponent, const UVrmAssetListObject* const VrmAsset);
	UFUNCTION(BlueprintCallable, Category="VRoid")
	static bool RenameBonesForHumanoid(const UVrmAssetListObject* VrmAsset);
	/** 
	 * Editor時のみ動作。VrmAssetListObject, VrmAssetListObject内のSkeletalMesh, Skeleton, MetaObjectがEditorで開かれていなければ明示的に破棄します。
	 * Only works in the editor. Explicitly discards SkeletalMesh, Skeleton, and MetaObjects in VrmAssetListObject and VrmAssetListObject if they are not opened in an editor.
	 */
	UFUNCTION(BlueprintCallable, Category="VRoid")
	static void DestroyAssetListObject(UVrmAssetListObject* VrmAssetListObject);
	/** 
	 * この関数は ThirdPerson テンプレートを使用していることを前提としています
	 * 指定されたレベルが ThirdPerson テンプレート内に存在するかを確認します
	 * This function assumes the use of the ThirdPerson Template.
	 * Checks whether the specified level exists within the ThirdPerson Template structure.
	 */
	UFUNCTION(BlueprintPure, Category="VRoid")
	static bool DoesThirdPersonLevelExist(const FName& LevelName);
	/** 
	 * この関数はエディタ環境でのみ使用可能です。
	 * 指定された SkeletalMeshComponent に対して、
	 * ThirdPerson テンプレートのアセットを使用して SkeletalMesh および AnimClass を設定します。
	 * This function is only available in the editor.
 	 * It sets the SkeletalMesh and AnimClass on the specified SkeletalMeshComponent
 	 * using assets from the ThirdPerson Template.
	 */
	UFUNCTION(BlueprintCallable, Category="VRoid|Editor", meta = (DefaultToSelf = "Object", AdvancedDisplay="1"))
	static void EditorInitializeMeshFromThirdPersonIfEmpty(USkeletalMeshComponent* SkeletalMeshComponent, const FString& TryLoadMeshPath, const FString& TryLoadAnimPath);
	/** 
	 * VrmAssetListObject内のVrmMetaObjectが保持するBlendShapeGroupを参照し、
	 * 一致するものがあればMorphTargetを更新します。
	 * OverrideBlendShapeNameが指定されている場合はBlendShapeTypeより優先されます。
	 * Searches BlendShapeGroup in VrmMetaObject of VrmAssetListObject and updates MorphTarget if a match is found.
	 * If OverrideBlendShapeName is specified, it takes precedence over BlendShapeType.
	 */
	UFUNCTION(BlueprintCallable, Category="VRoid")
	static bool ApplyVrmBlendShape(const UVrmAssetListObject* VrmAssetListObject, USkeletalMeshComponent* const SkeletalMeshComponent, const EVRMBlendShapeGroup BlendShapeType, const FString OverrideBlendShapeName, const float Value = 1.0f);

	static bool IsVrm10(const uint8_t* Data, const size_t Size);
	static bool ReadVrmFileFromPath(const UObject* WorldContextObject, const FString& FilePath, TArray<uint8>& Result);
	static bool ConvertVrmMetaRenamedWrapper(class VRMConverter* VrmConverter, UVrmAssetListObject*& OutVrmAsset, const struct aiScene* ScenePtr, const uint8* Data, const size_t Size);
	static UTexture2D* CreateTextureFromImageWrapper(const FString& TextureName, const struct aiTexture& Texture, const bool bGenerateMips = false, const bool bIsNormal = true);
	static void RunAndWaitForGameThreadTask(const TSharedRef<TFunction<void()>>& Function);
	static void PollTaskWithTimeout(FTimerManager& Timer, const TSharedPtr<FGraphEventRef>& Task, const TSharedPtr<TFunction<void()>>& Callback,
	                                const double StartTime, const float Timeout = 10.0f);
	static void PollTaskWithTimeout(const TSharedPtr<FGraphEventRef>& Task, const TSharedPtr<TFunction<void()>>& Callback,
	                                const ENamedThreads::Type ThreadType = ENamedThreads::AnyBackgroundThreadNormalTask,
	                                const float Timeout = 10.0f, const float SleepTime = 0.01f);
};
