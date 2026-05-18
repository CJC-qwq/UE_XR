// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "XRBPLibrary.generated.h"

UCLASS()
class XR_API UXRBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "XR")
	static bool IsXRPluginAvailable();

	UFUNCTION(BlueprintCallable, Category = "XR|Debug")
	static void ShowXRRuntimeDebugWorkspace();

	UFUNCTION(BlueprintCallable, Category = "XR|Debug")
	static void HideXRRuntimeDebugWorkspace();

	UFUNCTION(BlueprintCallable, Category = "XR|Debug")
	static void ToggleXRRuntimeDebugWorkspace();

	UFUNCTION(BlueprintPure, Category = "XR|Debug")
	static bool IsXRRuntimeDebugWorkspaceVisible();
};
