#pragma once

#include "CoreMinimal.h"

class AXRSceneTriggerController;
class SWindow;
class SXRRuntimeDebugWorkspacePanel;

class FXRRuntimeDebugWorkspaceManager
{
public:
	FXRRuntimeDebugWorkspaceManager();
	~FXRRuntimeDebugWorkspaceManager();

	void ShowWorkspace();
	void HideWorkspace();
	void ToggleWorkspace();
	bool IsWorkspaceVisible() const;

	AXRSceneTriggerController* GetActiveController() const;
	bool ExecuteTrigger(int32 TriggerId, FText& OutMessage);
	void StopAll(FText& OutMessage);

private:
	void CreateWorkspaceWindow(const FVector2D& WindowPosition, const FVector2D& WindowSize);
	void HandleWindowClosed(const TSharedRef<SWindow>& ClosedWindow);
	FVector2D ResolveWorkspacePosition(const FVector2D& DesiredWindowSize) const;
	UWorld* ResolveRuntimeWorld() const;
	AXRSceneTriggerController* ResolveController() const;

	TSharedPtr<SWindow> WorkspaceWindow;
	TSharedPtr<SXRRuntimeDebugWorkspacePanel> WorkspaceWidget;
};
