//
// Created by Mameo
// Copyright © 2023 pixiv Inc. All rights reserved.
//

#include "VRoidEditorSettings.h"

UVRoidEditorSettings::UVRoidEditorSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  MannequinMeshPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn")),
	  MannequinAnimPath(TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn")),
	  MannyAnimPath(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny")),
	  QuinnAnimPath(TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn"))
{
}
