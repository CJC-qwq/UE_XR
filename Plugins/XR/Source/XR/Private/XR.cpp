// Copyright Epic Games, Inc. All Rights Reserved.

#include "XR.h"
#include "XRRuntimeDebugWorkspace.h"

#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Misc/CoreDelegates.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "FXRModule"

namespace
{
	class FXRRuntimeDebugWorkspaceInputProcessor : public IInputProcessor
	{
	public:
		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
		{
		}

		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
		{
			if (InKeyEvent.GetKey() == EKeys::F10 && !InKeyEvent.GetModifierKeys().AnyModifiersDown())
			{
				if (FXRModule::IsAvailable())
				{
					FXRModule::Get().ToggleRuntimeDebugWorkspace();
					return true;
				}
			}

			return false;
		}

		virtual const TCHAR* GetDebugName() const override
		{
			return TEXT("FXRRuntimeDebugWorkspaceInputProcessor");
		}
	};
}

FXRModule& FXRModule::Get()
{
	return FModuleManager::LoadModuleChecked<FXRModule>(TEXT("XR"));
}

bool FXRModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded(TEXT("XR"));
}

void FXRModule::StartupModule()
{
	RuntimeDebugWorkspaceManager = MakeUnique<FXRRuntimeDebugWorkspaceManager>();

	if (FSlateApplication::IsInitialized())
	{
		RegisterRuntimeDebugSupport();
	}
	else
	{
		PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FXRModule::HandlePostEngineInit);
	}
}

void FXRModule::ShutdownModule()
{
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}

	UnregisterRuntimeDebugSupport();
	RuntimeDebugWorkspaceManager.Reset();
}

void FXRModule::ShowRuntimeDebugWorkspace()
{
	if (RuntimeDebugWorkspaceManager.IsValid())
	{
		RuntimeDebugWorkspaceManager->ShowWorkspace();
	}
}

void FXRModule::HideRuntimeDebugWorkspace()
{
	if (RuntimeDebugWorkspaceManager.IsValid())
	{
		RuntimeDebugWorkspaceManager->HideWorkspace();
	}
}

void FXRModule::ToggleRuntimeDebugWorkspace()
{
	if (RuntimeDebugWorkspaceManager.IsValid())
	{
		RuntimeDebugWorkspaceManager->ToggleWorkspace();
	}
}

bool FXRModule::IsRuntimeDebugWorkspaceVisible() const
{
	return RuntimeDebugWorkspaceManager.IsValid() && RuntimeDebugWorkspaceManager->IsWorkspaceVisible();
}

void FXRModule::HandlePostEngineInit()
{
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}

	RegisterRuntimeDebugSupport();
}

void FXRModule::RegisterRuntimeDebugSupport()
{
	if (bRuntimeDebugSupportRegistered || !FSlateApplication::IsInitialized())
	{
		return;
	}

	RuntimeDebugInputProcessor = MakeShared<FXRRuntimeDebugWorkspaceInputProcessor>();
	FSlateApplication::Get().RegisterInputPreProcessor(RuntimeDebugInputProcessor, EInputPreProcessorType::Engine);

	ToggleRuntimeDebugWorkspaceCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("XR.ToggleRuntimeDebugWorkspace"),
		TEXT("Toggle the XR runtime debug workspace window."),
		FConsoleCommandDelegate::CreateRaw(this, &FXRModule::ToggleRuntimeDebugWorkspace));

	ShowRuntimeDebugWorkspaceCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("XR.ShowRuntimeDebugWorkspace"),
		TEXT("Show the XR runtime debug workspace window."),
		FConsoleCommandDelegate::CreateRaw(this, &FXRModule::ShowRuntimeDebugWorkspace));

	HideRuntimeDebugWorkspaceCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("XR.HideRuntimeDebugWorkspace"),
		TEXT("Hide the XR runtime debug workspace window."),
		FConsoleCommandDelegate::CreateRaw(this, &FXRModule::HideRuntimeDebugWorkspace));

	bRuntimeDebugSupportRegistered = true;
}

void FXRModule::UnregisterRuntimeDebugSupport()
{
	if (!bRuntimeDebugSupportRegistered)
	{
		return;
	}

	if (RuntimeDebugWorkspaceManager.IsValid())
	{
		RuntimeDebugWorkspaceManager->HideWorkspace();
	}

	if (FSlateApplication::IsInitialized() && RuntimeDebugInputProcessor.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(RuntimeDebugInputProcessor);
	}

	RuntimeDebugInputProcessor.Reset();

	if (ToggleRuntimeDebugWorkspaceCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(ToggleRuntimeDebugWorkspaceCommand);
		ToggleRuntimeDebugWorkspaceCommand = nullptr;
	}

	if (ShowRuntimeDebugWorkspaceCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(ShowRuntimeDebugWorkspaceCommand);
		ShowRuntimeDebugWorkspaceCommand = nullptr;
	}

	if (HideRuntimeDebugWorkspaceCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(HideRuntimeDebugWorkspaceCommand);
		HideRuntimeDebugWorkspaceCommand = nullptr;
	}

	bRuntimeDebugSupportRegistered = false;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FXRModule, XR)
