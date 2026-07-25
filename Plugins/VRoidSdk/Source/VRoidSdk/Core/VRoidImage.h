// Copyright © 2024 pixiv Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
// #include "VRoidImage.generated.h"

struct FVRoidImage
{
	// GENERATED_BODY()
	TArray64<uint8> RawData;
	ETextureSourceFormat Format = TSF_BGRA8;
	int32 NumMips = 1;
	int32 SizeX = 0;
	int32 SizeY = 0;

	void Init(const int32 InSizeX, const int32 InSizeY, const ETextureSourceFormat InFormat, const uint8* Data = nullptr);
	static bool LoadImageFromMemory(const void* Buffer, const int64 Length, FVRoidImage& OutImage); 
};
