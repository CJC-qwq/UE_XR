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
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
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
		PendingTriggerId = 0;

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
							.FillWidth(1.0f)
							.Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SNumericEntryBox<int32>)
								.AllowSpin(true)
								.MinDesiredValueWidth(140.0f)
								.LabelVAlign(VAlign_Center)
								.Label()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Trigger Id")))
								]
								.Value(this, &SXRRuntimeDebugWorkspacePanel::GetPendingTriggerId)
								.OnValueChanged(this, &SXRRuntimeDebugWorkspacePanel::HandlePendingTriggerChanged)
								.OnValueCommitted(this, &SXRRuntimeDebugWorkspacePanel::HandlePendingTriggerCommitted)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("Run Trigger")))
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

	TSharedRef<SWidget> MakeReadonlyRow(const FString& Description) const
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

	TSharedRef<SWidget> BuildBackgroundSectionBody(const UXRSceneTriggerConfig* Config) const
	{
		TSharedRef<SVerticalBox> Entries = SNew(SVerticalBox);

		if (!Config || Config->BackgroundLevels.IsEmpty())
		{
			Entries->AddSlot()
			.AutoHeight()
			[
				MakeReadonlyRow(TEXT("No background levels configured."))
			];
			return Entries;
		}

		for (const FXRBackgroundLevelDefinition& Entry : Config->BackgroundLevels)
		{
			const FString Description = FString::Printf(
				TEXT("Label: %s | Id: %s | Level: %s | Sequence: %s | Reload On Retrigger: %s"),
				Entry.BackgroundLabel.IsEmpty() ? TEXT("(Empty)") : *Entry.BackgroundLabel,
				*NameOrNone(Entry.BackgroundId),
				*GetSoftAssetDisplayName(Entry.LevelAsset),
				*GetSoftAssetDisplayName(Entry.SequenceAsset),
				Entry.bReloadWhenTriggeredAgain ? TEXT("Yes") : TEXT("No"));

			Entries->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				MakeReadonlyRow(Description)
			];
		}

		return Entries;
	}

	void RebuildBackgroundSection()
	{
		ResetSectionBox(BackgroundSectionBox);

		const AXRSceneTriggerController* Controller = OwnerManager ? OwnerManager->GetActiveController() : nullptr;
		const UXRSceneTriggerConfig* Config = Controller ? Controller->TriggerConfig : nullptr;
		if (!BackgroundSectionBox.IsValid())
		{
			return;
		}

		BackgroundSectionBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SExpandableArea)
			.AreaTitle(FText::FromString(TEXT("Background Levels")))
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
				MakeReadonlyRow(TEXT("No effect levels configured."))
			];
			return Entries;
		}

		for (const FXREffectLevelDefinition& Entry : Config->EffectLevels)
		{
			const FString Description = FString::Printf(
				TEXT("Label: %s | Id: %s | Level: %s | Sequence: %s | Allowed Backgrounds: %s | Reload On Retrigger: %s | Auto Unload: %s | Fallback Delay: %.2fs"),
				Entry.EffectLabel.IsEmpty() ? TEXT("(Empty)") : *Entry.EffectLabel,
				*NameOrNone(Entry.EffectId),
				*GetSoftAssetDisplayName(Entry.LevelAsset),
				*GetSoftAssetDisplayName(Entry.SequenceAsset),
				*JoinNames(Entry.AllowedBackgroundIds),
				Entry.bReloadWhenTriggeredAgain ? TEXT("Yes") : TEXT("No"),
				Entry.bAutoUnloadWhenSequenceCompletes ? TEXT("Yes") : TEXT("No"),
				Entry.FallbackAutoUnloadDelay);

			Entries->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				MakeReadonlyRow(Description)
			];
		}

		return Entries;
	}

	void RebuildEffectSection()
	{
		ResetSectionBox(EffectSectionBox);

		const AXRSceneTriggerController* Controller = OwnerManager ? OwnerManager->GetActiveController() : nullptr;
		const UXRSceneTriggerConfig* Config = Controller ? Controller->TriggerConfig : nullptr;
		if (!EffectSectionBox.IsValid())
		{
			return;
		}

		EffectSectionBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SExpandableArea)
			.AreaTitle(FText::FromString(TEXT("Effect Levels")))
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
				MakeReadonlyRow(TEXT("No generic actions configured."))
			];
			return Entries;
		}

		for (const FXRGenericActionDefinition& Entry : Config->GenericActions)
		{
			const FString TargetSummary = Entry.TargetType == EXRGenericActionTargetType::Actor
				? FString::Printf(TEXT("Actor Tag: %s"), *NameOrNone(Entry.TargetActorTag))
				: TEXT("Persistent Level Blueprint");
			const FString Description = FString::Printf(
				TEXT("Label: %s | Id: %s | Target: %s | Trigger Action: %s | Stop Action: %s | Allowed Backgrounds: %s"),
				Entry.ActionLabel.IsEmpty() ? TEXT("(Empty)") : *Entry.ActionLabel,
				*NameOrNone(Entry.ActionId),
				*TargetSummary,
				*NameOrNone(Entry.ActionName),
				*NameOrNone(Entry.StopActionName),
				*JoinNames(Entry.AllowedBackgroundIds));

			Entries->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				MakeReadonlyRow(Description)
			];
		}

		return Entries;
	}

	void RebuildGenericActionSection()
	{
		ResetSectionBox(GenericActionSectionBox);

		const AXRSceneTriggerController* Controller = OwnerManager ? OwnerManager->GetActiveController() : nullptr;
		const UXRSceneTriggerConfig* Config = Controller ? Controller->TriggerConfig : nullptr;
		if (!GenericActionSectionBox.IsValid())
		{
			return;
		}

		GenericActionSectionBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SExpandableArea)
			.AreaTitle(FText::FromString(TEXT("Generic Actions")))
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
				MakeReadonlyRow(TEXT("No trigger actions configured."))
			];
			return Entries;
		}

		for (const FXRTriggerActionEntry& Entry : Config->TriggerActions)
		{
			const FString Description = FString::Printf(
				TEXT("Label: %s | Trigger Id: %d | Background: %s | Force Reload: %s | Effects: %s | Generic Actions: %s"),
				Entry.TriggerLabel.IsEmpty() ? TEXT("(Empty)") : *Entry.TriggerLabel,
				Entry.TriggerId,
				*NameOrNone(Entry.BackgroundToActivate),
				Entry.bForceBackgroundReload ? TEXT("Yes") : TEXT("No"),
				*JoinNames(Entry.EffectLevelsToTrigger),
				*JoinNames(Entry.GenericEffectsToTrigger));

			Entries->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(SBorder)
				.Padding(6.0f)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Text(FText::FromString(Description))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Run")))
						.OnClicked(this, &SXRRuntimeDebugWorkspacePanel::HandleRunSpecificTriggerClicked, Entry.TriggerId)
					]
				]
			];
		}

		return Entries;
	}

	void RebuildTriggerSection()
	{
		ResetSectionBox(TriggerSectionBox);

		const AXRSceneTriggerController* Controller = OwnerManager ? OwnerManager->GetActiveController() : nullptr;
		const UXRSceneTriggerConfig* Config = Controller ? Controller->TriggerConfig : nullptr;
		if (!TriggerSectionBox.IsValid())
		{
			return;
		}

		TriggerSectionBox->AddSlot()
		.AutoHeight()
		[
			SNew(SExpandableArea)
			.AreaTitle(FText::FromString(TEXT("Trigger Actions")))
			.InitiallyCollapsed(false)
			.BodyContent()
			[
				BuildTriggerSectionBody(Config)
			]
		];
	}

	TOptional<int32> GetPendingTriggerId() const
	{
		return PendingTriggerId;
	}

	void HandlePendingTriggerChanged(const int32 NewValue)
	{
		PendingTriggerId = NewValue;
	}

	void HandlePendingTriggerCommitted(const int32 NewValue, ETextCommit::Type CommitType)
	{
		PendingTriggerId = NewValue;
	}

	bool CanRunLastTrigger() const
	{
		return LastExecutedTriggerId.IsSet();
	}

	FReply HandleRunTriggerClicked()
	{
		if (!PendingTriggerId.IsSet())
		{
			LastActionStatus = FText::FromString(TEXT("Enter a trigger id before running."));
			return FReply::Handled();
		}

		return ExecuteTriggerAndReport(PendingTriggerId.GetValue());
	}

	FReply HandleRunSpecificTriggerClicked(const int32 TriggerId)
	{
		PendingTriggerId = TriggerId;
		return ExecuteTriggerAndReport(TriggerId);
	}

	FReply HandleRunLastTriggerClicked()
	{
		if (!LastExecutedTriggerId.IsSet())
		{
			LastActionStatus = FText::FromString(TEXT("No previous trigger has been run yet."));
			return FReply::Handled();
		}

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

		const FString Summary = FString::Printf(
			TEXT("Active Background: %s | Active Effects: %s | Active Generic Actions: %s"),
			*NameOrNone(Controller->GetActiveBackgroundId()),
			*JoinNames(Controller->GetActiveEffectIds()),
			*JoinNames(Controller->GetActiveGenericActionIds()));
		return FText::FromString(Summary);
	}

	FXRRuntimeDebugWorkspaceManager* OwnerManager = nullptr;
	TOptional<int32> PendingTriggerId;
	TOptional<int32> LastExecutedTriggerId;
	FText LastActionStatus;
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
	OutMessage = bDidExecute
		? FText::FromString(FString::Printf(TEXT("Executed trigger %d in %s."), TriggerId, *WorldMode))
		: FText::FromString(FString::Printf(TEXT("Trigger %d did not execute any configured action in %s."), TriggerId, *WorldMode));
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
