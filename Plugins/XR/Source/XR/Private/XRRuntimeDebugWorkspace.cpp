#include "XRRuntimeDebugWorkspace.h"

#include "XRSceneTriggerConfig.h"
#include "XRSceneTriggerController.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"
#include "Input/Reply.h"
#include "Internationalization/Text.h"
#include "Misc/Optional.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

namespace
{
	const FVector2D WorkspaceWindowSize(980.0f, 760.0f);
	const FVector2D WorkspaceWindowOffset(32.0f, 32.0f);

	template <typename TObjectType>
	FString GetSoftAssetDisplayName(const TSoftObjectPtr<TObjectType>& Asset)
	{
		if (Asset.IsNull())
		{
			return TEXT("(None)");
		}

		const FSoftObjectPath AssetPath = Asset.ToSoftObjectPath();
		const FString AssetName = AssetPath.GetAssetName();
		return AssetName.IsEmpty() ? AssetPath.ToString() : AssetName;
	}

	FString NameOrNone(const FName Value)
	{
		return Value.IsNone() ? TEXT("(None)") : Value.ToString();
	}

	FString BoolToYesNo(const bool bValue)
	{
		return bValue ? TEXT("Yes") : TEXT("No");
	}

	FString LabelOrEmpty(const FString& Label)
	{
		return Label.IsEmpty() ? TEXT("(Empty)") : Label;
	}

	FString LabelOrId(const FString& Label, const FName IdValue, const FString& EmptyFallback = TEXT("(None)"))
	{
		if (!Label.IsEmpty())
		{
			return Label;
		}

		return IdValue.IsNone() ? EmptyFallback : IdValue.ToString();
	}

	FString TriggerLabelOrId(const FXRTriggerActionEntry& Entry)
	{
		return !Entry.TriggerLabel.IsEmpty()
			? Entry.TriggerLabel
			: FString::Printf(TEXT("Trigger %d"), Entry.TriggerId);
	}

	FString ResolveConfiguredTriggerDisplayName(const UXRSceneTriggerConfig* Config, const int32 TriggerId)
	{
		if (Config)
		{
			if (const FXRTriggerActionEntry* Entry = Config->FindTriggerAction(TriggerId))
			{
				return TriggerLabelOrId(*Entry);
			}
		}

		return FString::Printf(TEXT("Trigger %d"), TriggerId);
	}

	FString JoinNames(const TArray<FName>& Values)
	{
		if (Values.IsEmpty())
		{
			return TEXT("(None)");
		}

		TArray<FString> Parts;
		Parts.Reserve(Values.Num());

		for (const FName Value : Values)
		{
			Parts.Add(NameOrNone(Value));
		}

		return FString::Join(Parts, TEXT(", "));
	}

	FString GetMonitorDisplayName(const FMonitorInfo& MonitorInfo)
	{
		if (!MonitorInfo.Name.IsEmpty())
		{
			return MonitorInfo.Name;
		}

		if (!MonitorInfo.ID.IsEmpty())
		{
			return MonitorInfo.ID;
		}

		return TEXT("Monitor");
	}

	FString GetWorldModeLabel(const UWorld* World)
	{
		if (!World)
		{
			return TEXT("Unavailable");
		}

		return World->IsGameWorld() ? TEXT("Runtime") : TEXT("Editor Preview");
	}

	bool RectanglesOverlap(const FPlatformRect& MonitorRect, const FVector2D& WindowPosition, const FVector2D& WindowSize)
	{
		const float WindowLeft = WindowPosition.X;
		const float WindowTop = WindowPosition.Y;
		const float WindowRight = WindowPosition.X + WindowSize.X;
		const float WindowBottom = WindowPosition.Y + WindowSize.Y;

		return WindowRight > MonitorRect.Left
			&& WindowLeft < MonitorRect.Right
			&& WindowBottom > MonitorRect.Top
			&& WindowTop < MonitorRect.Bottom;
	}

	int32 FindMonitorIndexForPoint(const TArray<FMonitorInfo>& Monitors, const FVector2D& ScreenPoint)
	{
		for (int32 MonitorIndex = 0; MonitorIndex < Monitors.Num(); ++MonitorIndex)
		{
			const FPlatformRect& MonitorRect = Monitors[MonitorIndex].DisplayRect;
			if (ScreenPoint.X >= MonitorRect.Left
				&& ScreenPoint.X < MonitorRect.Right
				&& ScreenPoint.Y >= MonitorRect.Top
				&& ScreenPoint.Y < MonitorRect.Bottom)
			{
				return MonitorIndex;
			}
		}

		return INDEX_NONE;
	}

	TArray<int32> ResolveBlockedMonitorIndices(const TArray<FMonitorInfo>& Monitors, const TSharedPtr<SWindow>& GameWindow)
	{
		TArray<int32> Result;
		if (!GameWindow.IsValid())
		{
			return Result;
		}

		const FVector2D WindowPosition = GameWindow->GetPositionInScreen();
		const FVector2D WindowSize = GameWindow->GetSizeInScreen();

		for (int32 MonitorIndex = 0; MonitorIndex < Monitors.Num(); ++MonitorIndex)
		{
			if (RectanglesOverlap(Monitors[MonitorIndex].DisplayRect, WindowPosition, WindowSize))
			{
				Result.Add(MonitorIndex);
			}
		}

		return Result;
	}

	int32 FindFallbackMonitorIndex(const TArray<FMonitorInfo>& Monitors, const TArray<int32>& BlockedMonitorIndices, const FVector2D& CursorPosition)
	{
		int32 BestMonitorIndex = INDEX_NONE;
		double BestDistanceSquared = TNumericLimits<double>::Max();

		for (int32 MonitorIndex = 0; MonitorIndex < Monitors.Num(); ++MonitorIndex)
		{
			if (BlockedMonitorIndices.Contains(MonitorIndex))
			{
				continue;
			}

			const FPlatformRect& MonitorRect = Monitors[MonitorIndex].WorkArea;
			const FVector2D MonitorCenter(
				(MonitorRect.Left + MonitorRect.Right) * 0.5f,
				(MonitorRect.Top + MonitorRect.Bottom) * 0.5f);
			const double DistanceSquared = FVector2D::DistSquared(CursorPosition, MonitorCenter);

			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				BestMonitorIndex = MonitorIndex;
			}
		}

		return BestMonitorIndex;
	}

	struct FXRRuntimeTriggerOption
	{
		int32 TriggerId = 0;
		FString DisplayLabel;
	};
}

class SXRRuntimeDebugWorkspacePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SXRRuntimeDebugWorkspacePanel)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FXRRuntimeDebugWorkspaceManager* InOwnerManager)
	{
		OwnerManager = InOwnerManager;

		ChildSlot
		[
			SNew(SBorder)
			.Padding(10.0f)
			.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("XR Runtime Debug Workspace")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 4.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.AutoWrapText(true)
							.Text(FText::FromString(TEXT("Configuration is read-only here. Runtime trigger controls stay active for live stage debugging.")))
						]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(6.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Refresh")))
						.OnClicked(this, &SXRRuntimeDebugWorkspacePanel::HandleRefreshClicked)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(6.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Hide")))
						.OnClicked(this, &SXRRuntimeDebugWorkspacePanel::HandleHideClicked)
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SBorder)
					.Padding(8.0f)
					.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.AutoWrapText(true)
							.Text(this, &SXRRuntimeDebugWorkspacePanel::GetControllerSummaryText)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 6.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.AutoWrapText(true)
							.Text(this, &SXRRuntimeDebugWorkspacePanel::GetOSCStatusText)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 6.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.AutoWrapText(true)
							.Text(this, &SXRRuntimeDebugWorkspacePanel::GetActiveStateText)
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SBorder)
					.Padding(8.0f)
					.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Runtime Controls")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Trigger Label")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							]

							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								SAssignNew(TriggerSelectionComboBox, SComboBox<TSharedPtr<FXRRuntimeTriggerOption>>)
								.OptionsSource(&TriggerOptions)
								.OnGenerateWidget(this, &SXRRuntimeDebugWorkspacePanel::GenerateTriggerOptionWidget)
								.OnSelectionChanged(this, &SXRRuntimeDebugWorkspacePanel::HandleTriggerOptionChanged)
								[
									SNew(STextBlock)
									.Text(this, &SXRRuntimeDebugWorkspacePanel::GetSelectedTriggerOptionText)
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("Run Trigger")))
								.IsEnabled(this, &SXRRuntimeDebugWorkspacePanel::HasSelectedTriggerOption)
								.OnClicked(this, &SXRRuntimeDebugWorkspacePanel::HandleRunTriggerClicked)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("Run Last Trigger")))
								.IsEnabled(this, &SXRRuntimeDebugWorkspacePanel::CanRunLastTrigger)
								.OnClicked(this, &SXRRuntimeDebugWorkspacePanel::HandleRunLastTriggerClicked)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("Stop All And Reset")))
								.OnClicked(this, &SXRRuntimeDebugWorkspacePanel::HandleStopAllClicked)
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.AutoWrapText(true)
							.Text(this, &SXRRuntimeDebugWorkspacePanel::GetLastActionStatusText)
						]
					]
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SScrollBox)

					+ SScrollBox::Slot()
					[
						SAssignNew(BackgroundSectionBox, SVerticalBox)
					]

					+ SScrollBox::Slot()
					[
						SAssignNew(EffectSectionBox, SVerticalBox)
					]

					+ SScrollBox::Slot()
					[
						SAssignNew(GenericActionSectionBox, SVerticalBox)
					]

					+ SScrollBox::Slot()
					[
						SAssignNew(TriggerSectionBox, SVerticalBox)
					]
				]
			]
		];

		RefreshData();
	}

	void RefreshData()
	{
		RefreshTriggerOptions();
		RebuildSections();
	}

private:
	void RebuildSections()
	{
		RebuildBackgroundSection();
		RebuildEffectSection();
		RebuildGenericActionSection();
		RebuildTriggerSection();
	}

	void ResetSectionBox(const TSharedPtr<SVerticalBox>& SectionBox)
	{
		if (SectionBox.IsValid())
		{
			SectionBox->ClearChildren();
		}
	}

	const UXRSceneTriggerConfig* GetActiveConfig() const
	{
		const AXRSceneTriggerController* Controller = OwnerManager ? OwnerManager->GetActiveController() : nullptr;
		return Controller ? Controller->TriggerConfig : nullptr;
	}

	FString ResolveBackgroundDisplayName(const UXRSceneTriggerConfig* Config, const FName BackgroundId) const
	{
		if (BackgroundId.IsNone())
		{
			return TEXT("(None)");
		}

		if (Config)
		{
			if (const FXRBackgroundLevelDefinition* Entry = Config->FindBackgroundLevel(BackgroundId))
			{
				return LabelOrId(Entry->BackgroundLabel, Entry->BackgroundId);
			}
		}

		return BackgroundId.ToString();
	}

	FString ResolveEffectDisplayName(const UXRSceneTriggerConfig* Config, const FName EffectId) const
	{
		if (EffectId.IsNone())
		{
			return TEXT("(None)");
		}

		if (Config)
		{
			if (const FXREffectLevelDefinition* Entry = Config->FindEffectLevel(EffectId))
			{
				return LabelOrId(Entry->EffectLabel, Entry->EffectId);
			}
		}

		return EffectId.ToString();
	}

	FString ResolveGenericActionDisplayName(const UXRSceneTriggerConfig* Config, const FName ActionId) const
	{
		if (ActionId.IsNone())
		{
			return TEXT("(None)");
		}

		if (Config)
		{
			if (const FXRGenericActionDefinition* Entry = Config->FindGenericAction(ActionId))
			{
				return LabelOrId(Entry->ActionLabel, Entry->ActionId);
			}
		}

		return ActionId.ToString();
	}

	FString JoinBackgroundDisplayNames(const UXRSceneTriggerConfig* Config, const TArray<FName>& BackgroundIds) const
	{
		if (BackgroundIds.IsEmpty())
		{
			return TEXT("(None)");
		}

		TArray<FString> Parts;
		Parts.Reserve(BackgroundIds.Num());
		for (const FName BackgroundId : BackgroundIds)
		{
			Parts.Add(ResolveBackgroundDisplayName(Config, BackgroundId));
		}

		return FString::Join(Parts, TEXT(", "));
	}

	FString JoinEffectDisplayNames(const UXRSceneTriggerConfig* Config, const TArray<FName>& EffectIds) const
	{
		if (EffectIds.IsEmpty())
		{
			return TEXT("(None)");
		}

		TArray<FString> Parts;
		Parts.Reserve(EffectIds.Num());
		for (const FName EffectId : EffectIds)
		{
			Parts.Add(ResolveEffectDisplayName(Config, EffectId));
		}

		return FString::Join(Parts, TEXT(", "));
	}

	FString JoinGenericActionDisplayNames(const UXRSceneTriggerConfig* Config, const TArray<FName>& ActionIds) const
	{
		if (ActionIds.IsEmpty())
		{
			return TEXT("(None)");
		}

		TArray<FString> Parts;
		Parts.Reserve(ActionIds.Num());
		for (const FName ActionId : ActionIds)
		{
			Parts.Add(ResolveGenericActionDisplayName(Config, ActionId));
		}

		return FString::Join(Parts, TEXT(", "));
	}

	TSharedRef<SWidget> MakeReadonlyMessageRow(const FString& Description) const
	{
		return SNew(SBorder)
			.Padding(6.0f)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(FText::FromString(Description))
			];
	}

	TSharedRef<SWidget> MakeReadonlyFieldRow(const FString& FieldLabel, const FString& FieldValue) const
	{
		return SNew(SBorder)
			.Padding(FMargin(2.0f, 3.0f))
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FieldLabel))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(FText::FromString(FieldValue))
				]
			];
	}

	TSharedRef<SWidget> MakeReadonlyEntryArea(const FString& EntryTitle, const TSharedRef<SWidget>& BodyContent, const bool bInitiallyCollapsed) const
	{
		return SNew(SBorder)
			.Padding(0.0f)
			.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SExpandableArea)
				.AreaTitle(FText::FromString(EntryTitle))
				.InitiallyCollapsed(bInitiallyCollapsed)
				.BodyContent()
				[
					SNew(SBorder)
					.Padding(8.0f)
					.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
					[
						BodyContent
					]
				]
			];
	}

	TSharedRef<SWidget> WrapSectionEntryList(const TSharedRef<SVerticalBox>& Entries, const float MaxHeight) const
	{
		return SNew(SBox)
			.MaxDesiredHeight(MaxHeight)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					Entries
				]
			];
	}

	TSharedRef<SWidget> BuildBackgroundSectionBody(const UXRSceneTriggerConfig* Config) const
	{
		TSharedRef<SVerticalBox> Entries = SNew(SVerticalBox);

		if (!Config || Config->BackgroundLevels.IsEmpty())
		{
			Entries->AddSlot()
			.AutoHeight()
			[
				MakeReadonlyMessageRow(TEXT("No background levels configured."))
			];
			return WrapSectionEntryList(Entries, 220.0f);
		}

		for (const FXRBackgroundLevelDefinition& Entry : Config->BackgroundLevels)
		{
			TSharedRef<SVerticalBox> EntryBody = SNew(SVerticalBox);
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Background Label"), LabelOrEmpty(Entry.BackgroundLabel))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Background Id"), NameOrNone(Entry.BackgroundId))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Level Asset"), GetSoftAssetDisplayName(Entry.LevelAsset))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Sequence Asset"), GetSoftAssetDisplayName(Entry.SequenceAsset))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Reload On Retrigger"), BoolToYesNo(Entry.bReloadWhenTriggeredAgain))];

			Entries->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeReadonlyEntryArea(LabelOrId(Entry.BackgroundLabel, Entry.BackgroundId, TEXT("Unnamed Background")), EntryBody, true)
			];
		}

		return WrapSectionEntryList(Entries, 240.0f);
	}

	void RebuildBackgroundSection()
	{
		ResetSectionBox(BackgroundSectionBox);

		const UXRSceneTriggerConfig* Config = GetActiveConfig();
		if (!BackgroundSectionBox.IsValid())
		{
			return;
		}

		BackgroundSectionBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SExpandableArea)
			.AreaTitle(FText::FromString(FString::Printf(TEXT("Background Levels (%d)"), Config ? Config->BackgroundLevels.Num() : 0)))
			.InitiallyCollapsed(false)
			.BodyContent()
			[
				BuildBackgroundSectionBody(Config)
			]
		];
	}

	TSharedRef<SWidget> BuildEffectSectionBody(const UXRSceneTriggerConfig* Config) const
	{
		TSharedRef<SVerticalBox> Entries = SNew(SVerticalBox);

		if (!Config || Config->EffectLevels.IsEmpty())
		{
			Entries->AddSlot()
			.AutoHeight()
			[
				MakeReadonlyMessageRow(TEXT("No effect levels configured."))
			];
			return WrapSectionEntryList(Entries, 220.0f);
		}

		for (const FXREffectLevelDefinition& Entry : Config->EffectLevels)
		{
			TSharedRef<SVerticalBox> EntryBody = SNew(SVerticalBox);
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Effect Label"), LabelOrEmpty(Entry.EffectLabel))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Effect Id"), NameOrNone(Entry.EffectId))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Level Asset"), GetSoftAssetDisplayName(Entry.LevelAsset))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Sequence Asset"), GetSoftAssetDisplayName(Entry.SequenceAsset))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Allowed Backgrounds"), JoinBackgroundDisplayNames(Config, Entry.AllowedBackgroundIds))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Reload On Retrigger"), BoolToYesNo(Entry.bReloadWhenTriggeredAgain))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Auto Unload"), BoolToYesNo(Entry.bAutoUnloadWhenSequenceCompletes))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Fallback Delay"), FString::Printf(TEXT("%.2fs"), Entry.FallbackAutoUnloadDelay))];

			Entries->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeReadonlyEntryArea(LabelOrId(Entry.EffectLabel, Entry.EffectId, TEXT("Unnamed Effect")), EntryBody, true)
			];
		}

		return WrapSectionEntryList(Entries, 240.0f);
	}

	void RebuildEffectSection()
	{
		ResetSectionBox(EffectSectionBox);

		const UXRSceneTriggerConfig* Config = GetActiveConfig();
		if (!EffectSectionBox.IsValid())
		{
			return;
		}

		EffectSectionBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SExpandableArea)
			.AreaTitle(FText::FromString(FString::Printf(TEXT("Effect Levels (%d)"), Config ? Config->EffectLevels.Num() : 0)))
			.InitiallyCollapsed(true)
			.BodyContent()
			[
				BuildEffectSectionBody(Config)
			]
		];
	}

	TSharedRef<SWidget> BuildGenericActionSectionBody(const UXRSceneTriggerConfig* Config) const
	{
		TSharedRef<SVerticalBox> Entries = SNew(SVerticalBox);

		if (!Config || Config->GenericActions.IsEmpty())
		{
			Entries->AddSlot()
			.AutoHeight()
			[
				MakeReadonlyMessageRow(TEXT("No generic actions configured."))
			];
			return WrapSectionEntryList(Entries, 220.0f);
		}

		for (const FXRGenericActionDefinition& Entry : Config->GenericActions)
		{
			const FString TargetSummary = Entry.TargetType == EXRGenericActionTargetType::Actor
				? FString::Printf(TEXT("Actor Tag: %s"), *NameOrNone(Entry.TargetActorTag))
				: TEXT("Persistent Level Blueprint");
			TSharedRef<SVerticalBox> EntryBody = SNew(SVerticalBox);
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Generic Action Label"), LabelOrEmpty(Entry.ActionLabel))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Generic Action Id"), NameOrNone(Entry.ActionId))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Target"), TargetSummary)];
			if (Entry.TargetType == EXRGenericActionTargetType::Actor)
			{
				EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Target Actor Tag"), NameOrNone(Entry.TargetActorTag))];
			}
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Trigger Action Name"), NameOrNone(Entry.ActionName))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Stop Action Name"), NameOrNone(Entry.StopActionName))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Allowed Backgrounds"), JoinBackgroundDisplayNames(Config, Entry.AllowedBackgroundIds))];

			Entries->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeReadonlyEntryArea(LabelOrId(Entry.ActionLabel, Entry.ActionId, TEXT("Unnamed Generic Action")), EntryBody, true)
			];
		}

		return WrapSectionEntryList(Entries, 240.0f);
	}

	void RebuildGenericActionSection()
	{
		ResetSectionBox(GenericActionSectionBox);

		const UXRSceneTriggerConfig* Config = GetActiveConfig();
		if (!GenericActionSectionBox.IsValid())
		{
			return;
		}

		GenericActionSectionBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SExpandableArea)
			.AreaTitle(FText::FromString(FString::Printf(TEXT("Generic Actions (%d)"), Config ? Config->GenericActions.Num() : 0)))
			.InitiallyCollapsed(true)
			.BodyContent()
			[
				BuildGenericActionSectionBody(Config)
			]
		];
	}

	TSharedRef<SWidget> BuildTriggerSectionBody(const UXRSceneTriggerConfig* Config)
	{
		TSharedRef<SVerticalBox> Entries = SNew(SVerticalBox);

		if (!Config || Config->TriggerActions.IsEmpty())
		{
			Entries->AddSlot()
			.AutoHeight()
			[
				MakeReadonlyMessageRow(TEXT("No trigger actions configured."))
			];
			return WrapSectionEntryList(Entries, 260.0f);
		}

		for (const FXRTriggerActionEntry& Entry : Config->TriggerActions)
		{
			TSharedRef<SVerticalBox> EntryBody = SNew(SVerticalBox);
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Trigger Label"), LabelOrEmpty(Entry.TriggerLabel))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Trigger Id"), FString::FromInt(Entry.TriggerId))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Background To Activate"), ResolveBackgroundDisplayName(Config, Entry.BackgroundToActivate))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Force Background Reload"), BoolToYesNo(Entry.bForceBackgroundReload))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Effect Levels To Trigger"), JoinEffectDisplayNames(Config, Entry.EffectLevelsToTrigger))];
			EntryBody->AddSlot().AutoHeight()[MakeReadonlyFieldRow(TEXT("Generic Actions To Trigger"), JoinGenericActionDisplayNames(Config, Entry.GenericEffectsToTrigger))];
			EntryBody->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Run This Trigger")))
				.OnClicked(this, &SXRRuntimeDebugWorkspacePanel::HandleRunSpecificTriggerClicked, Entry.TriggerId)
			];

			Entries->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeReadonlyEntryArea(TriggerLabelOrId(Entry), EntryBody, true)
			];
		}

		return WrapSectionEntryList(Entries, 300.0f);
	}

	void RebuildTriggerSection()
	{
		ResetSectionBox(TriggerSectionBox);

		const UXRSceneTriggerConfig* Config = GetActiveConfig();
		if (!TriggerSectionBox.IsValid())
		{
			return;
		}

		TriggerSectionBox->AddSlot()
		.AutoHeight()
		[
			SNew(SExpandableArea)
			.AreaTitle(FText::FromString(FString::Printf(TEXT("Trigger Actions (%d)"), Config ? Config->TriggerActions.Num() : 0)))
			.InitiallyCollapsed(false)
			.BodyContent()
			[
				BuildTriggerSectionBody(Config)
			]
		];
	}

	void RefreshTriggerOptions()
	{
		const UXRSceneTriggerConfig* Config = GetActiveConfig();
		const int32 PreviousSelectedTriggerId = SelectedTriggerOption.IsValid()
			? SelectedTriggerOption->TriggerId
			: (LastExecutedTriggerId.IsSet() ? LastExecutedTriggerId.GetValue() : INDEX_NONE);

		TriggerOptions.Reset();
		SelectedTriggerOption.Reset();

		if (Config)
		{
			for (const FXRTriggerActionEntry& Entry : Config->TriggerActions)
			{
				TSharedPtr<FXRRuntimeTriggerOption> Option = MakeShared<FXRRuntimeTriggerOption>();
				Option->TriggerId = Entry.TriggerId;
				Option->DisplayLabel = TriggerLabelOrId(Entry);
				TriggerOptions.Add(Option);
			}
		}

		if (TriggerSelectionComboBox.IsValid())
		{
			TriggerSelectionComboBox->RefreshOptions();
		}

		if (PreviousSelectedTriggerId != INDEX_NONE)
		{
			SelectTriggerOptionById(PreviousSelectedTriggerId);
		}

		if (!SelectedTriggerOption.IsValid() && !TriggerOptions.IsEmpty())
		{
			SelectedTriggerOption = TriggerOptions[0];
			if (TriggerSelectionComboBox.IsValid())
			{
				TriggerSelectionComboBox->SetSelectedItem(SelectedTriggerOption);
			}
		}
	}

	void SelectTriggerOptionById(const int32 TriggerId)
	{
		for (const TSharedPtr<FXRRuntimeTriggerOption>& Option : TriggerOptions)
		{
			if (Option.IsValid() && Option->TriggerId == TriggerId)
			{
				SelectedTriggerOption = Option;
				if (TriggerSelectionComboBox.IsValid())
				{
					TriggerSelectionComboBox->SetSelectedItem(Option);
				}
				return;
			}
		}
	}

	TSharedRef<SWidget> GenerateTriggerOptionWidget(TSharedPtr<FXRRuntimeTriggerOption> Option) const
	{
		const FString DisplayLabel = Option.IsValid() ? Option->DisplayLabel : TEXT("(Invalid Trigger)");
		return SNew(STextBlock)
			.Text(FText::FromString(DisplayLabel));
	}

	void HandleTriggerOptionChanged(TSharedPtr<FXRRuntimeTriggerOption> NewSelection, ESelectInfo::Type SelectInfo)
	{
		SelectedTriggerOption = NewSelection;
	}

	FText GetSelectedTriggerOptionText() const
	{
		return SelectedTriggerOption.IsValid()
			? FText::FromString(SelectedTriggerOption->DisplayLabel)
			: FText::FromString(TEXT("Select Trigger Label"));
	}

	bool HasSelectedTriggerOption() const
	{
		return SelectedTriggerOption.IsValid();
	}

	bool CanRunLastTrigger() const
	{
		return LastExecutedTriggerId.IsSet();
	}

	FReply HandleRunTriggerClicked()
	{
		if (!SelectedTriggerOption.IsValid())
		{
			LastActionStatus = FText::FromString(TEXT("Choose a trigger label before running."));
			return FReply::Handled();
		}

		return ExecuteTriggerAndReport(SelectedTriggerOption->TriggerId);
	}

	FReply HandleRunSpecificTriggerClicked(const int32 TriggerId)
	{
		SelectTriggerOptionById(TriggerId);
		return ExecuteTriggerAndReport(TriggerId);
	}

	FReply HandleRunLastTriggerClicked()
	{
		if (!LastExecutedTriggerId.IsSet())
		{
			LastActionStatus = FText::FromString(TEXT("No previous trigger has been run yet."));
			return FReply::Handled();
		}

		SelectTriggerOptionById(LastExecutedTriggerId.GetValue());
		return ExecuteTriggerAndReport(LastExecutedTriggerId.GetValue());
	}

	FReply ExecuteTriggerAndReport(const int32 TriggerId)
	{
		if (!OwnerManager)
		{
			LastActionStatus = FText::FromString(TEXT("Runtime debug workspace manager is unavailable."));
			return FReply::Handled();
		}

		FText ResultMessage;
		const bool bDidExecute = OwnerManager->ExecuteTrigger(TriggerId, ResultMessage);
		LastActionStatus = ResultMessage;

		if (bDidExecute)
		{
			LastExecutedTriggerId = TriggerId;
		}

		return FReply::Handled();
	}

	FReply HandleStopAllClicked()
	{
		if (!OwnerManager)
		{
			LastActionStatus = FText::FromString(TEXT("Runtime debug workspace manager is unavailable."));
			return FReply::Handled();
		}

		FText ResultMessage;
		OwnerManager->StopAll(ResultMessage);
		LastActionStatus = ResultMessage;
		return FReply::Handled();
	}

	FReply HandleRefreshClicked()
	{
		RefreshData();
		LastActionStatus = FText::FromString(TEXT("Workspace data refreshed."));
		return FReply::Handled();
	}

	FReply HandleHideClicked()
	{
		if (OwnerManager)
		{
			OwnerManager->HideWorkspace();
		}

		return FReply::Handled();
	}

	FText GetLastActionStatusText() const
	{
		return LastActionStatus.IsEmpty()
			? FText::FromString(TEXT("Ready."))
			: LastActionStatus;
	}

	FText GetControllerSummaryText() const
	{
		const AXRSceneTriggerController* Controller = OwnerManager ? OwnerManager->GetActiveController() : nullptr;
		if (!Controller)
		{
			return FText::FromString(TEXT("Controller: not found in the active runtime or editor world."));
		}

		const FString ConfigPath = Controller->TriggerConfig ? Controller->TriggerConfig->GetPathName() : TEXT("(None)");
		const FString Summary = FString::Printf(
			TEXT("Controller: %s | Mode: %s | Config Asset: %s"),
			*Controller->GetName(),
			*GetWorldModeLabel(Controller->GetWorld()),
			*ConfigPath);
		return FText::FromString(Summary);
	}

	FText GetOSCStatusText() const
	{
		const AXRSceneTriggerController* Controller = OwnerManager ? OwnerManager->GetActiveController() : nullptr;
		if (!Controller)
		{
			return FText::FromString(TEXT("OSC: unavailable because no runtime or editor controller is active."));
		}

		const FString Summary = FString::Printf(
			TEXT("OSC: %s | Running: %s | Port: %d | Path: %s | Resolved IP: %s"),
			Controller->bEnableOSCReceiver ? TEXT("Enabled") : TEXT("Disabled"),
			Controller->IsOSCReceiverRunning() ? TEXT("Yes") : TEXT("No"),
			Controller->OSCReceivePort,
			Controller->OSCAddressPath.IsEmpty() ? TEXT("(Empty)") : *Controller->OSCAddressPath,
			Controller->ResolvedLocalIPAddress.IsEmpty() ? TEXT("(Unknown)") : *Controller->ResolvedLocalIPAddress);
		return FText::FromString(Summary);
	}

	FText GetActiveStateText() const
	{
		const AXRSceneTriggerController* Controller = OwnerManager ? OwnerManager->GetActiveController() : nullptr;
		if (!Controller)
		{
			return FText::FromString(TEXT("Active State: unavailable because no runtime or editor controller is active."));
		}

		const UXRSceneTriggerConfig* Config = Controller->TriggerConfig;
		const FString Summary = FString::Printf(
			TEXT("Active Background: %s | Active Effects: %s | Active Generic Actions: %s"),
			*ResolveBackgroundDisplayName(Config, Controller->GetActiveBackgroundId()),
			*JoinEffectDisplayNames(Config, Controller->GetActiveEffectIds()),
			*JoinGenericActionDisplayNames(Config, Controller->GetActiveGenericActionIds()));
		return FText::FromString(Summary);
	}

	FXRRuntimeDebugWorkspaceManager* OwnerManager = nullptr;
	TOptional<int32> LastExecutedTriggerId;
	FText LastActionStatus;
	TArray<TSharedPtr<FXRRuntimeTriggerOption>> TriggerOptions;
	TSharedPtr<FXRRuntimeTriggerOption> SelectedTriggerOption;
	TSharedPtr<SComboBox<TSharedPtr<FXRRuntimeTriggerOption>>> TriggerSelectionComboBox;
	TSharedPtr<SVerticalBox> BackgroundSectionBox;
	TSharedPtr<SVerticalBox> EffectSectionBox;
	TSharedPtr<SVerticalBox> GenericActionSectionBox;
	TSharedPtr<SVerticalBox> TriggerSectionBox;
};

FXRRuntimeDebugWorkspaceManager::FXRRuntimeDebugWorkspaceManager()
{
}

FXRRuntimeDebugWorkspaceManager::~FXRRuntimeDebugWorkspaceManager()
{
	HideWorkspace();
}

void FXRRuntimeDebugWorkspaceManager::ShowWorkspace()
{
	if (!FSlateApplication::IsInitialized())
	{
		UE_LOG(LogTemp, Warning, TEXT("XR runtime debug workspace could not open because Slate is not initialized."));
		return;
	}

	const FVector2D WindowPosition = ResolveWorkspacePosition(WorkspaceWindowSize);

	if (!WorkspaceWindow.IsValid())
	{
		CreateWorkspaceWindow(WindowPosition, WorkspaceWindowSize);
	}
	else
	{
		WorkspaceWindow->MoveWindowTo(WindowPosition);
		WorkspaceWindow->ShowWindow();
		WorkspaceWindow->BringToFront(true);
		if (WorkspaceWidget.IsValid())
		{
			WorkspaceWidget->RefreshData();
		}
	}
}

void FXRRuntimeDebugWorkspaceManager::HideWorkspace()
{
	if (WorkspaceWindow.IsValid())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().RequestDestroyWindow(WorkspaceWindow.ToSharedRef());
		}
		else
		{
			WorkspaceWindow.Reset();
			WorkspaceWidget.Reset();
		}
	}
}

void FXRRuntimeDebugWorkspaceManager::ToggleWorkspace()
{
	if (IsWorkspaceVisible())
	{
		HideWorkspace();
	}
	else
	{
		ShowWorkspace();
	}
}

bool FXRRuntimeDebugWorkspaceManager::IsWorkspaceVisible() const
{
	return WorkspaceWindow.IsValid();
}

AXRSceneTriggerController* FXRRuntimeDebugWorkspaceManager::GetActiveController() const
{
	return ResolveController();
}

bool FXRRuntimeDebugWorkspaceManager::ExecuteTrigger(const int32 TriggerId, FText& OutMessage)
{
	AXRSceneTriggerController* Controller = ResolveController();
	if (!Controller)
	{
		OutMessage = FText::FromString(TEXT("No XRSceneTriggerController was found in the active runtime or editor world."));
		return false;
	}

	const bool bDidExecute = Controller->TriggerInteger(TriggerId);
	const FString WorldMode = GetWorldModeLabel(Controller->GetWorld());
	const FString TriggerDisplayName = Controller->TriggerConfig
		? ResolveConfiguredTriggerDisplayName(Controller->TriggerConfig, TriggerId)
		: FString::Printf(TEXT("Trigger %d"), TriggerId);
	OutMessage = bDidExecute
		? FText::FromString(FString::Printf(TEXT("Executed %s in %s."), *TriggerDisplayName, *WorldMode))
		: FText::FromString(FString::Printf(TEXT("%s did not execute any configured action in %s."), *TriggerDisplayName, *WorldMode));
	return bDidExecute;
}

void FXRRuntimeDebugWorkspaceManager::StopAll(FText& OutMessage)
{
	AXRSceneTriggerController* Controller = ResolveController();
	if (!Controller)
	{
		OutMessage = FText::FromString(TEXT("No XRSceneTriggerController was found in the active runtime or editor world."));
		return;
	}

	Controller->StopAllAndReset();
	OutMessage = FText::FromString(FString::Printf(TEXT("StopAllAndReset executed in %s."), *GetWorldModeLabel(Controller->GetWorld())));
}

void FXRRuntimeDebugWorkspaceManager::CreateWorkspaceWindow(const FVector2D& WindowPosition, const FVector2D& WindowSize)
{
	WorkspaceWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("XR Runtime Debug Workspace")))
		.ClientSize(WindowSize)
		.ScreenPosition(WindowPosition)
		.AutoCenter(EAutoCenter::None)
		.SizingRule(ESizingRule::UserSized)
		.SupportsMaximize(false)
		.SupportsMinimize(true)
		.HasCloseButton(true)
		.CreateTitleBar(true)
		.FocusWhenFirstShown(true)
		.UseOSWindowBorder(true);

	WorkspaceWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FXRRuntimeDebugWorkspaceManager::HandleWindowClosed));
	WorkspaceWindow->SetContent(
		SAssignNew(WorkspaceWidget, SXRRuntimeDebugWorkspacePanel, this));

	FSlateApplication::Get().AddWindow(WorkspaceWindow.ToSharedRef(), true);
	WorkspaceWindow->BringToFront(true);
}

void FXRRuntimeDebugWorkspaceManager::HandleWindowClosed(const TSharedRef<SWindow>& ClosedWindow)
{
	if (WorkspaceWindow == ClosedWindow)
	{
		WorkspaceWidget.Reset();
		WorkspaceWindow.Reset();
	}
}

FVector2D FXRRuntimeDebugWorkspaceManager::ResolveWorkspacePosition(const FVector2D& DesiredWindowSize) const
{
	FDisplayMetrics DisplayMetrics;
	FDisplayMetrics::RebuildDisplayMetrics(DisplayMetrics);

	const FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos();
	const int32 CursorMonitorIndex = FindMonitorIndexForPoint(DisplayMetrics.MonitorInfo, CursorPosition);
	const TSharedPtr<SWindow> GameWindow = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
	const TArray<int32> BlockedMonitorIndices = ResolveBlockedMonitorIndices(DisplayMetrics.MonitorInfo, GameWindow);

	int32 TargetMonitorIndex = CursorMonitorIndex;
	if (TargetMonitorIndex == INDEX_NONE || BlockedMonitorIndices.Contains(TargetMonitorIndex))
	{
		const int32 FallbackMonitorIndex = FindFallbackMonitorIndex(DisplayMetrics.MonitorInfo, BlockedMonitorIndices, CursorPosition);
		if (FallbackMonitorIndex != INDEX_NONE)
		{
			if (CursorMonitorIndex != INDEX_NONE && BlockedMonitorIndices.Contains(CursorMonitorIndex))
			{
				UE_LOG(LogTemp, Display, TEXT("XR runtime debug workspace skipped monitor '%s' because it overlaps the nDisplay window and moved to '%s'."),
					*GetMonitorDisplayName(DisplayMetrics.MonitorInfo[CursorMonitorIndex]),
					*GetMonitorDisplayName(DisplayMetrics.MonitorInfo[FallbackMonitorIndex]));
			}

			TargetMonitorIndex = FallbackMonitorIndex;
		}
	}

	if (!DisplayMetrics.MonitorInfo.IsValidIndex(TargetMonitorIndex))
	{
		return CursorPosition + WorkspaceWindowOffset;
	}

	const FPlatformRect& WorkArea = DisplayMetrics.MonitorInfo[TargetMonitorIndex].WorkArea;
	const bool bUseCursorMonitor = TargetMonitorIndex == CursorMonitorIndex && !BlockedMonitorIndices.Contains(TargetMonitorIndex);

	const float BaseX = bUseCursorMonitor ? CursorPosition.X + WorkspaceWindowOffset.X : WorkArea.Left + 40.0f;
	const float BaseY = bUseCursorMonitor ? CursorPosition.Y + WorkspaceWindowOffset.Y : WorkArea.Top + 40.0f;
	const float MaxX = FMath::Max<float>(WorkArea.Left, WorkArea.Right - DesiredWindowSize.X);
	const float MaxY = FMath::Max<float>(WorkArea.Top, WorkArea.Bottom - DesiredWindowSize.Y);

	return FVector2D(
		FMath::Clamp(BaseX, static_cast<float>(WorkArea.Left), MaxX),
		FMath::Clamp(BaseY, static_cast<float>(WorkArea.Top), MaxY));
}

UWorld* FXRRuntimeDebugWorkspaceManager::ResolveRuntimeWorld() const
{
	if (GEngine && GEngine->GameViewport)
	{
		if (UWorld* ViewportWorld = GEngine->GameViewport->GetWorld())
		{
			return ViewportWorld;
		}
	}

	if (!GEngine)
	{
		return nullptr;
	}

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType == EWorldType::Game || WorldContext.WorldType == EWorldType::PIE)
		{
			if (UWorld* CandidateWorld = WorldContext.World())
			{
				return CandidateWorld;
			}
		}
	}

#if WITH_EDITOR
	if (GEditor)
	{
		if (UWorld* EditorWorld = GEditor->GetEditorWorldContext().World())
		{
			return EditorWorld;
		}
	}
#endif

	return nullptr;
}

AXRSceneTriggerController* FXRRuntimeDebugWorkspaceManager::ResolveController() const
{
	UWorld* RuntimeWorld = ResolveRuntimeWorld();
	if (!RuntimeWorld)
	{
		return nullptr;
	}

	for (TActorIterator<AXRSceneTriggerController> ActorIt(RuntimeWorld); ActorIt; ++ActorIt)
	{
		return *ActorIt;
	}

	return nullptr;
}
