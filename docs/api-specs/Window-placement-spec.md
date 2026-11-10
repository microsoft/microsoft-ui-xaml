Window placement persistence
---

- [Window placement persistence](#window-placement-persistence)
- [1. Background](#1-background)
  - [1.1. Guiding principles](#11-guiding-principles)
- [2. Conceptual pages](#2-conceptual-pages)
  - [2.1. Let WinUI remember and restore your window's placement](#21-let-winui-remember-and-restore-your-windows-placement)
  - [2.2. How WinUI applies window placement data to a new window](#22-how-winui-applies-window-placement-data-to-a-new-window)
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
  - [4.2. Window.ApplyInitialPlacement](#42-windowapplyinitialplacement)
  - [4.3. Window.Activate](#43-windowactivate)
  - [4.4. Window.TryGetPlacement](#44-windowtrygetplacement)
  - [4.5. WindowInitialPlacementOptions](#45-windowinitialplacementoptions)
  - [4.6. WindowPlacementInfo](#46-windowplacementinfo)
  - [4.7. WindowPlacementInfo.TryLoadForPlacementId](#47-windowplacementinfotryloadforplacementid)
  - [4.8. WindowPlacementInfo.TryGetDisplayArea](#48-windowplacementinfotrygetdisplayarea)
  - [4.9. WindowPlacementInfo.MoveToDisplayArea](#49-windowplacementinfomovetodisplayarea)
  - [4.10. WindowPlacementState](#410-windowplacementstate)
  - [4.11. WindowPlacementReason](#411-windowplacementreason)
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
- **Let apps adjust the window before showing it.**
- **Let apps capture and apply placement.** Apps can capture placement data and supply it
  when placing a new window.
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

### 2.2. How WinUI applies window placement data to a new window

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
  display when your app creates the window again.
- **Virtual desktop (`VirtualDesktopId`).** WinUI remembers the virtual desktop the
  window was on (this is only used for restart scenarios).

WinUI does not capture visibility, activation, presenter kind, or Z-order.

The following table shows how WinUI adjusts a new window's position and size when the
display configuration has changed since its `WindowPlacementInfo` snapshot was captured or
saved. The goal is that the window appears where the user expects it.

| Display configuration when your app creates the window again | How WinUI adjusts saved bounds |
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
- You can set `Window.Width`, `Window.Height`, or both to choose a fallback size. Saved
  placement, custom placement, and cascading override these values (see
  [ApplyInitialPlacement](#42-windowapplyinitialplacement)).
- WinUI can cascade a new window from an open, visible window with the same `PlacementId`,
  rather than placing both windows at the exact same position.

### 2.3. Customize placement before showing the window

In some cases you may not want to use WinUI's placement logic for handling display changes, or your app
might have some particular rules about its window placement you want to enforce.

You can customize the way your window is shown by running WinUI's window placement logic before
calling `Activate()`:

- **Call `Window.ApplyInitialPlacement` to run WinUI's window placement logic.** This will
  modify the underlying windowing object (the Windows App SDK `AppWindow`), but not show it yet.
- **Modify bounds, state, or presenter as needed.** Use the `Window` or `AppWindow` APIs.
- **Call `Window.Activate()` to show the prepared window.** WinUI does not rerun the
  placement logic if you previously called `ApplyInitialPlacement` (and it didn't fail with an error).
  The window retains all the information it needs to open in the correct state.

See [Center a window in the work area of its last-used display using AppWindow APIs](#33-center-a-window-in-the-work-area-of-its-last-used-display-using-appwindow-apis)
for an example of preparing placement, adjusting position, and then activating the window.

### 2.4. Capture and apply placement yourself

You can capture a window's position, size, state, and display information in a
`WindowPlacementInfo` object by calling `Window.TryGetPlacement`.

To use a snapshot when creating a new window, set `WindowInitialPlacementOptions.CustomPlacement`
(and set `AllowCascading = false` if you do not want cascading to override it) and call
`Window.ApplyInitialPlacement` before calling `Window.Activate()`.

> _Spec note: WinUI does not currently expose `WM_DESTROY` or `WM_ENDSESSION` as public
> `Window` events. Windows may end a session without destroying each window. Complete
> custom-storage lifecycle support is tracked in
> [#12390](https://github.com/microsoft/microsoft-ui-xaml/issues/12390)._

### 2.5. Special considerations for app launch and restart

WinUI's placement logic works best if you give extra context about why the window is opening.
Set `WindowInitialPlacementOptions.Reason` to give this context:

- **Your window is being opened as part of your app's launch:** Set `ApplicationLaunch`.
  See [Tell WinUI your window is part of the app's launch process](#32-tell-winui-your-window-is-part-of-the-apps-launch-process).
- **Your window is being recreated after an app or system restart:** Your app detects the
  restart, recreates its windows, and calls `ApplyInitialPlacement` with
  `Reason = ApplicationRestart` before calling `Window.Activate()`. WinUI attempts to return
  each window to its previous virtual desktop, keeps minimized windows minimized, and shows
  each window without requesting activation.
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

### 3.2. Tell WinUI your window is part of the app's launch process

Set `Reason` to `ApplicationLaunch` to tell WinUI to use extra startup information from the OS when
determining how to place the window (this is in the Win32 `STARTUPINFO` struct).

If the app was launched from the taskbar, WinUI will prefer the display on which that taskbar is open.
The OS may also indicate the app should launch as maximized or minimized.

```csharp
private Window? _window;

protected override void OnLaunched(LaunchActivatedEventArgs args)
{
    _window = new MainWindow();
    _window.ApplyInitialPlacement(new WindowInitialPlacementOptions
    {
        Reason = WindowPlacementReason.ApplicationLaunch,
    });
    // The first Activate remembers the state we set when calling ApplyInitialPlacement.
    _window.Activate();
}
```

### 3.3. Center a window in the work area of its last-used display using AppWindow APIs

Suppose you want to open a normal window centered in the work area of the display where it was
last used, even if the saved window was maximized or snapped. You can use the `ApplyInitialPlacement` API to run WinUI's
window placement logic without making the window visible.  This allows you to make modifications
before calling `Window.Activate()` to show it.

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
}

// Apply placement while the window remains hidden.
window.ApplyInitialPlacement(new WindowInitialPlacementOptions
{
    CustomPlacement = placement,
    AllowCascading = false,
});

AppWindow appWindow = window.AppWindow;
DisplayArea display = DisplayArea.GetFromWindowId(appWindow.Id, DisplayAreaFallback.Nearest)
    ?? throw new InvalidOperationException("Cannot find the prepared window's display.");

// WorkArea and AppWindow APIs use physical pixels
RectInt32 workArea = display.WorkArea;
SizeInt32 size = appWindow.Size;

appWindow.Move(new PointInt32(
    workArea.X + (workArea.Width - size.Width) / 2,
    workArea.Y + (workArea.Height - size.Height) / 2));

// Activate does not apply placement again or undo the move.
window.Activate();
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
    DisplayArea? savedDisplay = placement.TryGetDisplayArea();
    if (savedDisplay is null)
    {
        // The saved display from placement isn't connected, fall back to primary display
        placement.MoveToDisplayArea(DisplayArea.Primary);
    }

    window.ApplyInitialPlacement(new WindowInitialPlacementOptions
    {
        CustomPlacement = placement,
        AllowCascading = false,
    });
}

window.Activate();
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

Prepare each window with `ApplicationRestart`, then call `Window.Activate()`.
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
            window.ApplyInitialPlacement(new WindowInitialPlacementOptions
            {
                Reason = WindowPlacementReason.ApplicationRestart,
            });
            // Honors ApplicationRestart option by NOT requesting activation.
            window.Activate();
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

To support this, call `ApplyInitialPlacement` with default options to prepare the window.
Then call `AppWindow.SetPresenter(AppWindowPresenterKind.FullScreen)` before calling `Activate()`.

```csharp
Window window = new MainWindow
{
    PlacementId = "Contoso.Editor",
};
window.ApplyInitialPlacement(new WindowInitialPlacementOptions());
window.AppWindow.SetPresenter(Microsoft.UI.Windowing.AppWindowPresenterKind.FullScreen);
window.Activate();
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
  to cascade from.

WinUI automatically restores initial placement only when you call `Window.Activate()` or
`Window.ApplyInitialPlacement()`; calling `AppWindow.Show()` alone does not apply initial
placement.

If your app does not have package identity, it must first call `ApplicationData.SetContextForUnpackaged()`
to enable placement persistence.  This is how WinUI knows where to store the placement data.

See `ApplyInitialPlacement` for more information.

### 4.2. Window.ApplyInitialPlacement

Call this method to set the window's position, size, and state before showing it.

```cs but actually midl3
Boolean ApplyInitialPlacement(WindowInitialPlacementOptions options);
```

Call this method on the window's UI thread before calling `Window.Activate`.

`ApplyInitialPlacement` returns true if WinUI successfully cascaded the window, applied custom
placement for the window, or applied saved placement for the window.  Otherwise, it returns false.

WinUI chooses placement in this order:

- **Cascading:** WinUI looks for an open, visible window with the same non-empty
  `PlacementId`, if `AllowCascading` is `true` and the reason is not `ApplicationRestart`.
- **Custom placement:** WinUI uses `CustomPlacement` if your app supplies a non-null value.
- **Saved placement:** WinUI looks for saved placement under `PlacementId`.
- **OS defaults:** WinUI keeps the window's existing bounds and state, which may be OS defaults.


Normally, the first call to [Window.Activate](#43-windowactivate) runs initial placement
automatically when `PlacementId` is set. Call `ApplyInitialPlacement` directly when you want
to pass options (such as `Reason` or `CustomPlacement`) or adjust the window before showing
it:

1. Call `Window.ApplyInitialPlacement` to run WinUI's placement logic while the window is hidden.
2. Adjust the window's bounds, state, or presenter if needed.
3. Call `Window.Activate` to show the window. Because `ApplyInitialPlacement` already
   completed without an error, `Activate` will not run placement again (even if
   `ApplyInitialPlacement` returned `false`).

You can call `ApplyInitialPlacement` multiple times before calling `Window.Activate`. Each call
re-runs WinUI's placement logic. See
[How WinUI applies window placement data to a new window](#22-how-winui-applies-window-placement-data-to-a-new-window)
for how WinUI adapts saved or custom bounds to the current display configuration.

If your app does not have package identity, it must first call `ApplicationData.SetContextForUnpackaged`
to enable placement persistence.  This is how WinUI knows where to store the placement data.

**Additional behavior and errors**

- This method does not show the window or request activation.
- WinUI sets the window's presenter to `OverlappedPresenter` if needed, even when no saved
  placement is available. This also applies when the first `Activate()` runs initial placement
  automatically. To use another presenter for first presentation, select it after calling
  `ApplyInitialPlacement` and before calling `Activate()`.
- WinUI reports an error before applying placement if your app calls from the wrong thread,
  the window is closed, or the options are null or contain invalid values.
- WinUI reports `E_ILLEGAL_METHOD_CALL` if `Window.Activate` has already been called on this
  window or another `ApplyInitialPlacement` call is in progress.

### 4.3. Window.Activate

> _Spec note: This is an existing method_

Call this method to show a new window according to its initial placement, or to restore
and request activation of a window that has already been shown.

```cs but actually midl3
void Activate();
```

**Call Activate to show a new, hidden window**

On the first call to `Activate` for a window:

1. **Apply initial placement (if needed):** If `PlacementId` is set and your app hasn't
   already called `ApplyInitialPlacement` without an error, WinUI calls
   `ApplyInitialPlacement` with default options.
2. **Show and activate the window:**
   - If initial placement ran (either in step 1 or from an earlier call to
     `ApplyInitialPlacement`), WinUI shows the window using the `Reason` from that
     placement call (see table below).
   - Otherwise, WinUI shows the window, restores it if minimized, and requests activation.

| `WindowPlacementReason` | First `Activate()` |
| --- | --- |
| `Default` | Show the window and request activation. If minimized, restore it to its restore state from `WindowPlacementInfo`. |
| `ApplicationLaunch` | Honor Windows requests to start minimized or maximized. Will request activation unless it starts minimized. |
| `ApplicationRestart` | Show the window without requesting activation. Keep it minimized if it is minimized. |

**Call Activate on an already-shown window to request activation and restore from minimized**

If you've previously successfully called `Activate` on your window:
- WinUI restores the window if it's minimized.
- WinUI requests activation for the window.

**Requesting Activation**

If the OS honors your app's request for window activation, it will bring your window to the top z-order
and give it input focus.

If the application is in the background, the OS will not bring your application to the foreground,
and the window will not be activated.  Internally, WinUI calls `SetActiveWindow`.
See `SetActiveWindow` for more information and restrictions.

**Errors**

- Call this method on the window's UI thread. WinUI reports an error if your app calls
  from the wrong thread or the window is closed.
- WinUI reports `E_ILLEGAL_METHOD_CALL` if an `ApplyInitialPlacement` call is in progress.

### 4.4. Window.TryGetPlacement

Call this method to capture a window's placement in a `WindowPlacementInfo` object.
Your app can pass the snapshot to `ApplyInitialPlacement` through
`WindowInitialPlacementOptions.CustomPlacement` when creating a new window.

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

### 4.5. WindowInitialPlacementOptions

Use this object to choose how WinUI places a window when you call `ApplyInitialPlacement`.

```cs but actually midl3
runtimeclass WindowInitialPlacementOptions
{
    WindowInitialPlacementOptions();

    WindowPlacementInfo CustomPlacement;
    WindowPlacementReason Reason;
    Boolean AllowCascading;
};
```

| Property | Default | Meaning |
| --- | --- | --- |
| `CustomPlacement` | `null` | Supply a specific placement to apply. This does not disable cascading; set `AllowCascading` to false to prevent cascading. |
| `Reason` | `Default` | Choose placement and first-presentation behavior. See [WindowPlacementReason](#411-windowplacementreason). |
| `AllowCascading` | `true` | When set to true (the default), WinUI's placement logic will first try to find another open, visible window from the same app with the same `PlacementId` (including in other processes) and cascade from that window if it finds one. WinUI never cascades when `Reason = ApplicationRestart`. |

For more information about WinUI's placement logic, see [ApplyInitialPlacement](#42-windowapplyinitialplacement).

You can read and edit the options and placement objects on any thread.

### 4.6. WindowPlacementInfo

You can capture, load, construct, and edit a snapshot of a window's position, size, state,
and saved display information:

```cs but actually midl3
runtimeclass WindowPlacementInfo
{
    WindowPlacementInfo();

    static WindowPlacementInfo TryLoadForPlacementId(String placementId);

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
to valid values before passing it to `ApplyInitialPlacement`.

| Data | Requirement |
| --- | --- |
| `NormalRect` and `WorkArea` | Positive width and height and a positive-area intersection with each other. |
| Present `SnapRect` | Positive width and height, even when state does not use snapping. |
| All rectangles | Right and bottom edges fit signed 32-bit coordinates. |
| `Dpi` | At least 96. |
| `State` | A defined value; snapped states require a non-null snap rectangle. |
| `DisplayDeviceName` | Empty or at most 31 UTF-16 characters, without embedded null. |

### 4.7. WindowPlacementInfo.TryLoadForPlacementId

Load the placement WinUI saved for a window into a new `WindowPlacementInfo` instance.

```cs but actually midl3
static WindowPlacementInfo TryLoadForPlacementId(String placementId);
```

You can call this synchronous method on any thread to read your app's placement store.
WinUI returns a copy of the stored data without adjusting it for the currently available
displays.

WinUI may return null when storage is unavailable or inaccessible, a record is missing or evicted,
or saved data is invalid or uses an unsupported version.

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
To place a new window from the updated snapshot, assign it to
`WindowInitialPlacementOptions.CustomPlacement` and call `Window.ApplyInitialPlacement`.

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

### 4.11. WindowPlacementReason

Use this enum to give WinUI context about why the window is being created. WinUI uses it
when applying initial window placement.

```cs but actually midl3
enum WindowPlacementReason
{
    Default = 0,
    ApplicationLaunch = 1,
    ApplicationRestart = 2,
};
```

Set `WindowInitialPlacementOptions.Reason` to one of these values:

| Value | Behavior |
| --- | --- |
| `Default` | Use the standard placement rules without process-launch hints or restart-specific behavior. On first `Activate()`, WinUI restores any minimized window and requests activation. |
| `ApplicationLaunch` | Use any configuration specified by the OS for the window. For example, Windows might request the app start minimized or maximized. It might also have a hint that the app start on a specific monitor. If there are no special requests from the OS, this has the same behavior as `Default`. |
| `ApplicationRestart` | If the window was minimized, keep it minimized. If it was saved to a particular virtual desktop, attempt to move it to that virtual desktop without switching the user's active desktop. Do not cascade windows. On first `Activate()`, do not request window activation. |

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
    enum WindowPlacementReason
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
    runtimeclass WindowInitialPlacementOptions
    {
        WindowInitialPlacementOptions();

        WindowPlacementInfo CustomPlacement;
        WindowPlacementReason Reason;
        Boolean AllowCascading;
    };

    [webhosthidden]
    unsealed runtimeclass Window
    {
        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        {
            String PlacementId;
            WindowPlacementInfo TryGetPlacement();
            Boolean ApplyInitialPlacement(WindowInitialPlacementOptions options);
        }
    };
}
```

## Appendix
<!-- Spec-only tracking notes; do not move this appendix to public API documentation. -->


_(nothing here right now)_
