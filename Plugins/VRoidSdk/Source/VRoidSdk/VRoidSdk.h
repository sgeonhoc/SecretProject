//
// Created by Mameo
// Copyright © 2022 pixiv Inc. All rights reserved.
//

#pragma once

class FVRoidSdkModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void* ModuleHandle = nullptr;
};
