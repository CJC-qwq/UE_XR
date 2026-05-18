#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "XRConfigEditorContext.generated.h"

class AXRSceneTriggerController;
class UXRSceneTriggerConfig;

UCLASS(Transient)
class XRCONFIGEDITOR_API UXRConfigEditorContext : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "XR Config")
	TObjectPtr<AXRSceneTriggerController> SceneTriggerController = nullptr;

	UPROPERTY(EditAnywhere, Category = "XR Config")
	TObjectPtr<UXRSceneTriggerConfig> TriggerConfig = nullptr;
};
