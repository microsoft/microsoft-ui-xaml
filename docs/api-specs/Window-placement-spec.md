Window placement persistence
---

- [Window placement persistence](#window-placement-persistence)
- [1. Background](#1-background)
  - [1.1. Guiding principles](#11-guiding-principles)
- [2. Conceptual pages](#2-conceptual-pages)
  - [2.1. Let WinUI remember and restore your window's placement](#21-let-winui-remember-and-restore-your-windows-placement)
  - [2.2. How WinUI applies window placement data](#22-how-winui-applies-window-placement-data)
  - [2.3. Customize placement before showing the window](#23-customize-placement-before-showing-the-window)
  - [2.4. Capture and apply placement yourself](#24-capture-and-apply-placement-yourself)
  - [2.5. Special considerations for app launch and restart](#25-special-considerations-for-app-launch-and-restart)
- [3. Examples](#3-examples)
  - [3.1. Persist window placement automatically](#31-persist-window-placement-automatically)
  - [3.2. Tell WinUI your window is part of the app's launch process](#32-tell-winui-your-window-is-part-of-the-apps-launch-process)
  - [3.3. Center a window in the work area of its last-used display using AppWindow APIs](#33-center-a-window-in-the-work-area-of-its-last-used-display-using-appwindow-apis)
  - [3.4. Move saved placement to another display](#34-move-saved-placement-to-another-display)
  - [3.5. Recreate windows after an app or system restart](#35-recreate-windows-after-an-app-or-system-restart)
  - [3.6. Start a new window in full-screen mode](#36-start-a-new-window-in-full-screen-mode)
- [4. API reference](#4-api-reference)
  - [4.1. Window.PlacementId](#41-windowplacementid)
  - [4.2. Window.ApplyPlacement](#42-windowapplyplacement)
  - [4.3. Window.Activate](#43-windowactivate)
  - [4.4. Window.TryGetPlacement](#44-windowtrygetplacement)
  - [4.5. WindowActivateOptions](#45-windowactivateoptions)
  - [4.6. WindowPlacementInfo](#46-windowplacementinfo)
  - [4.7. WindowPlacementInfo.TryLoadForPlacementId and TryCreateCascadedForPlacementId](#47-windowplacementinfotryloadforplacementid-and-trycreatecascadedforplacementid)
  - [4.8. WindowPlacementInfo.TryGetDisplayArea](#48-windowplacementinfotrygetdisplayarea)
  - [4.9. WindowPlacementInfo.MoveToDisplayArea](#49-windowplacementinfomovetodisplayarea)
  - [4.10. WindowPlacementState](#410-windowplacementstate)
  - [4.11. WindowInitialPlacementReason](#411-windowinitialplacementreason)
- [5. API details](#5-api-details)
- [Appendix](#appendix)

## 1. Background
<!--
This section will not be moved to public docs.  Its purpose is to frame reasons
and context for these APIs.
Audience: folks have a little familiarity with WinUI, they want to understand how it fits
with the WinUI framework and Windows app dev ecosystem.
-->

Users may expect a reopened window to return to its previous position, size, and state.
This is an often-requested WinUI/WASDK feature.
The APIs in this spec make it easy for an app to ensure its windows reopen where the user last left them.

### 1.1. Guiding principles

These principles explain the API shape:

- **Enable the common case with one property.** Apps can just set `PlacementId` in XAML markup or
  code to let WinUI manage the window placement automatically.
- **Let `Window.Activate()` still do the work.** Existing WinUI apps already call `Window.Activate()`
  to show their windows.  For the common case, don't make app devs write more code.
- **Layer automatic placement on top of explicit APIs.** Setting `PlacementId` and calling
  `Activate()` builds directly on `WindowPlacementInfo` and `Window.ApplyPlacement`, so apps
  can dip down to the explicit APIs whenever they need custom behavior.
- **Let apps adjust the window before showing it.**
- **Let apps capture and apply placement.** Apps can capture placement data and supply it
  when placing a new or existing window.
- **Support apps with and without package identity.**
- **Do no harm.**  
  - UWP apps can move to WinUI without regressions in this area.
  - Existing WinUI apps can enable automatic placement without regressions.

## 2. Conceptual pages
<!--
This section WILL be moved to public docs.  Its purpose is to clearly explain to app authors
how to approach these APIs.  It is high-level, don't discuss corner cases yet.
Audience: App developers with a basic understanding of WinUI and no understanding of this feature area.
-->

### 2.1. Let WinUI remember and restore your window's placement

When a user opens a window, they often expect it to open in the place they last left it.
WinUI's **window placement persistence** APIs let your app reopen windows at their previous
positions.

You can tell WinUI to manage the placement of a particular window by setting `Window.PlacementId`.
If your app doesn't have package identity, you'll need to call `ApplicationData.SetContextForUnpackaged` first.

```csharp
// Only required if your app does not have package identity:
Microsoft.Windows.Storage.ApplicationData.SetContextForUnpackaged("Contoso", "Editor");

var window = new MainWindow
{
    PlacementId = "EditorMainWindow",
};
window.Activate();
```

You can set `PlacementId` in XAML markup:

```xaml
<Window
    x:Class="MyApp.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    PlacementId="EditorMainWindow">
    <Grid />
</Window>
```

WinUI will save and load the window placement data according to the `PlacementId`, so only
use the same ID for windows that should share a saved placement.
Reuse the same `PlacementId` whenever your app creates that window again.  This works across
different versions of your app.
Use different IDs for windows whose placement you want to remember independently.

For apps with package identity, WinUI stores the placement data using `ApplicationData.GetDefault().LocalSettings`, so this data
will be automatically deleted when your app is uninstalled.

For apps without package identity, WinUI stores the placement data using `ApplicationData.GetForUnpackaged().LocalSettings`.
This writes to the Windows registry, and your app's uninstaller is responsible for removing these entries.

WinUI may evict older placement entries.

> _Spec note: The ApplicationData APIs discussed above are still in progress, see [ApplicationData.GetForUnpackaged() API spec](https://github.com/microsoft/WindowsAppSDK/pull/6946)_

### 2.2. How WinUI applies window placement data

`WindowPlacementInfo` records the following information about your window:

- **Normal position and size (`NormalRect`).** WinUI remembers the outer bounds to use when
  the window is not maximized, minimized, or snapped.
- **Window state (`State`).** WinUI remembers normal, maximized, or snapped state. For
  minimized windows, it also remembers the state to use when the user restores the window.
- **Snapped position and size (`SnapRect`).** WinUI remembers the visible bounds of a
  snapped window separately from its normal bounds.
- **Display work area (`WorkArea`).** WinUI remembers the display's usable rectangle,
  excluding the taskbar and other reserved areas.
- **Display scale (`Dpi`).** WinUI remembers the display's dots per inch so it can adjust
  the window's size when scaling changes.
- **Display name (`DisplayDeviceName`).** WinUI uses the name to try to find the same
  display when your app applies the placement later.
- **Virtual desktop (`VirtualDesktopId`).** WinUI remembers the virtual desktop the
  window was on (this is only used for restart scenarios).

WinUI does not capture visibility, activation, presenter kind, or Z-order.

The following table shows how WinUI adjusts a window's position and size when the
display configuration has changed since its `WindowPlacementInfo` snapshot was captured or
saved. The goal is that the window appears where the user expects it.

| Display configuration when your app applies the placement | How WinUI adjusts saved bounds |
| --- | --- |
| The display configuration is unchanged | Apply the saved normal size and position. |
| The saved position would be off-screen | Move the window and reduce its normal size to fit the selected display's work area. |
| Display scaling changes | Adjust normal size to preserve approximately the same size in DIPs, and adapt position to the selected display's work area. |
| The taskbar or another reserved area changes | Adjust bounds to the current work area. |
| Displays are rearranged | Try to find the saved display by name, then adapt bounds to its current position, work area, and scale. |
| The saved display is disconnected | Choose the connected display with the largest intersection with saved normal bounds, or the nearest display if none intersects. |

Additional notes:

- WinUI doesn't save or load information about the `AppWindow`'s current presenter.
  Your app chooses full-screen or compact-overlay mode separately. See
  [Start a new window in full-screen mode](#36-start-a-new-window-in-full-screen-mode).
- You can set `Window.Width`, `Window.Height`, or both to choose a fallback size before
  showing the window. Saved placement, custom placement, and cascading override these values
  (see [ApplyPlacement](#42-windowapplyplacement)).
- When placing a window before it has been shown, WinUI can cascade it from an open, visible
  window with the same `PlacementId`, rather than placing both windows at the exact same
  position.

### 2.3. Customize placement before showing the window

In some cases you may not want to use WinUI's placement logic for handling display changes, or your app
might have some particular rules about its window placement you want to enforce.

You can customize the way your window is shown by running WinUI's window placement logic before
calling `Activate`:

- **Load or create a `WindowPlacementInfo` and call `Window.ApplyPlacement`.** This runs
  WinUI's window placement logic and modifies the underlying windowing object (the Windows App
  SDK `AppWindow`), without showing it yet.
- **Modify bounds, state, or presenter as needed.** Use the `Window` or `AppWindow` APIs.
- **Call `Window.Activate` with `SkipAutomaticPlacement = true` to show the prepared window.**
  Setting `SkipAutomaticPlacement = true` tells `Activate` not to run automatic `PlacementId`
  placement on top of your changes.

See [Center a window in the work area of its last-used display using AppWindow APIs](#33-center-a-window-in-the-work-area-of-its-last-used-display-using-appwindow-apis)
for an example of preparing placement, adjusting position, and then activating the window.

### 2.4. Capture and apply placement yourself

You can capture a window's position, size, state, and display information in a
`WindowPlacementInfo` object by calling `Window.TryGetPlacement`.

To apply a snapshot to a new or existing window, pass it to `Window.ApplyPlacement`.
If `PlacementId` is also set on a new window, pass `SkipAutomaticPlacement = true` when
calling `Window.Activate` so automatic placement does not overwrite the snapshot.

> _Spec note: WinUI does not currently expose `WM_DESTROY` or `WM_ENDSESSION` as public
> `Window` events. Windows may end a session without destroying each window. Complete
> custom-storage lifecycle support is tracked in
> [#12390](https://github.com/microsoft/microsoft-ui-xaml/issues/12390)._

### 2.5. Special considerations for app launch and restart

WinUI's initial placement logic works best if you give extra context about why a new window
is opening. Set `WindowActivateOptions.InitialReason` when calling `Activate` (or pass
`initialReason` to `ApplyPlacement` before the window is shown) to give this context:

- **Your window is being opened as part of your app's launch:** Set `ApplicationLaunch`.
  See [Tell WinUI your window is part of the app's launch process](#32-tell-winui-your-window-is-part-of-the-apps-launch-process).
- **Your window is being recreated after an app or system restart:** Your app detects the
  restart, recreates its windows, and calls `Window.Activate` with
  `InitialReason = ApplicationRestart`. WinUI attempts to
  return each window to its previous virtual desktop, keeps minimized windows minimized, and
  shows each window without requesting activation.
  See [Recreate windows after an app or system restart](#35-recreate-windows-after-an-app-or-system-restart).

## 3. Examples

### 3.1. Persist window placement automatically

You can let WinUI remember a window's position, size, and state when it closes and use them
when your app recreates it. Set `PlacementId` to tell WinUI
to automatically persist the window's placement:

```xaml
<Window
    x:Class="MyApp.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    PlacementId="Contoso.Editor">
    <Grid />
</Window>
```

And then call `Activate` as normal to show your window:

```csharp
Window window = new MainWindow();
window.Activate();
```

Or do it only with code:

```csharp
Window window = new MainWindow
{
    PlacementId = "Contoso.Editor",
};
window.Activate();
```

If your app does not have package identity, call `ApplicationData.SetContextForUnpackaged`
before activating your window:

```csharp
// If your app doesn't have package identity, you need to tell ApplicationData your app's context,
// WinUI uses it for its data store.
Microsoft.Windows.Storage.ApplicationData.SetContextForUnpackaged("Contoso", "Editor");
```

### 3.2. Tell WinUI your window is part of the app's launch process

Set `InitialReason` to `ApplicationLaunch` to tell WinUI to use extra startup information from the OS when
determining how to place the window (this is in the Win32 `STARTUPINFO` struct).

If the app was launched from the taskbar, WinUI will prefer the display on which that taskbar is open.
The OS may also indicate the app should launch as maximized or minimized.

```csharp
private Window? _window;

protected override void OnLaunched(LaunchActivatedEventArgs args)
{
    _window = new MainWindow();
    _window.Activate(new WindowActivateOptions
    {
        InitialReason = WindowInitialPlacementReason.ApplicationLaunch,
    });
}
```

### 3.3. Center a window in the work area of its last-used display using AppWindow APIs

Suppose you want to open a normal window centered in the work area of the display where it was
last used, even if the saved window was maximized or snapped. You can use the `ApplyPlacement` API to run WinUI's
window placement logic without making the window visible.  This allows you to make modifications
before calling `Window.Activate` to show it.

```csharp
using System;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Windows.Graphics;

Window window = new MainWindow
{
    PlacementId = "Contoso.Editor",
};

// Load the WindowPlacementInfo from WinUI's storage
WindowPlacementInfo? placement =
    WindowPlacementInfo.TryLoadForPlacementId(window.PlacementId);

// Force the window to be in the normal state (e.g. not maximized)
if (placement is not null)
{
    placement.State = WindowPlacementState.Normal;
    window.ApplyPlacement(placement);
}

AppWindow appWindow = window.AppWindow;
DisplayArea display = DisplayArea.GetFromWindowId(appWindow.Id, DisplayAreaFallback.Nearest)
    ?? throw new InvalidOperationException("Cannot find the prepared window's display.");

// WorkArea and AppWindow APIs use physical pixels
RectInt32 workArea = display.WorkArea;
int widthInPixels = 600;
int heightInPixels = 600;

appWindow.MoveAndResize(new RectInt32(
    workArea.X + (workArea.Width - widthInPixels) / 2,
    workArea.Y + (workArea.Height - heightInPixels) / 2,
    widthInPixels,
    heightInPixels));

// Skip automatic placement so Activate does not undo the move.
window.Activate(new WindowActivateOptions
{
    SkipAutomaticPlacement = true,  // we already applied placement, don't do it again
});
```

### 3.4. Move saved placement to another display

This example uses the saved display when it is connected. If the saved display is not
connected, it re-targets the snapshot to the primary display before applying it:

```csharp
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;

Window window = new MainWindow
{
    PlacementId = "Contoso.Editor",
};

WindowPlacementInfo? placement =
    WindowPlacementInfo.TryLoadForPlacementId(window.PlacementId);

if (placement is not null)
{
    // When the previous display isn't available, WinUI falls back to the display nearest
    // to the physical screen location.
    // In this example, we override this behavior by using our own logic to fall back to the
    // primary display instead.
    DisplayArea? savedDisplay = placement.TryGetDisplayArea();
    if (savedDisplay is null)
    {
        // The saved display from placement isn't connected, fall back to primary display
        placement.MoveToDisplayArea(DisplayArea.Primary);
    }

    window.ApplyPlacement(placement);
}

window.Activate(new WindowActivateOptions
{
    SkipAutomaticPlacement = true,  // we already applied placement, don't do it again
});
```

### 3.5. Recreate windows after an app or system restart

Your app can recreate its windows after a restart and ask WinUI to apply their saved
position, size, and state. Unlike an ordinary launch, a restart should:

- Keep minimized windows minimized, including the state to use when restored.
- Return windows to their saved virtual desktops when possible, without switching the
  user's active desktop.
- Avoid requesting activation as your app recreates its windows.
- Use each window's saved placement rather than cascading from another open, visible window or
  preferring the display associated with the original launch.

Activate each recreated window with `InitialReason = ApplicationRestart`.
Your app must detect the restart and load its window inventory and contents. This example
registers for Windows restart and recognizes the relaunched process through an app-defined argument.

```csharp
using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Windows.Win32; // Use CsWin32: https://microsoft.github.io/CsWin32/docs/getting-started.html

namespace MyApp;

public partial class App : Application
{
    // App-defined argument, not a Windows or WinUI option.
    private const string RestoreSessionArgument = "--restore-session";

    private static void RegisterForSystemRestart()
    {
        PInvoke.RegisterApplicationRestart(RestoreSessionArgument, 0).ThrowOnFailure();
    }

    private bool TryRestoreSessionAfterRestart()
    {
        // For redirected activations, inspect your handler's activation arguments instead.
        if (Array.IndexOf(Environment.GetCommandLineArgs(), RestoreSessionArgument) < 0)
        {
            return false;
        }

        foreach (SavedDocument savedDocument in LoadSavedDocuments())
        {
            Window window = new MainWindow
            {
                PlacementId = savedDocument.FileName,
                Title = savedDocument.Title,
            };

            // Load document contents or other app state here, before showing.
            // Honors ApplicationRestart option by NOT requesting activation.
            window.Activate(new WindowActivateOptions
            {
                InitialReason = WindowInitialPlacementReason.ApplicationRestart,
            });
        }

        return true;
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        if (TryRestoreSessionAfterRestart())
        {
            // App restore successful.
        }
        else
        {
            // Do normal window launch...
        }
        RegisterForSystemRestart();
    }

    private sealed record SavedDocument(string Title, string FileName);
    private List<SavedDocument> LoadSavedDocuments() { /* app-defined */ return []; }

}
```

For more information about how to properly support restart scenarios:

- See [Windows restart registration](https://learn.microsoft.com/windows/win32/recovery/registering-for-application-restart)
- See [AppInstance.Restart](https://learn.microsoft.com/windows/apps/windows-app-sdk/applifecycle/applifecycle-restart)

### 3.6. Start a new window in full-screen mode

Suppose you want to use automatic placement persistence, but your app also has a user setting
to start the app in full-screen mode.

To support this, load and apply the window's placement to prepare its windowed bounds,
call `AppWindow.SetPresenter(AppWindowPresenterKind.FullScreen)`, and then call `Activate`
with `SkipAutomaticPlacement = true`.

```csharp
Window window = new MainWindow
{
    PlacementId = "Contoso.Editor",
};
WindowPlacementInfo? placement =
    WindowPlacementInfo.TryCreateCascadedForPlacementId(window.PlacementId)
    ?? WindowPlacementInfo.TryLoadForPlacementId(window.PlacementId);
window.ApplyPlacement(placement);
window.AppWindow.SetPresenter(Microsoft.UI.Windowing.AppWindowPresenterKind.FullScreen);
window.Activate(new WindowActivateOptions
{
    SkipAutomaticPlacement = true,  // we already applied placement, don't do it again
});
```

When your app selects the overlapped presenter again, the window returns to its prepared
windowed placement.

## 4. API reference

### 4.1. Window.PlacementId

Set `PlacementId` to group windows that should share saved placement and cascading behavior.
This also enables WinUI's automatic placement persistence for the window.

```cs but actually midl3
String PlacementId;
```

Set this property to a non-empty string ID to enable:

- **Automatic placement persistence:** Setting `PlacementId`
  opts this window into WinUI's automatic saving (when the window closes or the Windows
  session ends) and restoring of its position, size, and state. Reuse the same ID when you
  recreate a window so WinUI can find its saved placement.
- **Cascading:** WinUI also uses this identifier to find open, visible windows
  to cascade from when placing a window before it has been shown.

When `PlacementId` is set, WinUI applies automatic placement on the first call to
`Window.Activate` (unless `SkipAutomaticPlacement` is `true` or the window has already been shown). When the window closes, if `PlacementId`
is set, WinUI will save the window's placement data.

If you show the window a different way, for example if you call `AppWindow.Show()`, WinUI will not apply placement.
In this case you may manually call `Window.ApplyPlacement` to place the window.

If your app does not have package identity, it must first call `ApplicationData.SetContextForUnpackaged`
to enable placement persistence.  This is how WinUI knows where to store the placement data.

See `Activate` and `ApplyPlacement` for more information.

### 4.2. Window.ApplyPlacement

Call this method to set the window's position, size, and state before or after showing it.

```cs but actually midl3
Boolean ApplyPlacement(WindowPlacementInfo placement);
Boolean ApplyPlacement(WindowPlacementInfo placement, WindowInitialPlacementReason initialReason);
```

Calling `ApplyPlacement(placement)` is equivalent to calling
`ApplyPlacement(placement, WindowInitialPlacementReason.Default)`.

When called, `ApplyPlacement`:
- Immediately updates the window's position, size, and state using [WinUI's placement rules](#22-how-winui-applies-window-placement-data), while preserving its current visibility (it does not show a hidden window) and without requesting activation.
- Applies the provided `WindowPlacementInfo` snapshot when `placement` is non-null, or keeps the window's existing bounds and state when `placement` is null (applying only `initialReason` launch hints if applicable). `ApplyPlacement` never cascades automatically.
- Sets the window's presenter to `OverlappedPresenter` if needed when placement is applied.
- Returns `true` if WinUI applied `placement` or launch hints from `initialReason`; otherwise returns `false` (and leaves the window's presenter unchanged).

You can call `ApplyPlacement` multiple times before or after the window is shown. If you call `ApplyPlacement` before showing a window that has `PlacementId` set, pass `SkipAutomaticPlacement = true` when calling [Window.Activate](#43-windowactivate) so `Activate` does not overwrite your placement.

**Errors**

- Call this method on the window's UI thread. WinUI reports an error before applying placement if your app calls from the wrong thread, the window is closed, or `placement` contains invalid values.
- WinUI reports `E_ILLEGAL_METHOD_CALL` if `initialReason` is not `Default` after the window has already been shown (see [Window.Activate](#43-windowactivate) for how WinUI determines whether a window has been shown), or if another `ApplyPlacement` call is in progress.

### 4.3. Window.Activate

> _Spec note: `Activate()` is an existing method; `Activate(WindowActivateOptions)` is a new overload._

Call this method to show a new window according to its initial placement, or to restore
and request activation of a window that has already been shown.

```cs but actually midl3
void Activate(); // existing
void Activate(WindowActivateOptions options);
```

This method has different behavior depending on whether the window has previously been shown:

**If the window has _not_ been shown:**
- Calling `Activate()` is equivalent to calling `Activate(new WindowActivateOptions())`.
- If `options.SkipAutomaticPlacement` is `false`:
  - If `PlacementId` is set, WinUI first calls `WindowPlacementInfo.TryCreateCascadedForPlacementId(PlacementId)` (if `options.AllowCascading` is `true` and `options.InitialReason` is not `ApplicationRestart`).
  - If `PlacementId` is set and no window was found to cascade from, WinUI calls `WindowPlacementInfo.TryLoadForPlacementId(PlacementId)`.
  - WinUI calls `ApplyPlacement(placement, options.InitialReason)` to place the window (`placement` is `null` if `PlacementId` is not set or no placement data was found).
- WinUI shows and activates the window (unless prohibited by `WindowInitialPlacementReason`).

**If the window _has_ previously been shown:**
- Only the parameterless `Activate()` is valid.  Calling `Activate(WindowActivateOptions)` returns an error.
- If the window is minimized, WinUI restores the window.
- WinUI requests window activation for the window.

WinUI considers the window to have been "shown" if it has previously received a `WM_SHOWWINDOW` message for it
(where `wParam` is `TRUE`).  Here are some ways this can happen:
- The `Window.Activate()` method has been called on the window.
- The `AppWindow.Show()` method has been called on the window.
- The win32 `ShowWindow()` function has been called for the window.

**Requesting Activation**

If the OS honors your app's request for window activation, it will bring your window to the top z-order
and give it input focus.

If the application is in the background, the OS will not bring your application to the foreground,
and the window will not be activated.  Internally, WinUI calls `SetActiveWindow`.
See `SetActiveWindow` for more information and restrictions.

**Errors**

- Call this method on the window's UI thread. WinUI reports an error if your app calls
  from the wrong thread, the window is closed, or `options` is null or contains invalid values.
- WinUI reports `E_ILLEGAL_METHOD_CALL` if `Activate(WindowActivateOptions)` is called after
  the window has already been shown, or if an `ApplyPlacement` call is in progress.

### 4.4. Window.TryGetPlacement

Call this method to capture a window's placement in a `WindowPlacementInfo` object.
Your app can pass the snapshot to `ApplyPlacement` when placing a new or existing window.

```cs but actually midl3
WindowPlacementInfo TryGetPlacement();
```

WinUI returns an independent `WindowPlacementInfo` object containing the window's position,
size, state, and display information. Your app can edit the snapshot before applying it.
See [Capture and apply placement yourself](#24-capture-and-apply-placement-yourself).

When the window uses `OverlappedPresenter`, WinUI captures its current state and normal
restore bounds (the outer position and size when the window is not maximized, minimized, or
snapped). When the window's AppWindow Presenter is not an `OverlappedPresenter`, WinUI returns the most
recent snapshot, or `null` if the window has never had one.

WinUI does not capture visibility, activation, presenter kind, or Z-order.

You can call this method while the window is hidden, including before showing it for the
first time or after hiding it.

**Errors**

- If your app calls from the wrong thread, WinUI reports the existing thread-affinity error.
- If the window is closed, WinUI reports `E_INVALID_OPERATION`.
- If the window type is unsupported, WinUI reports `E_NOTIMPL`.

### 4.5. WindowActivateOptions

Use this object to configure automatic placement and initial presentation when calling
`Window.Activate` before the window has been shown.

```cs but actually midl3
runtimeclass WindowActivateOptions
{
    WindowActivateOptions();

    Boolean SkipAutomaticPlacement;
    Boolean AllowCascading;
    WindowInitialPlacementReason InitialReason;
};
```

| Property | Default | Meaning |
| --- | --- | --- |
| `SkipAutomaticPlacement` | `false` | When `true`, `Activate` skips automatic `PlacementId` cascading and loading. Set this to `true` when you have already positioned the window before showing it (for example via `ApplyPlacement` or `AppWindow` APIs). |
| `AllowCascading` | `true` | When `true` (the default) and automatic placement runs on `Activate`, WinUI first tries to find another open, visible window from the same app with the same `PlacementId` (including in other processes) and cascade from it. WinUI never cascades when `InitialReason = ApplicationRestart`. |
| `InitialReason` | `Default` | Choose initial placement and first-presentation behavior before the window has been shown. See [WindowInitialPlacementReason](#411-windowinitialplacementreason). |

For more information about how `Activate` uses these options, see [Window.Activate](#43-windowactivate).

You can read and edit the options and placement objects on any thread.

### 4.6. WindowPlacementInfo

You can capture, load, construct, and edit a snapshot of a window's position, size, state,
and saved display information:

```cs but actually midl3
runtimeclass WindowPlacementInfo
{
    WindowPlacementInfo();

    static WindowPlacementInfo TryLoadForPlacementId(String placementId);
    static WindowPlacementInfo TryCreateCascadedForPlacementId(String placementId);

    Microsoft.UI.Windowing.DisplayArea TryGetDisplayArea();
    void MoveToDisplayArea(Microsoft.UI.Windowing.DisplayArea displayArea);

    Windows.Graphics.RectInt32 NormalRect;
    Windows.Graphics.RectInt32 WorkArea;
    Int32 Dpi;
    WindowPlacementState State;
    Windows.Foundation.IReference<Windows.Graphics.RectInt32> SnapRect;
    String DisplayDeviceName;
    Windows.Foundation.IReference<Guid> VirtualDesktopId;
};
```

| Property | Initial value | Meaning |
| --- | --- | --- |
| `NormalRect` | All fields zero | Outer position and size to use when the window is not maximized, minimized, or snapped, including title bar and borders. |
| `WorkArea` | All fields zero | Saved display's usable bounds, excluding reserved areas. |
| `Dpi` | `0` | Saved display DPI. |
| `State` | `Normal` | Windowed state, including whether the window is minimized and the state to use when restored. |
| `SnapRect` | `null` | Optional snapped visible-frame bounds, excluding invisible resize borders. |
| `DisplayDeviceName` | Empty | Optional GDI display name, not a permanent physical-device ID. |
| `VirtualDesktopId` | `null` | Optional Windows virtual-desktop ID. |

You can edit all properties. Specify rectangles in physical pixels using screen coordinates spanning
all displays.
WinUI treats null display name as empty, and an empty GUID as no virtual-desktop ID.

A newly constructed object is not ready to apply. Set `NormalRect`, `WorkArea`, and `Dpi`
to valid values before passing it to `ApplyPlacement`.

| Data | Requirement |
| --- | --- |
| `NormalRect` and `WorkArea` | Positive width and height and a positive-area intersection with each other. |
| Present `SnapRect` | Positive width and height, even when state does not use snapping. |
| All rectangles | Right and bottom edges fit signed 32-bit coordinates. |
| `Dpi` | At least 96. |
| `State` | A defined value; snapped states require a non-null snap rectangle. |
| `DisplayDeviceName` | Empty or at most 31 UTF-16 characters, without embedded null. |

### 4.7. WindowPlacementInfo.TryLoadForPlacementId and TryCreateCascadedForPlacementId

Load the placement WinUI saved for a window, or create a cascaded placement from an open
window with the same `PlacementId`, into a new `WindowPlacementInfo` instance.

```cs but actually midl3
static WindowPlacementInfo TryLoadForPlacementId(String placementId);
static WindowPlacementInfo TryCreateCascadedForPlacementId(String placementId);
```

You can call `TryLoadForPlacementId` synchronously on any thread to read your app's
placement store. WinUI returns a copy of the stored data without adjusting it for the
currently available displays.

WinUI may return null from `TryLoadForPlacementId` when storage is unavailable or
inaccessible, a record is missing or evicted, or saved data is invalid or uses an
unsupported version.

`TryCreateCascadedForPlacementId` looks for an open, visible window from the same app with
the specified `PlacementId` (including in other processes) and returns a new
`WindowPlacementInfo` cascaded from that window, or `null` if no candidate window exists.

### 4.8. WindowPlacementInfo.TryGetDisplayArea

Resolve the saved `DisplayDeviceName` against the displays connected in the current display topology.

```cs but actually midl3
Microsoft.UI.Windowing.DisplayArea TryGetDisplayArea();
```

This method returns the connected `DisplayArea` whose device name matches
`DisplayDeviceName`. It returns null when the saved display name is empty or no connected
display matches.

### 4.9. WindowPlacementInfo.MoveToDisplayArea

Re-target this placement snapshot to a display in the current display topology.

```cs but actually midl3
void MoveToDisplayArea(Microsoft.UI.Windowing.DisplayArea displayArea);
```

A `WindowPlacementInfo` object represents the screen location where a window will later be displayed.
You can use the `MoveToDisplayArea` method to move this location to a different display than
the one it's currently targeting.  WinUI will update other `WindowPlacementInfo` fields
as necessary to represent a consistent, user-friendly location on the new display.

This method changes only the `WindowPlacementInfo` object. It does not move a live `Window`.
To apply the updated snapshot to a new or existing window, pass it to
`Window.ApplyPlacement`.

The target `DisplayArea` must be non-null and represent a currently connected display. If it
is null, disconnected, or stale, WinUI reports `E_INVALIDARG` and leaves the placement
object unchanged.

### 4.10. WindowPlacementState

Use one value to describe the window's state and, when minimized, the state to use when restored.

```cs but actually midl3
enum WindowPlacementState
{
    Normal = 0,
    Maximized = 1,
    Minimized = 2,
    Snapped = 3,
    MinimizedFromMaximized = 4,
    MinimizedFromSnapped = 5,
};
```

| Value | Number | Minimized | Restore or visible state |
| --- | --- | --- | --- |
| `Normal` | 0 | No | Normal |
| `Maximized` | 1 | No | Maximized |
| `Minimized` | 2 | Yes | Normal |
| `Snapped` | 3 | No | Snapped |
| `MinimizedFromMaximized` | 4 | Yes | Maximized |
| `MinimizedFromSnapped` | 5 | Yes | Snapped |

### 4.11. WindowInitialPlacementReason

Use this enum to give WinUI context about why a new window is being created. WinUI uses it
when applying initial window placement before the window has been shown.

```cs but actually midl3
enum WindowInitialPlacementReason
{
    Default = 0,
    ApplicationLaunch = 1,
    ApplicationRestart = 2,
};
```

Set `WindowActivateOptions.InitialReason` (or pass `initialReason` to `Window.ApplyPlacement`)
before the window is shown:

| Value | Behavior |
| --- | --- |
| `Default` | Use the standard placement rules without process-launch hints or restart-specific behavior. When used, WinUI restores any minimized window. |
| `ApplicationLaunch` | Use any configuration specified by the OS for the window. For example, Windows might request the app start minimized or maximized. It might also have a hint that the app start on a specific monitor. If there are no special requests from the OS, this has the same behavior as `Default`. |
| `ApplicationRestart` | If the window was minimized, keep it minimized. If it was saved to a particular virtual desktop, attempt to move it to that virtual desktop without switching the user's active desktop. Do not cascade windows. On first `Activate`, do not request window activation. |

## 5. API details

The following MIDL3 defines the API additions. Existing `Window` members are omitted.
The new APIs are part of `Microsoft.UI.Xaml.WinUIContract`, version 12.
Projection metadata requires review.

```cs but actually midl3
namespace Microsoft.UI.Xaml
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    enum WindowPlacementState
    {
        Normal = 0,
        Maximized = 1,
        Minimized = 2,
        Snapped = 3,
        MinimizedFromMaximized = 4,
        MinimizedFromSnapped = 5,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    enum WindowInitialPlacementReason
    {
        Default = 0,
        ApplicationLaunch = 1,
        ApplicationRestart = 2,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    [threading(both)]
    [marshaling_behavior(agile)]
    runtimeclass WindowPlacementInfo
    {
        WindowPlacementInfo();

        static WindowPlacementInfo TryLoadForPlacementId(String placementId);
        static WindowPlacementInfo TryCreateCascadedForPlacementId(String placementId);

        Microsoft.UI.Windowing.DisplayArea TryGetDisplayArea();
        void MoveToDisplayArea(Microsoft.UI.Windowing.DisplayArea displayArea);

        Windows.Graphics.RectInt32 NormalRect;
        Windows.Graphics.RectInt32 WorkArea;
        Int32 Dpi;
        WindowPlacementState State;
        Windows.Foundation.IReference<Windows.Graphics.RectInt32> SnapRect;
        String DisplayDeviceName;
        Windows.Foundation.IReference<Guid> VirtualDesktopId;
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    [threading(both)]
    [marshaling_behavior(agile)]
    runtimeclass WindowActivateOptions
    {
        WindowActivateOptions();

        Boolean SkipAutomaticPlacement;
        Boolean AllowCascading;
        WindowInitialPlacementReason InitialReason;
    };

    [webhosthidden]
    unsealed runtimeclass Window
    {
        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        {
            String PlacementId;
            void Activate(WindowActivateOptions options);
            WindowPlacementInfo TryGetPlacement();
            Boolean ApplyPlacement(WindowPlacementInfo placement);
            Boolean ApplyPlacement(WindowPlacementInfo placement, WindowInitialPlacementReason initialReason);
        }
    };
}
```

## Appendix
<!-- Spec-only tracking notes; do not move this appendix to public API documentation. -->


_(nothing here right now)_
