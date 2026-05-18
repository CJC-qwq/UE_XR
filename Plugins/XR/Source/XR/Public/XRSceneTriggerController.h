// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OSCAddress.h"
#include "OSCMessage.h"
#include "XRSceneTriggerController.generated.h"

class ALevelSequenceActor;
class ULevelSequencePlayer;
class ULevelStreaming;
class UOSCServer;
class UXRSceneTriggerConfig;

struct FXRBackgroundLevelDefinition;
struct FXREffectLevelDefinition;
struct FXRGenericActionDefinition;
struct FXRTriggerActionEntry;

UCLASS(BlueprintType, Blueprintable)
class XR_API AXRSceneTriggerController : public AActor
{
	GENERATED_BODY()

public:
	AXRSceneTriggerController();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR")
	TObjectPtr<UXRSceneTriggerConfig> TriggerConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR|OSC")
	bool bEnableOSCReceiver = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR|OSC", meta = (ClampMin = "1", ClampMax = "65535", EditCondition = "bEnableOSCReceiver"))
	int32 OSCReceivePort = 7000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR|OSC", meta = (EditCondition = "bEnableOSCReceiver"))
	FString OSCServerName = TEXT("XRSceneTriggerOSCServer");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR|OSC", meta = (EditCondition = "bEnableOSCReceiver"))
	FString OSCAddressPath = TEXT("/xr/trigger");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR|OSC", meta = (EditCondition = "bEnableOSCReceiver"))
	bool bLogOSCMessages = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "XR|OSC")
	FString ResolvedLocalIPAddress;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XR|Debug")
	int32 PreviewTriggerId = 0;

	UFUNCTION(BlueprintCallable, Category = "XR")
	bool TriggerInteger(int32 TriggerId);

	UFUNCTION(BlueprintCallable, Category = "XR")
	void StopAllAndReset();

	UFUNCTION(BlueprintPure, Category = "XR")
	FName GetActiveBackgroundId() const;

	UFUNCTION(BlueprintPure, Category = "XR")
	TArray<FName> GetActiveEffectIds() const;

	UFUNCTION(BlueprintPure, Category = "XR")
	TArray<FName> GetActiveGenericActionIds() const;

	UFUNCTION(BlueprintPure, Category = "XR|OSC")
	bool IsOSCReceiverRunning() const;

	UFUNCTION(CallInEditor, Category = "XR|Debug")
	void PreviewTrigger();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleOSCMessageReceived(const FOSCMessage& Message, const FString& IPAddress, int32 Port);

private:
	bool ActivateBackground(const FXRBackgroundLevelDefinition& BackgroundDefinition, bool bForceReload);
	bool TriggerEffectLevel(const FXREffectLevelDefinition& EffectDefinition);
	bool TriggerGenericAction(const FXRGenericActionDefinition& ActionDefinition);
	bool ExecuteGenericAction(const FXRGenericActionDefinition& ActionDefinition, FName ActionName);
	bool ExecutePersistentLevelScriptAction(FName ActionName);
	bool ExecuteActorAction(FName TargetActorTag, FName ActionName);
	bool IsLevelAssetCurrentlyActive(const TSoftObjectPtr<UWorld>& LevelAsset) const;
	bool AreAnyOtherBackgroundLevelsActive(const FName BackgroundId) const;
	void UnloadBackgroundLevelsExcept(const FName BackgroundId);
	void StopAllGenericEffects();
	void UnloadAllEffectLevels();
	void UnloadBackgroundLevel();
	void UnloadEffectLevel(const FName EffectId);
	bool LoadStreamingLevelByName(const TSoftObjectPtr<UWorld>& LevelAsset);
	bool UnloadStreamingLevelByName(const TSoftObjectPtr<UWorld>& LevelAsset);
	ULevelStreaming* FindStreamingLevel(const TSoftObjectPtr<UWorld>& LevelAsset) const;
	bool IsEffectAllowedForActiveBackground(const FXREffectLevelDefinition& EffectDefinition) const;
	void PlayConfiguredSequence(const TSoftObjectPtr<class ULevelSequence>& SequenceAsset, const FName PlaybackId, bool bTrackForAutoStop);
	void StopPlayback(const FName PlaybackId);
	void StopAllTrackedSequencePlaybacks();
	void QueueSequencePlaybackAfterLevelLoad(TSoftObjectPtr<UWorld> LevelAsset, TSoftObjectPtr<class ULevelSequence> SequenceAsset, FName PlaybackId, bool bTrackForAutoStop, int32 RetryCount);
	void InspectEffectLevelPlayback(const FName EffectId, int32 RetryCount);
	float ResolveAutoUnloadDelay(const FXREffectLevelDefinition& EffectDefinition) const;
	bool IsGenericActionAllowedForActiveBackground(const FXRGenericActionDefinition& ActionDefinition) const;
	void InitializeOSCReceiver();
	void ShutdownOSCReceiver();
	FString ResolveCurrentLocalIPAddress() const;
	FName GetLevelPackageName(const TSoftObjectPtr<UWorld>& LevelAsset) const;
	FLatentActionInfo MakeStreamingLatentActionInfo();

	TSet<FName> ActiveGenericActionIds;
	TMap<FName, FTimerHandle> EffectUnloadTimerHandles;
	TSet<FName> ActiveEffectIds;
	TMap<FName, TObjectPtr<ULevelSequencePlayer>> ActiveSequencePlayers;
	TMap<FName, TObjectPtr<ALevelSequenceActor>> ActiveSequenceActors;
	UPROPERTY(Transient)
	TObjectPtr<UOSCServer> OSCServer;
	FName ActiveBackgroundId;
	int32 StreamingLatentActionUUID = 1;
};
