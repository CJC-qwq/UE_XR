// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "XRGenericEffectComponent.generated.h"

UCLASS(ClassGroup = (XR), meta = (BlueprintSpawnableComponent))
class XR_API UXRGenericEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UXRGenericEffectComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	FName EffectId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	TArray<FName> ControlledComponentTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (ClampMin = "0.0"))
	float AutoStopDelay = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "XR")
	void PlayEffect();

	UFUNCTION(BlueprintCallable, Category = "XR")
	void StopEffect();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void GetControllableComponents(TArray<UActorComponent*>& OutComponents) const;
	bool IsComponentControlled(const UActorComponent* Component) const;
	void ScheduleAutoStop();

	FTimerHandle AutoStopTimerHandle;
};
