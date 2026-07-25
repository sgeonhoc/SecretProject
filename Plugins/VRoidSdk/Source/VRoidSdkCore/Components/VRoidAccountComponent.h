//
// Created by Mameo
// Copyright © 2022 pixiv Inc. All rights reserved.
//

#pragma once

#include "Components/ActorComponent.h"
#include "Library/vroidsdk/authorization/oauth.h"
#include "VRoidAccountComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VROIDSDKCORE_API UVRoidAccountComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVRoidAccountComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	FString AccessTokenPath;
	vroid::authorization::Oauth::Account Account;

	bool EnsureAndValidateAccessTokenPath();

public:
	bool WriteAccountJson(const FString& Chunk);
	bool LoadAccountJson();
	bool HasAccount() const;
	vroid::authorization::Oauth::Account GetAccount() const;

public:
	UFUNCTION(BlueprintPure, Category="VRoid")
	bool ValidateAccountFile() const;
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="VRoid")
	bool DeleteAccount() const;
};
