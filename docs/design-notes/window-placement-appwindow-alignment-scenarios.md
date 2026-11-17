# Window placement prototype scenarios

These are the scenarios we want `Window.PlacementId` to support in phase 2. Phase 2 builds on the
experimental `AppWindowPlacementDetails` APIs.

This document describes prototype behavior and proposed APIs, not released support.
Running the complete prototype requires both the WinUI implementation and a matching
AppWindow prototype build; a released AppWindow package alone is not sufficient.

The WinUI prototype implementation, tests, `Samples\WindowPlacementApp`, and this
scenario document live together on `user/jessecol/window-placement-prototype`.
The examples and expected behavior below are self-contained and do not require
access to the cross-repository design notes.

Three modes, set with `Window.PlacementMode`:

| Mode | First `Activate` | Close |
|---|---|---|
| `RestoreAndSave` (default) | Loads the saved placement and applies it with `AppWindow.SetCurrentPlacement`. | WinUI saves. |
| `SaveOnClose` | Applies nothing. The app calls `TryLoadPlacementDetails` and `AppWindow.SetCurrentPlacement` itself. | WinUI saves. |
| `Manual` | Applies nothing, same as `SaveOnClose`. | WinUI doesn't save. The app calls `SavePlacementDetails`. |

Restart behavior comes from `AppWindow.PlacementRestorationBehavior`.
Cascading is deferred. See [Cascade](#cascade).

Status key:

| Status | Meaning |
|---|---|
| Supported | Demonstrated with the prototype DLLs by a test or a manual sample-app check. Does not mean released support. |
| App-managed | The app does the work with existing APIs. WinUI doesn't test it. |
| Open | We want it, but it doesn't work yet, or the behavior isn't decided. |
| Deferred | Not planned for phase 2. |

Tests are in `dxaml\test\native\external\controls\window\WindowIntegrationTests_Placement.cpp`
in this branch. "HL n" identifies a recorded manual sample-app check driven
with Hyperloop. These coverage references describe prior prototype runs, not new runs
or validation of the current `main` branch.

Code samples are C#. A scenario has a sample only when the app has to do more than this:

```cs
window.PlacementId = "Main";
window.Activate();
```

## Summary

| Scenario | Status | Covered by |
|---|---|---|
| `first-launch-no-data` | Supported | `WindowPlacementFallbackWithoutSavedPlacement`, HL 12, HL 13 |
| `restore-normal` | Supported | `WindowPlacementRestoreNormal`, `WindowPlacementPersistMovedBounds`, HL 1 |
| `restore-maximized` | Supported | `WindowPlacementRestoreMaximized`, HL 2 |
| `restore-only-on-first-activate` | Supported | `WindowPlacementRestoresOnlyOnFirstActivation`, `WindowPlacementSettingIdAfterActivationDoesNotRestore` |
| `no-placement-id` | Supported | `WindowPlacementClearingIdDisablesSaving` |
| `save-on-every-close-path` | Supported | `WindowPlacementSavesOnlyConfirmedSessionEnd`, `WindowPlacementSavesFinalCloseHandlerChanges`, `WindowPlacementSavesOnDestroy`, `WindowPlacementSavesDuringNativeDestruction`, `WindowPlacementSavesDuringSentMessages` |
| `bad-saved-data` | Supported | `WindowPlacementRejectsInvalidRecords`, `WindowPlacementRecordCompatibility`, `WindowPlacementReportsHashCollision` |
| `restore-minimized-default` | Supported | `WindowPlacementRestoreMinimizedNormal`, `WindowPlacementRestoreMinimizedMaximized`, HL 3 |
| `restart-keep-minimized` | Supported | `WindowPlacementRestorationBehavior`, HL 4 |
| `startupinfo-minimized` | Supported (`SaveOnClose` or `Manual` mode only) | HL 11 |
| `startupinfo-minimized-no-data` | Deferred | Not tested |
| `startupinfo-maximized` | Open | Not tested |
| `restart-same-virtual-desktop` | Open | Not tested |
| `fullscreen-not-restored` | Supported | `WindowPlacementDoesNotRestoreFullScreen`, HL 5, HL G3 |
| `maximized-then-fullscreen` | Supported | `WindowPlacementDoesNotRestoreFullScreen`, HL 5, HL G1 |
| `fullscreen-before-first-activate` | Supported | `WindowPlacementSkipsUnsupportedPresenters` |
| `app-restores-fullscreen` | App-managed | Not tested |
| `restore-snapped` | Supported | `WindowPlacementRestoreSnapped` |
| `restore-minimized-snapped` | Open | `WindowPlacementDocumentsMinimizedSnappedCaptureLimitations`, `WindowPlacementRestoreMinimizedSnapped` |
| `cascade-same-process` | Deferred | Not tested |
| `cascade-new-process` | Deferred | Not tested |
| `no-cascade-by-default` | Supported | `WindowPlacementDoesNotCascadeForSharedId` |
| `multiple-ids` | Supported | `WindowPlacementSavesUsingCurrentId`, `WindowPlacementIdsAreCaseSensitive` |
| `app-loads-and-applies` | Supported | `WindowPlacementModes`, HL 9a, HL 9b |
| `app-reads-after-activate` | Supported | `WindowPlacementTryLoadPlacementDetails` |
| `app-saves-now` | Supported | `WindowPlacementSavePlacementDetails` |
| `app-overrides-size` | Supported | `WindowPlacementActivatedCanOverrideSize`, `WindowPlacementSizeChangedCanOverrideRestoredSize` |
| `app-own-storage` | App-managed | Not tested |
| `tool-window-beside-main` | App-managed | Not tested |
| `terminal-cell-sizing` | App-managed | Not tested |
| `fit-offscreen-bounds` | Supported | `WindowPlacementFitOffscreenBounds` |
| `fixed-size-window` | Supported | `WindowPlacementRespectsFixedSizing`, HL 10 |
| `display-changes` | Supported | `WindowPlacementPreferSameNameDisplay`, `WindowPlacementAdaptToReplacementDisplay`, `WindowPlacementPreserveLogicalSize`, `WindowPlacementMigratesChangedWorkArea` |
| `launch-on-display-hint` | Open | Not tested |
| `fallback-to-primary-display` | Open | Needs the proposed display-selection helpers (not in the prototype yet). Not tested. |
| `no-flash-on-apply` | Open | HL 6 (hidden contract only) |
| `restore-z-order` | Open | Not tested |
| `compact-overlay` | Open | `WindowPlacementSkipsUnsupportedPresenters` |
| `unpackaged-app` | Deferred | None |
| `intentionally-off-screen` | Deferred | None |

## Basic save and restore

Scenario `first-launch-no-data`

The first time an app runs on a machine, there's no saved data. That's normal, not an error.

1. Open a window with a `PlacementId` that has no saved record
2. Assert: Window opens at the app's size (`Width`/`Height`) or the OS default position
3. Assert: `TryLoadPlacementDetails` returns null
4. Move and resize the window, then close it
5. Re-open the window with the same id
6. Assert: Window opens normal, at the bounds from (4)

Scenario `restore-normal`

1. Open window, move and resize it fully inside the work area, then close it
2. Re-open window
3. Assert: Window is normal, with the bounds from (1)

Scenario `restore-maximized`

WinUI keeps the maximized state and the normal bounds separately.

1. Open window, move and resize it fully inside the work area
2. Maximize window, then close it
3. Re-open window
4. Assert: Window is maximized
5. Restore window
6. Assert: Window bounds are the bounds from (1)

> **STARTUPINFO.** Only a `SaveOnClose` or `Manual` app that passes `isFirstWindow: true` uses STARTUPINFO. Shipped
> AppWindow then lets `SW_SHOW` override a saved maximized state. The prototype preserves
> the saved state for neutral show commands such as `SW_SHOW`.

Scenario `restore-only-on-first-activate`

WinUI applies the saved record once, on the first `Activate`. Later calls only show or activate.

1. Open window with a saved record and call `Activate`
2. Move and resize the window, then call `Activate` again
3. Assert: Window keeps the bounds from (2)
4. Open a new window without an id and call `Activate`, then move it
5. Set `PlacementId` to an id that has a record, then call `Activate` again
6. Assert: Window keeps the bounds from (4). WinUI doesn't restore after the first `Activate`.

Scenario `no-placement-id`

A window with no `PlacementId` gets no save and no restore. Clearing the id before close turns off
saving for that window.

1. Save a placement for id "main"
2. Open a window with id "main", move it
3. Clear the id, then close the window
4. Assert: The record still has the bounds from (1)

```cs
// For example, a "don't remember window position" setting.
window.PlacementId = "";
```

Scenario `save-on-every-close-path`

WinUI saves no matter how the window goes away.

| Close path | Expected |
|---|---|
| `Window.Close` | Saves |
| User closes the window | Saves |
| `WM_ENDSESSION` with TRUE (logoff, shutdown) | Saves |
| `WM_ENDSESSION` with FALSE (shutdown canceled) | Doesn't save |
| `DestroyWindow` without `Close` | Saves |
| Window destroyed during a sent message | Saves |
| App moves the window in its `Closed` handler | Saves the moved bounds |

Not tested: process killed. Nothing saves. Use `SavePlacementDetails` (`app-saves-now`) if that
matters.

Scenario `bad-saved-data`

A bad record must never break the window. WinUI ignores it and opens the window as if there were
no saved data.

1. Write a bad record for the id: future version, zero or out-of-range DPI, empty or huge rects,
   unknown state, snapped state with no snap rect, wrong size, not binary
2. Open window with the id
3. Assert: Window opens at the app's size or the OS default, as in `first-launch-no-data`
4. Assert: A record for a different id in the same storage slot is not used

## Minimized windows and restart

Scenario `restore-minimized-default`

A user who closed a minimized window still expects to see it on the next launch.

1. Open window, move and resize it (optionally maximize it)
2. Minimize window, then close it without restoring it
3. Re-open window with the default `PlacementRestorationBehavior`
4. Assert: Window is not minimized. It is maximized if it was maximized before (2), otherwise normal
5. Assert: Normal bounds are the bounds from (1)

Scenario `restart-keep-minimized`

After a restart (for example, after an update or a reboot), the desktop should look the way the
user left it. That includes minimized windows.

1. Open window, move and resize it
2. Minimize window, then close it without restoring it
3. Re-open window. Before `Activate`, the app sets `AppWindow.PlacementRestorationBehavior` = `All`
4. Assert: Window is minimized and not activated
5. Restore window
6. Assert: Window bounds are the bounds from (1)

The app decides that this is a restart. WinUI can't tell on its own. This works in all three modes.
Instead of `All`, the app can add only the proposed `AllowShowMinimized` flag (value 16).

```cs
var window = new MainWindow();
window.PlacementId = "Main";
window.AppWindow.PlacementRestorationBehavior = PlacementRestorationBehavior.All;
window.Activate();
```

Scenario `startupinfo-minimized`

A launcher (like a shortcut set to "Run: Minimized") can ask for a minimized start. That request
wins over the saved state only in `SaveOnClose` or `Manual` mode, when the app passes
`isFirstWindow: true`. `RestoreAndSave` always passes false: WinUI cannot know whether an
earlier window used another framework or applied its own placement.

1. Open window, move and resize it, close it
2. Launch the process minimized (STARTUPINFO `wShowWindow` = `SW_SHOWMINNOACTIVE`)
3. Open window
4. Assert: Window is minimized
5. Restore window
6. Assert: Window bounds are the bounds from (1)

The default `PlacementRestorationBehavior` includes `UseStartupInfoForFirstWindow`, so the app only
needs to pass `true` for its first window:

```cs
var window = new MainWindow();
window.PlacementId = "Main";
window.PlacementMode = WindowPlacementMode.SaveOnClose;
var saved = window.TryLoadPlacementDetails();
if (saved != null)
{
    window.AppWindow.SetCurrentPlacement(saved, isFirstWindow: true);
}
window.Activate();
```

Scenario `startupinfo-maximized`

> **Open.** Not tested. We expect the same rule as `startupinfo-minimized`.

1. Open window, move and resize it, close it normal
2. Launch the process maximized (STARTUPINFO `wShowWindow` = `SW_SHOWMAXIMIZED`)
3. Open window
4. Expected: Window is maximized
5. Restore window
6. Expected: Window bounds are the bounds from (1)

Scenario `restart-same-virtual-desktop`

> **Open.** WinUI's record reserves room for a virtual desktop id, but WinUI doesn't fill or restore
> it yet. `AppWindowPlacementDetails` has no virtual desktop field.

1. Move window to virtual desktop 2, then close it
2. Re-open window with `PlacementRestorationBehavior` = `All`
3. Expected: Window opens on virtual desktop 2

Scenario `startupinfo-minimized-no-data`

> **Deferred.** The window comes up normal: WinUI has no placement to apply, and plain `Activate`
> does not explicitly apply the STARTUPINFO show command. This scenario and the interaction
> with native first-show startup handling have not been verified.

1. Clear saved data
2. Launch the process minimized
3. Open window
4. Expected: Window is minimized

## Full screen

WinUI never saves or restores full screen. The app decides at launch whether to go full screen.

Scenario `fullscreen-not-restored`

1. Open window, move and resize it
2. Switch to the FullScreen presenter, then close the window
3. Re-open window, in either mode and with any `PlacementRestorationBehavior`
4. Assert: Window uses the Overlapped presenter and is normal, with the bounds from (1)

Scenario `maximized-then-fullscreen`

The window should come back in the state it was in before it went full screen.

1. Open window, move and resize it, then maximize it
2. Switch to the FullScreen presenter, then close the window
3. Re-open window
4. Assert: Window uses the Overlapped presenter and is maximized
5. Restore window
6. Assert: Window bounds are the bounds from (1)

If the app wants the window to come back full screen, see `app-restores-fullscreen`.

Scenario `app-restores-fullscreen`

An app that wants full screen back stores that bit itself, then sets the presenter after
`Activate`. WinUI restores the Overlapped state underneath, so leaving full screen goes back to
it.

1. Open window, move and resize it, maximize it
2. Switch to the FullScreen presenter, then close the window
3. Re-open window. The app sets the FullScreen presenter after `Activate`.
4. Expected: Window is full screen
5. Switch to the Overlapped presenter
6. Expected: Window is maximized. Restore gives the bounds from (1).

```cs
var settings = ApplicationData.Current.LocalSettings.Values;
var window = new VideoWindow();
window.PlacementId = "Video";
window.Closed += (s, e) =>
{
    settings["Video.WasFullScreen"] =
        window.AppWindow.Presenter.Kind == AppWindowPresenterKind.FullScreen;
};

window.Activate();
if (settings["Video.WasFullScreen"] is true)
{
    window.AppWindow.SetPresenter(AppWindowPresenterKind.FullScreen);
}
```

> Not tested. The window may show Overlapped for a moment before it goes full screen.

Scenario `fullscreen-before-first-activate`

An app that goes full screen before its first `Activate`, with no saved data, has no real normal
bounds. WinUI must not save the default `CreateWindow` rect over a good record.

1. Save a placement for the id (open, move, close)
2. Open a new window with the same id. Set the FullScreen presenter, then call `Activate`
3. Assert: Window stays full screen
4. Close window
5. Assert: The saved normal bounds are still the ones from (1)

```cs
var window = new VideoWindow();
window.PlacementId = "Video";
window.AppWindow.SetPresenter(AppWindowPresenterKind.FullScreen);
window.Activate(); // Full screen. WinUI doesn't apply the saved record.
```

## Snapped windows

Scenario `restore-snapped`

1. Open window with Windows snapping on, move and resize it fully inside the work area
2. Snap window to the left side of the display, then close it
3. Re-open window
4. Assert: Window is snapped to the left side, at about the same bounds as after (2)
5. Restore window out of snap
6. Assert: Window bounds are the bounds from (1)

Scenario `restore-minimized-snapped`

> **Open.** Same as phase 1. WinUI doesn't reliably capture the snapped layout when a snapped
> window is minimized and then closed without being restored. Restoring from a good
> minimized-from-snapped record works (`WindowPlacementRestoreMinimizedSnapped`). The gap is in
> saving.

1. Snap window, minimize it, close it without restoring it
2. Re-open window
3. Expected: Window is snapped

## Cascade

> **Deferred.** Cut from the first batch to keep it small. Plan: add it back in the WinUI layer
> (for example `Window.TryCreateCascadedPlacement`) on public AppWindow APIs. No AppWindow change
> needed. Until then, an app can cascade itself in `SaveOnClose` mode: `GetCurrentPlacement` on the open
> window, offset `NormalRect`, then `AppWindowPlacementDetails.Create` and `SetCurrentPlacement`.

Scenario `cascade-same-process`

When the app opens a second window with the same id, it shouldn't land exactly on top of the first.

1. Open window A with id "doc" and move it
2. Open window B with id "doc" while A is visible
3. Expected: B is offset from A by one caption height, down and to the right, inside the work area

Scenario `cascade-new-process`

Same as `cascade-same-process`, but B is in another process of the same app.

Scenario `no-cascade-by-default`

1. Save a placement for id "doc"
2. Open window A with id "doc", then move it somewhere else
3. Open window B with id "doc" in `RestoreAndSave` mode
4. Assert: B opens at the saved bounds from (1), not offset from A

## Multiple ids

Scenario `multiple-ids`

Each id has its own record. Ids are case-sensitive. WinUI saves under the id the window has when it
closes.

1. Open window A with id "main" and window B with id "tools"
2. Move them to different places, then close both
3. Re-open both
4. Assert: Each window has its own saved bounds
5. Assert: Ids "Main" and "main" have separate records
6. Open a window with id "one", change its id to "two", then close it
7. Assert: The record is saved under "two". "one" has no record.

## App-controlled placement

Scenario `app-loads-and-applies`

An app can read the saved details and apply them itself, for example to center the saved size.
The app sets `PlacementMode` = `SaveOnClose`, so WinUI stays out of the way on `Activate`.

1. Save a placement for the id
2. Open a new window with the same id. Set `PlacementMode` = `SaveOnClose`, then call
   `TryLoadPlacementDetails` before the first `Activate`
3. Place the window itself. Either call `AppWindow.SetCurrentPlacement` with the details (optionally
   changed), or ignore them and call `MoveAndResize`
4. Call `Activate`
5. Assert: Window has the app's placement from (3). WinUI doesn't apply the saved record.
6. Close window
7. Assert: WinUI still saves the placement on close. (In `Manual` mode it doesn't.)

`TryLoadPlacementDetails` doesn't change the mode. In `RestoreAndSave` mode, WinUI still applies the
saved record on `Activate`, even if the app called `TryLoadPlacementDetails` first.

This example centers the saved size within its saved work area. AppWindow then adapts
the placement to the current displays.

```cs
var window = new MainWindow();
window.PlacementId = "Main";
window.PlacementMode = WindowPlacementMode.SaveOnClose;

AppWindowPlacementDetails saved = window.TryLoadPlacementDetails();
if (saved != null)
{
    RectInt32 work = saved.WorkArea;
    RectInt32 r = saved.NormalRect;
    r.X = work.X + (work.Width - r.Width) / 2;
    r.Y = work.Y + (work.Height - r.Height) / 2;

    var centered = AppWindowPlacementDetails.Create(
        r, saved.WorkArea, saved.Dpi, saved.ShowCmd,
        saved.ArrangeRect, saved.Flags, saved.DeviceName);

    // Pass true only for the app's first window.
    window.AppWindow.SetCurrentPlacement(centered, isFirstWindow: true);
}
else
{
    window.AppWindow.Resize(new SizeInt32(1200, 800));
}
window.Activate();
```

Scenario `app-reads-after-activate`

1. Save a placement for the id
2. Open window with the same id and call `Activate`
3. Call `TryLoadPlacementDetails`
4. Assert: It returns the saved details
5. Assert: Calling it after the window closes throws `E_INVALID_OPERATION`

Scenario `app-saves-now`

An app can save at a time it chooses, for example before a risky operation, so a crash doesn't
lose the user's layout.

1. Open window with an id, move it
2. Call `SavePlacementDetails`
3. Assert: The record has the bounds from (1), without closing the window
4. Maximize window, call `SavePlacementDetails`
5. Assert: The record is maximized, with the normal bounds from (1)
6. Switch to CompactOverlay, move it, call `SavePlacementDetails`
7. Assert: The record from (4) is unchanged
8. Assert: `SavePlacementDetails` with an empty `PlacementId` does nothing
9. Assert: `SavePlacementDetails` after the window closes throws `E_INVALID_OPERATION`

Not tested: kill the process after (2) and check that the next launch uses the saved record.

Scenario `app-overrides-size`

The app can change the size in its `Activated` or `SizeChanged` handler. The app's size wins over
the restored size.

1. Save a placement for the id
2. Open window with the id. In the first `SizeChanged` (or `Activated`), set `Width` and `Height`.
3. Assert: Window has the app's size from (2)

```cs
bool sized = false;
window.SizeChanged += (s, e) =>
{
    if (!sized) { sized = true; window.Width = 360; window.Height = 270; }
};
window.Activate();
```

Scenario `app-own-storage`

An app can keep placement in its own storage (a file, the cloud, a profile) and skip
`PlacementId`. There's no serialize API. The app stores the seven `AppWindowPlacementDetails`
properties itself.

1. Open window without an id, move it
2. Before close, call `AppWindow.GetCurrentPlacement` and store the result
3. Open a new window, call `AppWindow.SetCurrentPlacement` with the stored details, then `Activate`
4. Expected: Window has the bounds from (1). WinUI does nothing extra.

`MyStore` below represents the app's own serialization and storage code.

```cs
AppWindowPlacementDetails current = window.AppWindow.GetCurrentPlacement();
MyStore.Save("Main", current);

var newWindow = new MainWindow();
AppWindowPlacementDetails saved = MyStore.Load("Main");
if (saved != null)
{
    newWindow.AppWindow.SetCurrentPlacement(saved, isFirstWindow: true);
}
newWindow.Activate();
```

Scenario `tool-window-beside-main`

A tool window opens next to the main window instead of at its own saved spot. The tool window has
no `PlacementId`.

1. Open the main window with an id
2. Open the tool window, positioned from the main window's bounds
3. Expected: Tool window is beside the main window

```cs
main.PlacementId = "Main";
main.Activate();

var tools = new ToolsWindow(); // No PlacementId.
PointInt32 p = main.AppWindow.Position;
SizeInt32 sz = main.AppWindow.Size;
tools.AppWindow.MoveAndResize(new RectInt32(p.X + sz.Width, p.Y, 300, sz.Height));
tools.Activate();
```

> The app has to keep the tool window on screen itself. For example, put it on the left when there
> is no room on the right.

Scenario `terminal-cell-sizing`

A terminal or grid app wants a client size that is a whole number of cells.

1. Save a placement for the id
2. Open window, apply the saved placement, round the client size down to whole cells
3. Expected: Client size is a multiple of the cell size

```cs
// SaveOnClose mode, so Activate() doesn't apply the saved placement again and undo the rounding.
window.PlacementMode = WindowPlacementMode.SaveOnClose;

AppWindowPlacementDetails saved = window.TryLoadPlacementDetails();
if (saved != null)
{
    window.AppWindow.SetCurrentPlacement(saved, isFirstWindow: true);
}
if (window.AppWindow.Presenter is OverlappedPresenter { State: OverlappedPresenterState.Restored })
{
    SizeInt32 c = window.AppWindow.ClientSize;
    window.AppWindow.ResizeClient(new SizeInt32(
        c.Width - c.Width % cellWidth, c.Height - c.Height % cellHeight));
}
window.Activate();
```

> In the prototype, `SetCurrentPlacement` keeps a hidden window hidden, so the user doesn't
> see the resize. Shipped AppWindow shows the window during the call.

## Displays

Scenario `fit-offscreen-bounds`

A usable window matters more than an exact position.

1. Open a resizable window, move it partly off-screen, close it
2. Re-open window
3. Assert: Window fits inside the work area

Scenario `fixed-size-window`

1. Save a record that is bigger than the work area
2. Open a resizable window with the id
3. Assert: Window fits inside the work area
4. Open a window with `IsResizable` = false and the same id
5. Assert: Window keeps its own size or takes the saved size, not a mix
6. Assert: The title bar is inside the work area, so the user can move the window

> The window's current `IsResizable` decides shrink vs move, not the value saved in the record.
> In the prototype, `SetCurrentPlacement` reads it from the live window.
> The app must set `IsResizable` before the first `Activate`.

Scenario `display-changes`

Same rules as phase 1. AppWindow's PlacementEx now does this work.

| Change | Expected |
|---|---|
| Displays rearranged | Window opens on the display with the same name, at the same work-area offset |
| Saved display removed, new one added | Window opens on an available display, at the same work-area offset |
| Scaling changed | Window keeps about the same logical size |
| No display near the saved bounds | Window opens on the nearest display, not always the primary |
| Work area changed (taskbar moved or resized) | Window keeps its work-area offset. Works for normal, maximized, snapped, and minimized-from-snapped. |

> **Known limitation.** The prototype's `SetCurrentPlacement` fits the window to the monitor.
> Apps that rely on off-monitor or oversized windows will see a change.

Scenario `launch-on-display-hint`

> **Open.** Not tested. A launcher can say which display to use (STARTUPINFO `hStdOutput` monitor
> hint, for example from the taskbar on a second display). `RestoreAndSave` mode never honors STARTUPINFO.
> In the other modes, the app passes `isFirstWindow: true` to use it.

1. Clear saved data
2. Launch the app from the taskbar on display 2
3. Expected: Window opens on display 2

Scenario `fallback-to-primary-display`

> **Open.** Needs the proposed `TryGetDisplayArea()` and `MoveToDisplayArea(DisplayArea)`
> helpers on `AppWindowPlacementDetails`. Neither is in the prototype yet. By default, a window
> whose saved display is gone opens on the
> nearest display (see `display-changes`). Some apps want the primary display instead. Today the
> app can't tell that the saved display is gone, and can't redo PlacementEx's work-area and DPI
> math itself.

1. Save a placement on display 2, maximized
2. Disconnect display 2
3. Open a window with the same id. The app moves the saved details to the primary display.
4. Expected: Window opens on the primary display, maximized, with about the same logical size

```cs
var window = new MainWindow();
window.PlacementId = "Main";

// SaveOnClose mode, so Activate() doesn't apply the saved placement again on the nearest display.
window.PlacementMode = WindowPlacementMode.SaveOnClose;

AppWindowPlacementDetails saved = window.TryLoadPlacementDetails();
if (saved != null)
{
    // Proposed API, not in the prototype yet. Null if the saved display is not connected.
    if (saved.TryGetDisplayArea() == null)
    {
        // Proposed API, not in the prototype yet. Returns new details fitted to that display.
        saved = saved.MoveToDisplayArea(DisplayArea.Primary);
    }
    window.AppWindow.SetCurrentPlacement(saved, isFirstWindow: true);
}

window.Activate(); // Shows only. SaveOnClose mode.
```

## Open and deferred

Scenario `no-flash-on-apply`

> **Open.** Shipped AppWindow's `SetCurrentPlacement` shows and activates the window before WinUI's
> own show code runs. The prototype changes the call to preserve visibility and avoid activation.
> HL 6 confirms the window stays hidden and inactive during the call. We haven't tested for a
> visible flash with frame recording.

1. Open window with saved data
2. Expected: The window appears once, at its final bounds, with no visible move

Smaller open and deferred scenarios:

| Scenario | Status | Notes |
|---|---|---|
| `restore-z-order` | Open | WinUI doesn't save z-order between windows. An app that cares sets it after it opens its windows, for example `docB.AppWindow.MoveInZOrderBelow(docA.AppWindow.Id)`. |
| `compact-overlay` | Open | WinUI doesn't save placement while the window uses the CompactOverlay presenter. AppWindow doesn't remember the pre-CompactOverlay rect. |
| `unpackaged-app` | Deferred | Same as phase 1. Without package identity, `TryLoadPlacementDetails` returns null and saving does nothing. The window still opens normally. |
| `intentionally-off-screen` | Deferred | WinUI and AppWindow always fit the window on screen. An app that wants a window off screen moves it after `Activate`. |
