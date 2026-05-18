// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "UObject/PropertyText.h"
#include "XRSceneTriggerConfig.generated.h"

class ULevelSequence;
class UWorld;

UENUM(BlueprintType)
enum class EXRGenericActionTargetType : uint8
{
	Actor UMETA(DisplayName = "Actor"),
	PersistentLevelScript UMETA(DisplayName = "Persistent Level Blueprint")
};

USTRUCT(BlueprintType)
struct XR_API FXRBackgroundLevelDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (DisplayName = "Background Label"))
	FString BackgroundLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	FName BackgroundId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> LevelAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR|Sequence", meta = (AllowedClasses = "/Script/LevelSequence.LevelSequence"))
	TSoftObjectPtr<ULevelSequence> SequenceAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	bool bReloadWhenTriggeredAgain = true;
};

USTRUCT(BlueprintType)
struct XR_API FXREffectLevelDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (DisplayName = "Effect Label"))
	FString EffectLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	FName EffectId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> LevelAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR|Sequence", meta = (AllowedClasses = "/Script/LevelSequence.LevelSequence"))
	TSoftObjectPtr<ULevelSequence> SequenceAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	TArray<FName> AllowedBackgroundIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	bool bReloadWhenTriggeredAgain = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	bool bAutoUnloadWhenSequenceCompletes = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (ClampMin = "0.0"))
	float FallbackAutoUnloadDelay = 0.0f;
};

USTRUCT(BlueprintType)
struct XR_API FXRTriggerActionEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (DisplayName = "Trigger Label"))
	FString TriggerLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	int32 TriggerId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (GetOptions = "GetBackgroundLevelOptions"))
	FName BackgroundToActivate = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	bool bForceBackgroundReload = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (GetOptions = "GetEffectLevelOptions"))
	TArray<FName> EffectLevelsToTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (DisplayName = "Generic Actions To Trigger", GetOptions = "GetGenericActionOptions"))
	TArray<FName> GenericEffectsToTrigger;
};

USTRUCT(BlueprintType)
struct XR_API FXRGenericActionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (DisplayName = "Generic Action Label"))
	FString ActionLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (DisplayName = "Generic Action Id"))
	FName ActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	EXRGenericActionTargetType TargetType = EXRGenericActionTargetType::Actor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (DisplayName = "Target Actor Tag", EditCondition = "TargetType == EXRGenericActionTargetType::Actor", EditConditionHides))
	FName TargetActorTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (DisplayName = "Trigger Action Name"))
	FName ActionName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (DisplayName = "Stop Action Name"))
	FName StopActionName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	TArray<FName> AllowedBackgroundIds;
};

UCLASS(BlueprintType)
class XR_API UXRSceneTriggerConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (TitleProperty = "BackgroundLabel"))
	TArray<FXRBackgroundLevelDefinition> BackgroundLevels;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (TitleProperty = "EffectLabel"))
	TArray<FXREffectLevelDefinition> EffectLevels;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (TitleProperty = "ActionLabel"))
	TArray<FXRGenericActionDefinition> GenericActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR", meta = (TitleProperty = "TriggerLabel"))
	TArray<FXRTriggerActionEntry> TriggerActions;

	UFUNCTION()
	TArray<FPropertyTextFName> GetBackgroundLevelOptions() const;

	UFUNCTION()
	TArray<FPropertyTextFName> GetEffectLevelOptions() const;

	UFUNCTION()
	TArray<FPropertyTextFName> GetGenericActionOptions() const;

	const FXRBackgroundLevelDefinition* FindBackgroundLevel(FName BackgroundId) const;
	const FXREffectLevelDefinition* FindEffectLevel(FName EffectId) const;
	const FXRGenericActionDefinition* FindGenericAction(FName ActionId) const;
	const FXRTriggerActionEntry* FindTriggerAction(int32 TriggerId) const;

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif
};
