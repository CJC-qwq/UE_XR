// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "XRGenericActionReceiver.generated.h"

UINTERFACE(BlueprintType)
class XR_API UXRGenericActionReceiver : public UInterface
{
	GENERATED_BODY()
};

class XR_API IXRGenericActionReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "XR")
	void HandleXRGenericAction(FName ActionName);
};
