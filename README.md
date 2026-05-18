# UE_XR

`UE_XR` is an Unreal Engine 5.5 project that hosts the `XR` plugin used for stage / installation style scene control.

The core workflow is:

* define trigger-driven behavior in a `UXRSceneTriggerConfig` data asset
* place an `AXRSceneTriggerController` in the persistent level
* switch background sublevels, trigger effect sublevels, and dispatch generic actions
* optionally drive the system from OSC
* debug it in editor, PIE, or Switchboard / nDisplay runtime through the built-in XR tools

## Current capabilities

* Trigger-based scene scheduler for persistent-level streaming setups
* Background level switching with cleanup of old background / effect state
* Effect level triggering with explicit Level Sequence playback
* Generic Action dispatch to:
  * tagged actors in the persistent level
  * persistent level blueprint
* OSC receiver built on Unreal's OSC plugin
* `XR Config` editor panel for controller / config inspection and editing
* `XR Runtime Debug Workspace`:
  * available in runtime
  * available in PIE / Simulate
  * falls back to editor world when no runtime world is active
  * supports trigger execution and stop/reset without rendering into nDisplay output

## Requirements

* Unreal Engine `5.5`
* Visual Studio 2022 for C++ builds on Windows

The project already enables these engine plugins in [XR_Running.uproject](XR_Running.uproject):

* `OSC`
* `RemoteControlProtocolOSC`
* `ModelingToolsEditorMode` (editor only)

## Project layout

* [Plugins/XR](Plugins/XR)
  * runtime module: `XR`
  * editor module: `XRConfigEditor`
* [Content](Content)
  * host project content / maps used for testing and integration
* [docs](docs)
  * usage notes for OSC, generic actions, XR config panel, runtime debug workspace, and change history
* [tools](tools)
  * local helper scripts such as OSC sender testing

## Key runtime objects

### `AXRSceneTriggerController`

Main runtime entry point for:

* TriggerId execution
* OSC receive
* background / effect level control
* generic action dispatch
* preview / stop behavior

### `UXRSceneTriggerConfig`

Data asset that defines:

* background levels
* effect levels
* generic actions
* trigger action routing

## OSC notes

The current plugin matches incoming OSC messages against `OSC Address Path`.

For compatibility with an older Blueprint-based workflow in this project:

* the old Blueprint `CreateOSCServer` node's `ServerName`
* can be migrated directly into the current controller's `OSC Address Path`

Example:

* old Blueprint `Port = 2302`
* old Blueprint `ServerName = /Level_cut`

Current XR controller setup:

* `OSC Receive Port = 2302`
* `OSC Address Path = /Level_cut`

See [docs/XR_OSC_RECEIVER_USAGE.md](docs/XR_OSC_RECEIVER_USAGE.md) for the full setup and testing flow.

## Build

Open the project in Unreal Engine 5.5, or build from command line:

```powershell
D:\Unreal\UE_5.5\Engine\Build\BatchFiles\Build.bat XR_RunningEditor Win64 Development D:\Unreal\Project\C++\PluginsProject\XR_Running\XR_Running.uproject -WaitMutex -NoHotReload
```

## Useful docs

* [docs/XR_OSC_RECEIVER_USAGE.md](docs/XR_OSC_RECEIVER_USAGE.md)
* [docs/XR_CONFIG_PANEL_USAGE.md](docs/XR_CONFIG_PANEL_USAGE.md)
* [docs/XR_GENERIC_ACTION_USAGE.md](docs/XR_GENERIC_ACTION_USAGE.md)
* [docs/XR_RUNTIME_DEBUG_WORKSPACE.md](docs/XR_RUNTIME_DEBUG_WORKSPACE.md)
* [docs/OSC_LOCAL_TESTING.md](docs/OSC_LOCAL_TESTING.md)
* [docs/PROJECT_CHANGES.md](docs/PROJECT_CHANGES.md)

## Repository note

This repository intentionally excludes generated Unreal build products and caches such as:

* `Binaries`
* `Intermediate`
* `Saved`
* `DerivedDataCache`
* local backup archives

That keeps the repo focused on source, assets, docs, and tools.
