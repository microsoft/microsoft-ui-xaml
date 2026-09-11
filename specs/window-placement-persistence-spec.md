Window placement persistence
===

- [Background](#background)
- [Conceptual pages (How To)](#conceptual-pages-how-to)
- [Examples](#examples)
- [API Pages](#api-pages)
- [API Details](#api-details)
- [Appendix](#appendix)

# Background

_Spec note: This section supports API review and is not intended for publication on
learn.microsoft.com. This is a proposal, not a description of the prototype's current behavior._

Desktop apps commonly remember where a window was when it closed. Saving a rectangle is not
sufficient: the monitor can disappear, the display scale or work area can change, and the window
can have different restored, maximized, minimized, and snapped positions.

This proposal adds automatic placement persistence to `Microsoft.UI.Xaml.Window`. A packaged app
opts in by setting one stable `PersistPlacementId` before its existing `Activate()` call.
Optional APIs let an app capture placement, edit a detached value, and supply initial placement
without enabling automatic storage.

The proposal addresses [#2680](https://github.com/microsoft/microsoft-ui-xaml/issues/2680) and
[#9503](https://github.com/microsoft/microsoft-ui-xaml/issues/9503). Related requests include
[microsoft/WinUI-Gallery#1606](https://github.com/microsoft/WinUI-Gallery/issues/1606) and
[microsoft/WindowsAppSDK#5896](https://github.com/microsoft/WindowsAppSDK/issues/5896).

The API separates three operations:

- **Capture:** obtain a detached description of a window's placement.
- **Initial placement:** select and apply placement before displaying a window.
- **Persistence:** save the window's eventual placement for a later process instance.

Accepting an initial placement is not confirmation that it was applied or durably saved.

The public value describes placement rather than native flags. One state enum represents normal,
maximized, minimized, and snapped placement, including the state to restore after minimization.
Current target-window configuration determines sizing constraints and resize-to-fit behavior.

The new APIs apply to desktop WinUI windows. Automatic storage requires package identity in this
version. Capture, explicit initial placement, and display options also work in unpackaged desktop
apps. The proposal does not add APIs to classic UWP `Windows.UI.Xaml.Window`.

Apps that do not use the new APIs retain their existing runtime behavior. Adding instance
`Show()` and `Hide()` methods has a source-compatibility consideration discussed in the appendix.

_Spec note: The API declaration uses `WinUIContract` version 12 to describe the intended stable
shape. The shipping contract version and release are not assigned. This local draft does not
start the repository's public feedback period. Publication follows the
[public API review process](public-api-review-process.md)._

# Conceptual pages (How To)

_This section is intended for publication on learn.microsoft.com._

## Enable automatic persistence

Set `PersistPlacementId` before the operation that first displays the window:

```csharp
var window = new MainWindow();
window.PersistPlacementId = "MainWindow";
window.Activate();
```

Configure the property in the window's constructor, in XAML, or before calling `Show()` or
`Activate()`. Do not rely on a content element's `Loaded` event to run before initial placement.

```xml
<Window
    x:Class="Contoso.MainWindow"
    PersistPlacementId="MainWindow">
    <!-- Window content -->
</Window>
```

Use a stable, application-defined id. The id is not displayed or localized. Each window that
should be remembered independently needs its own id. An empty id disables automatic loading
and saving; it does not disable explicit placement or display options.

Automatic persistence is per user, per app identity, and local to the machine. It uses the
packaged app's default `Microsoft.Windows.Storage.ApplicationData` local settings. Without a
usable store, automatic loading and saving are skipped. Explicit placement and display options
continue to work.

## Understand what placement contains

A placement contains the window's normal rectangle, saved monitor work area and DPI, display
state, and optional monitor, snap, and virtual-desktop information.

**Normal rectangle** means the outer window rectangle used when the window is neither maximized
nor snapped. A maximized, snapped, or minimized window still has a normal rectangle.
**Snap rectangle** means the visible frame bounds of the snapped window; it excludes invisible
resize borders.

All rectangles in `WindowPlacement` use physical pixels in the virtual screen coordinate system.
Negative X and Y coordinates are valid for monitors to the left of or above the primary monitor.
They are not XAML device-independent units, client-area rectangles, or Win32 workspace coordinates.

The work area is the usable part of the saved monitor, excluding reserved areas such as the
taskbar. The recorded DPI and work area describe the coordinate environment of the saved
rectangles. Keep them together when storing or constructing placement data.

Hidden, active, and foreground are not placement states. A window can be hidden while retaining
normal, maximized, minimized, or snapped placement. In this document, a **shown** window is one
that has not been hidden. A minimized window or a window on another virtual desktop can be shown
without having visible content on the user's current desktop.

## Restore after display changes

Restoration uses the current display configuration. It does not blindly reuse saved screen
coordinates and does not require the current monitor layout to equal the saved layout.

These adjustments apply to automatically loaded and explicitly supplied placement alike.
They occur during the initial-placement operation, before its visible display. Changing the
display configuration later does not start another persistence restore; normal live windowing
behavior continues to apply.

### The original monitor is still connected

WinUI first tries to match the saved display device name. A match selects that monitor even if
its location, work area, resolution, or DPI has changed. WinUI adapts the placement to its current
work area and DPI.

The device name is a display-system identifier, not a monitor's friendly name or a guaranteed
physical-device serial number. Matching is best effort.

### The original monitor went missing

If the saved monitor cannot be matched, WinUI selects a current monitor using the saved normal
rectangle. It chooses the monitor with the largest intersection with that rectangle, or the
nearest monitor if none intersects. It does not always choose the primary monitor. The saved
work area is used to adapt the geometry after monitor selection, not to choose this fallback.

For example, when a laptop's only external monitor is disconnected, a window saved on that monitor
is relocated to the laptop display. Its normal position is adapted to the new work area, and its
size is adjusted for the new DPI and available space.

If you subsequently close the relocated window, its new placement is saved. This feature keeps
one placement per id, not a separate placement for every monitor topology. Reconnecting the old
monitor does not make a running window return to its earlier location.

### The DPI changed

WinUI adjusts the normal size to preserve its approximate logical size. Moving from 100% scale
to 200% scale doubles the nominal physical-pixel width and height before fitting and current
window constraints are applied.

For example, a saved normal size of 800 by 600 physical pixels at 96 DPI has a nominal target
size of 1600 by 1200 physical pixels at 192 DPI. A smaller target work area or the window's current
size constraints can change the final result. Pixel rounding and window-frame calculations can
also affect the final bounds.

Position and size use different adjustments. Position follows the window's relative location
within the saved work area; it is not simply multiplied by the DPI ratio.

### The resolution or taskbar work area changed

WinUI adapts the normal position to the current work area. For example, a window near the
right side of its former work area is placed toward the right side of the new work area rather
than keeping an absolute offset that could now be off-screen.

If the nominal restored size is too large, a resizable target window can be reduced to fit.
Current window sizing constraints take precedence over the saved size. Restoration does not
change whether the target window is user-resizable.

A fixed-size window, or a window whose minimum size exceeds the available work area, may not
fit completely. Fitting moves geometry toward the usable work area, but oversized windows can
remain partly off-screen. The precise fitted bounds can differ across Windows versions.
This API does not guarantee that all content or every window control will fit on the display.

### The saved window is completely or partly off-screen

A placement can be entirely off the current screens while still describing a valid position on
its former monitor. That position is not reapplied unchanged. WinUI relocates the normal rectangle
toward the selected monitor's work area and fits it when the target window permits resizing.

The default also brings a partly off-screen normal window back within the work area when it can
fit. This API does not expose an option to preserve intentionally off-screen placement. Oversized
windows remain subject to the sizing limitations described above.

The normal rectangle must still intersect the saved work area to describe a valid saved
placement. If stored data does not satisfy that requirement, or an adjustment cannot be
represented safely, WinUI ignores that placement and uses the initial-placement fallback.
If a live window is completely outside its current work area and no valid placement can be
captured, automatic saving leaves the previously stored value unchanged.

### The window was snapped

WinUI retains both the normal rectangle and the snap rectangle. A snapped placement uses the
snap rectangle for its visible frame; the normal rectangle remains the position used when the
window is restored out of snap.

When the target work area changes, the snap rectangle is adjusted relative to its edges.
A window snapped along the left side of its former monitor is therefore placed along the left
side of the target work area, subject to current system snapping behavior and window constraints.

This restores an individual window's snapped bounds. It does not recreate a Snap Layout selection,
a snap group, or the positions of other apps' windows.

If snapping is disabled or the target cannot accept the snapped placement, WinUI falls back to
the normal placement. A minimized window whose snapped restore state cannot be retained remains
minimized during non-activating application restart, but restores to normal rather than snapped.

### The window was minimized

For ordinary display, WinUI removes minimization while retaining the saved restore state:

| Saved state | State after normalization |
|---|---|
| `Minimized` | `Normal` |
| `MinimizedFromMaximized` | `Maximized` |
| `MinimizedFromSnapped` | `Snapped`, or `Normal` if snapping is unavailable |

Normal, maximized, and snapped placements otherwise retain their state, subject to current
system capabilities and constraints.

Only non-activating application restart preserves saved minimization. This avoids an ordinary
restore appearing to do nothing while allowing an app to reconstruct its previous minimized
windows. An explicit native launcher show command is separate from saved state; see
[Choose launch or restart policy](#choose-launch-or-restart-policy).

### The window was full-screen or compact overlay

Full-screen and compact-overlay presenters are app-controlled and are not persisted by this
feature. Capture and automatic saving use the most recent valid overlapped placement, including
its normal rectangle, maximized or snapped state, and matching monitor and DPI information.
They do not save the full-screen or compact-overlay bounds as normal window bounds.

If no valid overlapped placement is available, capture returns `false` and automatic saving is
skipped. WinUI does not invent a previous windowed placement.

Initial placement does not change an app-selected presenter. If the target is already using a
full-screen or compact-overlay presenter when its initial operation runs, placement is skipped
and the opportunity is consumed. Display options still apply. Returning to an overlapped
presenter later does not retry that initial placement.

To restore the windowed position before entering another presenter, first apply initial placement
while hidden and overlapped, then select the other presenter and reveal the window.

## Use the initial-placement opportunity

Each window has one initial-placement opportunity, tracked separately from its current visibility
and activation. Display can consume the opportunity; hiding never resets it. Assigning a
persistence id does not create another opportunity.

`Show()` uses the new initial pipeline. `Activate()` also uses it when a non-empty
`PersistPlacementId`, an explicit placement, or a non-null `InitialShowOptions` has been supplied.
An unconfigured initial `Activate()` retains its existing path and consumes the opportunity
without a persistence restore. The pipeline's reentrancy rules do not change that legacy path.

While the opportunity is open, an operation using the initial pipeline:

1. Snapshots and validates `InitialShowOptions`.
2. Consumes the opportunity before callbacks can start another initial operation.
3. Selects a copied explicit placement, or loads automatic placement if an id is assigned.
4. Applies policy and current-display adjustments to the selected placement.
5. Displays and requests activation as specified by the initial options.

An explicit placement prevents automatic loading for that attempt. It does not disable
automatic saving. The persistence id used for selection is read before placement callbacks;
changes made during the operation can affect later saving, not its selected placement.

| Action | Effect on the opportunity |
|---|---|
| Construct the window or set its properties | Remains open |
| Stage or clear explicit placement | Remains open |
| Hide an already hidden window | Remains open |
| Fail initial-option validation | Remains open |
| Run a valid initial operation, including `KeepHidden` | Consumed |
| Complete an unconfigured initial `Activate()` | Consumed without this restore |
| Directly display the native window, including through `AppWindow.Show()` | Consumed without this restore |
| Hide or show after consumption | Remains consumed |

Direct native display bypasses this pipeline. A subsequent `Window.Show()` or `Window.Activate()`
uses post-initial behavior, even if it is the first call to that particular method. Hiding a
directly displayed window does not reopen the opportunity.

Reentrant `Show()`, `Activate()`, and `Hide()` calls while the initial operation is in progress
are ignored. A valid `TrySetInitialPlacement` call during that operation returns `false`.
Property assignments remain accepted but cannot replace the operation's options or placement.
A reentrant close is not ignored: normal close behavior applies, and the initial operation does
not continue applying placement or revealing a window that has closed.

### Initial-placement fallback

A valid initial operation consumes the opportunity even when there is no persistence id, no
usable saved value, an unsupported target presenter, or a placement-application failure.

In those cases, the window continues through its normal initial-display path. Pending initial
sizing is applied before placement selection and supplies fallback geometry. Successfully applied
placement supersedes that pending size, subject to the target window's current constraints.
A failed native application is not a transaction: it can already have adjusted geometry before
reporting failure. Fallback uses valid current bounds rather than promising to undo every native
change, but it does not retry automatic loading.

For `Launch`, the shell monitor hint is also considered for fallback geometry when it can be
captured and adjusted. No saved value is required to use the hint.

When an explicit placement was selected but could not be applied, WinUI does not then load an
automatic placement. Explicit precedence is not a request to try multiple stored positions.

`KeepHidden` and `DoNotActivate` still apply when placement is unavailable or cannot be applied.
Failure to restore position must not turn a non-activating operation into an activating one.
Once the opportunity has been consumed, later display calls do not retry placement.

## Control display and activation

Assign `InitialShowOptions` before the opportunity is consumed. Its defaults are ordinary display,
activation requested, and not kept hidden:

```csharp
window.InitialShowOptions = new WindowInitialShowOptions
{
    Reason = WindowShowReason.Default,
    ActivationBehavior = WindowActivationBehavior.Activate,
    KeepHidden = false,
};
```

Both initial `Show()` and initial `Activate()` honor these options. `KeepHidden` leaves the window
hidden and suppresses activation, even if `ActivationBehavior` is `Activate`. It does not change
the placement-policy row selected by `Reason` and `ActivationBehavior`.

After consumption, the options are no longer consulted:

| Method | Post-initial behavior |
|---|---|
| `Show()` on a shown window | No-op, including while minimized |
| `Show()` on a hidden window | Reveals its current state; requests activation only if not minimized |
| `Activate()` | Reveals if hidden, restores if minimized, and requests activation |
| `Hide()` | Hides without closing or resetting placement; no-op if already hidden |

Showing a hidden minimized window does not unminimize it. Use `Activate()` when the window should
return from minimization. The restore target is normal, maximized, or snapped according to its
current restore state and system capabilities.

An activation request is subject to Windows activation policy; it is not a guarantee of foreground
ownership. `DoNotActivate` means this initial operation does not request activation or transfer
focus. It does not prevent the user, or subsequent app code, from activating the window.

Visibility changes raise `VisibilityChanged` using the existing Window visibility semantics;
non-activating display does not suppress that event merely because activation was not requested.
`KeepHidden` causes no visibility or activation events. Hiding causes the normal visibility
notification and, if applicable, deactivation. A visibility no-op does not raise a visibility event.

If you need a later non-activating reveal of a non-minimized window, use the existing
`window.AppWindow.Show(false)`. It does not reapply placement after the opportunity is consumed.

## Choose launch or restart policy

`Reason` selects initial-placement policy; it does not identify a main window, create a window
hierarchy, or register the app for restart.

| Reason | Activation behavior | Saved minimization | Saved desktop | Monitor hint |
|---|---|---|---|---|
| `Default` | `Activate` | Normalize | Ignore | Ignore |
| `Default` | `DoNotActivate` | Normalize | Ignore | Ignore |
| `Launch` | `Activate` | Normalize | Ignore | Use if valid |
| `Launch` | `DoNotActivate` | Normalize | Ignore | Use if valid |
| `ApplicationRestart` | `Activate` | Normalize | Ignore | Ignore |
| `ApplicationRestart` | `DoNotActivate` | Preserve | Restore, best effort | Ignore |

`Launch` prefers the monitor the shell selected for the launch over the saved monitor.
For example, launching from a taskbar on a different monitor can cause restoration on that
monitor. If no valid hint is available, normal saved-monitor selection applies.

Assign `Launch` to each window for which you want that policy. WinUI does not infer it from
construction order or the first `Activate()` call. Setting only `PersistPlacementId` uses
`Default`, including when that call occurs in `OnLaunched`.

Native launcher show commands, such as `start /min`, are distinct from saved minimization and
follow Windows startup behavior. This API does not redistribute a command already consumed by
another window, such as a splash screen. `Reason` does not replay that command for every window.
The initial operation must still honor explicit `KeepHidden` and `DoNotActivate`.

For application restart, your app owns restart registration, detection, and the inventory of
windows to reconstruct. Configure each reconstructed window with `ApplicationRestart` and
`DoNotActivate`. You can then explicitly activate a window if appropriate.

On that non-activating restart path, WinUI attempts to return each window to its saved virtual
desktop without switching the user's active desktop. If the saved desktop no longer exists or
the move cannot be completed, the window stays on its current desktop.

Virtual-desktop identity is optional and best effort. Capture uses the most recently available
identity for the live window; it can be absent or stale if a current identity cannot be obtained
safely. It is not a guarantee that every desktop move has already been observed.

Restart restores the last successfully persisted placement, not necessarily placement immediately
before a crash. This feature does not periodically save moves and resizes.

## Capture, change, and supply placement

`TryGetPlacement` returns a detached `WindowPlacement`. Mutating it does not move the source
window. `Clone()` makes an independent copy of that value.

`TrySetInitialPlacement` copies and validates a value for a window whose opportunity is still
open. It does not move or show the target immediately. A successful call replaces any previously
staged value. Passing `null` clears the staged value so automatic loading can be selected again.

The result describes **acceptance**, not successful application or storage:

- `true`: the supplied value, or the request to clear it, was accepted.
- `false`: the opportunity is already consumed or in progress.
- An argument error: a non-null value is structurally invalid.

Monitor removal, a changed DPI, a rectangle outside the current screens, and a missing virtual
desktop do not alone make placement structurally invalid. The saved geometry must still satisfy
its own work-area and data-validity rules. Accepted data can produce adjusted bounds rather than
its exact saved coordinates.

Capture and automatic saving use the same definition of durable placement:

- A shown overlapped window supplies its current geometry and placement state.
- A hidden overlapped window supplies its current geometry and last meaningful show state.
- A full-screen or compact-overlay window supplies its last valid overlapped placement.

Hiding does not replace the state with a fictitious `Hidden` value. Position or size changes made
while an overlapped window is hidden are included in subsequent capture and saving. An app-initiated
state change also updates its meaningful state; hiding alone does not.

Before any visible display or successful hidden placement, `TryGetPlacement` can capture valid
current overlapped geometry. If no show state has yet been established, it uses `Normal`.
Merely capturing or staging that geometry does not make the window eligible for automatic saving.

## Know when placement is saved

An automatically persisted window becomes eligible for saving after it has been shown through
any display path, or after an explicit or automatically loaded placement was successfully applied
while hidden. Adjusting fallback geometry for a launch-monitor hint alone does not make a hidden
window eligible.

A constructed window, a staged value, or a failed `KeepHidden` placement attempt does not by
itself make a window eligible. This prevents unused windows from replacing saved placement with
default geometry.

| Trigger | Saving behavior |
|---|---|
| Accepted normal close | Capture after close handlers finish and before native-window teardown |
| Cancelled close | Do not save for that close |
| Other native destruction | Best-effort backstop if normal close did not already save |
| Confirmed sign-out or shutdown | Best-effort save while placement is still available |
| Cancelled sign-out or shutdown | Do not save for that session-end request |
| Hide, move, resize, or placement application | No persistence write |
| Crash or forced process termination | No guaranteed save opportunity |

Saving reads the current `PersistPlacementId`. Setting it after initial display enables future
saves without moving the window. Changing it selects a different slot for future saves; clearing
it stops them. Old slots are not automatically migrated or deleted.

Ids use ordinal, case-sensitive comparison without Unicode normalization. Windows and processes
using the same app store and id share a slot; the last successful save wins. The feature does not
coordinate their window positions or cascade duplicate instances.

Missing storage, corrupt or unsupported stored data, and ordinary capture, placement, or storage
failures do not fail a display or close operation solely because persistence could not complete.
Automatic storage is best effort, not a durable-write acknowledgment.

The stored representation is private to WinUI. These APIs do not expose a serialization format,
a save-completed event, or an application-supplied persistence store.

Use one automatic placement mechanism per native window. WinUI does not coordinate with another
library or an app-owned routine that also restores and saves the same window. Use explicit
initial placement when you need to supply data to this pipeline rather than running a second
restore operation.

# Examples

_This section is intended for publication on learn.microsoft.com. The snippets assume an
initialized desktop WinUI app and application-defined window and content types._

## Remember the main window

An existing app can retain its normal activation path:

```csharp
private Window? _mainWindow;

protected override void OnLaunched(LaunchActivatedEventArgs args)
{
    _mainWindow = new MainWindow
    {
        PersistPlacementId = "MainWindow",
    };
    _mainWindow.Activate();
}
```

To also use the shell's launch-monitor preference, assign initial options before activation:

```csharp
_mainWindow.InitialShowOptions = new WindowInitialShowOptions
{
    Reason = WindowShowReason.Launch,
};
_mainWindow.Activate();
```

This second snippet replaces the first example's final activation call; it is not a second
activation after the initial opportunity has been consumed.

## Remember independent windows

```csharp
var document = new DocumentWindow
{
    PersistPlacementId = $"Document:{stableDocumentId}",
};
var inspector = new InspectorWindow
{
    PersistPlacementId = "Inspector",
};

document.Activate();
inspector.Activate();
```

`stableDocumentId` is an app-owned, non-sensitive identifier, not a localized title or a document
path. Use different ids for independently remembered windows.

## Show without requesting activation

```csharp
var window = new NotificationWindow
{
    PersistPlacementId = "Notifications",
    InitialShowOptions = new WindowInitialShowOptions
    {
        ActivationBehavior = WindowActivationBehavior.DoNotActivate,
    },
};
window.Show();
```

If usable placement exists, WinUI applies it with current-display adjustments. Otherwise, it uses
fallback placement. Neither case requests activation.

## Prepare a window while it remains hidden

```csharp
var window = new MainWindow
{
    PersistPlacementId = "MainWindow",
    InitialShowOptions = new WindowInitialShowOptions { KeepHidden = true },
};

window.Show(); // Initial placement attempted; the window remains hidden.

var data = await LoadDataAsync();
window.Content = new MainPage(data);
window.Activate();
```

The later activation does not retry placement. If you adjust an overlapped window's geometry
while hidden, that geometry is available to subsequent capture and automatic saving.

## Copy a window's placement without copying its state

```csharp
if (sourceWindow.TryGetPlacement(out var captured))
{
    var placement = captured.Clone();
    placement.State = WindowPlacementState.Normal;
    placement.SnapRect = null;
    placement.VirtualDesktopId = null;

    var replacement = new DocumentWindow { PersistPlacementId = "Replacement" };
    replacement.TrySetInitialPlacement(placement);
    replacement.Activate();
}
```

The target is newly constructed, so its opportunity is open. Staging copies the value; later
changes to `placement` cannot alter the target's selected data. Current-display adjustments still
apply, so this is not a guarantee of pixel-identical placement on a changed display configuration.

C++/WinRT uses the same explicit cloning operation:

```cpp
auto copy = captured.Clone();
copy.State(winrt::Microsoft::UI::Xaml::WindowPlacementState::Normal);
```

Ordinary assignment of a runtime-class reference is not a clone.

## Supply placement from app-owned data

This example assumes the app has validated its data and decided to use it for this opening:

```csharp
var placement = new WindowPlacement(savedNormalRect, savedWorkArea, savedDpi)
{
    State = savedWasMaximized
        ? WindowPlacementState.Maximized
        : WindowPlacementState.Normal,
    DisplayDeviceName = savedDisplayDeviceName,
};

var window = new MainWindow { PersistPlacementId = "MainWindow" };
window.TrySetInitialPlacement(placement);
window.Activate();
```

The rectangle, work area, and DPI must describe the same saved coordinate environment. Do not
substitute the current work area or DPI for missing saved metadata without first converting
the geometry. A legacy rectangle alone may not contain enough information to reproduce its
former logical size.

Explicit placement wins over automatic loading on every opening where you supply it.
For migration, your app owns the decision to stop supplying legacy data. Acceptance does not
acknowledge a persistence write, so do not treat it as proof that legacy data can safely be
deleted. This example does not implement a transactional migration protocol.

## Reconstruct windows after application restart

The app has registered for restart, detected the restart launch, and retained its own window
inventory:

```csharp
foreach (var savedWindow in savedWindowInventory)
{
    var window = CreateWindowFor(savedWindow);
    window.PersistPlacementId = savedWindow.PlacementId;
    window.InitialShowOptions = new WindowInitialShowOptions
    {
        Reason = WindowShowReason.ApplicationRestart,
        ActivationBehavior = WindowActivationBehavior.DoNotActivate,
    };
    window.Show();
}
```

`CreateWindowFor` creates content without displaying the window. This path preserves saved
minimization and attempts virtual-desktop restoration. The app can separately activate a window
when appropriate; it need not activate any window when reconstructing an all-minimized inventory.

See [RegisterApplicationRestart] for the native registration API. Registration, inventory storage,
and application-data recovery are outside placement persistence.

# API Pages

_This section is intended for publication on learn.microsoft.com._

## Window.PersistPlacementId property

Gets or sets the application-defined id used for automatic placement loading and saving.

```csharp
public string PersistPlacementId { get; set; }
```

The default is an empty string. Null is treated as empty. An empty value disables automatic
loading and saving without clearing an explicitly staged placement.

WinUI reads the id when selecting initial placement and again when saving. A late assignment
affects future saves, not the current window's restore. See
[Know when placement is saved](#know-when-placement-is-saved) for identity and save timing.

This property does not provide an application-supplied storage location. Automatic storage is
available for packaged apps; explicit placement does not require it.

## Window.InitialShowOptions property

Gets or sets the options used when this window consumes its initial-placement opportunity.

```csharp
public WindowInitialShowOptions InitialShowOptions { get; set; }
```

The default is null. When the initial pipeline runs, null supplies the default option values.
Assigning a non-null object also opts an initial `Activate()` into that pipeline, even if its
properties all have their default values. See
[Use the initial-placement opportunity](#use-the-initial-placement-opportunity).

The property retains the assigned reference. The initial operation copies its values once and
does not consult it again. Changing or mutating the assigned object after consumption has no
effect on this window's placement or display behavior. The getter still returns the current
property value; it does not return an execution record.

Undefined `Reason` or `ActivationBehavior` values fail validation with `E_INVALIDARG` only while
the opportunity is open. Validation failure leaves it open and does not apply placement, show,
or activate the window. After consumption, even invalid option values are behaviorally ignored.

Several windows can share one options object. Each takes its own snapshot when its opportunity
is consumed.

## Window.Show method

Runs the initial-display operation, or reveals a hidden window without restoring minimization.

```csharp
public void Show();
```

While the placement opportunity is open, this method honors `InitialShowOptions`, just as
`Activate()` does. With default options it attempts initial placement, displays, and requests
activation. With `KeepHidden` it consumes the opportunity without displaying.

After consumption, it is a no-op for a shown window, including a minimized one. For a hidden
window, it reveals the current state. It does not unminimize the window and requests activation
only when revealing a non-minimized window.

See [Control display and activation](#control-display-and-activation) for shared visibility,
event, and reentrancy behavior.

## Window.Activate method

Runs the initial-display operation, or reveals, restores, and requests activation of the window.

```csharp
public void Activate();
```

While the placement opportunity is open and the feature is configured, this existing method
runs the initial pipeline and honors all `InitialShowOptions`. It can therefore perform a
non-activating or hidden initial operation despite its name. An unconfigured initial call retains
the legacy path, as described in
[Use the initial-placement opportunity](#use-the-initial-placement-opportunity).

After consumption, it reveals a hidden window, restores a minimized window, and requests
activation through the existing WinUI activation path. It does not reapply initial placement or
consult initial options.

## Window.Hide method

Hides the window without closing it.

```csharp
public void Hide();
```

The native window and XAML content remain alive. `Hide()` does not raise `Closed`, perform a
persistence write, or reset the placement opportunity. It also hides windows displayed directly
through `AppWindow`. An already hidden window is unchanged.

See [Control display and activation](#control-display-and-activation) for event and reentrancy
behavior, and [Capture, change, and supply placement](#capture-change-and-supply-placement) for
capture and saving while hidden.

## Window.TryGetPlacement method

Attempts to capture a detached description of the window's current durable placement.

```csharp
public bool TryGetPlacement(out WindowPlacement placement);
```

Returns `true` with a valid placement, or `false` with `placement` set to null if no valid
placement can be captured. Missing optional monitor or virtual-desktop identity does not by
itself make capture fail.

The returned object is independent of the window and other captures. It does not keep the
window alive, and its properties do not update when the window moves.

Hidden and non-overlapped capture follow
[Capture, change, and supply placement](#capture-change-and-supply-placement). If the restore state
of a minimized window cannot be established, capture uses `Minimized` rather than inventing
maximized or snapped history.

This method does not load stored placement, write storage, or consume the initial opportunity.
A call on the wrong thread or on a closed window is an API-use error, not a `false` result.

## Window.TrySetInitialPlacement method

Attempts to stage or clear an explicit placement before this window's initial operation.

```csharp
public bool TrySetInitialPlacement(WindowPlacement placement);
```

A non-null value is snapshotted and validated before it is accepted. Subsequent mutation does
not change the staged value. Passing null clears explicit placement. Successful staging or
clearing returns `true` and leaves the opportunity open.

A non-null invalid value fails with `E_INVALIDARG` and leaves any previously staged value
unchanged. A valid value or a clear request returns `false` when the opportunity is consumed or
in progress. Staging does not move the window or acknowledge application or persistence.

Explicit placement prevents automatic loading for the attempt, including if later application
fails. Automatic saving remains governed by `PersistPlacementId`.

Thread and closed-window errors are checked first. Non-null argument validation precedes the
opportunity check, so invalid data remains an argument error even after the opportunity has
expired.

## WindowPlacement class

Represents a mutable, detached description of window placement.

```csharp
public sealed class WindowPlacement
{
    public WindowPlacement(RectInt32 normalRect, RectInt32 workArea, int dpi);
    public WindowPlacement Clone();

    public RectInt32 NormalRect { get; set; }
    public RectInt32 WorkArea { get; set; }
    public int Dpi { get; set; }
    public WindowPlacementState State { get; set; }
    public RectInt32? SnapRect { get; set; }
    public string DisplayDeviceName { get; set; }
    public Guid? VirtualDesktopId { get; set; }
}
```

This object contains no live window or presenter reference. See
[Understand what placement contains](#understand-what-placement-contains) for coordinate units
and bounds.

### Data validity

The constructor validates its required geometry and DPI. Property setters permit temporary
invalid combinations while you edit a value. `TrySetInitialPlacement` validates a complete
snapshot using these rules:

- `NormalRect` and `WorkArea` have positive width and height.
- `NormalRect` has a positive-area intersection with the saved `WorkArea`.
- Any present `SnapRect` has positive width and height.
- Each rectangle's right and bottom edges are representable as signed 32-bit coordinates when
  computed from its X, Y, width, and height using checked arithmetic.
- `Dpi` is at least 96.
- `State` is a defined `WindowPlacementState` value.
- `Snapped` and `MinimizedFromSnapped` require `SnapRect`.
- `DisplayDeviceName` is empty or contains at most 31 UTF-16 code units, without embedded NUL.

These are structural rules, not a requirement that the saved monitor or desktop still exists or
that the rectangle currently overlaps a monitor. The intersection rule refers to the supplied
saved work area, not the work area of a currently connected monitor. Application also uses
checked arithmetic; an invalid or unrepresentable adjusted rectangle fails placement application
and uses fallback.

`SnapRect` is ignored for other states. You can set `State` to `Normal` to remove both snapping
and minimized restore-state instructions without manipulating separate flags.

Null `DisplayDeviceName` is treated as empty. `VirtualDesktopId` can be null; `Guid.Empty` is
treated as no identity when placement is accepted.

### Threading

`WindowPlacement` is agile and can be shared across threads. Individual property accesses are
thread-safe. `Clone()` and staging copy the fields as one coherent snapshot.

Several property assignments are not a transaction. Another thread taking a snapshot between
assignments can observe a temporarily invalid combination, which staging rejects. Finish related
edits before sharing a value for use.

The `Window` methods themselves must run on that window's UI thread. Agility of the data does
not make the target window agile.

## WindowPlacement constructor

Initializes a placement from a normal rectangle and its associated monitor work area and DPI.

```csharp
public WindowPlacement(RectInt32 normalRect, RectInt32 workArea, int dpi);
```

The initial `State` is `Normal`, `SnapRect` and `VirtualDesktopId` are null, and
`DisplayDeviceName` is empty. Invalid required geometry or DPI fails with `E_INVALIDARG`
(`ArgumentException` in .NET).

The constructor does not inspect the current monitor layout or move a window.

## WindowPlacement.Clone method

Creates an independent copy of this placement value.

```csharp
public WindowPlacement Clone();
```

The copy contains all property values, including fields not currently used by `State`.
It can be edited independently. Cloning does not validate the value, so you can also copy
an intermediate value while editing it.

Ordinary reference assignment does not clone the object. There is no same-type constructor
whose behavior can be confused with reference copying in C++/WinRT.

## Other WindowPlacement properties

| Property | Meaning |
|---|---|
| `NormalRect` | Outer restored bounds in physical screen pixels |
| `WorkArea` | Saved monitor work area associated with the rectangles |
| `Dpi` | DPI associated with the saved geometry |
| `State` | Visible or minimized placement and its restore state |
| `SnapRect` | Optional visible frame bounds used for snapped states |
| `DisplayDeviceName` | Optional GDI display device name, such as `\\.\DISPLAY1` |
| `VirtualDesktopId` | Optional most recently available virtual-desktop identity |

Current target-window sizing capability and constraints are not properties of a placement
snapshot. Applying a value does not change the target's presenter or resizability configuration.

## WindowPlacementState enum

Specifies the placement state and, for a minimized window, its restore state.

| Name | Value | Meaning |
|---|---|---|
| `Normal` | 0 | Neither maximized, minimized, nor snapped; uses `NormalRect` |
| `Maximized` | 1 | Maximized; retains `NormalRect` for restoring |
| `Minimized` | 2 | Minimized; restores to normal |
| `Snapped` | 3 | Snapped at `SnapRect`; retains `NormalRect` for restoring |
| `MinimizedFromMaximized` | 4 | Minimized; restores to maximized |
| `MinimizedFromSnapped` | 5 | Minimized; restores to snapped |

These values describe the placement to reproduce, not a requirement to prove how an app-created
value acquired that state. Initial policy can normalize minimization, and current system
capabilities can require a snapped placement to fall back to normal.

## WindowInitialShowOptions class

Provides one-time placement policy and display options for a window's initial operation.

```csharp
public sealed class WindowInitialShowOptions
{
    public WindowInitialShowOptions();

    public WindowShowReason Reason { get; set; }
    public WindowActivationBehavior ActivationBehavior { get; set; }
    public bool KeepHidden { get; set; }
}
```

| Property | Default | Meaning |
|---|---|---|
| `Reason` | `Default` | Selects launch, restart, or ordinary placement policy |
| `ActivationBehavior` | `Activate` | Selects activation and restart policy |
| `KeepHidden` | `false` | Suppresses display and activation for the initial operation |

This object is agile. Individual property accesses are thread-safe, and an initial operation
copies its values as one coherent snapshot. Multiple assignments are not a transaction.
Invalid enum values are rejected when an open opportunity consumes the options, not by setters.

See [Choose launch or restart policy](#choose-launch-or-restart-policy) for the policy table.

## WindowShowReason enum

Specifies the policy used for initial placement.

| Name | Value | Meaning |
|---|---|---|
| `Default` | 0 | Ordinary restore without launch-monitor preference |
| `Launch` | 1 | Ordinary restore with a valid shell launch-monitor preference |
| `ApplicationRestart` | 2 | Reconstruction after application restart |

`ApplicationRestart` preserves saved minimization and permits virtual-desktop restoration only
when combined with `DoNotActivate`. `KeepHidden` does not substitute for that setting.

## WindowActivationBehavior enum

Specifies whether the initial operation requests activation.

| Name | Value | Meaning |
|---|---|---|
| `Activate` | 0 | Request activation unless kept hidden |
| `DoNotActivate` | 1 | Do not request activation |

These options do not govern later display or activation calls.

## Common Window API requirements

The new `Window` properties and methods have the same UI-thread affinity as the window.
Calls on the wrong thread or on a closed window fail with the corresponding existing Window
API error. These checks precede the reentrant-display rules for an initial operation.

Invalid app-supplied options or placement use `E_INVALIDARG` (`ArgumentException` in .NET).
Automatic persisted-data failures instead follow the fallback and best-effort saving rules.
The `Try` methods do not suppress programming errors, and their Boolean results are not
storage-success indicators.

# API Details

_Spec note: This section supports API review and is not intended for publication on
learn.microsoft.com. Existing Window members are omitted; `Activate()` is an existing method
whose initial-operation behavior is extended as described above._

```midl
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
    enum WindowShowReason
    {
        Default = 0,
        Launch = 1,
        ApplicationRestart = 2,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    enum WindowActivationBehavior
    {
        Activate = 0,
        DoNotActivate = 1,
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [webhosthidden]
    [threading(both)]
    [marshaling_behavior(agile)]
    runtimeclass WindowPlacement
    {
        WindowPlacement(
            Windows.Graphics.RectInt32 normalRect,
            Windows.Graphics.RectInt32 workArea,
            Int32 dpi);

        WindowPlacement Clone();

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
    runtimeclass WindowInitialShowOptions
    {
        WindowInitialShowOptions();

        WindowShowReason Reason;
        WindowActivationBehavior ActivationBehavior;
        Boolean KeepHidden;
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
    [webhosthidden]
    unsealed runtimeclass Window
    {
        // ... existing members ...

        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        {
            String PersistPlacementId;
            WindowInitialShowOptions InitialShowOptions;

            Boolean TryGetPlacement(out WindowPlacement placement);
            Boolean TrySetInitialPlacement(WindowPlacement placement);

            [method_name("ShowDefault")]
            void Show();

            [method_name("HideDefault")]
            void Hide();
        }
    };
}
```

# Appendix

_Spec note: This section supports API review and is not intended for publication on
learn.microsoft.com._

## Native engine and WinUI policy

The native placement engine is the checked-in
[PlacementEx implementation](../dxaml/xcp/components/windowplacement/inc/PlacementEx/PlacementEx.h).
Its default monitor migration and keep-on-screen behavior form the basis of the conceptual
scenarios. The public API does not expose the native structure or depend on its numeric flag values.

| Concern | Engine behavior or additional WinUI responsibility |
|---|---|
| Monitor selection | Match display device name, then select a monitor from the saved normal rectangle |
| Normal geometry | Adapt work-area-relative position and DPI-scaled size; keep on screen and fit where supported |
| Snapped geometry | Preserve visible frame bounds separately and adapt their work-area-edge relationships |
| Snap availability | Apply the documented snapping-availability policy before either backend |
| Sizing capability | Use current target configuration; do not persist the old source window's resizable bit |
| Public state | Translate the six public states into native show state and restore/snap flags |
| Minimized-from-snap normalization | Convert the saved restore-to-arranged state into arranged placement before ordinary adjustment |
| Hidden capture | Retain meaningful state while reading current overlapped geometry |
| Other presenters | Retain a coherent last overlapped placement rather than structurally detecting full-screen bounds |
| Coordinate context | Produce physical coordinates consistently; do not expose DPI-virtualized engine output as physical pixels |
| Safe public input | Validate complete snapshots and use checked conversion and geometry arithmetic |
| Persistence and eligibility | Own store identity, initial-operation ordering, close/session-end saving, and fallback |
| Virtual desktops | Use optional identity safely; do not require a blocking shell query on a synchronous window-message path |

`AdjustForMainWindow` already normalizes minimized-from-maximized placement. WinUI supplies
minimized-from-snapped normalization explicitly. `Default` and activating restart use adjustment
without startup flags; `Launch` requests the monitor hint. WinUI does not replay native startup
show commands for each window.

`FindClosestMonitor` explicitly selects by device name, then `normalRect`, even though the native
action API has its own work-area-based default. WinUI preserves PlacementEx's explicit selection.
The default adapter does not enable `AllowPartiallyOffScreen`. It derives downlevel `AllowSizing`
from the target at application time rather than trusting a captured source configuration.
The native path instead requests fit-to-monitor. Exact oversized-window fitting can differ
between these paths; current app constraints and presenter selection remain authoritative.

PlacementEx has native `ApplyWindowAction` and downlevel paths. Their internal steps are not the
public contract, and downlevel `NoActivate` support is not equivalent to the native flag.
Integration must preserve the requested visibility and activation on both paths. If it cannot
apply placement without violating those options, it uses fallback with the same display options.
Cloaking alone is not evidence that focus or activation was preserved.

The snapshot contract also requires WinUI-owned state tracking; `GetPlacement` captures
restore-to-maximized and currently arranged state, but does not reconstruct
`RestoreToArranged` history for an already minimized window. A single native geometry query
also does not establish hidden state or pre-presenter placement.

Capture effective window geometry after application rather than exposing the native in/out value
as a post-apply snapshot. The native action path can leave saved input metadata unchanged, while
the downlevel path mutates the normal rectangle into workspace coordinates. Neither is a
substitute for a coherent physical-coordinate snapshot of the resulting window.

Source references for these defaults:

- [Capture and native validity](../dxaml/xcp/components/windowplacement/inc/PlacementEx/PlacementEx.h#L429-L581)
- [Monitor selection and application](../dxaml/xcp/components/windowplacement/inc/PlacementEx/PlacementEx.h#L583-L790)
- [Normal and snapped geometry adjustment](../dxaml/xcp/components/windowplacement/inc/PlacementEx/PlacementEx.h#L1298-L1444)
- [Launch and minimized-state adjustment](../dxaml/xcp/components/windowplacement/inc/PlacementEx/PlacementEx.h#L1493-L1625)

`IsValid()` requires an intersection with the saved work area, not the current topology.
The public validity rule retains that distinction. Public-input handling must additionally
check arithmetic before invoking helpers whose native calculations assume valid desktop geometry.

## API-shape rationale

`PersistPlacementId` combines opt-in with application-owned identity. A Boolean alone would
require the framework to invent an identity from unstable properties such as creation order
or a XAML class name.

The optional value is mutable because editing a captured placement is a common operation.
A six-state enum avoids invalid combinations of native maximize, minimize, and arrange flags.
Current resizability is live window configuration, not a durable property copied from another
window. No native off-screen opt-out is exposed in this version.

`Clone()` explicitly creates an independent object in both C# and C++/WinRT. It avoids the
[same-type constructor ambiguity][Copy-construction guidance] of a WinRT runtime class.

`InitialShowOptions` keeps one configuration path for existing `Activate()` callers and new
`Show()` callers. It is intentionally initial-only. Existing `AppWindow.Show(false)` covers a
later non-activating reveal without adding another set of overlapping options.

`TrySetInitialPlacement(null)` reverses staging while the opportunity is open without adding a
separate clear method. The method's Boolean describes lifetime eligibility, not application or
storage success.

## Compatibility and non-goals

The instance methods can take precedence over existing `Show(this Window)` or `Hide(this Window)`
extension methods when an app is recompiled. Derived app classes may also already declare those
names. This source-compatibility cost requires API review.

`method_name("ShowDefault")` and `method_name("HideDefault")` distinguish ABI/implementation names
from existing private methods. They do not rename projected calls: C++/WinRT and C# apps call
`Show()` and `Hide()`.

This version does not provide:

- automatic persistence for apps without package identity;
- an app-supplied persistence store, public serialization format, or transactional migration;
- continuous saves or crash-time recovery of the latest move;
- per-topology placement histories or duplicate-instance cascading;
- full-screen or compact-overlay presenter restoration;
- snap-group reconstruction or coordination with other apps' windows;
- a guarantee of exact saved pixels despite topology or constraint changes.

Experimental AppWindow placement APIs are related, but are not a dependency of this contract.
A future implementation could use them after establishing the required state, policy, and
compatibility mappings. This proposal does not assert complete field or behavior equivalence
with an unversioned experimental surface.

## Implementation acceptance criteria

The implementation must demonstrate the published behavior on native and downlevel paths.
In particular, it must cover:

- removed and repositioned monitors, missing device names, DPI changes, and work-area changes;
- completely off-screen, partly off-screen, oversized, fixed-size, and constrained windows;
- normal, maximized, snapped, and all three minimized restore states;
- snapping disabled, missing virtual desktops, and unavailable desktop identity;
- hidden adjustments, successful and failed hidden placement, and absent overlapped history;
- both initial entry methods, every reason/activation row, and every `KeepHidden` combination;
- direct native display, reentrant display and close, invalid options, and consumed opportunities;
- snapshot independence, coherent concurrent reads, invalid intermediate edits, and C#/C++/WinRT projection;
- accepted and cancelled close, native destruction, confirmed and cancelled session end;
- corrupt or unavailable storage, explicit precedence, no automatic retry, and no-data fallback;
- native launcher commands with a preceding splash or other HWND, without unexpected activation;
- unchanged runtime behavior for apps that do not use the feature.

[RegisterApplicationRestart]: https://learn.microsoft.com/windows/win32/api/winbase/nf-winbase-registerapplicationrestart
[Copy-construction guidance]: https://learn.microsoft.com/windows/apps/develop/cpp-winrt/consume-apis#dont-copy-construct-by-mistake
