// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FXRRuntimeDebugWorkspaceManager;
class IConsoleObject;
class IInputProcessor;

class FXRModule : public IModuleInterface
{
public:
	static FXRModule& Get();
	static bool IsAvailable();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void ShowRuntimeDebugWorkspace();
	void HideRuntimeDebugWorkspace();
	void ToggleRuntimeDebugWorkspace();
	bool IsRuntimeDebugWorkspaceVisible() const;

private:
	void HandlePostEngineInit();
	void RegisterRuntimeDebugSupport();
	void UnregisterRuntimeDebugSupport();

	TUniquePtr<FXRRuntimeDebugWorkspaceManager> RuntimeDebugWorkspaceManager;
	TSharedPtr<IInputProcessor> RuntimeDebugInputProcessor;
	IConsoleObject* ToggleRuntimeDebugWorkspaceCommand = nullptr;
	IConsoleObject* ShowRuntimeDebugWorkspaceCommand = nullptr;
	IConsoleObject* HideRuntimeDebugWorkspaceCommand = nullptr;
	FDelegateHandle PostEngineInitHandle;
	bool bRuntimeDebugSupportRegistered = false;
};
