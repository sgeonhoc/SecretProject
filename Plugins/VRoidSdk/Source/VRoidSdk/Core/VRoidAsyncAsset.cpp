// Copyright © 2025 pixiv Inc. All rights reserved.

#include "VRoidAsyncAsset.h"

#include "VrmConvert.h"
#include "VrmMetaObject.h"
#include "VrmAssetListObject.h"
#include "VrmLicenseObject.h"
#include "Vrm1LicenseObject.h"

#include "VRoidConverter.h"
#include "Core/VRoidDefinitions.h"
#include "Core/VRoidLogger.h"
#include "Game/VRoidFunctionLibrary.h"
#include "Core/VRoidSdkCoreFunctionLibrary.h"

TPair<bool, const RAPIDJSON_NAMESPACE::Value*> TryGetJsonValue(const RAPIDJSON_NAMESPACE::Value& Root, const std::initializer_list<const char*> Path) noexcept
{
	const RAPIDJSON_NAMESPACE::Value* Current = &Root;
	for (const char* Key : Path)
	{
		if (Current->IsObject() == false)
		{
			return {false, nullptr};
		}
		if (const auto GenericMember = Current->FindMember(Key);
			GenericMember != Current->MemberEnd())
		{
			Current = &GenericMember->value;
		}
		else
		{
			return {false, nullptr};
		}
	}
	return {true, Current};
}

void ConvertVRMAMeta(const VRMConverter* VrmConverter, UVrmMetaObject* MetaObject, UVrmAssetListObject*& Out)
{
	// vrma.
	if (VRMConverter::Options::Get().IsVRMAModel() == false)
	{
		return;
	}
	// Human Bones.
	const auto HumanBonePair = TryGetJsonValue(VrmConverter->jsonData.doc, {"extensions", "VRMC_vrm_animation", "humanoid", "humanBones"});
	if (HumanBonePair.Key && HumanBonePair.Value != nullptr)
	{
		const auto& HumanBone = *HumanBonePair.Value;
		const auto& OrigBone = VrmConverter->jsonData.doc["nodes"];
		for (auto& Bone : HumanBone.GetObject())
		{
			const int NodeNo = Bone.value["node"].GetInt();
			if (FString(Bone.name.GetString()) == TEXT(""))
			{
				continue;
			}
			if (NodeNo >= 0 && NodeNo < static_cast<int>(OrigBone.Size()))
			{
				MetaObject->humanoidBoneTable.Add(UTF8_TO_TCHAR(Bone.name.GetString())) = UTF8_TO_TCHAR(OrigBone[NodeNo]["name"].GetString());
			}
			else
			{
				MetaObject->humanoidBoneTable.Add(UTF8_TO_TCHAR(Bone.name.GetString())) = TEXT("");
			}
		}
	}
	// Expression Preset.
	const auto PresetPair = TryGetJsonValue(VrmConverter->jsonData.doc, {"extensions", "VRMC_vrm_animation", "expressions", "preset"});
	if (PresetPair.Key && PresetPair.Value != nullptr)
	{
		for (const auto& Preset = *PresetPair.Value;
			const auto& Member : Preset.GetObject())
		{
			FVRMAnimationExpressionPreset ExpressionPreset;
			ExpressionPreset.expressionName = Member.name.GetString();
			ExpressionPreset.expressionNode = Member.value["node"].GetInt();
			ExpressionPreset.expressionNodeName = VrmConverter->jsonData.doc["nodes"].GetArray()[ExpressionPreset.expressionNode]["name"].GetString();
			Out->VrmMetaObject->VRMAnimationMeta.expressionPreset.Add(ExpressionPreset);
		}
	}
	// LookAt.
	const auto LookAtPair = TryGetJsonValue(VrmConverter->jsonData.doc, {"extensions", "VRMC_vrm_animation", "lookAt", "node"});
	if (LookAtPair.Key && LookAtPair.Value != nullptr)
	{
		if (const auto& LookAt = *LookAtPair.Value;
			LookAt.IsInt())
		{
			Out->VrmMetaObject->VRMAnimationMeta.lookAt.lookAtNode = LookAt.GetInt();
		}
	}
}

VRoidAsyncAsset::VRoidAsyncAsset()
{
	constexpr int DefaultMaxTextureNums = 64;
	NormalBoolTable.Reserve(DefaultMaxTextureNums);
}

VRoidAsyncAsset::~VRoidAsyncAsset()
{
	NormalBoolTable.Empty();
	Images.Empty();
}

bool VRoidAsyncAsset::ConvertVrmFirst(UVrmAssetListObject*& Out) const
{
	const auto Materials = VrmConverter->jsonData.doc["materials"].GetArray();
	// material
	if (IsVrm10Model)
	{
		// mtoon params
		Out->MaterialHasMToon.Empty();
		for (auto& Material : Materials)
		{
			bool UseMToon = false;
			if (Material.HasMember("extensions"))
			{
				if (Material["extensions"].HasMember("VRMC_materials_mtoon"))
				{
					UseMToon = true;
				}
			}
			Out->MaterialHasMToon.Add(UseMToon);
		}
	}
	// alpha cutoff flag
	Out->MaterialHasAlphaCutoff.Empty();
	if (VrmConverter->jsonData.IsEnable())
	{
		for (auto& Material : Materials)
		{
			bool HasAlphaCutoff = false;
			if (Material.HasMember("alphaCutoff"))
			{
				HasAlphaCutoff = true;
			}
			Out->MaterialHasAlphaCutoff.Add(HasAlphaCutoff);
		}
	}
	return true;
}

bool VRoidAsyncAsset::ConvertVrm0Meta(const uint8* const Data, const size_t Size, UVrmAssetListObject*& Out) const
{
	constexpr EObjectFlags Flags(RF_Public | RF_Transient);
	UVrmMetaObject* MetaObject = NewObject<UVrmMetaObject>(GetTransientPackage(), NAME_None, Flags);
	UVrmLicenseObject* License0 = NewObject<UVrmLicenseObject>(GetTransientPackage(), NAME_None, Flags);
	const auto MarkPackageDirty = [](const UObject* Object)
	{
		if (Object != nullptr)
		{
			const bool IsDirty(Object->MarkPackageDirty());
			(void)IsDirty;
		}
	};
	MarkPackageDirty(MetaObject);
	MarkPackageDirty(License0);
	Out->VrmMetaObject = MetaObject;
	Out->VrmLicenseObject = License0;
	Out->Vrm1LicenseObject = nullptr;
	MetaObject->VrmAssetListObject = Out;
	MetaObject->Version = 0;

	const VRM::VRMMetadata* SceneMeta = static_cast<VRM::VRMMetadata*>(AiScene->mVRMMeta);
	if (SceneMeta == nullptr || Data == nullptr || Size == 0)
	{
		return false;
	}
	// bone.
	for (auto& [humanBoneName, nodeName] : SceneMeta->humanoidBone)
	{
		if (FString(humanBoneName.C_Str()) == TEXT(""))
		{
			continue;
		}
		FString NodeName = UTF8_TO_TCHAR(nodeName.C_Str());
		if (VRMConverter::Options::Get().IsForceOriginalBoneName() == false)
		{
			NodeName = VRMUtil::MakeName(NodeName, true);
		}
		MetaObject->humanoidBoneTable.Add(UTF8_TO_TCHAR(humanBoneName.C_Str())) = NodeName;
	}
	// shape.
	MetaObject->BlendShapeGroup.SetNum(SceneMeta->blendShapeGroupNum);
	for (int i = 0; i < SceneMeta->blendShapeGroupNum; ++i)
	{
		const auto& [groupName, bindNum, bind] = SceneMeta->blendShapeGroup[i];
		FString Name = UTF8_TO_TCHAR(groupName.C_Str());
		if (VRMConverter::Options::Get().IsRemoveBlendShapeGroupPrefix())
		{
			if (int32 FindIndex = 0;
				Name.FindLastChar('.', FindIndex))
			{
				if (FindIndex < Name.Len() - 1)
				{
					Name = Name.RightChop(FindIndex + 1);
				}
			}
		}
		MetaObject->BlendShapeGroup[i].name = Name;
		MetaObject->BlendShapeGroup[i].BlendShape.SetNum(bindNum);
		for (int b = 0; b < bindNum; ++b)
		{
			auto& BlendShape = MetaObject->BlendShapeGroup[i].BlendShape[b];
			BlendShape.morphTargetName = UTF8_TO_TCHAR(bind[b].blendShapeName.C_Str());
			BlendShape.meshName = UTF8_TO_TCHAR(bind[b].meshName.C_Str());
			BlendShape.nodeName = UTF8_TO_TCHAR(bind[b].nodeName.C_Str());
			BlendShape.weight = bind[b].weight;
			BlendShape.meshID = bind[b].meshID;
			BlendShape.shapeIndex = bind[b].shapeIndex;
			if (VRMConverter::Options::Get().IsForceOriginalMorphTargetName() == false)
			{
				BlendShape.morphTargetName = VRMUtil::MakeName(BlendShape.morphTargetName);
			}
		}
	}
	// tmp shape.
	TMap<FString, FString> ParamTable;
	ParamTable.Add("_Color", "mtoon_Color");
	ParamTable.Add("_RimColor", "mtoon_RimColor");
	ParamTable.Add("_EmisionColor", "mtoon_EmissionColor");
	ParamTable.Add("_OutlineColor", "mtoon_OutColor");
	const auto GroupPair = TryGetJsonValue(VrmConverter->jsonData.doc, {"extensions", "VRM", "blendShapeMaster", "blendShapeGroups"});
	if (GroupPair.Key && GroupPair.Value != nullptr)
	{
		const auto& Group = *(GroupPair.Value);
		for (int i = 0; i < static_cast<int>(Group.Size()); ++i)
		{
			if (MetaObject->BlendShapeGroup.IsValidIndex(i) == false)
			{
				break;
			}
			auto& Bind = MetaObject->BlendShapeGroup[i];
			const auto& Shape = Group[i];
			if (Shape.HasMember("materialValues") == false || Shape["materialValues"].IsArray() == false)
			{
				continue;
			}
			for (auto& Mat : Shape["materialValues"].GetArray())
			{
				if (Mat.IsObject() == false)
				{
					continue;
				}
				if (Mat.HasMember("materialName") == false || Mat.HasMember("propertyName") == false || Mat.HasMember("targetValue") == false)
				{
					continue;
				}
				FVrmBlendShapeMaterialList MaterialList;
				MaterialList.materialName = Mat["materialName"].GetString();
				MaterialList.propertyName = Mat["propertyName"].GetString();

				const FString* Tmp = Out->MaterialNameOrigToAsset.Find( VrmConverter->NormalizeFileName(MaterialList.materialName));
				if (Tmp == nullptr)
				{
					continue;
				}
				MaterialList.materialName = *Tmp;
				if (ParamTable.Find(MaterialList.propertyName))
				{
					MaterialList.propertyName = ParamTable[MaterialList.propertyName];
				}
				MaterialList.color = FLinearColor(
					Mat["targetValue"].GetArray()[0].GetFloat(),
					Mat["targetValue"].GetArray()[1].GetFloat(),
					Mat["targetValue"].GetArray()[2].GetFloat(),
					Mat["targetValue"].GetArray()[3].GetFloat());
				Bind.MaterialList.Add(MaterialList);
			}
		}
	}
	// spring.
	MetaObject->VRMSpringMeta.SetNum(SceneMeta->springNum);
	for (int SpringIndex = 0; SpringIndex < SceneMeta->springNum; ++SpringIndex)
	{
		const auto& VrmSpring = SceneMeta->springs[SpringIndex];
		auto& SpringMeta = MetaObject->VRMSpringMeta[SpringIndex];
		SpringMeta.stiffness = VrmSpring.stiffness;
		SpringMeta.gravityPower = VrmSpring.gravityPower;
		SpringMeta.gravityDir.Set(VrmSpring.gravityDir[0], VrmSpring.gravityDir[1], VrmSpring.gravityDir[2]);
		SpringMeta.dragForce = VrmSpring.dragForce;
		SpringMeta.hitRadius = VrmSpring.hitRadius;
		SpringMeta.bones.SetNum(VrmSpring.boneNum);
		SpringMeta.boneNames.SetNum(VrmSpring.boneNum);
		for (int BoneIndex = 0; BoneIndex < VrmSpring.boneNum; ++BoneIndex)
		{
			SpringMeta.bones[BoneIndex] = VrmSpring.bones[BoneIndex];
			SpringMeta.boneNames[BoneIndex] = UTF8_TO_TCHAR(VrmSpring.bones_name[BoneIndex].C_Str());
		}
		SpringMeta.ColliderIndexArray.SetNum(VrmSpring.colliderGourpNum);
		for (int ColliderIndex = 0; ColliderIndex < VrmSpring.colliderGourpNum; ++ColliderIndex)
		{
			SpringMeta.ColliderIndexArray[ColliderIndex] = VrmSpring.colliderGroups[ColliderIndex];
		}
	}
	// collider.
	MetaObject->VRMColliderMeta.SetNum(SceneMeta->colliderGroupNum);
	for (int ColliderGroupIndex = 0; ColliderGroupIndex < SceneMeta->colliderGroupNum; ++ColliderGroupIndex)
	{
		const auto& VrmColliderGroup = SceneMeta->colliderGroups[ColliderGroupIndex];
		auto& ColliderMeta = MetaObject->VRMColliderMeta[ColliderGroupIndex];
		ColliderMeta.bone = VrmColliderGroup.node;
		ColliderMeta.boneName = UTF8_TO_TCHAR(VrmColliderGroup.node_name.C_Str());
		ColliderMeta.collider.SetNum(VrmColliderGroup.colliderNum);
		for (int ColliderIndex = 0; ColliderIndex < VrmColliderGroup.colliderNum; ++ColliderIndex)
		{
			ColliderMeta.collider[ColliderIndex].offset = FVector(
				VrmColliderGroup.colliders[ColliderIndex].offset[0],
				VrmColliderGroup.colliders[ColliderIndex].offset[1],
				VrmColliderGroup.colliders[ColliderIndex].offset[2]);
			ColliderMeta.collider[ColliderIndex].radius = VrmColliderGroup.colliders[ColliderIndex].radius;
		}
	}
	// vrma.
	// VRoid::ConvertVRMAMeta(VrmConverter.Get(), MetaObject, Out);

	// license.
	struct FLicenseTable
	{
		FString Key;
		FString& Value;
	};
	const FLicenseTable LicenseTables[] = {
		{TEXT("version"), License0->version},
		{TEXT("author"), License0->author},
		{TEXT("contactInformation"), License0->contactInformation},
		{TEXT("reference"), License0->reference},
		// texture skip
		{TEXT("title"), License0->title},
		{TEXT("allowedUserName"), License0->allowedUserName},
		{TEXT("violentUsageName"), License0->violentUsageName},
		{TEXT("sexualUsageName"), License0->sexualUsageName},
		{TEXT("commercialUsageName"), License0->commercialUsageName},
		{TEXT("otherPermissionUrl"), License0->otherPermissionUrl},
		{TEXT("licenseName"), License0->licenseName},
		{TEXT("otherLicenseUrl"), License0->otherLicenseUrl},

		{TEXT("violentUssageName"), License0->violentUsageName},
		{TEXT("sexualUssageName"), License0->sexualUsageName},
		{TEXT("commercialUssageName"), License0->commercialUsageName},
	};
	for (int i = 0; i < SceneMeta->license.licensePairNum; ++i)
	{
		auto& LicensePair = SceneMeta->license.licensePair[i];
		for (const auto& [Key, Value] : LicenseTables)
		{
			if (Key == LicensePair.Key.C_Str())
			{
				Value = UTF8_TO_TCHAR(LicensePair.Value.C_Str());
			}
		}
		if (Out)
		{
			if (FString(TEXT("texture")) == LicensePair.Key.C_Str())
			{
				if (const int TextureIndex = FCString::Atoi(*FString(LicensePair.Value.C_Str()));
					TextureIndex >= 0 &&
					TextureIndex < Out->Textures.Num())
				{
					License0->thumbnail = Out->Textures[TextureIndex];
				}
			}
		}
	}
	return true;
}

bool VRoidAsyncAsset::ConvertVrm10Meta(const uint8* const Data, const size_t Size, UVrmAssetListObject*& Out) const
{
	constexpr EObjectFlags Flags(RF_Public | RF_Transient);
	UVrmMetaObject* MetaObject = NewObject<UVrmMetaObject>(GetTransientPackage(), NAME_None, Flags);
	UVrm1LicenseObject* License10 = NewObject<UVrm1LicenseObject>(GetTransientPackage(), NAME_None, Flags);
	const auto MarkPackageDirty = [](const UObject* Object)
	{
		if (Object != nullptr)
		{
			const bool IsDirty(Object->MarkPackageDirty());
			(void)IsDirty;
		}
	};
	MarkPackageDirty(MetaObject);
	MarkPackageDirty(License10);
	Out->VrmMetaObject = MetaObject;
	Out->VrmLicenseObject = nullptr;
	Out->Vrm1LicenseObject = License10;
	MetaObject->VrmAssetListObject = Out;
	MetaObject->Version = 1;

	if (Data == nullptr || Size == 0)
	{
		return false;
	}
	// bone.
	const auto HumanBonesPair = TryGetJsonValue(VrmConverter->jsonData.doc, {"extensions", "VRMC_vrm", "humanoid", "humanBones"});
	if (HumanBonesPair.Key && HumanBonesPair.Value != nullptr)
	{
		const auto& HumanBones = *HumanBonesPair.Value;
		const auto& OrigBones = VrmConverter->jsonData.doc["nodes"];
		for (const auto& Node : HumanBones.GetObject())
		{
			if (const int NodeValue = Node.value["node"].GetInt();
				NodeValue >= 0 &&
				NodeValue < static_cast<int>(OrigBones.Size()))
			{
				FString NodeName = UTF8_TO_TCHAR(OrigBones[NodeValue]["name"].GetString());
				if (VRMConverter::Options::Get().IsForceOriginalBoneName() == false)
				{
					NodeName = VRMUtil::MakeName(NodeName, true);
				}
				MetaObject->humanoidBoneTable.Add(UTF8_TO_TCHAR(Node.name.GetString())) = NodeName;
			}
			else
			{
				MetaObject->humanoidBoneTable.Add(UTF8_TO_TCHAR(Node.name.GetString())) = TEXT("");
			}
		}
	}
	// shape.
	const auto PresetPair = TryGetJsonValue(VrmConverter->jsonData.doc, {"extensions", "VRMC_vrm", "expressions", "preset"});
	if (PresetPair.Key && PresetPair.Value != nullptr)
	{
		const auto& Presets = *PresetPair.Value;
		MetaObject->BlendShapeGroup.SetNum(Presets.Size());
		int PresetIndex = INDEX_NONE;
		for (const auto& PresetData : Presets.GetObject())
		{
			++PresetIndex;

			MetaObject->BlendShapeGroup[PresetIndex].name = UTF8_TO_TCHAR(PresetData.name.GetString()); // ex happy
			auto& BindData = PresetData.value["morphTargetBinds"];
			MetaObject->BlendShapeGroup[PresetIndex].BlendShape.SetNum(BindData.Size());
			MetaObject->BlendShapeGroup[PresetIndex].isBinary = PresetData.value["isBinary"].GetBool();
			MetaObject->BlendShapeGroup[PresetIndex].overrideBlink = PresetData.value["overrideBlink"].GetString();
			MetaObject->BlendShapeGroup[PresetIndex].overrideLookAt = PresetData.value["overrideLookAt"].GetString();
			MetaObject->BlendShapeGroup[PresetIndex].overrideMouth = PresetData.value["overrideMouth"].GetString();

			int BindIndex = INDEX_NONE;
			for (const auto& Bind : BindData.GetArray())
			{
				++BindIndex;

				// MetaObject->BlendShapeGroup[PresetIndex].BlendShape[BindIndex].morphTargetName = UTF8_TO_TCHAR(aiGroup.bind[b].blendShapeName.C_Str());
				auto& TargetShape = MetaObject->BlendShapeGroup[PresetIndex].BlendShape[BindIndex];
				// TargetShape.morphTargetName = UTF8_TO_TCHAR(aiGroup.bind[b].blendShapeName.C_Str());
				// TargetShape.meshName = UTF8_TO_TCHAR(aiGroup.bind[b].meshName.C_Str());
				// TargetShape.nodeName = UTF8_TO_TCHAR(aiGroup.bind[b].nodeName.C_Str());
				// TargetShape.weight = aiGroup.bind[b].weight;
				TargetShape.shapeIndex = Bind["index"].GetInt();
				int TmpNodeId = Bind["node"].GetInt(); // adjust offset
#if 0
				int TmpMeshId = VrmConverter->jsonData.doc["nodes"].GetArray()[TmpNodeId]["mesh"].GetInt();
				// meshID offset
				int Offset = 0;
				// for (int MeshNo = 0; MeshNo < TmpMeshId; ++MeshNo)
				// {
				// 	if (VrmConverter->jsonData.doc["meshes"].GetArray()[MeshNo].HasMember("primitives") == false) continue;
				// 	//offset += jsonData.doc["meshes"].GetArray()[meshNo]["primitives"].Size() - 1;
				// }
				TargetShape.meshID = TmpMeshId + Offset;
#else
				if (auto& JsonNodes = VrmConverter->jsonData.doc["nodes"];
					TmpNodeId >= 0 && TmpNodeId < static_cast<int>(JsonNodes.Size()))
				{
					if (const auto JsonMeshPair = TryGetJsonValue(JsonNodes.GetArray()[TmpNodeId], {"mesh"});
						JsonMeshPair.Key && JsonMeshPair.Value != nullptr)
					{
#if 0
						const int TmpMeshId = JsonMeshPair.Value->GetInt();
						//meshID offset
						int Offset = 0;
						for (int MeshNo = 0; MeshNo < TmpMeshId; ++MeshNo) {
							if (VrmConverter->jsonData.doc["meshes"].GetArray()[MeshNo].HasMember("primitives") == false)
							{
								continue;
							}
							//offset += jsonData.doc["meshes"].GetArray()[meshNo]["primitives"].Size() - 1;
						}
						TargetShape.meshID = TmpMeshId + Offset;
#else
						TargetShape.meshID = JsonMeshPair.Value->GetInt();
#endif
					}
				}
#endif
				if (TargetShape.meshID < static_cast<int>(VrmConverter->jsonData.doc["meshes"].Size()))
				{
					if (auto& TargetNames = VrmConverter->jsonData.doc["meshes"].GetArray()[TargetShape.meshID]["extras"]["targetNames"];
						TargetShape.shapeIndex < static_cast<int>(TargetNames.Size()))
					{
						TargetShape.morphTargetName = UTF8_TO_TCHAR(
							TargetNames.GetArray()[TargetShape.shapeIndex].GetString());
					}
					if (auto& TargetNames = VrmConverter->jsonData.doc["meshes"][TargetShape.meshID]["primitives"]["extras"]["targetNames"];
						TargetShape.shapeIndex < static_cast<int>(TargetNames.Size()))
					{
						TargetShape.morphTargetName = TargetNames[TargetShape.shapeIndex].GetString();
						if (VRMConverter::Options::Get().IsForceOriginalMorphTargetName() == false)
						{
							TargetShape.morphTargetName = VRMUtil::MakeName(TargetShape.morphTargetName);
						}
					}
					TargetShape.meshName = UTF8_TO_TCHAR(VrmConverter->jsonData.doc["meshes"].GetArray()[TargetShape.meshID]["name"].GetString());
				}
			}
		}
	}
	// spring.
	const auto JsonSpringPair = TryGetJsonValue(VrmConverter->jsonData.doc, {"extensions", "VRMC_springBone", "springs"});
	if (JsonSpringPair.Key && JsonSpringPair.Value != nullptr)
	{
		const auto& JsonSpring = *JsonSpringPair.Value;
		auto& SpringMetas = MetaObject->VRM1SpringBoneMeta.Springs;
		SpringMetas.SetNum(JsonSpring.Size());
		for (uint32 SpringNo = 0; SpringNo < JsonSpring.Size(); ++SpringNo)
		{
			const auto& JsonJoints = JsonSpring.GetArray()[SpringNo]["joints"];
			auto& SpringMeta = SpringMetas[SpringNo];
			SpringMeta.joints.SetNum(JsonJoints.Size());
			for (uint32 JointNo = 0; JointNo < JsonJoints.Size(); ++JointNo)
			{
				const auto& JsonJoint = JsonJoints.GetArray()[JointNo];
				auto& JointMeta = SpringMeta.joints[JointNo];
				JointMeta.dragForce = JsonJoint["dragForce"].GetFloat();
				JointMeta.gravityPower = JsonJoint["gravityPower"].GetFloat();
				if (JsonJoint.HasMember("gravityDir"))
				{
					if (JsonJoint["gravityDir"].GetArray().Size() == 3)
					{
						JointMeta.gravityDir.X = JsonJoint["gravityDir"][0].GetFloat();
						JointMeta.gravityDir.Y = JsonJoint["gravityDir"][1].GetFloat();
						JointMeta.gravityDir.Z = JsonJoint["gravityDir"][2].GetFloat();
					}
				}
				const int NodeValue = JsonJoint["node"].GetInt();
				JointMeta.boneNo = INDEX_NONE; // node; // reset after bone optimize
				if (const auto& JsonNode = VrmConverter->jsonData.doc["nodes"];
					NodeValue >= 0 &&
					NodeValue < static_cast<int>(JsonNode.Size()))
				{
					JointMeta.boneName = VRMUtil::GetSafeNewName(UTF8_TO_TCHAR(JsonNode[NodeValue]["name"].GetString()));
				}
				JointMeta.hitRadius = JsonJoint["hitRadius"].GetFloat();
				JointMeta.stiffness = JsonJoint["stiffness"].GetFloat();
			}
			const auto& JsonColliderGroups = JsonSpring.GetArray()[SpringNo]["colliderGroups"];
			SpringMeta.colliderGroups.SetNum(JsonColliderGroups.Size());
			for (uint32 ColliderIndex = 0; ColliderIndex < JsonColliderGroups.Size(); ++ColliderIndex)
			{
				SpringMeta.colliderGroups[ColliderIndex] = JsonColliderGroups[ColliderIndex].GetInt();
			}
		}
	}
	// Collider.
	const auto JsonCollidersPair = TryGetJsonValue(VrmConverter->jsonData.doc, {"extensions", "VRMC_springBone", "colliders"});
	if (JsonCollidersPair.Key && JsonCollidersPair.Value != nullptr)
	{
		const auto& JsonColliders = *JsonCollidersPair.Value;
		auto& ColliderMetas = MetaObject->VRM1SpringBoneMeta.Colliders;
		ColliderMetas.SetNum(JsonColliders.Size());
		for (int ColliderIndex = 0; ColliderIndex < static_cast<int>(JsonColliders.Size()); ++ColliderIndex)
		{
			auto& JsonCollider = JsonColliders[ColliderIndex];
			auto& ColliderMeta = ColliderMetas[ColliderIndex];
			const int NodeIndex = JsonCollider["node"].GetInt();
			if (const auto& JsonNode = VrmConverter->jsonData.doc["nodes"];
				NodeIndex >= 0 &&
				NodeIndex < static_cast<int>(JsonNode.Size()))
			{
				ColliderMeta.boneName = VRMUtil::GetSafeNewName(UTF8_TO_TCHAR(JsonNode[NodeIndex]["name"].GetString()));
			}
			if (JsonCollider["shape"].HasMember("sphere"))
			{
				ColliderMeta.offset.Set(
					JsonCollider["shape"]["sphere"]["offset"][0].GetFloat(),
					JsonCollider["shape"]["sphere"]["offset"][1].GetFloat(),
					JsonCollider["shape"]["sphere"]["offset"][2].GetFloat());
				ColliderMeta.radius = JsonCollider["shape"]["sphere"]["radius"].GetFloat();
				ColliderMeta.shapeType = TEXT("sphere");
			}
			if (JsonCollider["shape"].HasMember("capsule"))
			{
				ColliderMeta.offset.Set(
					JsonCollider["shape"]["capsule"]["offset"][0].GetFloat(),
					JsonCollider["shape"]["capsule"]["offset"][1].GetFloat(),
					JsonCollider["shape"]["capsule"]["offset"][2].GetFloat());
				ColliderMeta.radius = JsonCollider["shape"]["capsule"]["radius"].GetFloat();
				ColliderMeta.tail.Set(
					JsonCollider["shape"]["capsule"]["tail"][0].GetFloat(),
					JsonCollider["shape"]["capsule"]["tail"][1].GetFloat(),
					JsonCollider["shape"]["capsule"]["tail"][2].GetFloat());
				ColliderMeta.shapeType = TEXT("capsule");
			}
		}
	}
	// Collider Groups.
	const auto JsonColliderGroupsPair = TryGetJsonValue(VrmConverter->jsonData.doc, {"extensions", "VRMC_springBone", "colliderGroups"});
	if (JsonColliderGroupsPair.Key && JsonColliderGroupsPair.Value != nullptr)
	{
		const auto& JsonColliderGroups = *JsonColliderGroupsPair.Value;
		auto& ColliderGroups = MetaObject->VRM1SpringBoneMeta.ColliderGroups;
		ColliderGroups.SetNum(JsonColliderGroups.Size());
		for (int ColliderGroupIndex = 0; ColliderGroupIndex < static_cast<int>(JsonColliderGroups.Size()); ++ColliderGroupIndex)
		{
			ColliderGroups[ColliderGroupIndex].name = JsonColliderGroups[ColliderGroupIndex].GetString();
			for (int ColliderIndex = 0; ColliderIndex < static_cast<int>(JsonColliderGroups[ColliderGroupIndex]["colliders"].Size()); ++ColliderIndex)
			{
				ColliderGroups[ColliderGroupIndex].colliders.Add(JsonColliderGroups[ColliderGroupIndex]["colliders"][ColliderIndex].GetInt());
			}
		}
	}
	// SpringMetas.SetNum(JsonSpring.Size());
	// auto& jsonColliderGroups = JsonSpring.GetArray()[SpringNo]["collidergroups"];

	// constraint.
	for (const auto& Nodes = VrmConverter->jsonData.doc["nodes"];
	     auto& Node : Nodes.GetArray())
	{
		const auto ConstraintPair = TryGetJsonValue(Node, {"extensions", "VRMC_node_constraint", "constraint"});
		if (ConstraintPair.Key == false || ConstraintPair.Value == nullptr)
		{
			continue;
		}
		const auto& Constraint = *ConstraintPair.Value;
		if (Constraint.HasMember("roll"))
		{
			FVRMConstraintRoll ConstraintRoll;
			ConstraintRoll.source = Constraint["roll"]["source"].GetInt();
			if (ConstraintRoll.source < static_cast<int>(Nodes.Size()))
			{
				ConstraintRoll.sourceName = Nodes.GetArray()[ConstraintRoll.source]["name"].GetString();
			}
			ConstraintRoll.rollAxis = Constraint["roll"]["rollAxis"].GetString();
			ConstraintRoll.weight = Constraint["roll"]["weight"].GetFloat();

			FString NodeName = Node["name"].GetString();
			if (VRMConverter::Options::Get().IsForceOriginalBoneName() == false)
			{
				NodeName = VRMUtil::MakeName(NodeName, true);
				ConstraintRoll.sourceName = VRMUtil::MakeName(ConstraintRoll.sourceName, true);
			}
			FVRMConstraint VrmConstraint;
			VrmConstraint.constraintRoll = ConstraintRoll;
			VrmConstraint.type = EVRMConstraintType::Roll;

			MetaObject->VRMConstraintMeta.Add(NodeName, VrmConstraint);
		}
		if (Constraint.HasMember("aim"))
		{
			FVRMConstraintAim ConstraintAim;

			ConstraintAim.source = Constraint["aim"]["source"].GetInt();
			if (ConstraintAim.source < static_cast<int>(Nodes.Size()))
			{
				ConstraintAim.sourceName = Nodes.GetArray()[ConstraintAim.source]["name"].GetString();
			}
			ConstraintAim.aimAxis = Constraint["aim"]["aimAxis"].GetString();
			ConstraintAim.weight = Constraint["aim"]["weight"].GetFloat();

			FString Name = Node["name"].GetString();
			if (VRMConverter::Options::Get().IsForceOriginalBoneName() == false)
			{
				Name = VRMUtil::MakeName(Name, true);
				ConstraintAim.sourceName = VRMUtil::MakeName(ConstraintAim.sourceName, true);
			}

			FVRMConstraint VrmConstraint;
			VrmConstraint.constraintAim = ConstraintAim;
			VrmConstraint.type = EVRMConstraintType::Aim;

			MetaObject->VRMConstraintMeta.Add(Name, VrmConstraint);
		}
		if (Constraint.HasMember("rotation"))
		{
			FVRMConstraintRotation ConstraintRotation;

			ConstraintRotation.source = Constraint["rotation"]["source"].GetInt();
			if (ConstraintRotation.source < static_cast<int>(Nodes.Size()))
			{
				ConstraintRotation.sourceName = Nodes.GetArray()[ConstraintRotation.source]["name"].GetString();
			}
			ConstraintRotation.weight = Constraint["rotation"]["weight"].GetFloat();

			FString Name = Node["name"].GetString();
			if (VRMConverter::Options::Get().IsForceOriginalBoneName() == false)
			{
				Name = VRMUtil::MakeName(Name, true);
				ConstraintRotation.sourceName = VRMUtil::MakeName(ConstraintRotation.sourceName, true);
			}

			FVRMConstraint VrmConstraint;
			VrmConstraint.constraintRotation = ConstraintRotation;
			VrmConstraint.type = EVRMConstraintType::Rotation;

			MetaObject->VRMConstraintMeta.Add(Name, VrmConstraint);
		}
	}
	// vrma.
	// VRoid::ConvertVRMAMeta(VrmConverter.Get(), MetaObject, Out);

	// license.
	const auto MetaPair = TryGetJsonValue(VrmConverter->jsonData.doc, {"extensions", "VRMC_vrm", "meta"});
	if (MetaPair.Key && MetaPair.Value != nullptr)
	{
		const auto& Metas = *MetaPair.Value;
		for (auto Meta = Metas.MemberBegin(); Meta != Metas.MemberEnd(); ++Meta)
		{
			if (FString Key = UTF8_TO_TCHAR(Meta->name.GetString());
				Key.Find("allow") == 0)
			{
				FLicenseBoolDataPair BoolDataPair;
				BoolDataPair.key = Key;
				BoolDataPair.value = Meta->value.GetBool();
				License10->LicenseBool.Add(BoolDataPair);
			}
			else if (Key == "thumbnailImage")
			{
				if (Out)
				{
					if (int TextureIndex = Meta->value.GetInt();
						TextureIndex >= 0 && TextureIndex < Out->Textures.Num())
					{
						License10->thumbnail = Out->Textures[TextureIndex];
#if WITH_EDITORONLY_DATA
						Out->SmallThumbnailTexture = License10->thumbnail;
#endif // WITH_EDITORONLY_DATA
					}
				}
			}
			else
			{
				if (Meta->value.IsArray())
				{
					int Index = 0;
					bool bFound = false;
					for (const auto& LicenseStringData : License10->LicenseStringArray)
					{
						if (LicenseStringData.key != Key)
						{
							++Index;
							continue;
						}
						bFound = true;
						break;
					}
					if (bFound == false)
					{
						Index = License10->LicenseStringArray.AddDefaulted();
						License10->LicenseStringArray[Index].key = Key;
					}
					for (auto& a : Meta->value.GetArray())
					{
						License10->LicenseStringArray[Index].value.Add(UTF8_TO_TCHAR(a.GetString()));
					}
				}
				else
				{
					FLicenseStringDataPair p;
					p.key = Key;
					p.value = UTF8_TO_TCHAR(Meta->value.GetString());
					License10->LicenseString.Add(p);
				}
			}
		}
	}
	return true;
}

FString VRoidAsyncAsset::GetSequenceName() const
{
	const auto ConvertEnumPtr = StaticEnum<EVRoidConvertSequence>();
	return ConvertEnumPtr->GetNameStringByValue(static_cast<uint8>(ConvertSequenceStatus));
}

const aiTexture* VRoidAsyncAsset::GetTexture(const int Index) const
{
	if (AiScene == nullptr || Index < 0)
	{
		return nullptr;
	}
	if (AiScene->HasTextures() == false || static_cast<int32>(GetTextureNum()) <= Index)
	{
		return nullptr;
	}
	return AiScene->mTextures[Index];
}

bool VRoidAsyncAsset::UpdateImage(const int Index, const FVRoidImage& Image)
{
	if (Images.IsValidIndex(Index))
	{
		Images[Index] = Image;
		return true;
	}
	return false;
}

void VRoidAsyncAsset::Reset()
{
	Importer.Reset();
	AiScene.Reset();

	NormalBoolTable.Reset();
	Images.Reset();
}

void VRoidAsyncAsset::ReadVrmFromMemory(const FString& FilePath, const uint8* const Data, const size_t Size, UVrmAssetListObject* const VrmAssetList, TFuture<bool>& AsyncFileReadTask)
{
	constexpr unsigned int ReadFlag = (aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes | aiProcess_PopulateArmatureData);
	constexpr char VrmExtension[4] = "vrm";

	VRMConverter::Options::Get().ClearModelType();
	if (const FString Ext = FPaths::GetExtension(FilePath).ToLower(); Ext.Compare(VrmExtension) == 0)
	{
		if (UVRoidFunctionLibrary::IsVrm10(Data, Size))
		{
			IsVrm10Model = true;
			VRMConverter::Options::Get().SetVRM10Model(true);
		}
		else
		{
			VRMConverter::Options::Get().SetVRM0Model(true);
		}
	}

	AsyncFileReadTask = Async(EAsyncExecution::Thread, [this, FilePath, Data, Size]
	{
		if (Data == nullptr || Size <= 0)
		{
			VROID_ERROR(TEXT("Failed read vrm from File(%s)."), *FilePath);
			return false;
		}
		Importer = MakeShared<Assimp::Importer, ESPMode::ThreadSafe>();
		Importer->SetPropertyBool(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, false);

		const aiScene* RawScene = Importer->ReadFileFromMemory(Data, Size, ReadFlag, VrmExtension);
		if (RawScene == nullptr)
		{
			RawScene = Importer->ReadFile(UVRoidSdkCoreFunctionLibrary::FStringToStdString(FilePath), ReadFlag);
		}
		if (RawScene)
		{
			const TSharedPtr<aiScene, ESPMode::ThreadSafe> ScenePtr(
				const_cast<aiScene*>(RawScene),
				[this](aiScene*) mutable
				{
					Importer.Reset();
				}
			);
			AiScene = ScenePtr;
		}

		if (AiScene)
		{
			VrmConverter = MakeShared<VRMConverter, ESPMode::ThreadSafe>();
			constexpr int32 VrmEncryptedHeaderSize = 44;
			VrmConverter->Init(Data, Size - VrmEncryptedHeaderSize, AiScene.Get());
			VroidConverter = MakeShared<VRoidConverter, ESPMode::ThreadSafe>(AiScene.Get(), IsVrm10Model);
			const auto TextureNum(AiScene->mNumTextures);
			Images.SetNum(TextureNum);
			return true;
		}
		VROID_ERROR(TEXT("Failed read vrm from File(%s)."), *FilePath);
		return false;
	});

	VrmAssetList->FileFullPathName = FilePath;
	VrmAssetList->Package = GetTransientPackage();
}

void VRoidAsyncAsset::ConvertTexture(const int TexCount, const int SubCount, UVrmAssetListObject* const VrmAssetList, const bool IsFastConverter)
{
	if (VrmAssetList == nullptr || AiScene == nullptr)
	{
		return;
	}
	const int TextureNum(static_cast<int>(AiScene->mNumTextures));
	if (TextureNum <= TexCount)
	{
		return;
	}
	if (TexCount == 0 && SubCount == 0)
	{
		NormalBoolTable.Reset();
		VrmAssetList->Textures.Empty();

		NormalBoolTable.SetNum(TextureNum);
		VrmAssetList->Textures.SetNum(TextureNum);

		if (const VRM::VRMMetadata* Meta = static_cast<const VRM::VRMMetadata*>(AiScene->mVRMMeta))
		{
			for (int i = 0; i < Meta->materialNum; ++i)
			{
				if (const int t = Meta->material[i].textureProperties._BumpMap; t > 0 && NormalBoolTable.IsValidIndex(t))
				{
					NormalBoolTable[t] = true;
				}
			}
		}
	}
	if (AiScene->HasTextures() == false || VrmAssetList->Textures.Num() < TexCount)
	{
		return;
	}

	const auto& AITexture(*AiScene->mTextures[TexCount]);
	const bool IsNormal(NormalBoolTable.Num() >= TexCount && NormalBoolTable[TexCount]);
	if (IsFastConverter)
	{
		const auto NewTexture2D(VroidConverter->CreateTexture(Images[TexCount], IsNormal, false, &VrmAssetList->Texture_CompressTypeList));
		if (NewTexture2D == nullptr)
		{
			return;
		}
		VrmAssetList->Textures[TexCount] = NewTexture2D;
		return;
	}

	if (SubCount == 0)
	{
		FString BaseName = VRMConverter::NormalizeFileName(AITexture.mFilename.C_Str());
		if (BaseName.Len() == 0)
		{
			BaseName = TEXT("texture") + FString::FromInt(TexCount);
		}
		if (NormalBoolTable[TexCount])
		{
			BaseName += TEXT("_N");
		}
		const FString Name = FString(TEXT("T_")) + BaseName;
		VrmAssetList->Textures[TexCount] = UVRoidFunctionLibrary::CreateTextureFromImageWrapper(Name, AITexture, false, IsNormal);
		return;
	}
	if (SubCount == 1)
	{
		UTexture2D* const NewTexture2D = VrmAssetList->Textures[TexCount];
#if WITH_EDITOR
		NewTexture2D->DeferCompression = false;
#endif // WITH_EDITOR
		// Set options
		if (NormalBoolTable[TexCount])
		{
			NewTexture2D->CompressionSettings = TC_Normalmap;
			NewTexture2D->SRGB = 0;
		}
		if (NewTexture2D->SRGB && VRMConverter::Options::Get().IsBC7Mode())
		{
			NewTexture2D->CompressionSettings = TC_BC7;
		}
		if (VRMConverter::Options::Get().IsMipmapGenerateMode() == false)
		{
#if WITH_EDITORONLY_DATA
			NewTexture2D->MipGenSettings = TMGS_NoMipmaps;
#endif // WITH_EDITORONLY_DATA
		}
		NewTexture2D->UpdateResource();
	}
}

bool VRoidAsyncAsset::ConvertVrm(const uint8* const Data, const size_t Size, UVrmAssetListObject*& Out)
{
	bool bRet = true;
	const TSharedRef GameThreadTaskFunction(MakeShared<TFunction<void()>, ESPMode::ThreadSafe>());

	switch (ConvertSequenceStatus)
	{
	case EVRoidConvertSequence::Init:
		// bRet &= VrmConverter->ConvertVrmFirst(Out, Data, Size);
		bRet &= ConvertVrmFirst(Out);
		bRet &= VrmConverter->NormalizeBoneName(AiScene.Get());
		ConvertSequenceStatus = EVRoidConvertSequence::ConvertTextureAndMaterial;
		break;

	case EVRoidConvertSequence::ConvertTextureAndMaterial:
#if 0
		*GameThreadTaskFunction = [&] { bRet &= VrmConverter->ConvertTextureAndMaterial(Out); };
		UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadTaskFunction);
#else
		bRet &= VroidConverter->ConvertTextureAndMaterial(VrmConverter.Get(), Out);
#endif
		// bRet &= VrmConverter->ConvertVrmMeta(Out, AiScene.Get(), Data, Size);
		bRet &= IsVrm10Model ? ConvertVrm10Meta(Data, Size, Out) : ConvertVrm0Meta(Data, Size, Out);
		ConvertSequenceStatus = EVRoidConvertSequence::ConvertModel;
		break;

	case EVRoidConvertSequence::ConvertModel:
#if	UE_OLDER_5_5
		bRet &= VroidConverter->ConvertModel(Out);
#else
		*GameThreadTaskFunction = [&]
		{
			// bRet &= VrmConverter->ConvertModel(Out);
			bRet &= VroidConverter->ConvertModel(Out);
		};
		UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadTaskFunction);
#endif // UE_OLDER_5_5
		ConvertSequenceStatus = EVRoidConvertSequence::ConvertMetaPost;
		break;

	case EVRoidConvertSequence::ConvertMetaPost:
		// bRet &= VrmConverter->ConvertVrmMetaPost(Out, AiScene.Get(), Data, Size);
		*GameThreadTaskFunction = [&]
		{
			bRet &= UVRoidFunctionLibrary::ConvertVrmMetaRenamedWrapper(VrmConverter.Get(), Out, AiScene.Get(), Data, Size);
		};
		UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadTaskFunction);
		ConvertSequenceStatus = EVRoidConvertSequence::ConvertMorphTarget;
		break;

	case EVRoidConvertSequence::ConvertMorphTarget:
		bRet &= VrmConverter->ConvertRig(Out);
		// bRet &= VrmConverter->ConvertIKRig(Out);
		*GameThreadTaskFunction = [&] { bRet &= VrmConverter->ConvertIKRig(Out); };
		UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadTaskFunction);
		if (Out->bSkipMorphTarget == false)
		{
			// *GameThreadTaskFunction = [&] { bRet &= VrmConverter->ConvertMorphTarget(Out); };
			// UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadTaskFunction);
			bRet &= VroidConverter->ConvertMorphTarget(Out);
		}
#if WITH_EDITOR
		ConvertSequenceStatus = EVRoidConvertSequence::ConvertPose;
#else
		ConvertSequenceStatus = EVRoidConvertSequence::ConvertHumanoid;
#endif // WITH_EDITOR
		break;

	case EVRoidConvertSequence::ConvertPose:
#if WITH_EDITOR
		// Generation of POSE_face_asset. However, PoseAsset->Modify() causes the Dirty flag to be set on the Level.
		*GameThreadTaskFunction = [&] { bRet &= VrmConverter->ConvertPose(Out); };
		UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadTaskFunction);
#endif // WITH_EDITOR
		ConvertSequenceStatus = EVRoidConvertSequence::ConvertHumanoid;
		break;

	case EVRoidConvertSequence::ConvertHumanoid:
		// *GameThreadTaskFunction = [&] { bRet &= VrmConverter->ConvertHumanoid(Out); };
		// UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadTaskFunction);
		VroidConverter->ConvertMannequin(Out);

		Out->MeshReturnedData = nullptr;
#if 0
#if UE_NEWER_5_7
		// UE5.7 requires CreateMeshDescription to be called only after PostLoad.
		*GameThreadTaskFunction = [&] { Out->PostLoad(); };
		UVRoidFunctionLibrary::RunAndWaitForGameThreadTask(GameThreadTaskFunction);
#endif // UE_NEWER_5_7
#endif
		Out->VrmMetaObject->SkeletalMesh = Out->SkeletalMesh;
		// Out->VrmMetaObject->SkeletalMesh->SetPhysicsAsset(nullptr);
		ConvertSequenceStatus = EVRoidConvertSequence::Finish;
		break;

	case EVRoidConvertSequence::Finish:
	default:
		break;
	}
	if (bRet == false)
	{
		VROID_ERROR(TEXT("Failed convert VRM."));
		return false;
	}
	return bRet;
}
