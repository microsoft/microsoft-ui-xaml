Window placement persistence
---

See the [window placement scenarios](Window-placement-scenarios.md) for illustrated use
cases, app code, and acceptance assertions.

- [Window placement persistence](#window-placement-persistence)
- [1. Background](#1-background)
  - [1.1. Guiding principles](#11-guiding-principles)
- [2. Conceptual pages](#2-conceptual-pages)
  - [2.1. Let WinUI remember and restore your window's placement](#21-let-winui-remember-and-restore-your-windows-placement)
  - [2.2. How WinUI applies window placement data to a new window](#22-how-winui-applies-window-placement-data-to-a-new-window)
  - [2.3. Customize placement before showing the window](#23-customize-placement-before-showing-the-window)
  - [2.4. Handle the saving and loading of your window's placement data yourself](#24-handle-the-saving-and-loading-of-your-windows-placement-data-yourself)
  - [2.5. Special considerations for app launch and restart](#25-special-considerations-for-app-launch-and-restart)
- [3. Examples](#3-examples)
  - [3.1. Persist window placement automatically](#31-persist-window-placement-automatically)
  - [3.2. Tell WinUI your window is part of the app's launch process](#32-tell-winui-your-window-is-part-of-the-apps-launch-process)
  - [3.3. Center a window on the display where it was last used](#33-center-a-window-on-the-display-where-it-was-last-used)
  - [3.4. Save and load placement in your own storage](#34-save-and-load-placement-in-your-own-storage)
  - [3.5. Recreate windows after an app or system restart](#35-recreate-windows-after-an-app-or-system-restart)
  - [3.6. Start a new window in full-screen mode](#36-start-a-new-window-in-full-screen-mode)
- [4. API reference](#4-api-reference)
  - [4.1. Window.PlacementId](#41-windowplacementid)
  - [4.2. Window.PreventAutomaticPlacementPersistence](#42-windowpreventautomaticplacementpersistence)
  - [4.3. Window.ApplyInitialPlacement](#43-windowapplyinitialplacement)
  - [4.4. Window.Activate](#44-windowactivate)
  - [4.5. Window.Show](#45-windowshow)
  - [4.6. Window.TryGetPlacement](#46-windowtrygetplacement)
  - [4.7. WindowInitialPlacementOptions](#47-windowinitialplacementoptions)
  - [4.8. WindowPlacementInfo](#48-windowplacementinfo)
  - [4.9. WindowPlacementInfo.TryLoadForPlacementId](#49-windowplacementinfotryloadforplacementid)
  - [4.10. WindowPlacementState](#410-windowplacementstate)
  - [4.11. WindowPlacementReason](#411-windowplacementreason)
- [5. API details](#5-api-details)

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
- **Let apps own storage.** By default WinUI will manage the storage of the placement data, but
  apps can also save/load/apply the window placement data if they wish.
- **Support apps with package identity first.** We'll plan to flesh out the story for apps
  without package identity in a later update.
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

If your app has package identity, you can tell WinUI to manage the placement of a particular
window by setting `Window.PlacementId`:

```csharp
var window = new MainWindow
{
    PlacementId = "Contoso.Editor",
};
window.Activate();
```

Or in XAML markup:

```xaml
<Window
    x:Class="MyApp.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    PlacementId="Contoso.Editor">
    <Grid />
</Window>
```

WinUI will save and load the window placement data according to the `PlacementId`, so only
use the same ID for windows that should share a saved placement.
Reuse the same `PlacementId` whenever your app creates that window again, including across
app versions. Use different IDs for windows whose placement you want to remember independently.

WinUI stores the placement data using `ApplicationData.GetDefault().LocalSettings`, so this data
will be automatically deleted when your app is uninstalled.  WinUI may also evict older entries.

If your app doesn't have package identity, see the instructions below about using custom storage for window placement data.

> Spec note: We may add support for automatic placement persistence for apps without package identity in the future.

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

The following table shows how WinUI adjusts a new window's position and size when the
display configuration has changed since its placement was saved.  The goal is that the
window appears where the user expects it.

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
- You can set `Window.Width`, `Window.Height`, or both to choose a fallback size. WinUI uses
  these values only when it does not apply custom placement, cascade from another open,
  visible window, or load saved placement.
- WinUI can cascade a new window from an open, visible window with the same `PlacementId`,
  rather than placing both windows at the exact same position.

### 2.3. Customize placement before showing the window

In some cases you may not want to use WinUI's placement logic for handling display changes, or your app
might have some particular rules about its window placement you want to enforce.

You can customize the way your window is shown by running WinUI's window placement logic before
calling `Activate()` or `Show(bool)`:

- **Call `Window.ApplyInitialPlacement` to run WinUI's window placement logic.** This will
modify the underlying windowing object (the WindowsAppSDK `AppWindow`), but not show it yet.
- **Modify the window state or bounds as needed.** You can use the `Window` or `AppWindow` APIs.
- **Call `Window.Activate()` to show the window and request activation, or `Window.Show(false)`
  to show it without requesting activation.** WinUI does not repeat the placement logic after
  your explicit `ApplyInitialPlacement` call completes without error.

See [Center a window on the display where it was last used](#33-center-a-window-on-the-display-where-it-was-last-used)
for an example of preparing placement, adjusting position, and then activating the window.

### 2.4. Handle the saving and loading of your window's placement data yourself

You can handle the storage of the window position data yourself.

See [Save and load placement in your own storage](#34-save-and-load-placement-in-your-own-storage)
for an example.

### 2.5. Special considerations for app launch and restart

WinUI's placement logic works best if you give extra context about why the window is opening.
Set `WindowInitialPlacementOptions.Reason` to give this context:

- **Your window is being opened as part of your app's launch:** Set `ApplicationLaunch`.
  See [Tell WinUI your window is part of the app's launch process](#32-tell-winui-your-window-is-part-of-the-apps-launch-process).
- **Your window is being launched because of app or system restart:** If you set `ApplicationRestart`,
  WinUI preserves minimization and attempts to return the window
  to the virtual desktop it was previously on. Your app detects the restart and recreates its windows.
  Call `Window.Show(false)` after applying placement to show each window without requesting
  activation or restoring a minimized window.
  See [Recreate windows after an app or system restart](#35-recreate-windows-after-an-app-or-system-restart).

## 3. Examples

### 3.1. Persist window placement automatically

You can let WinUI remember a window's position, size, and state when it closes and use them
when your app recreates it. If your app has package identity, setting `PlacementId` tells WinUI
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
determining how to place the window (this is the Win32 `STARTUPINFO` struct).

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
    // Use Show instead of Activate because Activate will restore a minimized window.
    _window.Show(true);
}
```

### 3.3. Center a window on the display where it was last used

Suppose you want to open a normal window centered on the display where it was last used, even if the
saved window was maximized or snapped. You can use the `ApplyInitialPlacement` API to run WinUI's
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

// Load the WindowPlacementInfo from storage
WindowPlacementInfo placement = WindowPlacementInfo.TryLoadForPlacementId(window.PlacementId);

// Force the window to be in the normal state (e.g. not maximized)
if (placement is not null)
{
    placement.State = WindowPlacementState.Normal;
    placement.SnapRect = null;
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

> _Spec note: It's awkward here that WinUI has found the display to use but the app must call GetFromWindowId to get the DisplayArea for the window.
> Consider adding a better way to get the DisplayArea later._

### 3.4. Save and load placement in your own storage

You can capture a window's placement, save it in your app's own storage, and load it when
your app recreates the window. For an open window, call `TryGetPlacement` and save the
result if it is not null:

```csharp
WindowPlacementInfo placement = window.TryGetPlacement();
if (placement != null)
{
    // App-defined helper: serialize the placement in your own store.
    MyApp.SavePlacement(documentId, placement);
}
else
{
    System.Diagnostics.Trace.TraceWarning("No windowed placement is available to save.");
}
```

Save all the properties together (`NormalRect`, `WorkArea`, `Dpi`, `State`, `SnapRect`, `DisplayDeviceName`, and
`VirtualDesktopId`).  When your app recreates the window, load the saved placement and pass it to `ApplyInitialPlacement`.

```csharp
Window window = new MainWindow
{
    PlacementId = "Contoso.Editor.Documents",
    // Disable automatic loading and saving, but allow cascading if no placement is loaded.
    PreventAutomaticPlacementPersistence = true,
};

// Load placement data from app-defined helper
WindowPlacementInfo loadedPlacement = MyApp.LoadPlacement(documentId);
window.ApplyInitialPlacement(new WindowInitialPlacementOptions
{
    CustomPlacement = loadedPlacement,
});
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

Prepare each window with `ApplicationRestart`, then call `Window.Show(false)`.
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
            // Show without requesting activation; do not use Activate here.
            window.Show(false);
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
    private List<SavedDocument> LoadSavedDocuments() { /* app-defined */ }

}
```

For more information about how to properly support restart scenarios:

- See [Windows restart registration](https://learn.microsoft.com/windows/win32/recovery/registering-for-application-restart)
- See [AppInstance.Restart](https://learn.microsoft.com/windows/apps/windows-app-sdk/applifecycle/applifecycle-restart)

### 3.6. Start a new window in full-screen mode

Suppose you want to use automatic placement persistence, but your app also has a user setting
to start the app in full-screen mode.

To support this, call `ApplyInitialPlacement()` to prepare the window. Then call
`AppWindow.SetPresenter(AppWindowPresenterKind.FullScreen)` before calling `Activate()`.

```csharp
Window window = new MainWindow
{
    PlacementId = "Contoso.Editor",
};
window.ApplyInitialPlacement();
window.AppWindow.SetPresenter(Microsoft.UI.Windowing.AppWindowPresenterKind.FullScreen);
window.Activate();
```

When your app selects the overlapped presenter again, the window returns to its prepared
windowed placement.

## 4. API reference

### 4.1. Window.PlacementId

Set `PlacementId` to group windows that should share saved placement and cascading behavior.
This also enables WinUI's automatic placement persistence for the window.

```csharp
public string PlacementId { get; set; }
```

Set this property to a non-empty ID to enable:

- **Automatic placement persistence:** If your app has package identity, setting the ID
  opts this window into WinUI's automatic saving and loading of its position, size, and
  state, unless `PreventAutomaticPlacementPersistence` is `true`. Reuse the same ID when
  you recreate a window so WinUI can find its saved placement.
- **Cascade related windows:** WinUI also uses this identifier to find open, visible windows
  to cascade from.

For apps without package identity, `PlacementId` still groups windows for cascading, but
does not enable automatic placement persistence. Use
[your own storage](#34-save-and-load-placement-in-your-own-storage) to save and load placement.

### 4.2. Window.PreventAutomaticPlacementPersistence

Set this property to prevent WinUI from automatically loading and saving placement for the window.

```csharp
public bool PreventAutomaticPlacementPersistence { get; set; }
```

The default is `false`. WinUI still requires a non-empty `PlacementId` to automatically
load or save this window's placement. Setting this property to `true` does not disable
cascading, custom placement, or capture.

### 4.3. Window.ApplyInitialPlacement

Call this method to set the window's position, size, and state before showing it.

```csharp
public void ApplyInitialPlacement(WindowInitialPlacementOptions options);
public void ApplyInitialPlacement();
```

Call this method on the window's UI thread before the window has been shown.
Use the parameterless overload for default options.

WinUI chooses placement in this order:

- **Custom placement:** WinUI uses `CustomPlacement` if your app supplies a non-null value.
- **Cascading:** Otherwise, WinUI looks for an open, visible window with the same `PlacementId`,
  if `AllowCascading` is `true` and the reason is not `ApplicationRestart`.
- **Saved placement:** If there is no cascade source, WinUI looks for saved placement under
  `PlacementId`. (Automatic loading requires package identity, a non-empty ID, and
  `PreventAutomaticPlacementPersistence` set to `false`.)
- **Requested size:** If none of those sources provides placement, WinUI applies the
  `Window.Width` and `Window.Height` properties to the window size.
- **OS defaults:** WinUI keeps the window's existing or OS defaults.


See [Window.Activate](#44-windowactivate) and [Window.Show](#45-windowshow) for how WinUI
automatically applies initial placement before showing the window.

See [section 2.2](#22-how-winui-applies-window-placement-data-to-a-new-window) for scenarios.

**Additional behavior and errors**


  - WinUI reports an error before applying placement if your app calls from the wrong thread,
  the window is closed, or the options are null or contain invalid values.
- WinUI reports `E_ILLEGAL_METHOD_CALL` if the window has already been shown or another
  `ApplyInitialPlacement` call is in progress.

### 4.4. Window.Activate

> _Spec note: This is an existing method_

Call this method to show a new window or restore and request activation of an existing window.

```csharp
public void Activate();
```

`Activate()` calls [Show(true)](#45-windowshow) and also restores the window if it is minimized.

### 4.5. Window.Show

> _Spec note: Introducing Show() here because we need a way to tell WinUI to show a window without activating it_

Call this method to show the window and choose whether to request activation.

```csharp
public void Show(bool activateWindow);
```

| Argument | Behavior |
| --- | --- |
| `false` | Show the window without requesting activation. |
| `true` | Show the window and request activation. |

Before showing the window for the first time, `Show` calls
[`ApplyInitialPlacement()`](#43-windowapplyinitialplacement) unless an earlier call
to `ApplyInitialPlacement` completed without error. If the call to `ApplyInitialPlacement`
does result in an error, `Show` propagates the error without
showing the window or requesting activation. In this case, a later `Show` or `Activate` retries.

After applying initial placement if needed, WinUI preserves the window's bounds, state,
and presenter, including minimization. With `true`, WinUI requests activation through
Win32 `SetActiveWindow`, without requesting that Windows bring the app to the foreground.

WinUI handles first-show sizing and visibility notifications and keeps the managed
`Window` alive until it closes, whether or not activation is requested. See the
[restart example](#35-recreate-windows-after-an-app-or-system-restart) for using `Show(false)`.

**Errors**

- Call this method on the window's UI thread. WinUI reports an error if your app calls
  from the wrong thread or the window is closed.
- WinUI reports `E_ILLEGAL_METHOD_CALL` if an `ApplyInitialPlacement` call is in progress.

### 4.6. Window.TryGetPlacement

Call this method to capture a window's placement so your app can save it in its own storage
and apply it when recreating the window.

```csharp
public WindowPlacementInfo TryGetPlacement();
```

WinUI returns an independent `WindowPlacementInfo` object containing the window's position,
size, state, and display information. Your app can edit the snapshot before saving or
applying it. See [Save and load placement in your own storage](#34-save-and-load-placement-in-your-own-storage).

When the window uses an overlapped presenter, WinUI captures its normal restore bounds and
current windowed state. Normal restore bounds describe the outer position and size to use
when the window is not maximized, minimized, or snapped. In full-screen or compact-overlay
mode, WinUI returns its last complete, valid windowed snapshot, or null if none is available.
Null means no windowed snapshot is available; it is not an error.

WinUI does not capture visibility, activation, presenter kind, or Z-order.

You can call this method while the window is hidden, including before showing it for the
first time or after hiding it.

**Errors**

- If your app calls from the wrong thread, WinUI reports the existing thread-affinity error.
- If the window is closed, WinUI reports `E_INVALID_OPERATION`.
- If the window type is unsupported, WinUI reports `E_NOTIMPL`.

### 4.7. WindowInitialPlacementOptions

Use this object to choose how WinUI places a window when you call `ApplyInitialPlacement`.

```text
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
| `CustomPlacement` | `null` | Supply the placement to apply. With `null`, WinUI selects placement using the rules in [ApplyInitialPlacement](#43-windowapplyinitialplacement). |
| `Reason` | `Default` | Choose default, launch, or restart behavior. See [WindowPlacementReason](#411-windowplacementreason). |
| `AllowCascading` | `true` | Set to `false` to skip cascading from an open, visible window with the same `PlacementId`. WinUI never cascades custom placement or placement with `Reason = ApplicationRestart`. |

You can read and edit the options and placement objects on any thread.

### 4.8. WindowPlacementInfo

You can capture, load, construct, and edit a snapshot of a window's position, size, state,
and saved display information:

```text
runtimeclass WindowPlacementInfo
{
    WindowPlacementInfo();

    static WindowPlacementInfo TryLoadForPlacementId(String placementId);

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

### 4.9. WindowPlacementInfo.TryLoadForPlacementId

Load the placement WinUI saved for a window into a new `WindowPlacementInfo` instance.

```csharp
public static WindowPlacementInfo TryLoadForPlacementId(string placementId);
```

You can call this synchronous method on any thread to read your app's placement store.
WinUI returns a copy of the stored data without adjusting it for the currently available
displays.

WinUI may return null when storage is unavailable or inaccessible, a record is missing or evicted,
or saved data is invalid or uses an unsupported version.

### 4.10. WindowPlacementState

Use one value to describe the window's state and, when minimized, the state to use when restored.

| Value | Number | Minimized | Restore or visible state |
| --- | --- | --- | --- |
| `Normal` | 0 | No | Normal |
| `Maximized` | 1 | No | Maximized |
| `Minimized` | 2 | Yes | Normal |
| `Snapped` | 3 | No | Snapped |
| `MinimizedFromMaximized` | 4 | Yes | Maximized |
| `MinimizedFromSnapped` | 5 | Yes | Snapped |

### 4.11. WindowPlacementReason

Use this enum to give WinUI context about the reason this window is being shown.

```text
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
| `Default` | Use the standard placement rules without process-launch hints or restart-specific behavior. |
| `ApplicationLaunch` | Prefer the display indicated by Windows for the process's original launch. Honor Windows requests to start minimized or maximized. |
| `ApplicationRestart` | Preserve minimization and try to return the window to its saved virtual desktop without switching the user's active desktop. Ignore process-launch hints and do not cascade. |

## 5. API details

The following MIDL3 defines the API additions. Existing `Window` members are omitted.
The new APIs are part of `Microsoft.UI.Xaml.WinUIContract`, version 12.
Projection metadata requires review.

```text
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
            Boolean PreventAutomaticPlacementPersistence;
            void Show(Boolean activateWindow);
            WindowPlacementInfo TryGetPlacement();
            void ApplyInitialPlacement();
            void ApplyInitialPlacement(WindowInitialPlacementOptions options);
        }
    };
}
```
