// Copyright Epic Games, Inc. All Rights Reserved.

#include "XRSceneTriggerController.h"

#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "IPAddress.h"
#include "Kismet/GameplayStatics.h"
#include "LatentActions.h"
#include "Engine/LevelScriptActor.h"
#include "LevelSequenceActor.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelUtils.h"
#include "Misc/PackageName.h"
#include "MovieScene.h"
#include "OSCManager.h"
#include "OSCServer.h"
#include "SocketSubsystem.h"
#include "TimerManager.h"
#include "XRGenericActionReceiver.h"
#include "XRSceneTriggerConfig.h"

#if WITH_EDITOR
#include "EditorLevelUtils.h"
#endif

namespace XRSceneTriggerController
{
	constexpr int32 MaxInspectRetries = 20;
	constexpr float InspectRetryDelay = 0.15f;
	constexpr float SequenceLoadRetryDelay = 0.10f;

	bool IsEditorWorld(const UWorld* World)
	{
		return World && !World->IsGameWorld();
	}

	bool IsStreamingLevelReadyForUse(const UWorld* World, const ULevelStreaming* StreamingLevel)
	{
		if (!StreamingLevel || !StreamingLevel->GetLoadedLevel())
		{
			return false;
		}

#if WITH_EDITOR
		if (IsEditorWorld(World))
		{
			return StreamingLevel->GetShouldBeVisibleInEditor();
		}
#endif

		return StreamingLevel->IsLevelVisible();
	}

#if WITH_EDITOR
	void ApplyEditorLevelVisibility(UWorld* World, ULevelStreaming* StreamingLevel, const bool bShouldBeVisible)
	{
		if (!IsEditorWorld(World) || !StreamingLevel)
		{
			return;
		}

		if (ULevel* LoadedLevel = StreamingLevel->GetLoadedLevel())
		{
			UEditorLevelUtils::SetLevelVisibility(LoadedLevel, bShouldBeVisible, true);
		}
	}
#endif
}

AXRSceneTriggerController::AXRSceneTriggerController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AXRSceneTriggerController::BeginPlay()
{
	Super::BeginPlay();

	InitializeOSCReceiver();
}

void AXRSceneTriggerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownOSCReceiver();
	Super::EndPlay(EndPlayReason);
}

bool AXRSceneTriggerController::TriggerInteger(const int32 TriggerId)
{
	if (!TriggerConfig)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: TriggerConfig is not assigned."));
		return false;
	}

	const FXRTriggerActionEntry* TriggerAction = TriggerConfig->FindTriggerAction(TriggerId);
	if (!TriggerAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: No trigger mapping found for Id %d."), TriggerId);
		return false;
	}

	bool bDidExecute = false;

	if (TriggerAction->BackgroundToActivate != NAME_None)
	{
		if (const FXRBackgroundLevelDefinition* BackgroundDefinition = TriggerConfig->FindBackgroundLevel(TriggerAction->BackgroundToActivate))
		{
			bDidExecute |= ActivateBackground(*BackgroundDefinition, TriggerAction->bForceBackgroundReload);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: Missing background definition '%s'."), *TriggerAction->BackgroundToActivate.ToString());
		}
	}

	for (const FName EffectId : TriggerAction->EffectLevelsToTrigger)
	{
		if (const FXREffectLevelDefinition* EffectDefinition = TriggerConfig->FindEffectLevel(EffectId))
		{
			bDidExecute |= TriggerEffectLevel(*EffectDefinition);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: Missing effect definition '%s'."), *EffectId.ToString());
		}
	}

	for (const FName GenericActionId : TriggerAction->GenericEffectsToTrigger)
	{
		if (const FXRGenericActionDefinition* ActionDefinition = TriggerConfig->FindGenericAction(GenericActionId))
		{
			bDidExecute |= TriggerGenericAction(*ActionDefinition);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: Missing generic action definition '%s'."), *GenericActionId.ToString());
		}
	}

	return bDidExecute;
}

void AXRSceneTriggerController::StopAllAndReset()
{
	StopAllGenericEffects();
	UnloadAllEffectLevels();
	UnloadBackgroundLevel();
	StopAllTrackedSequencePlaybacks();
}

FName AXRSceneTriggerController::GetActiveBackgroundId() const
{
	return ActiveBackgroundId;
}

TArray<FName> AXRSceneTriggerController::GetActiveEffectIds() const
{
	TArray<FName> Result = ActiveEffectIds.Array();
	Result.Sort(FNameLexicalLess());
	return Result;
}

TArray<FName> AXRSceneTriggerController::GetActiveGenericActionIds() const
{
	TArray<FName> Result = ActiveGenericActionIds.Array();
	Result.Sort(FNameLexicalLess());
	return Result;
}

bool AXRSceneTriggerController::IsOSCReceiverRunning() const
{
	return OSCServer != nullptr;
}

void AXRSceneTriggerController::PreviewTrigger()
{
	TriggerInteger(PreviewTriggerId);
}

void AXRSceneTriggerController::HandleOSCMessageReceived(const FOSCMessage& Message, const FString& IPAddress, const int32 Port)
{
	const FOSCAddress MessageAddress = UOSCManager::GetOSCMessageAddress(Message);
	const FString ReceivedPath = UOSCManager::GetOSCAddressFullPath(MessageAddress);
	if (!OSCAddressPath.IsEmpty() && !ReceivedPath.Equals(OSCAddressPath, ESearchCase::CaseSensitive))
	{
		if (bLogOSCMessages)
		{
			UE_LOG(LogTemp, Display, TEXT("XRSceneTriggerController: Ignored OSC message from %s:%d on path '%s' because it does not match configured path '%s'."),
				*IPAddress,
				Port,
				*ReceivedPath,
				*OSCAddressPath);
		}
		return;
	}

	int32 TriggerId = 0;
	const bool bHasTriggerId = UOSCManager::GetInt32(Message, 0, TriggerId);
	if (!bHasTriggerId)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: OSC message matched '%s' from %s:%d but argument 0 is not an int32."),
			*ReceivedPath,
			*IPAddress,
			Port);
		return;
	}

	if (bLogOSCMessages)
	{
		UE_LOG(LogTemp, Display, TEXT("XRSceneTriggerController: Received OSC trigger %d from %s:%d on path '%s'."),
			TriggerId,
			*IPAddress,
			Port,
			*ReceivedPath);
	}

	TriggerInteger(TriggerId);
}

bool AXRSceneTriggerController::ActivateBackground(const FXRBackgroundLevelDefinition& BackgroundDefinition, const bool bForceReload)
{
	const bool bIsSameBackground = ActiveBackgroundId == BackgroundDefinition.BackgroundId;
	const bool bTargetBackgroundAlreadyActive = IsLevelAssetCurrentlyActive(BackgroundDefinition.LevelAsset);
	const bool bOtherBackgroundsActive = AreAnyOtherBackgroundLevelsActive(BackgroundDefinition.BackgroundId);
	const bool bShouldReload = bForceReload
		|| (!bIsSameBackground)
		|| BackgroundDefinition.bReloadWhenTriggeredAgain
		|| bOtherBackgroundsActive
		|| !bTargetBackgroundAlreadyActive;

	if (!bShouldReload)
	{
		return false;
	}

	StopAllGenericEffects();
	StopAllTrackedSequencePlaybacks();
	UnloadAllEffectLevels();
	UnloadBackgroundLevelsExcept(BackgroundDefinition.BackgroundId);

	if (bIsSameBackground && bTargetBackgroundAlreadyActive && !bForceReload && !BackgroundDefinition.bReloadWhenTriggeredAgain)
	{
		ActiveBackgroundId = BackgroundDefinition.BackgroundId;
		return true;
	}

	UnloadBackgroundLevel();

	const bool bLoaded = LoadStreamingLevelByName(BackgroundDefinition.LevelAsset);
	if (bLoaded)
	{
		ActiveBackgroundId = BackgroundDefinition.BackgroundId;
		if (!BackgroundDefinition.SequenceAsset.IsNull())
		{
			QueueSequencePlaybackAfterLevelLoad(
				BackgroundDefinition.LevelAsset,
				BackgroundDefinition.SequenceAsset,
				BackgroundDefinition.BackgroundId,
				false,
				0);
		}
		return true;
	}

	return false;
}

bool AXRSceneTriggerController::IsLevelAssetCurrentlyActive(const TSoftObjectPtr<UWorld>& LevelAsset) const
{
	UWorld* World = GetWorld();
	ULevelStreaming* StreamingLevel = FindStreamingLevel(LevelAsset);
	if (!World || !StreamingLevel || !StreamingLevel->GetLoadedLevel())
	{
		return false;
	}

#if WITH_EDITOR
	if (XRSceneTriggerController::IsEditorWorld(World))
	{
		return StreamingLevel->GetShouldBeVisibleInEditor();
	}
#endif

	return StreamingLevel->IsLevelVisible();
}

bool AXRSceneTriggerController::AreAnyOtherBackgroundLevelsActive(const FName BackgroundId) const
{
	if (!TriggerConfig)
	{
		return false;
	}

	for (const FXRBackgroundLevelDefinition& Entry : TriggerConfig->BackgroundLevels)
	{
		if (Entry.BackgroundId == NAME_None || Entry.BackgroundId == BackgroundId)
		{
			continue;
		}

		if (IsLevelAssetCurrentlyActive(Entry.LevelAsset))
		{
			return true;
		}
	}

	return false;
}

void AXRSceneTriggerController::UnloadBackgroundLevelsExcept(const FName BackgroundId)
{
	if (!TriggerConfig)
	{
		return;
	}

	for (const FXRBackgroundLevelDefinition& Entry : TriggerConfig->BackgroundLevels)
	{
		if (Entry.BackgroundId == NAME_None || Entry.BackgroundId == BackgroundId)
		{
			continue;
		}

		if (IsLevelAssetCurrentlyActive(Entry.LevelAsset))
		{
			UnloadStreamingLevelByName(Entry.LevelAsset);
			StopPlayback(Entry.BackgroundId);
		}
	}
}

bool AXRSceneTriggerController::TriggerEffectLevel(const FXREffectLevelDefinition& EffectDefinition)
{
	if (!IsEffectAllowedForActiveBackground(EffectDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: Effect '%s' is not allowed for active background '%s'."),
			*EffectDefinition.EffectId.ToString(),
			*ActiveBackgroundId.ToString());
		return false;
	}

	if (ActiveEffectIds.Contains(EffectDefinition.EffectId))
	{
		if (!EffectDefinition.bReloadWhenTriggeredAgain)
		{
			return false;
		}

		UnloadEffectLevel(EffectDefinition.EffectId);
	}

	if (!LoadStreamingLevelByName(EffectDefinition.LevelAsset))
	{
		return false;
	}

	ActiveEffectIds.Add(EffectDefinition.EffectId);

	if (!EffectDefinition.SequenceAsset.IsNull())
	{
		QueueSequencePlaybackAfterLevelLoad(
			EffectDefinition.LevelAsset,
			EffectDefinition.SequenceAsset,
			EffectDefinition.EffectId,
			true,
			0);
	}

	FTimerDelegate InspectDelegate;
	InspectDelegate.BindUObject(this, &AXRSceneTriggerController::InspectEffectLevelPlayback, EffectDefinition.EffectId, 0);
	GetWorldTimerManager().SetTimerForNextTick(InspectDelegate);

	return true;
}

bool AXRSceneTriggerController::TriggerGenericAction(const FXRGenericActionDefinition& ActionDefinition)
{
	if (!IsGenericActionAllowedForActiveBackground(ActionDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: Generic action '%s' is not allowed for active background '%s'."),
			*ActionDefinition.ActionId.ToString(),
			*ActiveBackgroundId.ToString());
		return false;
	}

	const bool bExecuted = ExecuteGenericAction(ActionDefinition, ActionDefinition.ActionName);
	if (bExecuted)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("XRSceneTriggerController: Generic action '%s' dispatch to target type '%s' with action '%s' succeeded."),
			*ActionDefinition.ActionId.ToString(),
			*StaticEnum<EXRGenericActionTargetType>()->GetValueAsString(ActionDefinition.TargetType),
			*ActionDefinition.ActionName.ToString());
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("XRSceneTriggerController: Generic action '%s' dispatch to target type '%s' with action '%s' failed."),
			*ActionDefinition.ActionId.ToString(),
			*StaticEnum<EXRGenericActionTargetType>()->GetValueAsString(ActionDefinition.TargetType),
			*ActionDefinition.ActionName.ToString());
	}

	if (bExecuted)
	{
		ActiveGenericActionIds.Add(ActionDefinition.ActionId);
	}

	return bExecuted;
}

bool AXRSceneTriggerController::ExecuteGenericAction(const FXRGenericActionDefinition& ActionDefinition, const FName ActionName)
{
	if (ActionName == NAME_None)
	{
		return false;
	}

	switch (ActionDefinition.TargetType)
	{
	case EXRGenericActionTargetType::PersistentLevelScript:
		return ExecutePersistentLevelScriptAction(ActionName);

	case EXRGenericActionTargetType::Actor:
	default:
		return ExecuteActorAction(ActionDefinition.TargetActorTag, ActionName);
	}
}

bool AXRSceneTriggerController::ExecutePersistentLevelScriptAction(const FName ActionName)
{
	UWorld* World = GetWorld();
	if (!World || !World->PersistentLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: Persistent level is unavailable for generic action '%s'."), *ActionName.ToString());
		return false;
	}

	ALevelScriptActor* LevelScriptActor = World->PersistentLevel->GetLevelScriptActor();
	if (!IsValid(LevelScriptActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: Persistent LevelScriptActor is invalid for generic action '%s'."), *ActionName.ToString());
		return false;
	}

	if (!LevelScriptActor->GetClass()->ImplementsInterface(UXRGenericActionReceiver::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: Persistent Level Blueprint does not implement XRGenericActionReceiver for action '%s'."), *ActionName.ToString());
		return false;
	}

	IXRGenericActionReceiver::Execute_HandleXRGenericAction(LevelScriptActor, ActionName);
	UE_LOG(LogTemp, Display, TEXT("XRSceneTriggerController: Sent generic action '%s' to Persistent Level Blueprint '%s'."),
		*ActionName.ToString(),
		*GetNameSafe(LevelScriptActor));
	return true;
}

bool AXRSceneTriggerController::ExecuteActorAction(const FName TargetActorTag, const FName ActionName)
{
	if (TargetActorTag == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: TargetActorTag is empty for generic action '%s'."), *ActionName.ToString());
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: World is unavailable for actor generic action '%s'."), *ActionName.ToString());
		return false;
	}

	bool bExecutedAny = false;
	for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if (!IsValid(Actor) || Actor->GetLevel() != World->PersistentLevel)
		{
			continue;
		}

		if (!Actor->ActorHasTag(TargetActorTag) || !Actor->GetClass()->ImplementsInterface(UXRGenericActionReceiver::StaticClass()))
		{
			continue;
		}

		IXRGenericActionReceiver::Execute_HandleXRGenericAction(Actor, ActionName);
		UE_LOG(LogTemp, Display, TEXT("XRSceneTriggerController: Sent generic action '%s' to actor '%s' by tag '%s'."),
			*ActionName.ToString(),
			*GetNameSafe(Actor),
			*TargetActorTag.ToString());
		bExecutedAny = true;
	}

	if (!bExecutedAny)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: No persistent actor with tag '%s' implementing XRGenericActionReceiver handled action '%s'."),
			*TargetActorTag.ToString(),
			*ActionName.ToString());
	}

	return bExecutedAny;
}

void AXRSceneTriggerController::StopAllGenericEffects()
{
	if (!TriggerConfig)
	{
		ActiveGenericActionIds.Reset();
		return;
	}

	const TArray<FName> ActionIdsToStop = ActiveGenericActionIds.Array();
	for (const FName ActionId : ActionIdsToStop)
	{
		const FXRGenericActionDefinition* ActionDefinition = TriggerConfig->FindGenericAction(ActionId);
		if (!ActionDefinition || ActionDefinition->StopActionName == NAME_None)
		{
			continue;
		}

		ExecuteGenericAction(*ActionDefinition, ActionDefinition->StopActionName);
	}

	ActiveGenericActionIds.Reset();
}

void AXRSceneTriggerController::UnloadAllEffectLevels()
{
	TArray<FName> EffectIdsToUnload = ActiveEffectIds.Array();
	for (const FName EffectId : EffectIdsToUnload)
	{
		UnloadEffectLevel(EffectId);
	}
}

void AXRSceneTriggerController::UnloadBackgroundLevel()
{
	if (ActiveBackgroundId == NAME_None || !TriggerConfig)
	{
		ActiveBackgroundId = NAME_None;
		return;
	}

	if (TriggerConfig->FindBackgroundLevel(ActiveBackgroundId))
	{
		UnloadStreamingLevelByName(TriggerConfig->FindBackgroundLevel(ActiveBackgroundId)->LevelAsset);
	}

	StopPlayback(ActiveBackgroundId);
	ActiveBackgroundId = NAME_None;
}

void AXRSceneTriggerController::UnloadEffectLevel(const FName EffectId)
{
	if (FTimerHandle* TimerHandle = EffectUnloadTimerHandles.Find(EffectId))
	{
		GetWorldTimerManager().ClearTimer(*TimerHandle);
		EffectUnloadTimerHandles.Remove(EffectId);
	}

	if (TriggerConfig)
	{
		if (const FXREffectLevelDefinition* EffectDefinition = TriggerConfig->FindEffectLevel(EffectId))
		{
			UnloadStreamingLevelByName(EffectDefinition->LevelAsset);
		}
	}

	StopPlayback(EffectId);
	ActiveEffectIds.Remove(EffectId);
}

bool AXRSceneTriggerController::LoadStreamingLevelByName(const TSoftObjectPtr<UWorld>& LevelAsset)
{
	UWorld* World = GetWorld();
	if (!World || LevelAsset.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: LoadStreamingLevelByName failed for '%s'. World or level asset is invalid."),
			*LevelAsset.ToSoftObjectPath().ToString());
		return false;
	}

	ULevelStreaming* StreamingLevel = FindStreamingLevel(LevelAsset);
	if (!StreamingLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: LoadStreamingLevelByName could not resolve streaming level for '%s'."),
			*LevelAsset.ToSoftObjectPath().ToString());
		return false;
	}

	if (XRSceneTriggerController::IsEditorWorld(World))
	{
		StreamingLevel->SetShouldBeLoaded(true);
		StreamingLevel->SetShouldBeVisible(true);
#if WITH_EDITOR
		StreamingLevel->SetShouldBeVisibleInEditor(true);
		World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
		XRSceneTriggerController::ApplyEditorLevelVisibility(World, StreamingLevel, true);
#else
		World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
#endif
	}
	else
	{
		UGameplayStatics::LoadStreamLevelBySoftObjectPtr(this, LevelAsset, true, true, MakeStreamingLatentActionInfo());
	}

	const int32 bShouldBeVisible = StreamingLevel->GetShouldBeVisibleFlag() ? 1 : 0;
#if WITH_EDITOR
	const int32 bShouldBeVisibleInEditor = StreamingLevel->GetShouldBeVisibleInEditor() ? 1 : 0;
#else
	const int32 bShouldBeVisibleInEditor = 0;
#endif

	UE_LOG(
		LogTemp,
		Display,
		TEXT("XRSceneTriggerController: Requested load for '%s'. LoadedLevel=%s Visible=%d EditorVisible=%d"),
		*LevelAsset.ToSoftObjectPath().ToString(),
		*GetNameSafe(StreamingLevel->GetLoadedLevel()),
		bShouldBeVisible,
		bShouldBeVisibleInEditor
	);
	return true;
}

bool AXRSceneTriggerController::UnloadStreamingLevelByName(const TSoftObjectPtr<UWorld>& LevelAsset)
{
	UWorld* World = GetWorld();
	if (!World || LevelAsset.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: UnloadStreamingLevelByName failed for '%s'. World or level asset is invalid."),
			*LevelAsset.ToSoftObjectPath().ToString());
		return false;
	}

	ULevelStreaming* StreamingLevel = FindStreamingLevel(LevelAsset);
	if (!StreamingLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: UnloadStreamingLevelByName could not resolve streaming level for '%s'."),
			*LevelAsset.ToSoftObjectPath().ToString());
		return false;
	}

	if (XRSceneTriggerController::IsEditorWorld(World))
	{
#if WITH_EDITOR
		XRSceneTriggerController::ApplyEditorLevelVisibility(World, StreamingLevel, false);
		StreamingLevel->SetShouldBeVisibleInEditor(false);
		StreamingLevel->SetShouldBeVisible(false);
		StreamingLevel->SetShouldBeLoaded(false);
		World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
#else
		StreamingLevel->SetShouldBeVisible(false);
		StreamingLevel->SetShouldBeLoaded(false);
		World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
#endif
	}
	else
	{
		UGameplayStatics::UnloadStreamLevelBySoftObjectPtr(this, LevelAsset, MakeStreamingLatentActionInfo(), true);
	}

	UE_LOG(LogTemp, Display, TEXT("XRSceneTriggerController: Requested unload for '%s'."), *LevelAsset.ToSoftObjectPath().ToString());
	return true;
}

ULevelStreaming* AXRSceneTriggerController::FindStreamingLevel(const TSoftObjectPtr<UWorld>& LevelAsset) const
{
	const FName LevelName = GetLevelPackageName(LevelAsset);
	if (LevelName.IsNone())
	{
		return nullptr;
	}

	const FString RequestedLongName = LevelName.ToString();
	const FString RequestedShortName = FPackageName::GetShortName(RequestedLongName);

	if (UWorld* World = GetWorld())
	{
		if (ULevelStreaming* ExactStreamingLevel = FLevelUtils::FindStreamingLevel(World, LevelName))
		{
			return ExactStreamingLevel;
		}

		for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
		{
			if (!StreamingLevel)
			{
				continue;
			}

			const FString CandidateLongName = StreamingLevel->GetWorldAssetPackageName();
			const FString CandidateShortName = FPackageName::GetShortName(CandidateLongName);
			if (CandidateLongName == RequestedLongName || CandidateShortName == RequestedShortName)
			{
				return StreamingLevel;
			}
		}
	}

	return UGameplayStatics::GetStreamingLevel(this, LevelName);
}

bool AXRSceneTriggerController::IsEffectAllowedForActiveBackground(const FXREffectLevelDefinition& EffectDefinition) const
{
	return EffectDefinition.AllowedBackgroundIds.IsEmpty() || EffectDefinition.AllowedBackgroundIds.Contains(ActiveBackgroundId);
}

bool AXRSceneTriggerController::IsGenericActionAllowedForActiveBackground(const FXRGenericActionDefinition& ActionDefinition) const
{
	return ActionDefinition.AllowedBackgroundIds.IsEmpty() || ActionDefinition.AllowedBackgroundIds.Contains(ActiveBackgroundId);
}

void AXRSceneTriggerController::InitializeOSCReceiver()
{
	ShutdownOSCReceiver();

	ResolvedLocalIPAddress = ResolveCurrentLocalIPAddress();
	if (!bEnableOSCReceiver)
	{
		return;
	}

	if (OSCAddressPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: OSC receiver is enabled but OSCAddressPath is empty."));
		return;
	}

	const FString ReceiveIPAddress = ResolvedLocalIPAddress.IsEmpty() ? FString(TEXT("0")) : ResolvedLocalIPAddress;
	const FString ServerName = OSCServerName.IsEmpty() ? GetName() : OSCServerName;
	OSCServer = UOSCManager::CreateOSCServer(ReceiveIPAddress, OSCReceivePort, false, true, ServerName, this);
	if (!OSCServer)
	{
		UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: Failed to create OSC server on %s:%d."), *ReceiveIPAddress, OSCReceivePort);
		return;
	}

	OSCServer->OnOscMessageReceived.AddDynamic(this, &AXRSceneTriggerController::HandleOSCMessageReceived);

	UE_LOG(LogTemp, Display, TEXT("XRSceneTriggerController: OSC receiver listening on %s:%d with address '%s'."),
		*ReceiveIPAddress,
		OSCReceivePort,
		*OSCAddressPath);
}

void AXRSceneTriggerController::ShutdownOSCReceiver()
{
	if (!OSCServer)
	{
		return;
	}

	OSCServer->Stop();
	OSCServer = nullptr;
}

FString AXRSceneTriggerController::ResolveCurrentLocalIPAddress() const
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return TEXT("127.0.0.1");
	}

	bool bCanBindAll = false;
	TSharedRef<FInternetAddr> LocalHostAddress = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBindAll);
	if (!LocalHostAddress->IsValid())
	{
		return TEXT("127.0.0.1");
	}

	return LocalHostAddress->ToString(false);
}

void AXRSceneTriggerController::PlayConfiguredSequence(const TSoftObjectPtr<ULevelSequence>& SequenceAsset, const FName PlaybackId, const bool bTrackForAutoStop)
{
	if (SequenceAsset.IsNull())
	{
		return;
	}

	ULevelSequence* Sequence = SequenceAsset.LoadSynchronous();
	if (!Sequence)
	{
		return;
	}

	StopPlayback(PlaybackId);

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* SequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(this, Sequence, PlaybackSettings, SequenceActor);
	if (!SequencePlayer)
	{
		return;
	}

	ActiveSequencePlayers.FindOrAdd(PlaybackId) = SequencePlayer;
	ActiveSequenceActors.FindOrAdd(PlaybackId) = SequenceActor;
	SequencePlayer->Play();

	if (!bTrackForAutoStop)
	{
		return;
	}
}

void AXRSceneTriggerController::StopPlayback(const FName PlaybackId)
{
	if (TObjectPtr<ULevelSequencePlayer>* PlayerPtr = ActiveSequencePlayers.Find(PlaybackId))
	{
		if (PlayerPtr->Get())
		{
			PlayerPtr->Get()->Stop();
		}
		ActiveSequencePlayers.Remove(PlaybackId);
	}

	if (TObjectPtr<ALevelSequenceActor>* ActorPtr = ActiveSequenceActors.Find(PlaybackId))
	{
		if (ActorPtr->Get())
		{
			ActorPtr->Get()->Destroy();
		}
		ActiveSequenceActors.Remove(PlaybackId);
	}
}

void AXRSceneTriggerController::StopAllTrackedSequencePlaybacks()
{
	TArray<FName> PlaybackIds;
	ActiveSequencePlayers.GetKeys(PlaybackIds);
	for (const FName PlaybackId : PlaybackIds)
	{
		StopPlayback(PlaybackId);
	}
}

void AXRSceneTriggerController::QueueSequencePlaybackAfterLevelLoad(
	TSoftObjectPtr<UWorld> LevelAsset,
	TSoftObjectPtr<ULevelSequence> SequenceAsset,
	FName PlaybackId,
	const bool bTrackForAutoStop,
	const int32 RetryCount)
{
	UWorld* World = GetWorld();
	ULevelStreaming* StreamingLevel = FindStreamingLevel(LevelAsset);
	if (!XRSceneTriggerController::IsStreamingLevelReadyForUse(World, StreamingLevel))
	{
		if (RetryCount >= XRSceneTriggerController::MaxInspectRetries)
		{
			int32 bVisible = 0;
			if (StreamingLevel)
			{
#if WITH_EDITOR
				bVisible = XRSceneTriggerController::IsEditorWorld(World)
					? (StreamingLevel->GetShouldBeVisibleInEditor() ? 1 : 0)
					: (StreamingLevel->IsLevelVisible() ? 1 : 0);
#else
				bVisible = StreamingLevel->IsLevelVisible() ? 1 : 0;
#endif
			}

			UE_LOG(LogTemp, Warning, TEXT("XRSceneTriggerController: Sequence playback wait timed out for '%s'. Loaded=%d Visible=%d"),
				*LevelAsset.ToSoftObjectPath().ToString(),
				StreamingLevel && StreamingLevel->GetLoadedLevel() ? 1 : 0,
				bVisible);
			return;
		}

		FTimerDelegate RetryDelegate;
		RetryDelegate.BindUObject(
			this,
			&AXRSceneTriggerController::QueueSequencePlaybackAfterLevelLoad,
			LevelAsset,
			SequenceAsset,
			PlaybackId,
			bTrackForAutoStop,
			RetryCount + 1);
		FTimerHandle RetryHandle;
		GetWorldTimerManager().SetTimer(RetryHandle, RetryDelegate, XRSceneTriggerController::SequenceLoadRetryDelay, false);
		return;
	}

	PlayConfiguredSequence(SequenceAsset, PlaybackId, bTrackForAutoStop);
}

void AXRSceneTriggerController::InspectEffectLevelPlayback(const FName EffectId, const int32 RetryCount)
{
	if (!TriggerConfig)
	{
		return;
	}

	const FXREffectLevelDefinition* EffectDefinition = TriggerConfig->FindEffectLevel(EffectId);
	if (!EffectDefinition)
	{
		return;
	}

	ULevelStreaming* StreamingLevel = FindStreamingLevel(EffectDefinition->LevelAsset);
	if (!StreamingLevel)
	{
		return;
	}

	if (!XRSceneTriggerController::IsStreamingLevelReadyForUse(GetWorld(), StreamingLevel))
	{
		if (RetryCount >= XRSceneTriggerController::MaxInspectRetries)
		{
			const float FallbackDelay = EffectDefinition->FallbackAutoUnloadDelay;
			if (FallbackDelay > 0.0f)
			{
				FTimerHandle& TimerHandle = EffectUnloadTimerHandles.FindOrAdd(EffectId);
				FTimerDelegate UnloadDelegate;
				UnloadDelegate.BindUObject(this, &AXRSceneTriggerController::UnloadEffectLevel, EffectId);
				GetWorldTimerManager().SetTimer(TimerHandle, UnloadDelegate, FallbackDelay, false);
			}
			return;
		}

		FTimerDelegate RetryDelegate;
		RetryDelegate.BindUObject(this, &AXRSceneTriggerController::InspectEffectLevelPlayback, EffectId, RetryCount + 1);
		FTimerHandle RetryHandle;
		GetWorldTimerManager().SetTimer(RetryHandle, RetryDelegate, XRSceneTriggerController::InspectRetryDelay, false);
		return;
	}

	const float AutoUnloadDelay = ResolveAutoUnloadDelay(*EffectDefinition);
	if (AutoUnloadDelay > 0.0f)
	{
		FTimerHandle& TimerHandle = EffectUnloadTimerHandles.FindOrAdd(EffectId);
		FTimerDelegate UnloadDelegate;
		UnloadDelegate.BindUObject(this, &AXRSceneTriggerController::UnloadEffectLevel, EffectId);
		GetWorldTimerManager().SetTimer(TimerHandle, UnloadDelegate, AutoUnloadDelay, false);
	}
}

float AXRSceneTriggerController::ResolveAutoUnloadDelay(const FXREffectLevelDefinition& EffectDefinition) const
{
	if (!EffectDefinition.bAutoUnloadWhenSequenceCompletes)
	{
		return EffectDefinition.FallbackAutoUnloadDelay;
	}

	ULevelSequence* Sequence = EffectDefinition.SequenceAsset.LoadSynchronous();
	if (!Sequence)
	{
		return EffectDefinition.FallbackAutoUnloadDelay;
	}

	double DurationSeconds = 0.0;
	if (UMovieScene* MovieScene = Sequence->GetMovieScene())
	{
		const FFrameNumber FrameCount = MovieScene->GetPlaybackRange().Size<FFrameNumber>();
		DurationSeconds = MovieScene->GetTickResolution().AsDecimal() > 0.0
			? static_cast<double>(FrameCount.Value) / MovieScene->GetTickResolution().AsDecimal()
			: 0.0;
	}

	if (DurationSeconds > 0.0)
	{
		return static_cast<float>(DurationSeconds);
	}

	return EffectDefinition.FallbackAutoUnloadDelay;
}

FName AXRSceneTriggerController::GetLevelPackageName(const TSoftObjectPtr<UWorld>& LevelAsset) const
{
	const FString LongPackageName = LevelAsset.ToSoftObjectPath().GetLongPackageName();
	return LongPackageName.IsEmpty() ? NAME_None : FName(*LongPackageName);
}

FLatentActionInfo AXRSceneTriggerController::MakeStreamingLatentActionInfo()
{
	FLatentActionInfo LatentActionInfo;
	LatentActionInfo.CallbackTarget = this;
	LatentActionInfo.UUID = StreamingLatentActionUUID++;
	LatentActionInfo.Linkage = 0;
	return LatentActionInfo;
}
