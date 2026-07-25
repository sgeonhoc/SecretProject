//
// Created by udemegane
// Copyright © 2022 pixiv Inc. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "VRoidCharacterModelLicense.h"
#include "VRoidCharacterModel.generated.h"

USTRUCT(BlueprintType, Category="VRoid")
struct FVRMCharacterModelVersionBoundingBox
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FVector Max = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FVector Min = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FVector Size = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FVector Center = FVector::ZeroVector;

	static FVRMCharacterModelVersionBoundingBox Deserialize(const TSharedPtr<FJsonObject>& BoundingBoxObject);
};

USTRUCT(BlueprintType, Category="VRoid")
struct FVRMCharacterModelMinimal
{
	GENERATED_BODY()

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FVRMLicense License;
	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FVRMLicense10 License10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	bool IsVrm10 = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString Name = TEXT("");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString UserName = TEXT("");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString ModelId = TEXT("");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	bool IsDownloadable = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	TObjectPtr<UTexture2DDynamic> PortraitImage = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString PortraitUrl = TEXT("");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	TObjectPtr<UTexture2DDynamic> FullBodyImage = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString FullBodyUrl = TEXT("");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString ExporterVersion = TEXT("");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString UserIconUrl = TEXT("");

	static FVRMCharacterModelMinimal Deserialize(const TSharedPtr<FJsonObject>& CharacterObject);
};

USTRUCT(BlueprintType, Category="VRoid")
struct FVRMCharacterModelProperty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FVRMCharacterModelVersionBoundingBox BoundingBox;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString CharacterModelVersionId = TEXT("");
#if 0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FCharacterModelVersionMaterial CharacterModelVersionMaterial;
#endif
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString ExporterVersion = TEXT("");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString Id = TEXT("");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	int JointCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	int MaterialCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	int MeshCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	int MeshPrimitiveCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	int MeshPrimitiveMorphCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString SpecVersion = TEXT("");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	int TextureCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	int TriangleCount = 0;

	static FVRMCharacterModelProperty Deserialize(const TSharedPtr<FJsonObject>& CharacterObject);
};

USTRUCT(BlueprintType, Category="VRoid")
struct FStaffPicksCharacterModelMinimal
{
	GENERATED_BODY()

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FVRMCharacterModelMinimal CharacterModel;
	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString CreatedAt = TEXT("");

	static FStaffPicksCharacterModelMinimal Deserialize(const TSharedPtr<FJsonObject>& CharacterObject);
};

/**
 * Along with deserialization, Y-up to Z-up conversion, Scale correction and Center coordinates are calculated.
 */
inline FVRMCharacterModelVersionBoundingBox FVRMCharacterModelVersionBoundingBox::Deserialize(const TSharedPtr<FJsonObject>& BoundingBoxObject)
{
	// Perform Y-up to Z-up conversion.
	const auto ObjectToVector = [&BoundingBoxObject](const FString& FieldName)
	{
		if (const TSharedPtr<FJsonObject>* Object; BoundingBoxObject->TryGetObjectField(FieldName, Object))
		{
			return FVector(Object->Get()->GetNumberField(TEXT("z")),
			               Object->Get()->GetNumberField(TEXT("x")),
			               Object->Get()->GetNumberField(TEXT("y"))) * 100.0f;
		}
		return FVector::ZeroVector;
	};

	// Set the deserialized value to 100x for Unreal.
	FVRMCharacterModelVersionBoundingBox boundingBox
	{
		FVector(ObjectToVector("max")),
		FVector(ObjectToVector("min")),
		FVector(ObjectToVector("size"))
	};
	// If size is not set, calculate size from min and max.
	if (boundingBox.Size.IsNearlyZero())
	{
		boundingBox.Size = (boundingBox.Max.GetAbs() + boundingBox.Min.GetAbs()) * 0.5f;
	}
	boundingBox.Center = (boundingBox.Max + boundingBox.Min) * 0.5f;

	return boundingBox;
}

inline FVRMCharacterModelMinimal FVRMCharacterModelMinimal::Deserialize(const TSharedPtr<FJsonObject>& CharacterObject)
{
	const TSharedPtr<FJsonObject>* Character;
	if (CharacterObject->TryGetObjectField(TEXT("character"), Character) == false)
	{
		return FVRMCharacterModelMinimal();
	}
	const TSharedPtr<FJsonObject>* User;
	if (Character->Get()->TryGetObjectField(TEXT("user"), User) == false)
	{
		return FVRMCharacterModelMinimal();
	}

	FVRMLicense vrmLicense{};
	if (const TSharedPtr<FJsonObject>* License; CharacterObject->TryGetObjectField(TEXT("license"), License))
	{
		vrmLicense = FVRMLicense::Deserialize(*License);
	}

	FString exporterVersion(TEXT(""));
	bool isVrm10 = false;
	FVRMLicense10 vrmLicense10{};
	if (const TSharedPtr<FJsonObject>* ModelVersion; CharacterObject->TryGetObjectField(TEXT("latest_character_model_version"), ModelVersion))
	{
		if (const auto ExporterField = ModelVersion->Get()->TryGetField(TEXT("exporter_version")); ExporterField->IsNull() == false)
		{
			exporterVersion = ExporterField->AsString();
		}
		if (const auto SpecField = ModelVersion->Get()->TryGetField(TEXT("spec_version")); SpecField->IsNull() == false)
		{
			if (const auto Spec = SpecField->AsString(); Spec.Contains("1.0"))
			{
				isVrm10 = true;
			}
		}
		if (const TSharedPtr<FJsonObject>* Meta; ModelVersion->Get()->TryGetObjectField(TEXT("vrm_meta"), Meta) && isVrm10)
		{
			vrmLicense10 = FVRMLicense10::Deserialize(*Meta);
		}
	}

	FString userIconUrl(TEXT(""));
	if (const TSharedPtr<FJsonObject>* Icon; User->Get()->TryGetObjectField(TEXT("icon"), Icon))
	{
		if (const TSharedPtr<FJsonObject>* sq; Icon->Get()->TryGetObjectField(TEXT("sq170"), sq))
		{
			userIconUrl = sq->Get()->GetStringField(TEXT("url"));
		}
	}

	return FVRMCharacterModelMinimal
	{
		vrmLicense,
		vrmLicense10,
		isVrm10,
		Character->Get()->GetStringField(TEXT("name")),
		User->Get()->GetStringField(TEXT("name")),
		CharacterObject->GetStringField(TEXT("id")),
		CharacterObject->GetBoolField(TEXT("is_downloadable")),
		nullptr,
		CharacterObject->GetObjectField(TEXT("portrait_image"))->GetObjectField(TEXT("original"))->GetStringField(TEXT("url")),
		nullptr,
		CharacterObject->GetObjectField(TEXT("full_body_image"))->GetObjectField(TEXT("original"))->GetStringField(TEXT("url")),
		exporterVersion,
		userIconUrl,
	};
}

inline FVRMCharacterModelProperty FVRMCharacterModelProperty::Deserialize(const TSharedPtr<FJsonObject>& CharacterObject)
{
	FString specVersion(TEXT(""));
	if (const auto SpecField = CharacterObject->TryGetField(TEXT("spec_version")); SpecField->IsNull() == false)
	{
		specVersion = SpecField->AsString();
	}
	FString exporterVersion(TEXT(""));
	if (const auto ExporterField = CharacterObject->TryGetField(TEXT("exporter_version")); ExporterField->IsNull() == false)
	{
		exporterVersion = ExporterField->AsString();
	}

	return FVRMCharacterModelProperty
	{
		FVRMCharacterModelVersionBoundingBox::Deserialize(CharacterObject->GetObjectField(TEXT("character_model_version_bounding_box"))),
		CharacterObject->GetStringField(TEXT("character_model_version_id")),
		//FCharacterModelVersionMaterial::Deserialize(CharacterObject->GetObjectField("character_model_version_material")),
		exporterVersion,
		CharacterObject->GetStringField(TEXT("id")),
		CharacterObject->GetIntegerField(TEXT("joint_count")),
		CharacterObject->GetIntegerField(TEXT("material_count")),
		CharacterObject->GetIntegerField(TEXT("mesh_count")),
		CharacterObject->GetIntegerField(TEXT("mesh_primitive_count")),
		CharacterObject->GetIntegerField(TEXT("mesh_primitive_morph_count")),
		specVersion,
		CharacterObject->GetIntegerField(TEXT("texture_count")),
		CharacterObject->GetIntegerField(TEXT("triangle_count")),
	};
}

inline FStaffPicksCharacterModelMinimal FStaffPicksCharacterModelMinimal::Deserialize(const TSharedPtr<FJsonObject>& StaffPicksCharacterObject)
{
	return FStaffPicksCharacterModelMinimal
	{
		FVRMCharacterModelMinimal::Deserialize(StaffPicksCharacterObject->GetObjectField(TEXT("character_model"))),
		StaffPicksCharacterObject->GetStringField(TEXT("created_at"))
	};
}
