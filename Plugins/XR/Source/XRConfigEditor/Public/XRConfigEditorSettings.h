#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/SoftObjectPath.h"
#include "XRConfigEditorSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings)
class XRCONFIGEDITOR_API UXRConfigEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Config)
	FString LastContextMapPath;

	UPROPERTY(Config)
	FSoftObjectPath LastSceneTriggerController;

	UPROPERTY(Config)
	FSoftObjectPath LastTriggerConfig;
};
