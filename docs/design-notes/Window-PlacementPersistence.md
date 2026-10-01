# Window placement persistence feature specification

> **Proposal, not a shipping-status claim.** This document specifies intended feature behavior
> and its implementation design. Passing helper tests does not establish feature completion.
> [Coverage and open gates](#coverage-and-open-gates) summarizes only evidence recorded in the
> original design notes.
>
> **AI-assisted document:** AI makes mistakes; technical claims require human review.

## Navigation and ownership

| Document | Authoritative content |
|---|---|
| [API proposal](../../specs/window-placement-persistence-spec.md) | Public declarations, field definitions and defaults, basic argument checks and threading, compatibility and scope, and primary workflows |
| This feature specification | Detailed behavior, scenarios S01-S37, implementation architecture, storage and codec, diagnostics, acceptance requirements, and evidence mapping |
| [Decisions and history](./Window-PlacementPersistence-decisions.md) | Rationale, alternatives, deferred proposals, dated investigations, reviews, implementation reports, and historical test results |

- [Feature behavior](#feature-behavior): [initial placement](#loading-applying-and-displaying),
  [display](#control-display-and-activation), [reason policy](#choose-launch-or-restart-policy),
  [cascading](#cascade-related-windows), [display adjustment](#restore-after-display-changes),
  [capture](#capture-change-and-supply-placement), [validity](#data-validity), and
  [saving](#know-when-placement-is-saved).
- [Stable scenario catalog](#placement-scenarios-and-winui-decisions) and
  [safe fallback](#safe-fallback-and-review-gates).
- [Extended workflows](#extended-workflows).
- [Implementation design](#implementation-design): coordinator, native integration, storage,
  capture caches, lifecycle, retention, codec, and diagnostics.
- [Acceptance requirements](#implementation-acceptance-criteria),
  [test infrastructure](#automated-testing), and [coverage](#coverage-and-open-gates).

The behavior sections are normative for this proposal unless explicitly marked Open or Deferred.
The implementation sections describe the design used to satisfy those requirements; their private
types, wire format, and source layout are not public guarantees. Historical reports do not override
either the public proposal or the behavior below.

## Feature behavior

Read the API proposal's [placement overview](../../specs/window-placement-persistence-spec.md#contract-at-a-glance),
[API pages](../../specs/window-placement-persistence-spec.md#api-pages), and
[threading and API-use requirements](../../specs/window-placement-persistence-spec.md#errors-and-threading)
before using this specification. The proposal owns signatures, property defaults, coordinate
definitions, required-argument precedence, and thread/host checks. This document does not
redeclare the public types.

### Window roles and placement needs

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
and compact-overlay describe presenters. The public placement-state enum combines normal,
maximized, or snapped placement with whether the window is minimized and its restore target.
It does not describe every aspect of window state. Shown, hidden, and cloaked describe visibility
conditions. Source, target, and cascade peer describe participation in a placement operation.
First-run, replacement, and reconstructed windows describe lifecycle scenarios rather than
distinct window types.

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


Use the [primary workflows](../../specs/window-placement-persistence-spec.md#primary-workflows)
for basic opt-in, explicit customization, and hidden preparation. The [extended workflows](#extended-workflows)
below cover identity choices, app-owned storage, and presenter preparation.

## Use the initial-placement phase

Configure the id and any persistence override in the constructor, XAML, or code before the
first placement/display operation. Assignment order does not matter. Do not wait for a content
element's `Loaded` event; initial placement may have already run.

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

Option setters do not validate enum values. An eligible initial request validates them even when
its reason or skip policy would suppress the corresponding action, including cascading during
restart. Validation failures follow the [API error contract](../../specs/window-placement-persistence-spec.md#errors-and-threading).

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
Before the operation, `Placement` retains the assigned reference rather than a copy of its fields.
The operation snapshots that value under the
[API threading contract](../../specs/window-placement-persistence-spec.md#errors-and-threading).

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

Once the window is actually displayed, fallback or a later failure does not reopen the phase.
The surviving, undisplayed case permits a retry but never makes that window eligible to save.

The [abort/retry decision](./Window-PlacementPersistence-decisions.md#why-a-pre-display-abort-can-retry)
records why reservation and actual display are separate. Apps retry with an ordinary `Show` or
`Activate` call; no public phase-query API is added.

### Initial-placement fallback

Actual first display ends the phase even if there is no eligible peer or usable saved placement,
the presenter does not support placement, or placement application fails. Passing validation
alone does not end the phase. Hidden application uses the same fallback rules but leaves it open.

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

Use the public proposal for [options and defaults](../../specs/window-placement-persistence-spec.md#defaults-and-value-model)
and [argument checks](../../specs/window-placement-persistence-spec.md#errors-and-threading).
`Show` displays; `TryApplyInitialPlacement` applies while hidden and never requests activation.
Neither retains an options property. The initial operation uses the reason policy below;
after first display, only the per-Show activation option remains applicable.

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

`Hide` leaves the HWND and XAML content alive. It does not raise `Closed`, write placement,
or reopen the phase. It can hide a window shown through `AppWindow`. The public/private
host boundary remains the one in the
[API compatibility section](../../specs/window-placement-persistence-spec.md#compatibility-and-scope).

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
`Show` does not retain the hidden request's `Reason`, so set `DoNotActivate` explicitly for a
non-activating reveal.

`Launch` prefers the monitor the shell selected for the launch over the saved monitor.
For example, launching from a taskbar on a different monitor can cause restoration on that
monitor. If no valid hint is available, normal saved-monitor selection applies.

The availability of the launch-monitor hint remains an integration question. See the
[dated investigation](./Window-PlacementPersistence-decisions.md#launch-monitor-hint-investigation);
do not infer hint delivery from selecting `Launch`.

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

A successful capture returns a valid independent value; a failed capture returns `false` and null.
The value does not keep the source window alive or update when it moves. Missing optional monitor
or desktop identity does not alone prevent capture. If a minimized restore target is unknown,
capture uses `Minimized`; it must not guess maximized or snapped history. API-use errors remain
errors rather than a `false` capture result.

### Data validity

The constructor validates the required geometry and DPI without inspecting the current monitor
layout or moving a window. You can temporarily leave properties
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

`SnapRect` is ignored for application in other states, but any supplied rectangle must still pass
the validity checks above. You can set `State` to `Normal` to remove both snapping and minimized
restore-state instructions without manipulating separate flags.

An app can also construct placement with any defined state. It specifies the desired state,
not history that WinUI must verify. Reason policy can still normalize minimization, and current
system capabilities can require snapped placement to fall back to normal. The
[public value model](../../specs/window-placement-persistence-spec.md#defaults-and-value-model)
defines the state values and property defaults.

Null `DisplayDeviceName` is treated as empty. `VirtualDesktopId` can be null; `Guid.Empty` is
treated as no identity when placement is accepted.

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
[Why a never-displayed window never saves](./Window-PlacementPersistence-decisions.md#why-a-never-displayed-window-never-saves).

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

### Placement cache lifetime

Stored placement follows the packaged app's local app-data lifetime: updates preserve the store,
and uninstall removes it. Apps need no placement-specific uninstall cleanup. See
[Store and retrieve settings and other app data][App data lifetime].

Automatic records are a bounded cache, not permanent per-document storage. The
[retention policy](#retaining-the-32-most-recently-saved-placements) owns the limit, successful-save
recency, and best-effort eviction rules. Loading does not refresh recency. Eviction affects future
loads, not live windows.

## Placement scenarios and WinUI decisions

These are the original S01-S37 identifiers and scenario meanings. They describe intended outcomes,
not passing tests. **Open** rows do not add guarantees. **Deferred** rows preserve future design
context, not acceptance requirements for launcher merging. Rule links point to the authoritative
behavior rather than repeating full policy tables here.

Unless stated otherwise, assume an overlapped window, usable saved placement, a non-empty id and
automatic persistence enabled before initial operation, no eligible peer, and no applicable native
launcher show command. Non-activating restart means an app-selected `ApplicationRestart` request;
WinUI neither detects restart nor reconstructs the app's window inventory.

### Saved placement, launch, and display changes

| Id | Scenario | Intended result and authoritative rule |
|---|---|---|
| S01 | Close on monitor A, remove A, then connect B at the same screen coordinates | A matching saved display name wins even if hardware changed. Otherwise choose B when it has the largest normal-rectangle intersection and adapt geometry, not exact pixels. [Monitor selection](#the-original-monitor-went-missing) |
| S02 | Close on A, then move A elsewhere in Display Settings | Follow a still-matching A rather than its old coordinates; otherwise use geometric selection. [Connected monitor](#the-original-monitor-is-still-connected) |
| S03 | Save on virtual desktop 3, then restart while desktop 1 is active | Attempt the saved GUID without desktop switching or activation; failure leaves the current desktop. Backend state safety remains open. [Restart policy](#choose-launch-or-restart-policy), [gates](#safe-fallback-and-review-gates) |
| S04 | Save on desktop 3, then launch normally while desktop 1 is active | Ignore saved desktop identity and remain on the new window's current desktop. [Reason policy](#choose-launch-or-restart-policy) |
| S05 | Launch from a taskbar button on a different monitor | `Launch` uses a valid hint for normal/snap adjustment; default opt-in does not select that policy. An already-running taskbar activation does not rerun placement. [Reason policy](#choose-launch-or-restart-policy) |
| S06 | Close with the taskbar at the bottom, move it to the top, and relaunch | Adapt work-area placement and normal restore bounds separately from maximization; no exact taskbar-height shift is promised across backends. [Work-area changes](#the-resolution-or-taskbar-work-area-changed) |
| S07 | Launch two instances of the same app with the same placement id | Prefer a captured live peer, then saved/fallback placement. Cascade does not reserve positions or prevent maximized/snapped/concurrent overlap; storage remains last-successful-save-wins. [Peer selection](#select-placement-from-a-live-peer), [storage scope](#application-identity-and-storage-scope) |
| S08 | Close at 100% display scale, then relaunch at 200% | Preserve approximate logical size before fitting and constraints; position follows separate adjustment. [DPI changes](#the-dpi-changed) |
| S09 | Save a large window on an external monitor, then relaunch with only a smaller laptop display | Adapt to the laptop; current resizing and minimum-size constraints can prevent full containment. [Missing monitor](#the-original-monitor-went-missing), [fitting](#the-resolution-or-taskbar-work-area-changed) |
| S10 | Minimize the app, close it, then launch normally | Use the saved normal/maximized/snapped restore state, with normal fallback if snap is unavailable. Native launcher minimize is separate (S15). [Saved minimization](#the-window-was-minimized) |
| S11 | Leave the app minimized, then restart Windows | Non-activating restart retains successfully saved minimization and restore target; initial `Activate` instead uses default normalization. After hidden preparation, reveal with a skipping `Show` before a later `Activate` restores the prepared state without reloading. [Restart](#choose-launch-or-restart-policy), [display](#control-display-and-activation), [gates](#safe-fallback-and-review-gates) |
| S12 | Save on desktop 3, delete that desktop, then restart | Attempt the saved GUID, not ordinal 3; failure keeps the current desktop without creating or substituting one. [Restart policy](#choose-launch-or-restart-policy) |
| S13 | Disconnect A, open and close on B, then reconnect A and relaunch | A successful save on B replaces A's record; there is no per-topology history. A valid `Launch` hint can still select A. [Missing monitor](#the-original-monitor-went-missing), [save scope](#application-identity-and-storage-scope) |
| S14 | Launch for the first time with no saved placement | Use current fallback and pending initial size, optionally adjusted by a valid `Launch` hint. Do not adopt a helper sample size. Visibility/activation restrictions still apply. [Fallback](#initial-placement-fallback) |
| S15 | Close maximized, then launch with an explicit minimize request | **Deferred:** no new `/min` versus saved-maximization precedence. Do not deliberately suppress existing native startup behavior. [Deferred launcher proposal](./Window-PlacementPersistence-decisions.md#launcher-show-command-scenarios-deferred) |
| S16 | Save snapped to the left half, then relaunch with a different work-area size | Adapt visible snap bounds and normal restore bounds separately; unsupported snap falls back to normal, without snap-group reconstruction. [Snap adjustment](#the-window-was-snapped) |
| S17 | Enter full-screen on A, move the full-screen window to B, then exit full-screen | Live presenter behavior is not another persistence restore or a new guarantee to return on B. Capture/save retain the last valid overlapped snapshot. [Presenters](#the-window-was-full-screen-or-compact-overlay) |

### Boundary cases that must remain covered

| Id | Scenario | Intended result and authoritative rule |
|---|---|---|
| S18 | The saved display name is missing and the normal rectangle intersects no current monitor | Choose the nearest monitor using the normal rectangle, not the primary monitor, work area, or snap rectangle. [Monitor fallback](#the-original-monitor-went-missing) |
| S19 | A valid saved window is partly or completely outside today's screens | Adjust and fit where possible; the saved normal rectangle must intersect its saved work area. Invalid data falls back; failed live capture preserves the previous record. [Off-screen geometry](#the-saved-window-is-completely-or-partly-off-screen), [validity](#data-validity) |
| S20 | A taskbar occupies the left side, or a monitor has negative screen coordinates | Keep physical virtual-screen coordinates; convert native workspace coordinates internally and apply work-area adaptation. [Value model](../../specs/window-placement-persistence-spec.md#defaults-and-value-model), [native adapter](#native-engine-and-winui-policy) |
| S21 | The saved window was resizable, but the new target is fixed-size or has different constraints | Use the current target's capabilities without changing them; oversized results retain S09's limits. [Fitting](#the-resolution-or-taskbar-work-area-changed) |
| S22 | A snapped window is minimized before capture, or snapping is disabled before restore | Retain known snapped restore history; unknown history becomes plain `Minimized`. Unsupported snap restores normally while restart retains minimization where supported. [Capture](#capture-change-and-supply-placement), [snap](#the-window-was-snapped) |
| S23 | Non-activating restart has no usable saved virtual-desktop identity | Stay on the current desktop without requesting activation, regardless of `DoNotActivate = false`; use safe fallback when required. [Reason policy](#choose-launch-or-restart-policy), [gates](#safe-fallback-and-review-gates) |
| S24 | Prepare placement while hidden, then reveal the window later | Hidden application is repeatable and leaves the phase open. A skipping first `Show` preserves preparation; add non-activation when needed. An unskipped first display can replace it, hidden changes are captured, and enrollment waits for first display. Partial application remains open. [Phase](#loading-applying-and-displaying), [capture](#capture-change-and-supply-placement), [gates](#safe-fallback-and-review-gates) |
| S25 | A launcher requests maximize, minimize-from-snapped, hide, or no activation; or a splash window was shown first | **Deferred:** no new command ownership, precedence, or replay contract. Existing startup compatibility and explicit visibility/activation restrictions still apply. [Deferred launcher proposal](./Window-PlacementPersistence-decisions.md#launcher-show-command-scenarios-deferred), [gates](#safe-fallback-and-review-gates) |
| S26 | Capture a full-screen or compact-overlay window with no valid overlapped history | Capture fails and saving is skipped; initial placement does not change the presenter, and switching back after display does not retry. [Presenters](#the-window-was-full-screen-or-compact-overlay) |
| S27 | Displays change while the window is already running | Use ordinary live windowing behavior, not another persistence restore; a later eligible successful save records the result. [Display changes](#restore-after-display-changes) |

### Developer customization

| Id | Scenario | Intended result and authoritative rule |
|---|---|---|
| S28 | The developer wants normal placement restoration, except that a missing saved monitor should fall back to the primary monitor | Load/edit a detached framework value, replace only a missing non-empty monitor name, and supply it with `Default` and disabled cascading while retaining saving. App-owned storage is optional. [Primary-monitor example](#prefer-the-primary-monitor-when-the-saved-monitor-is-missing) |
| S29 | The developer wants cascading but not automatic storage | Keep the id, opt out of storage, and enable cascading. Select a peer or fallback without explicit data; explicit data allows only same-monitor position adjustment. [Cascade behavior](#cascade-related-windows), [example](#use-expert-mode-cascading-without-automatic-storage) |
| S30 | The developer wants persistence without cascading this window | Opt in and disable cascading on Show, or on hidden application followed by a skipping Show. Saving continues; the shown window can still be another window's peer. [Membership](#group-membership-and-lifetime), [saving](#know-when-placement-is-saved) |
| S31 | Different apps use the same id | Separate packaged application namespaces; shared unpackaged groups can collide. Failed packaged identity resolution never joins the unpackaged group. [Cascade identity](#cascade-related-windows), [storage identity](#application-identity-and-storage-scope) |
| S32 | The app changes the id, Boolean, or options after initial placement | After display, no placement replay or late enrollment. Current Boolean/id gate enrolled saves; the current id controls group membership. Active snapshots are retained. Before display, edits after hidden application affect the next operation. [Phase](#loading-applying-and-displaying), [saving](#know-when-placement-is-saved) |
| S33 | A developer creates a new WinUI project | Generate a stable id and one `Show` with `Launch`, retaining default persistence/cascading; unpackaged ids are app-specific and imply no automatic storage. [Template requirement](#recommended-main-window-code-for-new-projects) |
| S34 | The app supplies explicit placement and also requests cascading | `Automatic` preserves explicit placement; `Enabled` adjusts only normal position from a same-monitor peer after ordinary adjustment. Restart never cascades. [Explicit cascade](#cascade-explicitly-supplied-placement) |
| S35 | A peer disappears, cannot be captured, or launches concurrently | Continue after failed candidate capture; preserve explicit data or proceed to allowed storage/fallback. Once application starts, do not retry sources. Concurrent offsets can overlap. [Peer selection](#select-placement-from-a-live-peer), [fallback](#initial-placement-fallback) |
| S36 | An existing app already calls `Activate()` and wants automatic placement | Set a stable id before initial `Activate`, retaining the default Boolean. Id-only adoption uses default policy and activation; empty id/opt-out keeps legacy behavior. [Initial placement](#use-the-initial-placement-phase), [primary workflows](../../specs/window-placement-persistence-spec.md#primary-workflows) |
| S37 | Maximize on monitor B, enter full-screen, close, then reopen full-screen with B still available | **Open:** retain overlapped maximized state and normal restore bounds; the app remembers full-screen separately. The desired first visible result is full-screen on B with no windowed flash or premature activation, then maximized on B when exiting. Hidden apply, presenter selection, and skipping `Show` express the sequence but do not solve the presenter's `SW_RESTORE` path. Normal-only preparation is insufficient. [Presenter example](#prepare-a-window-while-it-remains-hidden), [gates](#safe-fallback-and-review-gates), [rationale](./Window-PlacementPersistence-decisions.md#why-apply-without-showing) |

## Safe fallback and review gates

**Open:** supported hidden and non-activating maximized/minimized/snapped combinations, the exact
partial-application success boundary, and acceptable backend fallback states require validation.
This is not permission to weaken visibility or focus safety.

When exact state cannot be applied safely, the proposed fallback may retain valid current state
and safely adjusted geometry. For example, normal non-activating display instead of non-activating
maximization is a candidate fallback, not an approved universal result. Capture must report what
actually happened. Fallback is not successful selected placement, and the
[within-operation no-retry rule](#initial-placement-fallback) still applies.

The implementation must not show or activate first and then hide, cloak, or repair focus.
Choose a safe path before dispatch, preserving restrictions through outer display, activation,
and XAML focus handling as well as native placement.

| Gate | Required investigation or decision |
|---|---|
| Native capability | [WINDOW_ACTION][WINDOW_ACTION state model] separates placement state from visibility/activation and can carry minimized restore targets. Set the intended restore target explicitly, including normal. Verify [ApplyWindowAction][ApplyWindowAction requirements] approval, thread ownership, and per-monitor-DPI-awareness requirements; an export or position-only probe is insufficient. |
| Downlevel application | The helper's legacy path can show or activate before rehiding. Receiving `NoActivate` does not prove safety. Establish safe state/fallback behavior on actual supported downlevel systems, not only a forced path on one current OS. |
| Partial application | Establish which adjusted results count as successful selected placement versus fallback. Do not claim rollback or retry another source after native application starts. A fresh later hidden attempt remains permitted. |
| Presenter transition (S37) | Validate the complete hidden-apply, full-screen attach, reveal, and exit sequence, including maximized return state. The presenter's `ShowWindow(SW_RESTORE)` can expose or activate hidden min/max windows; normal bounds alone do not satisfy S37. |
| Native startup compatibility | Include native creation and any earlier splash/native/AppWindow operations. The reviewed [creation sequence](../../dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp#L2280-L2294) uses `WS_VISIBLE`, `CW_USEDEFAULT`, and `SW_HIDE`; [CreateWindowEx][CreateWindowEx startup behavior] can enter native show processing before a request. Do not assume startup input is consumed or preserved, or change unconfigured-app behavior without checking it. |
| Observability | Record native visibility/focus changes and XAML events throughout the operation, including failure, rather than relying on a final screenshot. |

Explicit startup show-command ownership, merging, and replay remain deferred. Their conditional
future gates are recorded with the
[deferred sketch](./Window-PlacementPersistence-decisions.md#launcher-show-command-scenarios-deferred),
not as required launcher outcomes for this feature. Deferral does not prove existing native
startup interactions harmless.

## Extended workflows

The [API primary workflows](../../specs/window-placement-persistence-spec.md#primary-workflows)
own the concise adoption examples. The following snippets retain specialized uses. They assume
an initialized desktop app and app-defined window/content types. They illustrate the proposal,
not a claim that the current prototype implements every sequence.

### Recommended main-window code for new projects

After the API is available, WinUI templates should assign a stable, non-localized main-window id,
retain the default persistence Boolean and automatic cascading, and issue exactly one initial
`Show` with `Reason = Launch`. Keep the existing main-window field; do not show from the constructor
or through `AppWindow` first, and do not add an initial `Activate` afterward.

The API's primary workflow gives the C# shape. The equivalent C++/WinRT template sequence is:

```cpp
m_window = winrt::make<MainWindow>();
m_window.PersistPlacementId(L"MainWindow");

winrt::Microsoft::UI::Xaml::WindowShowOptions options;
options.Reason(winrt::Microsoft::UI::Xaml::WindowShowReason::Launch);
m_window.Show(options);
```

For unpackaged templates, use an app-specific id such as `"Contoso.Editor:MainWindow"` to reduce
shared cascade-namespace collisions; this does not add automatic storage. Restart recovery uses
a separate `ApplicationRestart` path rather than applying the ordinary-launch pattern to every
reconstructed window.

### Remember independent windows

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

### Prepare a window while it remains hidden

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

### Copy a window's placement without copying its state

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

### Supply placement from app-owned data

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

### Use expert-mode cascading without automatic storage

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

### Prefer the primary monitor when the saved monitor is missing

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

### Reconstruct windows after application restart

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

## Implementation design

The following sections specify internal responsibilities and acceptance constraints, not additional
public API. Private format ratification, storage cache policy, native fallback, and infrastructure
remain review work where identified. Dated implementation statements and test counts live in the
[history](./Window-PlacementPersistence-decisions.md#implementation-history), not inline as requirements.

### Implementation boundaries

Keep the following responsibilities separate. These are logical components, not proposed public
types or committed source-file names.

| Component | Responsibility |
|---|---|
| Window coordinator | Own initial-operation snapshots, reentrancy, persistence enrollment, visibility history, and save hooks |
| Capture adapter | Produce a coherent placement snapshot from the HWND and WinUI's state caches |
| Store adapter | Resolve packaged application identity, derive names, read or replace complete values, and enforce placement retention |
| Record codec | Translate between validated placement data and a bounded, versioned byte representation |
| Placement adapter | Translate to PlacementEx, apply current-window policy, and report actual results |

The store adapter must not move windows. The codec must not query current monitors, activate WinRT
storage objects, or depend on C++ structure layout. The placement adapter must not choose a storage
identity or save data as a side effect.

Use the [initial-operation pipeline](#loading-applying-and-displaying),
[save eligibility](#know-when-placement-is-saved), and [validity rules](#data-validity) above.

### Coordinator ownership and operation boundaries

One private coordinator owned by `DesktopWindowImpl` owns phase and enrollment state; do not scatter
lifecycle flags among the public projection, store, and native adapter.

| Owner | Responsibility |
|---|---|
| Public projection | Copy public options and placement into private values; preserve the spec's API-use and argument-error mappings |
| Window coordinator | Classify the call, guard reentrancy, own phase and first-display enrollment, and choose whether to run placement |
| Per-call operation context | Hold immutable request/id/Boolean snapshots, selected source, working placement, and application outcome |
| Placement policy and native adapter | Adjust and apply one selected snapshot, then capture effective results; do not enroll or end the phase |
| Capture cache and save hooks | Retain effective window state; require enrollment and actual display before an eligible save |


Track phase-open, in-progress, ever-displayed, first-display enrollment, and close-save completion
separately. Keep selected-source outcome local to each attempt. The
[public operation table](#loading-applying-and-displaying) defines transitions; the host's actual
display observation closes the phase, not reservation of a call.

Classify late and reentrant calls before validating ignored fields, after the public
[required-argument/thread/closed-window checks](../../specs/window-placement-persistence-spec.md#errors-and-threading).
Snapshot an eligible initial request before invoking storage, peer, or native collaborators.
Scope the guard through display callbacks and release it on every exit. A surviving pre-display
abort discards the operation context so the next call starts fresh.

`SkipInitialPlacement` belongs at the coordinator's pass boundary, not inside `PlacementRequest`
or `NativeApplyOptions`. After validation, go directly to display and ordinary pending sizing,
without calling source selection, the reader, adjustment, application, or fallback positioning.

Hidden application creates a new context each time. `AllowsSourceRetry` limits retries within that
pass, not future calls. Effective geometry belongs in the capture cache; no retained request or
hidden outcome may become enrollment or deferred work. Save hooks use their own current-property
snapshot, not the enrollment id as a permanent destination.

### PlacementEx integration

The imported helper is under [external/inc/PlacementEx](../../external/inc/PlacementEx/PlacementEx.h).
Its [window-position guidance](../../external/inc/PlacementEx/RememberingWindowPositions.md) and
[Win32 concepts](../../external/inc/PlacementEx/Win32Concepts.md) provide native background, not
additional WinUI API guarantees.

Include the helper through [User32Utils.h](../../external/inc/PlacementEx/User32Utils.h). The imported
code already has `CaptureFlags::SkipVirtualDesktopId` and a
`PLACEMENTEX_STRING_SERIALIZATION` switch. The persistence adapter uses its own codec instead of
`ToString`, `FromString`, or registry-storage helpers. Do not assume the prototype component or blob
implementation mentioned in header comments has also been imported.

For virtual desktops, the helper conditionally includes
[VirtualDesktopIds.h](../../external/inc/PlacementEx/VirtualDesktopIds.h) when
`USE_VIRTUAL_DESKTOP_APIS` is defined. That header records COM and library requirements; verify them
against the WinUI build and supported deployment configurations rather than assuming that copying
the headers establishes a usable backend.

Use one engine configuration across translation units. The adapter enables virtual-desktop fields
and disables unused string/registry serialization consistently. WinUI owns bounded record encoding;
the helper's native constants and structure layout must not define durable public or stored values.

### Native engine and WinUI policy

The proposed implementation uses the checked-in
[PlacementEx implementation](../../external/inc/PlacementEx/PlacementEx.h) through a thin internal
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

- [Capture and native validity](../../external/inc/PlacementEx/PlacementEx.h#L429-L581)
- [Monitor selection and application](../../external/inc/PlacementEx/PlacementEx.h#L583-L790)
- [Normal and snapped geometry adjustment](../../external/inc/PlacementEx/PlacementEx.h#L1298-L1444)
- [Cascade adjustment](../../external/inc/PlacementEx/PlacementEx.h#L1448-L1489)
- [Previous-window sample policy](../../external/inc/PlacementEx/PlacementEx.h#L1074-L1112)
- [Launch and minimized-state adjustment](../../external/inc/PlacementEx/PlacementEx.h#L1493-L1625)

`IsValid()` requires an intersection with the saved work area, not the current topology.
The public validity rule retains that distinction. Public-input handling must additionally
check arithmetic before invoking helpers whose native calculations assume valid desktop geometry.

The policy unit owns source/reason/cascade decisions, public-state translation, current constraints,
and request shaping. Its backend-neutral request is not a second geometry implementation.
Adapter seams may inject topology for tests only when they still exercise imported-engine behavior.

Two design points are worth flagging:

| Point | Decision |
|---|---|
| Cascade coordinate space | A peer placement is cascaded in the peer's own work area before display adjustment, matching `CascadeOver`. Explicit placement is cascaded after adjustment, using the peer anchor and the explicit adjusted size. |
| Cascade offset | The caller supplies the caption-plus-frame offset. A zero or negative offset means the caller could not read system metrics, and the result is simply uncascaded. |

`BuildNativeRequest` must fail when the selected engine path cannot honor hidden application or
non-activation without showing or activating first. That is an adapter capability result, not a
reason to add a parallel implementation. The [hidden-preparation example](#prepare-a-window-while-it-remains-hidden)
accepts `applied == false`. Successful hidden maximized, minimized, or snapped application needs
a validated native transition and engine support; a policy-only workaround does not close the
[fallback gate](#safe-fallback-and-review-gates).

### Cross-process cascade discovery

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

Discovery and cascade calculation alone are neither successful application nor saving. A
peer-derived hidden application follows the same first-display enrollment boundary as any other
source; it cannot establish save eligibility by itself.

Unpackaged cross-app matches are an accepted consequence of the empty application identity.
No executable-path fallback or separate public app-identity/cascade-group property is proposed.
This does not change the packaged-only scope of automatic storage.

## Storage identity and layout

### Resolve the packaged application, not just its package

Use `Microsoft.Windows.Storage.ApplicationData.GetDefault()` and `LocalSettings` as the proposed
backend. Access the Windows App SDK storage implementation internally; do not add its types to the
public XAML metadata or expose a placement-specific store override.

The default packaged store supplies current-user and package-family scope. It does not distinguish
two registered applications in the same package. Resolve the registered application identity with
`GetCurrentApplicationUserModelId` and add an application namespace within that store, as required by
the spec's [identity rationale](./Window-PlacementPersistence-decisions.md#why-use-application-identity-rather-than-package-family-alone).

Use the registered identity, not the process's explicit shell grouping id, executable path, window
title, process id, or versioned package full name. Processes of the same registered application
must derive the same namespace across package version and architecture changes.


The [API scope](../../specs/window-placement-persistence-spec.md#compatibility-and-scope) remains
packaged-only for automatic storage. The wanted unpackaged default-store dependency is
[unresolved](./Window-PlacementPersistence-decisions.md#wanted-dependency-unpackaged-default-applicationdata);
it does not authorize a namespace or API extension here.

Failure to resolve a packaged application's identity makes storage unavailable for that operation.
It must not select an unpackaged namespace or a package-wide fallback. A missing application-scoped
record must not cause a read from an older package-wide prototype slot.

### Proposed packaged container and value names

Use this logical `ApplicationData` layout:

```text
LocalSettings
  Microsoft.UI.Xaml.WindowPlacement
    app1_<application-digest>
      wp1_<readable-slug>_<placement-id-digest> = <base64 record>
```

The application digest is `Base32(SHA-256(UTF-16LE(registeredApplicationId)))`. The placement-id
digest is `Base32(SHA-256(UTF-16LE(rawPersistPlacementId)))`. Separate containers keep the two
identity components unambiguous without concatenating arbitrary strings into a compound key.

Hash the exact UTF-16 code units, with no trailing terminator, case folding, or Unicode
normalization. Use the `HSTRING` length rather than a NUL-terminated string function. Preserve the
id's ordinal, case-sensitive semantics even if the settings backend compares names without case.
Use a platform SHA-256 implementation, not a process-specific string hash or a private hash algorithm.

Base32 uses the uppercase RFC 4648 alphabet without padding. A SHA-256 digest produces 52 characters.
Base64 and Base64url are not suitable for these names because their alphabets distinguish uppercase
and lowercase characters. They remain suitable for a settings value, whose contents are not a key.

The readable slug scans the raw placement id from left to right, keeps only ASCII `[A-Za-z0-9]`,
and stops after 16 accepted characters. Use `id` when none are accepted. For example,
`DocumentWindow:doc42` produces `DocumentWindowdo`. The complete value name is at most 73 characters.
The digest provides uniqueness; the slug is only a diagnostic aid and reveals part of the id.
Hashing does not make the name confidential or authenticate the record.

These namespaces prevent accidental collisions through the placement APIs. They do not create an
access-control boundary between applications that share the package's app-data store.

### Storage access and lifetime

Resolve storage only when a load or eligible save needs it. Property setters perform no storage I/O.
An explicit placement or selected live peer can avoid an automatic read while still allowing a
later eligible save.

Carry the resolved application namespace and id-derived value name in an immutable operation
context. An id change during capture or a callback must not redirect an operation already in
progress. A later save builds a context from its own Boolean/id snapshot.

Successful identity and naming results can be cached. Any cached storage interfaces must obey their
threading and lifetime rules; an agile public placement object does not make a storage interface
agile. Do not share an apartment-bound handle across UI threads or detached-load callers.

Treat unavailable storage as an operation result, not a permanent public opt-out. Any negative-cache
or retry policy remains an integration decision; a transient read failure must not silently remove
first-display enrollment or permanently prevent later eligible saves.

Reading a missing container or value returns no record without creating settings containers. Create
containers when writing. Do not delete or repair a record as a side effect of loading it. A later
eligible successful save can replace unreadable data.

Use `ApplicationData` for access and app-data lifetime. Do not depend on the backing registry path,
open a package's `settings.dat` directly, or document the private container as an app-facing reset
API. Changing an id neither deletes nor migrates its old record.
Subsequent successful saves can evict old records under the
[retention policy](#retaining-the-32-most-recently-saved-placements).

## Loading placement data

### One reader for automatic restore and detached loading

Both automatic loading and `WindowPlacement.LoadForPersistPlacementId` should use the same identity
resolver, value-name derivation, decoder, and structural validation. The shared reader accepts a
non-empty id snapshot and returns either an immutable saved snapshot or a categorized failure.

1. Resolve the packaged application namespace and default store.
2. Look up the application container and value without creating either.
3. Require a string value and reject an oversized encoded value before allocating a decoded buffer.
4. Base64-decode, validate the envelope, and parse bounded TLV fields.
5. Convert wire rectangles and state into a complete placement snapshot with checked arithmetic.
6. Validate the snapshot against its saved coordinate environment, not today's monitor topology.
7. Return saved data without display adjustment, peer discovery, native application, or a write.

Keep at least these internal outcomes distinct:

| Outcome | Meaning |
|---|---|
| Loaded | One valid, coherent saved snapshot |
| Missing | The application container or value does not exist |
| Unavailable | No supported identity/store, or access denied, under the phase-specific rules below |
| Invalid or unsupported | Wrong value type, invalid encoding/data, or unsupported major version |
| Unexpected failure | A failure outside the preceding categories; retain the original failing HRESULT |

The [API error contract](../../specs/window-placement-persistence-spec.md#errors-and-threading)
owns detached-load null/error results. Automatic restoration uses best-effort source fallback for
the same categorized failures. Both paths use the following phase-specific backend classification;
do not replace it with a broad catch or an arbitrary failed-HRESULT-to-unavailable mapping.

| Boundary/result | Classification |
|---|---|
| Packaged identity query reports `APPMODEL_ERROR_NO_PACKAGE` or `APPMODEL_ERROR_NO_APPLICATION`; supported capability detection reports no default store | Unavailable |
| Identity/store access reports `E_ACCESSDENIED` or the equivalent Win32 access-denied code | Unavailable; retain the normalized HRESULT for diagnostics |
| Container/value lookup reports absence through its documented lookup result | Missing; do not create a container or treat unrelated lookup errors as absence |
| A successfully read value fails type, size, encoding, version, or structural validation | Invalid or unsupported; no record repair or deletion |
| Any other failing identity, storage, decoder, or public-value construction operation | Unexpected; preserve the HRESULT, including `E_OUTOFMEMORY`, `E_FAIL`, activation/deployment errors, and `RO_E_CLOSED` |

Normalize a Win32 error with `HRESULT_FROM_WIN32`; retain an existing failing HRESULT unchanged.
For example, the identity query's expected buffer-size probe is not a failure. Failure of runtime
class activation is not proof that the product lacks a supported default-store capability.
Malformed data is distinct from failure to allocate its decode buffer. Expected-unavailability
classification must be explicit and covered by backend tests, not inferred from arbitrary
exception text or a broad catch.

Private backend, identity, and `LoadResult` boundaries carry status plus original error. An
unexpected result must carry a failing HRESULT; normalize Win32 errors, preserve existing HRESULTs,
and use `E_FAIL` only when no code exists. Map allocation failure to `E_OUTOFMEMORY`, not missing,
unavailable, or invalid data, and prevent it from escaping a `noexcept` boundary.

Report failures once at the operation boundary using the diagnostics below. Returning a failure
does not grant a read path permission to mutate storage, enrollment, caches on a window, or phase
state. Do not copy arbitrary backend exception text into diagnostics or newly originated public
error messages; preserving the HRESULT does not require preserving that text.

The detached API can run without a window and from any thread. It must not consult a window's
enrollment, end an initial-placement phase, or invoke the placement adapter to "validate" data by
moving an HWND. Each successful call creates an independent public value from the decoded snapshot.

### Where the reader fits in initial placement

Invoke the reader only when the [source-selection pipeline](#loading-applying-and-displaying)
permits storage. No speculative read accompanies explicit placement, a selected peer, opt-out,
or a skipping first `Show`. Keep source selection separate from application so a partial native
failure cannot fall through to another source. Policy adjusts a working copy, not the decoded,
caller-owned, or stored snapshot.

## Capturing data for persistence

### Maintain a coherent overlapped snapshot

The capture adapter supplies both public capture and saving. Start with current overlapped geometry
from `PlacementEx::GetPlacement`, then reconcile it with WinUI's tracked placement state. Run native
capture in a coordinate context that produces physical screen pixels; the helper can otherwise
return DPI-virtualized geometry.

Maintain the last meaningful show state independently from HWND visibility. Hiding alone must not
replace maximized, minimized, or snapped state with `SW_HIDE`. A hidden overlapped window can still
move, resize, or change state, so combine its current geometry with current meaningful state rather
than replaying an old entire snapshot solely because it is hidden.

WinUI must also track minimized-from-snapped history. The helper captures current arranged state
and restore-to-maximized information but cannot reconstruct every restore-to-snapped history from
an already minimized HWND.

Before entering full-screen or compact overlay, retain a complete valid overlapped snapshot:
normal and snap rectangles, show/restore state, monitor work area, DPI, and device name. The earlier
position-and-size-only cache is insufficient. A maximized window entering full-screen must retain
both its maximized state and normal restore bounds. Do not combine presenter bounds with stale
overlapped monitor metadata.

Use the spec's [capture selection rules](#capture-change-and-supply-placement)
when choosing current geometry or that cached snapshot. If meaningful state required for a valid
snapshot is unavailable, fail capture rather than fabricate a record. Initial valid overlapped
geometry can use `Normal` before any meaningful show state exists; that alone does not enable saving.

After application, capture the actual resulting placement. The mutable PlacementEx argument is not
a reliable post-apply snapshot: backend paths can mutate it differently, and partial application can
leave a state other than the requested state. Do not cache a requested special state as if it were
successfully applied.

### Cache optional virtual-desktop identity safely

Virtual-desktop queries can call into the shell through COM. The imported helper documents failures
before taskbar registration and from input-synchronous messages, as well as potential blocking.
Do not make a synchronous shell query from `WM_ENDSESSION` or another unsafe window-message path.

Use the existing `CaptureFlags::SkipVirtualDesktopId` option for those captures and overlay the last
safely cached non-empty desktop id. If no valid cached id exists, omit it. Geometry capture and saving
must not depend on obtaining the optional id.

Schedule a safe query after first display and refresh at suitable later opportunities. Correlate
completion with the live window's lifetime, not just its numeric HWND, so late results cannot attach
to a closed or reused window.

The design uses a posted registered window message on the window's UI thread for refresh, rather
than a sent message or helper thread. The
[scheduling rationale](./Window-PlacementPersistence-decisions.md#virtual-desktop-refresh-scheduling)
records the COM and lifetime reasons.

Scheduling happens on both display paths: the framework `Display` call and the native `WM_SHOWWINDOW`
path. `WM_SHOWWINDOW` arrives before the window reports itself visible, so that path states that a
display is happening instead of asking. Re-showing a window re-schedules, which is how the id is
refreshed later. A single display reaches both paths, so a pending-post flag coalesces them into one
shell query and is cleared when the message is handled.

The query is paid for only by windows that can actually use it. The id is only ever read back out of
a saved record, and every save path requires enrollment, so the handler drops the refresh for a
window that never enrolled. Enrollment is recorded after the first `Display` returns, so this is
checked when the posted message runs rather than when it is posted.

Three seams keep this testable without the shell:

- `ShouldScheduleVirtualDesktopRefresh(isClosed, displayed)` is the scheduling decision. It
  deliberately ignores the lifetime token, because the cache may not be bound yet at display time and
  the token is what correlates the *result*, not the request.
- `ShouldRunVirtualDesktopRefresh(isClosed, hasEnrollment)` is the decision made when the posted
  message arrives, after enrollment has settled.
- `TryRefreshVirtualDesktopId(cache, hwnd, query)` takes the lifetime token **before** running the
  injected query and only then calls `TrySetVirtualDesktopId`. A failed query, a zero token, or a
  window retired mid-query all end with the placement unchanged and still saveable.

If nothing has captured when the message arrives, the handler captures first so a live token exists
for the result to attach to. That capture is behind the enrollment check too, so a window that never
persists placement is not forced to capture on display.

Loading retains the saved optional id without contacting the shell. Whether application uses it
belongs to [reason policy](#choose-launch-or-restart-policy),
not the codec or store adapter.

## Saving placement data

### Lifecycle hooks

Integrate the [save triggers](#know-when-placement-is-saved) at these `DesktopWindowImpl` boundaries.
These are required ordering points, not claims about the state of an implementation checkpoint.

| Trigger | Proposed hook and ordering |
|---|---|
| Accepted framework close | In `CloseImpl`, after `Closed` handlers return and cancellation is checked, before `PrepareToClose`, content removal, or `Shutdown` |
| Native destruction bypassing normal close | At the start of `OnMessage` handling for `WM_DESTROY`, before the last-window `PostQuitMessage` early return |
| Confirmed session end | On `WM_ENDSESSION` with nonzero `wParam`, while valid geometry or tracked placement remains available |
| Cancelled close or cancelled session end | No write for that request |

`WM_CLOSE` reaches `CloseImpl` through `OnClosed`; `Window.Close()` also uses the framework close
path. Handlers can change live placement, the Boolean, or the id. Snapshot only after accepted-close handlers
finish so those changes affect the save.

`AppWindow.Destroy()`, external destruction, or teardown can bypass accepted close. The destruction
hook is a best-effort backstop, not permission to postpone the ordinary save until the HWND and
XAML state are already dismantled.

Keep normal-close/destruction deduplication separate from session-end handling. Mark a normal-close
save complete after a successful write; a failed attempt must not masquerade as saved data. A
backstop may still attempt a write if usable state remains. Do not suppress a later accepted-close
save merely because session end already wrote: shutdown can terminate the process before normal
teardown, and a redundant small write is preferable to relying on a later message.

### Save sequence

1. Check enrollment and the spec's visibility requirement: the window must have been displayed at
   least once. Hidden placement application alone is insufficient, whether it succeeded or failed.
2. Snapshot the current Boolean and id together before capture or storage access, after accepted-close
   handlers where applicable. Skip this attempt unless the Boolean is `true` and the id is non-empty.
   Keep both snapshots for the entire attempt; subsequent edits cannot redirect or cancel it.
3. Capture one coherent snapshot while the HWND and WinUI's tracked placement are valid. Use the
   message-safe virtual-desktop path when required.
4. Validate the complete snapshot and translate it into WinUI-owned wire state. Do not serialize
   native flags wholesale or save presenter geometry as overlapped geometry.
5. Resolve the supported application/store context and derive the value name from the id snapshot.
   Acquire the bounded per-namespace save/cleanup coordination described below. Create missing
   private containers only at this write stage.
6. Assign the next save sequence, then serialize and base64-encode the complete record in memory with
   checked sizes. Publish placement and sequence together, not in separate settings writes.
7. Replace one settings value with the complete string. Record the result and update the relevant
   close-save completion state only on success.
8. After a successful replacement, trim excess placement records under the same coordination, then
   release it. A cleanup failure does not turn the completed placement write into a failed save.

The [current-property save rules](#know-when-placement-is-saved) apply to every attempt, including
backstops: an old enrollment id is not a write destination, a skipped attempt is not completed,
and a cancelled close does not undo handler edits.

A failed capture or serialization must leave the prior stored value alone. Do not delete the old
value before inserting its replacement. A storage error is a failed save, not an empty successful
record. Display and close continue under the API's best-effort persistence contract, with diagnostic
reporting rather than an app-visible persistence exception.

Complete the bounded save attempt before the relevant teardown proceeds. An unawaited queued write
is not a reliable close/session-end save. Backend latency and failure behavior must be measured;
"best effort" does not establish that an arbitrary storage operation is safe on a shutdown path.

### Multiple windows and processes

Publish all fields as one value rather than independently updating geometry, DPI, and state keys.
The backend must support coherent whole-value reads and replacement for concurrent processes. Verify
that property; do not infer transaction or power-loss guarantees solely from using `LocalSettings`.

There is no read-modify-write merge. Two eligible saves to the same application/id slot follow the
spec's last-successful-save-wins rule. Different registered applications in one package use different
containers. A reader must see a complete record or report a read/parse failure, never apply mixed
fields from two saves.

Save-sequence assignment, replacement, and retention deletion require per-namespace cross-process
coordination. This protects storage ordering, not window positions. It is not the older proposed
cascade mutex or unoccupied-slot search. Peer discovery and placement follow the spec's
[cross-process cascade design](#cross-process-cascade-discovery)
and do not require a store transaction.

## Retaining the 32 most recently saved placements

### Policy and scope

Treat automatic placement storage as a bounded cache. After a successful save, if the application's
namespace contains more than **32 placement records**, delete the least recently saved records until
32 remain. Saving a 33rd distinct id normally evicts one record; an already oversized store can
require several deletions.

The count is distinct stored placement ids, not live windows, processes, monitors, or cascade peers.
Two windows saving the same id use one slot. The limit is per current-user/application storage
namespace, shared by its processes; another application in the same package has its own allowance.
The future unpackaged default-store integration must preserve equivalent application isolation.

Recency means last successful save, not last load, display, activation, or window creation. Updating
an existing slot makes it newest without increasing the count. Loading does not change retention
metadata or trigger cleanup. Failed writes and cancelled close/session-end requests do neither.

Eviction affects only stored placement. Do not close, move, or un-enroll a live window whose old
record is evicted. Its next eligible save can recreate the slot. A later detached load of an evicted
id returns no saved value; automatic placement follows normal peer/stored/fallback selection.
Do not add a public retention setting, pinning mechanism, or deletion API in this design.

### Persist save order with the record

Add the private optional `TAG_SAVE_SEQUENCE` field described below. Each successful save carries a
positive `u64` sequence greater than every supported sequence currently in its application namespace.
Allocate it while holding the save/cleanup guard, using the maximum stored sequence plus one; start
at one for an empty namespace. Reject overflow rather than wrapping.

This orders successful replacements across processes and restarts without relying on wall-clock
timestamps, which can tie or move backward. No separate persisted counter or recency index is needed:
the sequence and placement are published in the same value. A failed write does not publish a new
sequence, and a later attempt can reuse that number.

The sequence belongs to storage, not public `WindowPlacement`. Detached loading does not expose it,
explicit placement cannot choose it, and saving a loaded or copied placement assigns a fresh sequence.
Readers accept older records without the tag and treat their retention sequence as zero. Use ordinal
value-name order to break ties deterministically, including legacy zero sequences.

### Save and trim under one guard

Coordinate all writers and cleaners for the same user/application/store namespace, for example with a
named mutex whose identity and access control match that scope. It must cover processes in different
sessions if they share the same store, not just writers on one UI thread or in one session.
Serialization of an individual settings value is not sufficient to prevent a cleaner from deleting
a slot another process has just refreshed.

Capture geometry before entering the guard. Do not perform native placement or invoke app callbacks
while holding it; guard against reentrant saves too. Use a bounded acquisition and work budget,
especially during session end. If coordination cannot be obtained, report a failed best-effort save
without writing or deleting outside the protocol. Lock availability is not permission to block
shutdown indefinitely.

While holding the guard, enumerate only placement value names owned by this format in the current
application container. Read and validate their bounded record metadata, assign the new sequence, and
publish the new record. Only after that replacement succeeds:

1. Count the owned placement values, including the newly written one.
2. If the count exceeds 32, order eviction candidates by ascending save sequence, then value name.
3. Delete the oldest candidates until the count reaches 32. Never evict the just-saved value in this
   cleanup pass.

Do not delete first to make room for a save that might fail. Do not delete unrelated values,
unrecognized key formats, or another application's container. Recognized corrupt records can be
treated as sequence zero and evicted first; missing/zero-sequence legacy records join that oldest
group. A transient read error is not evidence that a record is corrupt.

If enumeration or a read fails, or an owned record uses an unsupported major format whose ordering
cannot be interpreted safely, do not guess which valid record is oldest. Defer the save/cleanup
cycle and report the reason. Readers of newer major formats must preserve or explicitly migrate the
ordering protocol before cooperating with this cleaner.

### Failure recovery and bounds

The save and multiple deletions are not one storage transaction. A crash after replacement or a
failed deletion can leave more than 32 records. Stop cleanup on a deletion failure, retain the
successful save, and report cleanup separately; a later successful save retries trimming against a
fresh enumeration. An abandoned guard similarly requires rereading store state, not trusting an
in-memory victim list.

The normal completed cycle retains at most 32 records. This is a best-effort bound during storage
failures, interruption, unsupported formats, or writes from older implementations that do not
participate in the guard protocol. Do not advertise a strict instantaneous quota or a hard byte
limit. The existing per-record size ceiling remains independent.

Measure enumeration and deletion costs for an oversized store as well as the usual 32-record case.
If the operation budget is exhausted, stop safely and report deferred work; do not risk shutdown
responsiveness to force the count down immediately. Retention checks do not introduce move/resize,
load-time, or background persistence writes.

## Private record format

### Envelope and bounds

Retain the earlier proposal's base64-encoded, versioned TLV representation, independently of
PlacementEx's string and registry helpers. WinUI owns the wire meanings; C++ packing, pointer size,
native enum changes, and helper revisions must not alter them accidentally.

| Part | Representation |
|---|---|
| Header | ASCII `WPL1` (4 bytes), major `u16`, minor `u16`, total decoded byte length `u32` |
| Field | tag `u16`, value byte length `u16`, then that many value bytes |

The header is 12 bytes. Version 1.0 writes major `1` and minor `0`; all integer fields are
little-endian. Total length includes the header and must exactly equal the decoded byte count.
There is no field count or implicit padding. Each TLV must fit entirely within the remaining bytes,
and the final field must end at the declared boundary.

Use a 4 KiB decoded-record ceiling, with a corresponding maximum of 5,464 base64 characters, before
allocating decode storage. Enforce backend value limits too; this parser ceiling is not a promise
that every future record of that size fits the store. The largest canonical record using all tags
below is only 194 decoded bytes, or 260 base64 characters.

Write each applicable tag once in ascending order. The reader need not require that order. Generate
golden byte sequences rather than persisting a C++ struct or accepting host-endian output.

### Fields

| Tag | Value | Encoding | Value length |
|---|---|---|---|
| `TAG_NORMAL_RECT` | `0x0001` | Four `i32`: left, top, right, bottom | 16 |
| `TAG_WORK_AREA` | `0x0002` | Four `i32` | 16 |
| `TAG_ARRANGE_RECT` | `0x0003` | Four `i32` | 16 |
| `TAG_DPI` | `0x0010` | `u32` | 4 |
| `TAG_SHOW_CMD` | `0x0011` | Canonical native show command as `u32` | 4 |
| `TAG_FLAGS` | `0x0012` | WinUI-owned state bits as `u32` | 4 |
| `TAG_DEVICE_NAME` | `0x0020` | UTF-16LE code units, no terminator | 0-62 |
| `TAG_VIRTUAL_DESKTOP_ID` | `0x0021` | Windows `GUID` field layout | 16 |
| `TAG_SAVE_SEQUENCE` | `0x0030` | `u64` save-order metadata, little-endian | 8 |

The normal rectangle, work area, and DPI are required. A missing show command defaults to normal;
missing flags default to zero. The writer emits the arrange rectangle only for a snapped restore
state, the device name only when non-empty, and the desktop id only when available and non-empty.
The automatic writer emits the assigned positive save sequence. It is optional on read for older
records; absence or zero means unknown/oldest recency and does not invalidate placement. Keep this
metadata out of the detached public snapshot and PlacementEx translation.

Encode a GUID as little-endian `u32 Data1`, little-endian `u16 Data2`, little-endian `u16 Data3`,
and the eight `Data4` bytes. Do not serialize a textual GUID or copy an unspecified structure layout.

Wire rectangles use edges; the public value uses X, Y, width, and height. Both conversions require
checked arithmetic. Every present rectangle must have positive dimensions representable by the
public type. The normal rectangle must intersect the saved work area with positive area. Do not
retain the older draft's arbitrary +/-1,000,000-coordinate cutoff: it is not part of the current
data-validity contract.

DPI must be at least 96 and representable by the public signed integer type. Device-name length must
be even, at most 62 bytes, and contain no embedded NUL. Empty device names and `GUID_NULL` mean no
identity. Loading does not reject a structurally valid record because its monitor or desktop no
longer exists.

### State translation, not persisted policy

Retain only these meanings from the earlier durable-flags proposal:

| Wire bit | Meaning |
|---|---|
| `0x0001` | Restore to maximized after minimization |
| `0x0002` | Snapped placement |
| `0x0020` | Restore to snapped after minimization |

The old `AllowPartiallyOffScreen` (`0x0004`) and `AllowSizing` (`0x0008`) bits are not emitted or
honored. Keep their values reserved rather than assigning new meanings. Current target-window
capabilities and WinUI policy govern fitting; saved source-window styles must not override them.

`KeepHidden`, `NoActivate`, forced-downlevel testing, structural `FullScreen`, and presenter kind
are not persisted by this feature. Do not persist the native virtual-desktop flag either: derive
it from the optional id and the current request policy at the placement boundary.

The canonical writer maps the six public states as follows. This is a wire conversion table, not a
second definition of their public behavior.

| Snapshot state | `TAG_SHOW_CMD` | State bits |
|---|---|---|
| `Normal` | `SW_SHOWNORMAL` (1) | None |
| `Maximized` | `SW_SHOWMAXIMIZED` (3) | None |
| `Minimized` | `SW_SHOWMINIMIZED` (2) | None |
| `Snapped` | `SW_SHOWNORMAL` (1) | `0x0002` |
| `MinimizedFromMaximized` | `SW_SHOWMINIMIZED` (2) | `0x0001` |
| `MinimizedFromSnapped` | `SW_SHOWMINIMIZED` (2) | `0x0020` |

Both snapped states require a valid arrange rectangle. The codec never writes `SW_HIDE`, and saving
does not normalize minimization into an ordinary-launch state. That normalization happens later on
an apply-time working copy; otherwise application restart would lose the saved restore state.

### Reader validation and evolution

Reject invalid base64, invalid magic, unsupported major versions, missing required fields, incorrect
known-field lengths, truncated TLVs, trailing bytes, and unsafe arithmetic. Run the reconstructed
snapshot through the common structural validator before exposing it or calling native placement.
`PlacementEx::IsValid()` alone does not enforce all public field and conversion requirements.

Retain these compatibility rules from the proposed reader:

- Accept higher minor versions under major version 1 and skip unknown tags by their checked length.
- Default absent optional fields. Canonicalize recognized normal, maximized, and minimized show
  aliases; treat an unknown or non-displaying command, including `SW_HIDE`, as an absent show tag.
- Ignore unknown and reserved flag bits rather than passing them to PlacementEx.
- After defaults and masking, require a state combination represented in the conversion table.
  Reject conflicting known restore-state bits rather than guessing a restore target.
- Require valid snap geometry for a snapped state. Validate every present rectangle even when the
  resulting state does not use its snap rectangle.
- Retain last-one-wins handling for duplicate tags, but validate every encountered known field's
  length and bounds. The canonical writer never emits duplicates.

An additive optional field can increment the minor version only if ignoring it remains safe.
A must-understand semantic change requires a new major version. Do not reuse tags or reserved bits
with incompatible meanings. The `WPL1` envelope marker and explicit version fields must be handled
consistently by future readers and writers.

Older readers can discard unknown fields when they later write their own canonical record. Do not
promise preservation of future fields across downgrade or describe private prototype records as a
shipped compatibility contract. Establish golden records and migration requirements before shipping
the first durable format.

## Diagnostics and implementation gates

### Report failures without logging placement data

Use structured internal results rather than success-shaped defaults. Report capture, identity,
decode, apply, and write failures through an ETW event and a debug breadcrumb containing the
operation, failure category, and error code. Treat an ordinary missing record as an expected outcome,
not an unexpected storage error.

Do not include raw ids, derived names/slugs, record contents, coordinates, device names,
virtual-desktop ids, or arbitrary backend error text. A hash is not anonymization and is not needed
to diagnose the operation class. Retain the original normalized HRESULT when one is available;
data validation without a native error uses its internal failure category, not a fabricated HRESULT.
Automatic persistence failures remain non-fatal to display and close; the detached loader's
unexpected failures propagate their original HRESULT. Emit one failure event/breadcrumb per failed
load or save attempt at the operation boundary, rather than duplicating reports in each helper.

### Warn a developer when Activate() discards a prepared placement

Emit development output when an opted-in first `Activate` follows hidden placement on the same
window. This must not change display, placement, enrollment, return values, or failure behavior;
apps must not depend on the diagnostic. Its
[rationale and rejected alternatives](./Window-PlacementPersistence-decisions.md#why-activate-gets-no-skip-affordance)
are separate from the implementation.

Keep the flag per coordinator, arm it only when a hidden pass reaches `ApplyPlacement`, and
report after a valid activation operation starts but before its automatic pass. Report at most
once per coordinator, in debug output only, not as a release event.

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
  explicit customized placement plus hidden apply retains opted-in saves, followed by presenter
  selection and then a skipping Show that preserves preparation, subject to the open S37 transition;
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

### Coordinator collaborator checks

Use injected collaborators to observe pass boundaries, not only final geometry. Pair these
requirements with real-window visibility, reentrancy, and enrollment tests.

| Sequence | Required observation |
|---|---|
| Hidden apply, then hidden apply with different options | Two independent passes; phase remains open; no enrollment or save |
| Hidden apply succeeds or partially fails, then first non-skipping `Show` | Fresh source selection and policy; previous outcome does not suppress the new pass |
| Hidden preparation, then opted-in first `Activate` | Default automatic pass still runs per the decided contract; no implicit skip or remembered options |
| Hidden preparation, then skipping first `Show` | No selector, read, adjustment, cascade, or placement-application calls; display and ordinary enrollment still occur |
| Invalid initial skip combination or enum | Argument error before collaborators run; no display, phase change, or enrollment |
| Eligible hidden apply with skip | Argument error even with null placement; a late or reentrant hidden request instead follows its early-false rule |
| Hidden apply, then direct native display, then framework display | Phase ended by native display; no later automatic pass or enrollment |
| First display, hide, then `Show` with invalid ignored placement fields | No placement validation or replay; only applicable display policy is used |
| Hidden apply, then close without any display | No save regardless of hidden-application success |
| Validated first `Show` whose callback aborts display while the window survives | Guard released; phase still open; no enrollment; a following `Show` runs a fresh full pass |
| Same abort, then close while still never displayed | No save |

### Storage-specific acceptance work

The following implementation-focused requirements supplement, rather than replace, the scenario
inventory and public acceptance criteria.

| Area | Required evidence |
|---|---|
| Identity and names | Golden digests for ASCII, non-ASCII, case-distinct, non-normalized, embedded-NUL, and surrogate-containing ids; separate applications in one package; stable results across processes and architectures |
| Store backend | Packaged read/write through the internal ABI; framework-dependent and self-contained deployment; supported value/container limits; thread use; update/uninstall lifetime |
| Proposed unpackaged default | Once the dependency is available: setup before first use, missing/late/repeated configuration, stable identity and cleanup, and the same namespace for automatic load, detached load, and save |
| Reader errors | Each phase-specific null case, original HRESULT propagation through identity/backend/reader/public boundaries, allocation failure, C#/C++/WinRT exception mapping, and automatic-restore fallback for the same injected failures; no writes during load or package-wide fallback |
| Codec | Golden little-endian records and all six states; bounded base64/TLV parsing; duplicate and unknown tags; unknown versions/bits; malformed lengths; signed conversion and scaling overflow |
| Save hooks | Handler changes to id and geometry; cancellation; destruction backstop before the early return; close deduplication; intentional session-end plus close writes |
| Eligibility | Never-displayed windows; hidden application alone never enrolls; opted-in skipping first `Show` enrolls; direct display without enrollment; late Boolean and id clear/reassign combinations; initially empty id and explicit `false` never enroll later; storage recovery does not require reenrollment |
| Concurrency | Same-slot whole-value replacement under multiple processes; independent application/id slots; read/write races; no partially published snapshot |
| Retention | 32-record cap by successful-save order, not read order; same-id refresh; deterministic legacy ties; cross-process guard; no eviction before a successful write; interrupted/failed cleanup recovery and application isolation |
| Capture caches | Hidden moves and state changes; minimized-from-snapped history; complete overlapped state across presenters; actual-state capture after partial application |
| Shutdown and COM | Message-safe capture using cached desktop identity; no blocking shell query on input-synchronous paths; scheduling gate for closed and never-displayed windows; failed shell query leaves placement saveable; late query completion after close or HWND reuse |
| Diagnostics | Operation/error information present, with identity and placement data absent |

Do not read this requirements table as a coverage report. Historical experiments do not validate
the current format, namespace, deployment, or every lifecycle path. The evidence mapping follows
the test-infrastructure requirements below.

## Automated testing

**Open infrastructure requirement:** We do not currently have a selected, validated solution for
automated multi-monitor and variable-DPI testing. This feature needs both. Synthetic geometry tests
and a single-monitor run cannot establish that native placement works across real topology and DPI
changes. Selecting and proving a repeatable test environment is part of the implementation work.

### Test layers

Use the existing repository test infrastructure where it fits. Separate tests that need only
controlled input data from tests that require real windows, storage, or display configuration:

| Layer | Coverage and approach |
|---|---|
| Codec and identity unit tests | Deterministic record/name inputs, golden data, malformed values, and checked arithmetic; no display changes or real user settings |
| Policy tests with synthetic topology | Inject saved/current monitor descriptions and DPI values at an internal test seam; exercise selection and geometry adjustment where they can be separated from native calls |
| Storage integration tests | Real supported `ApplicationData` access, isolated test identities/ids, cross-process save/load, whole-value concurrency, and injected storage failures |
| Window lifecycle tests | Real HWNDs and WinUI events for enrollment, accepted/cancelled close, hidden capture/application, presenter caches, and destruction backstops |
| Display-environment tests | Actual multi-monitor, mixed-DPI, and changing-topology behavior on a controlled interactive desktop; run native and forced-downlevel placement paths |

Injected topology is not evidence that Windows sends the expected notifications, reports physical
coordinates correctly, restores snapping, or honors visibility and activation restrictions. Keep
those assertions in native integration tests. Internal seams must not become new public placement
or storage APIs.

For persistence round trips, process A creates and changes a test window, then reaches an eligible
save point. Process B loads the saved record and restores a new window. This distinguishes durable
storage from an in-memory cache. Use separate test apps when checking application isolation within
one package, and avoid touching unrelated settings.

Test retention with 31, 32, 33, and already oversized sets of distinct ids. Verify that refreshing an
old slot protects it, reading it does not, and trimming removes exactly the oldest excess records.
Run competing save/cleanup processes against one namespace, and inject failures before replacement,
between replacement and deletion, and partway through deletion. Check recovery after interruption,
lock timeout/abandonment, legacy or corrupt records, unsupported versions, and sequence overflow.
Keep another application's records and unrelated settings present to detect overbroad cleanup.

Exercise confirmed/cancelled session-end handling with controlled integration hooks where possible.
Actual sign-out or shutdown tests require a disposable, dedicated environment. Do not sign out a
shared development or test machine merely to trigger a save.

### Multi-monitor and DPI environment to establish

Evaluate dedicated physical-display lab machines, a controllable virtual/indirect-display setup, and
VM configurations as candidates. No candidate is assumed to support the required topology changes,
independent per-monitor scale, shell behavior, or unattended runs. A remote session's reported
resolution is not proof of multi-monitor or effective-DPI coverage.

The selected environment must make these operations repeatable and observable:

- Establish one or multiple monitors with known bounds, work areas, identities, and effective DPI;
  include monitors left of or above the primary monitor.
- Save and restore with different DPI on different monitors, such as 100%, 150%, and 200% scale.
  Change DPI between runs and move a live window between monitors with different DPI.
- Remove or disable the saved monitor, reconnect it, change the primary monitor, and reorder monitor
  positions between save and restore.
- Change resolution or work area independently from DPI so those adjustments are tested separately.
- Observe configuration completion and actual monitor/DPI values before performing assertions;
  changing resolution or just writing a requested scale value is not proof that DPI changed.

Tie cases to the spec's
[placement scenario inventory](#placement-scenarios-and-winui-decisions),
rather than creating a second source of expected public behavior. Prioritize unchanged-topology
round trips, missing-monitor fallback, same-monitor DPI changes, and mixed-DPI monitor migration,
then combine those cases with normal, maximized, minimized, and snapped placement.

Each run must record its requested and observed display configuration, Windows build, app DPI
awareness, placement backend, and resulting placement. Assert actual normal/snap bounds, restore
state, visibility, and activation, not just a final screenshot. Use event observations for transient
visibility or focus violations. Compare backend-specific expected results or documented invariants,
not an unsupported assumption that all backends produce identical pixels.

This detailed output belongs to controlled tests using generated test data. It does not relax the
[production diagnostic restrictions](#diagnostics-and-implementation-gates).

### Isolation, cleanup, and readiness

Topology and DPI changes affect the desktop, not just the test window. Run them with exclusive
ownership of the test environment; do not parallelize conflicting display tests. Record the initial
configuration and restore it after failures, with recovery or reimaging available if the desktop
becomes inaccessible. Keep the controller able to detect failed configuration changes and clean up
test processes and settings.

Before claiming automated coverage, demonstrate unattended setup, save/restore, observed DPI and
topology changes, failure capture, and cleanup in the chosen environment. Assign infrastructure
ownership and decide which cases run in PR validation versus a dedicated scheduled lane. Missing
display capabilities must produce explicit unsupported/skipped results, not passing tests.

Unit and single-monitor integration tests can proceed while this infrastructure is unresolved.
Manual lab runs can supply interim evidence, but do not close the automated multi-monitor and
variable-DPI gap. Track that gap as a readiness requirement alongside backend implementation.

## Coverage and open gates

This is an evidence index, not a test run or an implementation-complete declaration. All evidence
comes from the preserved original documents and is linked to its
[archived source record](./Window-PlacementPersistence-decisions.md#implementation-history).
**Partial** means a named seam, one-display case, or limited runtime observation exists.
**Open** means the original records leave a design/integration gate unresolved.
**Unknown** means those records do not establish the specific acceptance result.
Historical suite totals are not counts of passing scenarios.

Some records contain updates without separate dates. They are identified as updates within their
dated parent record, not assigned an invented execution date. One capture update is explicitly
dated 2026-09-26, later than this refactor request; its date is preserved as supplied and does not
establish newly verified coverage. No status below relies on it to close S37.

### Scenario-to-evidence mapping

| Requirements | Status | Existing evidence and limit |
|---|---|---|
| S01-S02, S18: real monitor selection and relocation | Partial / Open | The [2026-09-25 implementation-plan record](./Window-PlacementPersistence-decisions.md#what-the-multi-monitorvariable-dpi-lane-still-owns) names `AStaleSavedDisplayNameFallsBackToTheConnectedMonitor` and synthetic selection tests. Actual selection among multiple connected displays and movement between them remain open; one display cannot distinguish the winner. |
| S03-S04, S12, S23: virtual-desktop reason policy and movement | Partial / Open | The [native application record and 2026-09-25 update](./Window-PlacementPersistence-decisions.md#native-application-status-2026-09-24) describe restart suppression after failure/skip, but explicitly leave virtual-desktop moves open. Policy/cached-id tests do not establish actual desktop restoration or deleted-desktop behavior. |
| S05: shell launch-monitor preference | Open | The [2026-09-25 probe record](./Window-PlacementPersistence-decisions.md#launch-monitor-hint-investigation) found no monitor hint on measured launch paths. Packaged and interactive shell paths remain unmeasured; redirected stdout is not a monitor hint. |
| S06, S08, S20-S21: work area, DPI, coordinate spaces, current constraints | Partial | The [display-lane record](./Window-PlacementPersistence-decisions.md#what-the-multi-monitorvariable-dpi-lane-still-owns) names `APlacementSavedAtAHigherDpiRescalesOnTheConnectedMonitor`, `TrackSizesScaleToTheTargetMonitorDpi`, and synthetic work-area cases. The [capture rework](./Window-PlacementPersistence-decisions.md#capture-rework-on-the-placement-engine-2026-09-24) addresses physical coordinates. Real mixed-DPI/topology changes and all taskbar/negative-coordinate combinations remain unestablished. |
| S07, S29-S31, S34-S35: peer selection, cascade, identity, and races | Partial / Open | The [2026-09-24 engine-seam correction](./Window-PlacementPersistence-decisions.md#status-correction-2026-09-24) records `WindowPlacementExplicitCascadeTests`, a 169/169 single-display run, and a later scripted `PeerWalkSkipsOffMonitorCandidatesAndKeepsWalking` update. The [native update](./Window-PlacementPersistence-decisions.md#native-application-status-2026-09-24) reports live markers and foreign-process enumeration. Live two-display Z-order enumeration remains open; these reports do not establish every identity, HWND reuse, or concurrent-opening case. |
| S09, S19: oversized/off-screen/fixed-size fitting | Partial | The [2026-09-24 native record](./Window-PlacementPersistence-decisions.md#native-application-status-2026-09-24) lists current-constraint and malformed/extreme-input application tests. It does not establish all real small-monitor/fixed-size/off-screen combinations or all backend results. |
| S10-S11: saved minimization and ordinary versus restart display | Partial | The [2026-09-25 minimized diagnosis](./Window-PlacementPersistence-decisions.md#native-application-status-2026-09-24) reports `SavedMinimizationRequiresApplicationRestart`, exact restore bounds, zero activations, and 18/18 native plus 18/18 packaged tests. It does not establish every minimized restore target on actual downlevel systems or hidden non-normal application. |
| S13-S14: latest slot, no-data fallback, and initial size | Partial | The [2026-09-25 plan update](./Window-PlacementPersistence-decisions.md#dependency-ordered-implementation-plan) records real cross-process round trips and per-id isolation; the [native record](./Window-PlacementPersistence-decisions.md#native-application-status-2026-09-24) lists single-load/source/fallback coverage. Full disconnect-save-reconnect topology behavior and delivered fallback monitor hints remain unestablished. |
| S15, S25: explicit launcher state merging/replay | Deferred; compatibility Unknown | [Deferred since 2026-09-22](./Window-PlacementPersistence-decisions.md#launcher-show-command-scenarios-deferred). Proposed launcher outcomes are not acceptance gates. The original records do not establish the full existing-native-startup compatibility matrix. |
| S16, S22: snap and minimized-from-snap | Partial / Open | The [2026-09-25 snapped diagnosis](./Window-PlacementPersistence-decisions.md#native-application-status-2026-09-24) records native refusal of new-window `Snapped`, with no shows/activation or source retry. `MinimizedFromSnappedRestoresToSavedSnapBounds` passes exact bounds in an 18/18 run on the named VM. Different work areas, disabled snapping, all capture histories, and actual downlevel paths are not collectively established. |
| S17, S26: live presenters and coherent overlapped history | Partial | The [native record's presenter update](./Window-PlacementPersistence-decisions.md#native-application-status-2026-09-24) names `FullScreenPresenterKeepsAndSavesThePreFullScreenPlacement` for an initially Normal window and explains its skip condition, without a separate passing count. [Capture review fixes](./Window-PlacementPersistence-decisions.md#capture-review-fixes-2026-09-24) report existing presenter integration tests, explicitly not private-cache assertions. These are not guarantees about moving a full-screen window between monitors or S37. |
| S24: repeatable hidden application, skip, and reveal | Partial / Open | [2026-09-25 display routing](./Window-PlacementPersistence-decisions.md#display-routing-through-the-coordinator-2026-09-25) reports ten routing tests and 140/140 placement tests; the [native record](./Window-PlacementPersistence-decisions.md#native-application-status-2026-09-24) covers repeated hidden saved application and skip. Hidden non-normal capability and exact partial-success/fallback boundaries remain open. |
| S27: live display changes do not rerun restore | Unknown | The original records do not identify runtime acceptance evidence for real live topology notifications proving no persistence replay. The [automated lane requirement](#multi-monitor-and-dpi-environment-to-establish) remains applicable. |
| S28: app-selected primary-monitor fallback | Unknown | The [extended example](#prefer-the-primary-monitor-when-the-saved-monitor-is-missing) specifies the workflow. No dated source record establishes the complete saved-present/missing/empty-name/topology-race acceptance matrix. |
| S32: late configuration and retained snapshots | Partial | The [historical acceptance notes](./Window-PlacementPersistence-decisions.md#historical-acceptance-and-diagnostics-notes) name close-time id/geometry, late opt-out, initially empty id, late options, and abort tests. They do not establish every callback race and suspend/resume combination. |
| S33: generated C# and C++/WinRT template startup | Unknown | This remains a [template requirement](#recommended-main-window-code-for-new-projects); the original implementation records do not report template generation/validation results. |
| S36: id-only initial Activate and unchanged legacy behavior | Partial | The [2026-09-25 plan record](./Window-PlacementPersistence-decisions.md#dependency-ordered-implementation-plan) names `LegacyShowDoesNotMoveOrResizeTheWindow` and `LegacyActivateDoesNotMoveOrResizeTheWindow`. [Projection](./Window-PlacementPersistence-decisions.md#public-show-and-hide-projection-2026-09-25-developer) and [host-boundary](./Window-PlacementPersistence-decisions.md#implementation-record-developer-2026-09-24) runs retain the `FirstActivateUsesPersistenceConfiguration` transient failure and passing isolated rerun; do not call those aggregate runs all-green. |
| S37: hidden full-screen reopen with maximized return on B | Open | The [hidden-application rationale](./Window-PlacementPersistence-decisions.md#why-apply-without-showing) identifies the `SW_RESTORE` presenter problem. Normal-state full-screen capture tests do not establish hidden maximized preparation, no flash/activation, target monitor B, and maximized return together. |

### Cross-cutting acceptance evidence

| Requirement | Status | Source-limited evidence |
|---|---|---|
| Codec, base64, and names | Partial | [First unit](./Window-PlacementPersistence-decisions.md#first-implementation-unit), [format review](./Window-PlacementPersistence-decisions.md#store-adapter-formatting-review-2026-09-23), and [codec review](./Window-PlacementPersistence-decisions.md#codec-review-2026-09-23) record data-only tests and review. They do not ratify a shipped durable format or establish storage/native integration. |
| Packaged store, retention, and coherent publication | Partial | [Third unit](./Window-PlacementPersistence-decisions.md#third-implementation-unit) records the 29-test data-only checkpoint and 12/12 store run. [Historical acceptance notes](./Window-PlacementPersistence-decisions.md#historical-acceptance-and-diagnostics-notes) name `ConcurrentSavesPublishOneWholeSnapshot`, `LoadDuringSaveSeesOldOrNewWholeSnapshot`, and the real packaged concurrent-save test. This supports whole-value concurrency, not general power-loss durability or every retention/deployment/lifetime case. |
| Accepted close, destruction, session end, and never-displayed saves | Partial | [Coordinator review](./Window-PlacementPersistence-decisions.md#code-review-of-the-coordinator-production-wiring-2026-09-24) records `FailedCloseSaveIsRetriedByTheDestroyBackstop`; the [plan](./Window-PlacementPersistence-decisions.md#dependency-ordered-implementation-plan) reports production destruction/session hooks. Actual sign-out/shutdown acceptance and the entire failure matrix are not established by hook implementation. |
| Pre-display abort and retry | Partial | [Historical acceptance notes](./Window-PlacementPersistence-decisions.md#historical-acceptance-and-diagnostics-notes) name four coordinator abort tests; [display routing](./Window-PlacementPersistence-decisions.md#display-routing-through-the-coordinator-2026-09-25) includes `AbortedDisplayIsRetriedAsAFirstDisplay`. These are bounded evidence, not a claim that every native display failure is tested. |
| Detached HRESULTs, argument checks, and projections | Partial | [Historical acceptance notes](./Window-PlacementPersistence-decisions.md#historical-acceptance-and-diagnostics-notes) name original-HRESULT diagnostics tests and the ABI empty/null-id case. The [required-options decision](./Window-PlacementPersistence-decisions.md#required-options-decision-2026-09-24) retains additional desktop hidden/null/reentrant coverage as a handoff; do not infer its completion. |
| Public/private Show and Hide host separation | Partial | [2026-09-24 implementation record](./Window-PlacementPersistence-decisions.md#implementation-record-developer-2026-09-24) reports 7/7 host tests and 9/9 desktop tests. Private audio teardown itself is not directly observed; dummy visibility comparisons allow the getter to fail consistently. |
| Diagnostics and optional desktop cache | Partial / Open | [Historical diagnostics notes](./Window-PlacementPersistence-decisions.md#historical-acceptance-and-diagnostics-notes) record operation/category/HRESULT reporting. [Capture review fixes](./Window-PlacementPersistence-decisions.md#capture-review-fixes-2026-09-24) record lifetime safeguards but no DPI-context/allocation fault injection. Actual desktop moves and complete thread/deployment validation remain open. |
| Actual downlevel and multi-monitor/mixed-DPI acceptance | Open | The [native update](./Window-PlacementPersistence-decisions.md#native-application-status-2026-09-24) explicitly leaves these lanes open. Synthetic engine tests and one-display DPI rescaling do not replace the required controlled environment. |
| Unpackaged default storage | Open dependency; outside current automatic-storage scope | [Wanted dependency and 2026-09-23 search](./Window-PlacementPersistence-decisions.md#wanted-dependency-unpackaged-default-applicationdata). `GetForUnpackaged` is not framework discovery of an app-configured process default. |

The feature is not ready to claim all proposed behavior on all supported systems from these records.
Retain the open gates rather than converting a source review, synthetic test, skipped environment,
or historical suite total into a public guarantee.

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
