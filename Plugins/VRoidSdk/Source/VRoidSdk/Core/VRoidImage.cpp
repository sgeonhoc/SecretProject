// Copyright © 2024 pixiv Inc. All rights reserved.

#include "VRoidImage.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

#include "Core/VRoidLogger.h"

namespace VRoid
{
	int32 GetBytesPerPixel(const ETextureSourceFormat Format)
	{
		switch (Format)
		{
		case TSF_G8: return 1;
		case TSF_G16: return 2;
		case TSF_BGRA8: return 4;
		case TSF_BGRE8: return 4;
		case TSF_RGBA16: return 8;
		case TSF_RGBA16F: return 8;
		default: return 0;
		}
	}
} // namespace VRoid

void FVRoidImage::Init(const int32 InSizeX, const int32 InSizeY, const ETextureSourceFormat InFormat, const uint8* Data)
{
	SizeX = InSizeX;
	SizeY = InSizeY;
	NumMips = 1;
	Format = InFormat;
	const int64 DataSize = SizeX * SizeY * VRoid::GetBytesPerPixel(Format);
	RawData.Reserve(DataSize);
	RawData.SetNumUninitialized(DataSize);
	if (Data)
	{
		constexpr int32 BlockSize = 1024 * 1024;
		for (int32 Offset = 0; Offset < DataSize; Offset += BlockSize)
		{
			const int32 SizeToCopy = FMath::Min(BlockSize, DataSize - Offset);
			FMemory::Memcpy(RawData.GetData() + Offset, Data + Offset, SizeToCopy);
		}
	}
}

bool FVRoidImage::LoadImageFromMemory(const void* Buffer, const int64 Length, FVRoidImage& OutImage)
{
	const uint8* Data = static_cast<const uint8*>(Buffer);

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	const TArray Formats = { EImageFormat::PNG, EImageFormat::JPEG, EImageFormat::BMP };
	TSharedPtr<IImageWrapper, ESPMode::ThreadSafe> ImageWrapper;
	for (const EImageFormat Format : Formats)
	{
		const TSharedPtr<IImageWrapper, ESPMode::ThreadSafe> Wrapper = ImageWrapperModule.CreateImageWrapper(Format);
		if (Wrapper->SetCompressed(Data, Length))
		{
			ImageWrapper = Wrapper;
			break;
		}
	}
	if (ImageWrapper.IsValid() == false)
	{
		return false;
	}

	TArray<uint8> OutRawData;
	if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, OutRawData) == false)
	{
		VROID_ERROR(TEXT("Failed to decode image data in BGRA 8-bit format."));
		return false;
	}
	const int32 Width = FMath::Max(ImageWrapper->GetWidth(), 1);
	const int32 Height = FMath::Max(ImageWrapper->GetHeight(), 1);

	OutImage.Init(Width, Height, TSF_BGRA8, OutRawData.GetData());
	return true;
}
