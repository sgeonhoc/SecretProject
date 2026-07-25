// Copyright © 2024 pixiv Inc. All rights reserved.

#include "VRoidConverter.h"

#include "Tasks/Task.h"
#include "Engine/SubsurfaceProfile.h"
#include "Materials/MaterialInstanceConstant.h"
#if WITH_EDITOR
#include "Rendering/SkeletalMeshModel.h"
#else
#include "Rendering/SkeletalMeshRenderData.h"
#endif // WITH_EDITOR

#include "assimp/texture.h"
#include "VrmConvert.h"
#include "VrmAssetListObject.h"
#include "LoaderBPFunctionLibrary.h"
#include "VrmMetaObject.h"

#include "VRoidImage.h"
#include "Game/VRoidFunctionLibrary.h"
#include "Core/VRoidDefinitions.h"
#include "Core/VRoidLogger.h"

namespace VRoid
{
	struct FVRoidRHIBulkData final : FResourceBulkDataInterface
	{
		FVRoidRHIBulkData(void* const InData, const int InSize)
			: Data(InData), Size(InSize)
		{}

		virtual const void* GetResourceBulkData() const override { return Data; }
		virtual uint32 GetResourceBulkDataSize() const override { return Size; }
		virtual void Discard() override {}
	private:
		void* Data;
		int32 Size;
	};

	template <class T>
	T* VRoid_NewObject(const FName& Name = NAME_None)
	{
		constexpr EObjectFlags Flags(RF_Public | RF_Transient);
		T* r = nullptr;
		const auto GameThreadNewObjectTask = MakeShared<TFunction<void()>, ESPMode::ThreadSafe>([&]
		{
			r = NewObject<T>(GetTransientPackage(), Name, Flags);
		});
		UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadNewObjectTask);
		return r;
	}

	void LocalTextureSet(UMaterialInstanceConstant* const DM, const FName& Name, UTexture2D* const Tex)
	{
		const FMaterialParameterInfo Info { Name, GlobalParameter, INDEX_NONE };
		FTextureParameterValue* v = new (DM->TextureParameterValues) FTextureParameterValue(Info);
		v->ParameterValue = Tex;
	}

	void LocalScalarParameterSet(UMaterialInstanceConstant* const DM, const FName& Name, const float Value)
	{
		FScalarParameterValue* v = nullptr;
		for (auto& a : DM->ScalarParameterValues)
		{
			if (a.ParameterInfo.Name == Name)
			{
				v = &a;
				break;
			}
		}
		if (v == nullptr)
		{
			v = new(DM->ScalarParameterValues) FScalarParameterValue();
		}
		v->ParameterInfo = { Name, GlobalParameter, INDEX_NONE };
		v->ParameterValue = Value;
	}

	void LocalVectorParameterSet(UMaterialInstanceConstant* const DM, const FName& Name, const FLinearColor& Color)
	{
		FVectorParameterValue* v = nullptr;
		for (auto& a : DM->VectorParameterValues)
		{
			if (a.ParameterInfo.Name == Name)
			{
				v = &a;
				break;
			}
		}
		if (v == nullptr)
		{
			v = new(DM->VectorParameterValues) FVectorParameterValue();
		}
		v->ParameterInfo = { Name, GlobalParameter, INDEX_NONE };
		v->ParameterValue = Color;
	}

	bool CreateAndAddMaterial(UMaterialInstanceConstant* const DM, const int MatIndex,
	                          const UVrmAssetListObject* VrmAssetList, const VRMConverter* VC,
	                          const TArray<int>& TextureTypeToIndex, const bool EnableMToon, const bool IsVrm10)
	{
		// default set function
		const auto SetLocalParamsForVector = [&DM]()
		{
			LocalVectorParameterSet(DM, TEXT("mtoon_Color"), FLinearColor::White);
			LocalVectorParameterSet(DM, TEXT("mtoon_ShadeColor"), FLinearColor::White);
			LocalVectorParameterSet(DM, TEXT("mtoon_OutlineColor"), FLinearColor::Black);
		};
		const auto SetLocalParamsForScale = [&DM]()
		{
			LocalScalarParameterSet(DM, TEXT("mtoon_BumpScale"), 1.f);
			LocalScalarParameterSet(DM, TEXT("mtoon_NormalScale"), 1.f);
			LocalScalarParameterSet(DM, TEXT("mtoon_ReceiveShadowRate"), 1.f);
			LocalScalarParameterSet(DM, TEXT("mtoon_OutlineLightingMix"), 1.f);
			LocalScalarParameterSet(DM, TEXT("mtoon_OutlineWidth"), 0.1f);
			LocalScalarParameterSet(DM, TEXT("mtoon_OutlineWidthMode"), 1.f);
		};

		// default for not vrm material
		if (DM && EnableMToon == false)
		{
			SetLocalParamsForVector();
			SetLocalParamsForScale();
		}

		// VRM10
		if (IsVrm10)
		{
			LocalScalarParameterSet(DM, TEXT("bVRM10Mode"), 1.f);
		}

		VRM::VRMMaterial VrmMat;
		if (int i = MatIndex; VC->GetMatParam(VrmMat, i) == false)
		{
			return false;
		}
		{
			struct FTT
			{
				FString Key;
				float* Value;
			};
			for (const FTT TableParam[] = {
					{TEXT("_Color"), VrmMat.vectorProperties._Color},
					{TEXT("_ShadeColor"), VrmMat.vectorProperties._ShadeColor},
					{TEXT("_MainTex"), VrmMat.vectorProperties._MainTex},
					{TEXT("_ShadeTexture"), VrmMat.vectorProperties._ShadeTexture},
					{TEXT("_BumpMap"), VrmMat.vectorProperties._BumpMap},
					{TEXT("_ReceiveShadowTexture"), VrmMat.vectorProperties._ReceiveShadowTexture},
					{TEXT("_ShadingGradeTexture"), VrmMat.vectorProperties._ShadingGradeTexture},
					{TEXT("_RimColor"), VrmMat.vectorProperties._RimColor},
					{TEXT("_RimTexture"), VrmMat.vectorProperties._RimTexture},
					{TEXT("_SphereAdd"), VrmMat.vectorProperties._SphereAdd},
					{TEXT("_EmissionColor"), VrmMat.vectorProperties._EmissionColor},
					{TEXT("_EmissionMap"), VrmMat.vectorProperties._EmissionMap},
					{TEXT("_OutlineWidthTexture"), VrmMat.vectorProperties._OutlineWidthTexture},
					{TEXT("_OutlineColor"), VrmMat.vectorProperties._OutlineColor},
					{TEXT("_UvAnimMaskTexture"), VrmMat.vectorProperties._UvAnimMaskTexture},
				}; auto& [key, value] : TableParam)
			{
				LocalVectorParameterSet(DM, *(TEXT("mtoon") + key), FLinearColor(value[0], value[1], value[2], value[3]));
			}
		}

		// default for not vrm material
		if (EnableMToon == false)
		{
			SetLocalParamsForVector();
		}
		{
			struct FTT
			{
				FString Key;
				float& Value;
			};
			for (const FTT TableParam[] = {
					{TEXT("_Cutoff"), VrmMat.floatProperties._Cutoff},
					{TEXT("_BumpScale"), VrmMat.floatProperties._BumpScale},
					{TEXT("_NormalScale"), VrmMat.floatProperties._BumpScale}, // VRM4U Custom
					{TEXT("_ReceiveShadowRate"), VrmMat.floatProperties._ReceiveShadowRate},
					{TEXT("_ShadeShift"), VrmMat.floatProperties._ShadeShift},
					{TEXT("_ShadeToony"), VrmMat.floatProperties._ShadeToony},
					{TEXT("_LightColorAttenuation"), VrmMat.floatProperties._LightColorAttenuation},
					{TEXT("_IndirectLightIntensity"), VrmMat.floatProperties._IndirectLightIntensity},
					{TEXT("_RimLightingMix"), VrmMat.floatProperties._RimLightingMix},
					{TEXT("_RimFresnelPower"), VrmMat.floatProperties._RimFresnelPower},
					{TEXT("_RimLift"), VrmMat.floatProperties._RimLift},
					{TEXT("_OutlineWidth"), VrmMat.floatProperties._OutlineWidth},
					{TEXT("_OutlineScaledMaxDistance"), VrmMat.floatProperties._OutlineScaledMaxDistance},
					{TEXT("_OutlineLightingMix"), VrmMat.floatProperties._OutlineLightingMix},
					{TEXT("_UvAnimScrollX"), VrmMat.floatProperties._UvAnimScrollX},
					{TEXT("_UvAnimScrollY"), VrmMat.floatProperties._UvAnimScrollY},
					{TEXT("_UvAnimRotation"), VrmMat.floatProperties._UvAnimRotation},
					{TEXT("_MToonVersion"), VrmMat.floatProperties._MToonVersion},
					{TEXT("_DebugMode"), VrmMat.floatProperties._DebugMode},
					{TEXT("_BlendMode"), VrmMat.floatProperties._BlendMode},
					{TEXT("_OutlineWidthMode"), VrmMat.floatProperties._OutlineWidthMode},
					{TEXT("_OutlineColorMode"), VrmMat.floatProperties._OutlineColorMode},
					{TEXT("_CullMode"), VrmMat.floatProperties._CullMode},
					{TEXT("_OutlineCullMode"), VrmMat.floatProperties._OutlineCullMode},
					{TEXT("_SrcBlend"), VrmMat.floatProperties._SrcBlend},
					{TEXT("_DstBlend"), VrmMat.floatProperties._DstBlend},
					{TEXT("_ZWrite"), VrmMat.floatProperties._ZWrite},
				};
				const auto& [Key, Value] : TableParam)
			{
				LocalScalarParameterSet(DM, *(TEXT("mtoon") + Key), Value);
			}
			// default for not vrm material
			if (EnableMToon == false)
			{
				SetLocalParamsForScale();
			}
			if (VrmMat.floatProperties._Cutoff != 0.f)
			{
				DM->BasePropertyOverrides.bOverride_OpacityMaskClipValue = true;
				DM->BasePropertyOverrides.OpacityMaskClipValue = VrmMat.floatProperties._Cutoff;
			}
		}
		{
			struct FTT
			{
				FString Key;
				int Value;
			};
			const FTT TableParam[] = {
				{TEXT("mtoon_tex_MainTex"), VrmMat.textureProperties._MainTex},
				{TEXT("mtoon_tex_ShadeTexture"), VrmMat.textureProperties._ShadeTexture},
				{TEXT("mtoon_tex_Shade"), VrmMat.textureProperties._ShadeTexture}, // vrm1
				{TEXT("mtoon_tex_BumpMap"), VrmMat.textureProperties._BumpMap},
				{TEXT("mtoon_tex_ReceiveShadowTexture"), VrmMat.textureProperties._ReceiveShadowTexture},
				{TEXT("mtoon_tex_ShadingGradeTexture"), VrmMat.textureProperties._ShadingGradeTexture},
				{TEXT("mtoon_tex_RimTexture"), VrmMat.textureProperties._RimTexture},
				{TEXT("mtoon_tex_RimMultiply"), VrmMat.textureProperties._RimTexture}, // vrm1
				{TEXT("mtoon_tex_SphereAdd"), VrmMat.textureProperties._SphereAdd},
				{TEXT("mtoon_tex_MatCap"), VrmMat.textureProperties._SphereAdd}, // vrm1
				{TEXT("mtoon_tex_EmissionMap"), VrmMat.textureProperties._EmissionMap},
				{TEXT("mtoon_tex_Emissive"), VrmMat.textureProperties._EmissionMap}, // vrm1
				{TEXT("mtoon_tex_OutlineWidthTexture"), VrmMat.textureProperties._OutlineWidthTexture},
				{TEXT("mtoon_tex_OutlineWidthMultiply"), VrmMat.textureProperties._OutlineWidthTexture}, // vrm1
				{TEXT("mtoon_tex_UvAnimMaskTexture"), VrmMat.textureProperties._UvAnimMaskTexture},
				{TEXT("mtoon_tex_UvAnimationMask"), VrmMat.textureProperties._UvAnimMaskTexture}, // vrm1
			};
			if (IsVrm10)
			{
				const FSoftObjectPath DummyTexturePath(TEXT("/VRM4U/MaterialUtil/T_DummyBlack.T_DummyBlack"));
				if (UObject* const u = DummyTexturePath.TryLoad())
				{
					if (const auto R2 = Cast<UTexture2D>(u))
					{
						LocalTextureSet(DM, "mtoon_tex_EmissionMap", R2);
					}
				}
			}

			// default texture
			if (const int n = TextureTypeToIndex[aiTextureType_DIFFUSE]; n >= 0)
			{
				LocalTextureSet(DM, TEXT("mtoon_tex_MainTex"), VrmAssetList->Textures[n]);
				LocalTextureSet(DM, TEXT("gltf_tex_diffuse"), VrmAssetList->Textures[n]);
				LocalTextureSet(DM, TEXT("mtoon_tex_Shade"), VrmAssetList->Textures[n]);
			}
			// mtoon texture
			const int32 TextureNum = VrmAssetList->Textures.Num();
			int Count = 0;
			for (const auto& [Key, Value] : TableParam)
			{
				++Count;
				if (Value < 0 || Value >= TextureNum)
				{
					continue;
				}
				LocalTextureSet(DM, *Key, VrmAssetList->Textures[Value]);
				if (Count == 1)
				{
					// main => shade tex
					LocalTextureSet(DM, *TableParam[1].Key, VrmAssetList->Textures[Value]);
					LocalTextureSet(DM, *TableParam[2].Key, VrmAssetList->Textures[Value]);
				}
			}

			// gltf default texture
			if (auto n = TextureTypeToIndex[aiTextureType_NORMALS];
				0 <= n && n < TextureNum)
			{
				LocalTextureSet(DM, TEXT("mtoon_tex_Normal"), VrmAssetList->Textures[n]);
			}
			if (auto n = TextureTypeToIndex[aiTextureType_EMISSIVE];
				0 <= n && n < TextureNum)
			{
				LocalTextureSet(DM, TEXT("mtoon_tex_Emissive"), VrmAssetList->Textures[n]);
			}
		}
		return true;
	}

	bool IsSameMaterial(const UMaterialInterface* MI1, const UMaterialInterface* MI2)
	{
		const auto M1 = Cast<UMaterialInstanceConstant>(MI1);
		const auto M2 = Cast<UMaterialInstanceConstant>(MI2);

		if (M1 == nullptr || M2 == nullptr)
		{
			return false;
		}
		const auto CheckParameter = [&M1, &M2]()
		{
			// check num.
			if (M1->TextureParameterValues.Num() != M2->TextureParameterValues.Num() ||
				M1->ScalarParameterValues.Num() != M2->ScalarParameterValues.Num() ||
				M1->VectorParameterValues.Num() != M2->VectorParameterValues.Num())
			{
				return false;
			}
			if (M1->BasePropertyOverrides != M2->BasePropertyOverrides)
			{
				return false;
			}
			// check tex.
			for (int i = 0; i < M1->TextureParameterValues.Num(); ++i)
			{
				if (M1->TextureParameterValues[i].ParameterValue != M2->TextureParameterValues[i].ParameterValue)
				{
					return false;
				}
				if (M1->TextureParameterValues[i].ParameterInfo.Name != M2->TextureParameterValues[i].ParameterInfo.Name)
				{
					return false;
				}
			}
			// check scalar.
			for (int i = 0; i < M1->ScalarParameterValues.Num(); ++i)
			{
				if (M1->ScalarParameterValues[i].ParameterValue != M2->ScalarParameterValues[i].ParameterValue)
				{
					return false;
				}
				if (M1->ScalarParameterValues[i].ParameterInfo.Name != M2->ScalarParameterValues[i].ParameterInfo.Name)
				{
					return false;
				}
			}
			// check vector.
			for (int i = 0; i < M1->VectorParameterValues.Num(); ++i)
			{
				if (M1->VectorParameterValues[i].ParameterValue != M2->VectorParameterValues[i].ParameterValue)
				{
					return false;
				}
				if (M1->VectorParameterValues[i].ParameterInfo.Name != M2->VectorParameterValues[i].ParameterInfo.Name)
				{
					return false;
				}
			}
			return true;
		};

		if (CheckParameter())
		{
#define VROID_TMP_COMPARE(a) if (M1->a != M2->a) return false;
			VROID_TMP_COMPARE(OpacityMaskClipValue);
			VROID_TMP_COMPARE(BlendMode);
			VROID_TMP_COMPARE(TwoSided);
			VROID_TMP_COMPARE(DitheredLODTransition);
			VROID_TMP_COMPARE(bCastDynamicShadowAsMasked);
			VROID_TMP_COMPARE(GetShadingModels());
#undef VROID_TMP_COMPARE
			return true;
		}
		return false;
	}

	UVrmImportMaterialSet* SelectMaterialSet(const EVRMImportMaterialType MaterialType, const UVrmAssetListObject* const VrmAssetList, bool& bMToon)
	{
		switch (MaterialType)
		{
		case EVRMImportMaterialType::VRMIMT_MToon:
			bMToon = true;
			return VrmAssetList->MtoonLitSet;
		case EVRMImportMaterialType::VRMIMT_MToonUnlit:
			bMToon = true;
			return VrmAssetList->MtoonUnlitSet;
		case EVRMImportMaterialType::VRMIMT_SSS:
			bMToon = true;
			return VrmAssetList->SSSSet;
		case EVRMImportMaterialType::VRMIMT_SSSProfile:
			bMToon = true;
			return VrmAssetList->SSSProfileSet;
		case EVRMImportMaterialType::VRMIMT_Unlit:
			bMToon = false;
			return VrmAssetList->UnlitSet;
		case EVRMImportMaterialType::VRMIMT_glTF:
			bMToon = false;
			return VrmAssetList->GLTFSet;
		case EVRMImportMaterialType::VRMIMT_UEFNUnlit:
			bMToon = false;
			return VrmAssetList->UEFNUnlitSet;
		case EVRMImportMaterialType::VRMIMT_UEFNLit:
			bMToon = false;
			return VrmAssetList->UEFNLitSet;
		case EVRMImportMaterialType::VRMIMT_UEFNSSSProfile:
			bMToon = false;
			return VrmAssetList->UEFNSSSProfileSet;
		case EVRMImportMaterialType::VRMIMT_Custom:
			bMToon = false;
			return VrmAssetList->CustomSet;
		case EVRMImportMaterialType::VRMIMT_Auto:
		default:
			return nullptr;
		}
	}

	void LocalPopulateDeltas(const USkeletalMesh* Sk, UMorphTarget* Morph, const TArray<FMorphTargetDelta>& Deltas, const int32 LODIndex,
							 const bool bCompareNormal = false,
							 const bool bGeneratedByReductionSetting = false,
							 const float PositionThreshold = THRESH_POINTS_ARE_NEAR)
	{
#if WITH_EDITOR
		const TArray<FSkelMeshSection>& Sections(Sk->GetImportedModel()->LODModels[0].Sections);
		Morph->PopulateDeltas(Deltas, LODIndex, Sections);
#else
		const auto& Sections = Sk->GetResourceForRendering()->LODRenderData[0].RenderSections;
		auto& MorphLODModels = Morph->GetMorphLODModels();

		// create the LOD entry if it doesn't already exist
		if (LODIndex >= MorphLODModels.Num())
		{
			MorphLODModels.AddDefaulted(LODIndex - MorphLODModels.Num() + 1);
		}
		// morph mesh data to modify
		FMorphTargetLODModel& MorphModel = MorphLODModels[LODIndex];
		// copy the wedge point indices
		// for now just keep every thing 

		// set the original number of vertices
		MorphModel.NumBaseMeshVerts = Deltas.Num();
		// empty morph mesh vertices first
		MorphModel.Vertices.Empty(Deltas.Num());
		// mark if generated by reduction setting, so that we can remove them later if we want to
		// we don't want to delete if it has been imported
		MorphModel.bGeneratedByEngine = bGeneratedByReductionSetting;

		// Still keep this (could remove in long term due to incoming data)
		for (const FMorphTargetDelta& Delta : Deltas)
		{
			if (Delta.PositionDelta.SizeSquared() > FMath::Square(PositionThreshold) ||
				(bCompareNormal && Delta.TangentZDelta.SizeSquared() > 0.01f))
			{
				MorphModel.Vertices.Add(Delta);
				for (int32 SectionIdx = 0; SectionIdx < Sections.Num(); ++SectionIdx)
				{
					if (MorphModel.SectionIndices.Contains(SectionIdx))
					{
						continue;
					}
					const uint32 BaseVertexBufferIndex = static_cast<uint32>(Sections[SectionIdx].GetVertexBufferIndex());
					const uint32 LastVertexBufferIndex = BaseVertexBufferIndex + Sections[SectionIdx].GetNumVertices();
					if (BaseVertexBufferIndex <= Delta.SourceIdx && Delta.SourceIdx < LastVertexBufferIndex)
					{
						MorphModel.SectionIndices.AddUnique(SectionIdx);
						break;
					}
				}
			}
		}

		// sort the array of vertices for this morph target based on the base mesh indices
		// that each vertex is associated with. This allows us to sequentially traverse the list
		// when applying the morph blends to each vertex.
		struct FCompareMorphTargetDeltas
		{
			FORCEINLINE bool operator()(const FMorphTargetDelta& A, const FMorphTargetDelta& B) const
			{
				return (static_cast<int32>(A.SourceIdx) - static_cast<int32>(B.SourceIdx) < 0);
			}
		};
		MorphModel.Vertices.Sort(FCompareMorphTargetDeltas());

		// remove array slack
		MorphModel.Vertices.Shrink();
		MorphModel.NumVertices = MorphModel.Vertices.Num();
#endif // WITH_EDITOR
	}

	bool ReadMorph(TArray<FMorphTargetDelta>& MorphDeltas, const aiString& TargetName, const aiScene* AIData, const UVrmAssetListObject* AssetList, const bool IsVrm10)
	{
		MorphDeltas.Reset(0);
		uint32_t CurrentVertex = 0;
		const bool EnableNormal = VRMConverter::Options::Get().IsEnableMorphTargetNormal();
		const float ModelScale = VRMConverter::Options::Get().GetModelScale();

		for (uint32_t m = 0; m < AIData->mNumMeshes; ++m)
		{
			const auto& MeshInfo(AssetList->MeshReturnedData->meshInfo[m]);
			const auto& VertexUseFlags(MeshInfo.vertexUseFlag);
			const bool HasVertexUseFlag(VertexUseFlags.Num() > 0);

			const aiMesh& AIMesh(*AIData->mMeshes[m]);
			for (uint32_t a = 0; a < AIMesh.mNumAnimMeshes; ++a)
			{
				const aiAnimMesh& AIAnimMesh(*AIMesh.mAnimMeshes[a]);
				if (TargetName != AIAnimMesh.mName)
				{
					continue;
				}
				TArray<FMorphTargetDelta> TmpData;
				TmpData.Reserve(AIAnimMesh.mNumVertices);

				int VertexCount = 0;
				for (uint32_t i = 0; i < AIAnimMesh.mNumVertices; ++i)
				{
					if (HasVertexUseFlag && VertexUseFlags[i] == false)
					{
						continue;
					}

					FMorphTargetDelta v = {FVector3f::ZeroVector, FVector3f::ZeroVector, VertexCount + CurrentVertex};
					++VertexCount;

					if (AIAnimMesh.mVertices)
					{
						auto Aiv = AIAnimMesh.mVertices[i] - AIMesh.mVertices[i];
						if (IsVrm10)
						{
							v.PositionDelta.Set(Aiv[0] * 100.f, -Aiv[2] * 100.f, Aiv[1] * 100.f);
						}
						else
						{
							v.PositionDelta.Set(-Aiv[0] * 100.f, Aiv[2] * 100.f, Aiv[1] * 100.f);
						}
						// apply original root bone rotation
						if (const auto Property = AssetList->GetClass()->FindPropertyByName(TEXT("model_root_transform")))
						{
							FStructProperty* StructProperty = CastField<FStructProperty>(Property);
							if (StructProperty && StructProperty->Struct == TBaseStructure<FTransform>::Get())
							{
								if (const void* StructMemory = StructProperty->ContainerPtrToValuePtr<void>(AssetList))
								{
									if (const FTransform* TransformValue = static_cast<const FTransform*>(StructMemory))
									{
										FVector Tmp;
										Tmp.Set(v.PositionDelta.X, v.PositionDelta.Y, v.PositionDelta.Z);
										Tmp = TransformValue->TransformVector(Tmp);
										v.PositionDelta.Set(Tmp.X, Tmp.Y, Tmp.Z);
									}
								}
							}
						}
					}
					v.PositionDelta *= ModelScale;
					if (EnableNormal)
					{
						auto Aiv = AIAnimMesh.mNormals[i] - AIMesh.mNormals[i];
						FVector3f n(-Aiv[0], Aiv[2], Aiv[1]);
						if (IsVrm10)
						{
							n.Set(Aiv[0], -Aiv[2], Aiv[1]);
						}
						if (n.Size() > 1.f)
						{
							v.TangentZDelta = n.GetUnsafeNormal();
						}
						if (IsVrm10)
						{
							v.TangentZDelta.X *= -1.f;
							v.TangentZDelta.Y *= -1.f;
						}
					}

					// skip invalid vertex data
					if (v.PositionDelta.Size() == 0)
					{
						if (EnableNormal == false)
						{
							continue;
						}
						if (v.TangentZDelta.Size() == 0)
						{
							continue;
						}
					}

					TmpData.Add(v);
				} // vertex loop


				MorphDeltas.Append(TmpData);
			}
			if (HasVertexUseFlag)
			{
				CurrentVertex += MeshInfo.useVertexCount;
			}
			else
			{
				CurrentVertex += AIMesh.mNumVertices;
			}
		}
		return MorphDeltas.Num() != 0;
	}
} // namespace VRoid

VRoidConverter::VRoidConverter(const aiScene* const Scene, const bool IsVrm10)
	: AiScene(Scene),
	  IsVrm10Model(IsVrm10)
{
}

FTexturePlatformData* VRoidConverter::CreatePlatformData(const int SizeX, const int SizeY, const uint8* Data, const int Size, const EPixelFormat Format)
{
	const auto PlatformData(new FTexturePlatformData());
	PlatformData->SizeX = SizeX;
	PlatformData->SizeY = SizeY;
	PlatformData->PixelFormat = Format;

	const int32 NumBlocksX = SizeX / GPixelFormats[Format].BlockSizeX;
	const int32 NumBlocksY = SizeY / GPixelFormats[Format].BlockSizeY;
	FTexture2DMipMap* const MipMap0 = new FTexture2DMipMap(SizeX, SizeY);
	PlatformData->Mips.Add(MipMap0);
	MipMap0->BulkData.Lock(LOCK_READ_WRITE);
	MipMap0->BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[Format].BlockBytes);
	MipMap0->BulkData.Unlock();

	constexpr int32 BlockSize = 1024 * 1024;
	uint8* const MipData = static_cast<uint8*>(PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
	if (Size < BlockSize)
	{
		FMemory::Memcpy(MipData, Data, Size);
	}
	else
	{
		for (int32 Offset = 0; Offset < Size; Offset += BlockSize)
		{
			const int32 SizeToCopy = FMath::Min(BlockSize, Size - Offset);
			FMemory::Memcpy(MipData + Offset, Data + Offset, SizeToCopy);
		}
	}
	PlatformData->Mips[0].BulkData.Unlock();

	return PlatformData;
}

FTextureResource* VRoidConverter::CreateResourceForRenderThread(UTexture2D* const Texture, const FTextureRHIRef& RHITexture)
{
	FTextureResource* Resource = nullptr;
	const FGraphEventRef CreateResourceTask = FFunctionGraphTask::CreateAndDispatchWhenReady
	(
		[&Resource, Texture, RHITexture]
		{
			Resource = Texture->CreateResource();
			Resource->TextureRHI = RHITexture;
			Resource->bSRGB = (RHITexture->GetFlags() & TexCreate_SRGB) != TexCreate_None;
			Resource->bIgnoreGammaConversions = !Resource->bSRGB;
			Resource->bGreyScaleFormat = (RHITexture->GetFormat() == PF_G8 || RHITexture->GetFormat() == PF_BC4);
		},
		TStatId(), nullptr, ENamedThreads::GetRenderThread()
	);

	const FGraphEventRef InitRHIResourceTask = FFunctionGraphTask::CreateAndDispatchWhenReady
	(
		[&Resource, Texture, RHITexture]
		{
			Texture->TextureReference.TextureReferenceRHI = RHICreateTextureReference(RHITexture);
			Resource->SetTextureReference(Texture->TextureReference.TextureReferenceRHI);
			Resource->InitResource(FRHICommandListExecutor::GetImmediateCommandList());
		},
		TStatId(), CreateResourceTask, ENamedThreads::GetRenderThread()
	);

	InitRHIResourceTask->Wait();
	return Resource;
}

FTextureRHIRef VRoidConverter::CreateRHITexture2D(UTexture2D* const Texture)
{
	const FTexturePlatformData* PlatformData = Texture->GetPlatformData();
	if (PlatformData == nullptr || PlatformData->Mips.Num() <= 0)
	{
		return FTextureRHIRef(nullptr);
	}
	const FTexture2DMipMap& MipMap0 = PlatformData->Mips[0];
	const void* DataPtr = MipMap0.BulkData.LockReadOnly();
	const int32 DataSize = MipMap0.BulkData.GetBulkDataSize();
	TArray<uint8> TextureData;
	TextureData.SetNumUninitialized(DataSize);
	FMemory::Memcpy(TextureData.GetData(), DataPtr, DataSize);
	MipMap0.BulkData.Unlock();

	const ETextureCreateFlags TextureFlags = Texture->SRGB ? TexCreate_ShaderResource | TexCreate_SRGB : TexCreate_ShaderResource;
	FTextureRHIRef RHITexture2D = nullptr;
	if (GRHISupportsAsyncTextureCreation)
	{
		FGraphEventRef CompletionEvent;
		void* MipDataArray[1] = { TextureData.GetData() };
#if UE_NEWER_5_4
		const TCHAR* DebugName = TEXT("VRoidAsyncRHITexture");
		const ERHIAccess ResourceState = RHIGetDefaultResourceState(TextureFlags, TextureData.GetData() != nullptr);
		RHITexture2D = RHIAsyncCreateTexture2D(PlatformData->SizeX, PlatformData->SizeY, PlatformData->PixelFormat,
			1, TextureFlags, ResourceState, MipDataArray, 1, DebugName, CompletionEvent
		);
#else
		RHITexture2D = RHIAsyncCreateTexture2D(PlatformData->SizeX, PlatformData->SizeY, PlatformData->PixelFormat,
			1, TextureFlags, MipDataArray, 1, CompletionEvent
		);
#endif // UE_NEWER_5_4
		return RHITexture2D;
	}

#if UE_NEWER_5_4
	const FGraphEventRef CreateTextureTask = FFunctionGraphTask::CreateAndDispatchWhenReady(
		[&]
		{
			const FRHITextureCreateDesc TextureCreateDesc =
				FRHITextureCreateDesc::Create2D(TEXT("VRoidConverterRHITextureData"))
				.SetExtent(PlatformData->SizeX, PlatformData->SizeY)
				.SetFormat(PlatformData->PixelFormat)
				.SetNumMips(1)
				.SetNumSamples(1)
				.SetFlags(TextureFlags)
				.SetInitialState(RHIGetDefaultResourceState(TextureFlags, true));
			const FTextureRHIRef NewRHITexture = RHICreateTexture(TextureCreateDesc);

			uint32 DestStride = 0;
			void* Dest = RHILockTexture2D(NewRHITexture, 0, RLM_WriteOnly, DestStride, false);
			const uint32 BytesPerPixel = GPixelFormats[PlatformData->PixelFormat].BlockBytes;
			const uint32 SrcPitch = BytesPerPixel * PlatformData->SizeX;
			if (DestStride == SrcPitch)
			{
				FMemory::Memcpy(Dest, TextureData.GetData(), TextureData.Num());
			}
			else
			{
				const uint8* Src = TextureData.GetData();
				uint8* Dst = static_cast<uint8*>(Dest);
				for (int32 y = 0; y < PlatformData->SizeY; ++y)
				{
					FMemory::Memcpy(Dst, Src, SrcPitch);
					Src += SrcPitch;
					Dst += DestStride;
				}
			}
			RHIUnlockTexture2D(NewRHITexture, 0, false);

			RHITexture2D = NewRHITexture;
		}, TStatId(), nullptr, ENamedThreads::GetRenderThread()
	);
#else
	VRoid::FVRoidRHIBulkData RHIBulkData(TextureData.GetData(), DataSize);
	const FRHIResourceCreateInfo CreateInfo(TEXT("VRoidConverterRHITextureData"), &RHIBulkData);
	const FGraphEventRef CreateTextureTask = FFunctionGraphTask::CreateAndDispatchWhenReady(
		[&]
		{
			RHITexture2D = RHICreateTexture(
				FRHITextureCreateDesc::Create2D(CreateInfo.DebugName)
				.SetExtent(PlatformData->SizeX, PlatformData->SizeY)
				.SetFormat(PlatformData->PixelFormat)
				.SetNumMips(1)
				.SetNumSamples(1)
				.SetFlags(TextureFlags)
				.SetInitialState(ERHIAccess::Unknown)
				.SetExtData(CreateInfo.ExtData)
				.SetBulkData(CreateInfo.BulkData)
				.SetGPUMask(CreateInfo.GPUMask)
				.SetClearValue(CreateInfo.ClearValueBinding)
			);
		}, TStatId(), nullptr, ENamedThreads::GetRenderThread()
	);
#endif // UE_NEWER_5_4
	CreateTextureTask->Wait();
	return RHITexture2D;
}

void VRoidConverter::ConvertTexture(UVrmAssetListObject* const VrmAssetList) const
{
	if (AiScene->HasTextures() == false)
	{
		return;
	}
	const uint32 SceneTextureNum = AiScene->mNumTextures;
	const uint32 TextureNum = static_cast<uint32>(VrmAssetList->Textures.Num());
	if (TextureNum == SceneTextureNum)
	{
		return;
	}

	TArray<bool> NormalBoolTable;
	NormalBoolTable.SetNum(SceneTextureNum);
	if (const VRM::VRMMetadata* Meta = static_cast<const VRM::VRMMetadata*>(AiScene->mVRMMeta))
	{
		for (int i = 0; i < Meta->materialNum; ++i)
		{
			if (const int t = Meta->material[i].textureProperties._BumpMap; NormalBoolTable.IsValidIndex(t))
			{
				NormalBoolTable[t] = true;
			}
		}
	}

	TArray<UTexture2D*> TexArray;
	TArray<EVRMImportTextureCompressType> TextureCompressTypeArray;
	TexArray.Reserve(SceneTextureNum);
	TextureCompressTypeArray.Reserve(SceneTextureNum);
	for (uint32 i = 0; i < SceneTextureNum; ++i)
	{
		if (TextureNum && i < TextureNum)
		{
			if (VrmAssetList->Textures[i])
			{
				TexArray.Add(VrmAssetList->Textures[i]);
				continue;
			}
		}
		const auto& Texture = *AiScene->mTextures[i];
		TexArray.Add(CreateTextureWithRHI(Texture, NormalBoolTable[i], &TextureCompressTypeArray));
	}
	VrmAssetList->Textures = TexArray;
	VrmAssetList->Texture_CompressTypeList = TextureCompressTypeArray;
}

void VRoidConverter::ConvertMaterial(const VRMConverter* const Converter, UVrmAssetListObject* const VrmAssetList) const
{
	if (AiScene->HasMaterials() == false)
	{
		return;
	}
	TArray<bool> MatFlagOpaqueArray;
	TArray<bool> MatFlagTwoSidedArray;
	TArray<bool> MatFlagTranslucentArray;
	TArray<UMaterialInterface*> MatArray;
	TMap<FString, int> MatNameList;
	bool bMToon = false;

	const int SceneMaterialNum = static_cast<int>(AiScene->mNumMaterials);
	// last material = gltf default material
	// const int MatNum = FMath::Max(1, Options.IsVrmModel ? SceneMaterialNum - 1 : SceneMaterialNum);
	const int MatNum = FMath::Max(1, SceneMaterialNum - 1);
	MatFlagOpaqueArray.Reserve(MatNum);
	MatFlagTwoSidedArray.Reserve(MatNum);
	MatFlagTranslucentArray.Reserve(MatNum);
	MatArray.Reserve(MatNum);
	MatNameList.Reserve(MatNum);

	VrmAssetList->Materials.SetNum(MatNum);
	for (int32_t IMat = 0; IMat < MatNum; ++IMat)
	{
		const auto& AIMat = *AiScene->mMaterials[IMat];
		const FString& ShaderName(AIMat.mShaderName.C_Str());

		// native mtoon model ?
		const bool HasMToonShader(ShaderName.Find(TEXT("MToon")) >= 0);
		const bool EnableMToon(HasMToonShader || IsVrm10Model);
		auto MaterialType = VRMConverter::Options::Get().GetMaterialType();
		VrmAssetList->ImportMode = MaterialType;
		if (MaterialType == EVRMImportMaterialType::VRMIMT_Auto)
		{
			// default unlit
			MaterialType = (EnableMToon) ? EVRMImportMaterialType::VRMIMT_MToonUnlit : EVRMImportMaterialType::VRMIMT_Unlit;
		}
		// select material set
		const UVrmImportMaterialSet* ImportMaterialSet = VRoid::SelectMaterialSet(MaterialType, VrmAssetList, bMToon);
		if (ImportMaterialSet == nullptr)
		{
			continue;
		}

		// mtoon material.
		bool bTranslucent = false;
		bool bOpaque = false;
		bool bTwoSided = false;

		aiString AlphaMode;
		AIMat.Get(AI_MATKEY_GLTF_ALPHAMODE, AlphaMode);
		const FString& Alpha(AlphaMode.C_Str());
		if (Alpha == TEXT("BLEND"))
		{
			// check also _ZWrite
			bTranslucent = true;
		}
		if (Alpha == TEXT("OPAQUE"))
		{
			bOpaque = true;
			if (VrmAssetList->MaterialHasAlphaCutoff.IsValidIndex(IMat) && VrmAssetList->MaterialHasAlphaCutoff[IMat])
			{
				// force mask mode
				bOpaque = false;
			}
			if (bool HasAiMatKeyTwoSided = false; AIMat.Get(AI_MATKEY_TWOSIDED, HasAiMatKeyTwoSided) == AI_SUCCESS)
			{
				if (HasAiMatKeyTwoSided)
				{
					bTwoSided = true;
				}
			}
		}
		if (HasMToonShader)
		{
			if (const VRM::VRMMetadata* Meta = static_cast<const VRM::VRMMetadata*>(AiScene->mVRMMeta))
			{
				if (IMat < Meta->materialNum)
				{
					const auto& FloatProperty = Meta->material[IMat].floatProperties;
					if (FloatProperty._CullMode == 0.f || FloatProperty._CullMode == 1.f)
					{
						bTwoSided = true;
					}
					if (FloatProperty._ZWrite == 1.f)
					{
						bTranslucent = false;
					}
				}
			}
		}
		else
		{
			if (ShaderName.Find(TEXT("UnlitTransparent")) >= 0)
			{
				bTranslucent = true;
			}
		}
		if (VRMConverter::Options::Get().IsForceOpaque())
		{
			bTranslucent = false;
		}
		if (VRMConverter::Options::Get().IsForceTwoSided())
		{
			bTwoSided = true;
		}

		// material set
		UMaterialInterface* BaseM = nullptr;
		if (bMToon)
		{
			// opaque/translucent, TwoSided
			UMaterialInterface* TableParam[2][2] = {
				{
					ImportMaterialSet->Opaque,
					ImportMaterialSet->OpaqueTwoSided,
				},
				{
					ImportMaterialSet->Translucent,
					ImportMaterialSet->TranslucentTwoSided,
				},
			};

			const int c[2] = {
				bTranslucent ? 1 : 0,
				bTwoSided ? 1 : 0,
			};
			BaseM = TableParam[c[0]][c[1]];

			if (MatArray.Num() == MatFlagTranslucentArray.Num())
			{
				MatFlagTranslucentArray.Add(c[0] != 0);
				MatFlagTwoSidedArray.Add(c[1] != 0);
				MatFlagOpaqueArray.Add(bOpaque);
			}
		}
		else
		{
			// not mtoon
			BaseM = bTranslucent ? ImportMaterialSet->Translucent : ImportMaterialSet->Opaque;
		}
		if (BaseM == nullptr)
		{
			continue;
		}

		TArray<int> TextureTypeToIndex;
		TextureTypeToIndex.SetNum(AI_TEXTURE_TYPE_MAX);
		for (auto& a : TextureTypeToIndex)
		{
			a = INDEX_NONE;
		}

		TArray<aiString> TexNameArray;
		TexNameArray.SetNum(AI_TEXTURE_TYPE_MAX);
		for (uint32_t t = 0; t < AI_TEXTURE_TYPE_MAX; ++t)
		{
			const uint32_t TextureCount(AIMat.GetTextureCount(static_cast<aiTextureType>(t)));
			for (uint32_t y = 0; y < FMath::Min(static_cast<uint32_t>(1), TextureCount); ++y)
			{
				AIMat.GetTexture(static_cast<aiTextureType>(t), y, &TexNameArray[t]);
			}
		}

		for (uint32_t i = 0; i < AiScene->mNumTextures; ++i)
		{
			for (int32_t t = 0; t < TexNameArray.Num(); ++t)
			{
				if (AiScene->mTextures[i]->mFilename == TexNameArray[t])
				{
					TextureTypeToIndex[t] = i;
					break;
				}
			}
		}

		for (uint32_t t = 0; t < AI_TEXTURE_TYPE_MAX; ++t)
		{
			aiString Path;
			if (const aiReturn AIReturn(AIMat.GetTexture(static_cast<aiTextureType>(t), 0, &Path)); AIReturn == AI_SUCCESS)
			{
				std::string s(Path.C_Str());
				s = s.substr(s.find_last_of('*') + 1);
				TextureTypeToIndex[t] = atoi(s.c_str());
			}
		}

		const FString OrigName(FString(TEXT("M_")) + VRMConverter::NormalizeFileName(AIMat.GetName().C_Str()));
		FString Name(OrigName);
		if (MatNameList.Find(OrigName))
		{
			Name += FString::Printf(TEXT("_%03d"), MatNameList[Name]); // TEXT("_2");
		}
		MatNameList.FindOrAdd(OrigName)++;

		UMaterialInstanceConstant* DM = VRoid::VRoid_NewObject<UMaterialInstanceConstant>();
		DM->Parent = BaseM;
		{
			const FMaterialParameterInfo Info{TEXT("gltf_basecolor"), GlobalParameter, INDEX_NONE};
			FVectorParameterValue* v = new(DM->VectorParameterValues) FVectorParameterValue(Info);

			aiColor4D Col(1.f, 1.f, 1.f, 1.f);
			AIMat.Get(AI_MATKEY_BASE_COLOR, Col);
			v->ParameterValue = FLinearColor(Col.r, Col.g, Col.b, Col.a);
		}

		{
			float f[2] = {1, 1};
			aiReturn Result0 = AIMat.Get(AI_MATKEY_ROUGHNESS_FACTOR, f[0]);
			aiReturn Result1 = AIMat.Get(AI_MATKEY_METALLIC_FACTOR, f[1]);
			if (Result0 == AI_SUCCESS || Result1 == AI_SUCCESS)
			{
				f[0] = (Result0 == AI_SUCCESS) ? f[0] : 1;
				f[1] = (Result1 == AI_SUCCESS) ? f[1] : 1;
				if (f[0] == 0 && f[1] == 0)
				{
					f[0] = f[1] = 1.f;
				}
				const FMaterialParameterInfo Info{TEXT("gltf_RM"), GlobalParameter, INDEX_NONE};
				FVectorParameterValue* v = new(DM->VectorParameterValues) FVectorParameterValue(Info);
				v->ParameterValue = FLinearColor(f[0], f[1], 0, 0);
			}
		}
		if (int IndexDiffuse = TextureTypeToIndex[aiTextureType_DIFFUSE];
			IndexDiffuse >= 0 &&
			IndexDiffuse < VrmAssetList->Textures.Num())
		{
			VRoid::LocalTextureSet(DM, TEXT("gltf_tex_diffuse"), VrmAssetList->Textures[IndexDiffuse]);
			const FString& Str(TEXT("mtoon_tex_ShadeTexture"));
			bool bFindShaderTex = false;
			for (const auto& t : DM->TextureParameterValues)
			{
				if (Str.Compare(t.ParameterInfo.Name.ToString(), ESearchCase::IgnoreCase))
				{
					continue;
				}
				if (t.ParameterValue)
				{
					bFindShaderTex = true;
					if (IsValid(VrmAssetList->Textures[IndexDiffuse]))
					{
						continue;
					}
					if (auto const Tmp = Cast<UTexture2D>(t.ParameterValue.Get()))
					{
						VRoid::LocalTextureSet(DM, TEXT("gltf_tex_diffuse"), Tmp);
					}
				}
			}
			if (bFindShaderTex == false)
			{
				VRoid::LocalTextureSet(DM, *Str, VrmAssetList->Textures[IndexDiffuse]);
			}
		}
		else
		{
			if (VRMConverter::Options::Get().IsDefaultGridTextureMode() == false)
			{
				// set white texture for default
				if (UTexture2D* const Tex = LoadObject<UTexture2D>(nullptr, TEXT("/VRM4U/MaterialUtil/T_DummyWhite.T_DummyWhite")))
				{
					VRoid::LocalTextureSet(DM, TEXT("gltf_tex_diffuse"), Tex);
					VRoid::LocalTextureSet(DM, TEXT("mtoon_tex_ShadeTexture"), Tex);
				}
			}
		}
		if (bMToon == false)
		{
			if (Alpha == TEXT("BLEND") && VRMConverter::Options::Get().IsForceOpaque() == false)
			{
				DM->BasePropertyOverrides.bOverride_BlendMode = true;;
				DM->BasePropertyOverrides.BlendMode = BLEND_Translucent;
			}
		}

		// mtoon
		if (bMToon || IsVrm10Model)
		{
			if (VRoid::CreateAndAddMaterial(DM, IMat, VrmAssetList, Converter, TextureTypeToIndex, EnableMToon, IsVrm10Model) == false)
			{
				VROID_ERROR(TEXT("Failed create material (Material Index : %d)."), IMat);
				continue;
			}

			if (MatFlagOpaqueArray.IsValidIndex(IMat))
			{
				if (MatFlagOpaqueArray[IMat])
				{
					VRoid::LocalScalarParameterSet(DM, TEXT("bOpaque"), 1.f);
				}
			}
			if (VrmAssetList->MaterialHasMToon.IsValidIndex(IMat))
			{
				if (VrmAssetList->MaterialHasMToon[IMat] == false)
				{
					for (auto& a : DM->VectorParameterValues)
					{
						if (a.ParameterInfo.Name != TEXT("mtoon_Color"))
						{
							continue;
						}
						VRoid::LocalVectorParameterSet(DM, TEXT("mtoon_ShadeColor"), a.ParameterValue);
						break;
					}
				}
			}
		}
		else
		{
			// gltf texture
			TArray<FString> MaterialParamName;
			MaterialParamName.SetNum(AI_TEXTURE_TYPE_MAX);
			MaterialParamName[aiTextureType_DIFFUSE] = TEXT("gltf_tex_diffuse");
			MaterialParamName[aiTextureType_NORMALS] = TEXT("gltf_tex_normal");
			MaterialParamName[aiTextureType_EMISSIVE] = TEXT("gltf_tex_Emission");
			MaterialParamName[aiTextureType_BASE_COLOR] = TEXT("gltf_tex_diffuse");
			MaterialParamName[aiTextureType_EMISSION_COLOR] = TEXT("gltf_tex_Emission");
			MaterialParamName[aiTextureType_METALNESS] = TEXT("gltf_tex_metalness");
			MaterialParamName[aiTextureType_DIFFUSE_ROUGHNESS] = TEXT("gltf_tex_roughness");

			for (uint32_t t = 0; t < AI_TEXTURE_TYPE_MAX; ++t)
			{
				if (MaterialParamName[t] == "")
				{
					continue;
				}
				const int Index(TextureTypeToIndex[t]);
				if (Index < 0 || VrmAssetList->Textures.IsValidIndex(Index) == false ||
					IsValid(VrmAssetList->Textures[Index]) == false)
				{
					continue;
				}
				VRoid::LocalTextureSet(DM, *(MaterialParamName[t]), VrmAssetList->Textures[Index]);
			}
		}
		const auto GameThreadMaterialPostLoadTask = MakeShared<TFunction<void()>, ESPMode::ThreadSafe>([&DM]
		{
			DM->PostLoad();
		});
		UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadMaterialPostLoadTask);
		MatArray.Add(DM);

		if (MatArray.Num() != MatFlagTranslucentArray.Num())
		{
			MatFlagTranslucentArray.SetNumZeroed(MatArray.Num());
			MatFlagTwoSidedArray.SetNumZeroed(MatArray.Num());
			MatFlagOpaqueArray.SetNumZeroed(MatArray.Num());
		}
		if (VrmAssetList->MaterialNameOrigToAsset.Num())
		{
			TArray<FString> t;
			VrmAssetList->MaterialNameOrigToAsset.GenerateValueArray(t);
			VrmAssetList->MaterialNameAssetToMatNo.Add(t[t.Num() - 1], MatArray.Num() - 1);
		}
	}

	// enable bc7 mode.
	if (VRMConverter::Options::Get().IsBC7Mode())
	{
		VrmAssetList->Texture_CompressType = EVRMImportTextureCompressType::VRMITC_BC7;
	}

	// merge material.
	if (VRMConverter::Options::Get().IsMergeMaterial())
	{
		TArray<UMaterialInterface*> Tmp;
		TArray<bool> TmpOpaque;
		TArray<bool> TmpTwoSided;
		TArray<bool> TmpTranslucent;
		VrmAssetList->MaterialMergeTable.Reset();

		const int MatArrayNum(MatArray.Num());
		TmpOpaque.Reserve(MatArrayNum);
		TmpTwoSided.Reserve(MatArrayNum);
		TmpTranslucent.Reserve(MatArrayNum);
		VrmAssetList->MaterialMergeTable.Reserve(MatArrayNum);

		for (int i = 0; i < MatArrayNum; ++i)
		{
			VrmAssetList->MaterialMergeTable.Add(i, 0);

			bool bFind = false;
			for (int j = 0; j < Tmp.Num(); ++j)
			{
				if (VRoid::IsSameMaterial(MatArray[i], Tmp[j]) == false)
				{
					continue;
				}
				bFind = true;
				VrmAssetList->MaterialMergeTable[i] = j;
				MatArray[i]->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);
				break;
			}
			if (bFind == false)
			{
				VrmAssetList->MaterialMergeTable[i] = Tmp.Add(MatArray[i]);
				TmpOpaque.Add(MatFlagOpaqueArray[i]);
				TmpTwoSided.Add(MatFlagTwoSidedArray[i]);
				TmpTranslucent.Add(MatFlagTranslucentArray[i]);
			}
		}
		TmpOpaque.Shrink();
		TmpTwoSided.Shrink();
		TmpTranslucent.Shrink();

		VrmAssetList->Materials = Tmp;
		VrmAssetList->MaterialFlag_Opaque = TmpOpaque;
		VrmAssetList->MaterialFlag_TwoSided = TmpTwoSided;
		VrmAssetList->MaterialFlag_Translucent = TmpTranslucent;

		for (auto& a : VrmAssetList->MaterialNameAssetToMatNo)
		{
			a.Value = VrmAssetList->MaterialMergeTable[a.Value];
		}
	}
	else
	{
		VrmAssetList->Materials = MatArray;
		VrmAssetList->MaterialFlag_Opaque = MatFlagOpaqueArray;
		VrmAssetList->MaterialFlag_TwoSided = MatFlagTwoSidedArray;
		VrmAssetList->MaterialFlag_Translucent = MatFlagTranslucentArray;
	}

	// outline Materials.
	if (VRMConverter::Options::Get().IsGenerateOutlineMaterial() && VrmAssetList->OptMToonOutlineMaterial)
	{
		for (const auto MaterialInterface : VrmAssetList->Materials)
		{
			const auto MIC = Cast<UMaterialInstanceConstant>(MaterialInterface);
			const FString& s(MIC->GetName() / TEXT("_outline"));
			if (const auto OutlineMaterial = VRoid::VRoid_NewObject<UMaterialInstanceConstant>(*s))
			{
				OutlineMaterial->Parent = VrmAssetList->OptMToonOutlineMaterial;
				OutlineMaterial->VectorParameterValues = MIC->VectorParameterValues;
				OutlineMaterial->ScalarParameterValues = MIC->ScalarParameterValues;
				OutlineMaterial->TextureParameterValues = MIC->TextureParameterValues;
				const auto GameThreadOutlineMaterialPostLoadTask = MakeShared<TFunction<void()>, ESPMode::ThreadSafe>([&OutlineMaterial]
				{
					OutlineMaterial->PostLoad();
				});
				UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadOutlineMaterialPostLoadTask);
				VrmAssetList->OutlineMaterials.Add(OutlineMaterial);
			}
		}
	}

	// subsurface profile.
	USubsurfaceProfile* SP = VRoid::VRoid_NewObject<USubsurfaceProfile>(*(FString(TEXT("SP_")) + VrmAssetList->BaseFileName));

#if WITH_EDITORONLY_DATA
	VrmAssetList->SSSProfile = SP;
#endif
	for (auto& a : VrmAssetList->Materials)
	{
		a->SubsurfaceProfile = SP;
		if (auto* const c = Cast<UMaterialInstance>(a))
		{
			c->bOverrideSubsurfaceProfile = true;
		}
	}
}

UTexture2D* VRoidConverter::CreateTextureWithRHI(const aiTexture& Texture, const bool IsNormal, TArray<EVRMImportTextureCompressType>* TextureCompressTypeArray) const
{
	FVRoidImage Image;
	if (auto LoadImageTasks = UE::Tasks::Launch
	(
		TEXT("VRoidLoadImageTasks"), [&Texture, &Image]
		{
			const char* Buffer(reinterpret_cast<const char*>(Texture.pcData));
			if (FVRoidImage::LoadImageFromMemory(Buffer, Texture.mWidth, Image))
			{
				return true;
			}
			if (VRMUtil::FImportImage VrmImage; VRMLoaderUtil::LoadImageFromMemory(Buffer, Texture.mWidth, VrmImage))
			{
				Image.Init(VrmImage.SizeX, VrmImage.SizeY, VrmImage.Format, VrmImage.RawData.GetData());
				return true;
			}
			return false;
		}
	); LoadImageTasks.Wait() == false || LoadImageTasks.GetResult() == false)
	{
		VROID_ERROR(TEXT("Failed Load Image."));
		return nullptr;
	}
	return CreateTexture(Image, IsNormal, true, TextureCompressTypeArray);
}

UTexture2D* VRoidConverter::CreateTexture(FVRoidImage& Image, const bool IsNormal, const bool IsCreateRHITexture, TArray<EVRMImportTextureCompressType>* TextureCompressTypeArray) const
{
	const auto Size(FVector2d(Image.SizeX, Image.SizeY));
	const uint8* ImageData = Image.RawData.GetData();
	if (Size.X <= 0 || Size.Y <= 0 || ImageData == nullptr)
	{
		VROID_ERROR(TEXT("Failed Load Image."));
		return nullptr;
	}

	constexpr EPixelFormat Format(PF_B8G8R8A8);
	const int64 ImageSize(Image.RawData.Num());
	UTexture2D* NewTexture2D = VRoid::VRoid_NewObject<UTexture2D>();
	NewTexture2D->SetPlatformData(CreatePlatformData(Size.X, Size.Y, ImageData, ImageSize, Format));
	if (IsNormal)
	{
		NewTexture2D->CompressionSettings = TC_Normalmap;
		NewTexture2D->SRGB = 0;
	}
	else if (NewTexture2D->SRGB && VRMConverter::Options::Get().IsBC7Mode())
	{
		NewTexture2D->CompressionSettings = TC_BC7;
	}
#if WITH_EDITOR
	constexpr int MinimalTextureSize = 256;
	const int TextureWidth = FMath::Min(MinimalTextureSize, Size.X);
	const int TextureHeight = FMath::Min(MinimalTextureSize, Size.Y);
	NewTexture2D->Source.Init(TextureWidth, TextureHeight, 1, 1, TSF_BGRA8, ImageData);
	if (TextureCompressTypeArray)
	{
		const bool bIsBc7 = (NewTexture2D->CompressionSettings == TC_BC7);
		TextureCompressTypeArray->Add(bIsBc7 ? EVRMImportTextureCompressType::VRMITC_BC7 : EVRMImportTextureCompressType::VRMITC_DXT1);
	}
#if WITH_EDITORONLY_DATA
	auto MipGenSettings = TMGS_NoMipmaps;
	if (VRMConverter::Options::Get().IsMipmapGenerateMode())
	{
		if (FMath::IsPowerOfTwo(TextureWidth) && FMath::IsPowerOfTwo(TextureHeight))
		{
			MipGenSettings = TMGS_FromTextureGroup;
		}
	}
	NewTexture2D->MipGenSettings = MipGenSettings;
#endif // WITH_EDITORONLY_DATA
#endif // WITH_EDITOR
	
	NewTexture2D->LinkStreaming();
	if (IsCreateRHITexture)
	{
		CreateAndBindRHITextureToResource(NewTexture2D);
	}
	return NewTexture2D;
}

void VRoidConverter::CreateAndBindRHITextureToResource(UTexture2D* Texture2D) const
{
	const TFuture<FTextureRHIRef> CreateRHITextureTask = Async(EAsyncExecution::Thread, [&]
	{
		return CreateRHITexture2D(Texture2D);
	});
	const FTextureRHIRef RHITexture = CreateRHITextureTask.Get();
	if (RHITexture == nullptr)
	{
		VROID_ERROR(TEXT("Failed Create RHI Texture."));
		return;
	}
	if (const auto Resource = CreateResourceForRenderThread(Texture2D, RHITexture);
		Resource && Resource->IsInitialized())
	{
		Texture2D->SetResource(Resource);
	}
	else
	{
		VROID_ERROR(TEXT("Failed initialize texture resource."));
		if (Resource)
		{
			Resource->ReleaseResource();
		}
	}
}

bool VRoidConverter::ConvertTextureAndMaterial(const VRMConverter* const Converter, UVrmAssetListObject* const VrmAssetList) const
{
	if (VrmAssetList == nullptr || AiScene == nullptr)
	{
		return false;
	}
	if (VRMConverter::Options::Get().IsNoMesh())
	{
		return true;
	}
	VrmAssetList->Materials.Reset(0);
	VrmAssetList->OutlineMaterials.Reset(0);

	// convert textures and materials.
	const auto GameThreadConvertTextureAndMaterialTask = MakeShared<TFunction<void()>, ESPMode::ThreadSafe>([&]
	{
		ConvertTexture(VrmAssetList);
		ConvertMaterial(Converter, VrmAssetList);
	});
	UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadConvertTextureAndMaterialTask);
	return true;
}

bool VRoidConverter::ConvertMorphTarget(const UVrmAssetListObject* const VrmAssetList) const
{
	if (VrmAssetList->SkeletalMesh == nullptr)
	{
		VROID_ERROR(TEXT("VrmAssetList SkeletalMesh is NULL"));
		return false;
	}

	TArray<FString> MorphNameList;
	TArray<FString> MorphNameListStrict;
	TArray<UMorphTarget*> MorphTargetList;
	const int MaxLoopCount = AiScene->mNumMeshes + AiScene->mNumMaterials;
	MorphNameList.Reserve(MaxLoopCount);
	MorphNameListStrict.Reserve(MaxLoopCount);
	MorphTargetList.Reserve(MaxLoopCount);

	const auto SkeletalMesh(VrmAssetList->SkeletalMesh);
	for (uint32_t m = 0; m < AiScene->mNumMeshes; ++m)
	{
		const aiMesh& AIMesh = (*AiScene->mMeshes[m]);
		for (uint32_t a = 0; a < AIMesh.mNumAnimMeshes; ++a)
		{
			const aiAnimMesh& AIAnimMesh = *(AIMesh.mAnimMeshes[a]);
			TArray<FMorphTargetDelta> MorphDeltas;
			FString MorphName = UTF8_TO_TCHAR(AIAnimMesh.mName.C_Str());
			FString MorphNameOrg = MorphName;
			if (VRMConverter::Options::Get().IsForceOriginalMorphTargetName() == false)
			{
				MorphName = VRMUtil::MakeName(MorphName);
				if (MorphName != MorphNameOrg && MorphNameListStrict.Find(MorphNameOrg) == INDEX_NONE)
				{
					auto Tmp = MorphName;
					int i = 0;
					if (VRMUtil::IsNoSafeName(MorphName))
					{
						Tmp = TEXT("UE5EA_patch_") + MorphName + TEXT("_") + FString::FromInt(i);
					}
					while (MorphNameList.Find(Tmp) != INDEX_NONE)
					{
						++i;
						Tmp = TEXT("UE5EA_patch_") + MorphName + TEXT("_") + FString::FromInt(i);
					}
					MorphName = Tmp;
				}
			}
			if (MorphNameList.Find(MorphName) != INDEX_NONE)
			{
				continue;
			}

			MorphNameList.Add(MorphName);
			MorphNameListStrict.Add(MorphNameOrg);
			if (VRoid::ReadMorph(MorphDeltas, AIAnimMesh.mName, AiScene, VrmAssetList, IsVrm10Model) == false)
			{
				continue;
			}

			FString SSS = MorphName; // FString::Printf(TEXT("%02d_%02d_"), m, a) + FString();
			UMorphTarget* MT = NewObject<UMorphTarget>(SkeletalMesh, *SSS);
			VRoid::LocalPopulateDeltas(SkeletalMesh, MT, MorphDeltas, 0);

			if (MT->HasValidData())
			{
				FMorphTargetLODModel MorphLODModel;
				MorphLODModel.Reset();
				MorphLODModel.NumBaseMeshVerts = MorphDeltas.Num();
				MorphLODModel.SectionIndices.Add(0);
				MorphLODModel.Vertices = MorphDeltas;

				MT->GetMorphLODModels().Add(MorphLODModel);
				MT->BaseSkelMesh = SkeletalMesh;

				MorphTargetList.Add(MT);
			}
		}
	}

#if WITH_EDITOR
#if UE_OLDER_5_4
	// to avoid no morph target
	// on Immediate
	SkeletalMesh->SetUseLegacyMeshDerivedDataKey(true);
	FSkeletalMeshImportData RawMesh;
	SkeletalMesh->LoadLODImportedData(0, RawMesh);
	RawMesh.MorphTargetNames = MorphNameList;

	// to avoid no morph target
	// on EditorRestart
	SkeletalMesh->SaveLODImportedData(0, RawMesh);
	SkeletalMesh->SetLODImportedDataVersions(0, ESkeletalMeshGeoImportVersions::Before_Versionning, ESkeletalMeshSkinningImportVersions::Before_Versionning);
#else
#if 0
	// RawMeshBulkData is uninitialized, so nothing is done.
	FSkeletalMeshImportData RawMesh;
	if (const FMeshDescription* MeshDescription = SkeletalMesh->GetMeshDescription(0))
	{
		if (MeshDescription->IsEmpty() == false)
		{
			RawMesh = FSkeletalMeshImportData::CreateFromMeshDescription(*MeshDescription);
			RawMesh.MorphTargetNames = MorphNameList;
		}
	}
	if (FMeshDescription RawMeshDescription; RawMesh.GetMeshDescription(nullptr, &SkeletalMesh->GetLODInfo(0)->BuildSettings, RawMeshDescription))
	{
		SkeletalMesh->CreateMeshDescription(0, MoveTemp(RawMeshDescription));
		SkeletalMesh->CommitMeshDescription(0);
	}
#endif
#endif // UE_OLDER_5_4
#endif // WITH_EDITOR

	if (FSkeletalMeshLODInfo* LODInfoPtr = SkeletalMesh->GetLODInfo(0))
	{
		LODInfoPtr->BuildSettings.bRecomputeNormals = false;
		LODInfoPtr->BuildSettings.bRecomputeTangents = false;
		LODInfoPtr->BuildSettings.bRemoveDegenerates = false;
		LODInfoPtr->ReductionSettings.NumOfTrianglesPercentage = 1.f;
	}
	// remove all morph & morph curve
	SkeletalMesh->UnregisterAllMorphTarget();

	const auto Skeleton(SkeletalMesh->GetSkeleton());
#if WITH_EDITOR
	TArray<FName> NameList;
	Skeleton->GetCurveMetaDataNames(NameList);
	for (const auto Name : NameList)
	{
		const FCurveMetaData* m = Skeleton->GetCurveMetaData(Name);
		if (m == nullptr)
		{
			continue;
		}
		if (m->Type.bMorphtarget)
		{
			Skeleton->RemoveCurveMetaData(Name);
		}
	}
#endif // WITH_EDITOR

	for (int i = 0; i < MorphTargetList.Num(); ++i)
	{
		auto* MT = MorphTargetList[i];
		if (i == MorphTargetList.Num() - 1)
		{
			SkeletalMesh->RegisterMorphTarget(MT);
		}
		else
		{
			SkeletalMesh->GetMorphTargets().Add(MT);
		}
	}
	for (auto Name : MorphNameList)
	{
		if (FCurveMetaData* FoundCurveMetaData = Skeleton->GetCurveMetaData(*Name))
		{
			FoundCurveMetaData->Type.bMorphtarget = true;
			continue;
		}
		const auto GameThreadAccumulateCurveMetaDataTask = MakeShared<TFunction<void()>, ESPMode::ThreadSafe>([&Skeleton, &Name]
		{
			Skeleton->AccumulateCurveMetaData(*Name, false, true);
		});
		UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadAccumulateCurveMetaDataTask);
	}

#if WITH_EDITOR
	const auto GameThreadPostEditChangeTask = MakeShared<TFunction<void()>, ESPMode::ThreadSafe>([&SkeletalMesh]
	{
		// SkeletalMesh->Build();
		SkeletalMesh->PostEditChange();
	});
	UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadPostEditChangeTask);
#else
	if (const auto RenderingResource = SkeletalMesh->GetResourceForRendering(); RenderingResource)
	{
		if (RenderingResource->LODRenderData.Num() > 0)
		{
			auto& Lod0 = RenderingResource->LODRenderData[0];
			if (VRMConverter::IsImportMode() == false)
			{
				for (auto& RenderSection : Lod0.RenderSections)
				{
					RenderSection.DuplicatedVerticesBuffer.DupVertData.SetNum(1);
				}
			}
#if	UE_OLDER_5_4
			// Lod0.InitResources(false, 0, SkeletalMesh->GetMorphTargets(), SkeletalMesh);
			const auto GameThreadInitResourcesTask = MakeShared<TFunction<void()>, ESPMode::ThreadSafe>([&Lod0, &SkeletalMesh]
			{
#if UE_OLDER_5_7
				Lod0.InitResources(false, 0, SkeletalMesh->GetMorphTargets(), SkeletalMesh);
#else
				Lod0.InitResources(false, 0, SkeletalMesh);
#endif // UE_OLDER_5_7
			});
			UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadInitResourcesTask);
#else
			TArray<UMorphTarget*> MorphTargets;
			for (auto MorphTarget : SkeletalMesh->GetMorphTargets())
			{
				MorphTargets.Add(MorphTarget);
			}
			// Lod0.InitResources(false, 0, MorphTargets, SkeletalMesh);
			const auto GameThreadInitResourcesTask = MakeShared<TFunction<void()>, ESPMode::ThreadSafe>([&Lod0, &MorphTargets, &SkeletalMesh]
			{
#if UE_OLDER_5_7
				Lod0.InitResources(false, 0, MorphTargets, SkeletalMesh);
#else
				Lod0.InitResources(false, 0, SkeletalMesh);
#endif // UE_OLDER_5_7
			});
			UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadInitResourcesTask);
#endif // UE_OLDER_5_4
		}
	}
#endif // WITH_EDITOR
	return true;
}

void VRoidConverter::ConvertMannequin(UVrmAssetListObject* const VrmAssetList) const
{
	if (VRMConverter::Options::Get().IsGenerateHumanoidRenamedMesh() == false)
	{
		return;
	}
	USkeletalMesh* const OverrideMannequinSkeletalMesh = VrmAssetList->SkeletalMesh;
	const USkeleton* Skeleton = OverrideMannequinSkeletalMesh->GetSkeleton();
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();

	// vrm -> copy Skeleton to mannequin and overwrite Bone information.
	auto& BoneInfos = const_cast<TArray<FMeshBoneInfo>&>(RefSkeleton.GetRawRefBoneInfo());
	BoneInfos[0].Name = TEXT("root");
#if WITH_EDITORONLY_DATA
	BoneInfos[0].ExportName = TEXT("root");
#endif // WITH_EDITORONLY_DATA
	for (auto& BoneInfo : BoneInfos)
	{
		FString ToVrmBone;
		for (auto& Tmp : VrmAssetList->VrmMetaObject->humanoidBoneTable)
		{
			if (BoneInfo.Name.ToString().ToLower() != Tmp.Value.ToLower())
			{
				continue;
			}
			ToVrmBone = Tmp.Key;
			break;
		}
		if (ToVrmBone.IsEmpty())
		{
			continue;
		}
		FString ToUEBone;
		for (const auto& [BoneUE, BoneVRM] : VRMUtil::table_ue4_vrm)
		{
			if (ToVrmBone.ToLower() != BoneVRM.ToLower())
			{
				continue;
			}
			ToUEBone = BoneUE;
			break;
		}
		if (ToUEBone.IsEmpty())
		{
			continue;
		}

		// const FString NewName(ToUE4Bone);
		for (auto& MeshBoneInfo : BoneInfos)
		{
			if (BoneInfo == MeshBoneInfo)
			{
				continue;
			}
			if (MeshBoneInfo.Name == *(ToUEBone.ToLower()))
			{
				VROID_WARNING(TEXT("Rename Bone: %s"), *MeshBoneInfo.Name.ToString());
				MeshBoneInfo.Name = *(MeshBoneInfo.Name.ToString() + TEXT("_renamed_ue"));
			}
		}
		BoneInfo.Name = *ToUEBone;
#if WITH_EDITORONLY_DATA
		BoneInfo.ExportName = ToUEBone;
#endif // WITH_EDITORONLY_DATA
	}

	const_cast<FReferenceSkeleton&>(RefSkeleton).RebuildRefSkeleton(Skeleton, true);
	OverrideMannequinSkeletalMesh->SetRefSkeleton(RefSkeleton);

	VrmAssetList->UE4SkeletalMesh = OverrideMannequinSkeletalMesh;
	VrmAssetList->VrmMannequinMetaObject->SkeletalMesh = OverrideMannequinSkeletalMesh;
}
