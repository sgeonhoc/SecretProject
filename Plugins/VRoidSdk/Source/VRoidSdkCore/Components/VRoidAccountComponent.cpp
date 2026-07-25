//
// Created by Mameo
// Copyright © 2022 pixiv Inc. All rights reserved.
//

#include "VRoidAccountComponent.h"

#include "Core/VRoidLogger.h"
#include "Core/VRoidSdkCoreFunctionLibrary.h"
#include "Subsystems/VRoidGameInstanceSubsystem.h"

UVRoidAccountComponent::UVRoidAccountComponent()
	: Account(vroid::authorization::Oauth::Account())
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UVRoidAccountComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureAndValidateAccessTokenPath();
}

void UVRoidAccountComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UVRoidAccountComponent::EnsureAndValidateAccessTokenPath()
{
	if (ValidateAccountFile())
	{
		return true;
	}
	AccessTokenPath = UVRoidSdkCoreFunctionLibrary::EnsureAccessTokenPath(this);
	return true;
}

bool UVRoidAccountComponent::WriteAccountJson(const FString& Chunk)
{
	FString ChunkCopy = Chunk;
	ChunkCopy = ChunkCopy.Replace(TEXT("\""), TEXT(""));
	ChunkCopy = ChunkCopy.Replace(TEXT("{"), TEXT(""));
	ChunkCopy = ChunkCopy.Replace(TEXT("}"), TEXT(""));

	TArray<FString> ChunkArray;
	ChunkCopy.ParseIntoArray(ChunkArray, TEXT(","));

	const TSharedPtr<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
	for (const FString Field : ChunkArray)
	{
		FString FieldName;
		FString FieldValue;
		Field.Split(TEXT(":"), &FieldName, &FieldValue);
		JsonRootObject->SetStringField(FieldName, FieldValue);
	}

	FString OutputString;
	const TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonRootObject.ToSharedRef(), JsonWriter);

	if (EnsureAndValidateAccessTokenPath())
	{
		return FFileHelper::SaveStringToFile(OutputString, *AccessTokenPath);
	}
	return false;
}

bool UVRoidAccountComponent::LoadAccountJson()
{
	return UVRoidSdkCoreFunctionLibrary::LoadAccountJson(this, Account);
}

bool UVRoidAccountComponent::HasAccount() const
{
	return (Account.access_token.empty() == false && Account.token_type.empty() == false &&
		Account.refresh_token.empty() == false && Account.scope.empty() == false && 
		Account.expires_in != 0 && Account.created_at != 0);
}

vroid::authorization::Oauth::Account UVRoidAccountComponent::GetAccount() const
{
	return Account;
}

bool UVRoidAccountComponent::ValidateAccountFile() const
{
	return (FPaths::ValidatePath(AccessTokenPath) && FPaths::FileExists(AccessTokenPath));
}

bool UVRoidAccountComponent::DeleteAccount() const
{
	if (ValidateAccountFile() == false)
	{
		return false;
	}
	const bool IsDelete = IFileManager::Get().Delete(*AccessTokenPath);
	if (IsDelete)
	{
		if (const auto VRoidSubsystem = UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(this))
		{
			VRoidSubsystem->SetIsLogin(false);
		}
		else
		{
			VROID_ERROR(TEXT("Failed get VRoidGameInstanceSubSystem."));
		}
	}
	return IsDelete;
}
