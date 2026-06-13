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
  Normal and Fullscreen states this matches `Bounds.Width` / `Bounds.Height`.
  In the Maximized or Minimized states the getter returns the **restore** size,
  the size the window will have when it goes back to Normal, so that
  `Width`/`Height` round-trip with the setter in those states. (Fullscreen is
  the one state where they do not round-trip; see below.)
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
on `<Window>`. See the Bindings and XAML markup remarks on the `Window.Width` page.

## A note on units: DIPs vs physical pixels

There are two windowing surfaces in the platform and they use different units, so
it is worth being clear up front:

- **`Microsoft.UI.Xaml.Window`** (this class) works in **DIPs**. `Bounds`,
  and now `Width` and `Height`, are all DIPs. The plan going forward is that
  every size-related property on `Xaml.Window` stays in DIPs, so you do not have
  to remember which one is which.
- **`Microsoft.UI.Windowing.AppWindow`** and its presenters work in **physical
  pixels** (they take `SizeInt32`). That includes `AppWindow.Size`,
  `AppWindow.Resize(...)`, and `AppWindow.ResizeClient(...)`.

So `Window.Width = 800` (DIPs) and `AppWindow.ResizeClient(new SizeInt32(800, 600))`
(physical pixels) are NOT the same on a scaled display. To go from DIPs to
physical pixels, multiply by `GetDpiForWindow(hwnd) / 96.0`.

If you want to size the **outer** window (chrome included) instead of the client
area, use `AppWindow.Resize(...)` (physical pixels). For a physical-pixel client
size, use `AppWindow.ResizeClient(...)`.

## WPF compare/contrast

WPF developers work with the WPF `Window.Width` and `Window.Height` properties. The
WinUI properties are meant to feel familiar but they are **not identical**.
Here is a side-by-side:

| Aspect                    | WPF `Window.Width/Height`                            | WinUI `Window.Width/Height` (this spec)         |
| ------------------------- | ---------------------------------------------------- | ----------------------------------------------- |
| Unit                      | DIPs (1/96 inch)                                     | DIPs (1/96 inch), same                          |
| What it measures          | **Outer** window (includes chrome)                   | **Client area** (matches `Window.Bounds`)       |
| Default / initial value   | `Double.NaN` (auto-size to content)                  | The current actual client size (never NaN)      |
| Setting `NaN`             | Allowed; means "size to content"                     | **Throws `E_INVALIDARG`**. No size-to-content.  |
| Setting negative          | Throws `ArgumentException`                           | Throws `E_INVALIDARG`                           |
| Setting `Infinity`        | Throws `ArgumentException`                           | Throws `E_INVALIDARG`                           |
| `MinWidth` / `MaxWidth`   | Exists and is enforced                               | Not part of this spec (may follow later)        |
| Behavior when Maximized   | Updates *restore* size; window stays maximized       | Updates *restore* size; window stays maximized  |
| Behavior when Minimized   | Updates *restore* size; window stays minimized       | Updates *restore* size; window stays minimized  |
| Behavior when Fullscreen  | WPF has no built-in fullscreen mode                  | **No-op** (stays fullscreen, restore not updated; subject to change) |
| Dependency property       | Yes; bindable                                        | Plain WinRT property; not bindable              |

The most important difference is **client rect vs window rect**. WPF picked window bounds
because WPF windows draw their own chrome. WinUI Windows usually host system
chrome (caption buttons drawn by DWM), so the client area is the more useful
unit for laying out app content. It also means `Width`/`Height` round-trip with
`Window.Bounds` right away:

```csharp
this.Width  = 800;
this.Height = 600;
Debug.Assert(this.Bounds.Width  == this.Width);
Debug.Assert(this.Bounds.Height == this.Height);
```

That round-trip holds in Normal, Maximized, and Minimized states. In the
Maximized and Minimized states the setter updates the *restore* bounds and the
getter reads them back, so both directions agree on the same value even though
the live window is a different size:

```csharp
// Window is maximized, live client area is e.g. 1920 x 1040.
this.Width = 800;        // updates the restore width only; live window unchanged
double w = this.Width;   // == 800 (restore width, NOT 1920)
// The live window is still maximized at 1920 x 1040.
// Un-maximize, then:
double w2 = this.Width;  // == 800 (now the live width too)
```

The one exception is Fullscreen: the setter is a silent no-op and the getter
returns the live fullscreen size, so they do not round-trip. See the per-state
table below.

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

In short: `Width == Bounds.Width` when the window is in its "natural" state
(Normal, Fullscreen, or CompactOverlay). In the Maximized and Minimized states,
`Width` and `Height` tell you what the window *will be* when restored, while
`Bounds` tells you what the window *is right now*.

If you want to size by outer bounds instead, you can still call
`AppWindow.Resize(...)` (physical pixels, not DIPs; see the units note above).

## Porting from WPF: watch the unit change

A WPF app that does `this.Width = 800` makes a window whose outer **window** rectangle
is 800 DIPs wide, so the usable client area inside is something like ~784 DIPs
after chrome. The same line in WinUI gives a **client area** of exactly 800
DIPs, so the outer window is ~816 DIPs. Both behaviors are self-consistent; the
cross-framework number just does not carry over 1:1.

This was a deliberate choice for WinUI. `Window.Bounds` was already the client
area, so the new setters round-trip with it right away (`Width == Bounds.Width`).
Making the setters mean "outer window" would have produced two different units on one
Window object, which is even more confusing.

# API Pages

_(Each of the following L2 sections correspond to a page that will be on docs.microsoft.com)_

## Window.Width property

Gets or sets the width of the Window's **client area** in device-independent
pixels (DIPs, 1/96 inch).

```csharp
public double Width { get; set; }
```

**Getter**: returns the current client-area width in DIPs. In the Normal and
Fullscreen states this equals `Window.Bounds.Width`. In the Maximized or
Minimized state the getter returns the **restore** width, the width the window
will have when it goes back to Normal, so the getter round-trips with the
setter in those states.

**Setter**: changes the window so its client area is `value` DIPs wide. The
window's non-client chrome (caption, borders, and so on) gets added on top of
`value` to work out the outer window rectangle, using the current per-monitor
DPI. Height is left unchanged.

Throws `E_INVALIDARG` for negative, `NaN`, or `Infinity` values.

### Behavior by window state

These four behaviors are driven by the window's show state, not directly by the
`AppWindow` presenter. The only presenter the setter special-cases is
`FullScreen` (see below). An `Overlapped` presenter maps to Normal / Maximized /
Minimized based on its state, and `CompactOverlay` maps to Normal.

- **Normal**: the window resizes right away to the requested client width.
  Position is preserved.
- **Maximized**: the live (maximized) window does not resize. The *restore*
  bounds (the size the window snaps to when un-maximized) get updated. The other
  axis (Height) of the restore bounds is preserved.
- **Minimized**: same as Maximized. The *restore* bounds get updated and the
  window stays minimized (it snaps to the new size when restored). The other
  axis of the restore bounds is preserved.
- **Fullscreen** (via `AppWindowPresenterKind.FullScreen`): the call is a
  silent **no-op** for now. The live window stays fullscreen and the restore
  bounds are not updated. We deliberately do not commit to "stash and apply on
  exit" or "throw" yet. A silent no-op keeps us free to pick a stronger contract
  later without breaking apps. Apps that need a specific post-fullscreen size
  should stash it themselves and re-apply after leaving fullscreen.
- **CompactOverlay** (picture-in-picture, via
  `AppWindowPresenterKind.CompactOverlay`): treated the same as Normal. The
  setter resizes the live compact-overlay window to the requested client size
  and the getter reads it back from `Bounds`. Note the CompactOverlay presenter
  may clamp the window to its own allowed size range, so the value you read back
  can differ from what you set. This path is not special-cased and is currently
  untested, so the behavior may change.

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

**Interaction with ExtendsContentIntoTitleBar.**
`Window.ExtendsContentIntoTitleBar` changes what the OS treats as "client area".
When it is **off** (the default), DWM draws the caption bar as non-client chrome,
so the client area starts below it. When it is **on**, the client area stretches
up to include the caption region, so app content can render behind the caption
buttons.

Because `Width` and `Height` measure the client area, the same pixel values mean
slightly different things in each mode:

```
  ExtendsContentIntoTitleBar = false          ExtendsContentIntoTitleBar = true
  +---[caption / min|max|close]---+           +---[   min|max|close  ]---+
  |                               |           |   (caption region)       |
  +-------------------------------+           |   now part of client     |
  |                               |           |                          |
  |   client area                 |           |   client area            |
  |   (Height measures this)      |           |   (Height measures       |
  |                               |           |    ALL of this)          |
  |                               |           |                          |
  +-------------------------------+           +--------------------------+
```

In both modes, `Width` and `Height` equal `Bounds.Width` and `Bounds.Height` in
the Normal state. The difference is only in how much non-client chrome sits
outside that client rect:

| Aspect                    | ECITB off (default)                  | ECITB on                                 |
| ------------------------- | ------------------------------------ | ---------------------------------------- |
| Top chrome                | Resize border + caption bar          | Resize border only (a few px)            |
| Side + bottom chrome      | Resize borders                       | Resize borders (same)                    |
| `Height = 600` means      | 600 DIPs of content below caption    | 600 DIPs including caption region        |
| Outer window height       | 600 + caption + borders              | 600 + borders (smaller outer window)     |
| Usable content height     | 600 DIPs (all content)               | ~570 DIPs (600 minus ~30 caption region) |

The setter handles this for you. If you set `Height = 600` and then flip
`ExtendsContentIntoTitleBar`, the OS recalculates the client area, so
`Bounds.Height` (and the getter) will change because the boundary between client
and non-client moved, even though the outer window size did not. If pixel-perfect
sizing matters after toggling, call the setter again.

**XAML markup.** Width and Height are settable from C# / C++/WinRT code-behind
only. They are **not** surfaced in XAML markup on `<Window>` for this
experimental release, so there is no markup attribute to set (and therefore no
markup-vs-code mismatch to trip over). XAML markup support could be added later
if there is demand and a sensible parsing order, that is, Width/Height applied
after Activate.

**Bindings.** Width and Height are not `DependencyProperty`-backed. They do not
take part in data binding and do not raise change notifications. This matches the
rest of `Window`'s mutable surface (Title, ExtendsContentIntoTitleBar) and keeps
the experimental surface small.

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
| Window is fullscreen          | Silent no-op (the call succeeds; the live window stays fullscreen and the restore size is unchanged). Subject to change in a future release. |
| `0`                           | Allowed. The window resizes so the client area has zero width (or height). The OS may clamp to a minimum tracking size; nothing crashes. |
| Very large value (> screen)   | Allowed. The OS clamps to its tracking-size maximum; the window grows as large as the OS allows. |

## Window.Height property

Gets or sets the height of the Window's **client area** in device-independent
pixels (DIPs, 1/96 inch).

```csharp
public double Height { get; set; }
```

Same semantics as [Window.Width](#windowwidth-property), just for the
client-area height instead of width. The getter, setter, per-state behavior,
DPI handling, ECITB interaction, errors, and threading rules are all identical.

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
here for posterity, not for DMC.

**Applying the size.** The setter computes the matching *outer* window rectangle
by adding the window's non-client chrome to the requested client size, then
applies it with `SetWindowPos` (Normal state) or by updating `rcNormalPosition`
via `SetWindowPlacement` (Maximized or Minimized). Fullscreen is a silent no-op.

**Where the chrome comes from.** For desktop (HWND) windows in Normal state, the
chrome is taken from the window's *observed* outer-window-minus-client delta rather than
from a purely style-based calculation (`AdjustWindowRectExForDpi`). This is what
makes `ExtendsContentIntoTitleBar` work correctly. When the window is Maximized
or Minimized, the live rects do not represent normal chrome, so the style-based
calculation is used instead, with a correction that zeros out the top chrome when
ECITB is active.

**Reading the size back.** The getter returns client-area size in DIPs. In the
Normal and Fullscreen states it reads from `Window.Bounds`. In the Maximized or
Minimized state it computes the restore client size from the `rcNormalPosition`
stored in `WINDOWPLACEMENT`, subtracting the same non-client chrome and DPI scale
the setter used. So the getter/setter round-trip in every state except
Fullscreen.

**UWP host.** When a `Window` is hosted by a UWP `CoreWindow` (legacy path), the
setter calls `SetWindowPos` directly with physical pixels (computed once via
`AdjustWindowRectExForDpi`). We deliberately bypass `CoreWindowWrapper::SetPosition`
here because that wrapper takes DIPs and re-scales internally. Going through it
would double-scale and produce a window roughly `scale^2` larger than requested.

## Design rationale

The big decision was **client area vs outer window** as the unit. WinUI picked
client area so `Width`/`Height` round-trip with `Window.Bounds`, which already
ships as the client area. The alternative (outer window bounds, like WPF) would have put
two different units on one Window object. See the "Porting from WPF" section for
the user-visible impact.

## Open questions

1. Should `Width` and `Height` become `DependencyProperty`-backed before
   promoting out of experimental? That would enable XAML markup setters and data
   binding, at the cost of a larger ABI commitment.
   **No**, because Window is not a DependencyObject (unlike WPF).
2. Should `Window.Bounds` change to track only the *desired* size (the most
   recent setter value) rather than the actual current size? No.
   `Window.Bounds` has shipped as the actual size and changing it would be a
   breaking change.
3. Do we add matching `MinWidth` / `MaxWidth` properties? Out of scope for now,
   coming back to this soon.
4. How should we expose "size to content" (the WPF `NaN` behavior)? Out of scope.
5. Implementation: should we call to the AppWindow for window-related operations
   whenever possible?
6. Presenters: the setter only special-cases the `FullScreen` presenter.
   `CompactOverlay`, and `Overlapped` windows configured as non-resizable
   (`IsResizable = false`), currently flow through the normal live-resize path
   and are untested. Should any of these be a no-op, throw, or clamp explicitly
   instead of relying on the OS/presenter to clamp?

## Acknowledgements

This API is based on community contribution
[microsoft/microsoft-ui-xaml#11052](https://github.com/microsoft/microsoft-ui-xaml/pull/11052)
by external contributors. Thanks!
