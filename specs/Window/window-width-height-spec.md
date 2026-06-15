Window.Width and Window.Height
===

# Background

_(This section will not be part of public docs)_

The WinUI 3 [Window](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window)
class is the host for XAML content in a desktop app. Until now, the only way to
change a Window's size from code has been to drop down to the Win32 HWND (via
`IWindowNative`) and call `SetWindowPos`, or to go through `AppWindow.Resize(...)`,
which works in physical pixels.

App developers keep asking for a more natural managed way to do this, the same
way they have it in WPF. This spec adds two new properties to
`Microsoft.UI.Xaml.Window`:

- `Window.Width` (Double, in DIPs)
- `Window.Height` (Double, in DIPs)

These mirror what people expect from
[`System.Windows.Window.Width`](https://learn.microsoft.com/dotnet/api/system.windows.window.width)
and `System.Windows.Window.Height` in WPF, with a couple of important
differences (see the WPF compare/contrast section below).

These APIs ship as **experimental** first (`[feature(Feature_ExperimentalApi)]`),
so the behavior may change based on early adopter feedback before they get
promoted to stable.

# Conceptual pages (How To)

_(This is conceptual documentation that will go to docs.microsoft.com "how to" page)_

WinUI Window already has a read-only
[`Bounds`](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window.bounds)
property that returns the **client area** of the window in DIPs. The 
`Width` and `Height` properties round out that surface:

- The getters return the **client area** width and height in DIPs. In the
  Normal, Fullscreen, and CompactOverlay states this matches `Bounds.Width` /
  `Bounds.Height`. In the Maximized or Minimized states the getter returns the
  **restore** size, the size the window will have when it goes back to Normal, so
  that `Width`/`Height` round-trip with the setter in those states. (Fullscreen
  and CompactOverlay are the states where they do not round-trip, because the
  setter is a no-op there; see below.)
- The setters change the **client area** size in DIPs.

```csharp
// Code-behind in a packaged WinUI desktop app
public MainWindow()
{
    this.InitializeComponent();

    // 800 x 600 DIP client area, no matter the monitor DPI.
    this.Width  = 800;
    this.Height = 600;
}
```

You set `Width` and `Height` from code (C# or C++/WinRT), the same way you set
`Title` or `ExtendsContentIntoTitleBar`. They are not declarable in XAML markup
on `<Window>`.

## Three sets of APIs size the same window: `Window`, `AppWindow`, and Win32

Every WinUI desktop window is, underneath, a single Win32 `HWND`. Three different
API layers can read and size that same window, and they do **not** all agree on
units or on what they measure. Here is the whole picture in one place.

The three layers:

- **`Microsoft.UI.Xaml.Window`** (this class, the XAML window) works in **DIPs**.
  `Bounds`, and now `Width` and `Height`, are all DIPs. 
- **`Microsoft.UI.Windowing.AppWindow`** (you get it from `Window.AppWindow`) and
  its presenters work in **physical pixels** (they take and return `SizeInt32`).
- **Raw Win32** on the `HWND` (get the handle from
  `WinRT.Interop.WindowNative.GetWindowHandle`, or `IWindowNative::get_WindowHandle`
  in C++) also works in **physical pixels**. These are the same functions any
  classic Win32 app uses, and can also be used to size the window.

Because two of the three layers are in physical pixels and one is in DIPs,
`Window.Width = 800` (DIPs) and `AppWindow.ResizeClient(new SizeInt32(800, 600))`
(pixels) are **not** the same on a scaled display. Convert with the window's DPI:

```
physicalPixels = dips * GetDpiForWindow(hwnd) / 96.0
```

Here is every sizing API across the three layers, side by side:

| API                                         | Layer         | Unit         | Measures                  | Read / write    | Notes                              |
| ------------------------------------------- | ------------- | ------------ | ------------------------- | --------------- | ---------------------------------- |
| `Window.Width` / `Height`                   | `Xaml.Window` | DIPs         | **Client** area           | get **and** set | Restore size when Max/Min          |
| `Window.Bounds`                             | `Xaml.Window` | DIPs         | **Client** area           | get only        | Always the live size               |
| `AppWindow.ClientSize`                      | `AppWindow`   | Physical px  | **Client** area           | get only        | Always the live size               |
| `AppWindow.Size`                            | `AppWindow`   | Physical px  | **Window** rect           | get only        | Always the live size               |
| `AppWindow.ResizeClient(sz)`                | `AppWindow`   | Physical px  | **Client** area           | set (method)    | Acts on the live window            |
| `AppWindow.Resize(sz)`                      | `AppWindow`   | Physical px  | **Window** rect           | set (method)    | Acts on the live window            |
| `GetClientRect(hwnd, &r)`                   | Win32         | Physical px  | **Client** area           | get only        | Live size; origin is (0,0)         |
| `GetWindowRect(hwnd, &r)`                   | Win32         | Physical px  | **Window** rect           | get only        | Live size; screen coordinates      |
| `SetWindowPos` / `MoveWindow`               | Win32         | Physical px  | **Window** rect           | set             | Acts on the live window            |
| `GetWindowPlacement` / `SetWindowPlacement` | Win32         | Physical px  | **Window** rect (normal/min/max rects) | get **and** set | Reads/writes the restore rect |

A few things to notice:

- **Unit.** `Xaml.Window` is DIPs. `AppWindow` and raw Win32 are physical pixels.
- **Client vs window rect.** `Window.Width/Height`, `Window.Bounds`,
  `AppWindow.ClientSize`, and `GetClientRect` measure the **client** area (where
  your XAML content lives). `AppWindow.Size` / `AppWindow.Resize(...)`,
  `GetWindowRect`, and `SetWindowPos` / `MoveWindow` measure the **window rect**
  (caption and borders included).
- **Live vs restore.** Almost every API reports the **live** window. The two that
  deal in the **restore** size are `Window.Width/Height` (in the Max/Min states)
  and Win32 `Get/SetWindowPlacement` (its `rcNormalPosition`). That is not a
  coincidence: the `Width/Height` getter and setter are built on exactly that
  placement rect.
- **Read vs write.** `Window.Width/Height` is the only single member you both read
  and write. `AppWindow` splits it (a get-only property plus a separate method),
  and Win32 splits it across different functions too.

Rule of thumb: to size the **client** area in **DIPs**, set `Window.Width/Height`.
For **physical pixels**, use `AppWindow.ResizeClient(...)` (client) or
`AppWindow.Resize(...)` (the window rect). Reach for raw Win32 (`SetWindowPos` and
friends) only when you need something the WinUI surfaces do not expose; mixing it
with the XAML window means you own the DIP/pixel conversion yourself.

### How the AppWindow presenter affects sizing

The same sizing APIs behave differently depending on which `AppWindowPresenter`
the window is using. You pick a presenter with `AppWindow.SetPresenter(...)`.
The dividing line is simple: **the default `Overlapped` presenter honors the
setter; any non-default presenter (`FullScreen`, `CompactOverlay`) makes the
setter a no-op.** The `Overlapped` presenter also has three show states
(Restored, Maximized, Minimized) that each behave on their own. Here is what the
`Window.Width/Height` setter and getter do in each:

| Presenter (`AppWindowPresenterKind`) | What it is                                   | `Window.Width/Height` **setter**                       | `Window.Width/Height` **getter** returns |
| ------------------------------------ | -------------------------------------------- | ------------------------------------------------------ | ---------------------------------------- |
| `Overlapped` - Restored (Normal)     | Standard window: caption + borders, resizable | Resizes the **live** window now                       | Live client size                         |
| `Overlapped` - Maximized             | Fills the monitor work area                  | Updates the **restore** size only; live window unchanged | Restore size (not the live maximized size) |
| `Overlapped` - Minimized             | Sits in the taskbar                          | Updates the **restore** size only; window stays minimized | Restore size                          |
| `FullScreen`                         | Borderless, covers the whole monitor         | **No-op** (does not resize)                            | Live fullscreen size                     |
| `CompactOverlay`                     | Small always-on-top picture-in-picture window | **No-op** (does not resize)                           | Live client size                         |

Notes:

- **`Overlapped`** is the default presenter and the only one that honors the
  setter. It is also the only one with multiple show states. `Restored` is the
  "Normal" case where `Width == Bounds.Width`. Maximized and Minimized are where
  the setter/getter switch to the *restore* size instead of the live size.
- **`FullScreen` and `CompactOverlay`** are the non-default presenters. Today the
  setter is a silent **no-op** for both, and the getter reports the live size, so
  `Width`/`Height` does not round-trip while you are in one of these modes. This
  is still under discussion; see the open question in the appendix.

For the two non-default presenters, this no-op behavior is close to, but not exactly, what
the lower-level `AppWindow.ResizeClient(...)` already does:

| Presenter        | `Window.Width/Height` setter | `AppWindow.ResizeClient(...)`                                  |
| ---------------- | ---------------------------- | -------------------------------------------------------------- |
| `FullScreen`     | No-op                        | No-op (the presenter owns the full-monitor bounds)           |
| `CompactOverlay` | No-op                        | May resize, but **clamped** to the presenter's allowed min/max range |

So in `FullScreen` the two surfaces agree (both do nothing). In `CompactOverlay`
they differ: `Window.Width/Height` does nothing at all, while `ResizeClient` can
still change the size within the presenter's limits. We chose the stricter no-op
for `Width`/`Height` to keep one simple rule ("non-default presenter = no-op").
If you actually need to resize a compact-overlay window, drop down to
`AppWindow.ResizeClient(...)` (physical pixels).

## Where Width/Height equals Bounds and where it does not

`Window.Bounds` always reports the **live** client area. `Width`/`Height`
diverge from it in the Maximized and Minimized states because the getters
return the restore size instead:

| Window state | `Width`/`Height` returns | `Bounds.Width`/`Bounds.Height` returns | Same? |
| ------------ | ------------------------ | -------------------------------------- | ----- |
| Normal       | Live client size         | Live client size                       | Yes   |
| Maximized    | **Restore** size         | Live maximized size                    | **No** |
| Minimized    | **Restore** size         | Live minimized size                    | **No** |
| Fullscreen   | Live fullscreen size     | Live fullscreen size                   | Yes   |
| CompactOverlay | Live client size       | Live client size                       | Yes   |

In short: the getter equals `Bounds` in Normal, Fullscreen, and CompactOverlay,
but for different reasons. In **Normal** the setter actually drove the window to
that size. In **Fullscreen** and **CompactOverlay** the setter is a no-op, so the
getter just reflects whatever live size the presenter chose. In the **Maximized**
and **Minimized** states the getter instead returns the *restore* size, so
`Width`/`Height` tell you what the window *will be* when restored while `Bounds`
tells you what it *is right now*.

If you want to size by the window rect instead, you can still call
`AppWindow.Resize(...)` (physical pixels, not DIPs; see the three-layers section above).

## WPF Comparison

WPF developers work with the WPF `Window.Width` and `Window.Height` properties. The
WinUI properties are meant to feel familiar but they are **not identical**.

A WPF app that does `this.Width = 800` makes a window whose **window rect**
is 800 DIPs wide, so the usable client area inside is something like ~784 DIPs
after chrome. The same line in WinUI gives a **client area** of exactly 800
DIPs, so the window rect is ~816 DIPs. Both behaviors are self-consistent; the
cross-framework number just does not carry over 1:1.

Here is a side-by-side breakdown of more WPF vs WinUI behavior:

| Aspect                    | WPF `Window.Width/Height`                            | WinUI `Window.Width/Height` (this spec)         |
| ------------------------- | ---------------------------------------------------- | ----------------------------------------------- |
| Unit                      | DIPs (1/96 inch)                                     | DIPs (1/96 inch), same                          |
| What it measures          | **Window rect** (includes chrome)                    | **Client area** (matches `Window.Bounds`)       |
| Default / initial value   | `Double.NaN` (auto-size to content)                  | The current actual client size (never NaN)      |
| Setting `NaN`             | Allowed; means "size to content"                     | **Throws `E_INVALIDARG`**. No size-to-content.  |
| Setting negative          | Throws `ArgumentException`                           | Throws `E_INVALIDARG`                           |
| Setting `Infinity`        | Throws `ArgumentException`                           | Throws `E_INVALIDARG`                           |
| `MinWidth` / `MaxWidth`   | Exists and is enforced                               | Not part of this spec (may follow later)        |
| Behavior when Maximized   | Updates *restore* size; window stays maximized       | Updates *restore* size; window stays maximized  |
| Behavior when Minimized   | Updates *restore* size; window stays minimized       | Updates *restore* size; window stays minimized  |
| Behavior in non-default presenter | WPF has no built-in fullscreen/PiP modes        | **No-op** in `FullScreen` and `CompactOverlay` (live window unchanged; subject to change) |
| Dependency property       | Yes; bindable                                        | Plain WinRT property; not bindable              |

# API Pages

_(Each of the following L2 sections correspond to a page that will be on docs.microsoft.com)_

> **Editor note:** The `Window.Width` and `Window.Height` sections are
> intentionally near-identical so each can stand alone as its own docs page. If
> you change behavior in one, make the matching change in the other.

## Window.Width property

Gets or sets the width of the Window's **client area** in device-independent
pixels (DIPs, 1/96 inch).

```csharp
public double Width { get; set; }
```

**Getter**: returns the current client-area width in DIPs. In the Normal,
Fullscreen, and CompactOverlay states this equals `Window.Bounds.Width`. In the
Maximized or Minimized state the getter returns the **restore** width, the width
the window will have when it goes back to Normal, so the getter round-trips with
the setter in those states.

**Setter**: changes the window so its client area is `value` DIPs wide. The
window's non-client chrome (caption, borders, and so on) gets added on top of
`value` to work out the window rect, using the current per-monitor
DPI. Height is left unchanged.

Throws `E_INVALIDARG` for negative, `NaN`, or `Infinity` values.

### Behavior by window state

These behaviors split by presenter. The default `Overlapped` presenter honors
the setter and maps to Normal / Maximized / Minimized based on its show state.
The non-default presenters, `FullScreen` and `CompactOverlay`, both make the
setter a **no-op**.

- **Normal**: the window resizes right away to the requested client width.
  Position is preserved.
- **Maximized**: the live (maximized) window does not resize. The *restore*
  bounds (the size the window snaps to when un-maximized) get updated. The other
  axis (Height) of the restore bounds is preserved.
- **Minimized**: same as Maximized. The *restore* bounds get updated and the
  window stays minimized (it snaps to the new size when restored). The other
  axis of the restore bounds is preserved.
- **Fullscreen** (via `AppWindowPresenterKind.FullScreen`): the call is a
  silent **no-op**. The live window stays fullscreen and the getter returns the
  live fullscreen size, so it does not round-trip in this mode.
- **CompactOverlay** (picture-in-picture, via
  `AppWindowPresenterKind.CompactOverlay`): same as Fullscreen, the setter is a
  silent **no-op** and the getter returns the live size.

### Remarks

**Units and DPI.** The setter converts the requested DIP value to physical
pixels using the window's current per-monitor DPI:

```
physicalPixels = round(dips * GetDpiForWindow(hwnd) / 96.0)
```

Because of integer-pixel rounding, the value you read back from the getter may
differ from the value you passed to the setter by up to a couple of DIPs. The
scenario tests allow a 2-DIP tolerance when comparing.

**DPI changes during the window's lifetime.** If the window moves to a monitor
with a different scale factor after you call the setter, the OS re-scales the
window per its normal rules. The client-area size in DIPs is preserved across
the move.

**Interaction with ExtendsContentIntoTitleBar.** Because `Width`/`Height` measure
the client area, toggling `Window.ExtendsContentIntoTitleBar` changes what counts
as client area, so the getter (and `Bounds`) can change even though the window
rect did not; re-apply the setter if you need a specific size after toggling.

**XAML markup.** Width and Height are settable from C# / C++/WinRT code-behind
only. They are **not** surfaced in XAML markup on `<Window>`, so there is no
markup attribute to set (and therefore no markup-vs-code mismatch to trip over).
XAML markup support could be added later if there is demand and a sensible
parsing order, that is, Width/Height applied after Activate.

**Bindings.** Width and Height are not `DependencyProperty`-backed. They do not
take part in data binding and do not raise change notifications. This matches the
rest of `Window`'s mutable surface (Title, ExtendsContentIntoTitleBar).

**Threading.** You must set these properties on the thread that owns the Window
(the dispatcher thread). Calls from other threads return the standard
`RPC_E_WRONG_THREAD` error from the XAML framework.

### Errors

| Input                         | Result                              |
| ----------------------------- | ----------------------------------- |
| `value < 0`                   | Throws `E_INVALIDARG`               |
| `Double.NaN`                  | Throws `E_INVALIDARG`               |
| `Double.PositiveInfinity`     | Throws `E_INVALIDARG`               |
| `Double.NegativeInfinity`     | Throws `E_INVALIDARG`               |
| Window in a non-default presenter | Silent no-op (the call succeeds; the live window is unchanged) when the window uses the `FullScreen` or `CompactOverlay` presenter. Subject to change in a future release. |
| `0`                           | Allowed. The window resizes so the client area has zero width. The OS may apply a minimum size. |
| Very large value (> screen)   | Allowed. The OS clamps to its tracking-size maximum; the window grows as large as the OS allows. |

## Window.Height property

Gets or sets the height of the Window's **client area** in device-independent
pixels (DIPs, 1/96 inch).

```csharp
public double Height { get; set; }
```

**Getter**: returns the current client-area height in DIPs. In the Normal,
Fullscreen, and CompactOverlay states this equals `Window.Bounds.Height`. In the
Maximized or Minimized state the getter returns the **restore** height, the height
the window will have when it goes back to Normal, so the getter round-trips with
the setter in those states.

**Setter**: changes the window so its client area is `value` DIPs tall. The
window's non-client chrome (caption, borders, and so on) gets added on top of
`value` to work out the window rect, using the current per-monitor DPI. Width is
left unchanged.

Throws `E_INVALIDARG` for negative, `NaN`, or `Infinity` values.

### Behavior by window state

These behaviors split by presenter. The default `Overlapped` presenter honors
the setter and maps to Normal / Maximized / Minimized based on its show state.
The non-default presenters, `FullScreen` and `CompactOverlay`, both make the
setter a **no-op**.

- **Normal**: the window resizes right away to the requested client height.
  Position is preserved.
- **Maximized**: the live (maximized) window does not resize. The *restore*
  bounds (the size the window snaps to when un-maximized) get updated. The other
  axis (Width) of the restore bounds is preserved.
- **Minimized**: same as Maximized. The *restore* bounds get updated and the
  window stays minimized (it snaps to the new size when restored). The other
  axis of the restore bounds is preserved.
- **Fullscreen** (via `AppWindowPresenterKind.FullScreen`): the call is a
  silent **no-op**. The live window stays fullscreen and the getter returns the
  live fullscreen size, so it does not round-trip in this mode.
- **CompactOverlay** (picture-in-picture, via
  `AppWindowPresenterKind.CompactOverlay`): same as Fullscreen, the setter is a
  silent **no-op** and the getter returns the live size.

### Remarks

**Units and DPI.** The setter converts the requested DIP value to physical
pixels using the window's current per-monitor DPI:

```
physicalPixels = round(dips * GetDpiForWindow(hwnd) / 96.0)
```

Because of integer-pixel rounding, the value you read back from the getter may
differ from the value you passed to the setter by up to a couple of DIPs. The
scenario tests allow a 2-DIP tolerance when comparing.

**DPI changes during the window's lifetime.** If the window moves to a monitor
with a different scale factor after you call the setter, the OS re-scales the
window per its normal rules. The client-area size in DIPs is preserved across
the move.

**Interaction with ExtendsContentIntoTitleBar.** Because `Width`/`Height` measure
the client area, toggling `Window.ExtendsContentIntoTitleBar` changes what counts
as client area, so the getter (and `Bounds`) can change even though the window
rect did not; re-apply the setter if you need a specific size after toggling.
Height is the axis most affected, since the caption region is at the top of the
window.

**XAML markup.** Width and Height are settable from C# / C++/WinRT code-behind
only. They are **not** surfaced in XAML markup on `<Window>`, so there is no
markup attribute to set (and therefore no markup-vs-code mismatch to trip over).
XAML markup support could be added later if there is demand and a sensible
parsing order, that is, Width/Height applied after Activate.

**Bindings.** Width and Height are not `DependencyProperty`-backed. They do not
take part in data binding and do not raise change notifications. This matches the
rest of `Window`'s mutable surface (Title, ExtendsContentIntoTitleBar).

**Threading.** You must set these properties on the thread that owns the Window
(the dispatcher thread). Calls from other threads return the standard
`RPC_E_WRONG_THREAD` error from the XAML framework.

### Errors

| Input                         | Result                              |
| ----------------------------- | ----------------------------------- |
| `value < 0`                   | Throws `E_INVALIDARG`               |
| `Double.NaN`                  | Throws `E_INVALIDARG`               |
| `Double.PositiveInfinity`     | Throws `E_INVALIDARG`               |
| `Double.NegativeInfinity`     | Throws `E_INVALIDARG`               |
| Window in a non-default presenter | Silent no-op (the call succeeds; the live window is unchanged) when the window uses the `FullScreen` or `CompactOverlay` presenter. Subject to change in a future release. |
| `0`                           | Allowed. The window resizes so the client area has zero height; the OS may apply a minimum size. |
| Very large value (> screen)   | Allowed. The OS clamps to its tracking-size maximum; the window grows as tall as the OS allows. |

# API Details

```csharp
namespace Microsoft.UI.Xaml
{
    [contentproperty("Content")]
    unsealed runtimeclass Window
    {
        // ... existing members ...

        // New, behind Feature_ExperimentalApi:
        Double Width;
        Double Height;
    }
}
```

In MIDL3 form, matching the existing IDL in
`dxaml/xcp/dxaml/idl/winrt/core/microsoft.ui.xaml.coretypes2.idl`:

```idl
[contract(Microsoft.UI.Xaml.WinUIContract, 10)]
[feature(Feature_ExperimentalApi)]
[interface_name("Microsoft.UI.Xaml.IWindowFeature_ExperimentalApi")]
{
    Double Width;
    Double Height;
}
```

# Appendix

_(This section will not be part of public docs)_

## Implementation notes

These are implementation details, not part of the public contract. They are
here for posterity, not for the public docs.

**Applying the size.** The setter computes the matching *window rect*
by adding the window's non-client chrome to the requested client size, then
applies it with `SetWindowPos` (Normal state) or by updating `rcNormalPosition`
via `SetWindowPlacement` (Maximized or Minimized). A non-default presenter
(`FullScreen` or `CompactOverlay`) is a silent no-op; the setter checks the
presenter kind up front and bails before touching the window.

**Where the chrome comes from.** For desktop (HWND) windows in Normal state, the
chrome is taken from the window's *observed* window-rect-minus-client delta rather than
from a purely style-based calculation (`AdjustWindowRectExForDpi`). This is what
makes `ExtendsContentIntoTitleBar` work correctly. When the window is Maximized
or Minimized, the live rects do not represent normal chrome, so the style-based
calculation is used instead, with a correction that zeros out the top chrome when
ECITB is active.

**Reading the size back.** The getter returns client-area size in DIPs. In the
Normal, Fullscreen, and CompactOverlay states it reads from `Window.Bounds`. In
the Maximized or Minimized state it computes the restore client size from the
`rcNormalPosition` stored in `WINDOWPLACEMENT`, subtracting the same non-client
chrome and DPI scale the setter used. So the getter/setter round-trip in every
state except the non-default presenters (`FullScreen` and `CompactOverlay`),
where the setter is a no-op.

**UWP host.** When a `Window` is hosted by a UWP `CoreWindow` (legacy path), the
setter calls `SetWindowPos` directly with physical pixels (computed once via
`AdjustWindowRectExForDpi`). We deliberately bypass `CoreWindowWrapper::SetPosition`
here because that wrapper takes DIPs and re-scales internally. Going through it
would double-scale and produce a window roughly `scale^2` larger than requested.

## Design rationale

The big decision was **client area vs window rect** as the unit. WinUI picked
client area so `Width`/`Height` round-trip with `Window.Bounds`, which already
ships as the client area. The alternative (the window rect, like WPF) would have put
two different units on one Window object. See the "Porting from WPF" section for
the user-visible impact.

## Open questions

1. Implementation: should we call to the AppWindow for window-related operations
   whenever possible?
2. Presenters and round-tripping: in a non-default presenter (`FullScreen` or
   `CompactOverlay`) the setter currently no-ops, so `Width`/`Height` does not
   round-trip in those modes. **Open for review, which behavior do we want?**
   - *Option 1 (current): no-op.* Simplest to implement; the rule is "non-default
     presenter = no-op." Cost: `Width`/`Height` does not round-trip while in one
     of these modes.
   - *Option 2: record and apply on return.* The setter records the desired
     *normal* size and applies it when the window returns to `Overlapped`, the
     way Maximized/Minimized already work. `Width`/`Height` round-trips in every
     state and the non-default-presenter special cases go away, leaving one rule:
     "`Width`/`Height` is the Normal-state client size; `Bounds` is the live size."
     Cost: the size applies later, which can surprise.
   - *Option 3: throw.* Setting in a non-default presenter throws instead of
     silently doing nothing. Loud and honest, but apps must guard every set with
     a presenter check.

   Separately, `Overlapped` windows configured as non-resizable
   (`IsResizable = false`) still flow through the normal live-resize path and are
   untested; should that be a no-op, throw, or clamp explicitly?

## FAQ

These came up as questions during design and now have answers.

**Will `Width` and `Height` be dependency properties so I can set or bind them in
XAML markup?** No. `Window` is not a `DependencyObject` (unlike WPF), so these are
plain WinRT properties you set from code, not `DependencyProperty`-backed and not
bindable.

**Will `Window.Bounds` change to report the *desired* size (the most recent setter
value) instead of the actual current size?** No. `Window.Bounds` has shipped as
the actual size, and changing it would be a breaking change.

**Are you adding matching `MinWidth` / `MaxWidth` properties?** Not in this spec.
It is out of scope for now, but we expect to come back to it soon.

**Is there a "size to content" behavior, like setting WPF's `Width` to `NaN`?** No,
that is out of scope. Setting `NaN` throws `E_INVALIDARG`.

## Acknowledgements

This API is based on community contribution
[microsoft/microsoft-ui-xaml#11052](https://github.com/microsoft/microsoft-ui-xaml/pull/11052)
from [dotMorten](https://github.com/dotMorten). Thanks!
