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

_Prototype status (2026-09-24): explicit/saved placement now reaches native PlacementEx application
and effective capture. Live peer/cascade selection and public-API acceptance are still incomplete.
Hidden non-normal operations and unsupported restricted native operations fail best effort.
See the [implementation status](../docs/design-notes/Window-PlacementPersistence.md#native-application-status-2026-09-24)
for measured coverage and blockers; these limitations do not change the proposed contract below._

Desktop apps commonly remember where a window was when it closed. Saving a rectangle alone is
not enough: the monitor can disappear, the display scale or work area can change, and the window
can have different restored, maximized, minimized, and snapped positions.

This proposal adds automatic placement persistence to `Microsoft.UI.Xaml.Window`. A packaged app
opts in by setting a stable, non-empty `PersistPlacementId` before its first placement or display operation.
The default policy also cascades a newly opened window from a live peer in the same group when possible.
This document calls that configuration **easy mode**. **Expert mode** describes using the same
APIs to capture and edit placement, control cascading, or manage storage in the app. These are
documentation terms, not API names, enum values, or separate runtime modes.

`Window.UseAutomaticPlacementPersistence` defaults to `true`. A non-empty id together with
this property enables automatic source selection and subsequent eligible automatic saves.
An empty id disables automatic persistence without error. Set the property explicitly to `false`
to use the id only for cascade grouping or app-owned storage. Setting the id never changes the
Boolean or overrides an explicit `false`. Explicit placement and explicitly requested cascading
do not require storage.
`Show` is the preferred display entry point. Initial `Show()` and `Activate()` both use the
window's persistence property. `Activate()` retains its activation and restore-from-minimization
semantics; existing apps need not replace it to adopt basic persistence.
After first display, the persistence property can suspend or resume future saves for an
enrolled window. It cannot create enrollment or rerun initial placement.

The proposal addresses [#2680](https://github.com/microsoft/microsoft-ui-xaml/issues/2680) and
[#9503](https://github.com/microsoft/microsoft-ui-xaml/issues/9503). Related requests include
[microsoft/WinUI-Gallery#1606](https://github.com/microsoft/WinUI-Gallery/issues/1606) and
[microsoft/WindowsAppSDK#5896](https://github.com/microsoft/WindowsAppSDK/issues/5896).

The API separates three operations:

- **Capture:** obtain a detached description of a window's placement.
- **Initial placement:** select and apply placement before displaying a window.
- **Persistence:** save the window's eventual placement for a later process instance.

`TryApplyInitialPlacement` applies placement immediately while the window remains hidden.
It does not use up the automatic placement that first display performs. To reveal a prepared
window without replacing that work, call
`Show(new WindowShowOptions { SkipInitialPlacement = true })`. A successful hidden application
does not mean placement has been saved.

Display and activation APIs are evaluated here only as needed for placement: restoring
windows without taking focus during application restart, and applying placement while hidden
before selecting another presenter. General notification, tool-window, and focus-management
features are not goals of this proposal. The window-role examples describe placement needs,
not additional reasons to expand the display API surface.

Explicit launcher show-command integration, including `start /min` and `start /max`, is
deferred. This version does not add instruction ownership, merging, or replay policy.
Restoring saved minimization and virtual-desktop placement during application restart remains
in scope. The launch-monitor hint is a separate policy question.

The public value describes placement rather than native flags. One state enum represents normal,
maximized, minimized, and snapped placement, including the state to restore after minimization.
The target window's current settings determine its size constraints and whether it can resize to fit.

The planned native placement engine is PlacementEx. The appendix's
[scenario inventory](#placement-scenarios-and-winui-decisions) records the intended WinUI result
for each concrete restoration scenario, including differences from helper defaults and open decisions.

Storage, serialization, and lifecycle integration are described in the
[implementation design notes](../docs/design-notes/Window-PlacementPersistence.md).

The new APIs apply to desktop WinUI windows. Automatic storage requires package identity and a
usable packaged application identity in this version. Capture, explicit initial placement,
cascading, and display options also work in unpackaged desktop apps. Unpackaged cascade grouping
uses an empty application-identity component, so unrelated unpackaged apps using the same id can
match. The proposal does not add APIs to classic UWP `Windows.UI.Xaml.Window`.

The public `Show()`, `Show(WindowShowOptions)`, and `Hide()` methods are supported only on
desktop-backed `Microsoft.UI.Xaml.Window` instances. On a UWP-backed or framework dummy
instance, an otherwise valid call fails with `E_NOTIMPL` without changing visibility,
activation, placement, persistence, or audio state. This is a runtime restriction, not
conditional removal of the methods from metadata. Existing private window lifecycle
operations retain their behavior. See the
[implementation decision](../docs/design-notes/Window-PlacementPersistence.md#public-display-api-host-boundary-decision-2026-09-24).

Apps that do not use the new APIs retain their existing runtime behavior. Adding instance
`Show()` and `Hide()` methods has a source-compatibility consideration discussed in the appendix.

_Spec note: The API declaration uses `WinUIContract` version 12 to describe the intended stable
shape. The shipping contract version and release are not assigned. This local draft does not
start the repository's public feedback period. Publication follows the
[public API review process](public-api-review-process.md)._

# Conceptual pages (How To)

_This section is intended for publication on learn.microsoft.com._

## Window roles and placement needs

A **window role** describes how an app uses a `Microsoft.UI.Xaml.Window`. The terms below
describe scenarios, not new API types, flags, or framework-assigned identities. An app can
use the same `Window` type for any of these roles, and roles can overlap.

| Term | Meaning in this document | Placement considerations |
|---|---|---|
| **Main window** | The window an app treats as its principal user interface. A single-main-window app can also create splash, tool, or notification windows. | A stable role id with the default persistence Boolean before initial placement provides automatic persistence. The app separately chooses whether to use launch-monitor policy. |
| **Document window** | A window for an independently usable document or workspace, such as an editor document or a browser window containing multiple tabs. A multi-window app need not designate one of these windows as its main window. | Choose whether placement is remembered per document/window or shared across the role. Consider cascading new windows and reconstructing the window inventory after restart. |
| **Splash window** | A temporary startup window, also called a splash screen. | The app may choose a startup position rather than persist it. Being created or displayed first does not identify it as the main window or settle which window should receive a startup show instruction. |
| **Tool window** | A supporting window such as a floating toolbar, palette, or inspector. This is an app role, not a declaration of the native `WS_EX_TOOLWINDOW` style. | Decide whether to remember placement independently or position it relative to another window. The app controls any relative placement; this proposal does not add anchoring or follow-another-window behavior. Activation is a separate choice. |
| **Notification window** | A window used to present a notification or brief status information. | The app may choose a transient position rather than persist or cascade it. Notification presentation behavior is outside this proposal's scope. |

These terms do not imply native ownership, modality, taskbar presence, presenter selection,
or lifetime rules. For example, a document window can also be the app's main window.
`MainWindow`, `DocumentWindow`, and `InspectorWindow` in sample code
are app-defined class names, not special WinUI window types. The inspector is an example
of a tool window.

Other window descriptions in this document are not additional roles. Overlapped, full-screen,
and compact-overlay describe presenters; normal, maximized, minimized, and snapped describe
placement states. Shown, hidden, and cloaked describe visibility conditions. Source, target,
and cascade peer describe participation in a placement operation. First-run, replacement,
and reconstructed windows describe lifecycle scenarios rather than distinct window types.

Keep these concepts separate:

- **Role:** what the window is for. WinUI does not infer a main-window role from creation
  order, the first `Activate()` call, or execution inside `OnLaunched`.
- **Placement identity:** `PersistPlacementId` selects a saved slot and cascade group under
  the applicable app/user scope. `"MainWindow"` is an ordinary id, not a reserved value.
  Shared ids share a cascade group and, where automatic storage is available, a
  last-successful-save-wins slot. Different ids allow independent persistence but separate
  the cascade groups.
- **Placement policy:** `Reason = Launch` requests launch placement policy; it does not
  identify a main window. `ApplicationRestart` describes reconstruction policy, not a role.
  More than one window can participate in launch or restart. Explicit handling of native
  startup show instructions is deferred.

The [main-window example](#remember-the-main-window) and
[independent-window example](#remember-independent-windows) illustrate different identity
choices. The app remains responsible for deciding which windows to recreate after restart.

## Choose easy mode or expert mode

**Easy mode** means assigning a stable, non-empty `PersistPlacementId` and leaving
`Window.UseAutomaticPlacementPersistence` at its default of `true` before initial `Show()` or `Activate()`. WinUI chooses initial
placement and attempts eligible later saves. On an ordinary opening, WinUI first tries to cascade from an
eligible existing window in the same group. Otherwise, it loads saved placement when available
or uses normal initial-display fallback. Existing apps can keep their initial `Activate()`
call after setting the id.

**Expert mode** means choosing which parts WinUI handles. You can supply a detached
`WindowPlacement`, own its storage, request or disable cascading, or prepare a window while
hidden. You still use WinUI's monitor, DPI, sizing, and state adjustments.

These names describe usage patterns, not mutually exclusive API modes. For example, you can
supply explicit initial placement and still let WinUI save it automatically. Setting
`WindowShowOptions.Reason` to `Launch` does not require expert mode.

| What the app needs | Configuration |
|---|---|
| Automatic placement and persistence | Set a non-empty `PersistPlacementId`; call `Show()` or `Activate()` |
| Easy mode with the shell's launch-monitor preference | Use `Show(options)` with `Reason = Launch` |
| Prepare placement while hidden, then reveal it unchanged | `TryApplyInitialPlacement(options)`, then `Show` with `SkipInitialPlacement = true` |
| Persistence without cascading this window | Opt in and set `CascadeBehavior = Disabled` |
| App-owned storage without automatic reads or writes | Set `UseAutomaticPlacementPersistence = false`; capture and supply `Placement` explicitly |
| Customize framework-stored placement and retain automatic saves | Set a non-empty id and retain the default Boolean; load with `WindowPlacement.LoadForPersistPlacementId`, edit, and supply `Placement` |
| Cascading with app-owned storage or no storage | Keep a non-empty `PersistPlacementId`, set `UseAutomaticPlacementPersistence = false`, and set `CascadeBehavior = Enabled` |

Opting in requires a non-empty id at the initial operation; an empty id disables automatic
persistence and grouping without error, even when the Boolean is `true`.
Property setters do not perform storage I/O. Automatic loading
and saving also require a usable packaged application identity and store. Store unavailability
is not an invalid option and does not prevent best-effort placement or cascading.
Unpackaged apps can use cascading and explicit placement, but this version does not automatically
save their placement.

## Enable automatic persistence

Set the id before initial display:

```csharp
var window = new MainWindow();
window.PersistPlacementId = "MainWindow";
window.Show();
```

An existing app can keep its initial `Activate()` instead of the `Show()` call above.
Use one initial display call, not both. `Show(options)` attempts initial placement, then requests display and activation unless
`DoNotActivate` is set or the initial reason is `ApplicationRestart`.
Use `TryApplyInitialPlacement(options)` to apply while hidden. See
[Control display and activation](#control-display-and-activation).

You can configure the id and optional Boolean override in the window's constructor, in XAML,
or in code before calling `Show`, `Activate`, or `TryApplyInitialPlacement(options)`.
Assignment order does not matter: neither setter changes the other property. Do not wait for a content element's
`Loaded` event; initial placement may have already run by then.

```xml
<Window
    x:Class="Contoso.MainWindow"
    PersistPlacementId="MainWindow">
    <!-- Window content -->
</Window>
```

Choose a stable id for each [window role](#window-roles-and-placement-needs) or document you
want to remember separately. The id is also its cascade-group id. Instances using the same
id can cascade together; different ids do
not. The id is not displayed or localized. An empty id prevents automatic storage and group-based
cascading. You can still supply explicit placement and use display options without opting in.

This XAML assignment configures identity and opts in through the default Boolean. Code can call
`Show()` or keep `Activate()` without an options object. Set the Boolean to `false` when your app owns storage.

Automatic persistence is per user, per registered packaged application identity, and local to
the machine. WinUI uses an application-specific namespace inside the packaged app's default
`Microsoft.Windows.Storage.ApplicationData` local settings. Applications in the same package
family do not share placement slots merely because they use the same `PersistPlacementId`.
Without a usable application identity or store, automatic loading and saving are skipped.
Explicit placement and display options continue to work. Cascading does not require a store;
its separate identity rules are described below.

Stored placement follows the packaged app's local app-data lifetime. Windows preserves the
local app-data store across app updates and removes it, including saved placement, when the app
is uninstalled. Apps do not need placement-specific uninstall cleanup. See
[Store and retrieve settings and other app data][App data lifetime].

Placement records are a bounded cache, not permanent per-document storage. After successful saves,
WinUI trims the application's stored placements to the 32 most recently saved ids under the same
user/application scope. Loading does not refresh recency. Cleanup is best effort, so interruption
or storage failures can temporarily leave more records. An evicted id has no saved placement on a
later load; live windows are unaffected.

## Cascade related windows

Cascading offsets a new window's normal position down and to the right of an existing window,
using PlacementEx's caption-and-frame-based offset. It wraps at work-area edges and remains
subject to display adjustment and current size constraints. It is a best-effort placement
policy, not a guarantee that windows never overlap.

`PersistPlacementId` identifies the group. For packaged apps, the group also includes the current
user and registered packaged application identity, just as a storage slot does. Another app in
the same package does not join the group merely by using the same id.

For unpackaged apps, the application-identity component is empty. Their groups are shared by
unpackaged apps for the same user in the applicable desktop scope. Two unrelated unpackaged apps
using `"MainWindow"` can therefore match. Choose an app-specific, non-sensitive id such as
`"Contoso.Editor:MainWindow"` when that is not desired. This grouping is not a security boundary.
A failure to resolve a packaged app's identity does not put it into the unpackaged group.

Set `WindowShowOptions.CascadeBehavior` in the initial request:

| Value | Behavior for `Default` or `Launch` |
|---|---|
| `Automatic` | Attempt cascading when automatic persistence is opted in and no explicit placement is supplied |
| `Enabled` | Attempt cascading even without automatic persistence or with explicit placement |
| `Disabled` | Do not cascade this window |

`Automatic` is the default. Its decision uses the initial opt-in and id, not whether
a storage read or write would succeed. An unpackaged app that opts in with a non-empty id
therefore gets default cascading even though automatic storage is unavailable.

Any cascade attempt still requires a non-empty id and an eligible peer. `ApplicationRestart` suppresses
cascading for every value, including `Enabled`, in both `Show(options)` and
`TryApplyInitialPlacement(options)`. A first `Show` with `SkipInitialPlacement = true` does not
cascade either, because it runs no placement policy at all.
Hidden application can select a cascaded position without display.

### Select placement from a live peer

WinUI looks for another shown, initialized, overlapped window in the same group and on the
current virtual desktop. It excludes the new window itself, hidden or cloaked windows, closing
windows, and windows whose current placement cannot be captured. It prefers the eligible window
highest in Z order and continues to another candidate if capture fails. The search can cross
process and UI-thread boundaries; it does not use the native window class as app identity.
If WinUI cannot establish a candidate's eligibility safely, it skips that candidate.
Peer capture uses the same validity and unknown-restore-state rules as `TryGetPlacement`;
matching an id does not supply missing minimized-from-snapped history.

When cascading is permitted and no explicit placement is supplied, a captured peer placement
takes precedence over saved placement. WinUI copies the peer's current placement and cascades
its normal rectangle, then applies the reason policy and current-display adjustments.
A `Launch` monitor hint can still move the result to the launch monitor. If no peer qualifies
or discovery is unavailable, WinUI loads saved placement only when automatic persistence is
enabled; otherwise it uses normal initial-display fallback.

This copies placement data, not the peer's live presenter or resize configuration. It does not
move the peer or overwrite storage at launch. Maximized and snapped states can still overlap:
the cascade offset changes normal restore bounds, not the visible maximized or snap bounds.
Simultaneous launches can also choose the same position, and wrapping can reuse an occupied
position. WinUI does not reserve positions across processes.

### Cascade explicitly supplied placement

With `Automatic` or `Disabled`, explicit placement is not cascaded. With `Enabled`, the app
permits a position adjustment, not replacement of its entire placement by a peer's value.

First resolve the explicit placement's target monitor and normal geometry using the usual
reason policy, DPI adjustment, and current constraints. Then look for an eligible same-group
peer on that target monitor. Start the new normal rectangle at the peer's normal position,
retain the explicit placement's adjusted normal size, and apply the cascade offset and
position fitting. If no such peer qualifies, retain the uncascaded explicit result.

Cascading itself does not change the selected monitor, size, state, snap bounds, or desktop
policy. Ordinary reason and display adjustments still apply, including a `Launch` monitor hint,
minimized-state normalization, and unsupported-snap fallback. Thus an explicitly maximized or
snapped window is not converted to normal just to make its cascade offset visible.
The app's supplied object is not mutated by the operation.

### Group membership and lifetime

A shown, initialized window with a non-empty id can be a peer even if its own cascading or
automatic persistence is disabled. These settings control its own operations, not whether
other windows may position themselves relative to it. A hidden prepared window is not a peer
until it is shown.

Changing the id updates the group used by future peer searches without moving this window.
Clearing it removes the window from discovery. The initial persistence opt-in does not control
group membership. No property change, later `Show()`, or later `Activate()` reruns this
window's initial cascade. Changing the id alone cannot override an explicit Boolean opt-out.
Late Boolean changes affect saving only for an enrolled window; they do not affect group membership.

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

When WinUI restores placement, it adapts to the displays that are connected now rather than
blindly reusing saved screen coordinates. You do not need the same monitor layout as before.

WinUI makes the same adjustments whether it loads placement automatically or you supply it.
These adjustments happen during initial placement, before the window is shown. Later display
changes do not trigger another persistence restore; the window follows normal live windowing
behavior.

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

If automatic persistence is enabled when you later close the relocated window, WinUI attempts
to save its new placement. This feature keeps
one placement per id, not a separate placement for every monitor topology. Reconnecting the old
monitor does not make a running window return to its earlier location.

### The DPI changed

WinUI scales the normal size to keep approximately the same logical size. Moving from 100% scale
to 200% scale first doubles the physical-pixel width and height. WinUI then fits the window and
applies its current size constraints.

For example, a saved normal size of 800 by 600 physical pixels at 96 DPI has a nominal target
size of 1600 by 1200 physical pixels at 192 DPI. A smaller target work area or the window's current
size constraints can change the final result. Pixel rounding and window-frame calculations can
also affect the final bounds.

Position and size use different adjustments. The saved work area helps interpret the old position;
position is not simply multiplied by the DPI ratio. WinUI adapts it to the current monitor geometry
and work area. Exact offsets can differ between Windows versions and native placement paths.

### The resolution or taskbar work area changed

WinUI adapts the normal position to the current work area. For example, a window near the
right side of its former work area is placed toward the right side of the new work area rather
than keeping an absolute offset that could now be off-screen.

If the scaled size is too large, WinUI can shrink a resizable window to fit. It still follows
the window's current size constraints, even if they differ from the saved size. Restoring
placement does not change whether the user can resize the window.

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

For ordinary display, WinUI uses a saved minimized window's restore state:

| Saved state | State used for ordinary placement |
|---|---|
| `Minimized` | `Normal` |
| `MinimizedFromMaximized` | `Maximized` |
| `MinimizedFromSnapped` | `Snapped`, or `Normal` if snapping is unavailable |

Normal, maximized, and snapped placements otherwise retain their state, subject to current
system capabilities and constraints.

Only `ApplicationRestart` in the initial placement request preserves
saved minimization. This avoids an ordinary restore appearing to do nothing while allowing an
app to reconstruct its previous minimized windows. An explicit native launcher show command is separate from saved state;
see [Choose launch or restart policy](#choose-launch-or-restart-policy).

### The window was full-screen or compact overlay

Your app controls full-screen and compact-overlay presenters; this feature does not save those
modes. Capture and automatic saving use the most recent valid overlapped placement, including
its normal rectangle, maximized or snapped state, and matching monitor and DPI information.
They do not save the full-screen or compact-overlay bounds as normal window bounds.

If no valid overlapped placement is available, capture returns `false` and automatic saving is
skipped. WinUI does not invent a previous windowed placement.

Initial placement does not change the presenter you selected. If the window is already using
full-screen or compact overlay when it is first displayed, WinUI skips placement. Display options
still apply. Switching back to an overlapped presenter later does not make WinUI try initial
placement again.

To restore the windowed position before entering another presenter, first call
`TryApplyInitialPlacement(options)` while overlapped, then select the other presenter and
call `Show(new WindowShowOptions { SkipInitialPlacement = true })` to reveal the window.
This separates placement from display; it does not solve the hidden minimized/maximized
presenter-transition issue in S37.

## Use the initial-placement phase

A window is in its **initial-placement phase** from creation until it is first displayed. Two
different things can happen during that phase:

- `TryApplyInitialPlacement(options)` applies placement immediately while the window stays
  hidden. You can call it more than once. It never displays the window and never ends the phase.
- The first display performs at most one automatic placement pass and then ends the phase. An
  attempt that never reaches display leaves the phase open; see
  [Aborting before display](#aborting-before-display).

`Show`, opted-in initial `Activate()`, and hidden application share the same source-selection
and adjustment pipeline. Hiding the window or changing a property does not start a new phase.

Set `WindowShowOptions.SkipInitialPlacement` to `true` on the first `Show` when the window is
already where you want it. Skip preserves the window's current placement: that display performs
no automatic loading, no peer selection, no cascading, and no fallback repositioning. It still
ends the phase, so skipping does not postpone restoration to a later `Show`.

Use skip to reveal a window you prepared while it was hidden:

```csharp
window.TryApplyInitialPlacement(new WindowShowOptions());
window.Show(new WindowShowOptions { SkipInitialPlacement = true });
```

Without skip, the first `Show` follows normal placement policy even if you already applied
placement while the window was hidden. Skip and an explicit `Placement` cannot be combined:
an initial request that sets both fails with `E_INVALIDARG`.

Initial `Activate()` with `Window.UseAutomaticPlacementPersistence = true` and a non-empty id uses `Default`
reason, `Automatic` cascading, no explicit placement, and requests display and activation.
It takes no options parameter and reads no retained options property, so it cannot skip initial
placement. After preparing a window while hidden, reveal it with a skipping `Show` and call
`Activate()` afterward if you also need the foreground. Calling `Activate()` first runs the
automatic pass and can replace the placement you prepared.
With the property `false` or an empty id, `Activate()` retains the legacy activation path and ends
the phase upon first display without persistence enrollment.

First display directly through `AppWindow` or native APIs bypasses this pipeline and ends the
phase without establishing persistence enrollment, even when the property is `true`. Hidden
application does not establish enrollment either, so a window revealed only through those paths
is never enrolled. This proposal does not intercept those display paths to run automatic placement.

### Loading, applying, and displaying

| Operation | Placement work | Ends the initial-placement phase? | Automatic saving |
|---|---|---|---|
| `WindowPlacement.LoadForPersistPlacementId(id)` | Reads detached saved data only; no peer selection, adjustment, or application | No; no live window is required | Does not enable saving or write data |
| `TryApplyInitialPlacement(options)` | Applies selected placement now while hidden; null `Placement` permits peer/stored/fallback selection when the window property is `true` and the id is non-empty | No; repeatable until the window is displayed | Does not enroll, establish eligibility, or save |
| First `Show()` or `Show(options)` without skip | Selects and applies initial placement, then displays using the request's activation policy | Yes, on display, including fallback or application failure | Snapshots the window opt-in and records enrollment; display establishes eligibility, but does not save now |
| First `Show(options)` with `SkipInitialPlacement = true` | None; displays the window at its current placement | Yes, on display | Same enrollment and eligibility as the row above |
| First `Activate()` with the window property `true` and a non-empty id | Runs automatic initial placement with default policy, then displays and requests activation | Yes, on display, including fallback or application failure | Snapshots the window opt-in and records enrollment; display establishes eligibility, but does not save now |
| First `Activate()` with the property `false` or an empty id, or direct `AppWindow`/native first display | No new placement pipeline | Yes, on display | Does not establish enrollment |

Validation errors do not display the window, so they do not end the phase. A hidden attempt
never ends it either: a later `Show` or `Activate` still runs its normal placement policy unless
that `Show` skips initial placement. Loading a detached value is inspection, not an application
attempt. A display operation that aborts before the window is displayed does not end the phase
either; see [Aborting before display](#aborting-before-display).

While the phase is open, the pipeline:

1. Snapshots the request (or `Activate`'s defaults), any supplied placement, and the window's
   `UseAutomaticPlacementPersistence` and current id before callbacks. It validates this
   snapshot. An empty id is valid and disables automatic persistence and grouping.
2. Blocks nested display and placement calls for the rest of the operation, so a callback cannot
   start another attempt. A display operation ends the phase when the window is actually
   displayed, and at that point records persistence enrollment when the snapshotted Boolean is
   `true` and the id is non-empty. Hidden application does neither. Later property edits cannot
   create or remove that enrollment; they gate future save attempts as described in
   [Know when placement is saved](#know-when-placement-is-saved).
3. Selects explicit placement if supplied. Otherwise, it selects an eligible peer when
   cascading is permitted, then saved placement only when opted in, then fallback geometry.
4. Applies reason policy, cascading, and current-display adjustments in the order described in
   [Cascade related windows](#cascade-related-windows).
5. For `Show`, displays and requests activation as permitted by the reason, `DoNotActivate`, and
   resulting state. `ApplicationRestart` always suppresses activation. For `Activate`, displays
   and requests activation using its existing semantics.
   For `TryApplyInitialPlacement`, leaves the window hidden and never requests activation.

A first `Show` with `SkipInitialPlacement = true` runs steps 1, 2, and 5 only. It still validates
`Reason` and `CascadeBehavior`, and `ApplicationRestart` still suppresses activation, but no
placement policy is evaluated and the window is not moved, resized, or restated by that call.
Any window size still pending from `Width` or `Height` applies as it does today; that is ordinary
`Window` sizing, not placement. Successful hidden application already replaces a pending size.

`Show()` without arguments supplies default options and reads the window's persistence property.
With opt-in and no explicit placement, the default chain is eligible peer, saved placement,
then fallback. With opt-out and `Automatic`, neither peer selection nor automatic loading occurs.
`Enabled` still permits peer selection without storage opt-in.

Null `Placement` means no explicit source, not "disable automatic restore." With the window
property `true` and a non-empty id, it permits the peer/stored/fallback chain. A non-null `Placement` replaces
peer-source selection and automatic loading while retaining opted-in automatic saves.
Only `Enabled` permits position-only cascading of that explicit value.
The operation does not mutate the supplied objects. Changes during callbacks cannot replace
its snapshots. Later id changes affect subsequent saves and peer discovery, not this operation.

After the phase has ended, later `Show` calls ignore `Placement`, `Reason`, `CascadeBehavior`,
and `SkipInitialPlacement`, including invalid values in those ignored fields. Callers need not
repeat the skip setting: a later `Show` never reloads, recascades, or repositions the window,
and later `Show` and `Activate` do not reenroll through initial placement.
`DoNotActivate` remains a per-Show display option. `Show()` does not reset the window's opt-in.
Late Boolean changes are accepted and govern future saves only for an enrolled window;
they never retroactively restore, move, cascade, or reopen the phase.

| Action | Effect on the phase |
|---|---|
| Construct the window, configure its id and persistence property, edit options, capture, or load a detached value | Still open |
| Apply placement while hidden, successfully or not | Still open; repeatable while the window is undisplayed |
| Hide an already hidden window | Still open |
| Fail initial-option or placement validation | Still open; no persistence choice established |
| Begin a display operation that aborts before any display, leaving the window alive and hidden | Still open; the attempt's snapshots, including its enrollment decision, are discarded |
| Display the window through `Show`, opted-in `Activate`, opted-out `Activate`, `AppWindow`, or native APIs | Ended, including a skipping `Show` and including fallback or application failure |
| Hide or show after the phase has ended | Still ended |

A first display ends the phase even if native application fails; there is no second automatic
pass. Hidden application does not promise atomic application or rollback either, but it leaves
the phase open, so an app can attempt it again while the window is still hidden.
The exact hidden partial-application success boundary and backend fallback guarantees remain
review gates; see [Safe fallback and review gates](#safe-fallback-and-review-gates).

While the initial pipeline is running, additional `Show`, `Activate`, and `Hide` calls are
ignored, including calls from event handlers. `TryApplyInitialPlacement` returns `false`
without inspecting request fields. Required-argument, UI-thread, and closed-window checks
still apply before these reentrancy rules; null options are not an ignored request.
Property edits cannot replace the active snapshots. A close request is not ignored: normal
close behavior applies, and the operation does not continue applying or revealing a closed window.
This guard also applies when opted-in initial `Activate()` enters the pipeline. Existing nested
activation behavior outside the new pipeline, including the opted-out legacy path, is unchanged.

### Aborting before display

A first `Show` or opted-in first `Activate()` can pass validation, run the placement pass, and
still end without ever displaying the window. A callback can close the window, or the display
step itself can fail. The reentrancy guard above reserves the operation, but that reservation
is not a display.

If the window survives such an abort and is still hidden, the initial-placement phase stays
open. The next `Show` or `Activate` runs a complete initial pass again: it takes fresh request,
Boolean, and id snapshots, selects a source again, and applies placement again. The aborted
attempt leaves no remembered options, source choice, placement outcome, or enrollment.
Property edits made between the two calls take effect on the retry, because the window has not
been displayed yet.

| Outcome of the aborted attempt | Phase | Enrollment | Saving |
|---|---|---|---|
| Window survives and is still hidden | Still open; the next call retries | None yet; the retry decides it from its own snapshot | Never; the window has not been displayed |
| Window is closed during the attempt | Ends with the window | None | Never |
| Window becomes visible, then the operation fails later | Ended | Recorded from the attempt's snapshot | Eligible, subject to the current Boolean and id |

The third row is the existing rule: once the window is actually displayed, fallback or a later
failure does not reopen the phase. Only the first row is new, and it never makes a
never-displayed window eligible to save. See
[Why a pre-display abort can retry](#why-a-pre-display-abort-can-retry).

Apps do not need to detect an abort. A retry is ordinary code: call `Show` again. There is no
API to ask whether the phase is still open.

### Initial-placement fallback

Once a first display passes option validation, it ends the phase even if there is no eligible
peer or usable saved placement, a presenter that does not support placement, or a failure while
applying placement. Hidden application uses the same fallback rules but leaves the phase open.

In those cases, `Show` continues with normal initial display, opted-in `Activate` continues with
its activation behavior, and hidden application remains hidden.
WinUI applies any pending initial size before choosing placement, so that size provides fallback
bounds. If placement succeeds, its bounds replace the pending size, subject to current constraints.

A native placement call can change some bounds before failing. Fallback uses valid current
bounds; within that operation it does not promise to undo every change or try loading automatic
placement again.

For `Launch`, the shell monitor hint is also considered for fallback geometry when it can be
captured and adjusted. No saved value is required to use the hint.

If WinUI cannot apply your explicitly supplied placement, it does not try the automatically
stored value instead. Likewise, once a live-peer placement has been selected and native
application begins, failure does not cause another peer search or a storage load. Failure to
capture a peer before selection instead allows normal source selection to continue.
If only the cascade calculation cannot be completed safely, retain the uncascaded selected
placement and continue normal display adjustment.

Fallback preserves the operation's visibility and activation restrictions. `Show(options)`
honors `DoNotActivate` and the initial restart activation restriction; hidden application
remains hidden and never requests activation.
Failure must not be handled by showing or activating first and undoing it afterward.
Hidden application returns `false` if no selected placement was successfully applied;
preparing fallback geometry alone is not success. You can call it again while the window is
still hidden. A first display that skips initial placement does not retry it.

## Control display and activation

Pass a complete request to `Show(options)` or `TryApplyInitialPlacement(options)`.
The window does not retain an options property. Both methods require a non-null options
object; null fails with `E_INVALIDARG` (`ArgumentException` in .NET). Use `Show()` for default
display behavior or `TryApplyInitialPlacement(new WindowShowOptions())` for default hidden
placement. There is no parameterless `TryApplyInitialPlacement` overload.
A newly constructed options object has these defaults:

```csharp
var options = new WindowShowOptions
{
    Placement = null,
    Reason = WindowShowReason.Default,
    CascadeBehavior = WindowCascadeBehavior.Automatic,
    DoNotActivate = false,
    SkipInitialPlacement = false,
};
```

`Show` displays; hidden application does not. There is no hidden-show option.
Both operations read the window's persistence property at initial placement. Initial opted-in
`Activate()` uses these default policies without accepting or retaining an options object and
requests display and activation. An opted-out initial `Activate()` keeps its legacy path.
`ApplicationRestart` always preserves saved minimization, attempts saved-desktop restoration,
and suppresses activation and cascading. `DoNotActivate = false` does not override that policy.
Hidden application never requests activation for any reason. For minimized restart preparation,
specify `ApplicationRestart` before applying.

`SkipInitialPlacement` applies only to a first `Show`. It is not valid on a
`TryApplyInitialPlacement` request, which exists to apply placement, and it cannot be combined
with an explicit `Placement`. Both combinations fail with `E_INVALIDARG`. A skipping first
`Show` still honors `DoNotActivate` and the `ApplicationRestart` activation restriction; the
other placement effects of `Reason` and `CascadeBehavior` do not occur because no placement
policy runs.

Later display calls use current state:

| Method | Behavior after the initial-placement phase has ended |
|---|---|
| `Show()` or `Show(options)` on a shown window | No-op, including while minimized |
| `Show()` or `Show(options)` on a hidden window | Reveals its current state; requests activation only if not minimized and `DoNotActivate` is `false` |
| `Activate()` | Reveals if hidden, restores if minimized, and requests activation |
| `Hide()` | Hides without closing or resetting placement; no-op if already hidden |

Showing a hidden minimized window does not unminimize it. Use `Activate()` when the window should
return from minimization. The restore target is normal, maximized, or snapped according to its
current restore state and system capabilities.

Windows decides whether your app can take the foreground. An activation request does not
guarantee that your window becomes the foreground window. `DoNotActivate` on any `Show(options)`
means that operation does not request activation or transfer focus. The user or your later app
code can still activate the window.

Visibility changes raise `VisibilityChanged` using the existing Window visibility semantics;
non-activating display does not suppress that event merely because activation was not requested.
`TryApplyInitialPlacement` causes no visibility or activation events. Hiding causes
the normal visibility notification and, if applicable, deactivation. A visibility no-op does not
raise a visibility event.
Opted-in initial `Activate()` still uses normal display and activation notifications; sharing
initial placement does not give it hidden application's or non-activating Show's restrictions.

To reveal later without requesting activation, use
`window.Show(new WindowShowOptions { DoNotActivate = true })`. The existing
`window.AppWindow.Show(false)` also remains available; neither reapplies initial placement.
When that reveal is the window's first display and you already applied placement while hidden,
set `SkipInitialPlacement = true` in the same request so the first `Show` does not run the
automatic pass.

## Choose launch or restart policy

`Reason` selects initial-placement policy; it does not assign a
[window role](#window-roles-and-placement-needs), create a window hierarchy, or register the app
for restart.

The table describes initial requests. `ApplicationRestart` implies non-activating reconstruction,
regardless of `DoNotActivate`. Initial opted-in `Activate()` uses the `Default`/`false` row,
normalizes saved minimization, and requests activation. It does not read or retain these options.
The placement columns apply only when the first `Show` runs the placement pass; with
`SkipInitialPlacement = true` only the activation column applies.

| Reason | `DoNotActivate` | Saved minimization | Saved desktop | Monitor hint | Initial Show activation |
|---|---|---|---|---|---|
| `Default` | `false` | Use restore state | Ignore | Ignore | Requested |
| `Default` | `true` | Use restore state | Ignore | Ignore | Suppressed |
| `Launch` | `false` | Use restore state | Ignore | Use if valid | Requested |
| `Launch` | `true` | Use restore state | Ignore | Use if valid | Suppressed |
| `ApplicationRestart` | Either | Preserve | Restore, best effort | Ignore | Suppressed |

`ApplicationRestart` suppresses cascading, even with `CascadeBehavior = Enabled`.
The other rows follow the configured cascade policy. When a live peer supplies placement,
the same state and monitor rules apply to that snapshot as to a stored snapshot.

The restart restriction applies throughout the initial operation, including fallback and when
no saved desktop identity is available. It does not mutate the options object or create a
lifetime activation restriction. To bring a reconstructed window forward afterward, call
`Activate()` deliberately. After hidden restart application, reveal the window with
`Show(new WindowShowOptions { SkipInitialPlacement = true, DoNotActivate = true })`. That first
`Show` does not reread `Reason`, so set `DoNotActivate` explicitly for a non-activating reveal.

`Launch` prefers the monitor the shell selected for the launch over the saved monitor.
For example, launching from a taskbar on a different monitor can cause restoration on that
monitor. If no valid hint is available, normal saved-monitor selection applies.

_Spec note (2026-09-25): measured. A `GetStartupInfoW` probe shows that a launch with
redirected standard output reports `dwFlags=0x00000100` (`STARTF_USESTDHANDLES`) and a live
non-null `hStdOutput`, while a direct launch, an `explorer.exe` launch, and a `ShellExecute`
launch all report `hStdOutput=0x0`. So the hint is absent on those paths rather than misread.
WinUI now ignores `hStdOutput` whenever `STARTF_USESTDHANDLES` is set, because that flag means
the field is a standard output handle and cannot be combined with a monitor hint. The
packaged-app propagation question stays open: investigate the packaged launch path and file an
OS bug if appropriate. This is an open integration issue, not a change to the intended `Launch`
policy; when no valid hint is available, the fallback above still applies._

Set `Reason` to `Launch` on each window where you want the shell's monitor preference. WinUI
does not infer this from the order you create windows or from the first `Activate()` call.
With the opt-in and no explicit reason, you get `Default`, even in `OnLaunched`.
Setting only a non-empty `PersistPlacementId` opts in with the default Boolean, but does not select `Launch`.
The recommended [new-project main-window code](#recommended-main-window-code-for-new-projects)
sets `Launch` explicitly.

Native launcher show commands, such as `start /min`, are distinct from saved minimization.
Explicit merging of those commands with placement is deferred. `Reason = Launch` selects
monitor policy only; it does not opt into launcher show-command handling. This proposal does
not add command redistribution or replay after another window, such as a splash window.
Deferral does not mean suppressing existing Windows startup behavior. Native integration must
still account for that behavior and honor hidden application and `DoNotActivate`, without
introducing a new launcher-state precedence guarantee.

WinUI does not register your app for restart or keep a list of windows to recreate. Your app
handles those tasks and recognizes restart launches. For each reconstructed window, pass
`Reason = ApplicationRestart` to `Show(options)`; no separate `DoNotActivate` setting is needed.
Set a non-empty id beforehand and retain the default persistence Boolean to restore from the framework slot and save later.
You can activate a window afterward if needed.

On that non-activating restart path, WinUI attempts to return each window to its saved virtual
desktop without switching the user's active desktop. If the saved desktop no longer exists or
the move cannot be completed, the window stays on its current desktop.

Virtual-desktop identity is optional and best effort. Capture uses the most recent identity
safely available to WinUI for the window. That value can be missing or out of date, so do not
assume a captured value already reflects every desktop move.

After restart, you get the last successfully saved placement, which may not include moves made
just before a crash. This feature does not save on moves or resizes.

## Capture, change, and supply placement

`TryGetPlacement` returns a `WindowPlacement` data object that is separate from the window.
You can edit it without moving the source window.

Supply the value in `WindowShowOptions.Placement` on the initial `Show` or
`TryApplyInitialPlacement` request. WinUI snapshots and validates it, then applies it immediately
with that request's policy. A null `Placement` permits normal source selection, including
automatic restore when the window property is `true` and the id is non-empty; it does not disable restore or clear
previously applied placement. A non-null value overrides
automatic loading without disabling opted-in saving. It cannot be combined with
`SkipInitialPlacement = true`.

For hidden application, the Boolean describes application, not durable saving:

- `true`: a selected explicit, peer, or stored placement was successfully applied while hidden.
- `false`: the window has already been displayed, an operation is running, no usable placement
  was selected, the presenter is unsupported, or application failed. Fallback geometry alone is
  not success.
- An argument error: an eligible initial request contains invalid options or placement.

A failed attempt leaves the initial-placement phase open, so the app can try again while the
window is still hidden. A native failure can leave partial changes; the contract does not
promise rollback, and exact fallback guarantees remain under review. Reveal a prepared window
with `Show(new WindowShowOptions { SkipInitialPlacement = true })`; a first `Show` without skip
applies normal placement policy and replaces what you prepared.
See [Window.TryApplyInitialPlacement](#windowtryapplyinitialplacement-method).

Monitor removal, a changed DPI, a rectangle outside the current screens, and a missing virtual
desktop do not alone make placement structurally invalid. The saved geometry must still satisfy
its own work-area and data-validity rules. Accepted data can produce adjusted bounds rather than
its exact saved coordinates.

Capture and automatic saving choose placement data in the same way:

- For a shown overlapped window, WinUI uses its current geometry and placement state.
- For a hidden overlapped window, WinUI uses its current geometry and last meaningful show state.
- For a full-screen or compact-overlay window, WinUI uses its last valid overlapped placement.

Hiding does not replace the state with a fictitious `Hidden` value. Position or size changes made
while an overlapped window is hidden are included in later capture and saving. An app-initiated
state change also updates its meaningful state; hiding alone does not.

You can call `TryGetPlacement` before showing the window or applying placement while hidden.
It can capture valid current overlapped geometry and uses `Normal` if the window has no show
state yet. Capturing, loading, or editing that data alone does not make the window eligible for automatic saving.

### Customize one placement decision

You can supply explicit placement to change one decision while retaining WinUI's display
adjustment, sizing, and state handling. For example, your app can prefer the primary monitor
when the previously used monitor is no longer available, instead of using the default geometric
fallback.

Call `WindowPlacement.LoadForPersistPlacementId(id)` to read a detached value from the
framework's saved slot before initial placement. It does not select a peer, adjust or apply
placement, or enable saving. `TryGetPlacement` instead captures a live window; it does not
retrieve a previous run's saved placement.

Edit the loaded value and supply it in `Placement`, with a non-empty window id and the default persistence Boolean
set before the operation to retain automatic saving. Explicit placement suppresses automatic loading.
Alternatively, load from app-owned storage and explicitly set the Boolean to `false`. Keep an id if you want
cascade grouping; use `CascadeBehavior` to choose whether the explicit position can move.
Neither approach requires implementing native geometry adjustment. See
[Prefer the primary monitor when the saved monitor is missing](#prefer-the-primary-monitor-when-the-saved-monitor-is-missing).

## Know when placement is saved

Enrollment requires that the window's first display used `Window.Show` or `Window.Activate` and
snapshotted `Window.UseAutomaticPlacementPersistence = true` and a non-empty id. Each save also requires
the current Boolean to be `true`, a non-empty current id, and a usable packaged application
identity and store. Enrollment does not depend on a successful load or available storage.
First `Show` or opted-in first `Activate()` establishes enrollment, including when
explicit placement suppresses automatic loading and when the request sets
`SkipInitialPlacement = true`. Skipping placement never changes saving: saving is governed by
the window's persistence configuration, not by whether placement ran. Use explicit `false` to
disable saving, not skip. After first display, setting the Boolean to `false` suspends future
saves without removing enrollment; setting it back to `true` resumes eligible saves.

In addition, the window must have been shown, whether through `Window`, `AppWindow`, or native
APIs.

Applying placement while hidden does not qualify on its own, and neither does moving hidden
fallback geometry to match a launch-monitor hint. Neither does just creating the window or
loading or editing a value. These rules keep an unused window from replacing saved placement
with its default geometry.

A window that is never displayed therefore never saves, even if it was prepared, moved, or
resized while hidden. Hiding is not the disqualifier; never having been displayed is. Once a
window has been displayed, hiding it again does not suspend saving, so a window closed while
hidden still saves its current geometry and last meaningful show state. See
[Why a never-displayed window never saves](#why-a-never-displayed-window-never-saves).

| Trigger | Saving behavior |
|---|---|
| Accepted normal close | Capture after close handlers finish and before the native window is destroyed |
| Cancelled close | Do not save for that close |
| Other native destruction | Best-effort backstop if normal close did not already save |
| Confirmed sign-out or shutdown | Best-effort save while placement is still available |
| Cancelled sign-out or shutdown | Do not save for that session-end request |
| Hide, move, resize, or placement application | No persistence write |
| Crash or forced process termination | No guaranteed save opportunity |

For each save attempt, WinUI snapshots the current Boolean and id before capture and storage
access, after close handlers when saving for an accepted close. For an enrolled window,
changing the id directs future saves to that slot; clearing it suspends saves, and assigning
a non-empty id again permits future eligible saves when the Boolean is `true`. These changes do
not copy or delete data, move the window, retry loading, or write immediately. A save already in
progress keeps both snapshots; a later opt-out does not cancel that attempt.

A window whose first display snapshots explicit `false` or an empty id is not enrolled.
Later Boolean or id assignments, in either order, cannot enable automatic saving for that window.
Direct first display through `AppWindow` or native APIs also does not establish enrollment,
regardless of the configuration. Neither hidden application nor different options on a later
`Show` can enroll it. Before first display, property edits still affect the next initial
operation, even after a hidden application; enrollment uses the first display's snapshot, not
the hidden attempt's configuration.

| First-display enrollment | Current Boolean | Current id | Future automatic saves |
|---|---|---|---|
| Enrolled | `true` | Non-empty | Eligible, subject to actual display, capture, and storage |
| Enrolled | `false` | Any | Suspended; enrollment retained |
| Enrolled | `true` | Empty | Suspended; enrollment retained |
| Not enrolled | Any | Any | Disabled for this window's lifetime |

Both setters remain usable on the live window under the common UI-thread requirements.
They do not perform I/O or change placement. Clearing the id also removes cascade-group
membership; Boolean changes do not. A cancelled close neither saves nor undoes handler changes
to these properties, so the next eligible save uses the resulting configuration.

### Application identity and storage scope

Each placement slot is scoped by the current user, package family, registered application
identity within that family, and `PersistPlacementId`. A package family identifies a package
across versions and processor architectures; it can contain more than one application.
The application identity distinguishes those applications within the package.

For example, an editor app and a viewer app in the same package can both use `"MainWindow"` without
sharing a placement slot. Windows and processes belonging to the same application identity and
user share a slot when they use the same placement id; the last successful save wins.
`PersistPlacementId` values use ordinal, case-sensitive comparison without Unicode normalization.
The same id also identifies a cascade group as described in
[Cascade related windows](#cascade-related-windows). Cascading does not change the last-save-wins
storage rule or provide cross-process position reservations.

An update that changes only the package version or processor architecture keeps the same
namespace. A different registered application identity uses a different namespace; WinUI does
not automatically move saved placement between them.

If WinUI cannot resolve a usable application identity, it skips automatic loading or saving
for that operation. It does not fall back to a package-wide namespace. A missing
application-scoped value also does not cause a lookup in a shared package-wide placement slot.
This namespacing prevents accidental collisions through these APIs; it is not an additional
access-control boundary within the package's app-data store.

Your display or close call does not fail just because automatic persistence failed. This includes
missing application identity or storage, corrupt or unsupported stored data, and ordinary capture,
placement, or storage failures. Saving is best effort: a successful display or close call does not
prove that placement was saved.

The stored representation is private to WinUI. These APIs do not expose a serialization format,
a save-completed event, an application-supplied persistence store, or a per-id deletion operation.

Use one automatic placement mechanism per native window. WinUI does not coordinate with another
library or an app-owned routine that also restores and saves the same window. Use explicit
initial placement when you need to supply data to this pipeline rather than running a second
restore operation.

# Examples

_This section is intended for publication on learn.microsoft.com. The snippets assume an
initialized desktop WinUI app and application-defined window and content types._

## Remember the main window

For easy mode in an existing packaged app, set the id and keep initial activation:

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

Alternatively, set the id in `MainWindow.xaml`:

```xml
<Window
    x:Class="Contoso.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    PersistPlacementId="MainWindow">
    <!-- Window content -->
</Window>
```

With the id set in markup, `OnLaunched` can keep its existing display call:

```csharp
_mainWindow = new MainWindow();
_mainWindow.Activate();
```

To also use the shell's launch-monitor preference, replace the initial `Activate()` call
with this `Show` request:

```csharp
_mainWindow.Show(new WindowShowOptions
{
    Reason = WindowShowReason.Launch,
});
```

Use this launch-monitor snippet in place of the final `Activate` call in either version.
Initial opted-in `Activate()` uses `Default`, not `Launch`; once it has displayed the window,
later `Show(options)` cannot change that placement policy.

## Recommended main-window code for new projects

_Spec note: After this API is available, the WinUI File > New > Project templates should generate
the following main-window startup pattern. This is a template requirement, not a claim that
the current templates already generate these APIs._

Use easy mode for the default main window and explicitly select `Launch` policy:

```csharp
private Window? _mainWindow;

protected override void OnLaunched(LaunchActivatedEventArgs args)
{
    _mainWindow = new MainWindow
    {
        PersistPlacementId = "MainWindow",
    };
    _mainWindow.Show(new WindowShowOptions
    {
        Reason = WindowShowReason.Launch,
    });
}
```

Keep the window in the app's existing main-window field. Set the id before the one
`Show(options)` call; do not show the window in its constructor or through `AppWindow` first.
C++/WinRT templates should perform equivalent setup in `OnLaunched`. Using the template's
`MainWindow` implementation type and an existing `Window`-typed field named `m_window`:

```cpp
m_window = winrt::make<MainWindow>();
m_window.PersistPlacementId(L"MainWindow");

winrt::Microsoft::UI::Xaml::WindowShowOptions options;
options.Reason(winrt::Microsoft::UI::Xaml::WindowShowReason::Launch);
m_window.Show(options);
```

Leave `Window.UseAutomaticPlacementPersistence` at `true` and `CascadeBehavior` at `Automatic`.
The packaged app gets same-group cascading, saved-placement fallback, and automatic saving.
`Launch` additionally honors a valid shell monitor hint. Without a peer or saved value, WinUI
uses normal initial-display fallback while still considering that hint.

Use a stable, non-localized id for the main-window role, not a process id, title, or random value.
For an unpackaged template, generate an app-specific id, for example
`"Contoso.Editor:MainWindow"`, to reduce accidental matches in the shared unpackaged cascade
namespace. That configuration supports cascading and launch placement but does not add automatic
storage for unpackaged apps.

Selecting `Launch` is still easy mode; apps do not need custom capture or storage code.
An app that later adds restart recovery should use its separate `ApplicationRestart` path
instead of applying this ordinary-launch configuration to every reconstructed window.

## Remember independent windows

This example remembers a document window and a tool window (an inspector) independently.

```csharp
var document = new DocumentWindow
{
    PersistPlacementId = $"Document:{stableDocumentId}",
};
var inspector = new InspectorWindow
{
    PersistPlacementId = "Inspector",
};

document.Show();
inspector.Show();
```

`stableDocumentId` is an app-owned, non-sensitive identifier, not a localized title or a document
path. Use different ids for independently remembered windows. These windows also belong to
different cascade groups; repeating the same document id joins that document's group.

## Show without requesting activation

During application restart, restore a document window without taking focus. The app determines
that this is a restart and which document windows to recreate.

```csharp
var window = new DocumentWindow
{
    PersistPlacementId = $"Document:{stableDocumentId}",
};
window.Show(new WindowShowOptions
{
    Reason = WindowShowReason.ApplicationRestart,
});
```

Restart suppresses cascading. WinUI applies usable stored placement with current-display
adjustments, preserves saved minimization, and attempts to restore the saved virtual desktop.
Otherwise, it uses fallback placement. Neither case requests activation. Calling initial `Activate()`
instead with the property `true` would restore under `Default` policy, normalize saved minimization,
ignore the saved desktop, and request activation. It is not a non-activating restart display.

## Prepare a window while it remains hidden

An app that remembers its full-screen preference separately needs to establish windowed
return placement before selecting the full-screen presenter. This is the intended sequence;
the hidden minimized/maximized presenter transition remains an open integration issue in S37.

```csharp
var window = new MainWindow
{
    PersistPlacementId = "MainWindow",
};

bool applied = window.TryApplyInitialPlacement(new WindowShowOptions());

window.AppWindow.SetPresenter(Microsoft.UI.Windowing.AppWindowPresenterKind.FullScreen);
window.Show(new WindowShowOptions { SkipInitialPlacement = true });
```

This example deliberately accepts current fallback geometry if hidden application returns
`false` (`applied == false`); it does not assume that a saved return state was restored in that case.
Hidden application never itself shows or activates, and it does not use up the automatic
placement that first display would perform. `SkipInitialPlacement = true` is what keeps the
revealing `Show()` from loading, readjusting, or cascading over the prepared result. A plain
`Show()` here would run the automatic pass instead, and so would `Activate()`. Selecting the
presenter is separate: hidden application does not guarantee that the presenter transition stays
hidden. S37 remains open, including the `FullScreenPresenter` restore path for minimized and
maximized windows.
No separate loader call is needed: null `Placement` selects a peer, saved value, or fallback
under the policy. Waiting for data to load alone does not require early placement application.
Automatic saving is unaffected: the window is still enrolled at first display because it has a
non-empty id and the default persistence Boolean.

To customize the saved value first while retaining automatic saving, use this sequence instead:

```csharp
var window = new MainWindow
{
    PersistPlacementId = "MainWindow",
};
var placement = WindowPlacement.LoadForPersistPlacementId(window.PersistPlacementId);
if (placement is not null)
{
    placement.State = WindowPlacementState.Normal;
    placement.SnapRect = null;
}

bool applied = window.TryApplyInitialPlacement(new WindowShowOptions { Placement = placement });
window.AppWindow.SetPresenter(Microsoft.UI.Windowing.AppWindowPresenterKind.FullScreen);
window.Show(new WindowShowOptions { SkipInitialPlacement = true });
```

Loading, editing, and applying while hidden all leave the initial-placement phase open; the
revealing `Show` ends it. A non-null value overrides peer-source selection and automatic loading
while retaining automatic saving; null still permits automatic source selection and is not a
restore-disable switch. Skip and an explicit `Placement` are rejected in the same request, so
the hidden call carries the placement and the revealing call carries the skip.
This example accepts fallback on `false`.
Its normal-state customization does not solve S37's requirement to retain a maximized return state.

## Copy a window's placement without copying its state

```csharp
if (sourceWindow.TryGetPlacement(out var placement))
{
    placement.State = WindowPlacementState.Normal;
    placement.SnapRect = null;
    placement.VirtualDesktopId = null;

    var replacement = new DocumentWindow
    {
        PersistPlacementId = "Replacement",
    };
    replacement.Show(new WindowShowOptions
    {
        Placement = placement,
    });
}
```

The replacement window has not been shown, so its initial request can supply placement.
`Show` snapshots the value; changing `placement` afterward does not change that
operation. WinUI still adjusts for the current displays, so the result may not be pixel-identical
if the display configuration has changed.

C++/WinRT can also edit the detached capture directly:

```cpp
captured.State(winrt::Microsoft::UI::Xaml::WindowPlacementState::Normal);
```

Ordinary assignment of a runtime-class reference shares the same object; it does not copy its contents.

## Supply placement from app-owned data

In this example, you have validated your saved data and want to use it for this window:

```csharp
var placement = new WindowPlacement(savedNormalRect, savedWorkArea, savedDpi)
{
    State = savedWasMaximized
        ? WindowPlacementState.Maximized
        : WindowPlacementState.Normal,
    DisplayDeviceName = savedDisplayDeviceName,
};

var window = new MainWindow
{
    PersistPlacementId = "MainWindow",
    UseAutomaticPlacementPersistence = false,
};
window.Show(new WindowShowOptions { Placement = placement });
```

The rectangle, work area, and DPI must describe the same saved coordinate environment. Do not
substitute the current work area or DPI for missing saved metadata without first converting
the geometry. A legacy rectangle alone may not contain enough information to reproduce its
former logical size.

When an initial request supplies explicit placement, WinUI uses it instead of automatically loading a value.
This example leaves automatic saving off. To migrate to framework saving, set
`window.UseAutomaticPlacementPersistence = true` before the initial request. You decide when to stop
supplying old data. Successful display or hidden application is not proof of saving and is not
a signal to delete the old data.
This example does not coordinate old and new storage as a single transaction.

## Use expert-mode cascading without automatic storage

Keep the id for grouping, explicitly disable automatic persistence, and request cascading:

```csharp
var window = new DocumentWindow
{
    PersistPlacementId = "Contoso.Editor:Documents",
    UseAutomaticPlacementPersistence = false,
};
window.Show(new WindowShowOptions
{
    CascadeBehavior = WindowCascadeBehavior.Enabled,
});
```

WinUI can use a live peer without reading or writing its placement store. With no peer and no
explicit placement, it uses normal initial-display fallback. An app using its own store can
also supply `Placement` in that request; `Enabled` then permits position-only
cascading while retaining the explicitly selected monitor, adjusted size, and state.

The app remains responsible for its own save and load policy. To use automatic storage but
suppress cascading instead, opt in and set `CascadeBehavior = Disabled`.

## Prefer the primary monitor when the saved monitor is missing

Suppose your app should reopen on its saved monitor when available, but use the primary monitor
when that monitor is gone. This differs from the default fallback, which selects a monitor using
the saved normal rectangle.

Load a detached value from the framework slot, edit it, and supply it explicitly while opting
in to subsequent automatic saves. You do not need app-owned storage for this customization.

The app-defined `EnumerateCurrentMonitors` helper returns a complete monitor snapshot with
`DeviceName` and `IsPrimary` for each entry. It reports enumeration or query failures rather
than returning a partial list. The example requires exactly one primary monitor in that snapshot.

Implement the monitor helper with [EnumDisplayMonitors] using null device-context and clip
arguments, then [GetMonitorInfoW] with `MONITORINFOEXW`. Use `szDevice` for `DeviceName` and the
[`MONITORINFOF_PRIMARY` flag][MONITORINFO primary flag] for `IsPrimary`. Initialize `cbSize` to
the size of `MONITORINFOEXW` before each query. Use the device name, not a friendly display name
or a monitor's ordinal position.

```csharp
using System;
using System.Linq;

var window = new MainWindow
{
    PersistPlacementId = "Contoso.Editor:MainWindow",
};
var placement = WindowPlacement.LoadForPersistPlacementId(window.PersistPlacementId);
var options = new WindowShowOptions
{
    Reason = WindowShowReason.Default,
    CascadeBehavior = WindowCascadeBehavior.Disabled,
};

if (placement is not null)
{
    var monitors = EnumerateCurrentMonitors();
    var primaryMonitor = monitors.Single(monitor => monitor.IsPrimary);
    var savedMonitorIsMissing = placement.DisplayDeviceName.Length != 0 &&
        !monitors.Any(monitor => string.Equals(
            monitor.DeviceName, placement.DisplayDeviceName, StringComparison.Ordinal));

    if (savedMonitorIsMissing)
    {
        placement.DisplayDeviceName = primaryMonitor.DeviceName;
    }

    options.Placement = placement;
}

window.Show(options);
```

Only the monitor name changes. Keep `NormalRect`, `SnapRect`, `WorkArea`, and `Dpi` in their
saved coordinate environment; WinUI uses them to adapt geometry to the selected monitor. Do
not replace just the saved work area or DPI with the primary monitor's current values.
`Default` prevents a shell launch-monitor hint from overriding this choice. Using `Launch`
instead would allow that hint to take precedence.

If the saved monitor still matches, leave the placement unchanged. If its name is empty, this
example also leaves it unchanged and uses normal geometric fallback: the app cannot determine
that a particular former monitor is missing. Device names do not prove physical hardware identity.
If the explicit load returns null, the initial request can try automatic loading again.
If that also finds no usable saved placement, it uses normal initial-display fallback rather
than forcing first launch onto the primary monitor.

The example opts in to automatic saving but disables its own initial cascading.
This is expert mode: the app customizes monitor selection while WinUI owns storage and adjusts
geometry and state. Change `CascadeBehavior` to `Enabled` if a position-only cascade on the
selected monitor is also wanted. Loading reads only the saved slot, not a live peer.
If loading returns null, this request permits a normal automatic load and fallback, without cascading.
For app-owned storage instead, supply your loaded value and explicitly set the Boolean to `false`.
Display enumeration and placement application are not atomic: if the topology changes
between them, WinUI uses the monitors available when placement is applied and may choose a different
monitor. This is a preference based on the app's snapshot, not a topology-locking guarantee.

## Reconstruct windows after application restart

In this example, your app has registered for restart, recognized the restart launch, and kept
a list of windows to recreate:

```csharp
foreach (var savedWindow in savedWindowInventory)
{
    var window = CreateWindowFor(savedWindow);
    window.PersistPlacementId = savedWindow.PlacementId;
    window.Show(new WindowShowOptions
    {
        Reason = WindowShowReason.ApplicationRestart,
    });
}
```

`CreateWindowFor` creates content without showing the window. This path preserves saved
minimization and attempts virtual-desktop restoration. You can activate a window afterward
if needed. If all the saved windows were minimized, you can recreate them without activating any.

See [RegisterApplicationRestart] for the native registration API. Registration, inventory storage,
and application-data recovery are outside placement persistence.

# API Pages

_This section is intended for publication on learn.microsoft.com._

## Window.PersistPlacementId property

Gets or sets the id used for placement storage and cascade grouping.

```csharp
public string PersistPlacementId { get; set; }
```

The default is an empty string. Null is treated as empty. This property identifies a storage
slot and cascade group. Setting a non-empty id before the initial operation opts into automatic
loading and saving unless `Window.UseAutomaticPlacementPersistence` is explicitly `false`.
An empty id is valid and disables automatic persistence and grouping. The property setters do
not change each other, perform storage I/O, or apply placement, so constructor and XAML assignment
order is flexible. Explicit placement and ordinary display do not require an id.
Changing or clearing this property does not delete saved placement.

WinUI snapshots the id when choosing initial placement and again when saving. Setting or changing
the id does not move the window or repeat initial placement. Future saves and peer discovery
use the new id. For an initially opted-in window, clearing the id suspends saving and assigning
a non-empty id again permits future eligible saves. Later id changes alone do not override
an explicit Boolean opt-out. If the id was empty in the first-display snapshot, later assignment
cannot enroll the window or enable automatic saving.
Clearing the id also removes cascade-group membership.
See
[Know when placement is saved](#know-when-placement-is-saved) for identity and save timing.

For packaged apps, the id names a slot and cascade group within the registered application's
namespace. Another application in the same package can use the same id independently. See
[Application identity and storage scope](#application-identity-and-storage-scope).

For unpackaged apps, the id identifies a group in the shared unpackaged cascade namespace.
It does not enable automatic storage. See [Cascade related windows](#cascade-related-windows)
for collision and identity rules. There is no separate cascade-group property in this version.

This property does not provide an application-supplied storage location or application identity.
Automatic storage requires a usable packaged application identity and store; explicit placement
does not require either. Leaving persistence opted out does not clear the group id.

## Window.UseAutomaticPlacementPersistence property

Gets or sets the window's opt-in to automatic initial placement and eligible placement saving.

```csharp
public bool UseAutomaticPlacementPersistence { get; set; }
```

The default is `true`. Set a stable, non-empty `PersistPlacementId` before the window's first
placement or display operation to enable automatic persistence. Set this
property to `false` beforehand to opt out while retaining the id for grouping.
Each operation snapshots both properties before callbacks, and first display records enrollment
from that snapshot. An empty id disables automatic
persistence without error, regardless of the Boolean. Neither setter changes the other property,
performs storage I/O, or applies placement. Explicit `false` works in either assignment order.

| Configuration before initial placement | Automatic persistence |
|---|---|
| Empty id, Boolean default or explicit `true` | Disabled; no error |
| Non-empty id, Boolean default or explicit `true` | Enabled, subject to storage availability |
| Any id, explicit `false` | Disabled; non-empty ids remain usable for explicit cascading |

For setup-time use, `true` together with a non-empty id enables automatic source selection and subsequent eligible saves.
With null `WindowShowOptions.Placement`, source selection uses an eligible peer, then the saved
slot, then fallback under the reason and cascade policies. A non-null explicit placement
overrides peer-source selection and automatic loading while preserving automatic saves.
`Enabled` cascading can still adjust only its position. Automatic storage requires a usable
packaged application identity and store; unavailability is not an argument error.

Initial opted-in `Activate()` uses default reason and cascade policy and requests activation.
Initial opted-out `Activate()` follows the legacy path. Direct `AppWindow` or native first display
bypasses enrollment even with this property `true`, and applying placement while hidden does not
enroll the window by itself.
Setting a non-empty id opts in under the default Boolean. Loading detached data or supplying
explicit placement does not itself enable saving without the required window configuration.
`WindowShowOptions.SkipInitialPlacement` does not change this property's meaning: it skips the
first display's placement pass, not automatic saving. Use explicit `false` here to turn saving off.

You can change this property after first display. The setter stores the new value; it does not
reject or ignore a change merely because the window has been displayed. On an enrolled window,
`false` suspends future saves and `true` resumes them when the current id is non-empty. Neither
value changes enrollment. A window not enrolled at first display cannot enable automatic saving
by setting this property or the id later.

Before first display, changes affect the next initial operation, including one following hidden
application. No setter performs I/O or applies placement. After first display, changes never
restore, move, cascade, or reopen the initial-placement phase. An in-progress operation keeps
its Boolean and id snapshots. An accepted-close save snapshots both after close handlers, so
a handler can disable that save by setting this property to `false`.

See [Know when placement is saved](#know-when-placement-is-saved) for eligibility and the
established current-id save rules, which remain unchanged.

## Window.Show method

Runs initial placement and display setup, or shows a hidden window without unminimizing it.

```csharp
public void Show();
public void Show(WindowShowOptions options);
```

Both overloads require a desktop-backed window. On a non-desktop instance, an otherwise
valid call fails with `E_NOTIMPL` without display or placement side effects.

While the window has not been displayed, this method snapshots and validates the complete request
and the window's persistence property and id, selects and applies placement, then shows the window.
The parameterless overload uses default options, as does `Show(new WindowShowOptions())`.
Both read `Window.UseAutomaticPlacementPersistence`. A non-empty id opts in under its default
of `true`. `Show(options)` requires a non-null options object on every call, including later
or reentrant calls; null fails with `E_INVALIDARG` before any display or placement work.

`SkipInitialPlacement = true` displays the window at its current placement. That first `Show`
performs no automatic loading, peer selection, cascading, or fallback repositioning, which is
how an app reveals a window it prepared with `TryApplyInitialPlacement`. Skip still ends the
initial-placement phase, so it does not defer restoration to a later `Show`. Combining skip with
a non-null `Placement` fails with `E_INVALIDARG`. Skip does not affect enrollment or automatic
saving.

An initial `ApplicationRestart` request preserves saved minimization, attempts saved-desktop
restoration, and suppresses activation and cascading, even when `DoNotActivate` is `false`.
This includes fallback when saved placement or desktop information is unavailable. With skip,
only its activation restriction applies.

A first display with the window property `true` and a non-empty id establishes
persistence enrollment, with or without skip. It enables automatic source selection when no
explicit `Placement` is supplied and no skip is requested, and eligible later saving. Store
unavailability uses best-effort fallback, not argument failure. Placement application failure
also uses fallback; the void method is not an application-success or save-success result.
Display ends the initial-placement phase even on application failure; validation errors do not
display the window and leave the phase open. An attempt that aborts before the window is
displayed also leaves the phase open if the window survives, so a later `Show` runs a complete
initial pass again. See [Aborting before display](#aborting-before-display).

Once the window has been displayed, `Show()` does nothing if the window is already shown,
even if it is minimized. If the window is hidden, `Show()` reveals its current state. It does
not unminimize the window and requests activation only when showing a non-minimized window
with `DoNotActivate = false`. Later requests ignore `Placement`, `Reason`, `CascadeBehavior`,
and `SkipInitialPlacement` without validating them; callers need not repeat the skip setting.
They do not repeat placement or initial enrollment. Late Boolean changes only gate future
saves for an already enrolled window, as described on the window property page.

See [Control display and activation](#control-display-and-activation) for shared visibility,
event, and reentrancy behavior.

## Window.Activate method

Displays the window and requests activation using existing activation behavior.

```csharp
public void Activate();
```

While the window has not been displayed and `Window.UseAutomaticPlacementPersistence` is
`true` and the id is non-empty, this method snapshots that property and the current id before
callbacks and runs automatic initial placement using `Default` reason, `Automatic` cascading,
and no explicit placement. It uses peer, saved slot, then fallback precedence. Display ends the
initial-placement phase even on placement failure. An attempt that aborts before the window is
displayed leaves the phase open if the window survives. Storage unavailability uses
fallback, not argument failure.

This path establishes persistence enrollment and then requests display and activation. It
normalizes saved minimization and ignores saved desktop identity under default policy.
It is not a non-activating restart operation. The method accepts no new parameter and reads
no retained `WindowShowOptions`. Use `Show(options)` to configure reason, cascading, explicit
placement, or skipping.

With the property `false` or an empty id, retain the legacy activation path; first display
ends the phase without automatic placement or enrollment. An empty id is not an error.
An existing app can set only the id and keep this call for basic persistence.

`Activate()` cannot skip initial placement. If the app already applied placement while the
window was hidden, calling `Activate()` as the first display runs the automatic pass and can
replace that prepared placement. Reveal the window with
`Show(new WindowShowOptions { SkipInitialPlacement = true })` first, then call `Activate()` when
the window also needs the foreground.

This is a decided contract, not a pending review item. There is no `Activate` overload, no
retained skip property, and no implicit skip after hidden preparation. Debug builds trace a
development diagnostic when an opted-in first `Activate()` follows a `TryApplyInitialPlacement`
call on the same window. That diagnostic does not change placement, display, activation,
enrollment, or any return value. See
[Why Activate() gets no skip affordance](#why-activate-gets-no-skip-affordance).

Once the window has been displayed, `Activate()` shows it if hidden, restores it if minimized,
and requests activation, without loading, adjusting, or cascading again. After hidden
non-activating restart preparation revealed with a skipping `Show`, it restores the prepared
minimization without reloading.
Unlike `Show`, it can restore minimization. The current restore state determines whether the
window returns to normal, maximized, or snapped, subject to system capabilities.

## Window.Hide method

Hides the window without closing it.

```csharp
public void Hide();
```

This method requires a desktop-backed window. On a non-desktop instance, an otherwise
valid call fails with `E_NOTIMPL` without changing visibility or audio state.

The native window and XAML content stay alive. `Hide()` does not raise `Closed`, save placement,
or reopen the initial-placement phase. It also hides windows you showed directly through
`AppWindow`. If the window is already hidden, `Hide()` does nothing.

See [Control display and activation](#control-display-and-activation) for event and reentrancy
behavior, and [Capture, change, and supply placement](#capture-change-and-supply-placement) for
capture and saving while hidden.

## Window.TryGetPlacement method

Tries to capture the window's placement as an independent data object.

```csharp
public bool TryGetPlacement(out WindowPlacement placement);
```

Returns `true` with a valid placement. If WinUI cannot capture valid placement, it returns
`false` and sets `placement` to null. Capture can succeed without an optional monitor name or
virtual-desktop identity.

You can edit the returned object without moving the window or changing earlier captures.
The object does not keep the window alive, and it does not update when the window moves.

Hidden and non-overlapped capture follow
[Capture, change, and supply placement](#capture-change-and-supply-placement). If the restore state
of a minimized window is unknown, capture uses `Minimized` rather than guessing that the window
should restore to maximized or snapped.

Calling this method does not read or write placement storage, or end the initial-placement phase.
Calling it on the wrong thread or after the window has closed produces an API-use error rather
than returning `false`.

## Window.TryApplyInitialPlacement method

Tries to apply initial placement immediately while the window remains hidden.

```csharp
public bool TryApplyInitialPlacement(WindowShowOptions options);
```

This method runs the same source-selection, reason, cascade, and display-adjustment pipeline
as initial `Show` and opted-in initial `Activate`, but never shows or requests activation.
It is not a staging method or a general repositioning API. The options object is required.
For default policy with no explicit placement, pass `new WindowShowOptions()`. There is no
parameterless overload. Null fails with `E_INVALIDARG` (`ArgumentException` in .NET), including
on late or reentrant calls; it does not return `false`.

After checking required arguments, WinUI checks the UI thread and closed-window state.
If the window has already been displayed or an operation is running, it returns `false`
without validating ignored request fields. Otherwise it snapshots the request, placement, window
persistence property, and id before callbacks and validates them. An empty id is valid and
disables automatic persistence and grouping. `SkipInitialPlacement = true` is not a valid
request for this method and fails with `E_INVALIDARG`.

Null `Placement` means no explicit source. With the window property `true` and a non-empty id, this method selects
an eligible peer, then a saved value, then fallback under policy; callers need not load first.
A non-null `Placement` uses that value instead of peer-source selection or automatic loading.
`Automatic` and `Disabled` do not cascade
explicit placement; `Enabled` permits only position adjustment from a same-monitor peer.
Restart suppresses all cascading and preserves saved minimization and desktop targeting
regardless of `DoNotActivate`. For every reason, this method never requests activation.

Returns `true` when a selected explicit, peer, or stored placement was successfully applied
while hidden. Returns `false` for no usable source, unsupported presenter, or application
failure; fallback geometry alone is not success. No result means durable saving occurred, and
this method never enrolls the window for automatic saving: enrollment happens at first display,
from the window's persistence configuration.

This method does not end the initial-placement phase, whether it succeeds or fails. You can
call it again while the window is still hidden. Because the phase is still open, the first
`Show` or `Activate` afterward applies its normal placement policy and can replace what you
prepared. Reveal a prepared window with
`Show(new WindowShowOptions { SkipInitialPlacement = true })`; add `DoNotActivate = true` when
the reveal must not request activation. `Activate()` cannot skip placement, so call it only
after that first `Show`. Loading a detached value alone applies nothing.
See [Loading, applying, and displaying](#loading-applying-and-displaying).
A failure can leave partial native changes; the contract does not promise
rollback. Exact partial-application success and safe fallback guarantees remain
[review gates](#safe-fallback-and-review-gates), including the presenter transition in S37.

## WindowPlacement class

Represents editable window placement data that is separate from any live window.

```csharp
public sealed class WindowPlacement
{
    public WindowPlacement(RectInt32 normalRect, RectInt32 workArea, int dpi);
    public static WindowPlacement? LoadForPersistPlacementId(string persistPlacementId);

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

The constructor validates the required geometry and DPI. You can temporarily leave properties
in an invalid combination while editing a value. Each eligible initial placement request checks a complete
snapshot against these rules:

- `NormalRect` and `WorkArea` have positive width and height.
- `NormalRect` has a positive-area intersection with the saved `WorkArea`.
- Any present `SnapRect` has positive width and height.
- Each rectangle's right and bottom edges are representable as signed 32-bit coordinates when
  computed from its X, Y, width, and height using checked arithmetic.
- `Dpi` is at least 96.
- `State` is a defined `WindowPlacementState` value.
- `Snapped` and `MinimizedFromSnapped` require `SnapRect`.
- `DisplayDeviceName` is empty or contains at most 31 UTF-16 code units, without embedded NUL.

These rules check the saved data, not whether the saved monitor or desktop still exists.
The normal rectangle must intersect the saved work area, not a currently connected monitor.
WinUI also uses checked arithmetic when adjusting placement. If the adjusted rectangle is invalid
or cannot be represented safely, WinUI uses fallback placement.

`SnapRect` is ignored for other states. You can set `State` to `Normal` to remove both snapping
and minimized restore-state instructions without manipulating separate flags.

Null `DisplayDeviceName` is treated as empty. `VirtualDesktopId` can be null; `Guid.Empty` is
treated as no identity when placement is accepted.

### Threading

You can share `WindowPlacement` across threads; it is an agile object. Individual property reads
and writes are thread-safe. Initial placement application copies all fields together
as one coherent snapshot.

Changing several properties is not one transaction. Another thread can take a snapshot between
your assignments and see an intermediate combination. If that snapshot is invalid, application
rejects it. Finish related edits before sharing a value for use.

You must still call `Window` methods on that window's UI thread. Sharing the placement object
across threads does not let you access the window from another thread.

## WindowPlacement constructor

Initializes a placement from a normal rectangle and its associated monitor work area and DPI.

```csharp
public WindowPlacement(RectInt32 normalRect, RectInt32 workArea, int dpi);
```

The initial `State` is `Normal`, `SnapRect` and `VirtualDesktopId` are null, and
`DisplayDeviceName` is empty. Invalid required geometry or DPI fails with `E_INVALIDARG`
(`ArgumentException` in .NET).

The constructor does not inspect the current monitor layout or move a window.

## WindowPlacement.LoadForPersistPlacementId method

Reads a detached placement value from the framework's saved slot.

```csharp
public static WindowPlacement? LoadForPersistPlacementId(string persistPlacementId);
```

Uses the same current-user, packaged-application identity, and exact id namespace as automatic
persistence. A null or empty id is `E_INVALIDARG` (`ArgumentException` in .NET).
The method is synchronous, can be called from any thread, and requires no live window.
It does not add automatic storage for unpackaged apps.

Returns a valid detached value, or null for the following outcomes:

| Outcome | Result |
|---|---|
| No packaged application identity or supported default store; access to the store is denied | Null |
| The application container or placement value is absent | Null |
| The value has the wrong type, exceeds the record bounds, is corrupt, or uses an unsupported record version | Null |
| Any other identity-resolution, storage, decoding, or value-construction failure | Fails with the original failing HRESULT |

An unexpected failure is not converted to null or a generic `E_FAIL`. C++/WinRT reports an
`hresult_error` or its derived type; .NET reports the exception corresponding to that HRESULT.
For example, allocation failure uses `E_OUTOFMEMORY` (`OutOfMemoryException` in .NET). A failure
with no native error code uses `E_FAIL`; it must still be diagnosed as an unexpected failure.
Deployment, threading, and object-lifetime errors are not expected store unavailability.
The private backend classification is defined in the
[shared reader design](../docs/design-notes/Window-PlacementPersistence.md#one-reader-for-automatic-restore-and-detached-loading).

WinUI records internal failure diagnostics containing the operation, category, and error code,
not ids, storage names, placement data, or arbitrary backend error text. An ordinary missing
record is not an error. Automatic restore remains best effort for the same unexpected failures;
this exception contract applies to the explicit detached load.
The returned object contains a coherent saved snapshot, not current-display-adjusted geometry.

Loading does not select a peer, apply placement, end the initial-placement phase, or enable
automatic saving. Editing the result does not write the slot. Supply the value in an initial
request's `Placement`, and configure a non-empty window id with the default persistence Boolean
before first display if subsequent automatic saving is wanted. This method does not adjust geometry
for current displays or affect any window's placement phase.

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

For explicit initial placement, you can change `DisplayDeviceName` to prefer another current
monitor while leaving the saved rectangles, work area, and DPI together for adjustment. A valid
shell monitor hint still takes precedence under `Launch` policy. See
[Prefer the primary monitor when the saved monitor is missing](#prefer-the-primary-monitor-when-the-saved-monitor-is-missing).

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

You can use these states in placement you construct yourself. They describe the state you want,
not a history that WinUI needs to verify. Initial policy can still restore a saved minimized
window, and current system capabilities can require snapped placement to fall back to normal.

## WindowShowOptions class

Provides an initial placement request and per-Show activation control.

```csharp
public sealed class WindowShowOptions
{
    public WindowShowOptions();

    public WindowPlacement? Placement { get; set; }
    public WindowShowReason Reason { get; set; }
    public WindowCascadeBehavior CascadeBehavior { get; set; }
    public bool DoNotActivate { get; set; }
    public bool SkipInitialPlacement { get; set; }
}
```

| Property | Default | Meaning |
|---|---|---|
| `Placement` | null | Null means no explicit source, so the window's `true` persistence property permits peer/stored/fallback selection; non-null overrides automatic loading while retaining opted-in saves |
| `Reason` | `Default` | Selects launch, restart, or ordinary initial-placement policy |
| `CascadeBehavior` | `Automatic` | Selects initial cascading; default cascading requires the window's persistence property `true`, a non-empty id, and no explicit placement; restart always suppresses it |
| `DoNotActivate` | `false` | Suppresses activation for this `Show`; initial `ApplicationRestart` suppresses activation regardless of this value |
| `SkipInitialPlacement` | `false` | On a first `Show`, displays the window at its current placement instead of running the placement pass |

`SkipInitialPlacement` skips automatic loading, peer selection, cascading, and fallback
repositioning on that one display. It still ends the initial-placement phase and does not change
persistence enrollment or automatic saving. It is meaningful only on a first `Show`:
`TryApplyInitialPlacement` rejects it, `Activate()` cannot express it, and a later `Show` ignores
it like the other placement fields.

Hidden application never activates regardless of `DoNotActivate`. `Activate()` does not accept
or read these options. After the initial-placement phase has ended, `Show` reads only
`DoNotActivate`; placement-specific fields are ignored, including invalid values.
Persistence opt-in is a property of `Window`, not this request. There is no placement-taking
constructor; supply a value with `new WindowShowOptions { Placement = value }`.

You can share this agile object across threads. Individual property reads and writes are
thread-safe, and each operation snapshots the option values together. `Placement` holds the
assigned reference until the operation snapshots its fields coherently. The options snapshot
and the referenced placement snapshot are not a transaction across both mutable objects.
Finish related edits before submitting a request; subsequent edits cannot affect that operation.
Multiple assignments are not one transaction. Each window takes its own snapshots.

For an eligible initial request, undefined `Reason` or `CascadeBehavior`, invalid placement, or
`SkipInitialPlacement = true` combined with a non-null `Placement`
fails with `E_INVALIDARG` before the window is displayed or
changed. `TryApplyInitialPlacement` additionally rejects `SkipInitialPlacement = true`.
Undefined enum values fail even if that policy would be skipped, such as
cascading during restart or any placement policy under skip. Setters do not validate enums.
Store unavailability is not an option validation failure.

See [Choose launch or restart policy](#choose-launch-or-restart-policy) and
[Cascade related windows](#cascade-related-windows) for the policy tables.

## WindowShowReason enum

Specifies the policy used for initial placement.

| Name | Value | Meaning |
|---|---|---|
| `Default` | 0 | Ordinary restore without launch-monitor preference |
| `Launch` | 1 | Ordinary restore with a valid shell launch-monitor preference |
| `ApplicationRestart` | 2 | Reconstruction after application restart |

`ApplicationRestart` preserves saved minimization, attempts virtual-desktop restoration, and
suppresses activation and cascading in the initial operation. `DoNotActivate = false` cannot
override its activation restriction. This policy applies to both initial `Show` and hidden
`TryApplyInitialPlacement`. With `SkipInitialPlacement = true`, only its activation restriction
applies, because no placement policy runs. Initial opted-in `Activate()` uses
`Default` policy and requests activation; it cannot select non-activating restart.
Restart suppresses cascading regardless of `CascadeBehavior`.

## WindowCascadeBehavior enum

Specifies whether an initial operation attempts to cascade from a related live window.

| Name | Value | Meaning |
|---|---|---|
| `Automatic` | 0 | Attempt cascading when automatic persistence is opted in and no explicit placement is supplied |
| `Enabled` | 1 | Attempt cascading without storage opt-in or with explicit placement |
| `Disabled` | 2 | Do not cascade this window |

All values are subject to the group-identity and eligible-peer rules.
`ApplicationRestart` suppresses cascading even with `Enabled`. `Enabled` with explicit placement
permits only the position adjustment described in
[Cascade explicitly supplied placement](#cascade-explicitly-supplied-placement).
It does not enable automatic loading or saving.

`Disabled` does not prevent the shown window from being a peer for other matching windows.
Later display calls do not read this option or cascade again, and a first `Show` with
`SkipInitialPlacement = true` does not cascade at all. Unknown values fail initial
option validation even when the operation would otherwise skip cascading.

## Common Window API requirements

Call the new `Window` properties and methods on the window's UI thread. If you call them from
another thread or after the window has closed with valid required arguments, they fail with the
corresponding existing Window API error. WinUI performs these checks even when an initial
operation is already running and would otherwise ignore a nested display call.

`Show(options)` and `TryApplyInitialPlacement(options)` reject a null options argument with
`E_INVALIDARG` before thread, closed-window, host, or phase checks. This required-argument
check applies to every call. It is separate from validating fields of a non-null request.
The nullable `WindowShowOptions.Placement` property remains valid and means no explicit source.

When a request is validated, invalid app-supplied options or placement use `E_INVALIDARG`
(`ArgumentException` in .NET). Calls after the initial-placement phase has ended, and reentrant
calls, ignore request fields as described in the method pages; they do not validate those
ignored fields.
Automatic persisted-data failures instead follow the fallback and best-effort saving rules.
The `Try` methods can still report programming errors. Neither method's Boolean result tells
you whether data was saved.

# API Details

_Spec note: This section supports API review and is not intended for publication on
learn.microsoft.com. Existing Window members are omitted; `Activate()` retains its signature
and activation behavior while adding opted-in initial automatic placement._

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
    enum WindowCascadeBehavior
    {
        Automatic = 0,
        Enabled = 1,
        Disabled = 2,
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

        static WindowPlacement LoadForPersistPlacementId(String persistPlacementId);

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
    runtimeclass WindowShowOptions
    {
        WindowShowOptions();

        WindowPlacement Placement;
        WindowShowReason Reason;
        WindowCascadeBehavior CascadeBehavior;
        Boolean DoNotActivate;
        Boolean SkipInitialPlacement;
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
    [webhosthidden]
    unsealed runtimeclass Window
    {
        // ... existing members ...

        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        {
            String PersistPlacementId;
            Boolean UseAutomaticPlacementPersistence;

            Boolean TryGetPlacement(out WindowPlacement placement);
            Boolean TryApplyInitialPlacement(WindowShowOptions options);

            [method_name("ShowDefault")]
            void Show();

            [method_name("ShowWithOptions")]
            void Show(WindowShowOptions options);

            [method_name("HideDefault")]
            void Hide();
        }
    };
}
```

# Appendix

_Spec note: This section supports API review and is not intended for publication on
learn.microsoft.com._

## Placement scenarios and WinUI decisions

PlacementEx is the planned native placement engine. This inventory retains the concrete scenarios
the design must address, rather than only describing the general adjustment algorithm. Using
PlacementEx does not automatically adopt every policy in its sample workflow.

The outcomes below summarize the current WinUI proposal, not verified prototype behavior.
Rows marked **Open** remain design or integration questions; they do not add guarantees to the
conceptual or API sections. Cases outside initial-placement restoration are retained so their
boundaries are explicit rather than silently omitted.

Unless a row says otherwise, assume an overlapped window with usable saved placement,
a non-empty id and `Window.UseAutomaticPlacementPersistence = true` before its initial operation,
no eligible live cascade peer, and no applicable native launcher
show command. A valid shell monitor hint takes precedence only with
`Reason = Launch`; `Default` ignores it. **Non-activating restart** means the app recreates the
window by calling `Show(options)` with `Reason = ApplicationRestart`, which implies no activation.
WinUI does not detect
restart or recreate the window inventory for the app. Storage remains subject to packaged
application identity, save eligibility, and best-effort loading and saving.

### Saved placement, launch, and display changes

| Id | Scenario | How this proposal handles it |
|---|---|---|
| S01 | Close on monitor A, remove A, then connect B at the same screen coordinates | If the saved display name no longer matches, select using the saved normal rectangle. B is selected if it has the largest intersection. Adapt the saved size and position to B; do not promise identical pixels. A matching saved display name takes precedence over this geometric fallback, even if different hardware now uses that name. |
| S02 | Close on A, then move A elsewhere in Display Settings | Follow A if its display device name still matches, even if B now occupies the old coordinates. Adapt to A's current position, work area, and DPI. If the name no longer matches, use normal-rectangle monitor selection. |
| S03 | Save on virtual desktop 3, then restart while desktop 1 is active | On non-activating restart, attempt to move to the saved desktop GUID without switching the active desktop or requesting activation. A failed or unavailable desktop move leaves the window on its current desktop. Safe state restoration on each backend remains subject to the review gates below. |
| S04 | Save on desktop 3, then launch normally while desktop 1 is active | Ignore the saved desktop identity and leave the newly created window on its current desktop. Ordinary placement policy does not return it to desktop 3. |
| S05 | Launch from a taskbar button on a different monitor | With `Reason = Launch`, prefer a valid shell monitor hint and adapt both normal and snapped bounds as applicable. With opt-in and no explicit reason, default policy uses saved-monitor selection. Setting only a non-empty id opts in under the default Boolean, but does not select launch-monitor policy. Clicking an already-running window's taskbar button does not rerun initial placement. |
| S06 | Close with the taskbar at the bottom, move it to the top, and relaunch | Adapt to the new work area and fit subject to current constraints. Do not promise a shift equal to the taskbar height: native and downlevel placement can produce different offsets. A maximized window uses its current monitor/work area, while its normal restore bounds are adapted separately. |
| S07 | Launch two instances of the same app with the same placement id | Easy mode prefers an eligible live peer, captures its current placement, and cascades its normal rectangle instead of loading the old saved value. With no peer, it uses saved placement or fallback. Maximized/snapped states, wrapping, and simultaneous launches can still overlap. There is no position reservation; automatic storage remains last-successful-save-wins. |
| S08 | Close at 100% display scale, then relaunch at 200% | Scale the normal size to retain approximately the same logical size. An 800 by 600 size has a nominal target of 1600 by 1200 physical pixels before fitting and constraints. Position is adjusted separately; frame calculations and rounding can change the final bounds. |
| S09 | Save a large window on an external monitor, then relaunch with only a smaller laptop display | Select the laptop display and adapt the placement. A currently resizable window can shrink to fit, subject to its size constraints. A fixed-size window or a minimum size larger than the work area can remain partly off-screen; do not promise complete containment. |
| S10 | Minimize the app, close it, then launch normally | Use the saved restore state: normal, maximized, or snapped. If snap is unavailable, use normal. This normalization is separate from an applicable launcher minimize request; see S15. |
| S11 | Leave the app minimized, then restart Windows | If the minimized state was successfully saved and the app uses non-activating restart, retain minimization and its restore target. Initial `Activate()` with the window property `true` instead uses `Default`, normalizes saved minimization, ignores the saved desktop, and requests activation. After hidden restart application, `Activate()` restores prepared minimization without reloading, unlike `Show`, which preserves it. Exact hidden/non-activating state support remains an implementation review gate. |
| S12 | Save on desktop 3, delete that desktop, then restart | On non-activating restart, attempt restoration by the saved GUID, not the ordinal number 3. If the move cannot be completed, stay on the current desktop. Do not create a replacement desktop or select a different desktop merely because it is now third. |
| S13 | Disconnect A, open and close on B, then reconnect A and relaunch | If the close successfully saved B's placement, use that latest placement. There is one saved value per id, not a history per display topology. Reconnecting A does not recover the older value; a valid launch-monitor hint can still select A under `Launch` policy. |
| S14 | Launch for the first time with no saved placement | Use normal initial-display fallback, including the app's pending initial size. Under `Launch`, consider a valid monitor hint when fallback geometry can be captured and adjusted. Do not adopt the PlacementEx sample's default window size as a new WinUI default. Effective hidden and activation options still apply. |
| S15 | Close maximized, then launch with an explicit minimize request | **Deferred:** this version does not add explicit launcher-state merging or guarantee that `/min` overrides saved maximization. Existing native startup behavior is not deliberately suppressed. Retain the earlier design discussion in [Launcher show-command scenarios](#launcher-show-command-scenarios-deferred). |
| S16 | Save snapped to the left half, then relaunch with a different work-area size | Adapt the visible snap rectangle relative to the target work-area edges and adapt the normal restore rectangle separately. Apply snap only when supported and enabled; otherwise use normal placement. Do not reconstruct the other windows or membership of a snap group. |
| S17 | Enter full-screen on A, move the full-screen window to B, then exit full-screen | This is a live presenter transition, not another initial-placement operation. The app and presenter control it; choosing PlacementEx for persistence does not establish a new guarantee that exiting full-screen restores on B. Capture and saving use the last valid overlapped placement, not full-screen bounds. Any additional live-transition requirement belongs in the presenter design. |

### Boundary cases that must remain covered

| Id | Scenario | How this proposal handles it |
|---|---|---|
| S18 | The saved display name is missing and the normal rectangle intersects no current monitor | Choose the nearest current monitor, not unconditionally the primary monitor. Use the saved normal rectangle for selection, not the saved work area or snap rectangle. |
| S19 | A valid saved window is partly or completely outside today's screens | Adapt to the selected monitor and fit where current constraints permit. Do not preserve intentionally off-screen placement. The saved normal rectangle must still intersect its own saved work area; invalid stored data uses fallback. A failed live capture does not overwrite a previously saved value. |
| S20 | A taskbar occupies the left side, or a monitor has negative screen coordinates | Keep public rectangles in physical virtual-screen pixels, including valid negative coordinates. Convert native coordinate systems internally; equal workspace-coordinate numbers are not a promise of equal screen positions. Apply the same work-area adaptation rules as S06. |
| S21 | The saved window was resizable, but the new target is fixed-size or has different constraints | Use the current target's capability and constraints, not saved source-window resizability. Placement does not change that configuration. Oversized results remain subject to S09. |
| S22 | A snapped window is minimized before capture, or snapping is disabled before restore | Use recorded minimized restore history when available. If the minimized restore state is unknown, capture `Minimized` rather than inventing snapped history. If a known snapped restore target cannot be retained, use normal; non-activating restart still retains minimization where supported. |
| S23 | Non-activating restart has no usable saved virtual-desktop identity | Leave the window on its current desktop. `ApplicationRestart` still prohibits an activation request, even with `DoNotActivate = false`; its behavior must not depend on whether a desktop GUID was captured. Use a safe fallback if the backend cannot apply the requested state under that restriction. |
| S24 | Prepare placement while hidden, then reveal the window later | `TryApplyInitialPlacement(options)` applies now without display or activation, and can be called again while the window is still hidden. It does not end the initial-placement phase. Null placement with the window property `true` selects peer/stored/fallback without requiring a loader call. Reveal with `Show(new WindowShowOptions { SkipInitialPlacement = true })` so the prepared placement is preserved; add `DoNotActivate = true` to reveal without requesting activation. A first `Show` or `Activate()` without skip still runs the automatic pass and can replace the prepared placement, because the phase is still open. `Activate()` cannot skip, so call it only after that first `Show`. Hidden geometry changes are included in capture. Hidden application does not enroll the window; automatic saving still requires enrollment at first display. Partial-application limits remain open. |
| S25 | A launcher requests maximize, minimize-from-snapped, hide, or no activation; or a splash window was shown first | **Deferred:** command ownership, precedence, and explicit replay handling are not part of this version. Retain the cases in [Launcher show-command scenarios](#launcher-show-command-scenarios-deferred) for future consideration. Compatibility with existing native startup behavior and honoring the app's explicit visibility and activation options remain implementation requirements. |
| S26 | Capture a full-screen or compact-overlay window with no valid overlapped history | Return `false` and skip automatic saving. Do not invent windowed bounds. If the automatic pass runs while a non-overlapped presenter is selected, end the initial-placement phase without changing the presenter; switching back later does not retry. |
| S27 | Displays change while the window is already running | Leave live migration to ordinary windowing behavior; do not rerun persistence restoration on a display-change notification. A later successful save records the resulting placement. This feature does not replace the system's live-window disconnect/reconnect handling. |

### Developer customization

| Id | Scenario | How this proposal handles it |
|---|---|---|
| S28 | The developer wants normal placement restoration, except that a missing saved monitor should fall back to the primary monitor | Set a non-empty window id, retain the default persistence Boolean, and load a detached value with `WindowPlacement.LoadForPersistPlacementId`. Loading does not end the initial-placement phase. If the saved non-empty name no longer matches, replace only `DisplayDeviceName` with the primary monitor's name. Supply `Placement` with `Default` reason and `Disabled` cascading; explicit placement overrides automatic loading while retaining automatic saves. App-owned storage is an alternative, not a requirement. WinUI still handles geometry, DPI, constraints, and state. See [the example](#prefer-the-primary-monitor-when-the-saved-monitor-is-missing). |
| S29 | The developer wants cascading but not automatic storage | Keep `PersistPlacementId`, explicitly set `Window.UseAutomaticPlacementPersistence = false`, and request `CascadeBehavior = Enabled` through `Show(options)` or hidden application. Without explicit placement, use a live peer or normal fallback. With explicit placement, retain that value and allow only position adjustment from a same-monitor peer. Neither case accesses automatic storage. |
| S30 | The developer wants persistence without cascading this window | Set a non-empty id and retain the default persistence Boolean before initial placement; use `CascadeBehavior = Disabled` through `Show(options)` or hidden application followed by a skipping `Show`. Initial source selection uses explicit placement or stored placement/fallback, not a peer. Later eligible saves still occur. Other windows may still use this shown window as a peer. |
| S31 | Different apps use the same id | Packaged apps use separate registered application namespaces, including within one package. Unpackaged apps use an empty application-identity component, so unrelated unpackaged apps with the same id can cascade together. They can avoid that by choosing app-specific ids. Unavailable packaged identity never falls back to the unpackaged group. |
| S32 | The app changes the id, Boolean, or options after initial placement | Do not move, reload, cascade again, or reopen the initial-placement phase. After first display, Boolean changes suspend/resume future saves only for enrolled windows; an initially empty id or explicit `false` in that display's snapshot prevents later enrollment. Future eligible saves use the current Boolean and id; clearing the id suspends saving and group membership, and restoring a non-empty id permits saves when the Boolean is `true`. Each operation retains its snapshots. Later placement options are ignored; `DoNotActivate` remains effective for that Show. Before first display, edits after hidden application affect the next initial operation. |
| S33 | A developer creates a new WinUI project | Generate easy mode with a stable id and the default persistence Boolean, then one `Show(new WindowShowOptions { Reason = WindowShowReason.Launch })` call. Leave cascading `Automatic`; do not add an `Activate()` call. Unpackaged templates use an app-specific id and do not promise automatic storage. |
| S34 | The app supplies explicit placement and also requests cascading | `Automatic` does not cascade explicit placement. `Enabled` can adjust only its normal position using a peer on the selected target monitor. Keep its ordinarily adjusted size, state, snap bounds, and monitor choice rather than copying the peer's entire placement. Restart never cascades. |
| S35 | A peer disappears, cannot be captured, or launches concurrently | Continue discovery after a failed candidate capture. With no peer, retain explicit placement if supplied; otherwise use permitted stored placement or fallback. Once native application of a selected placement begins, do not retry with another peer or stored value. Concurrent windows can choose the same offset; no non-overlap guarantee is made. |
| S36 | An existing app already calls `Activate()` and wants automatic placement | Set a stable, non-empty `PersistPlacementId` before the existing initial `Activate()` call, retaining the default persistence Boolean. This is id-only adoption. Activate shares automatic initial placement with default reason/cascade policy and retains activation semantics. An empty id or explicit `false` preserves legacy activation without automatic persistence. Show remains preferred for new display code and options-based policy, but migration to Show is not mandatory. See [Existing-app adoption](#existing-app-adoption). |
| S37 | Maximize on monitor B, enter full-screen, close, then reopen full-screen with B still available | **Open:** an opted-in window saves its last valid overlapped placement; the app remembers full-screen separately. With unchanged displays and saved-monitor policy, the desired first visible state is full-screen on B, with no intermediate windowed display or premature activation. Exiting full-screen should return to maximized on B, retaining normal restore bounds too. Configure both window properties, use `TryApplyInitialPlacement(options)` with null placement for automatic selection or an explicit customized value, select `FullScreenPresenter`, then `Show(new WindowShowOptions { SkipInitialPlacement = true })`. The skip is required here: without it, that first `Show` reruns the automatic pass over the prepared state. This clarifies API sequencing but does not solve the presenter's `ShowWindow(SW_RESTORE)` path for hidden minimized/maximized windows. Applying only normal bounds does not preserve the maximized return state. See [Why apply without showing?](#why-apply-without-showing). |

### Engine use does not settle every policy

The intended WinUI behavior deliberately differs from adopting an entire helper sample:
automatic persistence uses the application-scoped packaged store, `Default` does not request the
launch-monitor hint, and current target constraints govern fitting. Cascading uses id-based
discovery rather than native window-class matching, and explicit placement gets only a position
adjustment when cascading is requested. Live full-screen transitions are not added by this
proposal. These remain explicit design decisions even though PlacementEx is the planned engine.

The remaining native work is most visible in S03, S11, S23-S24, and S37: a request to restore state
must not violate effective visibility or activation restrictions. The
[safe-fallback review gates](#safe-fallback-and-review-gates) remain open. Retaining a scenario
here does not establish that either backend already implements it.

## Launch-monitor hint investigation

The `STARTUPINFO` preferred-monitor hint was not observed on any measured launch path. A
`GetStartupInfoW` probe recorded `hStdOutput=0x0` for a direct process launch, an `explorer.exe`
launch, and a `ShellExecute` launch. The only measured launch that populated the field was one with
redirected standard output, which sets `STARTF_USESTDHANDLES` and means the field is a file handle
rather than a monitor. WinUI ignores the field when that flag is set.

Two questions remain open, and neither is settled by the measurements above:

| Open question | Status |
|---|---|
| Whether a packaged (MSIX/AUMID) activation supplies the hint | Not measured. [Related bug 58693034](https://task.ms/58693034) is a possible lead, not a confirmed explanation. |
| Whether a Start menu, taskbar, jump list, or desktop shortcut launch supplies the hint | Not measured. These paths need an interactive session to exercise. |

Earlier notes described this as a packaged-app propagation problem. The measurements do not support
that scope, because unpackaged shell launches also supplied no hint.

The intended `Launch` policy is unchanged: use a valid hint when available; otherwise use normal
saved-monitor selection. Until a launch path is shown to supply a hint, apps launched on the
measured paths get saved-monitor selection.

## Launcher show-command scenarios (deferred)

_Scope decision (2026-09-22): Explicit launcher show-command handling is deferred for this
version, including `start /min` and `start /max`. The earlier sketch below is retained as
design history, not a requirement or an adoption gate for the adopted API shape. This decision
does not defer saved-state restoration during application restart or decide monitor-hint policy.
The sketch retains historical policy discussion, including activation overrides. Hidden-operation
examples use the current `TryApplyInitialPlacement` spelling for clarity; this does not adopt
the deferred launcher-state merging or its proposed outcomes._

### Earlier alternative: Launch opts into startup show requests

Earlier tentative direction (2026-09-21, now deferred): when a window is initially shown with `Reason = Launch`
and the process was launched with `start /min`, minimize that window while retaining its
restore target. Initial `Activate()` should not immediately undo that minimization. This
would broaden `Launch` from selecting monitor policy to opting into startup show requests,
without introducing a main-window designation.

If revisited, determine whether `/max` should follow the same rule, whether the request affects
every `Launch` window or only one, and how to handle prior consumption by a splash window or
other window without unexpected native replay. This direction was not adopted by the main
contract or the earlier sketch below; its instruction-ownership rules remain unresolved.

### Applicability in the earlier sketch

In these tables, an **applicable instruction** is a startup instruction that the native integration
has confirmed should affect this window (HWND). The integration must also know how to handle it
without Windows applying it unexpectedly later. The reviews did not establish how to do this.
The tables depend on solving that problem; a window's first `Show()` call is not enough to prove it.

### Proposed precedence

Initial-placement policy and launcher instructions have different purposes. Placement supplies
the previous geometry and state. A launcher can request that this opening start minimized or
maximized. An ordinary launch should not discard a remembered maximized or snapped state.

The proposal works out the complete operation before changing the native window:

1. Validate initial options and resolve the calling method's overrides.
2. Select explicit, live-peer, stored, or fallback placement under the main pipeline's rules,
   including the disable flag and cascade policy. Apply reason policy, cascade adjustment,
   saved-minimization normalization, and current-display adjustments.
3. Merge an applicable launcher instruction into the planned state. `Reason = Launch` does not
   grant ownership of it. Neither does being the first window created by WinUI.
4. Combine the effective app options with the launcher's visibility and activation restrictions.
   Either can suppress display or activation; neither can remove the other's restrictions.
5. Execute only operations that preserve those restrictions, including during intermediate
   native transitions and fallback.

A launcher instruction that suppresses activation does not change the reason-policy row.
WinUI first selects that row using the effective app options, then adds the launcher's restrictions.

An applicable maximize request selects `Maximized` while retaining the normal rectangle, not
the snap rectangle, as the later restore position. An applicable minimize request retains the
unminimized restore target: normal, maximized, or snapped. If the selected placement is already
minimized, its existing restore target is retained rather than treating minimized as a restore
target itself.

Resolve snap availability during preparation. If snap cannot be retained, record normal as the
restore target. A retained snapped target remains subject to current system capabilities when
the window is later restored. Re-enabling snapping does not recreate history already discarded.

The launcher instruction is separate from saved minimization. Initial `Activate()` normalizes
saved minimization and overrides the app's hidden/non-activating options, but an applicable
launcher minimize request can still make this first display minimized. The pipeline must not
immediately undo that request with a second restore operation. A later `Activate()` is an
explicit request to restore the minimized window and request activation. An activation request
does not itself guarantee a non-minimized foreground window.

### Proposed command categories

This is a placement-aware translation of startup input, not literal equivalence to every raw
[ShowWindow command][ShowWindow startup behavior]. In particular, normal-style startup input
does not reset a remembered maximized or snapped placement.

| Startup input | Planned placement state | Additional display restriction |
|---|---|---|
| No `STARTF_USESHOWWINDOW` | Keep the reason-policy result | None |
| `SW_SHOWNORMAL`/`SW_NORMAL` (1), `SW_SHOW` (5), or `SW_RESTORE` (9) | Keep the reason-policy result; ordinary launch in this sketch | None |
| `SW_SHOWMAXIMIZED`/`SW_MAXIMIZE` (3) | Maximized with the normal restore rectangle | None |
| `SW_SHOWMINIMIZED` (2) | Minimized with the selected restore target | Target activation is permitted if effective app options permit it |
| `SW_MINIMIZE` (6) or `SW_SHOWMINNOACTIVE` (7) | Minimized with the selected restore target | Do not request target activation |
| `SW_SHOWNOACTIVATE` (4) or `SW_SHOWNA` (8) | Keep the reason-policy result | Do not request target activation |
| `SW_HIDE` (0) | Keep the reason-policy result while hidden | No display or activation |

The state-preserving treatment of commands 1, 4, and 9 is an intentional placement policy, not
a description of their raw Win32 effects. The operation does not synthesize extra focus transfers
to reproduce those effects. In particular, effective `DoNotActivate` continues to prohibit focus
transfer throughout the operation.

Minimization does not imply non-activation: command 2 permits activation, unlike command 7.
`start /min` is scenario shorthand, not a substitute for testing the exact startup command.
The native [STARTUPINFO contract][STARTUPINFO show command] excludes `SW_SHOWDEFAULT` from
`wShowWindow`; it is not another ordinary startup value in this table. Values not covered here,
including `SW_FORCEMINIMIZE`, require an explicit handling decision before this sketch is promoted.

### Proposed launch outcomes

These rows assume an overlapped window, a startup instruction known to apply to it, and a backend
that can safely apply the planned state. Activation follows the command table and effective app
options. Safe fallback may not reproduce the saved state exactly, as described below.

| Selected placement | Applicable launcher instruction | Proposed initial result |
|---|---|---|
| `Normal` | Ordinary launch | Normal at the adjusted normal rectangle |
| `Maximized` | Ordinary launch | Maximized; retain the adjusted normal rectangle for restore |
| `Snapped` | Ordinary launch | Snapped if supported; otherwise normal |
| `MinimizedFromMaximized` | Ordinary launch with ordinary placement policy | Maximized after saved-minimization normalization |
| `Maximized` | Minimize | Minimized; restoring returns to maximized |
| `MinimizedFromMaximized` with ordinary placement policy | Minimize | Normalize to maximized, then minimize with maximized as the restore target |
| `Snapped` | Minimize | Minimized; retain snapped as the restore target where supported |
| `Normal` | Maximize, such as a shortcut configured to run maximized | Maximized; retain the adjusted normal rectangle for restore |
| Explicitly staged `Snapped` | Maximize | Maximized; restoring returns to the normal rectangle, not the old snap rectangle |
| `MinimizedFromMaximized` on non-activating application restart | Ordinary launch | Keep minimized and attempt the saved desktop |
| Same non-activating restart | Maximize | Maximize without activation and attempt the saved desktop |
| No usable placement | Minimize or maximize | Use fallback geometry; apply the launcher state only through a safe supported operation |
| Selected placement could not be applied | Minimize or maximize | Use safe fallback and valid current bounds; do not reload automatic placement or claim the failed state was applied |
| Any placement | Instruction already handled by a splash window or another window | WinUI does not replay it; use this window's selected placement policy |

Two precedence choices are deliberate proposals:

- Explicit placement wins over automatic loading, not over an applicable launcher instruction.
  The instruction can change the planned state without modifying the app's staged object.
- Restart policy preserves saved minimization unless an independently applicable explicit
  launcher instruction overrides it. A current maximize request wins over historical minimization,
  but does not cancel `DoNotActivate` or the restart desktop policy. Without that instruction, an
  all-minimized restart inventory remains minimized.

This keeps one state-precedence rule rather than introducing a restart-specific exception.
Assigning `ApplicationRestart` to several windows does not make a startup instruction applicable
to each of them. If revisited, the qualifications would need reconciliation with the current
conceptual pipeline, restart policy table, and display and hidden-application API pages.

### Hidden preparation and later reveal

These hidden results depend on first solving startup-instruction handling: WinUI must know the
instruction applies to this window and how to avoid an unexpected later application. The proposal
then resolves the launcher state during the hidden initial operation, rather than saving a command
to replay when showing the window later. In this sketch, "a skipping `Show()`" means
`Show(new WindowShowOptions { SkipInitialPlacement = true })`, which reveals the prepared state
without rerunning placement. The window stays hidden throughout, with no
`VisibilityChanged` or activation events caused by preparation. Successfully changing a hidden
window's state does not prove that Windows has finished handling the startup instruction.

| Initial operation and placement | Applicable launcher instruction | Proposed prepared state | Later call and result |
|---|---|---|---|
| `TryApplyInitialPlacement(options)`, saved maximized | Minimize | Hidden, minimized, restore target maximized | A skipping `Show()` reveals minimized without activation |
| Same preparation | Minimize | Hidden, minimized, restore target maximized | `Activate()` after that skipping `Show()` restores to maximized and requests activation |
| `TryApplyInitialPlacement(options)`, saved normal | Maximize | Hidden, maximized | A skipping `Show()` reveals maximized and requests activation |
| `TryApplyInitialPlacement(options)`, no usable placement, safe state operation available | Minimize | Hidden, minimized using fallback geometry | A skipping `Show()` reveals minimized without activation |
| Hidden operation falls back to geometry-only preparation | Minimize or maximize | Hidden at valid current geometry/state, not a claimed min/max result | Later calls use the actual prepared state without retrying placement |
| Hidden initial operation has handled the instruction, then another window is displayed | Any handled instruction | No deferred startup instruction | WinUI does not reapply it to the other window |

The no-replay proposal applies to WinUI's handling. Your app can separately request native startup
handling again with `ShowWindow(SW_SHOWDEFAULT)`; these APIs do not block or intercept every
later native operation your app makes.

Hidden preparation does not establish automatic-save eligibility. Actual display does: a first-run
window shown maximized using fallback geometry can save that placement on an accepted close. If the
app needs a later non-activating reveal of a non-minimized window, it uses
`AppWindow.Show(false)` because later display calls ignore initial options.

## Safe fallback and review gates

**TODO (open): Define and validate guarantees across supported Windows versions.**

Placement visibility and activation safeguards remain in scope despite the launcher-command
deferral. Gates below concerning explicit startup-command ownership or merging apply only if
that feature is revisited; compatibility with existing native behavior remains in scope.

We have not settled which maximized, minimized, and snapped restore combinations can be
guaranteed during hidden application or with `DoNotActivate`. Establish the native behavior on the supported
paths and decide which fallback results are acceptable before finalizing the contract.

For example, an app asks to restore a maximized window with `DoNotActivate`. If a backend cannot
maximize without activation, the proposed fallback is to show the window normally without
activation. Confirm whether that is acceptable, and document any limits before treating the
behavior as a guarantee.

The proposed fallback policy puts visibility and focus safety first, even if WinUI cannot
restore the saved state exactly. If a backend cannot safely set maximized, minimized, or snapped
state, it may keep valid current state and safely adjusted geometry instead. It must not show
or activate first and then hide, cloak, or return focus to compensate. Capture must report the
actual resulting state, not the desired special state when only geometry was prepared.

Using fallback does not mean the selected placement was successfully applied. The existing
best-effort placement and no-retry rules still apply. We also need to know whether Windows still
has a startup instruction to handle; taking a fallback path does not answer that.

The reviews identified these implementation gates:

- For future explicit command handling, establish whether the native startup instruction is still
  applicable and how its native handling is completed. `GetStartupInfo` provides creation-time
  input, not the target HWND or consumption status. Neither a per-window first-show flag nor a
  WinUI-only process latch accounts for arbitrary earlier native or `AppWindow` activity.
- Include native window creation in that proof. The current
  [creation sequence](../dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp#L2280-L2294) uses
  `WS_VISIBLE`, `CW_USEDEFAULT`, and `SW_HIDE`. The documented
  [CreateWindowEx behavior][CreateWindowEx startup behavior] can invoke native show processing
  there, before an initial request is submitted. Do not assume this sequence either consumes or
  preserves the startup instruction, or replace it without checking unconfigured-app compatibility.
- [WINDOW_ACTION][WINDOW_ACTION state model] can express state independently of visibility and
  activation, including minimized restore targets. Set the selected restore target explicitly,
  including normal, rather than accidentally inheriting the native window's previous state. However,
  [ApplyWindowAction][ApplyWindowAction requirements] has documented Limited Access Feature
  approval, thread-ownership, and per-monitor-DPI-awareness requirements. An exported symbol
  or a position-only capability probe does not establish authorization or all required behavior.
- The existing downlevel placement path can show or activate before rehiding and does not
  implement the strict safeguards merely by receiving `NoActivate`. Select a safe path before
  dispatch; cloaking or focus repair afterward does not satisfy the contract.
- If explicit command handling is added, carry startup-derived restrictions through the outer WinUI display, activation, and XAML
  focus paths, not just the placement helper. In particular, a no-activate startup variant must
  not be followed by an unconditional activation request.
- Test each command category in a fresh process, with and without stored data, with direct
  native/AppWindow display, an earlier splash window, two UI threads, and partial application failure.
  For hidden cases, reveal the prepared window and then display another window to detect replay.
  Record native visibility/focus changes and XAML events, not only the final screenshot.
- This state-merging sketch covers overlapped windows. It does not authorize changing an
  app-selected full-screen or compact-overlay presenter. Define any interaction with native
  launcher state for those presenters before promoting the sketch.

The reviews examined source and documentation, not runtime behavior. Explicit instruction
ownership and merging remain deferred. The current implementation must still preserve the
placement contract's visibility and activation safeguards and evaluate native startup
compatibility; deferral is not proof that the native interactions are harmless.

## Native engine and WinUI policy

The proposed implementation uses the checked-in
[PlacementEx implementation](../external/inc/PlacementEx/PlacementEx.h) through a thin internal
adapter. Its default monitor migration, DPI/work-area adjustment, keep-on-screen behavior, and
ordinary cascade operation are the single geometry source of truth. The public API does not expose
the native structure or depend on its numeric flag values.

| Concern | Engine behavior or additional WinUI responsibility |
|---|---|
| Monitor selection | Delegate saved-device-name and normal-rectangle fallback selection to `PlacementEx`; WinUI may override it only for a validated `Launch` monitor hint |
| Normal geometry | Delegate work-area/DPI adaptation and keep-on-screen fitting to `PlacementEx` |
| Snapped geometry | Delegate arrange-rectangle work-area adaptation to `PlacementEx`; retain only WinUI's saved snap/history metadata |
| Snap availability | Apply the documented snapping-availability policy before either backend |
| Sizing capability | Use current target configuration; do not persist the old source window's resizable bit |
| Public state | Translate the six public states into native show state and restore/snap flags |
| Minimized-from-snap normalization | Convert the saved restore-to-arranged state into arranged placement before ordinary adjustment |
| Hidden capture | Retain meaningful state while reading current overlapped geometry |
| Other presenters | Retain a coherent last overlapped placement rather than structurally detecting full-screen bounds |
| Coordinate context | Produce physical coordinates consistently; do not expose DPI-virtualized engine output as physical pixels |
| Safe public input | Validate complete snapshots and use checked conversion and geometry arithmetic |
| Persistence and eligibility | Own store identity, initial-operation ordering, close/session-end saving, and fallback |
| Cascade discovery | Match the user, packaged/unpackaged scope, application identity, and placement id; do not use a shared WinUI window class |
| Cascade adjustment | Call `PlacementEx::Cascade` for peer placement; seed a working normal rectangle for explicit position-only cascading, then preserve the explicit adjusted size, monitor, and state |
| Virtual desktops | Use optional identity safely; do not require a blocking shell query on a synchronous window-message path |

`AdjustForMainWindow` already normalizes minimized-from-maximized placement. WinUI supplies
minimized-from-snapped normalization explicitly. `Default` uses adjustment
without startup flags; `Launch` requests the monitor hint. WinUI does not replay native startup
show commands for each window. The helper's name does not assign a main-window role.

`FindClosestMonitor` selects by device name, then by `normalRect`; the adapter delegates that
selection rather than approximating it with center distance or another WinUI rule. A validated
`Launch` monitor hint is the one intentional selection override. The default adapter does not
enable `AllowPartiallyOffScreen`. It derives downlevel `AllowSizing` from the target at application
time rather than trusting a captured source configuration, and must re-enter the engine's fitting
path after any constraint-driven size change.
The native path instead requests fit-to-monitor. Exact oversized-window fitting can differ
between these paths; current app constraints and presenter selection remain authoritative.

Differences are not limited to oversized windows. Normal-position adjustment and frame calculations
can also differ between backends. A worked coordinate example for the header's adjustment path
must not become an exact-pixel guarantee for all paths. S06 retains the taskbar-movement scenario
without requiring every backend to produce the same offset.

PlacementEx has native `ApplyWindowAction` and downlevel paths. Their internal steps are not the
public contract, and downlevel `NoActivate` support is not equivalent to the native flag.
Integration must preserve hidden application's visibility restriction and the request's
activation restriction on both paths. If it cannot apply placement without violating them, it uses
fallback with the same effective display options. Cloaking alone is not evidence that focus or
activation was preserved.

The adapter must not duplicate the engine's monitor, DPI, fitting, or cascade arithmetic merely to
produce a backend-neutral request. It may validate WinUI-owned constraints and prepare a working
value, but the resulting geometry must be produced by `PlacementEx` or an engine-compatible
adapter seam. A test that passes only because a synthetic topology is consumed by a separate
center-distance or rectangle-scaling implementation is not coverage of the native placement path.

The snapshot contract also requires WinUI-owned state tracking; `GetPlacement` captures
restore-to-maximized and currently arranged state, but does not reconstruct
`RestoreToArranged` history for an already minimized window. A single native geometry query
also does not establish hidden state or pre-presenter placement.

Capture effective window geometry after application rather than exposing the native in/out value
as a post-apply snapshot. The native action path can leave saved input metadata unchanged, while
the downlevel path mutates the normal rectangle into workspace coordinates. Neither is a
substitute for a coherent physical-coordinate snapshot of the resulting window.

Source references for these defaults:

- [Capture and native validity](../external/inc/PlacementEx/PlacementEx.h#L429-L581)
- [Monitor selection and application](../external/inc/PlacementEx/PlacementEx.h#L583-L790)
- [Normal and snapped geometry adjustment](../external/inc/PlacementEx/PlacementEx.h#L1298-L1444)
- [Cascade adjustment](../external/inc/PlacementEx/PlacementEx.h#L1448-L1489)
- [Previous-window sample policy](../external/inc/PlacementEx/PlacementEx.h#L1074-L1112)
- [Launch and minimized-state adjustment](../external/inc/PlacementEx/PlacementEx.h#L1493-L1625)

`IsValid()` requires an intersection with the saved work area, not the current topology.
The public validity rule retains that distinction. Public-input handling must additionally
check arithmetic before invoking helpers whose native calculations assume valid desktop geometry.

## Cross-process cascade discovery

Use a private property on each participating top-level HWND as a discovery marker. The intended
group key contains a protocol version, current-user identity, packaged/unpackaged scope,
application identity, and the exact `PersistPlacementId`. For packaged processes, use the
registered application identity already required for storage. For genuinely unpackaged
processes, leave the application-identity component empty. Failure to query a packaged identity
must not be reclassified as an unpackaged process.

One possible private representation is:

```text
Property name:  Microsoft.UI.Xaml.PlacementGroup.v1.<digest>
Property value: 1
```

Use a stable digest of an unambiguous, length-prefixed encoding of the group key. Preserve the
id's ordinal, case-sensitive, non-normalized semantics before hashing. Do not include a process
id or package version, use a process-specific string hash, or put a process-local string/object
pointer in the property value. The marker is an implementation detail, not a public API.

Publish membership when the window is shown and initial setup is complete. Update the marker
when its id changes and remove it when the id is cleared or the HWND is destroyed, following
the [SetPropW] cleanup requirements. Hidden windows may retain the marker but are not eligible
peers. A window shown directly through `AppWindow` or native APIs can become a peer without
rerunning its skipped initial-placement operation.

Use [EnumWindows] and the eligibility rules to find peers rather than `FindWindow` by class name.
Revalidate identity, membership, and window lifetime before capture. A marker is a discovery hint,
not authentication, and a capture can fail or race with a move or close. Avoid synchronous
callbacks into another app's UI thread. A failed candidate must not prevent considering another
eligible peer. Do not activate, move, or change the selected peer.

The selected live snapshot supplies ordinary automatic placement. For explicitly supplied placement
with `Enabled`, instead resolve its target monitor and adjusted geometry first, use an eligible
peer on that monitor as the normal-position anchor, and apply the cascade offset with the
explicit adjusted size. Rebase the working geometry to a coherent target coordinate environment
before applying the offset. Do not run another DPI transform on an already adjusted rectangle
or expose a mixed old/new work-area snapshot.

Both paths must preserve their documented failure and visibility rules. A peer-derived placement
successfully applied while hidden counts as applied placement for save eligibility, but neither
discovery nor an offset calculation by itself counts as a save or successful application.

Unpackaged cross-app matches are an accepted consequence of the empty application identity.
No executable-path fallback or separate public app-identity/cascade-group property is proposed.
This does not change the packaged-only scope of automatic storage.

## Adopted window opt-in and request-based placement design

_Design decision (2026-09-23, revised): Window-level persistence opt-in with request-based
placement policy is now the primary proposal, not a
competing alternative. Adoption of this shape is not a claim of implementation or backend
validation. The [API pages](#api-pages) and [IDL](#api-details) define the current surface._

_Design decision (2026-09-24): Hidden placement application and automatic startup placement are
now separate. `TryApplyInitialPlacement` is repeatable and no longer ends the one-time
placement pass; the initial-placement phase ends only at first display.
`WindowShowOptions.SkipInitialPlacement` (default `false`) lets a first `Show` display the window
at its current placement. Automatic-save enrollment moves entirely to first display, so hidden
application no longer establishes save eligibility. See
[Why an explicit skip option rather than a Showing event?](#why-an-explicit-skip-option-rather-than-a-showing-event)._

The earlier proposal coupled automatic persistence to assigning an id and extended `Activate`.
It retained initial options on the window and staged placement for a later hidden-show operation.
The adopted design puts the positive `UseAutomaticPlacementPersistence` property on
`Window`, defaults it to `true`, and uses a non-empty id as the setup-time opt-in. Explicit `false`
permits grouping without automatic storage. Placement policy is passed to `Show` or `TryApplyInitialPlacement`. An intermediate
draft put the Boolean on `WindowShowOptions` and required existing apps to migrate from
`Activate`; that decision is superseded. Initial `Show` and opted-in `Activate` now share
automatic placement. Activate keeps default policy and its activation semantics; it reads no
retained options. Hidden application applies now, is repeatable, and does not end the
initial-placement phase; display remains a separate call, and
`WindowShowOptions.SkipInitialPlacement` lets that first display preserve the prepared placement.
`DoNotActivate` replaces the activation enum.
No public copying method or copy constructor is provided. Capture and loading return detached
values, and initial placement application retains its coherent snapshot guarantee.

There is no general-purpose placement setter for already-displayed windows and no public
monitor-adjustment, cascade, serialization, or save operation. Expert customization can use
`LoadForPersistPlacementId` without taking ownership of storage.

### Existing-app adoption

S36 permits adoption without replacing the existing initial `Activate()` call. Set the id first:

```csharp
window.PersistPlacementId = "MainWindow";
window.Activate();
```

Setting a non-empty id is the opt-in when the Boolean retains its default of `true`.
initial `Activate()` selects automatic placement with `Default` reason and `Automatic` cascading,
then requests activation. `Show` remains preferred for new code and configurable display policy;
it is not mandatory for basic persistence adoption. The example assumes usable packaged storage
and no competing app-owned placement mechanism.

### Why apply without showing?

The strongest scenario is restoring overlapped placement before selecting another presenter.
The intended sequence is to apply while hidden, switch to full-screen, then display without
flashing the windowed state, while retaining the windowed placement for the return transition.
S37 requires retaining a maximized return state as well as the selected monitor and normal
restore bounds. The reviewed `FullScreenPresenter` implementation calls `ShowWindow(SW_RESTORE)`
when attaching to a minimized or maximized window, which can show and activate it before
full-screen sizing. Hidden placement application alone therefore does not establish a
hidden, non-activating presenter transition; this integration remains open.
The method name includes `Initial` because this is not a lifetime repositioning API. It applies
only while the window is undisplayed, during the initial-placement phase.
Keep `TryApplyInitialPlacement` for now and review that naming concern explicitly.

Non-activating restart ordinarily needs only `Show(options)` with `ApplicationRestart`,
not a separate Boolean or hidden step. Waiting for data to load also does not by itself
require early placement application. Inspecting restored geometry before arranging content
or related windows is another possible use, but needs a concrete scenario.
Hidden min/max/snap application and desktop restoration remain backend review gates, not
proven capabilities of this shape.

These placement scenarios, not general notification or tool-window presentation, motivate
the display operations. Existing `AppWindow.Show(false)` can also reveal after successful
hidden application, but does not itself run the initial-placement pipeline.

### Customize framework-stored placement without owning storage

`LoadForPersistPlacementId` reads only the framework slot into a detached editable value,
without choosing a peer, applying policy, or ending the initial-placement phase. The app supplies
that value in the initial request and configures a non-empty window id with the default Boolean if it wants
subsequent saves. Non-null placement overrides automatic loading, not automatic saving. The
[primary-monitor example](#prefer-the-primary-monitor-when-the-saved-monitor-is-missing)
demonstrates this without app-owned storage.

### Remaining contract and backend review

Two items from the previous revision are now decided and are no longer review gates:
`Activate()` gets no skip affordance, and a never-displayed window never saves. The
`SkipInitialPlacement` name and its `E_INVALIDARG` rejections are retained. See
[Why Activate() gets no skip affordance](#why-activate-gets-no-skip-affordance) and
[Why a never-displayed window never saves](#why-a-never-displayed-window-never-saves).
Late configuration and detached-loader errors are also decided (2026-09-24): enrollment is fixed
at first display, the current Boolean gates future saves only for enrolled windows, and
unexpected detached-load failures preserve their HRESULT. See
[the configuration rationale](#why-keep-enrollment-fixed-but-allow-saving-to-be-suspended) and
[the loader rationale](#why-propagate-unexpected-detached-load-errors).
The pre-display abort/retry boundary is decided as well (2026-09-24): a surviving, undisplayed
window keeps its phase open and retries with fresh snapshots, and a never-displayed window
remains ineligible to save. See
[Aborting before display](#aborting-before-display) and
[Why a pre-display abort can retry](#why-a-pre-display-abort-can-retry).
The items below remain open.

- Keep restore/save coupling as a separate future discussion. This revision does not adopt
  a persistence-mode enum, save-only switch, separate group id, or additional overload.
- Review configuration intent versus direct `AppWindow`/native first display. The current
  boundary still bypasses enrollment even with the Boolean `true`; no new interception is promised.
- Can hidden application avoid display and activation for normal, maximized, minimized, and
  snapped placement on every supported backend? Do not show and then hide to simulate it.
- Validate the proposed no-retry policy for failed hidden attempts and the exact boundary
  between successful adjusted placement and partial-application fallback. A failed operation
  is not atomic and must not be described as rolled back.
- Confirm the safe fallback state for each backend and the meaning of a successful hidden
  min/max/snap result. The Boolean and void Show fallback rules in the API pages describe
  intended behavior, not proof that the engine supplies it.
- Verify S37's hidden presenter transition and maximized return state, not only the preceding
  placement call. Hidden application clarifies sequencing but does not fix presenter behavior.
- Explicit `start /min` and `start /max` handling is deferred, not a prerequisite for adopting
  this shape. Retain native startup compatibility and placement visibility/activation safeguards.
- Monitor-policy simplification remains under discussion, not adopted. Keep the existing
  Default/Launch/ApplicationRestart monitor and state policy, peer-before-saved precedence,
  and maximized cascade policy until separately decided.

## API-shape rationale

`PersistPlacementId` supplies an app-defined slot and group identity.
`Window.UseAutomaticPlacementPersistence` defaults to `true` and permits automatic loading and
eligible saving when a non-empty id is configured before the initial operation. Explicit `false`
disables automatic storage without discarding the group identity.
The Boolean cannot replace the id: the framework does not invent identities from creation
order, a main-window designation, or a XAML class name.

### Why use documentation terms rather than an EasyMode or ExpertMode API?

Apps can adopt one capability at a time. An app might use automatic saving with explicit initial
placement, or use cascade discovery with its own store. Two mutually exclusive API modes would
hide those combinations. The documentation introduces easy mode first, then explains expert-mode
customization using the same properties and methods.

### Why put the opt-in on Window?

Persistence is window configuration shared by initial `Show`, `Activate`, and hidden application.
This lets existing apps retain `Activate` after setting only the id. The Boolean defaults to
`true` rather than being changed by the id setter, so assignment order cannot override an explicit
opt-out. An empty id disables automatic persistence without error and preserves legacy activation.
Neither setter starts I/O or applies placement. The initial operation snapshots the Boolean and id
before callbacks and records enrollment when both permit persistence. No `WindowShowOptions`
property is retained.

Later display is not a second placement operation. Ignoring later placement-specific fields
prevents placement replay. The current Boolean and id gate each future save without changing
first-display enrollment. Clearing the id suspends saves and group membership without deleting
data; changing it alone cannot override explicit `false`.

### Why keep enrollment fixed but allow saving to be suspended?

_PM decision (2026-09-24): Accept late setter changes, but do not create late enrollment._

| Alternative | Tradeoff |
|---|---|
| Reject changes after first display, or silently freeze the value | Prevents a close handler from opting out of a save; silent freezing also makes assignment misleading |
| Recompute enrollment from the current Boolean/id | Supports late save-only adoption, but can overwrite an existing slot with a window that never opted in at first display; it also changes the direct-native-display boundary |
| Keep enrollment fixed and check the current Boolean/id at each save | Preserves setup-time opt-in and native-display behavior while allowing an enrolled window to suspend/resume saving |

Choose the third option. Initial `false` or an empty id means no enrollment, even if later
assignments make both properties look opted in. A non-empty first-display id is not a permanent
write destination: an enrolled window still saves under its current id. Missing storage at
first display does not remove enrollment or preclude a later eligible save.

This is not a new save-only mode or a way to replay restoration. Callers that need automatic
saving without initial restore can opt in before first display and use explicit placement or
`SkipInitialPlacement`. Snapshotting the Boolean together with the id also lets accepted-close
handlers suppress a save without allowing callbacks during capture to redirect or cancel it.
Property changes between hidden attempts and first display remain ordinary setup changes.

### Why propagate unexpected detached-load errors?

_PM decision (2026-09-24): Preserve the original HRESULT outside the specified null cases._

Returning null for every error would make a missing record indistinguishable from allocation,
deployment, or programming failures. Replacing all unexpected errors with `E_FAIL` would retain
an error signal but discard useful diagnosis and the language projection's specific exception.
Preserving the original HRESULT lets the app distinguish these failures without a new public
result type. The null cases still support normal first-run and unavailable-store behavior.

Automatic restore and explicit load share classification, not error handling. Automatic restore
continues to its documented fallback and reports diagnostics; an explicit load reports the error
to its caller. Neither path repairs, deletes, or creates storage while reading. Private identity,
backend, and reader results must therefore carry the original error code through to the caller.

### Why reuse PersistPlacementId for cascading?

Repeated instances of the same remembered window role already share an id. Reusing it keeps
the identity setup to one assignment. An expert-mode app can retain that identity without
opting into storage and request cascading independently through `CascadeBehavior.Enabled`.

This version deliberately couples storage identity and cascade-group identity. It does not let
different persistence ids join a separate shared cascade group. Apps that need app-specific
isolation in the unpackaged namespace include that distinction in their id.

### Why preserve explicit placement when cascading is Enabled?

Explicit placement may encode an app-selected monitor, size, or state. Requesting cascading
should not silently discard that customization. `Enabled` permits normal-position adjustment
on the chosen target monitor, while `Automatic` preserves explicit-placement precedence.
Without explicit placement, using the live peer's complete placement follows the PlacementEx
previous-window workflow and reflects where the user has moved the running instance.

### Why use an editable detached value?

The optional value is mutable because editing a captured placement is a common operation.
A six-state enum avoids invalid combinations of native maximize, minimize, and arrange flags.
Current resizability is live window configuration, not a durable property copied from another
window. No native off-screen opt-out is exposed in this version.

Capture and loading return detached objects that the app can edit directly. Applying placement
takes a coherent snapshot, so later edits do not affect that operation. No public copying method
is needed for these scenarios. A [C++/WinRT copy constructor][Copy-construction guidance] copies
a reference rather than the underlying object's contents.

`Placement = null` in an initial request selects the permitted peer/stored/fallback chain
instead of explicit data. It does not disable automatic restore. A non-null value replaces
automatic loading while retaining opted-in saving. There is no staged value to clear.
Hidden application returns success for application, not acceptance of deferred work or durable
saving. It is repeatable and does not end the initial-placement phase, so the first display still
decides final placement unless it sets `SkipInitialPlacement`.

### Why prefer Show while retaining Activate adoption?

`Show` is the preferred display entry point for the new placement contract. It carries the
placement policy and per-display activation option, and can reveal a prepared minimized window
without restoring it. Non-activating restart and hidden preparation before presenter changes
motivate this distinction, rather than general focus-management features.

`Activate` keeps its existing reveal, restore-from-minimization, and activation meaning.
When initially opted in through the window property, it also selects automatic placement with
default reason/cascade policy. It does not read a retained options object or become a
non-activating restart display. Existing apps can retain the call; use Show options or hidden
application before Activate for custom placement policy.

### Why are placement options initial-only but DoNotActivate per-Show?

Source, geometry, reason, and cascading are decided once. Persistence enrollment is snapshotted
at first display; late Boolean changes gate only future eligible saves. Later display must not move a window
that the user or app has changed or replay a cascade. `DoNotActivate` instead describes the current display request, including
a reveal after hidden application. Apps do not need to clear retained initial options because
the window retains no such property.

Hidden application is a distinct operation rather than a Show request that suppresses display.
Its name states that it applies now and only during the initial-placement phase. A first `Show`
with `SkipInitialPlacement = true` reveals the result without rerunning placement; a first `Show`
or `Activate` without skip applies the automatic pass instead.

### Why an explicit skip option rather than a Showing event?

Preserving prepared placement needs a way to tell the first display not to place the window. One
alternative is a cancelable `Showing` event that the app handles to suppress the automatic pass.
We prefer the explicit option for these reasons:

- The caller already owns the sequence. The code that calls `TryApplyInitialPlacement` is usually
  the code that calls `Show`, so it can pass the option directly.
- An event adds a new contract: ordering against XAML content loading, reentrancy during the
  event, what cancellation means for `Show` callers who did not subscribe, and what happens when
  the first display comes from `AppWindow` or a native call.
- An option is statically checkable. Combining it with an explicit `Placement`, or passing it to
  `TryApplyInitialPlacement`, fails with `E_INVALIDARG` at the call site.
- `Activate()` takes no options, so an event would be the only way to skip from `Activate`. We
  would rather document that `Activate()` cannot skip than add an event to enable it.

The cost is that hidden preparation followed by a plain `Show()` or `Activate()` replaces the
prepared placement. `Show` can express the skip; `Activate()` cannot. The next section resolves
that asymmetry.

### Why Activate() gets no skip affordance

`Activate()` takes no parameter and reads no retained options, so an opted-in first `Activate()`
always runs the automatic placement pass. If the app already prepared the window with
`TryApplyInitialPlacement`, that pass replaces the prepared placement. We considered these ways
to close the gap.

| Option | Assessment |
|---|---|
| Keep `Activate()` optionless and document the `Show`-then-`Activate` sequence | Adopted |
| Add an `Activate(WindowShowOptions)` overload | Rejected. It duplicates `Show(options)` with different activation semantics and grows the API surface for one recovery case. `Show(options)` already expresses every field such an overload would carry. |
| Skip the automatic pass implicitly after a successful hidden application | Rejected. It makes the first-display contract depend on the hidden-application success boundary, which is still a backend review gate. A partial application would decide whether the first display places the window. |
| Skip the automatic pass implicitly after any `TryApplyInitialPlacement` call | Rejected. It removes the retry path, and a WinRT Boolean cannot distinguish an unset `SkipInitialPlacement` from an explicit `false`, so the caller could not ask for the pass again. |
| End the initial-placement phase on hidden application | Rejected. It contradicts the repeatable-until-displayed contract and makes one failed hidden attempt permanently forfeit automatic placement. |
| Add a retained `Window` skip property or a cancelable `Showing` event | Rejected earlier. See the section above and [Why are placement options initial-only but DoNotActivate per-Show?](#why-are-placement-options-initial-only-but-donotactivate-per-show) |

The deciding argument is asymmetric cost. Every implicit-skip variant removes the documented
retry path while hidden application is still unproven for maximized, minimized, and snapped
placement. A caller that loses the automatic pass has no way to ask for it back. A caller that
calls `Activate()` in the wrong order loses only the prepared placement and still gets a placed,
displayed window.

The exposure is also narrow. The gap needs a window that is opted in, is prepared with
`TryApplyInitialPlacement`, and then uses `Activate()` rather than `Show` for its first display.
An opted-out window's `Activate()` never ran a placement pass, and an app that never prepares a
hidden window has nothing to lose.

To keep the gap from being silent during development, the implementation traces a debug
diagnostic when an opted-in first `Activate()` follows at least one `TryApplyInitialPlacement`
call on the same window. This is development output only. It is not an event, not a failure, and
not part of the behavioral contract, and apps must not depend on it.

### Why a pre-display abort can retry

A first display reserves the operation before its callbacks run, so no callback can start a
second placement pass. Earlier revisions described that reservation as the end of the phase,
while the lifecycle tables ended the phase only on actual display. The two descriptions differ
when a validated attempt aborts before the window is visible and the window survives.

| Option | Assessment |
|---|---|
| The phase stays open; the next call runs a fresh pass | Adopted |
| The reservation permanently consumes the opportunity | Rejected. The window is still hidden, so nothing the user can see happened, yet the app would lose automatic placement for the life of the window with no way to ask for it back. |
| The reservation consumes the opportunity, but record enrollment anyway | Rejected. It creates a window that is enrolled and never displayed. That state exists only to be filtered out at save time, and any bug in that filter writes placement the user never saw. |
| Retry the pass, but lock enrollment to the aborted attempt's snapshot | Rejected. It splits one contract into two rules with different snapshot times for no benefit. The retry already reads the same properties. |
| Expose the phase state so apps can decide | Rejected. It adds API surface for a failure the app can handle by calling `Show` again. |

Three properties decide this. First, the phase is defined by display, not by intent, everywhere
else in this document: validation failures and hidden application both leave it open. Second,
enrollment is recorded at first display, so with no display there is nothing to record and no
stale snapshot to carry. Third, saving already requires an actual display, so a retry cannot
make a never-displayed window save under any of these options.

The repeat cost is bounded. Each attempt is app-driven and does the same work as a first `Show`,
which is the same position hidden application is already in: repeatable while the window stays
hidden. An app that loops on a failing `Show` has a larger problem than placement.

### Why a never-displayed window never saves

Automatic saving requires enrollment at first display and an actual display, so a window that is
prepared with `TryApplyInitialPlacement` and never displayed never saves. This removes a
capability an earlier revision allowed. We audited the scenarios that could have depended on it.

| Scenario | Needs saving a never-displayed window? | Result |
|---|---|---|
| Prepare hidden, reveal with a skipping `Show`, then close | No. First display enrolls and establishes eligibility. | Saves |
| Prepare hidden, move the still-hidden window, reveal, then close | No. Hidden position and size changes are captured at save time. | Saves |
| Display a window, hide it, then close it while hidden | No. Enrollment came from the first display, and hiding does not suspend saving. | Saves |
| Prepare hidden, then close or sign out without ever displaying | Yes | No save |
| Preallocate hidden windows and track their layout | Yes | No save. Use `TryGetPlacement` with app-owned storage. |

Only the last two rows lose behavior, and in both of them saving is undesirable:

- Hidden preparation usually applies the value that was just loaded, so saving it back is a no-op
  at best. When that value was adjusted for the current monitor, work area, or DPI, saving it back
  writes the adjusted rectangle over the original. An app that repeatedly prepares windows it
  never displays would ratchet its saved placement toward the adjusted result.
- When hidden application returns `false`, the window holds fallback geometry that the user never
  saw and never chose. Saving that would overwrite real saved placement with a default.

The removal therefore protects stored data rather than costing a scenario. An app that must
persist geometry for windows it never displays can capture with `TryGetPlacement` and write to
its own storage, as in [Supply placement from app-owned data](#supply-placement-from-app-owned-data).

### Why use application identity rather than package family alone?

The default packaged app-data store is package-family scoped. A [package family][Package identity]
is stable across package versions and architectures, but a package can contain multiple
applications. Package-family scope alone would make two such applications using `"MainWindow"`
overwrite the same placement slot.

WinUI adds an application-specific namespace so each app can choose its placement ids
independently. Multiple processes of the same app still share slots. This does not require a
new public setting or make the app responsible for prefixing ids with deployment information.

The implementation should resolve the registered packaged application identity using
[GetCurrentApplicationUserModelId][Current application identity] and scope its private storage
accordingly, for example with an application-specific container above the existing placement-id
keys. Use the registered identity, not a display name, process id, window title, or versioned
package full name.

If identity cannot be resolved, automatic persistence is unavailable for that operation.
Falling back to package-family-only storage would reintroduce the collisions this rule prevents.
Likewise, a missing application-scoped value must not fall back to an older package-wide slot
whose owning application cannot be established.

### Why is there no API to delete saved placement?

Automatic placement uses the packaged app's local app-data store. Windows removes that store
when the app is uninstalled, so apps do not need a placement-specific API for uninstall cleanup.
App updates preserve the store.

Deleting selected entries while the app remains installed is a separate scenario, such as
resetting one window's saved layout or removing an unused document id. We are leaving that API
out of this version. We can revisit it if app scenarios show a need for a dedicated deletion API.

Clearing `PersistPlacementId` suspends future saving and group membership without erasing
saved data. For an initially opted-in window, assigning a non-empty id again permits future
eligible saves. Explicit Boolean opt-out preserves grouping without automatic storage.

## Compatibility and non-goals

The instance methods can take precedence over existing `Show(this Window)` or `Hide(this Window)`
extension methods when an app is recompiled. Derived app classes may also already declare those
names. This source-compatibility cost requires API review.

`method_name("ShowDefault")`, `method_name("ShowWithOptions")`, and
`method_name("HideDefault")` distinguish ABI/implementation names. They do not rename projected
calls: C++/WinRT and C# apps call `Show()`, `Show(options)`, and `Hide()`.

This version does not provide:

- explicit launcher show-command ownership, merging, or replay policy, including for `start /min`
  and `start /max`;
- automatic persistence without package identity or a usable application identity;
- an app-supplied persistence store, public serialization format, or transactional migration;
- per-id deletion of automatically stored placement;
- continuous saves or crash-time recovery of the latest move;
- per-topology placement histories;
- a separate cascade-group id, cross-process position reservations, or guaranteed non-overlap;
- full-screen or compact-overlay presenter restoration;
- snap-group reconstruction or coordination with other apps' windows;
- a guarantee of exact saved pixels despite topology or constraint changes.

Experimental AppWindow placement APIs are related, but are not a dependency of this contract.
A future implementation could use them after establishing the required state, policy, and
compatibility mappings. This proposal does not assert complete field or behavior equivalence
with an unversioned experimental surface.

## Implementation acceptance criteria

The implementation must demonstrate the published behavior on native and downlevel paths.
Use [Placement scenarios and WinUI decisions](#placement-scenarios-and-winui-decisions) as the
concrete scenario inventory. For each applicable restore case, record resulting normal and snap
bounds, state and restore target, visibility, activation, and desktop, and check that later capture
reports the actual result. Open rows remain review gates, not claims of supported behavior.
Deferred rows are future design context, not required launcher-merging outcomes for this version.
Cases outside initial restoration must not accidentally trigger another restore.

In addition, it must cover:

- removed and repositioned monitors, missing device names, DPI changes, and work-area changes;
- all cascade enum values with empty/non-empty ids, explicit/no explicit placement, and both
  Boolean values; accept an empty id without persistence or grouping and skip restart cascading even with `Enabled`;
- live-peer precedence, no-peer fallback, failed candidate capture, and no source retry after
  native application begins; no automatic storage access for an opted-out operation;
- sequential and concurrent same-group openings, offset wrapping, and maximized/snapped peers;
- position-only cascading of explicit placement without replacing its target monitor, adjusted
  size, state, or snap bounds; no same-monitor peer and topology-change cases;
- packaged app/user isolation, cross-process matching, the shared unpackaged namespace, and
  packaged identity-query failure without joining the unpackaged group;
- hidden, cloaked, non-overlapped, closing, and uninitialized peers; late id changes, native
  display, HWND reuse, property cleanup, and mixed-architecture discovery;
- request/window-Boolean/id/source snapshots before callbacks and during reentrancy;
  initial enrollment for setup-time configuration; ignored late options do not replay placement;
  late Boolean changes never reopen the initial-placement phase;
- enrolled windows suspend/resume saves on Boolean changes and id clear/reassign; non-enrolled
  windows remain ineligible after late assignments in either order, including initially empty ids,
  initial explicit `false`, and direct native first display;
- current Boolean/id snapshots for saves after close handlers, including handler opt-out,
  cancelled-close edits, and callback changes after the snapshot; no setter I/O, data deletion,
  or late restoration, and grouping independent of storage opt-in;
- configuration changes after hidden application affect first-display enrollment; unavailable
  storage at first display does not prevent an enrolled window from saving when storage recovers;
- the proposed C# and C++/WinRT template startup pattern, including stable ids, `Launch`,
  id-only persistence opt-in, default automatic cascading, and exactly one Show entry point;
- explicit primary-monitor preference with the saved monitor present or missing, an empty saved
  name, and display changes after app enumeration; unchanged source geometry metadata and no
  launch-monitor override under `Default`;
- completely off-screen, partly off-screen, oversized, fixed-size, and constrained windows;
- normal, maximized, snapped, and all three minimized restore states;
- snapping disabled, missing virtual desktops, and unavailable desktop identity;
- hidden adjustments, successful and failed hidden placement, and absent overlapped history;
- initial `Show(options)` and hidden application with every reason/DoNotActivate row;
  hidden application never activates; restart preserves saved minimization, attempts desktop
  restoration, and suppresses activation and cascading with either Boolean value, including
  missing-data and missing-desktop fallback; do not mutate the options object;
- repeated hidden application while the window stays hidden: each call applies and none ends the
  initial-placement phase; hidden application never enrolls the window;
- hidden application followed by a first `Show` with `SkipInitialPlacement = true`: the prepared
  placement is preserved, with no loading, peer selection, cascading, or fallback repositioning;
  the phase ends, so a later `Show` or `Activate` never places the window;
- hidden application followed by a first `Show` without skip, or by a first `Activate()`: the
  automatic pass runs and can replace the prepared placement; no implicit skip is applied and the
  optional debug diagnostic changes no observable behavior;
- a prepared but never-displayed window performs no save on close, native destruction, or
  confirmed session end, whether hidden application succeeded or failed; a window that was
  displayed and then hidden still saves on an accepted close;
- a validated first display that aborts before any display and leaves the window alive: the
  phase stays open, the next `Show` or `Activate` runs a complete initial pass with fresh
  request/Boolean/id snapshots and fresh source selection, the aborted attempt records no
  enrollment, and the window remains ineligible to save until an actual display;
- `SkipInitialPlacement = true` with a non-null `Placement`, and on a `TryApplyInitialPlacement`
  request: both fail with `E_INVALIDARG` before the window is displayed or changed; undefined
  `Reason`/`CascadeBehavior` values still fail under skip;
- `SkipInitialPlacement` ignored on a `Show` after the phase has ended, including invalid
  combinations; skip does not change enrollment, save eligibility, or automatic saving;
- skip with `DoNotActivate` true and false, and with `ApplicationRestart`, which retains only its
  activation restriction under skip;
- a pending `Width`/`Height` still applies under skip;
- validated hidden application, successful or failed and explicit or automatic, followed by
  a skipping Show, non-activating Show, or Activate: a skipping Show preserves minimization and
  does not reload;
- id-only setup enables automatic load/save under the default Boolean; existing apps set the id and retain
  first Activate, which uses default policy and requests activation; opted-out Activate retains
  the legacy path and ends the initial-placement phase on first display without enrollment;
- opted-in initial `Show()`, `Show(new WindowShowOptions())`, and initial Activate run automatic placement;
  Activate has no retained options and is not non-activating restart;
- validation before the window is displayed or changed, including invalid placement,
  undefined reason/cascade values, and skipped cascade policy during restart or with an empty id;
  default and explicit true-plus-empty-id must preserve ordinary display without persistence;
  explicit false must suppress persistence in either property assignment order, without setter I/O or placement;
- null options rejected by both public options methods with `E_INVALIDARG`, including late and
  reentrant calls, without display, placement, storage, or enrollment side effects; a rejected
  initial call leaves the phase open for a subsequent valid request;
- default hidden requests use a non-null empty options object, select peer/stored/fallback under
  the normal policy, and remain hidden; null `Placement` is valid and distinct from null options;
- ignored late placement fields, false late/reentrant hidden requests with non-null options,
  failed validated attempts, partial native changes without claimed rollback, and later per-Show activation;
- direct AppWindow/native first display bypasses the pipeline and does not establish enrollment
  even with the Boolean true, whether or not hidden application already ran;
  reentrant display and close, invalid options, and calls after the phase has ended;
- snapshot independence, coherent concurrent reads, invalid intermediate edits, and C#/C++/WinRT projection;
- accepted and cancelled close, native destruction, confirmed and cancelled session end;
- corrupt or unavailable storage, explicit precedence, no automatic retry, and no-data fallback;
- detached framework-slot loading, null/error cases, no peer selection, adjustment, application,
  phase change, or saving enrollment during load; null Placement permits automatic selection;
  explicit customized placement plus hidden apply retains opted-in saves and is preserved by a
  skipping Show before presenter selection and display, subject to the open S37 transition;
- detached-load failures preserve injected HRESULTs through identity/backend/reader/public
  boundaries in C# and C++/WinRT; expected unavailability and invalid data return null, allocation
  and unexpected deployment/lifetime errors do not; automatic restore uses fallback for the same
  failures; failure diagnostics omit ids, names, placement data, and backend error text;
- independent slots for two applications in one package using the same placement id, and shared
  slots across processes of the same application;
- stable namespaces across package version/architecture updates, and unavailable application
  identity or missing scoped values without package-wide fallback;
- id changes without deletion, and packaged app-data lifetime across app updates and uninstall;
- compatibility with existing native startup behavior, including a preceding splash window or
  other HWND, without treating deferred launcher-merging outcomes as new guarantees;
- unchanged runtime behavior for apps that do not use the feature.

[SetPropW]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setpropw
[EnumWindows]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-enumwindows
[EnumDisplayMonitors]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-enumdisplaymonitors
[GetMonitorInfoW]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getmonitorinfow
[MONITORINFO primary flag]: https://learn.microsoft.com/windows/win32/api/winuser/ns-winuser-monitorinfo
[RegisterApplicationRestart]: https://learn.microsoft.com/windows/win32/api/winbase/nf-winbase-registerapplicationrestart
[Copy-construction guidance]: https://learn.microsoft.com/windows/apps/develop/cpp-winrt/consume-apis#dont-copy-construct-by-mistake
[App data lifetime]: https://learn.microsoft.com/windows/apps/develop/data/store-and-retrieve-app-data
[ShowWindow startup behavior]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-showwindow
[STARTUPINFO show command]: https://learn.microsoft.com/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfow
[CreateWindowEx startup behavior]: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-createwindowexw
[WINDOW_ACTION state model]: https://learn.microsoft.com/windows/win32/winmsg/winuser/ns-winuser-windowaction
[ApplyWindowAction requirements]: https://learn.microsoft.com/windows/win32/winmsg/winuser/nf-winuser-applywindowaction
[Package identity]: https://learn.microsoft.com/windows/apps/desktop/modernize/package-identity-overview#package-family-name
[Current application identity]: https://learn.microsoft.com/windows/win32/api/appmodel/nf-appmodel-getcurrentapplicationusermodelid
