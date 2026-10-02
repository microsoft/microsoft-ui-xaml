<!--
    Before submitting, delete all "<!-- TEMPLATE marked" comments in this file,
    and the following quote banner:
-->
> See comments in Markdown for how to use this spec template

<!-- TEMPLATE
    The purpose of this spec is to describe new APIs, in a way
    that will transfer to learn.microsoft.com (LMC).

    There are two audiences for the spec. The first are people that want to evaluate and
    give feedback on the API, as part of the submission process.
    When it's complete it will be incorporated into the public documentation at
    http://learn.microsoft.com (LMC).
    Hopefully we'll be able to copy it mostly verbatim. So the second audience is
    everyone that reads there to learn how and why to use this API.
    Some of this text also shows up in Visual Studio Intellisense.

    For example, much of the examples and descriptions in the `RadialGradientBrush` API spec
    (https://github.com/microsoft/microsoft-ui-xaml-specs/blob/master/active/RadialGradientBrush/RadialGradientBrush.md)
    were carried over to the public API page on LMC
    (https://learn.microsoft.com/windows/winui/api/microsoft.ui.xaml.media.radialgradientbrush?view=winui-2.5)

    Once the API is on LMC, that becomes the official copy, and this spec becomes an archive.
    For example if the description is updated, that only needs to happen on LMC and needn't
    be duplicated here.

    Examples:
    * New class (RadialGradientBrush):
      https://github.com/microsoft/microsoft-ui-xaml-specs/blob/master/active/RadialGradientBrush/RadialGradientBrush.md
    * New member on an existing class (UIElement.ProtectedCursor):
      https://github.com/microsoft/microsoft-ui-xaml-specs/blob/master/active/UIElement/ElementCursor.md

    Style guide:
    * Use second person; speak to the developer who will be learning/using this API.
    (For example "you use this to..." rather than "the developer uses this to...")
    * Use hard returns to keep the page width within ~100 columns.
    (Otherwise it's more difficult to leave comments in a GitHub PR.)
    * Talk about an API's behavior, not its implementation.
    (Speak to the developer using this API, not to the team implementing it.)
    * A picture is worth a thousand words.
    * An example is worth a million words.
    * Keep examples realistic but simple; don't add unrelated complications.
    (An example that passes a stream needn't show the process of launching the File-Open dialog.)

-->

Window placement persistence
===

<!-- TEMPLATE
    (Optional)
    
    For longer docs, consider adding section headers with Table of Contents.

    You can use the VS Code extension [Markdown All in One](https://marketplace.visualstudio.com/items?itemName=yzhang.markdown-all-in-one):
    - Install the extension
    - Open .md in VS Code
    - `Ctrl+Shift+P` and select `Markdown All in One: Add/Update section numbers`
    - `Ctrl+Shift+P` and select `Markdown All in One: Create Table of Contents`
-->

# 1. Background

<!-- TEMPLATE
    Use this section to provide background context for the new API(s) 
    in this spec. Try to briefly provide enough information to be able to read
    the rest of the document.

    This section and the appendix are the only sections that likely
    do not get copied to LMC; they're just an aid to reading this spec.

    For example this is a place to provide a brief explanation of some dependent
    area, just explanation enough to understand this new API, rather than telling
    the reader "go read 100 pages of background information posted at ...".

    For example this section is a place to explain why you're adding this new API rather than
    using an existing related API.

    For a simple example see the spec for the UIElement.ProtectedCursor property
    (https://github.com/microsoft/microsoft-ui-xaml-specs/blob/master/active/UIElement/ElementCursor.md)
    which has some of the thinking about how this Xaml API relates to existing
    Composition and WPF APIs. This is interesting background both for the current reader
    and the future reader trying to understand why we designed it this way,
    but not the kind of information
    that would land on LMC.
-->

# 2. Conceptual pages (How To)

_(This is conceptual documentation that will go to learn.microsoft.com "how to" page)_

<!-- TEMPLATE
    (Optional)

    All APIs have a page on LMC, some APIs or groups of APIs have an additional high level,
    conceptual page (called a "how-to" page). This section can be used for that content.

    For example, there are several Xaml controls for different forms of text input,
    each with an API page, and then there's also a conceptual page that
    discusses them collectively
    (https://learn.microsoft.com/windows/uwp/design/controls-and-patterns/text-controls).

    Another way to use this section is as a draft of a blog post that introduces the new feature.

    Sometimes it's difficult to decide if text belongs on a how-to page or an API page.
    It's not important to make a final decision on that in this spec; we can always
    adjust it when copying to LMC.
-->

## 2.1. Scenarios

WinUI remembers where you left a window and adapts its placement to the displays available
when you reopen your app. These scenarios explain what the user sees first, then describe
the APIs and important limits.

The images show the user's displays just before close and when the app next opens. They
illustrate the concept, not exact pixel coordinates. Dashed outlines show remembered window
positions or a disconnected display, not additional open windows.

**Shared API setup**

Set a stable, non-empty `PersistPlacementId` before the window's first display to opt in to
automatic persistence. `PreventAutomaticPlacementPersistence` defaults to `false`; leave it
at that value. Set it to `true` to prevent automatic loading and saving.
The identifier defaults to an empty string, so leaving the Boolean at `false` does not
opt the window in by itself:

```csharp
window.PersistPlacementId = "MainWindow";
window.Show();
```

WinUI attempts to save placement when the window closes and loads it during a later initial
placement operation. Automatic storage requires a packaged desktop app with a usable registered
application identity. In an unpackaged app, you provide storage and supply placement yourself;
WinUI makes the same display adjustments.

Ordinary initial `Show()` and opted-in `Activate()` calls use `WindowShowReason.Default`.
Use `Show(options)` when you need launch or restart policy. The notes below call out those
choices. See [API Pages](#4-api-pages) for the complete API requirements.

### 2.1.1. Bring an off-screen window back into view

If the display layout changes, a remembered window position might fall off-screen. WinUI
brings the window back into view and, when possible, resizes it to fit.

![At close, the app fits the larger display. On reopening on a smaller display, WinUI moves and resizes the window to bring it into view.](images/window-placement-persistence/off-screen.svg)

**API notes**

WinUI makes this adjustment during initial placement, whether it loads placement automatically
or your app supplies `WindowShowOptions.Placement`. It fits normal bounds to the selected
display's work area.

WinUI respects the window's current resizing and minimum-size constraints. A fixed-size
window or a minimum size larger than the work area can prevent a complete fit.

### 2.1.2. Keep a familiar position and size after display scaling changes

When display scaling changes, WinUI keeps the window in approximately the same position and
at the same apparent size from the user's point of view.

![At close and on reopening after display scaling changes, the app window appears in the same relative position and at the same size.](images/window-placement-persistence/dpi-change.svg)

**API notes**

WinUI uses the saved DPI and work area to adjust placement during initial placement. It
adjusts the normal size to preserve approximately the same logical size.

For example, an 800 by 600 physical-pixel window at 96 DPI (100% scale) has a nominal target
size of 1600 by 1200 physical pixels at 192 DPI (200% scale). The image illustrates the user's
view, not a physical-pixel scale drawing.

WinUI adapts position to the target display's geometry and work area rather than simply
multiplying coordinates by the DPI ratio. Work-area limits, current size constraints, rounding,
and window-frame calculations can change the final bounds.

### 2.1.3. Avoid a taskbar whose position has changed

If the taskbar moves or changes size, WinUI adjusts the window so that the taskbar does not
cover it, when the window can fit.

![At close, the taskbar is at the bottom. On reopening, the taskbar moves to the left and WinUI shifts the app to keep it clear of the taskbar.](images/window-placement-persistence/work-area-change.svg)

**API notes**

WinUI makes this adjustment during initial placement. It uses the current work area, which
excludes space that the taskbar and other reserved areas occupy. Your app does not need to
detect the taskbar change itself.

WinUI still respects the window's current size constraints, which can prevent a complete fit.

### 2.1.4. Keep a window on its display after monitors are rearranged

When you rearrange displays or change the primary display, WinUI tries to reopen your app
on the display it used before.

![At close, the app is on display B to the right of display A. On reopening after the monitors are rearranged, the app remains on display B, now on the left.](images/window-placement-persistence/monitor-topology-change.svg)

**API notes**

WinUI first tries to match the saved `DisplayDeviceName`. If it finds a match, it keeps the
window on that display and adapts placement to its current position, work area, and DPI.

The display device name does not permanently identify a physical device. WinUI cannot
guarantee a match. If it finds no match, it chooses an available display as the next section
describes.

### 2.1.5. Choose an available display when a monitor is removed

If you disconnect the display your app used before, WinUI reopens the window on an available
display and adjusts it to fit when possible.

![At close, the app is on external display B. On reopening after B is disconnected, the app moves to the remaining display A.](images/window-placement-persistence/monitor-removed.svg)

**API notes**

If WinUI cannot match the saved display, it chooses the connected display with the largest
intersection with the saved normal rectangle, or the nearest display if none intersects.
It does not always choose the primary display. WinUI then adjusts placement for that display's
work area and DPI.

When WinUI later saves the relocated window successfully, it replaces the previous record.
WinUI keeps one placement per identifier, not a history for each monitor layout. Reconnecting
a monitor does not move a running window back to its old position.

### 2.1.6. Cascade additional instances instead of stacking their title bars

When you open more copies of your app, WinUI can offset their windows so that their title
bars do not sit directly on top of one another.

![At close, one app window has a remembered position. On reopening it and launching more copies, WinUI offsets the windows in a cascade.](images/window-placement-persistence/cascading.svg)

**API notes**

Use the same `PersistPlacementId` to group the windows. For packaged apps, the group also
requires the same application identity. WinUI can use an eligible open window's placement
instead of the saved record.

The default `WindowCascadeBehavior.Automatic` allows cascading when you enable automatic
persistence with a non-empty identifier and supply no explicit placement. The first reopened
window must stay open to serve as a source for later windows. Set `CascadeBehavior` to
`WindowCascadeBehavior.Disabled` to prevent the new window from cascading.

WinUI cannot guarantee non-overlap. Maximized or snapped windows can still overlap, and
simultaneous launches can select the same position.

### 2.1.7. Use the display from which the user launches the app

When you launch the app from a display's taskbar, WinUI can open its window on that display
instead of the display it used before.

![At close, the app is on display B. On reopening from display A's taskbar, the app appears on A when launch information is available.](images/window-placement-persistence/launch-from-taskbar.svg)

**API notes**

Set `Reason = WindowShowReason.Launch` on the initial `Show` request. WinUI then lets a valid
monitor hint from the Windows shell take precedence over the saved monitor.

If the shell supplies no valid hint, WinUI uses the same monitor selection as `Default`.
Calling `Activate()` or opening the first window does not select launch policy.

_Spec note: The scenario notes describe unconditional monitor selection. The current proposal
depends on a valid hint, and we still need to establish whether actual activation paths,
including packaged app launches, supply it. Selecting `Launch` does not guarantee a hint._

### 2.1.8. Open minimized or maximized on request (deferred)

You can launch an app with a request to open minimized or maximized rather than reuse its
remembered state. This scenario shows the requested behavior; the proposal does not yet
specify how WinUI handles it.

![At close, the app is a normal window. The requested reopening outcomes show the app minimized or maximized; the proposal does not yet specify this behavior.](images/window-placement-persistence/launcher-show-command.svg)

**API notes**

Windows startup information can carry requests such as `start /min` or `start /max`. Those
requests differ from the state in saved placement. The proposal does not define how launcher
commands merge with that state. `WindowShowReason.Launch` selects monitor policy, not
launcher-command handling.

_Spec note: We have deferred this scenario, not requested that WinUI suppress existing Windows
startup behavior. We still need to decide how that behavior interacts with saved placement
and non-activating display._

### 2.1.9. Reopen a snapped window after the available space changes

When you close a snapped window, WinUI tries to reopen it in the same snapped position.
If the available space changes, WinUI adjusts the window to that space.

![At close, the app occupies the left half of the available space. On reopening after the taskbar moves, the app follows the left edge of the new available space.](images/window-placement-persistence/snapped.svg)

**API notes**

WinUI saves both `NormalRect` and `SnapRect`. It uses `SnapRect` for the visible snapped
frame and keeps `NormalRect` for restoring the window out of snap.

WinUI adjusts the snapped bounds relative to the current work-area edges, subject to system
snapping support and current window constraints. It restores only this window, not a snap
group or other apps' windows. If the target cannot accept the snapped placement, WinUI uses
normal placement.

### 2.1.10. Reopen a maximized window

When you close a maximized window, WinUI reopens it maximized. If you then restore the window,
WinUI uses its remembered normal size and position.

![At close and on reopening, the app fills the same display. A dashed outline shows the normal size and position that WinUI remembers for restoring the window.](images/window-placement-persistence/maximized.svg)

**API notes**

WinUI saves the maximized state and `NormalRect`, then adapts both to the current display
environment. It keeps the window on the saved display when it can match that display, unless
launch policy selects another one.

WinUI does not discard the normal rectangle when you maximize a window. It uses those bounds
when you restore the window from maximization.

### 2.1.11. Reopen after the app was in full-screen mode

WinUI remembers the windowed placement, not full-screen mode. Your app chooses whether to
enter full-screen again when it reopens.

![At close, the app fills the entire display in full-screen mode. On reopening in a window, the app uses its last windowed placement.](images/window-placement-persistence/full-screen.svg)

**API notes**

Your app selects full-screen or compact-overlay presenters explicitly. WinUI captures and
saves the most recent valid windowed placement, including its normal bounds and maximized
or snapped state. The image uses a window whose last windowed state was normal.

If no valid windowed placement exists, `TryGetPlacement` returns `false` and WinUI skips
automatic saving. If your app selects full-screen or compact-overlay before first display,
WinUI skips initial placement and keeps that presenter. Switching back afterward does not
run initial placement again.

_Spec note: The scenario notes describe restoring only normal bounds. The current proposal
also retains the last valid windowed maximized or snapped state._

### 2.1.12. Return to the previous virtual desktop after an app restart

When you launch your app normally, it opens on the desktop you currently use. When the app
restarts after an update or system restart, WinUI can return its windows to their previous
virtual desktops without switching you away from your current desktop.

![At close, the app is on virtual desktop 1 and you use desktop 2. An ordinary reopening uses desktop 2; an application restart can return the app to desktop 1 while you stay on desktop 2.](images/window-placement-persistence/restart-virtual-desktops.svg)

**API notes**

Use `Reason = WindowShowReason.ApplicationRestart` on each initial `Show` request when your
app reconstructs its windows. WinUI then attempts to restore each saved `VirtualDesktopId`.
`Default` and `Launch` ignore that identifier.

WinUI cannot guarantee virtual-desktop restoration. The identifier can be missing or out of
date. If the saved desktop no longer exists or the move fails, WinUI leaves the window on
its current desktop.

Your app detects restart launches and recreates its windows; WinUI does not do those tasks
automatically.

### 2.1.13. Preserve minimization only for an application restart

When you launch your app normally, WinUI restores a minimized window so you can see it.
When your app restarts after an update or system restart, WinUI can keep the window minimized.

![At close, the app is minimized. An ordinary reopening shows its window; an application restart keeps it minimized.](images/window-placement-persistence/restart-minimized.svg)

**API notes**

Use `Reason = WindowShowReason.ApplicationRestart` to preserve minimization and the recorded
restore target. With `Default` or `Launch`, WinUI instead uses that restore target: normal,
maximized, or snapped when supported.

The image uses a window minimized from normal. This policy concerns saved state, not explicit
launcher requests such as `start /min`.

### 2.1.14. Restart without taking focus

When your app restarts, WinUI can reopen its windows without taking focus from the app you
currently use.

![At close, you use another app in the foreground. On application restart, WinUI reopens the app in the background without requesting focus.](images/window-placement-persistence/restart-activation.svg)

**API notes**

Use `Reason = WindowShowReason.ApplicationRestart` on the initial `Show` request. WinUI
suppresses activation regardless of `DoNotActivate` and disables cascading. Your app can
deliberately call `Activate()` afterward.

If you prepare restart placement while hidden with `TryApplyInitialPlacement`, the later
`Show` does not retain that request's reason. Set both `SkipInitialPlacement = true` and
`DoNotActivate = true` on that `Show` to keep the prepared placement without requesting
activation.

This policy prevents an activation request from the restart operation. It does not lock
foreground focus against other activation requests.

# 3. Examples

_(This is conceptual documentation that will go to learn.microsoft.com "how to" page)_

<!-- TEMPLATE
    (Optional)

    Consider adding examples here if it would help clarify the conceptual pages and/or
    tie together the API Pages.
-->

# 4. API Pages

_(Each of the following L2 sections correspond to a page that will be on learn.microsoft.com)_

<!-- TEMPLATE

  Each of the L2 sections in this "API Pages" section corresponds to a page on LMC.

  It's not necessary to have a section for every class member though:
  * If its purpose and usage is obvious from it's name/type, it's not necessary to
    create a section for it.
  * If its purpose and usage is fully explained by brief description, either
      put it in a table in the "Other [class] members" section
      put it with /// comments in the IDL section

  Create an L2 section here for each API that needs more description or examples.
  For a new class with members, the members should go in their own L2 section.

  Example layout
    ## MyClass
    ## MyClass.Member1
    ## MyClass.Member2
    ## Other MyClass members
    ## MyOtherClass
    ## ...

  Notes:
  * The first line of each of these sections should become that first line on the LMC page,
    which then becomes the description you see in Intellisense.
  * Each page can have description, examples, and remarks.
    Remarks are where the documentation calls out special considerations that the developer
    should be aware of.
  * It can be helpful at the top of an API page (or after the Intellisense text) to add the
    API signature in C#
  * Add a "_Spec note: ..._" to add a note that's useful in this spec but shouldn't go to LMC.
  * Show _examples_, not _samples_; an example is a snippet, a sample is a full working app.

-->

The APIs in this section are in the `Microsoft.UI.Xaml` namespace.

A **placement** records a window's position, size, display environment, and normal, maximized,
or snapped state. It also records whether the window is minimized and which state it should
return to when restored. Visibility, activation, full-screen mode, and compact-overlay mode
are separate from placement. A snapped window occupies a region selected through Windows
snapping, such as one half of a display.

**First display** means the first time the window is shown, whether through `Window.Show`,
`Window.Activate`, `AppWindow`, or a native Windows API. Hiding a window does not make a later
display its first display.

Call the `Window` members on the UI thread that owns the window, before it closes. Calls on
the wrong thread or after close report the corresponding `Window` API error. Methods whose
names begin with `Try` can still report these programming errors.

You can use `WindowPlacement` and `WindowShowOptions` from any thread. Individual property
access is thread-safe, but a series of assignments is not a single transaction. Finish related
edits before passing either object to a method. Each operation takes a consistent snapshot of
each object; later edits do not change that operation. The options object and its referenced
placement are snapshotted separately, not as one combined transaction.

`Show(options)` and `TryApplyInitialPlacement(options)` require non-null options, including
after first display and during nested calls. A null argument reports `E_INVALIDARG` before
thread, closed-window, or window-type checks. In .NET, `E_INVALIDARG` becomes
`ArgumentException`. Other reported errors retain their HRESULT, the error code exposed by
Windows Runtime APIs.

_Spec note: These APIs are proposed. The source proposal identifies unresolved requirements
for hidden placement and launch-monitor hints; the relevant pages below retain those notes._

## 4.1. Window.PersistPlacementId property

Gets or sets the identifier used to save this window's placement and group it with related windows.

The default is an empty string. Assigning null is equivalent to assigning an empty string.
An empty identifier disables automatic placement persistence and cascade grouping for this
window.

To enable automatic persistence, set a non-empty identifier before first display through
`Window.Show` or `Window.Activate` and leave `PreventAutomaticPlacementPersistence` at its default
value of `false`. Leaving only the Boolean at `false` does not enable persistence.

Identifiers are case-sensitive and are not normalized for Unicode. Use a stable identifier
across app sessions. Windows that use the same identifier within the same application's store
share one saved placement record; the last successful save replaces the previous record.

The identifier also defines a **cascade group**. Cascading offsets a new window from an eligible
open window in the group so that their title bars are not directly on top of each other.
For packaged apps, grouped windows must belong to the same application, but can be in different
processes. Unrelated unpackaged apps that use the same identifier can join the same group, so
use an app-specific identifier in an unpackaged app.

Use different identifiers for windows whose placements you want to remember independently.
There are no separate identifiers for storage and cascading.

Setting this property does not move the window, read or write saved data, or change
`PreventAutomaticPlacementPersistence`. You can set the two properties in either order.

## 4.2. Window.PreventAutomaticPlacementPersistence property

Gets or sets whether to prevent automatic loading and saving of this window's placement.

The default is `false`. Setting this property to `true` prevents automatic loading and saving,
but leaves the identifier available for cascading.

Leaving this property at `false` does not enable automatic persistence by itself. You must
also set a non-empty `PersistPlacementId`, which defaults to an empty string.

Setting this property does not move the window, read or write saved data, or change
`PersistPlacementId`.

Automatic storage requires a packaged desktop app with a usable registered application
identity. Apps in the same package have separate placement stores. Saved placement is local
to the current user and computer; it is not synchronized between devices. In an unpackaged
app, you can capture and supply placement, but must provide your own storage to persist it.

### Automatic saving

To enable automatic saving, set a non-empty `PersistPlacementId` and leave
`PreventAutomaticPlacementPersistence` set to `false` before the window's first display through
`Window.Show` or `Window.Activate`. That first display establishes the window's eligibility
for future automatic saves. It does not save placement immediately.

If those properties are not configured at first display, changing them afterward does not
enable automatic saving for that window. Showing a window first through `AppWindow` or a
native Windows API also bypasses this setup; a later `Window.Show` or `Window.Activate` does
not enable automatic saving.

For a window that became eligible at first display, setting this property to `true` or
clearing the identifier suspends future saves. Setting the Boolean back to `false` and
providing a non-empty identifier resumes saving under the current identifier.

WinUI saves placement when a close request is accepted. A canceled close does not save.
WinUI also attempts to save during window destruction and confirmed Windows session shutdown.
Moving, resizing, hiding, and capturing the window do not save placement. A window that has
never been displayed does not save; a previously displayed window can still save after it
has been hidden.

Saved placement is a cache, not permanent application data. These APIs do not report whether
an automatic save succeeded. Do not rely on saving during a crash or forced termination.

## 4.3. Window.TryGetPlacement method

Captures the window's placement in an independent, editable `WindowPlacement` object.

```csharp
public bool TryGetPlacement(out WindowPlacement placement);
```

Returns `true` and sets `placement` to a valid object when capture succeeds. Returns `false`
and sets `placement` to null when capture is unavailable.

For a standard desktop window, capture uses its current geometry and last meaningful placement
state, including while the window is hidden. In full-screen or compact-overlay mode, capture
uses the last complete, valid standard-window snapshot. If no such snapshot exists, the method
returns `false`.

The captured placement does not record full-screen mode, compact-overlay mode, visibility,
or activation. Editing it does not change the window.

Capture does not read or write saved placement, move the window, or enable automatic saving.

## 4.4. Window.TryApplyInitialPlacement method

Applies placement before the window's first display, without showing or activating the window.

```csharp
public bool TryApplyInitialPlacement(WindowShowOptions options);
```

Pass a non-null `WindowShowOptions`. The method uses the same placement selection and reason
policy as an initial `Show(options)` call, but leaves the window hidden and never requests
activation. You can call it more than once before first display. It does not enable automatic
saving or complete the initial-placement phase.

Returns `true` only if an explicit placement, a related window's placement, or saved placement
was selected and successfully applied. Returns `false` in these cases:

| Condition | Result |
|---|---|
| The window has already been displayed. | No initial placement is applied. |
| A placement operation is already running for this window. | The nested request is ignored. |
| No usable placement source is available. | Adjusting fallback geometry alone does not count as success. |
| The window's display mode does not support placement. | No successful placement is reported. |
| Applying the selected placement fails. | Partial placement changes can remain. |

Application is not all-or-nothing. If it fails after making changes, the method does not undo
those changes. The window remains hidden.

A later first `Show()` or `Activate()` runs placement selection again and can replace the
prepared placement. To reveal the resulting placement without selecting it again, pass
`SkipInitialPlacement = true` to `Show`. `Activate()` has no skip option.

For example, given a new `Window` named `window`, this code accepts the resulting placement
even if the hidden attempt returns `false`, then reveals it without requesting activation:

```csharp
window.PersistPlacementId = "MainWindow";
bool applied = window.TryApplyInitialPlacement(new WindowShowOptions());
window.Show(new WindowShowOptions
{
    SkipInitialPlacement = true,
    DoNotActivate = true,
});
```

_Spec note: Applying minimized, maximized, or snapped placement while hidden still requires
supported-version review. The source proposal also leaves the maximized-return behavior
unresolved when hidden preparation is followed by entry into full-screen mode._

## 4.5. Window.Show methods

Shows the window, applying initial placement options if the window has not been displayed before.

```csharp
public void Show();
public void Show(WindowShowOptions options);
```

`Show()` uses a new set of default options. `Show(options)` requires a non-null
`WindowShowOptions`.

### Before first display

Unless `SkipInitialPlacement` is `true`, WinUI selects placement in this order:

1. Use `options.Placement` if it is non-null.
2. Otherwise, use an eligible open window in the same cascade group if cascading is allowed.
3. Otherwise, load saved placement if `PreventAutomaticPlacementPersistence` is `false` and
   `PersistPlacementId` is non-empty.
4. If no usable source is available, show the window without restoring a placement.

The selected placement is adjusted for the current display environment and the request's
`Reason`. Cascading follows `CascadeBehavior`. A non-null `Placement` replaces automatic
source selection; it does not disable future automatic saves.

If applying the selected placement fails, WinUI still attempts to show the window, subject
to the request's activation restrictions. Once application begins, WinUI does not select a
different placement source.

`SkipInitialPlacement = true` shows the window at its current placement without selecting or
applying a placement. Skipping placement does not disable future automatic saves.

First display completes the initial-placement phase, even if no placement was restored or
application failed. Hiding the window does not reopen that phase.

### After first display

Only `DoNotActivate` is read. `Placement`, `Reason`, `CascadeBehavior`, and
`SkipInitialPlacement` are ignored, including invalid values in those fields.

| Current visibility | Behavior |
|---|---|
| Visible, including while minimized | No effect. |
| Hidden | Shows the current placement, including minimization. Requests activation only if the window is not minimized and `DoNotActivate` is `false`. |

Showing a hidden minimized window does not restore it from minimization. Use `Activate()`
when you want to restore a minimized window and request activation.

### Activation and nested calls

`DoNotActivate = true` prevents the call from requesting activation. On first display,
`Reason = ApplicationRestart` also prevents activation, regardless of `DoNotActivate`.
An activation request does not guarantee that Windows will give the window foreground focus.

During an initial placement operation, nested `Show`, `Activate`, and `Hide` calls for that
window are ignored after the required API-use checks. A nested `TryApplyInitialPlacement`
returns `false`. A close request is not ignored; the operation does not continue applying
placement or showing a window that has closed.

`Activate()` differs from `Show`: it can restore a minimized window and request activation.
On first display, when `PreventAutomaticPlacementPersistence` is `false` and
`PersistPlacementId` is non-empty,
`Activate()` also selects initial placement using `Reason = Default`,
`CascadeBehavior = Automatic`, and no explicit placement.

Neither `Show` overload reports whether placement was applied or saved successfully.
Otherwise valid calls on non-desktop or framework dummy windows report `E_NOTIMPL` without
display, placement, persistence, or audio side effects.

## 4.6. Window.Hide method

Hides the window without closing it or saving its placement.

```csharp
public void Hide();
```

The window and its XAML content remain alive. Hiding does not make the window eligible for
initial placement again. If the window is already hidden, the method has no effect.

Use `Show` to reveal the window with its current placement. Use `Activate` to restore it if
minimized and request activation.

Calls made during an initial placement operation are ignored after the required API-use
checks. Otherwise valid calls on non-desktop or framework dummy windows report `E_NOTIMPL`
without display, placement, persistence, or audio side effects.

## 4.7. WindowPlacement class

Represents an editable copy of a window's placement and saved display environment.

The object is independent of any live window or saved record. Editing it does not move a
window or write saved data. To use the edited placement, assign it to
`WindowShowOptions.Placement` and pass the options to an initial placement operation.

Rectangles use physical pixels in the coordinate space spanning all displays. For example,
a display to the left of the primary display can have negative coordinates. These coordinates
are not XAML logical units, client-area bounds, or Win32 workspace coordinates.

`NormalRect` describes the outer window bounds, including the title bar and borders.
`SnapRect` describes the snapped visible frame, excluding invisible resize borders.
`WorkArea` is the saved display's usable rectangle, excluding areas reserved for items such
as the taskbar. `Dpi` records the saved display scale in dots per inch.

Keep the rectangles, work area, and DPI together when storing or constructing placement.
WinUI uses the saved display environment to adjust placement for current displays; restoration
does not guarantee the same physical pixels on a different display.

## 4.8. WindowPlacement constructor

Initializes placement from normal window bounds, a display work area, and DPI.

```csharp
public WindowPlacement(
    Windows.Graphics.RectInt32 normalRect,
    Windows.Graphics.RectInt32 workArea,
    int dpi);
```

The constructor sets these properties:

| Property | Initial value |
|---|---|
| `NormalRect` | `normalRect` |
| `WorkArea` | `workArea` |
| `Dpi` | `dpi` |
| `State` | `WindowPlacementState.Normal` |
| `SnapRect` | null |
| `DisplayDeviceName` | Empty string |
| `VirtualDesktopId` | null |

The constructor validates the required geometry and DPI without inspecting currently
connected displays or moving a window. Invalid arguments report `E_INVALIDARG`.

### Placement validity

Property setters allow temporarily invalid combinations while you edit an object. An eligible
initial placement request validates a complete snapshot before making changes:

| Data | Requirement |
|---|---|
| `NormalRect` and `WorkArea` | Positive width and height. `NormalRect` must have a positive-area intersection with the saved `WorkArea`. |
| `SnapRect`, when present | Positive width and height, even if `State` does not use snapping. |
| Every rectangle | Its right and bottom edges, computed from position and size, must fit in signed 32-bit coordinates. |
| `Dpi` | At least 96. |
| `State` | A defined `WindowPlacementState` value. `Snapped` and `MinimizedFromSnapped` require a non-null `SnapRect`. |
| `DisplayDeviceName` | Empty, or at most 31 UTF-16 code units with no embedded NUL character. |

These checks validate saved data, not whether the saved display or virtual desktop still
exists. In particular, `NormalRect` must intersect the saved work area, not a currently
connected display.

`SnapRect` is not used when applying other states, but a supplied rectangle must still be
valid. Setting `State` to `Normal` requests a normal, non-minimized window without snapping.

## 4.9. WindowPlacement.LoadForPersistPlacementId method

Loads saved placement for the specified identifier without creating or changing a window.

This synchronous method can run on any thread. It returns an independent `WindowPlacement`
object containing the saved data, without adjusting it for current displays.

The method uses the calling application's placement store. It does not select a related
window, apply placement, write saved data, or enable automatic saving.

Null and empty identifiers report `E_INVALIDARG`. This differs from
`Window.PersistPlacementId`, where an empty identifier is valid and disables persistence.

Returns null in these cases:

| Condition |
|---|
| Automatic storage is unavailable or inaccessible. |
| No saved record exists for the identifier. |
| Saved data is invalid or uses an unsupported version. |

Other failures are reported with their original HRESULT. For example, allocation failure
reports `E_OUTOFMEMORY` rather than returning null. .NET and C++/WinRT expose these failures
as exceptions.

Automatic restoration handles storage failures without requiring you to catch them. When
you call this method directly, handle both its null result and its possible exceptions.

Editing the returned placement changes neither a live window nor the saved record.
To use the edited placement, supply it in `WindowShowOptions.Placement`.

## 4.10. Other WindowPlacement members

All of these properties are editable.

| Member | Description |
|---|---|
| `NormalRect` | Outer window bounds for the normal, non-snapped state, in physical pixels. |
| `WorkArea` | Saved display's usable rectangle associated with the saved window geometry, in physical pixels. |
| `Dpi` | Saved display scale, in dots per inch. |
| `State` | Placement and minimization state, including the state to use when restored from minimization. |
| `SnapRect` | Optional snapped visible-frame bounds, in physical pixels, excluding invisible resize borders. |
| `DisplayDeviceName` | Optional GDI display name. It is not a permanent physical-device identifier. |
| `VirtualDesktopId` | Optional identifier for a Windows virtual desktop. |

Null `DisplayDeviceName` is treated as an empty string. `VirtualDesktopId` can be null;
`Guid.Empty` is treated as no virtual-desktop identifier when placement is accepted.

Property setters allow temporarily invalid combinations. The operation that uses the
placement validates the complete value.

## 4.11. WindowShowOptions class

Specifies placement and display choices for one window operation.

Create an options object with its parameterless constructor, then set the properties you
need. The window does not retain the options for later calls.

| Member | Default | Description |
|---|---|---|
| `Placement` | null | Placement supplied by your app instead of automatic source selection. |
| `Reason` | `WindowShowReason.Default` | Policy for ordinary display, launch, or application restart. |
| `CascadeBehavior` | `WindowCascadeBehavior.Automatic` | Whether initial placement can cascade from a related window. |
| `DoNotActivate` | `false` | Prevents this `Show` call from requesting activation when `true`. |
| `SkipInitialPlacement` | `false` | Shows the window at its current placement without selecting or applying initial placement when `true`. |

Null `Placement` means no explicit placement was supplied. It does not disable automatic
restoration. To disable automatic loading and saving, set
`Window.PreventAutomaticPlacementPersistence` to `true`.

`Placement` retains a reference to the assigned object, not a copy of its fields. An operation
snapshots the placement when it uses the options and does not modify either supplied object.

`SkipInitialPlacement` applies only to the first `Show`. It does not disable future automatic
saves. For an eligible initial request, either of these combinations reports `E_INVALIDARG`:

| Invalid combination | Why |
|---|---|
| `SkipInitialPlacement = true` and non-null `Placement` | The request cannot both skip placement and supply placement. |
| `SkipInitialPlacement = true` passed to `TryApplyInitialPlacement` | The hidden operation exists to apply placement, not display the window. |

Invalid placement and undefined `Reason` or `CascadeBehavior` values also report `E_INVALIDARG`
before side effects. Property setters do not validate the enum values. Eligible initial
requests validate them even if the selected policy suppresses the corresponding action.

After first display, `Show(options)` reads only `DoNotActivate`. A late
`TryApplyInitialPlacement` returns `false`. Nested display and placement calls are ignored
as described above. In each case, ignored option fields are not validated, but null options,
wrong-thread calls, and calls after close still report errors.

## 4.12. WindowPlacementState enum

Specifies a window's placement and minimization state, including the state to use when restored.

Each value combines normal, maximized, or snapped placement with whether the window is
minimized. Choose one value. This is not a flags enum; do not combine its values.

| Value | Numeric value | Minimized? | Placement when not minimized |
|---|---|---|---|
| `Normal` | 0 | No | Normal |
| `Maximized` | 1 | No | Maximized |
| `Minimized` | 2 | Yes | Normal |
| `Snapped` | 3 | No | Snapped to `SnapRect` |
| `MinimizedFromMaximized` | 4 | Yes | Maximized |
| `MinimizedFromSnapped` | 5 | Yes | Snapped to `SnapRect` |

`Snapped` and `MinimizedFromSnapped` require a non-null `SnapRect`.
For a placement you construct or edit, the value specifies the requested state and restore
target. It does not describe a sequence of states that the window must have passed through.
The request's `Reason` can change how minimization is applied, and current system capabilities
can require snapped placement to fall back to normal placement.

Visibility, activation, full-screen mode, and compact-overlay mode are not represented by
this enum. Control visibility and activation through display operations and their options.
Select full-screen or compact-overlay mode separately.

## 4.13. WindowShowReason enum

Specifies how an initial placement operation treats placement state and launch information.

For minimized placement, the **restore target** is its recorded normal, maximized, or snapped
state.

| Value | Numeric value | Initial placement policy |
|---|---|---|
| `Default` | 0 | Uses the restore target rather than preserving minimization. |
| `Launch` | 1 | Uses the restore target and prefers a valid launch-monitor hint. |
| `ApplicationRestart` | 2 | Preserves minimization and attempts virtual-desktop restoration. |

`Default` and `Launch` ignore the saved virtual-desktop identifier. Both use the configured
`CascadeBehavior` and `DoNotActivate` settings. `Default` ignores launch-monitor hints.

The launch-monitor hint identifies the display from which the user launched the app. If no
valid hint is available, `Launch` uses the same monitor selection as `Default`. WinUI does
not infer `Launch` from the order in which you create windows or from an `Activate()` call;
set the reason explicitly when you want launch policy.

`ApplicationRestart` attempts to return a window to its saved virtual desktop without
switching the user's active desktop. If the desktop no longer exists or the move fails,
the window stays on its current desktop. The reason does not register your app for restart
or recreate its windows; your app handles those tasks.

`ApplicationRestart` ignores launch-monitor hints and suppresses cascading and activation,
regardless of `CascadeBehavior` and `DoNotActivate`. Minimized windows retain their recorded
restore target.

These policies apply during initial placement, not later `Show` calls. When initial placement
is skipped, only the reason's activation restriction applies. A hidden preparation call
does not carry its reason into a later reveal. To reveal without requesting activation,
set `DoNotActivate = true` on that `Show` call.

The restart reason does not impose a lifetime activation restriction. You can call
`Activate()` afterward to restore a minimized window and request activation.

_Spec note: Availability of launch-monitor hints on actual activation paths, including
packaged activation paths, remains under review. Selecting `Launch` does not guarantee that
a hint will be available._

## 4.14. WindowCascadeBehavior enum

Specifies whether initial placement can offset the window from a related open window.

| Value | Numeric value | Behavior |
|---|---|---|
| `Automatic` | 0 | Allows cascading under the automatic-persistence conditions below. |
| `Enabled` | 1 | Allows cascading even without automatic persistence or with explicit placement. |
| `Disabled` | 2 | Prevents this operation from cascading. |

`Automatic` allows cascading when `PreventAutomaticPlacementPersistence` is `false`,
`PersistPlacementId` is non-empty, and no explicit placement is supplied. Automatic storage
does not have to be available.

Cascading always requires a non-empty `PersistPlacementId` and an eligible related window
in the same group. It does not guarantee that windows never overlap.

With `Enabled` and explicit placement, cascading changes only the position, using a related
window on the selected monitor. It does not replace the supplied placement.

`Disabled` prevents the current operation from cascading, but does not prevent this window
from serving as a placement source for another window.

`Reason = ApplicationRestart` and `SkipInitialPlacement = true` suppress cascading even when
`CascadeBehavior` is `Enabled`.

# 5. API Details

```c# (but really MIDL3)
namespace Microsoft.Name.Space
{
  runtimeclass MyExample
  {
      Int32 PropertyOne;
      String PropertyTwo { get; }
      void MethodOne();

       /// Brief description of the MethodTwo method
      void MethodTwo();
  }
}
```

# 6. Appendix

<!-- TEMPLATE
  Anything else that you want to write down about implementation notes and for posterity,
  but that isn't necessary to understand the purpose and usage of the API.

  This or the Background section are a good place to describe alternative designs
  and why they were rejected.
-->
