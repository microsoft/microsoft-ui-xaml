Window placement persistence
===

- [Window placement persistence](#window-placement-persistence)
- [1. Background](#1-background)
- [2. Conceptual pages (How To)](#2-conceptual-pages-how-to)
  - [2.1. Turning on placement persistence](#21-turning-on-placement-persistence)
  - [2.2. When placement is saved and restored](#22-when-placement-is-saved-and-restored)
  - [2.3. Controlling the initial operation](#23-controlling-the-initial-operation)
  - [2.4. Working with placement explicitly](#24-working-with-placement-explicitly)
- [3. Examples](#3-examples)
  - [3.1. Remember where the user left the main window](#31-remember-where-the-user-left-the-main-window)
  - [3.2. Remember several windows separately](#32-remember-several-windows-separately)
  - [3.3. Copy and adjust placement for another window](#33-copy-and-adjust-placement-for-another-window)
  - [3.4. Migrate placement from app-owned data](#34-migrate-placement-from-app-owned-data)
  - [3.5. Show a window without taking focus](#35-show-a-window-without-taking-focus)
  - [3.6. Restore windows after an application restart](#36-restore-windows-after-an-application-restart)
  - [3.7. Place a window before showing it](#37-place-a-window-before-showing-it)
  - [3.8. Hide and show a window again](#38-hide-and-show-a-window-again)
- [4. API Pages](#4-api-pages)
  - [4.1. Window.PersistPlacementId property](#41-windowpersistplacementid-property)
  - [4.2. Window.InitialShowOptions property](#42-windowinitialshowoptions-property)
  - [4.3. Window.Show method](#43-windowshow-method)
  - [4.4. Window.Activate method](#44-windowactivate-method)
  - [4.5. Window.Hide method](#45-windowhide-method)
  - [4.6. WindowPlacement class](#46-windowplacement-class)
  - [4.7. Window.TryGetPlacement method](#47-windowtrygetplacement-method)
  - [4.8. Window.TrySetInitialPlacement method](#48-windowtrysetinitialplacement-method)
  - [4.9. WindowPlacementShowState enum](#49-windowplacementshowstate-enum)
  - [4.10. WindowPlacementFlags enum](#410-windowplacementflags-enum)
  - [4.11. WindowInitialShowOptions class](#411-windowinitialshowoptions-class)
  - [4.12. WindowShowReason enum](#412-windowshowreason-enum)
  - [4.13. WindowActivationBehavior enum](#413-windowactivationbehavior-enum)
- [5. API Details](#5-api-details)
- [6. Appendix](#6-appendix)
  - [6.1. What is not in this proposal](#61-what-is-not-in-this-proposal)
  - [6.2. How are Show, Hide, and Activate different?](#62-how-are-show-hide-and-activate-different)
  - [6.3. Comparison with WPF and UWP](#63-comparison-with-wpf-and-uwp)
  - [6.4. PlacementEx](#64-placementex)
  - [6.5. Relationship to experimental AppWindow placement APIs](#65-relationship-to-experimental-appwindow-placement-apis)
  - [6.6. Alternatives considered](#66-alternatives-considered)
  - [6.7. Behavior notes](#67-behavior-notes)


Addresses [#2680](https://github.com/microsoft/microsoft-ui-xaml/issues/2680).
Related: [#9503](https://github.com/microsoft/microsoft-ui-xaml/issues/9503),
[#1606](https://github.com/microsoft/microsoft-ui-xaml/issues/1606),
[WindowsAppSDK#5896](https://github.com/microsoft/WindowsAppSDK/issues/5896).

- [Background](#background)
- [Conceptual pages (How To)](#conceptual-pages-how-to)
- [Examples](#examples)
- [API Pages](#api-pages)
- [API Details](#api-details)
- [Appendix](#appendix)

# 1. Background

_Spec note: This section is for API review and will not be published to learn.microsoft.com._

Desktop applications commonly restore window position, size, and state across launches. WinUI
does not currently provide an API for this behavior, so applications implement persistence
themselves. A complete implementation must save and restore placement while accounting for cases
in which the saved placement is no longer valid:

* The monitor it was on has been unplugged.
* The monitor is still there but its resolution or scale factor changed.
* The saved rectangle is entirely off-screen, or far enough off-screen that the title bar cannot
  be grabbed.
* The window was maximized, minimized, or snapped when it was saved.
* The window was on a virtual desktop that no longer exists.

Saving only the window rectangle can restore a window off-screen after the display topology
changes. Handling every case requires applications to reproduce placement logic already present in
Win32 and the shell.

[Issue #2680](https://github.com/microsoft/microsoft-ui-xaml/issues/2680) asks for this as a
framework feature. It has been open since 2020 and lists restoring size and position as a "Must".

This spec proposes two layers of opt-in API:

* A packaged app can set one property and let WinUI load, apply, and save placement.
* An app with advanced requirements can capture a placement, change its data, or supply an
  explicit placement before its initial-placement opportunity is consumed.

A window that does not use any new API behaves exactly as it does today.

_Spec note: the API details use `WinUIContract` version 12 to show the intended stable shape. The
shipping contract version has not been decided._

# 2. Conceptual pages (How To)

_This section is intended for publication on learn.microsoft.com._

## 2.1. Turning on placement persistence

Set `PersistPlacementId` to opt a window into placement persistence:

```csharp
var window = new Window();
window.PersistPlacementId = "MainWindow";
window.Activate();
```

WinUI uses the id to restore placement before the initial display and to save placement when the
window closes.

The id names the saved placement. It is your identifier, not a display string, and it is never
shown to the user. Use a stable id that means something in your app, such as `"MainWindow"` or
`"DocumentWindow"`. Two windows that use the same id share one saved placement, so give each
window that should be remembered separately its own id.

An empty or unset `PersistPlacementId` means the window does not participate in automatic
placement persistence. An app can still supply an explicit placement through `Window`.

## 2.2. When placement is saved and restored

WinUI restores placement once per window, on the first `Show()` or `Activate()` call. It
restores before the window is displayed, so a restored window is never painted in the wrong
place first.

For a window with a non-empty `PersistPlacementId`, WinUI saves placement when the window closes,
and as a backstop when the window is destroyed or the user signs out. It does not save
continuously while you drag or resize, so moving a window and then terminating the process
abnormally does not persist that move.

Setting `PersistPlacementId` after the initial `Show()` or `Activate()` call has no effect on that
window's restore. The new id is still used when the window is saved.

## 2.3. Controlling the initial operation

`InitialShowOptions` carries one-time options for the initial `Show()` or `Activate()` operation.
WinUI reads them when that operation consumes the placement attempt, and ignores them afterward.

You use these options to tell WinUI *why* the window is being shown. WinUI applies a different
placement policy for an ordinary display, an app launch, and a reconstruction after an
application restart.

```csharp
var window = new Window();
window.PersistPlacementId = "MainWindow";
window.InitialShowOptions = new WindowInitialShowOptions
{
    Reason = WindowShowReason.Launch,
};
window.Show();
```

The first `Show()` or `Activate()` honors every option. With default options, both display and
request activation. An app can keep its existing initial `Activate()` call while using
`ActivationBehavior` or `KeepHidden`.

## 2.4. Working with placement explicitly

`PersistPlacementId` provides automatic persistence for packaged apps. The explicit placement APIs
are optional:

```xml
<Window PersistPlacementId="MainWindow" />
```

`Window.TryGetPlacement` and `Window.TrySetInitialPlacement` provide the advanced path. They let an
app capture a detached snapshot of a window's current placement or supply an explicit placement
before the first `Show()` or `Activate()` call.

`WindowPlacement` exposes every durable placement field WinUI understands. It is mutable so an app
can change only the fields it cares about. Changing the object does not move a live window. The
target window copies and validates the placement when `TrySetInitialPlacement` accepts it.

An explicit placement and automatic persistence can be used together. When both are present:

1. The explicit placement is used for the current initial-placement attempt.
2. The automatically stored placement is not loaded for that attempt.
3. WinUI still captures the final placement and saves it under `PersistPlacementId`.

After WinUI selects the explicit placement, it applies the same `InitialShowOptions` policy used
for an automatically loaded placement. For example, ordinary launch still normalizes a minimized
placement, and virtual-desktop restoration remains limited to a non-activating application
restart.

This behavior supports one-time migration, transformation, testing, and placement reuse while
retaining automatic persistence.

# 3. Examples

_These examples are intended for publication on learn.microsoft.com._

## 3.1. Remember where the user left the main window

Set `PersistPlacementId` before the initial `Activate()` call. WinUI restores the placement saved
under that id and updates it when the window closes.

```csharp
public partial class App : Application
{
    private Window _window;

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        _window = new Window();
        _window.PersistPlacementId = "MainWindow";
        _window.Activate();
    }
}
```

## 3.2. Remember several windows separately

Give each window its own id. Leave `PersistPlacementId` unset for a document window whose placement
should not be saved.

```csharp
var main = new Window { PersistPlacementId = "MainWindow" };
var inspector = new Window { PersistPlacementId = "Inspector" };
var scratch = new Window(); // not persisted

main.Activate();
inspector.Activate();
scratch.Activate();
```

## 3.3. Copy and adjust placement for another window

The placement returned by `TryGetPlacement` is a detached snapshot. You can change it and supply
it to a window that has not yet been displayed:

```csharp
if (sourceWindow.TryGetPlacement(out WindowPlacement placement))
{
    placement.ShowState = WindowPlacementShowState.Normal;
    placement.VirtualDesktopId = null;

    var replacement = new Window
    {
        PersistPlacementId = "ReplacementWindow",
    };

    replacement.TrySetInitialPlacement(placement);
    replacement.Activate();
}
```

The explicit placement wins over a placement previously saved for `"ReplacementWindow"` on this
restore attempt. When the replacement window closes, automatic persistence saves its final
placement under that id.

## 3.4. Migrate placement from app-owned data

You can construct a placement from legacy data and supply it before showing the window:

```csharp
var placement = new WindowPlacement(legacyNormalRect, currentWorkArea, currentDpi)
{
    ShowState = legacyWasMaximized
        ? WindowPlacementShowState.Maximized
        : WindowPlacementShowState.Normal,
    DisplayDeviceName = legacyMonitorName,
};
var window = new Window { PersistPlacementId = "MainWindow" };
window.TrySetInitialPlacement(placement);
window.Activate();
```

Automatic persistence saves the resulting placement in WinUI's current format when the window
closes. The app does not need to migrate the legacy data again.

## 3.5. Show a window without taking focus

Use `Show()` with `DoNotActivate` when displaying a window must not change the foreground
activation.

```csharp
var window = new Window();
window.PersistPlacementId = "Notifications";
window.InitialShowOptions = new WindowInitialShowOptions
{
    ActivationBehavior = WindowActivationBehavior.DoNotActivate,
};
window.Show();
```

The window becomes visible in its saved position without being activated, and the user's current
foreground window keeps focus.

## 3.6. Restore windows after an application restart

When Windows restarts the app after an update or a crash, the app can reconstruct its previous
windows and restore their saved placements, including minimized state.

Your app is responsible for calling
[RegisterApplicationRestart](https://learn.microsoft.com/windows/win32/api/winbase/nf-winbase-registerapplicationrestart)
and for recording which windows were open. WinUI restores each window's placement.

```csharp
foreach (string id in savedWindowIds)
{
    var window = new Window();
    window.PersistPlacementId = id;
    window.InitialShowOptions = new WindowInitialShowOptions
    {
        Reason = WindowShowReason.ApplicationRestart,
        ActivationBehavior = WindowActivationBehavior.DoNotActivate,
    };
    window.Show();
}
```

`ApplicationRestart` with `DoNotActivate` is the only combination that preserves a saved
minimized state. Every other combination normalizes a minimized window. Preserving minimized state
is limited to non-activating application restart.

## 3.7. Place a window before showing it

`KeepHidden` applies the saved placement but leaves the window hidden. Use this when you want the
window sized and positioned before its content is visible.

```csharp
var window = new Window();
window.PersistPlacementId = "MainWindow";
window.InitialShowOptions = new WindowInitialShowOptions { KeepHidden = true };

window.Show();      // placement applied, window still hidden

await LoadContentAsync();

window.Activate();  // now reveal it
```

## 3.8. Hide and show a window again

`Hide()` removes a window from view without closing it. The window keeps its content and can be
shown again:

```csharp
window.Hide();

// The same Window and content remain alive.
window.Show();
```

Hiding does not save or reapply placement. The later `Show()` displays and requests activation
through the normal post-initial path.

# 4. API Pages

_These API pages are intended for publication on learn.microsoft.com._

## 4.1. Window.PersistPlacementId property

Gets or sets the id under which this window's placement is saved and restored.

```csharp
public string PersistPlacementId { get; set; }
```

Setting a non-empty id opts the window into placement persistence. The default is an empty
string, which means the window does not participate.

The id is an application-defined identifier. It is never displayed to the user and is not
localized. Windows that share an id share one saved placement.

WinUI reads this property when the first `Show()` or `Activate()` call runs the window's one
restore attempt, so you can set it any time before then, including from a `Loaded` handler. WinUI
reads it again when the window is saved.

**Remarks**

Placement persistence requires a usable default `ApplicationData` store. Packaged apps have this
store. Without it, restore and save are no-ops and the window opens at its default placement.

Saved placement is per-user and per-app. It is not roamed between machines.

## 4.2. Window.InitialShowOptions property

Gets or sets one-time options that describe this window's initial `Show()` or `Activate()`
operation.

```csharp
public WindowInitialShowOptions InitialShowOptions { get; set; }
```

The default is `null`, which behaves the same as a `WindowInitialShowOptions` with all properties
at their default values.

WinUI reads this property once, when the first `Show()` or `Activate()` call consumes the
window's placement attempt. Changing it afterward has no effect. Mutating the
`WindowInitialShowOptions` object after that point also has no effect, because WinUI takes a
snapshot of the values rather than holding a reference.

**Remarks**

If you set `InitialShowOptions` to an object whose `Reason` or `ActivationBehavior` holds a value
that is not a defined enumeration value, the `Show()` or `Activate()` call fails and the window
is not displayed. The placement attempt is not consumed, so you can correct the value and show
the window again.

## 4.3. Window.Show method

Runs the initial-display pipeline or displays a hidden window.

```csharp
public void Show();
```

The first `Show()` or
[Activate](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window.activate)
call honors every value in `InitialShowOptions`. With the default options, either method displays
and requests activation.

**Remarks**

The first `Show()` or `Activate()` call consumes the window's one placement attempt. Later calls
do not re-read `InitialShowOptions` or reapply placement. `Show()` displays and requests activation
for a hidden window, and is otherwise a no-op for a visible window, including a minimized window.
A later `Activate()` follows existing WinUI behavior: it displays a hidden window, restores a
minimized window, and calls `SetActiveWindow()`.

## 4.4. Window.Activate method

Runs the initial-display pipeline or activates the window.

```csharp
public void Activate();
```

When `Activate()` is the initial operation, it runs the same initial-display pipeline as `Show()`
and honors every value in `InitialShowOptions`. It can therefore display without activation when
`ActivationBehavior` is `DoNotActivate`, or apply placement without displaying when `KeepHidden`
is `true`.

After the initial operation, `Activate()` does not re-read `InitialShowOptions` or reapply
placement. It retains its existing WinUI behavior: it displays a hidden window, restores a
minimized window, and calls `SetActiveWindow()`.

## 4.5. Window.Hide method

Hides the window without closing it.

```csharp
public void Hide();
```

`Hide()` keeps the native window and its XAML content alive. It does not raise `Closed`, save
placement, reset `InitialShowOptions`, or reset the initial-placement opportunity.

`Hide()` hides the window whenever it is currently visible, including when it became visible
through `AppWindow.Show()`. It is a no-op while the window is hidden, including before the initial
`Show()` or `Activate()` operation, and does not consume an open placement opportunity.

A reentrant `Hide()` call made while the initial `Show()` or `Activate()` operation is in progress
is ignored. The outer operation determines the final visibility.

Hiding a visible window raises `VisibilityChanged`. If the window was active, existing activation
event behavior reports its deactivation. A no-op `Hide()` raises no visibility event.

A later `Show()` or `Activate()` displays the same window without reapplying persisted or
explicitly supplied placement. If automatic persistence saves a window while it is hidden, WinUI
saves its last non-hidden placement rather than a hidden show state.

When `KeepHidden` applies placement before the window has ever been visible, that applied placement
becomes the last non-hidden placement used for saving.

This method must be called on the window's UI thread. Calling it after `Close()` follows the
existing closed-window error behavior.

## 4.6. WindowPlacement class

Represents a mutable, detached snapshot of a window placement.

```csharp
public sealed class WindowPlacement
{
    public WindowPlacement(
        RectInt32 normalRect,
        RectInt32 workArea,
        int dpi);

    public WindowPlacement(WindowPlacement source);

    public RectInt32 NormalRect { get; set; }
    public RectInt32 WorkArea { get; set; }
    public int Dpi { get; set; }
    public WindowPlacementShowState ShowState { get; set; }
    public WindowPlacementFlags Flags { get; set; }
    public string DisplayDeviceName { get; set; }
    public RectInt32? ArrangeRect { get; set; }
    public Guid? VirtualDesktopId { get; set; }
}
```

| Name | Description |
|-|-|
| `NormalRect` | The restored position and size of the window. For a maximized or minimized window, this is its normal restored rectangle. |
| `WorkArea` | The work area of the monitor when the placement was captured. |
| `Dpi` | The DPI associated with `NormalRect` and `WorkArea`. |
| `ShowState` | Whether the captured window was normal, maximized, or minimized. |
| `Flags` | Additional durable state needed to reproduce the placement. |
| `DisplayDeviceName` | The optional device name used to match the original monitor. |
| `ArrangeRect` | The optional visible bounds of a snapped window. |
| `VirtualDesktopId` | The optional virtual desktop associated with the placement. |

The three-argument constructor creates a normal placement with no flags or optional identities.
The copy constructor creates an independent snapshot containing all placement data from `source`.

Changing a placement has no immediate effect. Pass it to `Window.TrySetInitialPlacement` to use it
for a window's initial placement.

The target window validates the complete object when accepting it. `NormalRect` and `WorkArea` must
have positive dimensions, `Dpi` must be at least 96, enumeration and flag values must be defined,
and arranged flags require a valid `ArrangeRect`. Invalid placement fails with `E_INVALIDARG`
(`ArgumentException` in .NET).

`WindowPlacement` contains durable placement data, not display instructions. Activation,
`KeepHidden`, and why the window is being shown remain in `WindowInitialShowOptions`.

## 4.7. Window.TryGetPlacement method

Gets a detached snapshot of the window's current durable placement.

```csharp
public bool TryGetPlacement(out WindowPlacement placement);
```

`TryGetPlacement` returns `true` and sets `placement` when WinUI captures a valid placement. It
returns `false` and sets `placement` to `null` when the window does not have a valid native window
or WinUI cannot capture a valid placement.

Full-screen and compact-overlay windows return their most recent overlapped placement.

This method must be called on the window's UI thread.

## 4.8. Window.TrySetInitialPlacement method

Supplies an explicit placement for the window's initial `Show()` or `Activate()` operation.

```csharp
public bool TrySetInitialPlacement(WindowPlacement placement);
```

The method copies and stages `placement`. It returns `true` only while the window's
initial-placement opportunity is still open. It returns `false` after a successful initial
`Show()` or `Activate()` operation consumes the opportunity, or after direct display through
`AppWindow` bypasses it. A call that fails while validating `InitialShowOptions` does not consume
the opportunity.
Passing `null` or an invalid placement fails with `E_INVALIDARG` (`ArgumentException` in .NET).

Mutating the supplied object after `TrySetInitialPlacement` returns does not change the staged
placement.

When the window also has a non-empty `PersistPlacementId`, the explicit placement takes precedence
over automatic restore for this attempt. Automatic saving remains enabled.

This method must be called on the window's UI thread.

## 4.9. WindowPlacementShowState enum

Specifies the display state captured in a placement.

| Name | Value | Description |
|-|-|-|
| `Normal` | 0 | The window is in its normal restored state. |
| `Maximized` | 1 | The window is maximized. |
| `Minimized` | 2 | The window is minimized. |

## 4.10. WindowPlacementFlags enum

Specifies additional durable state needed to reproduce a placement.

| Name | Value | Description |
|-|-|-|
| `None` | 0x0000 | No additional state. |
| `RestoreToMaximized` | 0x0001 | A minimized window returns to maximized when restored. |
| `Arranged` | 0x0002 | The window was snapped at `ArrangeRect`. |
| `AllowPartiallyOffScreen` | 0x0004 | Applying the placement may retain a partially off-screen normal rectangle. |
| `Resizable` | 0x0008 | The captured window was resizable, so its size may adapt to a smaller target work area. |
| `RestoreToArranged` | 0x0020 | A minimized window returns to its snapped placement when restored. |

`WindowPlacementFlags` contains only durable state. It does not contain transient instructions such
as whether to activate, remain hidden, or use a particular native placement implementation.

## 4.11. WindowInitialShowOptions class

Provides one-time options for a window's initial `Show()` or `Activate()` operation.

```csharp
public sealed class WindowInitialShowOptions
{
    public WindowInitialShowOptions();

    public WindowShowReason Reason { get; set; }
    public WindowActivationBehavior ActivationBehavior { get; set; }
    public bool KeepHidden { get; set; }
}
```

| Name | Description |
|-|-|
| `Reason` | Why the window is being shown. Selects the placement policy. Defaults to `WindowShowReason.Default`. |
| `ActivationBehavior` | Whether the initial `Show()` or `Activate()` call should activate the window. Defaults to `WindowActivationBehavior.Activate`. |
| `KeepHidden` | When `true`, the initial `Show()` or `Activate()` call applies placement but leaves the window hidden. Defaults to `false`. |

**Remarks**

When `KeepHidden` is `true`, the window remains hidden and is not activated, regardless of the
`ActivationBehavior` setting.

A window initially placed with `KeepHidden` has already consumed its placement attempt. A later
`Show()` displays and requests activation without restoring placement a second time. A later
`Activate()` displays it, calls `SetActiveWindow()`, and does not restore placement a second time.

## 4.12. WindowShowReason enum

Specifies why a window is being shown for the first time.

| Name | Value | Description |
|-|-|-|
| `Default` | 0 | Ordinary display. |
| `Launch` | 1 | The window is being shown as part of app launch. |
| `ApplicationRestart` | 2 | The app is reconstructing the window after an application restart. |

The reason selects which policy WinUI applies to the saved placement:

| Reason | ActivationBehavior | Saved minimized state | Saved virtual desktop | Shell monitor hint |
|-|-|-|-|-|
| `Default` | `Activate` | Normalize | Ignore | Ignore |
| `Default` | `DoNotActivate` | Normalize | Ignore | Ignore |
| `Launch` | `Activate` | Normalize | Ignore | Apply |
| `Launch` | `DoNotActivate` | Normalize | Ignore | Apply |
| `ApplicationRestart` | `Activate` | Normalize | Ignore | Ignore |
| `ApplicationRestart` | `DoNotActivate` | Preserve | Restore, best effort | Ignore |

"Normalize" means a window saved in a minimized state is restored to its normal position rather
than reopening minimized.

"Shell monitor hint" means the monitor the shell chose for the launch is preferred over the saved
monitor. This matters when the user launches your app from a taskbar on a different monitor than
the one the window was last closed on.

## 4.13. WindowActivationBehavior enum

Specifies whether the initial `Show()` or `Activate()` operation activates the window.

| Name | Value | Description |
|-|-|-|
| `Activate` | 0 | Request activation unless `KeepHidden` is `true`. |
| `DoNotActivate` | 1 | Do not request activation. |

`DoNotActivate` prevents the window from taking focus and selects the non-activating row in the
placement policy table.

# 5. API Details

_This section is for API review and will not be published to learn.microsoft.com._

```csharp (but really MIDL3)
namespace Microsoft.UI.Xaml
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    enum WindowShowReason
    {
        Default = 0,
        Launch = 1,
        ApplicationRestart = 2,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    enum WindowActivationBehavior
    {
        Activate = 0,
        DoNotActivate = 1,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    enum WindowPlacementShowState
    {
        Normal = 0,
        Maximized = 1,
        Minimized = 2,
    };

    [flags]
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    enum WindowPlacementFlags
    {
        None = 0x0000,
        RestoreToMaximized = 0x0001,
        Arranged = 0x0002,
        AllowPartiallyOffScreen = 0x0004,
        Resizable = 0x0008,
        RestoreToArranged = 0x0020,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    runtimeclass WindowInitialShowOptions
    {
        WindowInitialShowOptions();

        /// Why the window is being shown. Selects the initial placement policy.
        WindowShowReason Reason { get; set; };

        /// Whether the initial Show or Activate call should activate the window.
        WindowActivationBehavior ActivationBehavior { get; set; };

        /// When true, the initial Show or Activate call applies placement while leaving the
        /// window hidden.
        Boolean KeepHidden { get; set; };
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    runtimeclass WindowPlacement
    {
        WindowPlacement(
            Windows.Graphics.RectInt32 normalRect,
            Windows.Graphics.RectInt32 workArea,
            Int32 dpi);

        WindowPlacement(WindowPlacement source);

        Windows.Graphics.RectInt32 NormalRect { get; set; };
        Windows.Graphics.RectInt32 WorkArea { get; set; };
        Int32 Dpi { get; set; };
        WindowPlacementShowState ShowState { get; set; };
        WindowPlacementFlags Flags { get; set; };
        String DisplayDeviceName { get; set; };
        Windows.Foundation.IReference<
            Windows.Graphics.RectInt32> ArrangeRect { get; set; };
        Windows.Foundation.IReference<Guid> VirtualDesktopId { get; set; };
    };

    runtimeclass Window
    {
        // ... existing members ...

        /// The id under which this window's placement is saved and restored.
        /// An empty id means the window does not participate.
        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        String PersistPlacementId { get; set; };

        /// One-time options consumed by this window's first Show or Activate call.
        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        WindowInitialShowOptions InitialShowOptions { get; set; };

        /// Gets a detached snapshot of the window's current placement.
        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        Boolean TryGetPlacement(out WindowPlacement placement);

        /// Supplies an explicit placement for the window's initial Show or Activate operation.
        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        Boolean TrySetInitialPlacement(WindowPlacement placement);

        /// Runs the initial-display pipeline or displays a hidden window.
        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        void Show();

        /// Hides the window without closing it.
        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        void Hide();
    };
}
```

_Spec note: in the C++ projection the new `Show()` and `Hide()` methods are named `ShowDefault`
and `HideDefault`, because `Window` already carries private `IWindowPrivate::Show` and
`IWindowPrivate::Hide` methods. This does not affect the C# or WinRT surface._

# 6. Appendix

_This section is for API review and will not be published to learn.microsoft.com._

## 6.1. What is not in this proposal

* **Saving anything other than placement.** No window state beyond position, size, and show
  state. No per-window app data.
* **Persistence without an explicit opt-in.** A window with no `PersistPlacementId` and no
  explicitly supplied placement behaves exactly as it does today.
* **Roaming.** Saved placement stays on the machine that produced it.
* **Continuous saving.** Placement is saved on close, not on every move or resize.
* **An app-supplied store.** WinUI writes to the app's local settings. There is no hook for you
  to supply your own storage.

## 6.2. How are Show, Hide, and Activate different?

Whichever method is called first, `Show()` or `Activate()`, runs the same initial-display pipeline.
It snapshots and honors every value in `InitialShowOptions`, applies placement, and consumes the
window's one initial-placement opportunity.

With default options, either method makes the window visible and requests activation. With
`ActivationBehavior` set to `DoNotActivate`, either method displays without requesting activation.
With `KeepHidden` set to `true`, either method applies placement without displaying or activating
the window.

| Behavior | First `Show()` | First `Activate()` |
|-|-|-|
| Consumes initial placement | Yes | Yes |
| Honors `Reason` | Yes | Yes |
| Honors `ActivationBehavior` | Yes | Yes |
| Honors `KeepHidden` | Yes | Yes |

After the initial operation, neither method reapplies persisted or explicitly supplied placement.
`Show()` displays and requests activation for a hidden window but is otherwise a no-op for a
visible window, including a minimized window. `Activate()` retains its existing WinUI behavior: it
displays a hidden window, restores a minimized window, and calls `SetActiveWindow()`.

`Hide()` operates outside the initial-display pipeline. It hides a visible window without closing
it and does not consume an initial-placement opportunity that is still open. A later `Show()` or
`Activate()` reuses the same window and content.

Existing apps can retain `Activate()` as their initial entry point. When `InitialShowOptions`
requests no activation or no visibility, the initial `Activate()` does not perform the action
implied by its name. This exception applies only to the initial operation. Later `Activate()` calls
use the established activation behavior.

## 6.3. Comparison with WPF and UWP

The names `Show` and `Activate` come from different framework histories:

| API surface | `Show` | `Hide` | `Activate` |
|-|-|-|-|
| UWP `Windows.UI.Xaml.Window` | Not present | Not present | Serves as the initial display entry point and requests activation |
| WPF `System.Windows.Window` | Displays the window; `ShowActivated` controls activation | Hides without closing, so the same window can be shown again | Requests activation for a displayed window |
| Existing WinUI `Microsoft.UI.Xaml.Window` | Not present | Not present | Preserves the UWP entry point and activation behavior |
| This proposal | Runs the initial-display pipeline or later displays a hidden window | Hides without closing or consuming an open placement opportunity | Runs the same initial-display pipeline; later calls preserve existing WinUI activation behavior |

UWP established `Activate()` as the initial display method for a XAML window. Existing WinUI apps
use the same pattern. Requiring `Show()` would make those apps change their initial display call
solely to enable placement persistence.

WPF separates the concepts more explicitly. `Show()` displays a window, while `Activate()` requests
activation for a window that has been shown. WPF's `ShowActivated` property can suppress activation
when the window is first shown.

This proposal retains UWP's initial `Activate()` entry point and adds the WPF `Show()` and `Hide()`
method names. `InitialShowOptions` controls the first operation regardless of whether the app uses
`Show()` or `Activate()`. After that operation, `Show()` and `Hide()` control visibility while
`Activate()` retains its current WinUI activation semantics.

Adding an instance `Show()` or `Hide()` method can silently change source binding for an app that
currently calls a `Show(this Window)` or `Hide(this Window)` extension method. After recompilation,
the same call binds to the instance method instead. API review must account for this
source-compatibility cost.

## 6.4. PlacementEx

PlacementEx is an internal native helper used by this proposal. It captures and applies complete
Win32 window placement, matches a saved monitor to the current display topology, migrates geometry
across DPI changes, restores snapped placement, and keeps invalid or stale rectangles on a usable
screen.

PlacementEx is not a public API. Apps use `PersistPlacementId`, `WindowPlacement`, and the
`Window` methods described above. The public contract expresses durable placement concepts rather
than PlacementEx's native structure, flag values, or helper operations. WinUI can replace the
native implementation without changing the API shape.

## 6.5. Relationship to experimental AppWindow placement APIs

Windows App SDK experimental builds contain `AppWindow` placement APIs, including
`PersistedStateId`, `GetCurrentPlacement`, `SetCurrentPlacement`, `SaveCurrentPlacement`, and
`AppWindowPlacementDetails`.

Both designs use the same native placement logic for geometry. This proposal is a WinUI contract
that also defines one-property XAML opt-in, initial `Window` display behavior, string identity,
storage policy, session-end saving, virtual-desktop state, and interaction with `Show()` and
`Activate()`.

The public `WindowPlacement` type represents all durable fields WinUI currently saves.
`AppWindowPlacementDetails` currently represents those fields except virtual-desktop identity. A
future implementation can use stable `AppWindow` placement APIs underneath this contract if they
meet WinUI's requirements.

## 6.6. Alternatives considered

**A boolean opt-in instead of an id.** `PersistPlacement = true` requires less API surface, but a
multi-window app could not identify placements independently. WinUI would also have to derive
identity from information such as window class, XAML type, or creation order, none of which is
stable across app versions.

**Persisting automatically for every window.** Rejected. It changes behavior for every existing
app, and there is no identity WinUI could safely derive to key the saved data.

**Restoring on window construction rather than on first show.** Rejected. It would apply
placement before the app has had a chance to set `PersistPlacementId`, and it would make the
restore ordering depend on when the app happened to construct the window.

**An immutable placement plus a builder.** This keeps every accepted placement valid, but adds a
second type and a `Build()` step whenever an app changes placement data. A mutable detached
snapshot is used instead. The target window copies and validates the complete object when it is
accepted.

**Projecting PlacementEx directly.** Rejected. `PlacementEx` also contains transient display
instructions, test controls, and methods that implement policy. `WindowPlacement` projects its
durable data by meaning rather than exposing its native layout or every native operation.

## 6.7. Behavior notes

* Placement is restored before the window is first displayed, so a restored window is never
  painted at its default placement first.
* A window that is constructed but never shown does not overwrite good saved data with default
  geometry.
* `Hide()` does not close the window, save placement, or consume an open initial-placement
  opportunity.
* Saving a hidden window uses its last non-hidden placement.
* Sizes set through `Width` and `Height` before the first display are applied first, and a
  successfully restored placement supersedes them.
* Explicit placement supplied through `Window.TrySetInitialPlacement` takes precedence over
  automatic restore. A non-empty `PersistPlacementId` still enables automatic saving.
* Restoring placement does not trigger a save. Saving happens on close, on destroy, and on
  session end.
