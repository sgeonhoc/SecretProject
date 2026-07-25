//
// Created by Mameo
// Copyright © 2023 pixiv Inc. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"

VROIDSDKCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogVRoid, Log, All);

enum class EVRoidLogType : uint8
{
	Log,
	Warning,
	Error,
	Fatal,
	Display,
	Verbose,
};

class FVRoidLogger
{
public:
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION > 5)
	template <typename... Types>
	static void LogImpl(const EVRoidLogType LogType, const FString& File, const uint32 Line,
	                    UE::Core::TCheckedFormatString<FString::FmtCharType, Types...> Fmt, Types... Args)
#else
	template <typename FmtType, typename... Types>
	static void LogImpl(const EVRoidLogType LogType, const FString& File, const uint32 Line, const FmtType& Fmt, Types... Args)
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION > 5)
	{
		const FString Format = FString::Printf(Fmt, Args...);
		const FString LineLocation = FString::Printf(TEXT("(%s:%d)"), *FPaths::GetCleanFilename(File), Line);
		const FString Message = FString::Printf(TEXT("%s %s"), *Format, *LineLocation);

		switch (LogType)
		{
		default:
		case EVRoidLogType::Log:
			UE_LOG(LogVRoid, Log, TEXT("%s"), *Message);
			break;
		case EVRoidLogType::Warning:
			UE_LOG(LogVRoid, Warning, TEXT("%s"), *Message);
			break;
		case EVRoidLogType::Error:
			UE_LOG(LogVRoid, Error, TEXT("%s"), *Message);
			break;
		case EVRoidLogType::Fatal:
			UE_LOG(LogVRoid, Fatal, TEXT("%s"), *Message);
			break;
		case EVRoidLogType::Display:
			UE_LOG(LogVRoid, Display, TEXT("%s"), *Message);
			break;
		case EVRoidLogType::Verbose:
			UE_LOG(LogVRoid, Verbose, TEXT("%s"), *Message);
			break;
		}
	}
};

#define VROID_LOG(fmt, ...) FVRoidLogger::LogImpl(EVRoidLogType::Log, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VROID_WARNING(fmt, ...) FVRoidLogger::LogImpl(EVRoidLogType::Warning, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VROID_ERROR(fmt, ...) FVRoidLogger::LogImpl(EVRoidLogType::Error, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VROID_FATAL(fmt, ...) FVRoidLogger::LogImpl(EVRoidLogType::Fatal, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VROID_DISPLAY(fmt, ...) FVRoidLogger::LogImpl(EVRoidLogType::Display, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VROID_VERBOSE(fmt, ...) FVRoidLogger::LogImpl(EVRoidLogType::Verbose, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
