Window.MinWidth, MinHeight, MaxWidth, and MaxHeight
===

- [Window.MinWidth, MinHeight, MaxWidth, and MaxHeight](#windowminwidth-minheight-maxwidth-and-maxheight)
- [1. Background](#1-background)
- [2. Conceptual pages (How To)](#2-conceptual-pages-how-to)
  - [2.1. What the constraints measure](#21-what-the-constraints-measure)
  - [2.2. When constraints take effect](#22-when-constraints-take-effect)
  - [2.3. Relationship with AppWindow](#23-relationship-with-appwindow)
  - [2.4. ExtendsContentIntoTitleBar](#24-extendscontentintotitlebar)
  - [2.5. DPI and large values](#25-dpi-and-large-values)
  - [2.6. XAML, errors, and threading](#26-xaml-errors-and-threading)
- [3. Examples](#3-examples)
  - [3.1. Set initial size and constraints](#31-set-initial-size-and-constraints)
  - [3.2. Change or remove constraints](#32-change-or-remove-constraints)
- [4. API Pages](#4-api-pages)
  - [4.1. Window.MinWidth property](#41-windowminwidth-property)
  - [4.2. Window.MinHeight property](#42-windowminheight-property)
  - [4.3. Window.MaxWidth property](#43-windowmaxwidth-property)
  - [4.4. Window.MaxHeight property](#44-windowmaxheight-property)
- [5. API Details](#5-api-details)
- [6. Appendix](#6-appendix)
  - [6.1. Implementation tracking](#61-implementation-tracking)
  - [6.2. Sizing and state transitions](#62-sizing-and-state-transitions)
  - [6.3. Title-bar and frame changes](#63-title-bar-and-frame-changes)
  - [6.4. Platform limits and DPI](#64-platform-limits-and-dpi)
  - [6.5. XAML notes](#65-xaml-notes)
  - [6.6. Related specs and source](#66-related-specs-and-source)
  - [6.7. Runtime observations](#67-runtime-observations)

# 1. Background

_Spec Note: Background for reviewers; not intended for public documentation._

`Window.MinWidth`, `MinHeight`, `MaxWidth`, and `MaxHeight` let apps constrain a window's size
in XAML or code. They complement
[`Window.Width` and `Window.Height`](../../specs/Window/window-width-height-spec.md).
Apps do not need to convert logical pixels to physical pixels or account for the window frame.

WinUI's `Window` uses `AppWindow`, a lower-level windowing API in the Windows App SDK,
accessible through `Window.AppWindow`. AppWindow uses a **presenter** to control how a window
is displayed, such as a normal desktop window, full screen, or compact overlay.
`OverlappedPresenter` handles the normal desktop window and exposes preferred minimum and
maximum sizes. The four Window properties use those constraints.

These properties do not add size-to-content behavior or a custom snap-layout policy.
They follow AppWindow's behavior when applying constraints, including while maximized.

_Spec Note: This document follows the
[API spec template](https://github.com/microsoft/WindowsAppSDK/blob/main/specs/spec_template.md).
The main text describes the contract; the appendix records where implementation or review is pending._

# 2. Conceptual pages (How To)

_Spec Note: Intended for a learn.microsoft.com how-to page._

## 2.1. What the constraints measure

These properties express size limits in logical pixels of the restored client area.
The client area contains the window's content, excluding the non-client title bar and borders.

`Window.AppWindow` exposes the lower-level Windows App SDK windowing API. Its `Presenter`
controls how the window is displayed. A window is in the **restored state** when it uses an
`OverlappedPresenter` (the normal desktop presenter) and is neither minimized nor maximized.

These constraints do not resize the live window while using a non-overlapped presenter,
such as full screen or compact overlay. The values are retained until an overlapped presenter
is active. A maximized window still uses an overlapped presenter: applying a constraint can
resize its live size, matching AppWindow's behavior.
The same outer-window limits apply while maximized; WinUI does not calculate separate
maximized-client limits. Differences in the frame can therefore affect the resulting client size.

Like `Window.Width` and `Height`, these constraints describe the restored client area's width
and height in logical pixels. Unlike WPF's Window constraints, they exclude the non-client frame.
They constrain the window, not individual content elements.

Setting a minimum value to `0` removes the app-requested minimum. Setting a maximum value to
`Double.PositiveInfinity` removes the app-requested maximum. The operating system can still impose limits:
even `MaxWidth = 0` does not guarantee a zero-width window.

If a minimum exceeds its corresponding maximum, **the minimum takes precedence**.
Both properties retain their assigned values.

## 2.2. When constraints take effect

**First shown** means the window has been made visible at least once, not that it has received
focus or become the foreground window. Hiding and showing it again does not reset this state.
Apps currently show a WinUI `Window` by calling `Window.Activate()`.

If you set a constraint before the window is first shown, WinUI stores the value and configures
an overlapped presenter if available. Setting a constraint does not show the window or bring it
to the foreground. Pending initial size requests are constrained when applied. The presenter-state
rules below also apply while hidden.

| Window state | Effect of setting a constraint |
| --- | --- |
| Restored | Apply it and resize the window before the setter returns if its current size is outside the effective limits. Any pending initial `Width` or `Height` request remains pending until the window is first shown. |
| Maximized | Apply it immediately through the overlapped presenter. A maximum smaller than the current size can resize the live window without leaving the maximized state. |
| Minimized | Keep the window minimized and its saved restore bounds unchanged. Apply the limits to its size when restored. |
| Full screen, compact overlay, or another non-overlapped presenter | Store it until an overlapped presenter is active. Do not resize the live window. |
| Closed | Valid assignments succeed without effect. Getters return the values retained at close. |

Maximizing also honors an existing maximum. The window enters the maximized state but may
not fill the available work area.
If your app uses maximum sizes and does not want users to maximize the window, set
`IsMaximizable` to `false` on its `OverlappedPresenter`.

Constraint getters return the assigned values, or defaults if never assigned. User resizing,
pixel rounding, and platform limits do not change those values.

Closing freezes the four constraint values after `Closed` handlers complete without canceling
and immediately before native teardown. Getters retain the latest assigned values, including
changes in those handlers, or defaults for properties never assigned. During `Closed` handlers,
the properties still have their normal open-window behavior. A canceled close does not freeze them.

After close, getters remain available on the owning thread. Valid assignments succeed without
changing retained values, creating pending work, writing presenter constraints, raising
`SizeChanged`, or reopening the window. Input validation and thread affinity still apply.
This does not change the closed-state behavior of other members, such as `Bounds`.

While a `Width` or `Height` request is pending, its getter returns the requested size, even if
it is outside the constraint range. The constraints apply when the request is applied:

```csharp
// Before the window is first shown:
window.Width = 300;
window.MinWidth = 500;
// Width still returns 300. When the window is first shown with an overlapped presenter,
// WinUI sizes it using Width, subject to MinWidth and platform limits.
```

The initial constrained size does not depend on XAML attribute order. Removing a constraint
does not itself undo a live resize. Restoring a minimized or maximized window uses its saved
normal bounds, subject to the remaining limits; this can return it to an earlier size.
Set `Width` or `Height` to request a new size.
If a constraint causes a live resize, the normal `Window.SizeChanged` behavior applies.
Storing a constraint alone does not raise a size event. Constraint application is not atomic:
size handlers can run before the setter returns, and a single update can resize more than once.
There is no guarantee of an exact size-event count or order.

If a handler assigns a newer constraint value, WinUI retains that value. Any further constraint
writes WinUI issues after the handler returns use the current values, not an earlier snapshot.
If a handler closes the window without canceling, WinUI issues no further constraint writes
and does not change the values retained at close. These rules also apply during reapplication
for presenter, DPI, and title-bar changes.

These guarantees apply to WinUI's stored values and the calls it issues. WinUI does not cancel,
undo, or control the completion of an AppWindow operation already in progress when a callback runs.
Reentrant changes therefore do not guarantee a particular final native size or presenter state.

## 2.3. Relationship with AppWindow

WinUI's `Window` builds on `AppWindow`, the Windows App SDK's lower-level windowing API.
`Window.AppWindow` gives you access to that layer for the same window; it is not a separate window.

Window constraints use **client-area logical pixels**. The matching properties on AppWindow's
`OverlappedPresenter`, such as `PreferredMinimumWidth`, use **outer-window physical pixels**,
including the frame.
`AppWindow.ResizeClient` requests client pixels; `AppWindow.Resize` requests outer-window pixels.

**Presenter changes do not update Window constraint getters.** Setting
`OverlappedPresenter.PreferredMinimumWidth` does not change `Window.MinWidth`, even if the actual window resizes.
The same is true of the other three constraints. Native HWND changes do not update these
getters either.

Once you assign a Window constraint, WinUI manages the matching presenter property.
Do not also assign that presenter property directly: WinUI overwrites it when it reapplies
its constraints. While the window is open, reapplication occurs when:

- A valid Window constraint is assigned before close. All constraints managed by WinUI are reapplied.
- An overlapped presenter becomes active, including replacement with another overlapped presenter.
- The DPI changes.
- `ExtendsContentIntoTitleBar` is set while an overlapped presenter is available.

Presenter-switch updates occur through the windowing change notification, not necessarily
before the switch call returns.

Outside these triggers, WinUI does not guarantee that it detects or responds to changes made
directly through AppWindow or native HWND APIs. This includes frame changes made through
`OverlappedPresenter.SetBorderAndTitleBar`. Some changes may trigger recalculation, but apps
must not rely on it. A valid assignment to a Window constraint while the window is open,
even of its current value, requests reapplication
of all constraints managed by WinUI and overwrites their corresponding presenter values.

This behavior applies separately to each property. Setting `MinWidth` leaves never-assigned
`MaxWidth`, `MinHeight`, and `MaxHeight` constraints alone.

| Case | Behavior while the window is open |
| --- | --- |
| Never assign `MinWidth` | WinUI leaves the presenter's minimum width alone. |
| Assign `MinWidth = 500` | WinUI sets and reapplies the corresponding presenter constraint. |
| Then assign `MinWidth = 0` | WinUI clears that presenter constraint. It clears it again whenever you set any of the four Window min/max size properties, even if the app changed the presenter value directly. |

Zero means **no minimum**, not **stop writing to AppWindow**. The same distinction applies
to resetting a maximum to `Double.PositiveInfinity`. Resetting does not restore an earlier
presenter value. DPI changes, overlapped-presenter changes, and setting
`ExtendsContentIntoTitleBar` also cause WinUI to apply the constraints again, as described above.
Constraints are applied when an overlapped presenter is available.
There is no API to make WinUI stop managing a constraint once it has been assigned.

These properties use platform sizing behavior; they are not a guarantee against arbitrary
native manipulation of the HWND.

## 2.4. ExtendsContentIntoTitleBar

When `ExtendsContentIntoTitleBar` is true, the title-bar region becomes part of the client area.
In that case, `MinHeight` and `MaxHeight` include that region, not just the content below your
custom title bar.

Toggling this property leaves the logical-pixel constraints unchanged. WinUI recalculates the
outer-window limits in physical pixels to account for the updated window frame. Extending content
into the title bar normally reduces the outer height needed for the same client-height constraint.

If you explicitly set `Height`, toggling `ExtendsContentIntoTitleBar` preserves the current client
height after the window has been shown, while it is overlapped and restored. For example, with
`MaxHeight = 200` and `Height = 200`, enabling extended content leaves both values at 200 and
the client area 200 logical pixels tall, assuming no larger minimum or platform limit prevents
that size and subject to pixel rounding. The outer window gets shorter;
your custom title bar occupies part of those 200 logical pixels rather than adding to them.
If the user has resized the window since you set `Height`, the toggle preserves that current
height, subject to the constraints. Setting only `Width`, `MinHeight`, or `MaxHeight` does not
enable this preservation.

Before the window is first shown, initial sizing uses the title-bar configuration in effect when the size
request is applied, regardless of assignment order. A toggle while minimized, maximized, or
non-overlapped does not itself preserve restored client height. On return to restored sizing,
the constraints use the current frame.

## 2.5. DPI and large values

WinUI converts logical pixels to physical pixels at the current DPI, adds the restored non-client frame, and
rounds to integer pixels. It recalculates the constraints it manages when DPI changes.
Stored logical-pixel values remain unchanged.

A finite value too large for the presenter's Int32 pixel range is accepted and converted to
`Int32.MaxValue`; the getter still returns the assigned logical-pixel value. The operating system may
impose a smaller limit. Rounding can also affect the resulting size; no fixed logical-pixel tolerance
is guaranteed.

## 2.6. XAML, errors, and threading

Compiled `x:Bind` can assign these properties. Classic `Binding` is not supported.
There are no public dependency-property identifiers or property-change notifications.
Presenter changes and user resizing do not update a binding source through these properties.

Invalid values return `E_INVALIDARG` (`ArgumentException` in .NET) without changing the stored
value. The individual property pages below list accepted values. Input validation and thread
affinity still apply after close.

For a valid assignment before close, WinUI stores the requested value before applying it.
If a presenter write fails, WinUI continues attempting the remaining constraints it manages
while the window remains open, then returns the first error. Failure to obtain a usable
presenter while the window is open can prevent all writes.
Each WinUI invocation records the first failure returned by the calls it makes; a nested
invocation tracks its errors separately.
Stored values, successful presenter writes, and resizes are not rolled back. A getter therefore
reports the latest request even after an application failure, not proof that the presenter accepted it.
A non-overlapped presenter is not an error; application is deferred.

WinUI does not introduce an error solely because the window closed, including when a handler
closes it during application. The interrupted invocation still reports failures returned by
calls it has already issued, including an AppWindow call in progress when closing occurred.
Otherwise it succeeds without issuing further writes. A valid assignment made after close
succeeds without application.

Read and write these properties on the thread that owns the Window; other threads receive
`RPC_E_WRONG_THREAD`.

# 3. Examples

_Spec Note: Intended for examples in the public documentation._

## 3.1. Set initial size and constraints

```xaml
<Window
    x:Class="Contoso.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    Width="800" Height="600"
    MinWidth="480" MinHeight="320"
    MaxWidth="1200" MaxHeight="900">
    <Grid />
</Window>
```

All sizes describe the client area in logical pixels. The frame is additional.
Omit `MaxWidth` or `MaxHeight` from the XAML if you do not want to set that maximum.

## 3.2. Change or remove constraints

```csharp
window.ExtendsContentIntoTitleBar = true;
window.MinHeight = 320; // Includes the custom title-bar region.

window.MinWidth = 0;
window.MaxWidth = double.PositiveInfinity;
```

The last two assignments remove the width limits. WinUI continues to clear the matching
presenter constraints whenever it reapplies its values.
Assignments are separate operations, not an atomic size-range update.

# 4. API Pages

_Spec Note: Each subsection corresponds to a learn.microsoft.com property page.
Shared behavior is documented above rather than repeated on all four pages._

All four properties retain their getter values after close and ignore valid post-close assignments.
See [When constraints take effect](#22-when-constraints-take-effect) and
[XAML, errors, and threading](#26-xaml-errors-and-threading).

## 4.1. Window.MinWidth property

Gets or sets the minimum width of the window's restored client area, in logical pixels.

```csharp
public double MinWidth { get; set; }
```

Accepts finite values greater than or equal to zero. The default is `0` (no app-requested minimum).
Negative values, `NaN`, and either infinity are invalid.

## 4.2. Window.MinHeight property

Gets or sets the minimum height of the window's restored client area, in logical pixels.

```csharp
public double MinHeight { get; set; }
```

Accepts finite values greater than or equal to zero. The default is `0` (no app-requested minimum).
Negative values, `NaN`, and either infinity are invalid.

## 4.3. Window.MaxWidth property

Gets or sets the maximum width of the window's restored client area, in logical pixels.

```csharp
public double MaxWidth { get; set; }
```

Accepts finite values greater than or equal to zero, or `Double.PositiveInfinity`.
The default is `Double.PositiveInfinity` (no app-requested maximum).
Negative values, `NaN`, and negative infinity are invalid.

## 4.4. Window.MaxHeight property

Gets or sets the maximum height of the window's restored client area, in logical pixels.

```csharp
public double MaxHeight { get; set; }
```

Accepts finite values greater than or equal to zero, or `Double.PositiveInfinity`.
The default is `Double.PositiveInfinity` (no app-requested maximum).
Negative values, `NaN`, and negative infinity are invalid.

# 5. API Details

_Spec Note: API declarations for review; public reference pages document these members individually._

```cs (but actually idl)
namespace Microsoft.UI.Xaml
{
    [webhosthidden]
    [contentproperty("Content")]
    unsealed runtimeclass Window
    {
      // (existing APIs...)
      [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
      {
        // (existing APIs...)

        // New APIs:
        Double MinWidth;
        Double MinHeight;
        Double MaxWidth;
        Double MaxHeight;
      }
    };
}
```

# 6. Appendix

_Spec Note: Review notes, not public documentation. These APIs remain experimental; no stable
release is assigned._

## 6.1. Implementation tracking

Known implementation gaps, regression coverage, and release follow-ups are tracked in
[#11965: Update Window MinMaxSize to match latest spec](https://github.com/microsoft/microsoft-ui-xaml/issues/11965).
The experimental implementation does not yet establish every behavior specified here.
The issue tracks the work; the latest version of this spec, whether in an open PR or merged,
defines the intended contract and takes precedence over the issue's checklist.

## 6.2. Sizing and state transitions

Setting a constraint on an active overlapped presenter applies the size restriction synchronously.
The restored-window resize does not require a separate call to `AppWindow.Resize`.

Presenter-level probes confirm synchronous constraint assignment, same-value assignment,
equal limits, and relaxation without restoring an old size. See [Runtime observations](#67-runtime-observations).

WinUI follows AppWindow's constraint-assignment behavior while maximized. A manual check
confirmed that setting `OverlappedPresenter.PreferredMaximumWidth = 300` resizes the maximized
window while retaining its maximized state. There is no separate WinUI rule to suppress that resize.
Another manual check confirmed that maximizing with a maximum already set honors the limit:
the window enters the maximized state without filling the available work area.

Conversion uses restored-frame measurements even while maximized; presenter-only probes
do not establish exact maximized client limits.

Presenter assignments while minimized leave the window minimized and its saved normal bounds
unchanged. Restoration applies the constraints. Assignments while maximized can resize the
live window but also leave saved normal bounds unchanged until restore.

Pending Width/Height requests remain unchanged until applied, matching the companion spec.
This does not make Width/Height permanent records of the app's preferred size: once the
request is applied, their normal restored-size getter behavior resumes.

Keep the current presenter-change order: apply pending Width/Height requests, then constraints.
Either order can cause two resizes. On successful application while the window remains open,
and without reentrant changes, the final size is constrained by the applied values, subject to
platform limits. A single resize or an exact event count is not guaranteed.

## 6.3. Title-bar and frame changes

Agreed behavior: after an `ExtendsContentIntoTitleBar` change while minimized, maximized, or non-overlapped,
the constraints use the current frame when restored. Their logical-pixel values stay unchanged.
This does not preserve the exact previous client height across the non-restored toggle.

Direct AppWindow or native frame changes have no guaranteed automatic refresh behavior.
This is an intentional contract boundary.

## 6.4. Platform limits and DPI

Presenter geometry probes confirm minimum-over-maximum precedence in both assignment orders,
equal limits, and unchanged size when a limit is relaxed.

A zero presenter maximum is ignored; this is not necessarily equivalent to a zero Window maximum
because WinUI adds the non-client frame.

Presenter probes confirmed minimum-width enforcement through `AppWindow.Resize`, `ResizeClient`,
and `SetWindowPos`. `IsResizable=false` did not prevent constraint-driven resizing.

Current DPI handling clears owned constraints before resizing and reapplies them afterward.
Pixel conversion saturates at
`INT_MAX`; zero minima and infinite maxima clear the presenter property instead.

## 6.5. XAML notes

Positive-infinity spellings in XAML are out of scope for now. The documented guidance remains
to omit a maximum or clear it in code. Private dependency-property metadata supports parsing
but is not a public dependency-property contract.

## 6.6. Related specs and source

The declaration above uses contract 12 provisionally; it does not assign these experimental
APIs to a stable release.

The [Width/Height spec](../../specs/Window/window-width-height-spec.md) uses the same first-shown
sizing boundary. This describes visibility, not foreground activation, and does not propose
a new showing API. [#11240](https://github.com/microsoft/microsoft-ui-xaml/pull/11240) covers
related showing and activation behavior.

The implementation is in [DesktopWindowImpl.cpp](../../dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp).
Its [header](../../dxaml/xcp/dxaml/lib/DesktopWindowImpl.h) stores each constraint independently
as an optional double: absent means never assigned.
[Window_Partial.cpp](../../dxaml/xcp/dxaml/lib/Window_Partial.cpp) returns `E_NOTIMPL` when
the feature is disabled.

[WindowIntegrationTests.cpp](../../dxaml/test/native/external/controls/window/WindowIntegrationTests.cpp)
already covers defaults, invalid inputs, markup values, presenter swaps, AppWindow.Resize
clamping, untouched unassigned constraints, and a restored standard-to-extended title-bar toggle.
It does not establish every behavior above; the remaining coverage is tracked in #11965.

## 6.7. Runtime observations

The investigation covered 22 presenter-level cases on each of stock Windows App SDK 2.5.1 and 2.3.1,
on x64 Windows at 96 DPI. Both runs agreed. Measurements were taken immediately
after each operation and after processing messages; geometry and state matched at both times.
Sizes below are outer-window physical pixels, not Window client-area logical pixels.
All assignments and getters in this table are presenter operations; clearing means assigning
`null`, not zero.

| Case, starting from saved normal bounds of 500 by 400 | Result |
| --- | --- |
| Minimize, set minimum width 700 | Stays minimized; saved bounds remain 500 by 400. Restores to 700 by 400. |
| Minimize, set maximum width 300 | Stays minimized; saved bounds remain 500 by 400. Restores to 300 by 400. |
| Set height constraints while minimized | Same behavior on the height axis; the other axis is preserved. |
| Set then clear a minimum while minimized | Restores to the original size. |
| Maximize, set maximum width 300 | Live width becomes 300; state remains maximized; saved bounds stay unchanged until restore. |
| Clear that maximum before restoring | Live maximized width stays 300; restoring returns to the original 500 by 400. |
| Minimum width 700 and maximum width 300 | Width is 700 in either assignment order; both constraint getters retain their values. |

_Spec Note: Neither stock projection exposed the experimental Window
sizing properties. Window getter behavior, ownership, title-bar conversion, closed-state behavior,
bindings, and cross-monitor DPI behavior remain unverified by these runs._
