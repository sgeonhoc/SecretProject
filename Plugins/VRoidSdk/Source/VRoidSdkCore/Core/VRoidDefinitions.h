// Copyright © 2025 pixiv Inc. All rights reserved.

#pragma once

// #include "CoreMinimal.h"
// #include "VRoidDefinitions.generated.h"

/** 
 * エンジンバージョン別のラッパーマクロ
 * バージョンごとのコードの検索コストを上げるために定義
 * Wrapper macros for Unreal Engine version checks.
 * Defined to make code differences between engine versions easier to locate.
 */
#define UE_OLDER_5_4 UE_VERSION_OLDER_THAN(5, 4, 0)
#define UE_OLDER_5_5 UE_VERSION_OLDER_THAN(5, 5, 0)
#define UE_OLDER_5_6 UE_VERSION_OLDER_THAN(5, 6, 0)
#define UE_OLDER_5_7 UE_VERSION_OLDER_THAN(5, 7, 0)

#define UE_NEWER_5_4 UE_VERSION_NEWER_THAN(5, 4, 0)
#define UE_NEWER_5_5 UE_VERSION_NEWER_THAN(5, 5, 0)
#define UE_NEWER_5_6 UE_VERSION_NEWER_THAN(5, 6, 0)
#define UE_NEWER_5_7 UE_VERSION_NEWER_THAN(5, 7, 0)
