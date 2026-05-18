// Copyright Epic Games, Inc. All Rights Reserved.

#include "XRSceneTriggerConfig.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

namespace
{
	FName MakeLevelEntryId(const TSoftObjectPtr<UWorld>& LevelAsset)
	{
		const FSoftObjectPath AssetPath = LevelAsset.ToSoftObjectPath();
		return AssetPath.IsValid() ? FName(*AssetPath.GetAssetName()) : NAME_None;
	}

	FText MakeEntryDisplayName(const FString& Label, const FName IdValue)
	{
		if (!Label.IsEmpty())
		{
			return FText::FromString(Label);
		}

		if (!IdValue.IsNone())
		{
			return FText::FromName(IdValue);
		}

		return FText::FromString(TEXT("(None)"));
	}
}

TArray<FPropertyTextFName> UXRSceneTriggerConfig::GetBackgroundLevelOptions() const
{
	TArray<FPropertyTextFName> Options;
	Options.Reserve(BackgroundLevels.Num());

	for (const FXRBackgroundLevelDefinition& Entry : BackgroundLevels)
	{
		if (Entry.BackgroundId.IsNone())
		{
			continue;
		}

		FPropertyTextFName& Option = Options.AddDefaulted_GetRef();
		Option.ValueString = Entry.BackgroundId;
		Option.DisplayName = MakeEntryDisplayName(Entry.BackgroundLabel, Entry.BackgroundId);
	}

	return Options;
}

TArray<FPropertyTextFName> UXRSceneTriggerConfig::GetEffectLevelOptions() const
{
	TArray<FPropertyTextFName> Options;
	Options.Reserve(EffectLevels.Num());

	for (const FXREffectLevelDefinition& Entry : EffectLevels)
	{
		if (Entry.EffectId.IsNone())
		{
			continue;
		}

		FPropertyTextFName& Option = Options.AddDefaulted_GetRef();
		Option.ValueString = Entry.EffectId;
		Option.DisplayName = MakeEntryDisplayName(Entry.EffectLabel, Entry.EffectId);
	}

	return Options;
}

TArray<FPropertyTextFName> UXRSceneTriggerConfig::GetGenericActionOptions() const
{
	TArray<FPropertyTextFName> Options;
	Options.Reserve(GenericActions.Num());

	for (const FXRGenericActionDefinition& Entry : GenericActions)
	{
		if (Entry.ActionId.IsNone())
		{
			continue;
		}

		FPropertyTextFName& Option = Options.AddDefaulted_GetRef();
		Option.ValueString = Entry.ActionId;
		Option.DisplayName = MakeEntryDisplayName(Entry.ActionLabel, Entry.ActionId);
	}

	return Options;
}

const FXRBackgroundLevelDefinition* UXRSceneTriggerConfig::FindBackgroundLevel(const FName BackgroundId) const
{
	return BackgroundLevels.FindByPredicate([BackgroundId](const FXRBackgroundLevelDefinition& Entry)
	{
		return Entry.BackgroundId == BackgroundId;
	});
}

const FXREffectLevelDefinition* UXRSceneTriggerConfig::FindEffectLevel(const FName EffectId) const
{
	return EffectLevels.FindByPredicate([EffectId](const FXREffectLevelDefinition& Entry)
	{
		return Entry.EffectId == EffectId;
	});
}

const FXRGenericActionDefinition* UXRSceneTriggerConfig::FindGenericAction(const FName ActionId) const
{
	return GenericActions.FindByPredicate([ActionId](const FXRGenericActionDefinition& Entry)
	{
		return Entry.ActionId == ActionId;
	});
}

const FXRTriggerActionEntry* UXRSceneTriggerConfig::FindTriggerAction(const int32 TriggerId) const
{
	return TriggerActions.FindByPredicate([TriggerId](const FXRTriggerActionEntry& Entry)
	{
		return Entry.TriggerId == TriggerId;
	});
}

#if WITH_EDITOR
void UXRSceneTriggerConfig::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	const FName ChangedPropertyName = PropertyChangedEvent.GetPropertyName();

	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(FXRBackgroundLevelDefinition, LevelAsset))
	{
		const int32 BackgroundEntryIndex = PropertyChangedEvent.GetArrayIndex(TEXT("BackgroundLevels"));
		if (BackgroundLevels.IsValidIndex(BackgroundEntryIndex))
		{
			FXRBackgroundLevelDefinition& Entry = BackgroundLevels[BackgroundEntryIndex];
			Entry.BackgroundId = MakeLevelEntryId(Entry.LevelAsset);
		}

		const int32 EffectEntryIndex = PropertyChangedEvent.GetArrayIndex(TEXT("EffectLevels"));
		if (EffectLevels.IsValidIndex(EffectEntryIndex))
		{
			FXREffectLevelDefinition& Entry = EffectLevels[EffectEntryIndex];
			Entry.EffectId = MakeLevelEntryId(Entry.LevelAsset);
		}
	}

	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}
#endif
