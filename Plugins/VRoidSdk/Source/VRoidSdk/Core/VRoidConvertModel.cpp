// Copyright © 2024 pixiv Inc. All rights reserved.

#include "VRoidConverter.h"

#if WITH_EDITOR
#include "CommonFrameRates.h"
// #include "Kismet2/KismetEditorUtilities.h"
#endif // WITH_EDITOR
#include "BoneWeights.h"
#include "PhysicsEngine/PhysicsAsset.h"
// #include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshRenderData.h"

#include "VrmSkeleton.h"
#include "VrmMetaObject.h"
#include "VrmAssetUserData.h"
#include "VrmAssetListObject.h"
#include "LoaderBPFunctionLibrary.h"

#include "Game/VRoidFunctionLibrary.h"
#include "Core/VRoidDefinitions.h"
#include "Core/VRoidLogger.h"

#if UE_NEWER_5_5
#include "PhysicsEngine/SkeletalBodySetup.h"
#endif // UE_NEWER_5_5

namespace VRoid
{
#if WITH_EDITOR
	typedef FSoftSkinVertex FSoftSkinVertexRuntime;
#else
	struct FSoftSkinVertexRuntime
	{
		FVector3f Position;
		FVector3f TangentX;
		FVector3f TangentY;
		FVector4f TangentZ;
		FVector2f UVs[MAX_TEXCOORDS];
		FColor Color;
		FBoneIndexType InfluenceBones[MAX_TOTAL_INFLUENCES];
		uint16 InfluenceWeights[MAX_TOTAL_INFLUENCES];
	};
#endif // WITH_EDITOR
	constexpr EObjectFlags TransientFlag(RF_Public | RF_Transient);
	constexpr float VrmToUnrealScaleOffset = 100.0f;
	constexpr int MaxMobileBoneNum = 75;
	constexpr int MaxWarningCount = 50;
	constexpr int MaxWeight = 100000;

	TArray<FString> AddedList;
	struct FBoneMapOption
	{
		int BoneIndex;
		float Weight;
		FBoneMapOption() : BoneIndex(0), Weight(0.f)
		{
		}
		bool operator<(const FBoneMapOption& BoneMapOpt) const
		{
			return Weight < BoneMapOpt.Weight;
		}
	};

	static const aiNode* FindBoneNodeFromMeshIndex(const int& MeshIndex, const aiNode* AiNode)
	{
		if (AiNode == nullptr)
		{
			return nullptr;
		}
		for (uint32_t i = 0; i < AiNode->mNumMeshes; ++i)
		{
			if (AiNode->mMeshes[i] == MeshIndex)
			{
				return AiNode;
			}
		}
		for (const aiNode* ChildNode : TArrayView<aiNode*>(AiNode->mChildren, AiNode->mNumChildren))
		{
			if (const auto Node = FindBoneNodeFromMeshIndex(MeshIndex, ChildNode))
			{
				return Node;
			}
		}
		return nullptr;
	}

	static const aiNode* FindNodeFromMeshIndex(const int MeshIndex, const aiScene* AiData)
	{
		return FindBoneNodeFromMeshIndex(MeshIndex, AiData->mRootNode);
	}

	static void FindMeshInfo(const uint32_t MeshIndex, const aiScene* AiScene, const aiNode* AiNode, FReturnedData& Result)
	{
		const int MeshNo = AiNode->mMeshes[MeshIndex];
		const aiMesh* Mesh = AiScene->mMeshes[MeshNo];
		FMeshInfo_VRM4U& MeshInfo = Result.meshInfo[MeshNo];
		// transform
		const auto* ParentNode = AiNode->mParent;
		const bool IsValidParent(ParentNode != nullptr);
		aiMatrix4x4 TempTrans = IsValidParent ? AiNode->mTransformation : aiMatrix4x4();
		if (IsValidParent)
		{
			while (ParentNode->mParent) // Trace back to just before the origin offset.
			{
				TempTrans *= ParentNode->mTransformation;
				ParentNode = ParentNode->mParent;
			}
		}
		const FMatrix TempMatrix = {
			{TempTrans.a1, TempTrans.b1, TempTrans.c1, TempTrans.d1},
			{TempTrans.a2, TempTrans.b2, TempTrans.c2, TempTrans.d2},
			{TempTrans.a3, TempTrans.b3, TempTrans.c3, TempTrans.d3},
			{TempTrans.a4, TempTrans.b4, TempTrans.c4, TempTrans.d4}
		};
		MeshInfo.RelativeTransform = FTransform(TempMatrix);

		auto& VertexUseFlags = MeshInfo.vertexUseFlag;
		if (VRMConverter::Options::Get().IsOptimizeVertex())
		{
			// use flag
			VertexUseFlags.AddZeroed(Mesh->mNumVertices);
			for (uint32_t FaceIndex = 0; FaceIndex < Mesh->mNumFaces; ++FaceIndex)
			{
				const auto& Face = Mesh->mFaces[FaceIndex];
				for (uint32_t Indices = 0; Indices < Face.mNumIndices; ++Indices)
				{
					const auto& Index = Face.mIndices[Indices];
					VertexUseFlags[Index] = true;
				}
			}
			// optimize table
			bool bHasSkipVertex = false;
			TArray<uint32_t> UseTable;
			UseTable.AddZeroed(VertexUseFlags.Num());
			int VertexFlagCount = 0;
			for (int i = 0; i < VertexUseFlags.Num(); ++i)
			{
				UseTable[i] = i - VertexFlagCount;
				if (VertexUseFlags[i] == false)
				{
					++VertexFlagCount;
					bHasSkipVertex = true;
				}
			}
			if (bHasSkipVertex)
			{
				// replace face index
				// for (uint32_t FaceIndex = 0; FaceIndex < Mesh->mNumFaces; ++FaceIndex)
				ParallelFor(Mesh->mNumFaces, [&](const uint32_t FaceIndex)
				{
					const auto& Face = Mesh->mFaces[FaceIndex];
					for (uint32_t Indices = 0; Indices < Face.mNumIndices; ++Indices)
					{
						Face.mIndices[Indices] = UseTable[Face.mIndices[Indices]];
					}
				});
				// replace weight index
				// for (uint32_t BoneIndex = 0; BoneIndex < Mesh->mNumBones; ++BoneIndex)
				ParallelFor(Mesh->mNumBones, [&](const uint32_t BoneIndex)
				{
					const auto& Bone = Mesh->mBones[BoneIndex];
					// for (uint32_t WeightIndex = 0; WeightIndex < Bone->mNumWeights; ++WeightIndex)
					ParallelFor(Bone->mNumWeights, [&](const uint32_t WeightIndex)
					{
						auto& Weight = Bone->mWeights[WeightIndex];
						const uint32_t Id = Weight.mVertexId;
						Weight.mVertexId = UseTable[Id];
						if (VertexUseFlags[Id] == false)
						{
							Weight.mWeight = 0.f;
						}
					});
				});
			}
		}
		if (VertexUseFlags.Num() > 0)
		{
			MeshInfo.vertexIndexOptTable.SetNumZeroed(VertexUseFlags.Num());
		}
		MeshInfo.useVertexCount = 0;
		// Vertex
		for (uint32_t v = 0; v < Mesh->mNumVertices; ++v)
		{
			if (static_cast<int>(v) < VertexUseFlags.Num())
			{
				if (VertexUseFlags[v] == false)
				{
					continue;
				}
				MeshInfo.vertexIndexOptTable[v] = MeshInfo.useVertexCount;
				MeshInfo.useVertexCount++;
			}
			FVector Vertex(Mesh->mVertices[v].x, Mesh->mVertices[v].y, Mesh->mVertices[v].z);
			Vertex = MeshInfo.RelativeTransform.TransformFVector4(Vertex);
			MeshInfo.Vertices.Add(Vertex);
			// Normal
			const FVector Normal(Mesh->HasNormals()
				                     ? FVector(Mesh->mNormals[v].x, Mesh->mNormals[v].y, Mesh->mNormals[v].z)
				                     : FVector::RightVector); // Use RightVector to point upward in Y-up conformance.
			MeshInfo.Normals.Add(Normal);
			// UV Coordinates - inconsistent coordinates
			for (int u = 0; u < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++u)
			{
				if (Mesh->HasTextureCoords(u) == false)
				{
					continue;
				}
				if (MeshInfo.UV0.Num() <= u)
				{
					MeshInfo.UV0.Add(TArray<FVector2D>());
#if UE_BUILD_DEVELOPMENT
					// if (u >= 1)
					// {
					// 	VROID_WARNING(TEXT("test uv2."));
					// }
#endif // UE_BUILD_DEVELOPMENT
				}
				const FVector2D UV(Mesh->mTextureCoords[u][v].x, 1.0f - Mesh->mTextureCoords[u][v].y);
				MeshInfo.UV0[u].Add(UV);
			}
			// Tangent
			if (Mesh->HasTangentsAndBitangents())
			{
				const auto Tangents(Mesh->mTangents[v]);
				MeshInfo.Tangents.Add(FVector(Tangents.x, Tangents.y, Tangents.z));
			}
			// Vertex color
			if (Mesh->HasVertexColors(0))
			{
				const auto VertexColor(Mesh->mColors[0][v]);
				MeshInfo.VertexColors.Add(FLinearColor(VertexColor.r, VertexColor.g, VertexColor.b, VertexColor.a));
			}
		}
	}

	static void FindMesh(const aiScene* AiScene, const aiNode* AiNode, FReturnedData& Result)
	{
		// for (uint32_t MeshIndex = 0; MeshIndex < AiNode->mNumMeshes; MeshIndex++)
		ParallelFor(AiNode->mNumMeshes, [&](const uint32_t MeshIndex)
		{
			FindMeshInfo(MeshIndex, AiScene, AiNode, Result);
		});
		// for (const aiNode* ChildNode : TArrayView<aiNode*>(AiNode->mChildren, AiNode->mNumChildren))
		ParallelFor(AiNode->mNumChildren, [&](const uint32_t ChildIndex)
		{
			// FindMesh(AiScene, ChildNode, Result);
			FindMesh(AiScene, AiNode->mChildren[ChildIndex], Result);
		});
	}
#if 0 // un used.
	static UPhysicsConstraintTemplate* CreateConstraint(USkeletalMesh* SkeletalMesh, UPhysicsAsset* PhysicsAsset,
	                                                    const VRM::VRMSpring& Spring,
	                                                    const FName ConstraintBone1, const FName ConstraintBone2)
	{
		// UPhysicsConstraintTemplate* ConstraintTemplate = NewObject<UPhysicsConstraintTemplate>(PhysicsAsset, NAME_None, RF_Transactional);
		UPhysicsConstraintTemplate* ConstraintTemplate = NewObject<UPhysicsConstraintTemplate>(PhysicsAsset, NAME_None, TransientFlag);
		PhysicsAsset->ConstraintSetup.Add(ConstraintTemplate);
		// "skirt_01_01"
		// ConstraintTemplate->Modify(false);
		const FString ConstraintName = ConstraintBone1.ToString() + TEXT("_") + ConstraintBone2.ToString();
		FString JointName = ConstraintName;
		int Index = 0;
		while (PhysicsAsset->FindConstraintIndex(*JointName) != INDEX_NONE)
		{
			JointName = FString::Printf(TEXT("%s_%d"), *ConstraintName, Index++);
		}
		ConstraintTemplate->DefaultInstance.JointName = *JointName;
		ConstraintTemplate->DefaultInstance.ConstraintBone1 = ConstraintBone1;
		ConstraintTemplate->DefaultInstance.ConstraintBone2 = ConstraintBone2;
		ConstraintTemplate->DefaultInstance.SetAngularSwing1Limit(ACM_Limited, 10);
		ConstraintTemplate->DefaultInstance.SetAngularSwing2Limit(ACM_Limited, 10);
		ConstraintTemplate->DefaultInstance.SetAngularTwistLimit(ACM_Limited, 10);
		ConstraintTemplate->DefaultInstance.ProfileInstance.ConeLimit.Stiffness = VrmToUnrealScaleOffset * Spring.stiffness;
		ConstraintTemplate->DefaultInstance.ProfileInstance.TwistLimit.Stiffness = VrmToUnrealScaleOffset * Spring.stiffness;

		const auto& RefSkeleton(SkeletalMesh->GetRefSkeleton());
		const int32 BoneIndex1 = RefSkeleton.FindBoneIndex(ConstraintTemplate->DefaultInstance.ConstraintBone1);
		const int32 BoneIndex2 = RefSkeleton.FindBoneIndex(ConstraintTemplate->DefaultInstance.ConstraintBone2);
		if (BoneIndex1 == INDEX_NONE || BoneIndex2 == INDEX_NONE)
		{
			return ConstraintTemplate;
		}
		check(BoneIndex1 != INDEX_NONE);
		check(BoneIndex2 != INDEX_NONE);

		const TArray<FTransform>& RawRefBonePose = RefSkeleton.GetRawRefBonePose();
		const FTransform BoneTransform2 = RawRefBonePose[BoneIndex2]; // EditorSkelComp->GetBoneTransform(BoneIndex2);

		int32 BoneIndex = BoneIndex2;
		while (true)
		{
			const int32 ParentIndex = RefSkeleton.GetRawParentIndex(BoneIndex);
			if (ParentIndex < 0)
			{
				break;
			}
			if (ParentIndex == BoneIndex1)
			{
				break;
			}
			BoneIndex = ParentIndex;
		}

		// auto b = BoneTransform1;
		ConstraintTemplate->DefaultInstance.Pos1 = FVector::ZeroVector;
		ConstraintTemplate->DefaultInstance.PriAxis1 = FVector::ForwardVector;
		ConstraintTemplate->DefaultInstance.SecAxis1 = FVector::RightVector;

		const auto BoneTransform = BoneTransform2; // .GetRelativeTransform(BoneTransform2);
		// auto r = BoneTransform1.GetRelativeTransform(BoneTransform2);
		// const auto Twist = BoneTransform.GetLocation().GetSafeNormal();
		// auto P1 = Twist;
		// P1.X = P1.Z = 0.f;
		// const auto P2 = FVector::CrossProduct(Twist, P1).GetSafeNormal();
		// P1 = FVector::CrossProduct(P2, Twist).GetSafeNormal();

		ConstraintTemplate->DefaultInstance.Pos2 = -BoneTransform.GetLocation();
		//ct->DefaultInstance.PriAxis2 = P1;
		//ct->DefaultInstance.SecAxis2 = P2;
		ConstraintTemplate->DefaultInstance.PriAxis2 = BoneTransform.GetUnitAxis(EAxis::X);
		ConstraintTemplate->DefaultInstance.SecAxis2 = BoneTransform.GetUnitAxis(EAxis::Y);

		// child 
		// ConstraintTemplate->DefaultInstance.SetRefFrame(EConstraintFrame::Frame1, FTransform::Identity);
		// parent
		// ConstraintTemplate->DefaultInstance.SetRefFrame(EConstraintFrame::Frame2, BoneTransform1.GetRelativeTransform(BoneTransform2));
#if WITH_EDITOR
		ConstraintTemplate->SetDefaultProfile(ConstraintTemplate->DefaultInstance);
#endif // WITH_EDITOR
		// ConstraintTemplate->DefaultInstance.InitConstraint();
		return ConstraintTemplate;
	}
#endif
	static void CreateSwingTail(const USkeletalMesh* SkeletalMesh, /*const VRM::VRMSpring& Spring,*/ const FName& BoneName,
	                            const USkeletalBodySetup* BodySetup, const int BodyIndex1, TArray<int>& SwingBoneIndexArray)
	{
		const auto& ReferenceSkeleton = SkeletalMesh->GetSkeleton()->GetReferenceSkeleton();
		const int32 FindBoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
		const int32 NumBones = ReferenceSkeleton.GetRawBoneNum();
		TArray<int32> Children;
		for (int32 ChildIndex = FindBoneIndex + 1; ChildIndex < NumBones; ChildIndex++)
		{
			if (FindBoneIndex == ReferenceSkeleton.GetParentIndex(ChildIndex))
			{
				Children.Add(ChildIndex);
			}
		}
		UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset();
		for (const auto& Child : Children)
		{
			const auto& ChildBoneName = ReferenceSkeleton.GetBoneName(Child).ToString().ToLower(); 
			if (AddedList.Find(ChildBoneName) >= 0)
			{
				continue;
			}
			AddedList.Add(ChildBoneName);
			// USkeletalBodySetup* const SkeletalBodySetup = Cast<USkeletalBodySetup>(StaticDuplicateObject(BodySetup, PhysicsAsset, NAME_None));
			USkeletalBodySetup* SkeletalBodySetup = nullptr;
			const auto GameThreadTaskFunction(MakeShared<TFunction<void()>, ESPMode::ThreadSafe>());
			*GameThreadTaskFunction = [&]
			{
				SkeletalBodySetup = Cast<USkeletalBodySetup>(StaticDuplicateObject(BodySetup, PhysicsAsset, NAME_None));
			};
			UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadTaskFunction);
			SkeletalBodySetup->BoneName = ReferenceSkeleton.GetBoneName(Child);
			SkeletalBodySetup->PhysicsType = PhysType_Simulated;
			SkeletalBodySetup->CollisionReponse = EBodyCollisionResponse::BodyCollision_Enabled;

			const int BodyIndex2 = PhysicsAsset->SkeletalBodySetups.Add(SkeletalBodySetup);
			// const auto* PhysicsConstraintTemplate = CreateConstraint(SkeletalMesh, PhysicsAsset, Spring, BoneName, SkeletalBodySetup->BoneName);
			PhysicsAsset->DisableCollision(BodyIndex1, BodyIndex2);

			SwingBoneIndexArray.AddUnique(BodyIndex2);
			// CreateSwingTail(SkeletalMesh, SkeletalBodySetup->BoneName, SkeletalBodySetup, BodyIndex2, SwingBoneIndexArray);
		}
	}

	static void CreateSwingHead(const USkeletalMesh* SkeletalMesh, const VRM::VRMSpring& Spring, const FName& BoneName, TArray<int>& SwingBoneIndexArray)
	{
		if (SkeletalMesh->GetRefSkeleton().FindRawBoneIndex(BoneName) == INDEX_NONE)
		{
			return;
		}
		UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset();
		USkeletalBodySetup* BodySetup = nullptr;
		int BodyIndex1 = INDEX_NONE;
		if (const auto& LowerBoneName(BoneName.ToString().ToLower()); AddedList.Find(LowerBoneName) < 0)
		{
			AddedList.Add(LowerBoneName);
			// BodySetup = NewObject<USkeletalBodySetup>(PhysicsAsset, NAME_None, RF_Transactional);
			BodySetup = NewObject<USkeletalBodySetup>(PhysicsAsset, NAME_None, TransientFlag);
			FKAggregateGeom AggregateGeom;
			FKSphereElem SphereElem;
			SphereElem.Center = FVector(0);
			SphereElem.Radius = Spring.hitRadius * VrmToUnrealScaleOffset;
			AggregateGeom.SphereElems.Add(SphereElem);

			// BodySetup->Modify();
			BodySetup->BoneName = BoneName;
			BodySetup->AddCollisionFrom(AggregateGeom);
			BodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
			// newly created bodies default to simulating
			BodySetup->PhysicsType = PhysType_Kinematic; // fix
			//bs->get
			BodySetup->CollisionReponse = EBodyCollisionResponse::BodyCollision_Disabled;
			BodySetup->DefaultInstance.InertiaTensorScale.Set(2, 2, 2);
			BodySetup->DefaultInstance.LinearDamping = 10.0f * Spring.dragForce;
			BodySetup->DefaultInstance.AngularDamping = 10.0f * Spring.dragForce;

			BodySetup->InvalidatePhysicsData();
			BodySetup->CreatePhysicsMeshes();
			BodyIndex1 = PhysicsAsset->SkeletalBodySetups.Add(BodySetup);
			// PhysicsAsset->UpdateBodySetupIndexMap();
// #if WITH_EDITOR
			// PhysicsAsset->InvalidateAllPhysicsMeshes();
// #endif // WITH_EDITOR
			if (BodyIndex1 >= 0)
			{
				CreateSwingTail(SkeletalMesh, BoneName, BodySetup, BodyIndex1, SwingBoneIndexArray);
			}
			return;
		}
		for (int i = 0; i < PhysicsAsset->SkeletalBodySetups.Num(); ++i)
		{
			const auto& SkeletalBodySetup = PhysicsAsset->SkeletalBodySetups[i];
			if (SkeletalBodySetup->BoneName != BoneName)
			{
				continue;
			}
			BodyIndex1 = i;
			BodySetup = SkeletalBodySetup;
			break;
		}
		if (BodyIndex1 >= 0)
		{
			CreateSwingTail(SkeletalMesh, BoneName, BodySetup, BodyIndex1, SwingBoneIndexArray);
		}
	}
} // namespace VRoid

bool VRoidConverter::ConvertModel(UVrmAssetListObject* const VrmAssetList) const
{
	static double StartTime = 0.f;
	static double DeltaTime = 0.f;
	static double StartMeshTime = 0.f;
	static double MeshDeltaTime = 0.f;
	constexpr auto Log = [](const FString& Str, const bool ForceOutput = false)
	{
		if (CVarVRoidRuntimeMeshLoadVerboseLog.GetValueOnAnyThread() == false)
		{
			return;
		}
		const double MsLoadTime = (FPlatformTime::Seconds() - StartTime) * 1000.0;
		const double Delta = MsLoadTime - DeltaTime < 0.0 ? 0.0 : MsLoadTime - DeltaTime;
		// if (Delta > 1.0 || ForceOutput)
		{
			VROID_LOG(TEXT("VRoid ConvertModel=%05.2lf ms (delta: %05.2lf ms) (%s)"), MsLoadTime, Delta, *Str);
		}
		DeltaTime = MsLoadTime;
	};
	constexpr auto MeshLog = [](const FString& Str)
	{
		if (CVarVRoidRuntimeMeshLoadVerboseLog.GetValueOnAnyThread() == false)
		{
			return;
		}
		const double MsLoadTime = (FPlatformTime::Seconds() - StartMeshTime) * 1000.0;
		const double Delta = MsLoadTime - MeshDeltaTime < 0.0 ? 0.0 : MsLoadTime - MeshDeltaTime;
		// if (Delta > 0.3)
		{
			VROID_LOG(TEXT("VRoid ConvertModelMesh=%05.2lf ms (delta: %05.2lf ms) (%s)"), MsLoadTime, Delta, *Str);
		}
		MeshDeltaTime = MsLoadTime;
	};

	if (VrmAssetList == nullptr)
	{
		return false;
	}
	if (AiScene == nullptr)
	{
		VROID_WARNING(TEXT("test null."));
	}

	StartTime = FPlatformTime::Seconds();
	DeltaTime = FPlatformTime::Seconds() * 1000.0;
	Log(TEXT("Init"));

	VrmAssetList->MeshReturnedData = MakeShareable(new FReturnedData());
	FReturnedData& MeshReturnedData = *(VrmAssetList->MeshReturnedData);
	MeshReturnedData.bSuccess = false;
	MeshReturnedData.meshInfo.Empty();
	MeshReturnedData.NumMeshes = 0;
	if (AiScene->HasMeshes() && VRMConverter::Options::Get().IsDebugNoMesh() == false)
	{
		// remove degenerate triangles
		if (VRMConverter::Options::Get().IsRemoveDegenerateTriangles())
		{
			// for (uint32_t MeshIndex = 0; MeshIndex < AiScene->mNumMeshes; ++MeshIndex)
			ParallelFor(AiScene->mNumMeshes, [&](const uint32_t MeshIndex)
			{
				const auto& Mesh = AiScene->mMeshes[MeshIndex];
				// Triangle number
				// for (uint32_t FaceIndex = 0; FaceIndex < Mesh->mNumFaces; ++FaceIndex)
				ParallelFor(Mesh->mNumFaces, [&](const uint32_t FaceIndex)
				{
					const auto& Face = Mesh->mFaces[FaceIndex];
					for (uint32_t Indices = 0; Indices < Face.mNumIndices; ++Indices)
					{
						if ((Indices % 3 == 0) && (Indices + 2 < Face.mNumIndices))
						{
							const uint32_t TmpIndex[3] = {
								Face.mIndices[Indices], Face.mIndices[Indices + 1], Face.mIndices[Indices + 2]
							};
							const aiVector3D Vertices[2] = {
								Mesh->mVertices[TmpIndex[0]] - Mesh->mVertices[TmpIndex[2]],
								Mesh->mVertices[TmpIndex[1]] - Mesh->mVertices[TmpIndex[2]],
							};
							if ((Vertices[0] ^ Vertices[1]).SquareLength() == 0)
							{
								VROID_WARNING(TEXT("degenerate face %d"), FaceIndex);
								// del
								Face.mIndices[Indices] = 0;
								Face.mIndices[Indices + 1] = 0;
								Face.mIndices[Indices + 2] = 0;
								Indices += 2;
								continue;
							}
						}
					}
				});
			});
			Log(TEXT("IsRemoveDegenerateTriangles"));
		}
		// find and remove unused vertex
#if	UE_OLDER_5_5
		MeshReturnedData.meshInfo.SetNum(AiScene->mNumMeshes, false);
#else
		MeshReturnedData.meshInfo.SetNum(AiScene->mNumMeshes, EAllowShrinking::No);
#endif // UE_OLDER_5_5
		VRoid::FindMesh(AiScene, AiScene->mRootNode, MeshReturnedData);
		// for (uint32_t MeshIndex = 0; MeshIndex < AiScene->mNumMeshes; ++MeshIndex)
		ParallelFor(AiScene->mNumMeshes, [&](const uint32_t MeshIndex)
		{
			const auto& Mesh = AiScene->mMeshes[MeshIndex];
			// Triangle number
			for (uint32_t FaceIndex = 0; FaceIndex < Mesh->mNumFaces; ++FaceIndex)
			{
				const auto& Face = Mesh->mFaces[FaceIndex];
				for (uint32_t Indices = 0; Indices < Face.mNumIndices; ++Indices)
				{
					if ((Indices % 3 == 0) && (Indices + 2 < Face.mNumIndices))
					{
						if ((Face.mIndices[Indices] + Face.mIndices[Indices + 1] + Face.mIndices[Indices + 2]) == 0)
						{
							// remove
							Indices += 2;
							continue;
						}
					}
					MeshReturnedData.meshInfo[MeshIndex].Triangles.Add(Face.mIndices[Indices]);
				}
			}
		});
		MeshReturnedData.bSuccess = true;
		Log(TEXT("remove unused vertex"));
	}

	USkeletalMesh* NewSkeletalMesh = NewObject<USkeletalMesh>(GetTransientPackage(), NAME_None, VRoid::TransientFlag);
	UVrmAssetUserData* AssetUserData = NewObject<UVrmAssetUserData>(NewSkeletalMesh, NAME_None, VRoid::TransientFlag);
	AssetUserData->VrmAssetListObject = VrmAssetList;
	NewSkeletalMesh->AddAssetUserData(AssetUserData);
#if WITH_EDITOR
	NewSkeletalMesh->PreEditChange(nullptr);
#endif // WITH_EDITOR
	USkeleton* Skeleton = VRMConverter::Options::Get().GetSkeleton();
	const bool bCreateSkeleton = (Skeleton == nullptr);
	if (bCreateSkeleton)
	{
		Skeleton = NewObject<USkeleton>(GetTransientPackage(), NAME_None, VRoid::TransientFlag);
	}
	int AllIndex = 0;
	int AllVertex = 0;
	int UVNum = 1;
	for (const auto& MeshInfo : MeshReturnedData.meshInfo)
	{
		AllIndex += MeshInfo.Triangles.Num();
		AllVertex += MeshInfo.Vertices.Num();
		UVNum = FMath::Clamp(UVNum, MeshInfo.UV0.Num(), MAX_TEXCOORDS);
#if UE_BUILD_DEVELOPMENT
		// if (UVNum >= 2)
		// {
		// 	VROID_WARNING(TEXT("test uv2."));
		// }
#endif // UE_BUILD_DEVELOPMENT
	}
	static int BoneOffset = 0;
	NewSkeletalMesh->SetSkeleton(Skeleton);
	// Skeleton->MergeAllBonesToBoneTree(NewSkeletalMesh);
	USkeletalMesh* TempSkeletalMesh = NewObject<USkeletalMesh>(GetTransientPackage(), NAME_None, VRoid::TransientFlag);
	FReferenceSkeleton& TempRefSkeleton(TempSkeletalMesh->GetRefSkeleton());
	VRMSkeleton::readVrmBone(const_cast<aiScene*>(AiScene), BoneOffset, TempRefSkeleton, VrmAssetList);
	Log(TEXT("read vrm bone"));
#if 0
	// force set PMX bone table for addIK.
	if (VRMConverter::Options::Get().IsPMXModel())
	{
		// add meta pmx bone map.
		for (const auto& [PmxBoneUE4, PmxBoneVRM] : VRMUtil::table_ue4_pmx)
		{
			FString PmxBone;
			VRMUtil::GetReplacedPMXBone(PmxBone, PmxBoneVRM);
			const FString TargetList[2] = {PmxBone, PmxBoneVRM,};
			bool bFinish = false;
			for (const auto Target : TargetList)
			{
				if (TempRefSkeleton.FindBoneIndex(*Target) == INDEX_NONE)
				{
					continue;
				}
				for (const auto& [BoneUE4, BoneVRM] : VRMUtil::table_ue4_vrm)
				{
					if (BoneUE4 != PmxBoneUE4)
					{
						continue;
					}
					if (BoneVRM.IsEmpty())
					{
						continue;
					}
					// renew bone map
					VrmAssetList->VrmMetaObject->humanoidBoneTable.Add(BoneVRM) = Target;
					bFinish = true;
					break;
				}
				if (bFinish)
				{
					break;
				}
			} // 2 loop
			VrmAssetList->VrmMetaObject->humanoidBoneTable.Add("leftEye") = TEXT("左目");
			VrmAssetList->VrmMetaObject->humanoidBoneTable.Add("rightEye") = TEXT("右目");
		}
	} // pmx bone table end
#endif

	VRMSkeleton::addIKBone(VrmAssetList, TempSkeletalMesh);
	Log(TEXT("AddIkBone"));
#if 0
	USkeleton* TempSkeleton = NewObject<USkeleton>(GetTransientPackage(), NAME_None, VRoid::TransientFlag);
	TempSkeleton->readVrmBone(const_cast<aiScene*>(AiScene), BoneOffset);
	TempSkeleton->addIKBone(VrmAssetList);
	TempSkeletalMesh->SetSkeleton(TempSkeleton);
	TempSkeletalMesh->SetRefSkeleton(TempSkeleton->GetReferenceSkeleton());
	TempSkeleton->SetPreviewMesh(TempSkeletalMesh);
	TempSkeleton->UpdateReferencePoseFromMesh(TempSkeletalMesh);
	TempSkeleton->RecreateBoneTree(TempSkeletalMesh);
#endif
	if (Skeleton->MergeAllBonesToBoneTree(TempSkeletalMesh) == false)
	{
		// Import failure.
		VROID_ERROR(TEXT("VRoid: Failed MergeAllBonesToBoneTree \"%s\" to \"%s\""), *NewSkeletalMesh->GetName(), *Skeleton->GetName());
		NewSkeletalMesh->SetSkeleton(nullptr);
		NewSkeletalMesh->ConditionalBeginDestroy();
		if (bCreateSkeleton)
		{
			Skeleton->ConditionalBeginDestroy();
		}
		return false;
	}

	NewSkeletalMesh->SetRefSkeleton(Skeleton->GetReferenceSkeleton());
	FReferenceSkeleton& ReferenceSkeleton(NewSkeletalMesh->GetRefSkeleton());
	if (bCreateSkeleton == false)
	{
		// Update ref pose
		FReferenceSkeletonModifier ReferenceSkeletonModifier(ReferenceSkeleton, Skeleton);
		const auto& SrcReferenceSkeleton = TempRefSkeleton;
		auto& DstReferenceSkeleton = ReferenceSkeleton;

		for (int BoneIndex = 0; BoneIndex < SrcReferenceSkeleton.GetRawBoneNum(); ++BoneIndex)
		{
			const auto BoneName = SrcReferenceSkeleton.GetBoneName(BoneIndex);
			if (const auto FindBoneIndex = DstReferenceSkeleton.FindRawBoneIndex(BoneName); FindBoneIndex >= 0)
			{
				const auto BonePose = SrcReferenceSkeleton.GetRawRefBonePose()[BoneIndex];
				ReferenceSkeletonModifier.UpdateRefPoseTransform(FindBoneIndex, BonePose);
			}
		}
#if 0
		for (int i = 0; i < ReferenceSkeleton.GetRawBoneNum(); ++i)
		{
			const auto BonePose = SrcReferenceSkeleton.GetRawRefBonePose()[i];
			ReferenceSkeletonModifier.UpdateRefPoseTransform(i, BonePose);
		}
#endif
	}
	NewSkeletalMesh->CalculateInvRefMatrices();
	NewSkeletalMesh->CalculateExtendedBounds();
#if WITH_EDITOR
	NewSkeletalMesh->UpdateGenerateUpToData();
#if WITH_EDITORONLY_DATA
	NewSkeletalMesh->ConvertLegacyLODScreenSize();
	Skeleton->SetPreviewMesh(NewSkeletalMesh);
#endif // WITH_EDITORONLY_DATA
#endif // WITH_EDITOR
	if (bCreateSkeleton)
	{
		Skeleton->RecreateBoneTree(NewSkeletalMesh);
	}
	// sk end
	Log(TEXT("SkEnd"));

	VrmAssetList->SkeletalMesh = NewSkeletalMesh;
	if (NewSkeletalMesh->GetLODInfo(0) == nullptr)
	{
		NewSkeletalMesh->AddLODInfo();
	}

	bool bVrm10UseBindToRestPose = true;
	if (const UScriptStruct* ImportOptionType = TBaseStructure<FImportOptionData>::Get())
	{
		if (const FProperty* Property = ImportOptionType->FindPropertyByName(TEXT("bVrm10UseBindToRestPose")); Property)
		{
			if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
			{
				bVrm10UseBindToRestPose = BoolProperty->GetPropertyValue(&VRMConverter::Options::Get().ImportOption);
			}
		}
	}
	// if (VRMConverter::Options::Get().IsVRM10Model() &&
	if (IsVrm10Model &&
		VRMConverter::Options::Get().IsVRM10Bindpose() == false &&
		VRMConverter::Options::Get().IsDebugOneBone() == false &&
		bVrm10UseBindToRestPose == true)
	{
		if (VrmAssetList->Pose_bind.Num() == 0 || VrmAssetList->Pose_tpose.Num() == 0)
		{
			VROID_WARNING(TEXT("BindPose -> TPose :: no bind pose array!"));
		}
		else
		{
			auto& MeshInfo = VrmAssetList->MeshReturnedData->meshInfo;
			struct FWeightData
			{
				FString BoneName;
				float Weight = 0;
			};
			TMap<int, TArray<FWeightData>> WeightTable;
			const auto* Scene = const_cast<aiScene*>(AiScene);
			// generate weightTable
			int VertexOffset = 0;
			for (uint32_t MeshIndex = 0; MeshIndex < Scene->mNumMeshes; ++MeshIndex)
			{
				auto* Mesh = Scene->mMeshes[MeshIndex];
				for (uint32_t BoneIndex = 0; BoneIndex < Mesh->mNumBones; ++BoneIndex)
				{
					auto* Bone = Mesh->mBones[BoneIndex];
					for (uint32_t WeightIndex = 0; WeightIndex < Bone->mNumWeights; ++WeightIndex)
					{
						auto Weight = Bone->mWeights[WeightIndex];
						FWeightData WeightData;
						WeightData.BoneName = UTF8_TO_TCHAR(Bone->mName.C_Str());
						WeightData.Weight = Weight.mWeight;
						WeightTable.FindOrAdd(VertexOffset + Weight.mVertexId).Add(WeightData);
					}
				}
				VertexOffset += Mesh->mNumVertices;
			}
			// weight check
			for (auto w : WeightTable)
			{
				float f = 0.f;
				for (const auto [BoneName, Weight] : w.Value)
				{
					f += Weight;
				}
#if UE_BUILD_DEVELOPMENT
				if (fabs(f - 1.f) > 0.01f)
				{
					VROID_WARNING(TEXT("BindPose -> TPose :: bad weight!"));
				}
#endif // UE_BUILD_DEVELOPMENT
			} // end weightTable

			// bind pose -> t pose
			VertexOffset = 0;
			// ParallelFor(Scene->mNumMeshes, [&](const uint32_t MeshIndex)
			for (uint32_t MeshIndex = 0; MeshIndex < Scene->mNumMeshes; ++MeshIndex)
			{
				if (MeshInfo.IsValidIndex(MeshIndex) == false)
				{
					// return;
					continue;
				}
				const auto* Mesh = Scene->mMeshes[MeshIndex];
				for (int VertexIndex = 0; VertexIndex < MeshInfo[MeshIndex].Vertices.Num(); ++VertexIndex)
				// ParallelFor(MeshInfo[MeshIndex].Vertices.Num(), [&](const int VertexIndex)
				{
					const int VertexId(VertexOffset + VertexIndex);
					if (WeightTable.Find(VertexId) == nullptr)
					{
						VROID_WARNING(TEXT("BindPose -> TPose :: no weight data %d"), VertexId);
						// return;
						continue;
					}
					// FVector VertexOrig = MeshInfo[MeshNo].Vertices[VertexNo];
					// VertexOrig.Set(Mesh->mVertices[VertexNo].x, -Mesh->mVertices[VertexNo].z, Mesh->mVertices[VertexNo].y);
					const FVector VertexOrig(Mesh->mVertices[VertexIndex].x, -Mesh->mVertices[VertexIndex].z, Mesh->mVertices[VertexIndex].y);
					FVector Vertex(FVector::ZeroVector);
					for (const auto [BoneName, Weight] : WeightTable[VertexOffset + VertexIndex])
					// ParallelFor(WeightTable[VertexId].Num(), [&](const int WeightTableIndex)
					{
						// const auto [BoneName, Weight] = WeightTable[VertexId][WeightTableIndex];
						const auto TPose = VrmAssetList->Pose_tpose.Find(BoneName);
						const auto BindPose = VrmAssetList->Pose_bind.Find(BoneName);
						if (TPose && BindPose)
						{
							const FVector Diff = (BindPose->Inverse() * *TPose).TransformPosition(VertexOrig * VRoid::VrmToUnrealScaleOffset);
							Vertex += Diff * Weight;
#if UE_BUILD_DEVELOPMENT
							if (Vertex.Length() >= VRoid::MaxWeight)
							{
								VROID_WARNING(TEXT("BindPose -> TPose :: bad weight!"));
							}
						}
						else
						{
							VROID_WARNING(TEXT("BindPose -> TPose :: no pose transform %p %p"), TPose, BindPose);
						}
#else
						}
#endif // UE_BUILD_DEVELOPMENT
					}
					Vertex.Set(Vertex.X, Vertex.Z, -Vertex.Y);
					MeshInfo[MeshIndex].Vertices[VertexIndex] = Vertex / VRoid::VrmToUnrealScaleOffset;
				}
				VertexOffset += Mesh->mNumVertices;
			}
		}
	} // end bind -> t pose
	Log(TEXT("EndBind"));

	// begin vertex
	// NewSkeletalMesh->CacheDerivedData();
	NewSkeletalMesh->AllocateResourceForRendering();
	FSkeletalMeshRenderData* MeshRenderData = NewSkeletalMesh->GetResourceForRendering();
	// MeshRenderData->Cache(NewSkeletalMesh);
	// NewSkeletalMesh->OnPostMeshCached().Broadcast(SkeletalMesh);

	if (MeshRenderData->LODRenderData.Num() == 0)
	{
		auto* Tmp = new FSkeletalMeshLODRenderData();
		MeshRenderData->LODRenderData.Add(Tmp);
	}
	FSkeletalMeshLODRenderData& SkeletalMeshLODRenderData = MeshRenderData->LODRenderData[0];
	if (AllVertex > 0)
	{
		SkeletalMeshLODRenderData.StaticVertexBuffers.PositionVertexBuffer.Init(AllVertex);
		SkeletalMeshLODRenderData.StaticVertexBuffers.ColorVertexBuffer.InitFromSingleColor(FColor(255, 255, 255, 255), AllVertex);
		SkeletalMeshLODRenderData.StaticVertexBuffers.StaticMeshVertexBuffer.CleanUp();
		SkeletalMeshLODRenderData.StaticVertexBuffers.StaticMeshVertexBuffer.Init(AllVertex, UVNum);
	}
	TArray<VRoid::FSoftSkinVertexRuntime> SoftSkinVertexArray;
	SoftSkinVertexArray.SetNum(AllVertex);
	TArray<FSkinWeightInfo> VertexWeightInfos;
	VertexWeightInfos.AddUninitialized(AllVertex);
	// for (int32 VertexIndex = 0; VertexIndex < AllVertex; VertexIndex++)
	ParallelFor(AllVertex, [&](const int32 VertexIndex)
	{
		const VRoid::FSoftSkinVertexRuntime& SrcVertex = SoftSkinVertexArray[VertexIndex];
		FMemory::Memcpy(VertexWeightInfos[VertexIndex].InfluenceBones, SrcVertex.InfluenceBones, MAX_TOTAL_INFLUENCES * sizeof(FBoneIndexType));
		FMemory::Memcpy(VertexWeightInfos[VertexIndex].InfluenceWeights, SrcVertex.InfluenceWeights, MAX_TOTAL_INFLUENCES * sizeof(uint16));
	});
	SkeletalMeshLODRenderData.SkinWeightVertexBuffer = VertexWeightInfos;
	// NewSkeletalMesh->InitResources();
	const auto GameThreadTaskFunction(MakeShared<TFunction<void()>, ESPMode::ThreadSafe>());
	*GameThreadTaskFunction = [&]
	{
		NewSkeletalMesh->InitResources();
	};
	UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadTaskFunction);
	TArray<FSkeletalMaterial>& Materials = NewSkeletalMesh->GetMaterials();
	if (VRMConverter::Options::Get().IsDebugNoMaterial() == false)
	{
		Materials.SetNum(VrmAssetList->Materials.Num());
	}
	for (int i = 0; i < Materials.Num(); ++i)
	{
		Materials[i].MaterialInterface = VrmAssetList->Materials[i];
		Materials[i].MaterialSlotName = UTF8_TO_TCHAR(AiScene->mMaterials[i]->GetName().C_Str());
		Materials[i].UVChannelData = FMeshUVChannelInfo(1);
	}
	if (Materials.IsEmpty())
	{
		Materials.SetNum(1);
	}

	const auto& NewRefSkeleton = NewSkeletalMesh->GetSkeleton()->GetReferenceSkeleton();
	const int BoneNum = NewRefSkeleton.GetRawBoneNum();
	SkeletalMeshLODRenderData.RequiredBones.SetNum(BoneNum);
	SkeletalMeshLODRenderData.ActiveBoneIndices.SetNum(BoneNum);
	// for (int i = 0; i < BoneNum; ++i)
	ParallelFor(BoneNum, [&](const int i)
	{
		SkeletalMeshLODRenderData.RequiredBones[i] = i;
		SkeletalMeshLODRenderData.ActiveBoneIndices[i] = i;
	});

	VRoid::FSoftSkinVertexRuntime SoftSkinVertexLocalZero;
	SoftSkinVertexLocalZero.Position = SoftSkinVertexLocalZero.TangentX = SoftSkinVertexLocalZero.TangentY = FVector3f::Zero();
	SoftSkinVertexLocalZero.TangentZ.Set(0, 0, 0, 1);
	SoftSkinVertexLocalZero.Color = FColor::White;
	memset(SoftSkinVertexLocalZero.UVs, 0, sizeof(SoftSkinVertexLocalZero.UVs));
	memset(SoftSkinVertexLocalZero.InfluenceBones, 0, sizeof(SoftSkinVertexLocalZero.InfluenceBones));
	memset(SoftSkinVertexLocalZero.InfluenceWeights, 0, sizeof(SoftSkinVertexLocalZero.InfluenceWeights));

	TArray<VRoid::FSoftSkinVertexRuntime> SoftSkinVertices;
	SoftSkinVertices.Init(SoftSkinVertexLocalZero, AllVertex);
#if WITH_EDITORONLY_DATA
	FSkeletalMeshModel* const ImportedModel = NewSkeletalMesh->GetImportedModel();
	if (ImportedModel != nullptr)
	{
		if (ImportedModel->LODModels.Num() == 0)
		{
			ImportedModel->LODModels.Add(new FSkeletalMeshLODModel());
		}
		ImportedModel->LODModels[0].Sections.Empty();
		ImportedModel->LODModels[0].Sections.SetNum(MeshReturnedData.meshInfo.Num());
	}
#endif // WITH_EDITORONLY_DATA

	SkeletalMeshLODRenderData.RenderSections.Empty();
	SkeletalMeshLODRenderData.RenderSections.SetNum(MeshReturnedData.meshInfo.Num());
	Log(TEXT("SoftSkinVertex"));

	StartMeshTime = FPlatformTime::Seconds();
	MeshDeltaTime = FPlatformTime::Seconds() * 1000.0;
	FStaticMeshVertexBuffers& StaticMeshVertexBuffers = SkeletalMeshLODRenderData.StaticVertexBuffers;
	TArray<uint32> Triangles;
	TArray<int> AllActiveBones;
	int CurrentIndex = 0;
	int CurrentVertex = 0;
	MeshLog(TEXT("Init"));
	for (int MeshIndex = 0; MeshIndex < MeshReturnedData.meshInfo.Num(); ++MeshIndex)
	{
		TArray<VRoid::FSoftSkinVertexRuntime> MeshWeight;
		auto& MeshInfo = MeshReturnedData.meshInfo[MeshIndex];
		MeshWeight.Init(SoftSkinVertexLocalZero, MeshInfo.Vertices.Num());
		for (int v = 0; v < MeshInfo.Vertices.Num(); ++v)
		// ParallelFor(MeshInfo.Vertices.Num(), [&](const int v)
		{
			const int VertexIndex(CurrentVertex + v);
			const auto VertexPosition = MeshInfo.Vertices[v] * VRoid::VrmToUnrealScaleOffset;
			auto& VertexBufferPosition(StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex));
			VertexBufferPosition.Set(VertexPosition.X, VertexPosition.Z, VertexPosition.Y);
			// if (VRMConverter::Options::Get().IsVRM10Model() || VRMConverter::Options::Get().IsPMXModel() || VRMConverter::Options::Get().IsBVHModel())
			if (IsVrm10Model)
			{
				VertexBufferPosition.Y *= -1.f;
			}
			else
			{
				VertexBufferPosition.X *= -1.f;
			}
			VertexBufferPosition *= VRMConverter::Options::Get().GetModelScale();

			for (int u = 0; u < FMath::Min(MeshInfo.UV0.Num(), static_cast<int>(MAX_TEXCOORDS)); ++u)
			{
				const FVector2D UV(v < MeshInfo.UV0[u].Num() ? MeshInfo.UV0[u][v] : FVector2D::ZeroVector);
				StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(VertexIndex, u, FVector2f(UV));
				MeshWeight[v].UVs[u] = FVector2f(UV);
			}

			if (v < MeshInfo.Tangents.Num())
			{
				// StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(VertexIndex, FVector3f(1, 0, 0), FVector3f(0, 1, 0), FVector3f(0, 0, 1));
				// StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(VertexIndex, MeshInfo.Tangents);
				const auto& Normal = MeshInfo.Normals[v];
				FVector TmpNormal(Normal.X, Normal.Z, Normal.Y);
				FVector TmpTangent(MeshInfo.Tangents[v].X, MeshInfo.Tangents[v].Z, MeshInfo.Tangents[v].Y);
				// if (VRMConverter::Options::Get().IsVRM10Model() || VRMConverter::Options::Get().IsPMXModel() || VRMConverter::Options::Get().IsBVHModel())
				if (IsVrm10Model)
				{
					TmpNormal.Y *= -1.f;
					TmpTangent.Y *= -1.f;
				}
				else
				{
					TmpNormal.X *= -1.f;
					TmpTangent.X *= -1.f;
				}
				TmpNormal.Normalize();
				TmpTangent.Normalize();

				MeshWeight[v].TangentX = FVector3f(TmpTangent);
				MeshWeight[v].TangentY = FVector3f(TmpNormal ^ TmpTangent);
				MeshWeight[v].TangentZ = FVector4f(FVector3f(TmpNormal), 1);
				StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(VertexIndex, MeshWeight[v].TangentX, MeshWeight[v].TangentY, MeshWeight[v].TangentZ);
			}
			if (v < MeshInfo.VertexColors.Num())
			{
				auto& VertexColor = MeshInfo.VertexColors[v];
				MeshWeight[v].Color = FColor(VertexColor.R, VertexColor.G, VertexColor.B, VertexColor.A);
			}
			VRoid::FSoftSkinVertexRuntime& SkinVertex = SoftSkinVertices[VertexIndex];
			SkinVertex.Position = VertexBufferPosition;
			MeshWeight[v].Position = VertexBufferPosition;
		} // vertex loop
		MeshLog(TEXT("VertexLoop"));
		const auto& AiMesh = AiScene->mMeshes[MeshIndex];
		TArray<int> BoneMap;
		TArray<VRoid::FBoneMapOption> BoneAll;
		// aiData->mRootNode->mMeshes
		for (uint32_t BoneIndex = 0; BoneIndex < AiMesh->mNumBones; ++BoneIndex)
		{
			const auto& AiBone = AiMesh->mBones[BoneIndex];
			const int FindBoneIndex = ReferenceSkeleton.FindBoneIndex(UTF8_TO_TCHAR(AiBone->mName.C_Str()));
			if (FindBoneIndex < 0)
			{
				continue;
			}
			for (uint32_t WeightIndex = 0; WeightIndex < AiBone->mNumWeights; ++WeightIndex)
			{
				const auto& AiVertexWeight = AiBone->mWeights[WeightIndex];
				if (FMath::IsNearlyZero(AiVertexWeight.mWeight))
				{
					continue;
				}
				if (SoftSkinVertices.IsValidIndex(AiVertexWeight.mVertexId + CurrentVertex) == false)
				{
					continue;
				}
				const float VertexWeight = FMath::Clamp(AiVertexWeight.mWeight, 0.f, 1.f);
				if (VertexWeight < UE::AnimationCore::InvMaxRawBoneWeightFloat)
				{
					continue;
				}
				bool bIsFindBone = false;
				for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
				{
					const auto& SoftSkinVertex = SoftSkinVertices[AiVertexWeight.mVertexId + CurrentVertex];
					if (SoftSkinVertex.InfluenceWeights[i] > 0)
					{
						continue;
					}
					BoneMap.AddUnique(FindBoneIndex);
					AllActiveBones.AddUnique(FindBoneIndex);
					bIsFindBone = true;
					break;
				}
				if (bIsFindBone)
				{
					break;
				}
			}
		}
		// BoneMap.Sort();
		MeshLog(TEXT("BoneLoop"));

		for (uint32_t BoneIndex = 0; BoneIndex < AiMesh->mNumBones; ++BoneIndex)
		// ParallelFor(AiMesh->mNumBones, [&](const uint32_t BoneIndex)
		{
			const auto& AIBone = AiMesh->mBones[BoneIndex];
			const int FindBoneIndex = ReferenceSkeleton.FindBoneIndex(UTF8_TO_TCHAR(AIBone->mName.C_Str()));
			if (FindBoneIndex < 0)
			{
				continue;
			}
			for (uint32_t WeightIndex = 0; WeightIndex < AIBone->mNumWeights; ++WeightIndex)
			// ParallelFor(AIBone->mNumWeights, [&](const uint32_t WeightIndex)
			{
				const auto& AiVertexWeight = AIBone->mWeights[WeightIndex];
				const auto Weight(AiVertexWeight.mWeight);
				if (FMath::IsNearlyZero(Weight))
				{
					continue;
				}
				const uint32_t VertexId(AiVertexWeight.mVertexId);
				if (SoftSkinVertices.IsValidIndex(VertexId + CurrentVertex) == false)
				{
					continue;
				}
				const float VertexWeight = FMath::Clamp(Weight, 0.f, 1.f);
				if (VertexWeight < UE::AnimationCore::InvMaxRawBoneWeightFloat)
				{
					continue;
				}
				for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
				{
					auto& SoftSkinVertex = SoftSkinVertices[VertexId + CurrentVertex];
					if (SoftSkinVertex.InfluenceWeights[i] > 0)
					{
						continue;
					}
					int TabledIndex = BoneMap.AddUnique(FindBoneIndex);
					// int TabledIndex = BoneMap[FindBoneIndex];
					if (TabledIndex == INDEX_NONE)
					{
						VROID_WARNING(TEXT("bone map add error!"));
						TabledIndex = 0;
					}
#if UE_BUILD_DEVELOPMENT
					if (TabledIndex > 255)
					{
						VROID_WARNING(TEXT("bone map over!"));
					}
#endif // UE_BUILD_DEVELOPMENT
					if (VRMConverter::Options::Get().IsDebugOneBone())
					{
						TabledIndex = 0;
					}
					SoftSkinVertex.InfluenceBones[i] = TabledIndex;
					// SoftSkinVertex.InfluenceWeights[i] = static_cast<uint16>(FMath::TruncToInt(VertexWeight * 255.f + (0.5f - KINDA_SMALL_NUMBER)));
					SoftSkinVertex.InfluenceWeights[i] = static_cast<uint16>(FMath::TruncToInt(VertexWeight * UE::AnimationCore::MaxRawBoneWeightFloat));

					MeshWeight[VertexId].InfluenceBones[i] = SoftSkinVertex.InfluenceBones[i];
					MeshWeight[VertexId].InfluenceWeights[i] = SoftSkinVertex.InfluenceWeights[i];

					if (VRMConverter::Options::Get().IsMobileBone())
					{
						auto BoneMapOption = BoneAll.FindByPredicate(
							[&](const VRoid::FBoneMapOption& Option)
							{
								return Option.BoneIndex == FindBoneIndex;
							});
						if (BoneMapOption)
						{
							BoneMapOption->Weight += Weight;
						}
						else
						{
							VRoid::FBoneMapOption MobileBoneMapOption;
							MobileBoneMapOption.BoneIndex = FindBoneIndex;
							MobileBoneMapOption.Weight = Weight;
							BoneAll.Add(MobileBoneMapOption);
						}
					}
					break;
				}
			}
		} // bone loop
		MeshLog(TEXT("BoneWeightLoop"));

		// mobile remap
		if (VRMConverter::Options::Get().IsMobileBone() && BoneAll.Num() > VRoid::MaxMobileBoneNum)
		{
			BoneMap.Sort();
			TMap<int, int> MobileMap;
			auto BoneMapNew = BoneMap;
			while (BoneAll.Num() > VRoid::MaxMobileBoneNum)
			{
				BoneAll.Sort();
				// bone 0 == weight 0
				// search from 1
				const auto& Removed = BoneAll[1];
				int FindParent = Removed.BoneIndex;
				while (FindParent >= 0)
				{
					FindParent = ReferenceSkeleton.GetParentIndex(FindParent);
					auto BoneMapOption = BoneAll.FindByPredicate([&](const VRoid::FBoneMapOption& MobileBoneMapOption)
					{
						return MobileBoneMapOption.BoneIndex == FindParent;
					});
					if (BoneMapOption == nullptr)
					{
						continue;
					}
					BoneMapOption->Weight += Removed.Weight;
					while (auto FindBoneIndex = MobileMap.FindKey(Removed.BoneIndex))
					{
						MobileMap[*FindBoneIndex] = BoneMapOption->BoneIndex;
					}
					MobileMap.FindOrAdd(Removed.BoneIndex) = BoneMapOption->BoneIndex;
					break;
				}
				BoneMapNew.Remove(Removed.BoneIndex);
				BoneAll.RemoveAt(1);
			}
			if (MobileMap.Num())
			{
				for (auto& SoftSkinVertex : SoftSkinVertices)
				{
					for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
					{
						auto& InfluenceBone = SoftSkinVertex.InfluenceBones[i];
						auto& InfluenceWeight = SoftSkinVertex.InfluenceWeights[i];
						if (BoneMap.IsValidIndex(InfluenceBone) == false)
						{
							InfluenceWeight = 0.f;
							InfluenceBone = 0;
							continue;
						}
						const auto SrcBoneIndex = BoneMap[InfluenceBone];
						if (const auto FindIndex = MobileMap.Find(SrcBoneIndex))
						{
							const auto DstBoneIndex = *FindIndex;
							InfluenceBone = BoneMapNew.IndexOfByKey(DstBoneIndex);
						}
						else
						{
							InfluenceBone = BoneMapNew.IndexOfByKey(SrcBoneIndex);
						}
					}
				}
				for (auto& SoftSkinVertex : MeshWeight)
				{
					for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
					{
						auto& InfluenceBone = SoftSkinVertex.InfluenceBones[i];
						auto& InfluenceWeight = SoftSkinVertex.InfluenceWeights[i];
						if (BoneMap.IsValidIndex(InfluenceBone) == false)
						{
							InfluenceWeight = 0.f;
							InfluenceBone = 0;
							continue;
						}
						const auto SrcBoneIndex = BoneMap[InfluenceBone];
						if (const auto FindIndex = MobileMap.Find(SrcBoneIndex))
						{
							const auto DstBoneIndex = *FindIndex;
							InfluenceBone = BoneMapNew.IndexOfByKey(DstBoneIndex);
						}
						else
						{
							InfluenceBone = BoneMapNew.IndexOfByKey(SrcBoneIndex);
						}
					}
				}
			}
			BoneMap = BoneMapNew;
		} // mobile remap
		MeshLog(TEXT("MobileRemap"));

		// normalize weight
		for (auto& SoftSkinVertex : MeshWeight)
		// ParallelFor(MeshWeight.Num(), [&](const int MeshWeightIndex)
		{
			static int WarnCount = 0;
			// auto& SoftSkinVertex = MeshWeight[MeshWeightIndex];
			if (&SoftSkinVertex == &MeshWeight[0])
			{
				WarnCount = 0;
			}
			// sort by Weight
			for (int i = 0; i < MAX_TOTAL_INFLUENCES - 1; ++i)
			{
				for (int j = i + 1; j < MAX_TOTAL_INFLUENCES; ++j)
				{
					if (SoftSkinVertex.InfluenceWeights[i] < SoftSkinVertex.InfluenceWeights[j])
					{
						Swap(SoftSkinVertex.InfluenceWeights[i], SoftSkinVertex.InfluenceWeights[j]);
						Swap(SoftSkinVertex.InfluenceBones[i], SoftSkinVertex.InfluenceBones[j]);
					}
				}
			}

			int TotalWeights = 0;
			int MaxWeight = 0;
			for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
			{
				TotalWeights += SoftSkinVertex.InfluenceWeights[i];
				if (MaxWeight < SoftSkinVertex.InfluenceWeights[i])
				{
					MaxWeight = SoftSkinVertex.InfluenceWeights[i];
				}
			}
			if (TotalWeights > UE::AnimationCore::MaxRawBoneWeight)
			{
				VROID_WARNING(TEXT("over"));
				SoftSkinVertex.InfluenceWeights[0] -= static_cast<uint16>(TotalWeights - UE::AnimationCore::MaxRawBoneWeight);
			}
			if (TotalWeights <= (UE::AnimationCore::MaxRawBoneWeight - MAX_TOTAL_INFLUENCES))
			{
				if (WarnCount < VRoid::MaxWarningCount)
				{
					// VROID_WARNING(TEXT("less"));
					WarnCount++;
				}
			}
			if (TotalWeights == 0)
			{
				if (const auto* AINode = VRoid::FindNodeFromMeshIndex(MeshIndex, AiScene))
				{
					const int32 FindBoneIndex(ReferenceSkeleton.FindBoneIndex(UTF8_TO_TCHAR(AINode->mName.C_Str())));
					const int DummyBone = FindBoneIndex == INDEX_NONE ? 0 : FindBoneIndex;
					// add active bone for simple static mesh (not skinned mesh)
					AllActiveBones.AddUnique(DummyBone);
				}
				const auto BoneMapSize = (BoneMap.Num() <= 0) ? 0 : BoneMap.Num();
				SoftSkinVertex.InfluenceBones[0] = BoneMapSize;
				SoftSkinVertex.InfluenceWeights[0] += static_cast<uint16>(UE::AnimationCore::MaxRawBoneWeight -TotalWeights);
			}
			else
			{
				const int BoneWeightInfluenceNum = VRMConverter::Options::Get().GetBoneWeightInfluenceNum();
				// re calc weight
				TotalWeights = 0;
				for (int i = 0; i < BoneWeightInfluenceNum; ++i)
				{
					TotalWeights += SoftSkinVertex.InfluenceWeights[i];
				}
				if (TotalWeights == 0)
				{
					TotalWeights = 1;
				}
				for (int i = BoneWeightInfluenceNum; i < MAX_TOTAL_INFLUENCES; ++i)
				{
					SoftSkinVertex.InfluenceBones[i] = 0;
					SoftSkinVertex.InfluenceWeights[i] = 0;
				}
				int Total = 0;
				for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
				{
					const int t = SoftSkinVertex.InfluenceWeights[i];
					SoftSkinVertex.InfluenceWeights[i] = static_cast<uint16>(FMath::TruncToInt(UE::AnimationCore::MaxRawBoneWeightFloat * t / TotalWeights));
					Total += SoftSkinVertex.InfluenceWeights[i];
				}
				// adjust
				if (Total > UE::AnimationCore::MaxRawBoneWeight)
				{
					VROID_WARNING(TEXT("over"));
					SoftSkinVertex.InfluenceWeights[0] -= static_cast<uint16>(Total - UE::AnimationCore::MaxRawBoneWeight);
				}
				SoftSkinVertex.InfluenceWeights[0] += static_cast<uint16>(UE::AnimationCore::MaxRawBoneWeight - Total);
			}
		} // nomalize weight
		MeshLog(TEXT("Normalize Weight"));

		FSkelMeshRenderSection& NewRenderSection = SkeletalMeshLODRenderData.RenderSections[MeshIndex];
		const bool bUseMergeMaterial = VRMConverter::Options::Get().IsMergeMaterial() &&
									   static_cast<int>(AiMesh->mMaterialIndex) < VrmAssetList->MaterialMergeTable.Num();
		NewRenderSection.MaterialIndex = bUseMergeMaterial ? VrmAssetList->MaterialMergeTable[AiMesh->mMaterialIndex] : AiMesh->mMaterialIndex;
		if (NewRenderSection.MaterialIndex >= VrmAssetList->Materials.Num())
		{
			NewRenderSection.MaterialIndex = 0;
		}
		NewRenderSection.BaseIndex = CurrentIndex;
		NewRenderSection.NumTriangles = MeshInfo.Triangles.Num() / 3;
		// NewRenderSection.bRecomputeTangent = ModelSection.bRecomputeTangent;
		NewRenderSection.bCastShadow = true; // ModelSection.bCastShadow;
		NewRenderSection.BaseVertexIndex = CurrentVertex;
		// currentVertex;// currentVertex;// ModelSection.BaseVertexIndex;
		// NewRenderSection.ClothMappingData = ModelSection.ClothMappingData;
		// NewRenderSection.BoneMap.SetNum(1);//ModelSection.BoneMap;
		// NewRenderSection.BoneMap[0] = 10;
		// NewRenderSection.BoneMap.SetNum(NewSkeletalMesh->GetSkeleton().GetBoneTree().Num());//ModelSection.BoneMap;
		// for (int i = 0; i < NewRenderSection.BoneMap.Num(); ++i)
		// {
		// 	NewRenderSection.BoneMap[i] = i;
		// }
		if (BoneMap.Num() > 0)
		{
			NewRenderSection.BoneMap.SetNum(BoneMap.Num()); // ModelSection.BoneMap;
			for (int i = 0; i < NewRenderSection.BoneMap.Num(); ++i)
			{
				NewRenderSection.BoneMap[i] = BoneMap[i];
			}
		}
		else
		{
			NewRenderSection.BoneMap.SetNum(1);
			const auto* AINode = VRoid::FindNodeFromMeshIndex(MeshIndex, AiScene);
			int32 BoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(UTF8_TO_TCHAR(AINode->mName.C_Str()));
			if (BoneIndex <= 0)
			{
				BoneIndex = MeshIndex;
			}
			NewRenderSection.BoneMap[0] = BoneIndex;
		}
		NewRenderSection.NumVertices = MeshInfo.Vertices.Num();
		// MeshInfo.Triangles.Num();// allVertex;// result.meshInfo[meshID].Vertices.Num();// ModelSection.NumVertices;
		NewRenderSection.MaxBoneInfluences = VRMConverter::Options::Get().GetBoneWeightInfluenceNum(); // ModelSection.MaxBoneInfluences;
		// NewRenderSection.CorrespondClothAssetIndex = ModelSection.CorrespondClothAssetIndex;
		// NewRenderSection.ClothingData = ModelSection.ClothingData;
		TMap<int32, TArray<int32>> OverlappingVertices;
		NewRenderSection.DuplicatedVerticesBuffer.Init(NewRenderSection.NumVertices, OverlappingVertices);
		NewRenderSection.bDisabled = false; // ModelSection.bDisabled;
		// RenderSections.Add(NewRenderSection);
		// rd.RenderSections[0] = NewRenderSection;
		MeshLog(TEXT("Initialize RenderSection"));

#if WITH_EDITORONLY_DATA
		auto& SkelMeshSection = ImportedModel->LODModels[0].Sections[MeshIndex];
		SkelMeshSection.MaterialIndex = 0;
		const bool IsMergeMaterial = VRMConverter::Options::Get().IsMergeMaterial() &&
									 static_cast<int>(AiMesh->mMaterialIndex) < VrmAssetList->MaterialMergeTable.Num();
		SkelMeshSection.MaterialIndex = (IsMergeMaterial) ? VrmAssetList->MaterialMergeTable[AiMesh->mMaterialIndex] : AiMesh->mMaterialIndex;
		if (SkelMeshSection.MaterialIndex >= VrmAssetList->Materials.Num())
		{
			SkelMeshSection.MaterialIndex = 0;
		}
		SkelMeshSection.OriginalDataSectionIndex = MeshIndex;
		SkelMeshSection.BaseIndex = CurrentIndex;
		SkelMeshSection.NumTriangles = MeshInfo.Triangles.Num() / 3;
		SkelMeshSection.BaseVertexIndex = CurrentVertex;
		SkelMeshSection.SoftVertices = MeshWeight;
		if (BoneMap.Num() > 0)
		{
			SkelMeshSection.BoneMap.SetNum(BoneMap.Num()); // ModelSection.BoneMap;
			for (int i = 0; i < SkelMeshSection.BoneMap.Num(); ++i)
			{
				SkelMeshSection.BoneMap[i] = BoneMap[i];
			}
		}
		else
		{
			SkelMeshSection.BoneMap.SetNum(1);
			auto* AINode = VRoid::FindNodeFromMeshIndex(MeshIndex, AiScene);
			int32 i = Skeleton->GetReferenceSkeleton().FindBoneIndex(UTF8_TO_TCHAR(AINode->mName.C_Str()));
			if (i <= 0)
			{
				i = MeshIndex;
			}
			SkelMeshSection.BoneMap[0] = i;
		}
		SkelMeshSection.NumVertices = MeshWeight.Num();
		SkelMeshSection.MaxBoneInfluences = VRMConverter::Options::Get().GetBoneWeightInfluenceNum();
#endif // WITH_EDITORONLY_DATA
		MeshLog(TEXT("Initialize ImportedModel"));

		// rd.MultiSizeIndexContainer.CopyIndexBuffer(MeshReturnedData.meshInfo[0].Triangles);
		const int T1 = Triangles.Num();
		Triangles.Append(MeshInfo.Triangles);
		const int T2 = Triangles.Num();
		for (int i = T1; i < T2; ++i)
		{
			Triangles[i] += CurrentVertex;
		}
		CurrentIndex += MeshInfo.Triangles.Num();
		CurrentVertex += MeshInfo.Vertices.Num();
	} // mesh loop
	Log(TEXT("MeshLoop"));

#if WITH_EDITORONLY_DATA
	if (VRMConverter::Options::Get().IsMergePrimitive())
	{
		// merge lod model section
		auto& LodModel = ImportedModel->LODModels[0];
		for (int MeshId = LodModel.Sections.Num() - 1; MeshId > 0; --MeshId)
		{
			auto& SkelMeshSection0 = LodModel.Sections[MeshId - 1];
			auto& SkelMeshSection1 = LodModel.Sections[MeshId];
			if (AiScene->mMeshes[MeshId]->mNumAnimMeshes > 0)
			{
				// skip skin mesh
				continue;
			}
			if (SkelMeshSection1.NumVertices == 0)
			{
				continue;
			}
			if (SkelMeshSection0.MaterialIndex != SkelMeshSection1.MaterialIndex)
			{
				continue;
			}
			auto NewBoneMap = SkelMeshSection0.BoneMap;
			for (auto& SoftSkinVertex : SkelMeshSection1.SoftVertices)
			{
				for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
				{
					if (SoftSkinVertex.InfluenceWeights[i] == 0)
					{
						continue;
					}
					const int BoneId = SkelMeshSection1.BoneMap[SoftSkinVertex.InfluenceBones[i]];
					if (int FindIndex = 0; NewBoneMap.Find(BoneId, FindIndex))
					{
						SoftSkinVertex.InfluenceBones[i] = FindIndex;
					}
					else
					{
						SoftSkinVertex.InfluenceBones[i] = NewBoneMap.Add(BoneId);
					}
				}
			}
			if (VRMConverter::Options::Get().IsMobileBone())
			{
				if (NewBoneMap.Num() > VRoid::MaxMobileBoneNum)
				{
					continue;
				}
			}
			SkelMeshSection0.SoftVertices.Append(SkelMeshSection1.SoftVertices);
			SkelMeshSection0.NumVertices += SkelMeshSection1.NumVertices;
			SkelMeshSection0.NumTriangles += SkelMeshSection1.NumTriangles;
			SkelMeshSection0.BoneMap = NewBoneMap;
			SkelMeshSection1.SoftVertices.SetNum(0);
			SkelMeshSection1.NumVertices = 0;
		}
		for (int MeshId = 0; MeshId < LodModel.Sections.Num(); ++MeshId)
		{
			if (LodModel.Sections[MeshId].NumVertices > 0)
			{
				continue;
			}
			LodModel.Sections.RemoveAt(MeshId);
			MeshId--;
		}
		Log(TEXT("MergePrimitive"));
	} // merge primitive
#endif // WITH_EDITORONLY_DATA

	int WarnCount = 0;
	for (auto& SoftSkinVertex : SoftSkinVertices)
	// ParallelFor(SoftSkinVertices.Num(), [&](const int SoftSkinIndex)
	{
		// auto& SoftSkinVertex(SoftSkinVertices[SoftSkinIndex]);
		int TotalWeight = 0;
		int MaxIndex = 0;
		int MaxWeight = 0;
#if 0 // !WITH_EDITOR
		for (int i = 0; i < VRMConverter::Options::Get().GetBoneWeightInfluenceNum(); ++i)
		{
			SoftSkinVertex.InfluenceWeights[i] = static_cast<uint16>(SoftSkinVertex.InfluenceWeights[i] / 256 * 256);
		}
#endif // !WITH_EDITOR
		for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
		{
			TotalWeight += SoftSkinVertex.InfluenceWeights[i];
			if (MaxWeight < SoftSkinVertex.InfluenceWeights[i])
			{
				MaxWeight = SoftSkinVertex.InfluenceWeights[i];
				MaxIndex = i;
			}
		}
		if (TotalWeight > UE::AnimationCore::MaxRawBoneWeight)
		{
			VROID_WARNING(TEXT("over"));
			SoftSkinVertex.InfluenceWeights[0] -= static_cast<uint16>(TotalWeight - UE::AnimationCore::MaxRawBoneWeight);
		}
		if (TotalWeight < UE::AnimationCore::MaxRawBoneWeight)
		{
			if (TotalWeight <= (UE::AnimationCore::MaxRawBoneWeight - MAX_TOTAL_INFLUENCES))
			{
				if (WarnCount < VRoid::MaxWarningCount)
				{
					// VROID_WARNING(TEXT("less"));
					WarnCount++;
				}
			}
			SoftSkinVertex.InfluenceWeights[MaxIndex] += static_cast<uint16>(UE::AnimationCore::MaxRawBoneWeight - TotalWeight);
		}
	}

#if WITH_EDITOR
	FSkeletalMeshLODModel* SkeletalMeshLODModel = &(ImportedModel->LODModels[0]);
	SkeletalMeshLODModel->NumVertices = AllVertex;
	SkeletalMeshLODModel->NumTexCoords = UVNum; // AllVertex;
	SkeletalMeshLODModel->IndexBuffer = Triangles;
	SkeletalMeshLODModel->ActiveBoneIndices = SkeletalMeshLODRenderData.ActiveBoneIndices;
	SkeletalMeshLODModel->RequiredBones = SkeletalMeshLODRenderData.RequiredBones;
#else // game
	// force reinit render data
	FSkeletalMeshRenderData* SkeletalMeshRenderData = NewSkeletalMesh->GetResourceForRendering();
	auto* LODRenderData = &(SkeletalMeshRenderData->LODRenderData[0]);
	TArray<FSkinWeightInfo> InVertices;
	InVertices.SetNum(SoftSkinVertices.Num());

	int Elem = 0;
	if (SoftSkinVertices.Num())
	{
		Elem = std::extent_v<decltype(SoftSkinVertices[0].InfluenceBones)>;
	}
	FSkinWeightInfo Info = {};
	Elem = FMath::Min(Elem, static_cast<int>(std::extent_v<decltype(Info.InfluenceBones)>));

	for (int i = 0; i < SoftSkinVertices.Num(); ++i)
	{
		FSkinWeightInfo SkinWeightInfo = {};
		for (int j = 0; j < Elem; ++j)
		{
			// uint8 <> uint16
			SkinWeightInfo.InfluenceBones[j] = SoftSkinVertices[j].InfluenceBones[j];
			SkinWeightInfo.InfluenceWeights[j] = SoftSkinVertices[j].InfluenceWeights[j];
		}
		InVertices[i] = SkinWeightInfo;
	}

	const auto GameThreadReleaseResourceTask(MakeShared<TFunction<void()>, ESPMode::ThreadSafe>());
	*GameThreadReleaseResourceTask = [&]
	{
		LODRenderData->ReleaseResources();
	};
	UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadReleaseResourceTask);
	LODRenderData->SkinWeightVertexBuffer = InVertices;
#if	UE_OLDER_5_4
	const auto GameThreadInitResourceTask(MakeShared<TFunction<void()>, ESPMode::ThreadSafe>());
	*GameThreadInitResourceTask = [&]
	{
#if UE_OLDER_5_7
		LODRenderData->InitResources(false, 0, NewSkeletalMesh->GetMorphTargets(), NewSkeletalMesh);
#else
		LODRenderData->InitResources(false, 0, NewSkeletalMesh);
#endif // UE_OLDER_5_7
	};
	UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadInitResourceTask);
#else
	TArray<UMorphTarget*> MorphTargets;
	for (auto& MorphTarget : NewSkeletalMesh->GetMorphTargets())
	{
		MorphTargets.Add(MorphTarget);
	}
	const auto GameThreadInitResourceTask(MakeShared<TFunction<void()>, ESPMode::ThreadSafe>());
	*GameThreadInitResourceTask = [&]
	{
#if UE_OLDER_5_7
		LODRenderData->InitResources(false, 0, MorphTargets, NewSkeletalMesh);
#else
		LODRenderData->InitResources(false, 0, NewSkeletalMesh);
#endif // UE_OLDER_5_7
	};
	UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadInitResourceTask);
#endif // UE_OLDER_5_4
#endif // WITH_EDITOR
	Log(TEXT("BeforeRender"));
	ENQUEUE_RENDER_COMMAND(UpdateCommand)(
		[NewSkeletalMesh, Triangles, SoftSkinVertices](FRHICommandListImmediate& RHICmdList)
		{
			FSkeletalMeshLODRenderData& LODRenderData0 = NewSkeletalMesh->GetResourceForRendering()->LODRenderData[0];
			if (LODRenderData0.MultiSizeIndexContainer.IsIndexBufferValid())
			{
				LODRenderData0.MultiSizeIndexContainer.GetIndexBuffer()->ReleaseResource();
			}
			LODRenderData0.MultiSizeIndexContainer.RebuildIndexBuffer(sizeof(uint32), Triangles);
			LODRenderData0.MultiSizeIndexContainer.GetIndexBuffer()->InitResource(RHICmdList);
			// LODRenderData0.AdjacencyMultiSizeIndexContainer.CopyIndexBuffer(Triangles);
#if WITH_EDITOR
			LODRenderData0.SkinWeightVertexBuffer.Init(SoftSkinVertices);
#else
			TArray<FSkinWeightInfo> InWeights;
			InWeights.Reserve(SoftSkinVertices.Num());
			int InfluenceBones = 0;
			if (SoftSkinVertices.Num())
			{
				InfluenceBones = std::extent<decltype(SoftSkinVertices[0].InfluenceBones)>::value;
				// InfluenceBones = std::extent_v<decltype(SoftSkinVertices[0].InfluenceBones)>;
			}
			FSkinWeightInfo Info = {};
			(void)Info;
			InfluenceBones = FMath::Min(InfluenceBones, static_cast<int>(std::extent<decltype(Info.InfluenceBones)>::value));
			// InfluenceBones = FMath::Min(InfluenceBones, static_cast<int>(std::extent_v<decltype(Info.InfluenceBones)>));
			for (const auto SoftSkinVertex : SoftSkinVertices)
			{
				const auto SkinWeightInfo = new(InWeights) FSkinWeightInfo;
				FSkinWeightInfo WeightInfo = {};
				for (int i = 0; i < InfluenceBones; ++i)
				{
					// uint8 <> uint16
					WeightInfo.InfluenceBones[i] = SoftSkinVertex.InfluenceBones[i];
					WeightInfo.InfluenceWeights[i] = SoftSkinVertex.InfluenceWeights[i];
				}
				*SkinWeightInfo = WeightInfo;
			}
			LODRenderData0.SkinWeightVertexBuffer = InWeights;
			// FIXME: Provisionally updated VertexBuffer for SkinWeight. Prevented a crash.
			if (LODRenderData0.SkinWeightVertexBuffer.GetDataVertexBuffer() != nullptr)
			{
				LODRenderData0.SkinWeightVertexBuffer.GetDataVertexBuffer()->UpdateRHI(RHICmdList);
			}
#endif // WITH_EDITOR
			LODRenderData0.StaticVertexBuffers.PositionVertexBuffer.UpdateRHI(RHICmdList);
			LODRenderData0.StaticVertexBuffers.StaticMeshVertexBuffer.UpdateRHI(RHICmdList);
			LODRenderData0.MultiSizeIndexContainer.GetIndexBuffer()->UpdateRHI(RHICmdList);
		});
	Log(TEXT("AfterRender"));
	const FVector BoundMin(-100, -100, 0);
	const FVector BoundMax(100, 100, 200);
	const FBox BoundingBox(BoundMin, BoundMax);
	NewSkeletalMesh->SetImportedBounds(FBoxSphereBounds(BoundingBox));
#if WITH_EDITOR
	SkeletalMeshLODModel->NumTexCoords = UVNum;
	if (VRMConverter::Options::Get().IsActiveBone())
	{
		for (int i = 0; i < AllActiveBones.Num(); ++i)
		{
			if (const auto BoneIndex = NewRefSkeleton.GetParentIndex(AllActiveBones[i]); BoneIndex >= 0)
			{
				AllActiveBones.AddUnique(BoneIndex);
			}
		}
		AllActiveBones.Sort();

		SkeletalMeshLODModel->ActiveBoneIndices.SetNum(AllActiveBones.Num());
		// for (int i = 0; i < AllActiveBones.Num(); ++i)
		ParallelFor(AllActiveBones.Num(), [&](const int i)
		{
			SkeletalMeshLODModel->ActiveBoneIndices[i] = AllActiveBones[i];
		});
	}
	else
	{
		const int RawBoneNum = NewRefSkeleton.GetRawBoneNum();
		SkeletalMeshLODModel->ActiveBoneIndices.SetNum(RawBoneNum);
		for (int i = 0; i < RawBoneNum; ++i)
		{
			SkeletalMeshLODModel->ActiveBoneIndices[i] = i;
		}
	}
	// SkeletalMeshLODModel->NumVertices = AllVertex;
	// SkeletalMeshLODModel->NumTexCoords = UVNum; // allVertex;
	// SkeletalMeshLODModel->IndexBuffer = Triangles;
	// SkeletalMeshLODModel->RequiredBones = MeshLODRenderData.RequiredBones;
#endif // WITH_EDITOR
	Log(TEXT("ActiveBone"));

	if (AiScene->mVRMMeta && VRMConverter::Options::Get().IsSkipPhysics() == false)
	{
		if (VRM::VRMMetadata* Meta = static_cast<VRM::VRMMetadata*>(AiScene->mVRMMeta); Meta->springNum > 0)
		{
			UPhysicsAsset* PhysicsAsset = NewObject<UPhysicsAsset>(GetTransientPackage(), NAME_None, VRoid::TransientFlag);
			// PhysicsAsset->Modify();
#if WITH_EDITORONLY_DATA
			PhysicsAsset->SetPreviewMesh(NewSkeletalMesh);
#endif // WITH_EDITORONLY_DATA
			NewSkeletalMesh->SetPhysicsAsset(PhysicsAsset);

			VRoid::AddedList.Empty();
			TArray<int> SwingBoneIndexArray;
			for (int i = 0; i < Meta->springNum; ++i)
			{
				const auto& Spring = Meta->springs[i];
				for (int j = 0; j < Spring.boneNum; ++j)
				{
					const auto& BoneName = Spring.bones_name[j];
					const FString SpringBoneName = UTF8_TO_TCHAR(BoneName.C_Str());
					if (ReferenceSkeleton.FindRawBoneIndex(*SpringBoneName) == INDEX_NONE)
					{
						continue;
					}
					const FName ParentName = *SpringBoneName;
					VRoid::CreateSwingHead(VrmAssetList->SkeletalMesh, Spring, ParentName, SwingBoneIndexArray);
				}
			} // all spring
			SwingBoneIndexArray.Sort();
			for (int i = 0; i < SwingBoneIndexArray.Num() - 1; ++i)
			{
				for (int j = i + 1; j < SwingBoneIndexArray.Num(); ++j)
				{
					PhysicsAsset->DisableCollision(SwingBoneIndexArray[i], SwingBoneIndexArray[j]);
				}
			}
			// collision
			for (int i = 0; i < Meta->colliderGroupNum; ++i)
			{
				const auto& VrmColliderGroup = Meta->colliderGroups[i];
				FString VrmColliderNodeName = UTF8_TO_TCHAR(VrmColliderGroup.node_name.C_Str());
				if (ReferenceSkeleton.FindRawBoneIndex(*VrmColliderNodeName) == INDEX_NONE)
				{
					continue;
				}
				VrmColliderNodeName = VrmColliderNodeName.ToLower();

				USkeletalBodySetup* SkeletalBodySetup = nullptr;
				for (const auto& BodySetup : PhysicsAsset->SkeletalBodySetups)
				{
					if (BodySetup->BoneName.IsEqual(*VrmColliderNodeName))
					{
						SkeletalBodySetup = BodySetup;
						break;
					}
				}

				if (SkeletalBodySetup == nullptr)
				{
					SkeletalBodySetup = NewObject<USkeletalBodySetup>(PhysicsAsset, NAME_None, RF_Transactional);
					SkeletalBodySetup->InvalidatePhysicsData();
					PhysicsAsset->SkeletalBodySetups.Add(SkeletalBodySetup);
				}
				FKAggregateGeom AggregateGeom;
				for (int j = 0; j < VrmColliderGroup.colliderNum; ++j)
				{
					FKSphereElem SphereElem;
					VRM::vec3 ColliderOffset = {
						VrmColliderGroup.colliders[j].offset[0] * VRoid::VrmToUnrealScaleOffset,
						VrmColliderGroup.colliders[j].offset[1] * VRoid::VrmToUnrealScaleOffset,
						VrmColliderGroup.colliders[j].offset[2] * VRoid::VrmToUnrealScaleOffset,
					};
					SphereElem.Center = FVector(-ColliderOffset[0], ColliderOffset[2], ColliderOffset[1]);
					SphereElem.Radius = VrmColliderGroup.colliders[j].radius * VRoid::VrmToUnrealScaleOffset;
					AggregateGeom.SphereElems.Add(SphereElem);
				}
				SkeletalBodySetup->Modify();
				SkeletalBodySetup->BoneName = UTF8_TO_TCHAR(VrmColliderGroup.node_name.C_Str());
				SkeletalBodySetup->AddCollisionFrom(AggregateGeom);
				SkeletalBodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
				// newly created bodies default to simulating
				SkeletalBodySetup->PhysicsType = PhysType_Kinematic; // fix!
				SkeletalBodySetup->CreatePhysicsMeshes();
			} // collision

			PhysicsAsset->UpdateBodySetupIndexMap();
			RefreshSkelMeshOnPhysicsAssetChange(NewSkeletalMesh);
#if WITH_EDITOR
			PhysicsAsset->RefreshPhysicsAssetChange();
#endif // WITH_EDITOR
			PhysicsAsset->UpdateBoundsBodiesArray();
		}
	}
	Log(TEXT("Physics"));

#if WITH_EDITOR
	// animation
	if (AiScene->mNumAnimations > 0)
	{
		UAnimSequence* const AnimSequence =  NewObject<UAnimSequence>(VrmAssetList->Package, *(TEXT("A_") + VrmAssetList->BaseFileName), RF_Public | RF_Standalone);
		AnimSequence->SetSkeleton(Skeleton);
		int FrameNum = 0;
		for (uint32_t AnimNo = 0; AnimNo < AiScene->mNumAnimations; AnimNo++)
		{
			const aiAnimation* AIAnimation = AiScene->mAnimations[AnimNo];
			for (uint32_t ChannelNo = 0; ChannelNo < AIAnimation->mNumChannels; ChannelNo++)
			{
				const aiNodeAnim* AINodeAnim = AIAnimation->mChannels[ChannelNo];
				FrameNum = FMath::Max(FrameNum, static_cast<int>(AINodeAnim->mNumPositionKeys));
				FrameNum = FMath::Max(FrameNum, static_cast<int>(AINodeAnim->mNumRotationKeys));
			}
		}
		auto& Controller = AnimSequence->GetController();
#define LOCTEXT_NAMESPACE "VRoidSdk"
		Controller.OpenBracket(LOCTEXT("VRoidSdk", "Importing BVH"), false);
#undef LOCTEXT_NAMESPACE
		Controller.ResetModel();

		double AnimDeltaTime = 0.f;
		for (uint32_t AnimNo = 0; AnimNo < AiScene->mNumAnimations; AnimNo++)
		{
			const aiAnimation* AIAnimation = AiScene->mAnimations[AnimNo];
			for (uint32_t ChannelNo = 0; ChannelNo < AIAnimation->mNumChannels; ChannelNo++)
			{
				const aiNodeAnim* AINodeAnim = AIAnimation->mChannels[ChannelNo];
				for (int i = 0; i < static_cast<int>(AINodeAnim->mNumRotationKeys) - 1; ++i)
				{
					const auto DiffAnimDeltaTime = (AINodeAnim->mRotationKeys[i + 1].mTime - AINodeAnim->mRotationKeys[i].mTime) / AIAnimation->mTicksPerSecond;
					if (FMath::IsNearlyZero(AnimDeltaTime) && FMath::IsNearlyZero(DiffAnimDeltaTime) == false)
					{
						AnimDeltaTime = DiffAnimDeltaTime;
					}
				}
			}
		}
		Controller.InitializeModel();
		AnimSequence->ResetAnimation();
		AnimSequence->SetPreviewMesh(NewSkeletalMesh);
		const FFrameRate FrameRate(AnimDeltaTime > 0.0f ? FFrameRate(1.0f / AnimDeltaTime, 1) : FCommonFrameRates::FPS_30());
		Controller.SetFrameRate(FrameRate);
		Controller.SetNumberOfFrames(FrameNum - 1);
		AnimSequence->RateScale = VRMConverter::Options::Get().GetAnimationPlayRateScale();

		float TotalTime = 0.f;
		int TotalFrameNum = 0;
		// TArray<AnimationTransformDebug::FAnimationTransformDebugData> TransformDebugData;
		for (uint32_t AnimNo = 0; AnimNo < AiScene->mNumAnimations; AnimNo++)
		{
			const aiAnimation* AIAnimation(AiScene->mAnimations[AnimNo]);
			for (uint32_t ChannelNo = 0; ChannelNo < AIAnimation->mNumChannels; ChannelNo++)
			{
				const aiNodeAnim* AINodeAnim(AIAnimation->mChannels[ChannelNo]);
				FName NodeName = UTF8_TO_TCHAR(AINodeAnim->mNodeName.C_Str());
				if (VRMConverter::Options::Get().IsVRMAModel() && VrmAssetList->VrmMetaObject)
				{
					for (const auto& BoneTable : VrmAssetList->VrmMetaObject->humanoidBoneTable)
					{
						if (BoneTable.Value == NodeName.ToString())
						{
							NodeName = *BoneTable.Key;
							break;
						}
					}
				}
				if (auto BoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(NodeName); BoneIndex != INDEX_NONE)
				{
					BoneIndex = Skeleton->GetReferenceSkeleton().GetParentIndex(BoneIndex);
					// if (BoneIndex == INDEX_NONE)
					// {
					// 	// root bone. no parent.
					// }
				}
				FRawAnimSequenceTrack RawTrack;
				for (uint32_t i = 0; i < AINodeAnim->mNumPositionKeys; ++i)
				{
					const auto& AnimNodePosition(AINodeAnim->mPositionKeys[i].mValue);
					// FVector pos(v.x, v.y, v.z);
					const float Scale = VRMConverter::Options::Get().IsVRMAModel() ? VRoid::VrmToUnrealScaleOffset : 1.0f;
					FVector TrackPosition(AnimNodePosition.x, AnimNodePosition.z, AnimNodePosition.y);
					// if (VRMConverter::Options::Get().IsVRM10Model())
					if (IsVrm10Model)
					{
						TrackPosition.Y *= -1.f;
					}
					else
					{
						TrackPosition.X *= -1.f;
					}
					TrackPosition *= Scale * VRMConverter::Options::Get().GetAnimationTranslateScale();
#if 0
					if (VRMConverter::Options::Get().IsPMXModel() || VRMConverter::Options::Get().IsBVHModel())
					{
						TrackPosition.X *= -1.f;
						TrackPosition.Y *= -1.f;
					}
					// if (VRMConverter::Options::Get().IsVRMAModel())
					// {
					// 	if (isRootBone)
					// 	{
					// 		TrackPosition.X *= -1.f;
					// 		TrackPosition.Y *= -1.f;
					// 	}
					// 	else
					// 	{
					// 		TrackPosition.Set(AnimNodePosition.x, AnimNodePosition.y, AnimNodePosition.z);
					// 		TrackPosition *= Scale * VRMConverter::Options::Get().GetAnimationTranslateScale();
					// 	}
					// }
#endif
					RawTrack.PosKeys.Add(FVector3f(TrackPosition));
					TotalTime = FMath::Max(static_cast<float>(AINodeAnim->mPositionKeys[i].mTime), TotalTime);
				}
				// if (ChannelNo == 0 && RawTrack.PosKeys.Num())
				// {
				// 	AnimSequence->bEnableRootMotion = true;
				// }

				for (uint32_t i = 0; i < AINodeAnim->mNumRotationKeys; ++i)
				{
					const auto& AnimNodeRotation(AINodeAnim->mRotationKeys[i].mValue);
					FQuat4f AnimNodeQuat(AnimNodeRotation.x, AnimNodeRotation.z, AnimNodeRotation.y, AnimNodeRotation.w);
#if 0
					if (VRMConverter::Options::Get().IsBVHModel())
					{
						const auto Quat = FQuat4f(FVector3f(1, 0, 0), -PI / 2.f);
						AnimNodeQuat = Quat * AnimNodeQuat * Quat.Inverse();
					}
#endif
					// if (VRMConverter::Options::Get().IsVRM10Model())
					if (IsVrm10Model)
					{
						AnimNodeQuat.Y *= -1.f;
					}
					else
					{
						AnimNodeQuat.X *= -1.f;
					}
					// FQuat q(AnimNodeRotation.x, AnimNodeRotation.y, AnimNodeRotation.z, AnimNodeRotation.w);
					// FVector a = q.GetRotationAxis();
					// q = FQuat(FVector(0, 0, 1), PI) * q;
					// FMatrix m = q.GetRotationAxis
					// q = FRotator(0, 90, 0).Quaternion() * q;
					RawTrack.RotKeys.Add(AnimNodeQuat);
					TotalTime = FMath::Max(static_cast<float>(AINodeAnim->mRotationKeys[i].mTime), TotalTime);
				}

				for (uint32_t i = 0; i < AINodeAnim->mNumScalingKeys; ++i)
				{
					const auto& AnimNodeScale = AINodeAnim->mScalingKeys[i].mValue;
					const FVector Scale(AnimNodeScale.x, AnimNodeScale.y, AnimNodeScale.z);
					RawTrack.ScaleKeys.Add(FVector3f(Scale));
					TotalTime = FMath::Max(static_cast<float>(AINodeAnim->mScalingKeys[i].mTime), TotalTime);
				}

				if (RawTrack.RotKeys.Num() || RawTrack.PosKeys.Num() || RawTrack.ScaleKeys.Num())
				{
					while (RawTrack.PosKeys.Num() < FrameNum)
					{
						RawTrack.PosKeys.Add(FVector3f::ZeroVector);
					}
					while (RawTrack.RotKeys.Num() < FrameNum)
					{
						RawTrack.RotKeys.Add(FQuat4f::Identity);
					}
					while (RawTrack.ScaleKeys.Num() < FrameNum)
					{
						RawTrack.ScaleKeys.Add(FVector3f::OneVector);
					}
					if (VRMConverter::Options::Get().IsVRMAModel())
					{
						for (auto& Preset : VrmAssetList->VrmMetaObject->VRMAnimationMeta.expressionPreset)
						{
							if (NodeName != *Preset.expressionNodeName)
							{
								continue;
							}
							FFloatCurve FloatCurve;
							FloatCurve.SetCurveTypeFlag(AACF_Editable, true);
							if (Skeleton->GetReferenceSkeleton().FindBoneIndex(*Preset.expressionNodeName) != INDEX_NONE)
							{
								for (int i = 0; i < RawTrack.PosKeys.Num(); ++i)
								{
									FloatCurve.UpdateOrAddKey(RawTrack.PosKeys[i].X / VRoid::VrmToUnrealScaleOffset, i * AnimDeltaTime);
								}
							}
							FAnimationCurveIdentifier AnimationCurveIdentifier(*Preset.expressionName, ERawCurveTrackTypes::RCT_Float);
							Controller.AddCurve(AnimationCurveIdentifier);
							Controller.SetCurveKeys(AnimationCurveIdentifier, FloatCurve.FloatCurve.GetConstRefOfKeys());
						}
					}
					if (Controller.AddBoneCurve(NodeName))
					{
						Controller.SetBoneTrackKeys(NodeName, RawTrack.PosKeys, RawTrack.RotKeys, RawTrack.ScaleKeys);
					}
				}
				TotalFrameNum = FMath::Max(TotalFrameNum, RawTrack.RotKeys.Num());
				TotalFrameNum = FMath::Max(TotalFrameNum, RawTrack.PosKeys.Num());
				TotalTime = TotalFrameNum / AIAnimation->mTicksPerSecond;
			}
		}
		Controller.NotifyPopulated();
		Controller.CloseBracket(true);
		AnimSequence->PostEditChange();
	}
	Log(TEXT("Animation"));

#if 0 // Skip PostProcess Animation Blueprint settings.
	FSoftObjectPath PostProcessAnimPath(TEXT("/VRM4U/Util/Actor/latest/ABP_PostProcessBase.ABP_PostProcessBase"));
	if (UObject* PostProcessAnimObject = PostProcessAnimPath.TryLoad();
		PostProcessAnimObject && PostProcessAnimObject->IsA<UAnimBlueprint>() && VrmAssetList->Package)
	{
		FString Name = FString(TEXT("ABP_Post_")) + VrmAssetList->BaseFileName;
		const auto PostProcessAnimBlueprint = StaticDuplicateObject(PostProcessAnimObject, VrmAssetList->Package, *Name, VRoid::TransientFlag, nullptr, EDuplicateMode::PIE);
		if (const auto AnimBlueprint = Cast<UAnimBlueprint>(PostProcessAnimBlueprint))
		{
			bool bCond = AnimBlueprint->MarkPackageDirty();
			(void)bCond;
			AnimBlueprint->TargetSkeleton = Skeleton;
			AnimBlueprint->SetPreviewMesh(NewSkeletalMesh);
			FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
			UBlueprintGeneratedClass* InPostProcessAnimBlueprint = Cast<UBlueprintGeneratedClass>(AnimBlueprint->GeneratedClass);
			NewSkeletalMesh->SetPostProcessAnimBlueprint(InPostProcessAnimBlueprint);
			// NewSkeletalMesh->PostEditChange();
		}
	}
#endif
#endif // WITH_EDITOR
	Log(TEXT("Finish"), true);

	return true;
}
