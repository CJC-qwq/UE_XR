// Copyright Epic Games, Inc. All Rights Reserved.

#include "XRBPLibrary.h"

#include "XR.h"

bool UXRBPLibrary::IsXRPluginAvailable()
{
	return true;
}

void UXRBPLibrary::ShowXRRuntimeDebugWorkspace()
{
	if (FXRModule::IsAvailable())
	{
		FXRModule::Get().ShowRuntimeDebugWorkspace();
	}
}

void UXRBPLibrary::HideXRRuntimeDebugWorkspace()
{
	if (FXRModule::IsAvailable())
	{
		FXRModule::Get().HideRuntimeDebugWorkspace();
	}
}

void UXRBPLibrary::ToggleXRRuntimeDebugWorkspace()
{
	if (FXRModule::IsAvailable())
	{
		FXRModule::Get().ToggleRuntimeDebugWorkspace();
	}
}

bool UXRBPLibrary::IsXRRuntimeDebugWorkspaceVisible()
{
	return FXRModule::IsAvailable() && FXRModule::Get().IsRuntimeDebugWorkspaceVisible();
}
