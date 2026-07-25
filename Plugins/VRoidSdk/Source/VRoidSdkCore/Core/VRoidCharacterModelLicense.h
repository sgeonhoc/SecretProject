//
// Created by udemegane
// Copyright © 2022 pixiv Inc. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "VRoidCharacterModelLicense.generated.h"

USTRUCT(BlueprintType, Category="VRoid|License")
struct FVRMLicense
{
	GENERATED_BODY()

	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString Modification{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString Redistribution{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString Credit{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString CharacterizationAllowedUser{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString SexualExpression{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString ViolentExpression{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString CorporateCommercialUse{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString PersonalCommercialUse{TEXT("default")};

	static FVRMLicense Deserialize(const TSharedPtr<FJsonObject>& LicenseObject);
};

USTRUCT(BlueprintType, Category="VRoid|License")
struct FVRMLicense10
{
	GENERATED_BODY()

	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString AntisocialOrHateUse{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString ExcessivelySexualUse{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString ExcessivelyViolentUse{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString PoliticalOrReligiousUse{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString Redistribution{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString AvatarPermission{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString CorporateCommercialUse{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString CommercialUse{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString Credit{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString Modification{TEXT("default")};
	UPROPERTY(Transient, EditAnywhere, BlueprintReadOnly, Category="VRoid|License")
	FString ModificationRedistribution{TEXT("default")};
	
	static FVRMLicense10 Deserialize(const TSharedPtr<FJsonObject>& LicenseObject);
};

constexpr auto DefaultToUndefined = [](const FString& LicenseText)
{
	if (FCString::Strcmp(*LicenseText, TEXT("default")) == 0)
	{
		return TEXT("未設定");
	}
	return *LicenseText;
};

inline FVRMLicense FVRMLicense::Deserialize(const TSharedPtr<FJsonObject>& LicenseObject)
{
	return FVRMLicense
	{
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("modification"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("redistribution"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("credit"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("characterization_allowed_user"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("sexual_expression"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("violent_expression"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("corporate_commercial_use"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("personal_commercial_use"))),
	};
}

inline FVRMLicense10 FVRMLicense10::Deserialize(const TSharedPtr<FJsonObject>& LicenseObject)
{
	return FVRMLicense10
	{
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("allowAntisocialOrHateUsage"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("allowExcessivelySexualUsage"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("allowExcessivelyViolentUsage"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("allowPoliticalOrReligiousUsage"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("allowRedistribution"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("avatarPermission"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("commercialUsage"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("commercialUsage"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("creditNotation"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("modification"))),
		DefaultToUndefined(LicenseObject->GetStringField(TEXT("modification"))),
	};
}