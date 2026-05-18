# XR Runtime Debug Workspace

## 1. Purpose

`XR Runtime Debug Workspace` is a debug window for both:

* Switchboard / nDisplay runtime launches
* Unreal Editor world / PIE preview debugging

It is designed for live scene debugging after the project is already on screen:

* The configuration display is read-only
* Runtime trigger actions remain available
* The window is opened as a separate desktop window instead of rendering inside the nDisplay output
* The same workspace can also fall back to the current editor world when no runtime world is active

## 2. What it can do

The runtime workspace can:

* show the active `AXRSceneTriggerController`
* show the currently bound `TriggerConfig`
* show OSC runtime status
* show active background / effect / generic action state
* inspect configured backgrounds, effects, generic actions, and trigger actions
* execute `TriggerInteger`
* execute `StopAllAndReset`

The runtime workspace cannot:

* edit controller properties
* edit config asset data
* save any configuration changes

## 3. How to open it

Default runtime hotkey:

* `F10`

Blueprint / C++ runtime entry points are also available through `UXRBPLibrary`:

* `ShowXRRuntimeDebugWorkspace`
* `HideXRRuntimeDebugWorkspace`
* `ToggleXRRuntimeDebugWorkspace`
* `IsXRRuntimeDebugWorkspaceVisible`

Console commands are also registered:

* `XR.ShowRuntimeDebugWorkspace`
* `XR.HideRuntimeDebugWorkspace`
* `XR.ToggleRuntimeDebugWorkspace`

## 4. Display behavior

This workspace is intentionally **not** drawn inside the game viewport.

Instead, it opens as an independent Slate window with these placement rules:

* it first tries to open on the monitor where the mouse currently is
* if that monitor overlaps the active game / nDisplay window, it is treated as blocked
* when blocked, the workspace automatically falls back to the nearest non-nDisplay monitor

This is meant for the exact use case where:

* Switchboard starts the render process on the same machine
* at least one extra non-nDisplay monitor is available for backstage debugging

In the editor:

* if PIE / Simulate is active, the workspace targets the PIE world first
* if no PIE world is active, it falls back to the current editor world and can still drive preview-style trigger debugging

## 5. Recommended live-debug workflow

1. Start the project through Switchboard / nDisplay
2. Move the mouse to the non-nDisplay monitor
3. Press `F10`
4. Use:
   * `Run Trigger`
   * per-trigger `Run`
   * `Stop All And Reset`
5. Press `F10` again or click `Hide` when done

## 6. Editor usage

You can also use the same window while working in Unreal Editor:

1. Open the map that contains `AXRSceneTriggerController`
2. Press `F10`
3. If PIE is active, the workspace controls the PIE world
4. If PIE is not active, the workspace falls back to the current editor world
