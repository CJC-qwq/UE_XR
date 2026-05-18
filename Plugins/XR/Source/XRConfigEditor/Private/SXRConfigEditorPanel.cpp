#include "SXRConfigEditorPanel.h"

#include "Editor.h"
#include "DetailLayoutBuilder.h"
#include "IDetailsView.h"
#include "IDetailCustomization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Selection.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"
#include "XRConfigEditorContext.h"
#include "XRConfigEditorSettings.h"
#include "XRSceneTriggerConfig.h"
#include "XRSceneTriggerController.h"

namespace
{
	class FXRControllerDetailsCustomization : public IDetailCustomization
	{
	public:
		static TSharedRef<IDetailCustomization> MakeInstance()
		{
			return MakeShared<FXRControllerDetailsCustomization>();
		}

		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
		{
			static const FName HiddenCategories[] =
			{
				TEXT("Transform"),
				TEXT("Actor"),
				TEXT("Cooking"),
				TEXT("Input"),
				TEXT("Replication"),
				TEXT("Rendering"),
				TEXT("HLOD"),
				TEXT("Physics"),
				TEXT("Collision"),
				TEXT("Data Layers"),
				TEXT("World Partition"),
				TEXT("WorldPartition"),
				TEXT("LOD"),
				TEXT("Tick"),
				TEXT("Tags"),
				TEXT("AssetUserData"),
				TEXT("Events"),
				TEXT("Layers")
			};

			for (const FName CategoryName : HiddenCategories)
			{
				DetailBuilder.HideCategory(CategoryName);
			}
		}
	};

	const FName SceneTriggerControllerPropertyName(TEXT("SceneTriggerController"));
	const FName TriggerConfigPropertyName(TEXT("TriggerConfig"));
	const FName EnableOSCReceiverPropertyName(TEXT("bEnableOSCReceiver"));
	const FName OSCReceivePortPropertyName(TEXT("OSCReceivePort"));
	const FName OSCServerNamePropertyName(TEXT("OSCServerName"));
	const FName OSCAddressPathPropertyName(TEXT("OSCAddressPath"));
	const FName LogOSCMessagesPropertyName(TEXT("bLogOSCMessages"));
	const FName ResolvedLocalIPAddressPropertyName(TEXT("ResolvedLocalIPAddress"));
	const FName PreviewTriggerIdPropertyName(TEXT("PreviewTriggerId"));
	const FName BackgroundLevelsPropertyName(TEXT("BackgroundLevels"));
	const FName EffectLevelsPropertyName(TEXT("EffectLevels"));
	const FName GenericActionsPropertyName(TEXT("GenericActions"));
	const FName TriggerActionsPropertyName(TEXT("TriggerActions"));

	bool MatchesPropertyOrParent(const FPropertyAndParent& PropertyAndParent, const FName TargetPropertyName)
	{
		if (PropertyAndParent.Property.GetFName() == TargetPropertyName)
		{
			return true;
		}

		for (const FProperty* ParentProperty : PropertyAndParent.ParentProperties)
		{
			if (ParentProperty && ParentProperty->GetFName() == TargetPropertyName)
			{
				return true;
			}
		}

		return false;
	}

	FDetailsViewArgs MakeDetailsViewArgs(bool bAllowSearch)
	{
		FDetailsViewArgs Args;
		Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		Args.bAllowSearch = bAllowSearch;
		Args.bHideSelectionTip = true;
		Args.bLockable = false;
		Args.bUpdatesFromSelection = false;
		Args.bShowScrollBar = true;
		return Args;
	}

	TSharedRef<SWidget> MakeSectionToggle(
		const FText& Label,
		TAttribute<ECheckBoxState> CheckState,
		FOnCheckStateChanged OnCheckStateChanged,
		float MinWidth,
		float MinHeight,
		const FName& FontStyle,
		int32 FontSize)
	{
		FSlateFontInfo ToggleFont = FAppStyle::GetFontStyle(FontStyle);
		ToggleFont.Size = FontSize;

		return SNew(SBox)
			.MinDesiredWidth(MinWidth)
			.MinDesiredHeight(MinHeight)
			[
				SNew(SCheckBox)
				.Style(FAppStyle::Get(), "DetailsView.SectionButton")
				.IsChecked(CheckState)
				.OnCheckStateChanged(OnCheckStateChanged)
				[
					SNew(SBox)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FMargin(10.0f, 4.0f))
					[
						SNew(STextBlock)
						.Text(Label)
						.Justification(ETextJustify::Center)
						.Font(ToggleFont)
					]
				]
			];
	}
}

SXRConfigEditorPanel::~SXRConfigEditorPanel()
{
	if (OnMapOpenedHandle.IsValid())
	{
		FEditorDelegates::OnMapOpened.Remove(OnMapOpenedHandle);
		OnMapOpenedHandle.Reset();
	}
}

void SXRConfigEditorPanel::Construct(const FArguments& InArgs)
{
	const FName ContextName = MakeUniqueObjectName(GetTransientPackage(), UXRConfigEditorContext::StaticClass(), TEXT("XRConfigEditorContext"));
	ContextObject = NewObject<UXRConfigEditorContext>(GetTransientPackage(), ContextName, RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	ContextDetailsView = PropertyEditorModule.CreateDetailView(MakeDetailsViewArgs(false));
	ControllerGeneralDetailsView = PropertyEditorModule.CreateDetailView(MakeDetailsViewArgs(false));
	ControllerOSCDetailsView = PropertyEditorModule.CreateDetailView(MakeDetailsViewArgs(false));
	ControllerPreviewDetailsView = PropertyEditorModule.CreateDetailView(MakeDetailsViewArgs(false));
	ConfigBackgroundDetailsView = PropertyEditorModule.CreateDetailView(MakeDetailsViewArgs(true));
	ConfigEffectDetailsView = PropertyEditorModule.CreateDetailView(MakeDetailsViewArgs(true));
	ConfigGenericActionDetailsView = PropertyEditorModule.CreateDetailView(MakeDetailsViewArgs(true));
	ConfigTriggerDetailsView = PropertyEditorModule.CreateDetailView(MakeDetailsViewArgs(true));

	ControllerGeneralDetailsView->RegisterInstancedCustomPropertyLayout(AXRSceneTriggerController::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FXRControllerDetailsCustomization::MakeInstance));
	ControllerOSCDetailsView->RegisterInstancedCustomPropertyLayout(AXRSceneTriggerController::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FXRControllerDetailsCustomization::MakeInstance));
	ControllerPreviewDetailsView->RegisterInstancedCustomPropertyLayout(AXRSceneTriggerController::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FXRControllerDetailsCustomization::MakeInstance));

	ContextDetailsView->SetObject(ContextObject);
	ContextDetailsView->OnFinishedChangingProperties().AddSP(this, &SXRConfigEditorPanel::HandleContextFinishedChangingProperties);

	FIsPropertyVisible ContextVisibility;
	ContextVisibility.BindSP(this, &SXRConfigEditorPanel::IsContextPropertyVisible);
	ContextDetailsView->SetIsPropertyVisibleDelegate(ContextVisibility);

	FIsPropertyVisible ControllerGeneralVisibility;
	ControllerGeneralVisibility.BindSP(this, &SXRConfigEditorPanel::IsControllerGeneralPropertyVisible);
	ControllerGeneralDetailsView->SetIsPropertyVisibleDelegate(ControllerGeneralVisibility);

	FIsPropertyVisible ControllerOSCVisibility;
	ControllerOSCVisibility.BindSP(this, &SXRConfigEditorPanel::IsControllerOSCPropertyVisible);
	ControllerOSCDetailsView->SetIsPropertyVisibleDelegate(ControllerOSCVisibility);

	FIsPropertyVisible ControllerPreviewVisibility;
	ControllerPreviewVisibility.BindSP(this, &SXRConfigEditorPanel::IsControllerPreviewPropertyVisible);
	ControllerPreviewDetailsView->SetIsPropertyVisibleDelegate(ControllerPreviewVisibility);

	FIsPropertyVisible ConfigBackgroundVisibility;
	ConfigBackgroundVisibility.BindSP(this, &SXRConfigEditorPanel::IsConfigBackgroundPropertyVisible);
	ConfigBackgroundDetailsView->SetIsPropertyVisibleDelegate(ConfigBackgroundVisibility);

	FIsPropertyVisible ConfigEffectVisibility;
	ConfigEffectVisibility.BindSP(this, &SXRConfigEditorPanel::IsConfigEffectPropertyVisible);
	ConfigEffectDetailsView->SetIsPropertyVisibleDelegate(ConfigEffectVisibility);

	FIsPropertyVisible ConfigGenericVisibility;
	ConfigGenericVisibility.BindSP(this, &SXRConfigEditorPanel::IsConfigGenericActionPropertyVisible);
	ConfigGenericActionDetailsView->SetIsPropertyVisibleDelegate(ConfigGenericVisibility);

	FIsPropertyVisible ConfigTriggerVisibility;
	ConfigTriggerVisibility.BindSP(this, &SXRConfigEditorPanel::IsConfigTriggerPropertyVisible);
	ConfigTriggerDetailsView->SetIsPropertyVisibleDelegate(ConfigTriggerVisibility);

	OnMapOpenedHandle = FEditorDelegates::OnMapOpened.AddSP(this, &SXRConfigEditorPanel::HandleMapOpened);

	LoadSavedContext();
	RefreshFromContext();

	ChildSlot
	[
		SNew(SBorder)
		.Padding(14.0f)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("XR Config")))
				.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 12.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Use the top-level workspace, controller, and config asset pages instead of editing everything in one crowded details panel.")))
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 14.f)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(6.f, 0.f))
				+ SUniformGridPanel::Slot(0, 0)
				[
					MakeSectionToggle(
						FText::FromString(TEXT("XR Config Workspace")),
						TAttribute<ECheckBoxState>::CreateSP(this, &SXRConfigEditorPanel::GetPageCheckState, EXRConfigEditorPage::Workspace),
						FOnCheckStateChanged::CreateSP(this, &SXRConfigEditorPanel::HandlePageCheckStateChanged, EXRConfigEditorPage::Workspace),
						230.0f,
						38.0f,
						"HeadingExtraSmall",
						13)
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					MakeSectionToggle(
						FText::FromString(TEXT("Scene Trigger Controller")),
						TAttribute<ECheckBoxState>::CreateSP(this, &SXRConfigEditorPanel::GetPageCheckState, EXRConfigEditorPage::SceneTriggerController),
						FOnCheckStateChanged::CreateSP(this, &SXRConfigEditorPanel::HandlePageCheckStateChanged, EXRConfigEditorPage::SceneTriggerController),
						230.0f,
						38.0f,
						"HeadingExtraSmall",
						13)
				]
				+ SUniformGridPanel::Slot(2, 0)
				[
					MakeSectionToggle(
						FText::FromString(TEXT("Trigger Config Asset")),
						TAttribute<ECheckBoxState>::CreateSP(this, &SXRConfigEditorPanel::GetPageCheckState, EXRConfigEditorPage::TriggerConfigAsset),
						FOnCheckStateChanged::CreateSP(this, &SXRConfigEditorPanel::HandlePageCheckStateChanged, EXRConfigEditorPage::TriggerConfigAsset),
						230.0f,
						38.0f,
						"HeadingExtraSmall",
						13)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SWidgetSwitcher)
				.WidgetIndex_Lambda([this]() { return static_cast<int32>(CurrentPage); })
				+ SWidgetSwitcher::Slot()
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 12.f)
						[
							SNew(SBorder)
							.Padding(12.0f)
							.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
							[
								SNew(STextBlock)
								.Text(this, &SXRConfigEditorPanel::GetSummaryText)
								.AutoWrapText(true)
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 12.f)
						[
							SNew(SBorder)
							.Padding(12.0f)
							.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.f, 0.f, 0.f, 8.f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Workspace Context")))
									.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									ContextDetailsView.ToSharedRef()
								]
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 12.f)
						[
							SNew(SUniformGridPanel)
							.SlotPadding(FMargin(6.f))
							+ SUniformGridPanel::Slot(0, 0)
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("Use Selected Controller")))
								.OnClicked(this, &SXRConfigEditorPanel::HandleUseSelectedController)
							]
							+ SUniformGridPanel::Slot(1, 0)
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("Use Config From Controller")))
								.OnClicked(this, &SXRConfigEditorPanel::HandleUseConfigFromController)
							]
							+ SUniformGridPanel::Slot(0, 1)
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("Open Config Asset")))
								.OnClicked(this, &SXRConfigEditorPanel::HandleOpenConfigAsset)
							]
							+ SUniformGridPanel::Slot(1, 1)
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("Focus Controller")))
								.OnClicked(this, &SXRConfigEditorPanel::HandleFocusController)
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.Padding(12.0f)
							.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
							[
								SNew(STextBlock)
								.Text(this, &SXRConfigEditorPanel::GetWorkspaceHintText)
								.AutoWrapText(true)
							]
						]
					]
				]
				+ SWidgetSwitcher::Slot()
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 12.f)
						[
							SNew(SBorder)
							.Padding(12.0f)
							.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
							[
								SNew(STextBlock)
								.Text(this, &SXRConfigEditorPanel::GetControllerStatusText)
								.AutoWrapText(true)
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 12.f)
						[
							SNew(SUniformGridPanel)
							.SlotPadding(FMargin(6.f, 0.f))
							+ SUniformGridPanel::Slot(0, 0)
							[
								MakeSectionToggle(
									FText::FromString(TEXT("General")),
									TAttribute<ECheckBoxState>::CreateSP(this, &SXRConfigEditorPanel::GetControllerSectionCheckState, EXRControllerPanelSection::General),
									FOnCheckStateChanged::CreateSP(this, &SXRConfigEditorPanel::HandleControllerSectionChanged, EXRControllerPanelSection::General),
									160.0f,
									34.0f,
									"PropertyWindow.BoldFont",
									11)
							]
							+ SUniformGridPanel::Slot(1, 0)
							[
								MakeSectionToggle(
									FText::FromString(TEXT("OSC")),
									TAttribute<ECheckBoxState>::CreateSP(this, &SXRConfigEditorPanel::GetControllerSectionCheckState, EXRControllerPanelSection::OSC),
									FOnCheckStateChanged::CreateSP(this, &SXRConfigEditorPanel::HandleControllerSectionChanged, EXRControllerPanelSection::OSC),
									160.0f,
									34.0f,
									"PropertyWindow.BoldFont",
									11)
							]
							+ SUniformGridPanel::Slot(2, 0)
							[
								MakeSectionToggle(
									FText::FromString(TEXT("Preview / Stop")),
									TAttribute<ECheckBoxState>::CreateSP(this, &SXRConfigEditorPanel::GetControllerSectionCheckState, EXRControllerPanelSection::Preview),
									FOnCheckStateChanged::CreateSP(this, &SXRConfigEditorPanel::HandleControllerSectionChanged, EXRControllerPanelSection::Preview),
									160.0f,
									34.0f,
									"PropertyWindow.BoldFont",
									11)
							]
						]
						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						[
							SNew(SWidgetSwitcher)
							.WidgetIndex_Lambda([this]() { return static_cast<int32>(CurrentControllerSection); })
							+ SWidgetSwitcher::Slot()
							[
								SNew(SBorder)
								.Padding(12.0f)
								.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								[
									ControllerGeneralDetailsView.ToSharedRef()
								]
							]
							+ SWidgetSwitcher::Slot()
							[
								SNew(SBorder)
								.Padding(12.0f)
								.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								[
									ControllerOSCDetailsView.ToSharedRef()
								]
							]
							+ SWidgetSwitcher::Slot()
							[
								SNew(SBorder)
								.Padding(12.0f)
								.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.f, 0.f, 0.f, 10.f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Use this page to preview the current Trigger Id or force a full stop without exposing unrelated Actor settings.")))
										.AutoWrapText(true)
									]
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.f, 0.f, 0.f, 10.f)
									[
										ControllerPreviewDetailsView.ToSharedRef()
									]
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SUniformGridPanel)
										.SlotPadding(FMargin(6.f))
										+ SUniformGridPanel::Slot(0, 0)
										[
											SNew(SButton)
											.Text(FText::FromString(TEXT("Preview Current Trigger")))
											.OnClicked(this, &SXRConfigEditorPanel::HandlePreviewCurrentTrigger)
										]
										+ SUniformGridPanel::Slot(1, 0)
										[
											SNew(SButton)
											.Text(FText::FromString(TEXT("Stop All And Reset")))
											.OnClicked(this, &SXRConfigEditorPanel::HandleStopAllAndReset)
										]
									]
								]
							]
						]
					]
				]
				+ SWidgetSwitcher::Slot()
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 12.f)
						[
							SNew(SBorder)
							.Padding(12.0f)
							.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
							[
								SNew(STextBlock)
								.Text(this, &SXRConfigEditorPanel::GetConfigStatusText)
								.AutoWrapText(true)
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 12.f)
						[
							SNew(SUniformGridPanel)
							.SlotPadding(FMargin(6.f, 0.f))
							+ SUniformGridPanel::Slot(0, 0)
							[
								MakeSectionToggle(
									FText::FromString(TEXT("Background Levels")),
									TAttribute<ECheckBoxState>::CreateSP(this, &SXRConfigEditorPanel::GetConfigSectionCheckState, EXRConfigPanelSection::Backgrounds),
									FOnCheckStateChanged::CreateSP(this, &SXRConfigEditorPanel::HandleConfigSectionChanged, EXRConfigPanelSection::Backgrounds),
									180.0f,
									34.0f,
									"PropertyWindow.BoldFont",
									11)
							]
							+ SUniformGridPanel::Slot(1, 0)
							[
								MakeSectionToggle(
									FText::FromString(TEXT("Effect Levels")),
									TAttribute<ECheckBoxState>::CreateSP(this, &SXRConfigEditorPanel::GetConfigSectionCheckState, EXRConfigPanelSection::Effects),
									FOnCheckStateChanged::CreateSP(this, &SXRConfigEditorPanel::HandleConfigSectionChanged, EXRConfigPanelSection::Effects),
									180.0f,
									34.0f,
									"PropertyWindow.BoldFont",
									11)
							]
							+ SUniformGridPanel::Slot(2, 0)
							[
								MakeSectionToggle(
									FText::FromString(TEXT("Generic Actions")),
									TAttribute<ECheckBoxState>::CreateSP(this, &SXRConfigEditorPanel::GetConfigSectionCheckState, EXRConfigPanelSection::GenericActions),
									FOnCheckStateChanged::CreateSP(this, &SXRConfigEditorPanel::HandleConfigSectionChanged, EXRConfigPanelSection::GenericActions),
									180.0f,
									34.0f,
									"PropertyWindow.BoldFont",
									11)
							]
							+ SUniformGridPanel::Slot(3, 0)
							[
								MakeSectionToggle(
									FText::FromString(TEXT("Trigger Actions")),
									TAttribute<ECheckBoxState>::CreateSP(this, &SXRConfigEditorPanel::GetConfigSectionCheckState, EXRConfigPanelSection::Triggers),
									FOnCheckStateChanged::CreateSP(this, &SXRConfigEditorPanel::HandleConfigSectionChanged, EXRConfigPanelSection::Triggers),
									180.0f,
									34.0f,
									"PropertyWindow.BoldFont",
									11)
							]
						]
						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						[
							SNew(SWidgetSwitcher)
							.WidgetIndex_Lambda([this]() { return static_cast<int32>(CurrentConfigSection); })
							+ SWidgetSwitcher::Slot()
							[
								SNew(SBorder)
								.Padding(12.0f)
								.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								[
									ConfigBackgroundDetailsView.ToSharedRef()
								]
							]
							+ SWidgetSwitcher::Slot()
							[
								SNew(SBorder)
								.Padding(12.0f)
								.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								[
									ConfigEffectDetailsView.ToSharedRef()
								]
							]
							+ SWidgetSwitcher::Slot()
							[
								SNew(SBorder)
								.Padding(12.0f)
								.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								[
									ConfigGenericActionDetailsView.ToSharedRef()
								]
							]
							+ SWidgetSwitcher::Slot()
							[
								SNew(SBorder)
								.Padding(12.0f)
								.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								[
									ConfigTriggerDetailsView.ToSharedRef()
								]
							]
						]
					]
				]
			]
		]
	];
}

FReply SXRConfigEditorPanel::HandleUseSelectedController()
{
	if (!GEditor)
	{
		return FReply::Handled();
	}

	for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
	{
		if (AXRSceneTriggerController* Controller = Cast<AXRSceneTriggerController>(*It))
		{
			ContextObject->SceneTriggerController = Controller;
			if (UXRSceneTriggerConfig* ControllerConfig = Controller->TriggerConfig)
			{
				ContextObject->TriggerConfig = ControllerConfig;
			}
			ContextDetailsView->ForceRefresh();
			SaveContext();
			RefreshFromContext();
			break;
		}
	}

	return FReply::Handled();
}

FReply SXRConfigEditorPanel::HandleClearController()
{
	if (!ContextObject)
	{
		return FReply::Handled();
	}

	ContextObject->SceneTriggerController = nullptr;
	ContextDetailsView->ForceRefresh();
	SaveContext();
	RefreshFromContext();
	return FReply::Handled();
}

FReply SXRConfigEditorPanel::HandleUseConfigFromController()
{
	SanitizeContextObjects();
	if (ContextObject)
	{
		if (AXRSceneTriggerController* Controller = ResolveSceneTriggerController())
		{
			ContextObject->TriggerConfig = Controller->TriggerConfig;
			ContextDetailsView->ForceRefresh();
			SaveContext();
			RefreshFromContext();
		}
	}

	return FReply::Handled();
}

FReply SXRConfigEditorPanel::HandleClearConfigAsset()
{
	if (!ContextObject)
	{
		return FReply::Handled();
	}

	ContextObject->TriggerConfig = nullptr;
	ContextDetailsView->ForceRefresh();
	SaveContext();
	RefreshFromContext();
	return FReply::Handled();
}

FReply SXRConfigEditorPanel::HandleOpenConfigAsset()
{
	SanitizeContextObjects();
	if (GEditor)
	{
		if (UXRSceneTriggerConfig* Config = ResolveTriggerConfig())
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Config);
		}
	}

	return FReply::Handled();
}

FReply SXRConfigEditorPanel::HandleFocusController()
{
	SanitizeContextObjects();
	if (!GEditor || !ContextObject)
	{
		return FReply::Handled();
	}

	AXRSceneTriggerController* Controller = ResolveSceneTriggerController();
	if (!Controller)
	{
		return FReply::Handled();
	}

	GEditor->SelectNone(false, true);
	GEditor->SelectActor(Controller, true, true, true);
	GEditor->MoveViewportCamerasToActor(*Controller, false);
	return FReply::Handled();
}

FReply SXRConfigEditorPanel::HandlePreviewCurrentTrigger()
{
	SanitizeContextObjects();
	if (AXRSceneTriggerController* Controller = ResolveSceneTriggerController())
	{
		Controller->PreviewTrigger();
	}

	return FReply::Handled();
}

FReply SXRConfigEditorPanel::HandleStopAllAndReset()
{
	SanitizeContextObjects();
	if (AXRSceneTriggerController* Controller = ResolveSceneTriggerController())
	{
		Controller->StopAllAndReset();
	}

	return FReply::Handled();
}

void SXRConfigEditorPanel::LoadSavedContext()
{
	if (!ContextObject)
	{
		return;
	}

	const UXRConfigEditorSettings* Settings = GetDefault<UXRConfigEditorSettings>();
	if (!Settings)
	{
		return;
	}

	const FString CurrentMapPath = GetCurrentMapPath();
	if (!Settings->LastContextMapPath.IsEmpty() && !CurrentMapPath.IsEmpty() && Settings->LastContextMapPath != CurrentMapPath)
	{
		UXRConfigEditorSettings* MutableSettings = GetMutableDefault<UXRConfigEditorSettings>();
		if (MutableSettings)
		{
			MutableSettings->LastContextMapPath = CurrentMapPath;
			MutableSettings->LastSceneTriggerController.Reset();
			MutableSettings->LastTriggerConfig.Reset();
			MutableSettings->SaveConfig();
		}
		return;
	}

	ContextObject->SceneTriggerController = Cast<AXRSceneTriggerController>(Settings->LastSceneTriggerController.ResolveObject());
	if (!ContextObject->SceneTriggerController)
	{
		ContextObject->SceneTriggerController = Cast<AXRSceneTriggerController>(FindObject<UObject>(nullptr, *Settings->LastSceneTriggerController.ToString()));
	}

	ContextObject->TriggerConfig = Cast<UXRSceneTriggerConfig>(Settings->LastTriggerConfig.ResolveObject());
	if (!ContextObject->TriggerConfig && !Settings->LastTriggerConfig.IsNull())
	{
		ContextObject->TriggerConfig = Cast<UXRSceneTriggerConfig>(Settings->LastTriggerConfig.TryLoad());
	}

	SanitizeContextObjects();
}

void SXRConfigEditorPanel::SaveContext() const
{
	UXRConfigEditorSettings* Settings = GetMutableDefault<UXRConfigEditorSettings>();
	if (!Settings)
	{
		return;
	}

	Settings->LastContextMapPath = GetCurrentMapPath();
	Settings->LastSceneTriggerController = (ContextObject && ContextObject->SceneTriggerController)
		? FSoftObjectPath(ContextObject->SceneTriggerController)
		: FSoftObjectPath();
	Settings->LastTriggerConfig = (ContextObject && ContextObject->TriggerConfig)
		? FSoftObjectPath(ContextObject->TriggerConfig)
		: FSoftObjectPath();

	Settings->SaveConfig();
}

void SXRConfigEditorPanel::RefreshFromContext()
{
	SanitizeContextObjects();

	AXRSceneTriggerController* Controller = ResolveSceneTriggerController();
	UXRSceneTriggerConfig* Config = ResolveTriggerConfig();

	ControllerGeneralDetailsView->SetObject(Controller);
	ControllerOSCDetailsView->SetObject(Controller);
	ControllerPreviewDetailsView->SetObject(Controller);
	ConfigBackgroundDetailsView->SetObject(Config);
	ConfigEffectDetailsView->SetObject(Config);
	ConfigGenericActionDetailsView->SetObject(Config);
	ConfigTriggerDetailsView->SetObject(Config);
	RefreshSummary();
}

void SXRConfigEditorPanel::SanitizeContextObjects()
{
	if (!ContextObject)
	{
		return;
	}

	if (ContextObject->SceneTriggerController && !IsValid(ContextObject->SceneTriggerController))
	{
		ContextObject->SceneTriggerController = nullptr;
	}

	if (ContextObject->TriggerConfig && !IsValid(ContextObject->TriggerConfig))
	{
		ContextObject->TriggerConfig = nullptr;
	}

	if (!ContextObject->TriggerConfig)
	{
		if (AXRSceneTriggerController* Controller = ResolveSceneTriggerController())
		{
			ContextObject->TriggerConfig = Controller->TriggerConfig;
		}
	}
}

void SXRConfigEditorPanel::ClearContext(bool bSaveSettings)
{
	if (!ContextObject)
	{
		return;
	}

	ContextObject->SceneTriggerController = nullptr;
	ContextObject->TriggerConfig = nullptr;

	if (bSaveSettings)
	{
		SaveContext();
	}
}

void SXRConfigEditorPanel::HandleMapOpened(const FString& Filename, bool bAsTemplate)
{
	ClearContext(true);
	RefreshFromContext();
}

AXRSceneTriggerController* SXRConfigEditorPanel::ResolveSceneTriggerController() const
{
	if (!ContextObject || !ContextObject->SceneTriggerController)
	{
		return nullptr;
	}

	return IsValid(ContextObject->SceneTriggerController) ? ContextObject->SceneTriggerController.Get() : nullptr;
}

UXRSceneTriggerConfig* SXRConfigEditorPanel::ResolveTriggerConfig() const
{
	if (!ContextObject || !ContextObject->TriggerConfig)
	{
		return nullptr;
	}

	return IsValid(ContextObject->TriggerConfig) ? ContextObject->TriggerConfig.Get() : nullptr;
}

void SXRConfigEditorPanel::RefreshSummary()
{
	SanitizeContextObjects();

	if (!ContextObject)
	{
		SummaryText = FText::FromString(TEXT("No XR workspace context selected."));
		return;
	}

	const AXRSceneTriggerController* Controller = ResolveSceneTriggerController();
	const UXRSceneTriggerConfig* Config = ResolveTriggerConfig();

	TArray<FString> Lines;
	Lines.Add(TEXT("Workspace keeps only the objects you actually configure in daily use."));
	if (Controller)
	{
		Lines.Add(FString::Printf(TEXT("Controller: %s"), *Controller->GetName()));
	}
	else if (ContextObject->SceneTriggerController)
	{
		Lines.Add(FString::Printf(TEXT("Controller: Currently unavailable (%s)"), *ContextObject->SceneTriggerController->GetName()));
	}
	else
	{
		Lines.Add(TEXT("Controller: Not selected"));
	}

	if (Config)
	{
		Lines.Add(FString::Printf(TEXT("Config Asset: %s"), *Config->GetName()));
	}
	else if (ContextObject->TriggerConfig)
	{
		Lines.Add(FString::Printf(TEXT("Config Asset: Currently unavailable (%s)"), *ContextObject->TriggerConfig->GetName()));
	}
	else
	{
		Lines.Add(TEXT("Config Asset: Not selected"));
	}

	if (Config)
	{
		Lines.Add(FString::Printf(TEXT("Background Levels: %d"), Config->BackgroundLevels.Num()));
		Lines.Add(FString::Printf(TEXT("Effect Levels: %d"), Config->EffectLevels.Num()));
		Lines.Add(FString::Printf(TEXT("Generic Actions: %d"), Config->GenericActions.Num()));
		Lines.Add(FString::Printf(TEXT("Trigger Actions: %d"), Config->TriggerActions.Num()));
	}

	if (Controller && Config && Controller->TriggerConfig != Config)
	{
		Lines.Add(TEXT("Warning: controller TriggerConfig is not the same asset currently selected in the workspace."));
	}

	SummaryText = FText::FromString(FString::Join(Lines, TEXT("\n")));
}

void SXRConfigEditorPanel::HandleContextFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	SaveContext();
	RefreshFromContext();
}

FText SXRConfigEditorPanel::GetSummaryText() const
{
	return SummaryText;
}

FText SXRConfigEditorPanel::GetWorkspaceHintText() const
{
	return FText::FromString(TEXT("Start in XR Config Workspace to bind the controller and config asset, then switch to the dedicated Controller or Config Asset pages for focused editing."));
}

FText SXRConfigEditorPanel::GetControllerStatusText() const
{
	const AXRSceneTriggerController* Controller = ResolveSceneTriggerController();
	if (!Controller)
	{
		return FText::FromString(TEXT("No Scene Trigger Controller selected yet."));
	}

	return FText::FromString(FString::Printf(
		TEXT("Editing controller '%s'. This view only keeps runtime trigger, preview, and OSC-related fields."),
		*Controller->GetName()));
}

FText SXRConfigEditorPanel::GetConfigStatusText() const
{
	const UXRSceneTriggerConfig* Config = ResolveTriggerConfig();
	if (!Config)
	{
		return FText::FromString(TEXT("No Trigger Config Asset selected yet."));
	}

	return FText::FromString(FString::Printf(
		TEXT("Editing config asset '%s'. Use the section toggles to focus on one content block at a time."),
		*Config->GetName()));
}

FString SXRConfigEditorPanel::GetCurrentMapPath() const
{
	if (!GEditor)
	{
		return FString();
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return FString();
	}

	UPackage* WorldPackage = EditorWorld->GetOutermost();
	return WorldPackage ? WorldPackage->GetName() : FString();
}

ECheckBoxState SXRConfigEditorPanel::GetPageCheckState(EXRConfigEditorPage Page) const
{
	return CurrentPage == Page ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SXRConfigEditorPanel::HandlePageCheckStateChanged(ECheckBoxState NewState, EXRConfigEditorPage Page)
{
	if (NewState == ECheckBoxState::Checked)
	{
		CurrentPage = Page;
	}
}

ECheckBoxState SXRConfigEditorPanel::GetControllerSectionCheckState(EXRControllerPanelSection Section) const
{
	return CurrentControllerSection == Section ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SXRConfigEditorPanel::HandleControllerSectionChanged(ECheckBoxState NewState, EXRControllerPanelSection Section)
{
	if (NewState == ECheckBoxState::Checked)
	{
		CurrentControllerSection = Section;
	}
}

ECheckBoxState SXRConfigEditorPanel::GetConfigSectionCheckState(EXRConfigPanelSection Section) const
{
	return CurrentConfigSection == Section ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SXRConfigEditorPanel::HandleConfigSectionChanged(ECheckBoxState NewState, EXRConfigPanelSection Section)
{
	if (NewState == ECheckBoxState::Checked)
	{
		CurrentConfigSection = Section;
	}
}

bool SXRConfigEditorPanel::IsContextPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	return MatchesPropertyOrParent(PropertyAndParent, SceneTriggerControllerPropertyName)
		|| MatchesPropertyOrParent(PropertyAndParent, TriggerConfigPropertyName);
}

bool SXRConfigEditorPanel::IsControllerGeneralPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	return MatchesPropertyOrParent(PropertyAndParent, TriggerConfigPropertyName);
}

bool SXRConfigEditorPanel::IsControllerOSCPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	return MatchesPropertyOrParent(PropertyAndParent, EnableOSCReceiverPropertyName)
		|| MatchesPropertyOrParent(PropertyAndParent, OSCReceivePortPropertyName)
		|| MatchesPropertyOrParent(PropertyAndParent, OSCServerNamePropertyName)
		|| MatchesPropertyOrParent(PropertyAndParent, OSCAddressPathPropertyName)
		|| MatchesPropertyOrParent(PropertyAndParent, LogOSCMessagesPropertyName)
		|| MatchesPropertyOrParent(PropertyAndParent, ResolvedLocalIPAddressPropertyName);
}

bool SXRConfigEditorPanel::IsControllerPreviewPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	return MatchesPropertyOrParent(PropertyAndParent, PreviewTriggerIdPropertyName);
}

bool SXRConfigEditorPanel::IsConfigBackgroundPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	return MatchesPropertyOrParent(PropertyAndParent, BackgroundLevelsPropertyName);
}

bool SXRConfigEditorPanel::IsConfigEffectPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	return MatchesPropertyOrParent(PropertyAndParent, EffectLevelsPropertyName);
}

bool SXRConfigEditorPanel::IsConfigGenericActionPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	return MatchesPropertyOrParent(PropertyAndParent, GenericActionsPropertyName);
}

bool SXRConfigEditorPanel::IsConfigTriggerPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	return MatchesPropertyOrParent(PropertyAndParent, TriggerActionsPropertyName);
}
