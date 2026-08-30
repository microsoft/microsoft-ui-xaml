Window placement persistence
===

Feedback for this spec is open from **2026-08-30** to **2026-09-30**.
See [the public spec review process](./public-api-review-process.md).

Addresses [#2680](https://github.com/microsoft/microsoft-ui-xaml/issues/2680).
Related: [#9503](https://github.com/microsoft/microsoft-ui-xaml/issues/9503),
[#1606](https://github.com/microsoft/microsoft-ui-xaml/issues/1606),
[WindowsAppSDK#5896](https://github.com/microsoft/WindowsAppSDK/issues/5896).

- [Background](#background)
- [Conceptual pages (How To)](#conceptual-pages-how-to)
- [Examples](#examples)
- [API Pages](#api-pages)
- [API Details](#api-details)
- [Open questions for reviewers](#open-questions-for-reviewers)
- [Appendix](#appendix)

# Background

When you close a desktop app and open it again, most apps put their window back where you left
it. WinUI has no API for this today, so each app writes it by hand. Doing it correctly means
saving the window rectangle, restoring it, and then handling the cases where the saved rectangle
is no longer valid:

* The monitor it was on has been unplugged.
* The monitor is still there but its resolution or scale factor changed.
* The saved rectangle is entirely off-screen, or far enough off-screen that the title bar cannot
  be grabbed.
* The window was maximized, minimized, or snapped when it was saved.
* The window was on a virtual desktop that no longer exists.

Apps that handle only the first case ship windows that open off-screen when a monitor is
disconnected. Apps that handle all of them are re-implementing logic that Win32 already has in
`WINDOWPLACEMENT` and the shell already has in its restart behavior.

[Issue #2680](https://github.com/microsoft/microsoft-ui-xaml/issues/2680) asks for this as a
framework feature. It has been open since 2020 and lists restoring size and position as a "Must".

This spec proposes an opt-in API. A window that does not opt in behaves exactly as it does
today.

_Spec note: this API is proposed as experimental. It is gated behind `Feature_ExperimentalApi`
and is not present in stable builds._

# Conceptual pages (How To)

## Turning on placement persistence

You opt a window into placement persistence by giving it an id:

```csharp
var window = new Window();
window.PersistPlacementId = "MainWindow";
window.Activate();
```

That is the whole opt-in. WinUI restores the saved placement before the window is first
displayed, and saves the placement again when the window closes.

The id names the saved placement. It is your identifier, not a display string, and it is never
shown to the user. Use a stable id that means something in your app, such as `"MainWindow"` or
`"DocumentWindow"`. Two windows that use the same id share one saved placement, so give each
window that should be remembered separately its own id.

An empty or unset `PersistPlacementId` means the window does not participate. There is no
implicit or automatic persistence.

## When placement is saved and restored

WinUI restores placement once per window, on the first `Show()` or `Activate()` call. It
restores before the window is displayed, so a restored window is never painted in the wrong
place first.

WinUI saves placement when the window closes, and as a backstop when the window is destroyed or
the user signs out. It does not save continuously while you drag or resize, so moving a window
and then terminating the process abnormally does not persist that move.

Setting `PersistPlacementId` after the window has already been displayed has no effect on that
window's restore, because the one restore attempt has already been consumed. The new id is still
used when the window is saved.

## Controlling the first display

`InitialShowOptions` carries one-time options that describe the first display. WinUI reads them
when the first `Show()` or `Activate()` call consumes the placement attempt, and ignores them
afterward.

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

`Show()` honors these options. `Activate()` is an explicit request to show and focus, so it
overrides `ActivationBehavior` and `KeepHidden`, but it still honors `Reason`.

# Examples

## Remember where the user left the main window

This is the common case. Three lines, and the window opens where it was last closed.

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

## Remember several windows separately

Give each window its own id. A document window that should not be remembered simply does not set
an id.

```csharp
var main = new Window { PersistPlacementId = "MainWindow" };
var inspector = new Window { PersistPlacementId = "Inspector" };
var scratch = new Window(); // not persisted

main.Activate();
inspector.Activate();
scratch.Activate();
```

## Show a window without taking focus

A window that appears in response to a background event should not steal focus from whatever the
user is doing. Use `Show()` with `DoNotActivate`.

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

## Restore windows after an application restart

When Windows restarts your app after an update or a crash, you can put the user's windows back
the way they were, including a window that was minimized.

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
minimized state. Every other combination normalizes a minimized window, because a user who
launches an app expects to see a window.

## Place a window before showing it

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

# API Pages

## Window.PersistPlacementId property

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

Placement persistence requires a place to store the saved placement. WinUI stores placement in
your app's local settings, which requires that your app have package identity. In an app without
package identity, restore and save are no-ops and the window opens at its default placement. See
[Open questions](#open-questions-for-reviewers), which asks whether this should be observable to
you at runtime.

Saved placement is per-user and per-app. It is not roamed between machines.

## Window.InitialShowOptions property

Gets or sets one-time options that describe this window's first display.

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

## Window.Show method

Displays the window according to its `InitialShowOptions`.

```csharp
public void Show();
```

`Show()` differs from
[Activate](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window.activate)
in that it honors `ActivationBehavior` and `KeepHidden`. `Activate()` is an explicit request to
show and focus the window, so it overrides both of those, though it still honors `Reason`.

Use `Show()` when you want a window to appear without taking focus, or when you want placement
applied to a window that stays hidden. Use `Activate()` when you want the ordinary behavior of
bringing a window up in front of the user.

**Remarks**

The first `Show()` or `Activate()` call consumes the window's one placement attempt. Later calls
display the window normally and do not re-read `InitialShowOptions`.

## WindowInitialShowOptions class

Provides one-time options for a window's initial display.

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
| `ActivationBehavior` | Whether showing the window should also activate it. Defaults to `WindowActivationBehavior.Activate`. |
| `KeepHidden` | When `true`, `Show()` applies placement but leaves the window hidden. Defaults to `false`. |

**Remarks**

`KeepHidden` is honored by `Show()`. `Activate()` overrides it, because activating a window that
stays hidden is not meaningful.

A window shown with `KeepHidden` has already consumed its placement attempt. A later `Show()` or
`Activate()` reveals it through the ordinary path and does not restore placement a second time.

## WindowShowReason enum

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

## WindowActivationBehavior enum

Specifies whether showing a window also activates it.

| Name | Value | Description |
|-|-|-|
| `Activate` | 0 | Request activation. |
| `DoNotActivate` | 1 | Do not request activation. |

`DoNotActivate` also selects a non-activating placement policy, so a window shown this way does
not pull focus away from the user's current foreground window.

# API Details

```csharp (but really MIDL3)
namespace Microsoft.UI.Xaml
{
    [contract(WinUIContract, Experimental)]
    enum WindowShowReason
    {
        Default = 0,
        Launch = 1,
        ApplicationRestart = 2,
    };

    [contract(WinUIContract, Experimental)]
    enum WindowActivationBehavior
    {
        Activate = 0,
        DoNotActivate = 1,
    };

    [contract(WinUIContract, Experimental)]
    runtimeclass WindowInitialShowOptions
    {
        WindowInitialShowOptions();

        /// Why the window is being shown. Selects the initial placement policy.
        WindowShowReason Reason { get; set; };

        /// Whether showing the window should also activate it.
        WindowActivationBehavior ActivationBehavior { get; set; };

        /// When true, Show applies placement while leaving the window hidden.
        Boolean KeepHidden { get; set; };
    };

    runtimeclass Window
    {
        // ... existing members ...

        /// The id under which this window's placement is saved and restored.
        /// An empty id means the window does not participate.
        [contract(WinUIContract, Experimental)]
        String PersistPlacementId { get; set; };

        /// One-time options consumed by this window's first Show or Activate call.
        [contract(WinUIContract, Experimental)]
        WindowInitialShowOptions InitialShowOptions { get; set; };

        /// Displays the window according to its InitialShowOptions.
        [contract(WinUIContract, Experimental)]
        void Show();
    };
}
```

_Spec note: in the C++ projection the new `Show()` is named `ShowDefault`, because `Window`
already carries a private `IWindowPrivate::Show`. This does not affect the C# or WinRT surface._

# Open questions for reviewers

These are the points we would most like feedback on.

**1. Should an app be able to tell that persistence is unavailable?**

Today, if there is no usable store, restore and save are quiet no-ops. The window opens at its
default placement and nothing tells you why. That keeps the API small, and it means an app never
has to handle a storage failure. It also means an app can set `PersistPlacementId`, ship, and
never learn that placement was not being saved.

The alternative is to expose something observable, such as a read-only capability property or a
status on the window. That is more API surface for a condition most apps cannot do anything
about.

**2. Is `WindowShowReason.ApplicationRestart` in scope for v1?**

`ApplicationRestart` exists to support restart-manager scenarios, where the app is reconstructing
windows after Windows restarted it. It is the only value that preserves a saved minimized state
and the only one that tries to restore a virtual desktop.

The app still owns `RegisterApplicationRestart` and owns the record of which windows were open.
We would like to know whether apps doing restart work find this value useful as specified, or
whether `Default` and `Launch` are enough for a first release.

**3. Is a string id the right shape for the opt-in?**

A string id makes the opt-in explicit, lets one window class have several remembered instances,
and keeps the saved data addressable by the app. The alternative is a boolean that keys off
something WinUI derives, which is smaller but gives you no control over identity and makes
multi-window apps ambiguous.

**4. Is `KeepHidden` on the options object, or should it be a separate method?**

`KeepHidden` is the only option that changes whether the window becomes visible at all, which
makes it a slightly different kind of thing from `Reason` and `ActivationBehavior`. An
alternative is a distinct method, for example `ApplyPlacement()`, that positions without showing.

# Appendix

## What is not in this proposal

* **Saving anything other than placement.** No window state beyond position, size, and show
  state. No per-window app data.
* **Automatic persistence.** A window with no `PersistPlacementId` behaves exactly as it does
  today.
* **Roaming.** Saved placement stays on the machine that produced it.
* **Continuous saving.** Placement is saved on close, not on every move or resize.
* **An app-supplied store.** WinUI writes to the app's local settings. There is no hook for you
  to supply your own storage.

## Alternatives considered

**A boolean opt-in instead of an id.** `PersistPlacement = true` is smaller. It was rejected
because a multi-window app has no way to say which windows should be remembered separately, and
because WinUI would have to derive an identity from something (window class, XAML type, creation
order) that is not stable across app versions.

**Persisting automatically for every window.** Rejected. It changes behavior for every existing
app, and there is no identity WinUI could safely derive to key the saved data.

**Restoring on window construction rather than on first show.** Rejected. It would apply
placement before the app has had a chance to set `PersistPlacementId`, and it would make the
restore ordering depend on when the app happened to construct the window.

**Exposing the saved placement as a readable structure.** Rejected for a first release. It
enlarges the API surface, and it invites apps to interpret placement data whose format we want to
stay free to change.

## Behavior notes

* Placement is restored before the window is first displayed, so a restored window is never
  painted at its default placement first.
* A window that is constructed but never shown does not overwrite good saved data with default
  geometry.
* Sizes set through `Width` and `Height` before the first display are applied first, and a
  successfully restored placement supersedes them.
* Restoring placement does not trigger a save. Saving happens on close, on destroy, and on
  session end.
