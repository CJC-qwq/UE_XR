#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
struct FPropertyAndParent;
class UXRConfigEditorContext;
class AXRSceneTriggerController;
class UXRSceneTriggerConfig;

enum class EXRConfigEditorPage : uint8
{
	Workspace,
	SceneTriggerController,
	TriggerConfigAsset
};

enum class EXRControllerPanelSection : uint8
{
	General,
	OSC,
	Preview
};

enum class EXRConfigPanelSection : uint8
{
	Backgrounds,
	Effects,
	GenericActions,
	Triggers
};

class SXRConfigEditorPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SXRConfigEditorPanel) {}
	SLATE_END_ARGS()

	~SXRConfigEditorPanel();

	void Construct(const FArguments& InArgs);

private:
	FReply HandleUseSelectedController();
	FReply HandleClearController();
	FReply HandleUseConfigFromController();
	FReply HandleClearConfigAsset();
	FReply HandleOpenConfigAsset();
	FReply HandleFocusController();
	FReply HandlePreviewCurrentTrigger();
	FReply HandleStopAllAndReset();

	void LoadSavedContext();
	void SaveContext() const;
	void RefreshFromContext();
	void RefreshSummary();
	void SanitizeContextObjects();
	void ClearContext(bool bSaveSettings);
	void HandleMapOpened(const FString& Filename, bool bAsTemplate);
	void HandleContextFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	AXRSceneTriggerController* ResolveSceneTriggerController() const;
	UXRSceneTriggerConfig* ResolveTriggerConfig() const;

	FText GetSummaryText() const;
	FText GetWorkspaceHintText() const;
	FText GetControllerStatusText() const;
	FText GetConfigStatusText() const;

	ECheckBoxState GetPageCheckState(EXRConfigEditorPage Page) const;
	void HandlePageCheckStateChanged(ECheckBoxState NewState, EXRConfigEditorPage Page);

	ECheckBoxState GetControllerSectionCheckState(EXRControllerPanelSection Section) const;
	void HandleControllerSectionChanged(ECheckBoxState NewState, EXRControllerPanelSection Section);

	ECheckBoxState GetConfigSectionCheckState(EXRConfigPanelSection Section) const;
	void HandleConfigSectionChanged(ECheckBoxState NewState, EXRConfigPanelSection Section);

	bool IsContextPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	bool IsControllerGeneralPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	bool IsControllerOSCPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	bool IsControllerPreviewPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	bool IsConfigBackgroundPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	bool IsConfigEffectPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	bool IsConfigGenericActionPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	bool IsConfigTriggerPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	FString GetCurrentMapPath() const;

	TSharedPtr<IDetailsView> ContextDetailsView;
	TSharedPtr<IDetailsView> ControllerGeneralDetailsView;
	TSharedPtr<IDetailsView> ControllerOSCDetailsView;
	TSharedPtr<IDetailsView> ControllerPreviewDetailsView;
	TSharedPtr<IDetailsView> ConfigBackgroundDetailsView;
	TSharedPtr<IDetailsView> ConfigEffectDetailsView;
	TSharedPtr<IDetailsView> ConfigGenericActionDetailsView;
	TSharedPtr<IDetailsView> ConfigTriggerDetailsView;
	TObjectPtr<UXRConfigEditorContext> ContextObject = nullptr;
	FDelegateHandle OnMapOpenedHandle;
	FText SummaryText;
	EXRConfigEditorPage CurrentPage = EXRConfigEditorPage::Workspace;
	EXRControllerPanelSection CurrentControllerSection = EXRControllerPanelSection::General;
	EXRConfigPanelSection CurrentConfigSection = EXRConfigPanelSection::Backgrounds;
};
