#pragma once

#include "Modules/ModuleInterface.h"

class FXRConfigEditorModule : public IModuleInterface
{
public:
	static const FName ConfigPanelTabName;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void UnregisterMenus();
};
