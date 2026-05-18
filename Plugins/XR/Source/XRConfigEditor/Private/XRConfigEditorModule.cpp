#include "XRConfigEditorModule.h"

#include "LevelEditor.h"
#include "SXRConfigEditorPanel.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

const FName FXRConfigEditorModule::ConfigPanelTabName(TEXT("XRConfigEditorPanel"));

#define LOCTEXT_NAMESPACE "FXRConfigEditorModule"

void FXRConfigEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ConfigPanelTabName,
		FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&)
		{
			return SNew(SDockTab)
				.TabRole(ETabRole::NomadTab)
				[
					SNew(SXRConfigEditorPanel)
				];
		}))
		.SetDisplayName(LOCTEXT("XRConfigPanelTitle", "XR Config"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FXRConfigEditorModule::RegisterMenus));
}

void FXRConfigEditorModule::ShutdownModule()
{
	UnregisterMenus();

	if (FModuleManager::Get().IsModuleLoaded("WorkspaceMenuStructure"))
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ConfigPanelTabName);
	}
}

void FXRConfigEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	FToolMenuSection& Section = WindowMenu->FindOrAddSection("WindowLayout");
	Section.AddMenuEntry(
		"OpenXRConfigPanel",
		LOCTEXT("OpenXRConfigPanelLabel", "XR Config"),
		LOCTEXT("OpenXRConfigPanelTooltip", "Open the XR configuration workspace."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FXRConfigEditorModule::ConfigPanelTabName);
		})));

	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("PluginTools");
	ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
		"OpenXRConfigPanelToolbar",
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FXRConfigEditorModule::ConfigPanelTabName);
		})),
		LOCTEXT("OpenXRConfigToolbarLabel", "XR Config"),
		LOCTEXT("OpenXRConfigToolbarTooltip", "Open the XR configuration workspace."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.GameSettings")));
}

void FXRConfigEditorModule::UnregisterMenus()
{
	if (UToolMenus::TryGet())
	{
		UToolMenus::UnregisterOwner(this);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FXRConfigEditorModule, XRConfigEditor)
