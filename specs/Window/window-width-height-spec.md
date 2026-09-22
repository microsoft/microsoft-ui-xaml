Window.Width and Window.Height
===

Table of Contents


- [Window.Width and Window.Height](#windowwidth-and-windowheight)
- [1. Background](#1-background)
- [2. Conceptual pages (How To)](#2-conceptual-pages-how-to)
  - [2.1. Three sets of APIs size the same window: `Window`, `AppWindow`, and Win32](#21-three-sets-of-apis-size-the-same-window-window-appwindow-and-win32)
  - [2.2. How Window sizing works with Window.ExtendsContentIntoTitleBar](#22-how-window-sizing-works-with-windowextendscontentintotitlebar)
  - [2.3. WPF Comparison](#23-wpf-comparison)
  - [2.4. Sizing properties after the window closes](#24-sizing-properties-after-the-window-closes)
- [3. Examples](#3-examples)
  - [3.1. Set Initial Window Size in XAML Markup](#31-set-initial-window-size-in-xaml-markup)
  - [3.2. Set Initial Window Size with x:Bind](#32-set-initial-window-size-with-xbind)
  - [3.3. Set Window Size in Code-Behind](#33-set-window-size-in-code-behind)
- [4. API Pages](#4-api-pages)
  - [4.1. Window.Width property](#41-windowwidth-property)
    - [4.1.1. Behavior by window state](#411-behavior-by-window-state)
    - [4.1.2. Remarks](#412-remarks)
    - [4.1.3. Errors](#413-errors)
  - [4.2. Window.Height property](#42-windowheight-property)
    - [4.2.1. Behavior by window state](#421-behavior-by-window-state)
    - [4.2.2. Remarks](#422-remarks)
    - [4.2.3. Errors](#423-errors)
- [5. API Details](#5-api-details)
- [6. Appendix](#6-appendix)
  - [6.1. Release plan](#61-release-plan)
  - [6.2. Implementation notes](#62-implementation-notes)
  - [6.3. Design rationale](#63-design-rationale)
  - [6.4. FAQ](#64-faq)
  - [6.5. Acknowledgements](#65-acknowledgements)


# 1. Background

_(This section will not be part of public docs)_

The WinUI 3 [Window](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window)
class is the host for XAML content in a desktop app. Until now, the only way to
change a Window's size from code has been to drop down to the Win32 HWND (via
`IWindowNative`) and call `SetWindowPos`, or to go through `AppWindow.Resize(...)`,
which works in physical pixels.

App developers keep asking for a more natural managed way to do this, the same
way they have it in WPF. This spec adds two new properties to
`Microsoft.UI.Xaml.Window`:

- `Window.Width` (Double, in logical pixels)
- `Window.Height` (Double, in logical pixels)

These mirror what people expect from
[`System.Windows.Window.Width`](https://learn.microsoft.com/dotnet/api/system.windows.window.width)
and `System.Windows.Window.Height` in WPF, with a couple of important
differences (see the WPF compare/contrast section below).

A driving scenario behind this work is setting Width and Height directly in XAML markup -- something WPF developers
already do and find natural.

These APIs ship as **experimental** first (`[feature(Feature_ExperimentalApi)]`),
so the behavior may change based on early adopter feedback before they get
promoted to stable.

# 2. Conceptual pages (How To)

_(This is conceptual documentation that will go to docs.microsoft.com "how to" page)_

`Width` and `Height` get or set the restored size of the window's client area in logical pixels.

The **restored size** is conceptually the size the window is (or will be) when in the restored state -- when its
AppWindow is using the default presenter, and it's not minimized or maximized. For the default WinUI `Window`,
the `AppWindow.Presenter` is `Overlapped`. That's the "normal" desktop window mode.

While the window is open, setting `Width` or `Height` updates the restored size. After first activation,
if the window has the default `Overlapped` presenter and is restored, it resizes immediately.

Otherwise, the resize is deferred:

- Before first activation, the runtime stores the request and applies it on activation if the
  presenter supports sizing.
- If `Window.AppWindow.Presenter` is `FullScreen` or `CompactOverlay`, the runtime remembers the
  restored size and reapplies it when the presenter switches back to `Overlapped`.
- If the Win32 window is minimized or maximized, the runtime updates the Win32 restored size.

In these cases, the getters work the same way: they return the restored size, not the current size.

**A user resize wins over an app-set size.** If your app sets `Width`/`Height` and the user later drags the
window to a new size, the user's size is the one that sticks. For example:

1. App sets `Window.Width = 500` (the window becomes 500 logical pixels wide).
2. User drags the edge to resize it to 600.
3. App enters the `FullScreen` presenter.
4. App exits `FullScreen` (back to `Overlapped`).

The window comes back at width **600**, not 500. The WinUI runtime tracks the restored size from the window's last
size in the overlapped, restored state, no matter how it got there, so the user's resize in step 2 is what gets restored.

Given that the `Width` and `Height` getters don't always return the current client size, use `Bounds`
while the window is open when you need the current client-area size for layout. `Bounds` reflects the
live size except when the window is minimized.

After the window closes, valid assignments to `Width` and `Height` have no effect. Their getters return
the values preserved at close. See [Sizing properties after the window closes](#24-sizing-properties-after-the-window-closes).

You can set the `Width` and `Height` properties in XAML markup and in code-behind. You can use
`x:Bind` (`{x:Bind ...}`) bindings to set the `Width` and `Height` properties, but you can't use classic
data binding (`{Binding ...}`) to set them. This is because the `Window` type is not a `FrameworkElement`.

## 2.1. Three sets of APIs size the same window: `Window`, `AppWindow`, and Win32

Every WinUI desktop window is, underneath, a single Win32 `HWND`. Three different
API layers can read and size that same window, and they do **not** all agree on
units or on what they measure. Here is the whole picture in one place.

The three layers:

- **`Microsoft.UI.Xaml.Window`** (this class, the XAML window) works in **logical pixels**.
  `Bounds`, and now `Width` and `Height`, are all logical pixels. 
- **`Microsoft.UI.Windowing.AppWindow`** (you get it from `Window.AppWindow`) and
  its presenters work in **physical pixels** (they take and return `SizeInt32`).
- **Raw Win32** on the `HWND` (get the handle from
  `WinRT.Interop.WindowNative.GetWindowHandle`, or `IWindowNative::get_WindowHandle`
  in C++) also works in **physical pixels**. These are the same functions any
  classic Win32 app uses, and can also be used to size the window.

Because two of the three layers are in physical pixels and one is in logical pixels,
`Window.Width = 800` (logical pixels) and `AppWindow.ResizeClient(new SizeInt32(800, 600))`
(pixels) are **not** the same on a scaled display.

Here are the key sizing APIs across the three layers, side by side:

| API                                         | Layer         | Unit         | Measures                  | Read / write    | Notes                              |
| ------------------------------------------- | ------------- | ------------ | ------------------------- | --------------- | ---------------------------------- |
| `Window.Width` / `Height`                   | `Xaml.Window` | Logical px   | **Client** area           | get **and** set | Restored size                       |
| `Window.Bounds`                             | `Xaml.Window` | Logical px   | **Client** area           | get only        | Live size                          |
| `AppWindow.ClientSize`                      | `AppWindow`   | Physical px  | **Client** area           | get only        | Live size                          |
| `AppWindow.Size`                            | `AppWindow`   | Physical px  | **Window** rect           | get only        | Live size                          |
| `AppWindow.ResizeClient(sz)`                | `AppWindow`   | Physical px  | **Client** area           | set (method)    | Acts on the live window            |
| `AppWindow.Resize(sz)`                      | `AppWindow`   | Physical px  | **Window** rect           | set (method)    | Acts on the live window            |
| `GetClientRect(hwnd, &r)`                   | Win32         | Physical px  | **Client** area           | get only        | Live size; origin is (0,0)         |
| `GetWindowRect(hwnd, &r)`                   | Win32         | Physical px  | **Window** rect           | get only        | Live size; screen coordinates      |
| `SetWindowPos` / `MoveWindow`               | Win32         | Physical px  | **Window** rect           | set             | Acts on the live window            |
| `GetWindowPlacement` / `SetWindowPlacement` | Win32         | Physical px  | **Window** rect (normal/min/max rects) | get **and** set | Reads/writes the restored rect |

A few things to notice:

- **Unit.** `Xaml.Window` is logical pixels. `AppWindow` and raw Win32 are physical pixels.
- **Client vs window rect.** `Window.Width/Height`, `Window.Bounds`,
  `AppWindow.ClientSize`, and `GetClientRect` measure the **client** area (where
  your XAML content lives). `AppWindow.Size` / `AppWindow.Resize(...)`,
  `GetWindowRect`, and `SetWindowPos` / `MoveWindow` measure the **window rect**
  (caption and borders included).
- **Live vs restored.** Almost every API reports the **live** window.
  `Window.Width/Height` reports the window's restored size. When the window is in the overlapped, restored state, that is
  also the live size. When the window is maximized, minimized, not yet activated/shown, or using
  another AppWindow presenter, it may differ from the live size.
- **Minimized state.** The APIs that report "live size" use the restored size when
  the window is minimized (rather than returning 0,0).
- **Read vs write.** `Window.Width/Height` is the only single member you both read
  and write. `AppWindow` splits it (a get-only property plus a separate method),
  and Win32 splits it across different functions too.

Rule of thumb: to size the **client** area in **logical pixels**, set `Window.Width/Height`.
For **physical pixels**, use `AppWindow.ResizeClient(...)` (client) or
`AppWindow.Resize(...)` (the window rect). Reach for raw Win32 (`SetWindowPos` and
related) only when you need something the WinUI surfaces do not expose; mixing it
with the XAML window means you must handle the logical/physical pixel conversion yourself.

## 2.2. How Window sizing works with Window.ExtendsContentIntoTitleBar

Setting `Window.ExtendsContentIntoTitleBar` to `true` tells the window you want to draw its title bar
yourself. The area occupied by the standard title bar becomes part of the client area.

While the window is in the overlapped, restored state, if your app has explicitly set `Window.Height`,
toggling `ExtendsContentIntoTitleBar` preserves the current client height. The outer window height
adjusts to account for the change in non-client area. This preserves a subsequent user resize, not
necessarily the last height assigned by your app.

If your app has not set `Height`, toggling `ExtendsContentIntoTitleBar` leaves the outer window size
unchanged, so the client height changes instead. Setting `Width` alone does not opt into client-height
preservation.

Both examples assume the window is already displayed in its normal desktop state, not minimized,
maximized, full screen, or compact overlay. They show that you can set these properties in either order.
In each example, the app has not previously set `Height`. Either order produces the same client height:

```cs
window.Height = 300;                      // Requests a client height of 300 logical pixels.
window.ExtendsContentIntoTitleBar = true; // Preserves that client height; reduces the outer height.
```
versus:
```cs
window.ExtendsContentIntoTitleBar = true; // Changes the client area without resizing the outer window.
window.Height = 300;                      // Requests a client height of 300 logical pixels.
```

Before first activation, a pending `Height` request uses the title-bar configuration in effect when
the request is applied. Toggling `ExtendsContentIntoTitleBar` while minimized, maximized, or using a non-default
presenter does not itself preserve the restored client height.

## 2.3. WPF Comparison

WPF developers work with the WPF `Window.Width` and `Window.Height` properties. The
WinUI properties are meant to feel familiar but they are **not identical**.

A WPF app that does `this.Width = 800` makes a window whose **window rect**
is 800 logical pixels wide, so the usable client area inside is something like ~784 logical pixels
after chrome. The same line in WinUI gives a **client area** of exactly 800
logical pixels, so the window rect is ~816 logical pixels. Both behaviors are self-consistent; the
cross-framework number just does not carry over 1:1.

Here is a side-by-side breakdown of more WPF vs WinUI behavior:

| Aspect                    | WPF `Window.Width/Height`                            | WinUI `Window.Width/Height` (this spec)         |
| ------------------------- | ---------------------------------------------------- | ----------------------------------------------- |
| Unit                      | Logical pixels                                       | Logical pixels, same                            |
| What it measures          | **Window rect** (includes chrome)                    | **Client area** (matches `Window.Bounds`)       |
| Default / initial value   | `Double.NaN` (auto-size to content)                  | The current actual client size (never NaN)      |
| Setting `NaN`             | Allowed; means "size to content"                     | **Returns `E_INVALIDARG`**. No size-to-content.  |
| Setting negative          | Throws `ArgumentException`                           | Returns `E_INVALIDARG`                           |
| Setting `Infinity`        | Throws `ArgumentException`                           | Returns `E_INVALIDARG`                           |
| `MinWidth` / `MaxWidth`   | Exists and is enforced                               | Not part of this spec (may follow later)        |
| Behavior when Maximized   | Updates *restored* size; window stays maximized       | Updates *restored* size; window stays maximized  |
| Behavior when Minimized   | Updates *restored* size; window stays minimized       | Updates *restored* size; window stays minimized  |
| Behavior in non-default presenter | WPF has no built-in fullscreen/PiP modes     | Updates restored size; applied on return to `Overlapped` |
| Dependency property       | Yes; bindable                                        | Plain WinRT property; bindable only with x:Bind |

In .NET an `E_INVALIDARG` error is thrown as an `ArgumentException`.

## 2.4. Sizing properties after the window closes

Closing a window freezes its `Width` and `Height` values. Valid assignments after close are ignored:
they do not change the preserved values, create a pending resize, raise `SizeChanged`, or reopen the
window. The getters remain available on the owning thread.

Each preserved value is the value its getter would have returned after the `Closed` handlers complete
without canceling and immediately before native teardown, not necessarily the last value assigned by your app:

- If the user resized the restored window, preserve the resulting client size.
- If the window is minimized or maximized, preserve the restored size, not the live size.
- If a request is still pending, such as before first activation or while using a non-default
  presenter, preserve that request.

For example, this code runs on the owning thread and assumes no `Closed` handler cancels closing:

```csharp
window.Close();
double closedWidth = window.Width;
double closedHeight = window.Height;

window.Width = 800;  // Ignored. Width still returns closedWidth.
window.Height = 600; // Ignored. Height still returns closedHeight.
```

The `Closed` event is raised before native teardown, and a handler can cancel closing by setting
`Handled` to `true`. During those handlers, the sizing properties retain their normal open-window
behavior. A canceled close does not freeze them.

Closing alone is not an error for these properties. Argument validation and thread affinity still
apply: invalid sizes return `E_INVALIDARG`, and access from another thread returns
`RPC_E_WRONG_THREAD`. A valid post-close assignment succeeds without changing either preserved value.
This behavior does not change the closed-state contract of other members, such as `Bounds`.

# 3. Examples

## 3.1. Set Initial Window Size in XAML Markup

It's often natural to set your Window's initial size in your XAML markup near the markup for your app.

For example, Contoso's PointOfSale app defines its main window's size at development time via XAML markup:

```xml
<!-- MainWindow.xaml -->
<?xml version="1.0" encoding="utf-8"?>
<Window
    x:Class="PointOfSale.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PointOfSale"
    xmlns:d="http://schemas.microsoft.com/expression/blend/2008"
    xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
    mc:Ignorable="d"
    Title="Contoso PointOfSale"
    Width="800"
    Height="600"
    >

    <!-- window content -->
</Window>
```

## 3.2. Set Initial Window Size with x:Bind

Here, Contoso PointOfSale binds Width/Height to code-behind:

```xml
<!-- MainWindow.xaml -->
<?xml version="1.0" encoding="utf-8"?>
<Window
    x:Class="PointOfSale.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PointOfSale"
    xmlns:d="http://schemas.microsoft.com/expression/blend/2008"
    xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
    mc:Ignorable="d"
    Title="Contoso PointOfSale"
    Width="{x:Bind InitialWindowClientWidth}"
    Height="{x:Bind InitialWindowClientHeight}"
    >

    <!-- window content -->
</Window>
```

In code-behind, the PointOfSale app defaults to a smaller size when a private variable `_isAppUsingCompactView`
is set:

```cs
// MainWindow.xaml.cs
namespace PointOfSale
{
    public sealed partial class MainWindow : Window
    {
        private bool _isAppUsingCompactView;

        // ...

        public double InitialWindowClientWidth => _isAppUsingCompactView ? 400 : 800;
        public double InitialWindowClientHeight => _isAppUsingCompactView ? 200 : 600;
    }
}
```

## 3.3. Set Window Size in Code-Behind

The PointOfSale app also has a button users can click to enter its compact view.  The app sets Window.Width and Window.Height
in code-behind to change the window's size.


```xml
<!-- MainWindow.xaml -->
  <!-- Inside the app's XAML markup -->
  <Button Click="SwitchToCompactView_Button_Click">Switch to Compact View</Button>
```

```cs
// MainWindow.xaml.cs
namespace PointOfSale
{
    public sealed partial class MainWindow : Window
    {
        // ...
        private void SwitchToCompactView_Button_Click(object sender, RoutedEventArgs e)
        {
            this.Width = 400;
            this.Height = 200;
        }
    }
}
```        


# 4. API Pages

_(Each of the following L2 sections correspond to a page that will be on docs.microsoft.com)_

> **Editor note:** The `Window.Width` and `Window.Height` sections are
> intentionally almost identical so each can stand alone as its own docs page. If
> you change behavior in one, you should probably make the same change in the other.
> (one possible exception is that Height has extra conditions due to TitleBar special cases)

## 4.1. Window.Width property

Gets or sets the width of the Window's **client area** in logical pixels.

```csharp
public double Width { get; set; }
```

**Getter**: returns the restored client-area width in logical pixels.

- Before first activation, a pending `Width` request is returned as supplied. Without a pending
  request, the getter uses the current presenter's restored-width behavior described below.
- In the **Restored** state after activation this equals `Window.Bounds.Width`.
- In the **Maximized** or **Minimized** state it returns the *restored* width -- the
  width the window will have when it returns to the restored state. The OS tracks this
  restored size, so it works even if you never set `Width`.
- When the AppWindow is using a non-default presenter (`FullScreen`, `CompactOverlay`),
  it returns the restored width -- the last width the window had in the restored state
  (whether set by your code or by the user resizing), or a value you set on
  `Width` while in the non-default presenter.
- After the window closes, it returns the width preserved immediately before native teardown.
  Subsequent assignments do not change this value.

**Setter**: while the window is open, requests a client width of `value` logical pixels.
The window's non-client chrome (caption, borders, and so on) gets added on top of
`value` to work out the window rect, using the current per-monitor
DPI. Height is left unchanged. After close, a valid assignment has no effect.

Returns `E_INVALIDARG` for negative, `NaN`, or `Infinity` values.
In .NET an `E_INVALIDARG` error is thrown as an `ArgumentException`.

### 4.1.1. Behavior by window state

These behaviors split by presenter. The default `Overlapped` presenter honors
the setter and maps to restored / maximized / minimized based on its show state.
The non-default presenters, `FullScreen` and `CompactOverlay`, remember the
requested size and apply it when the window returns to `Overlapped`, without
affecting the live window in the meantime.

- **Before first activation**: the requested width is stored and returned by the getter. It is
  applied on activation if the presenter supports sizing, or when it subsequently does.
- **Restored, after activation**: the window resizes right away to the requested client width.
  Position is preserved.
- **Maximized**: the live (maximized) window does not resize. The *restored*
  bounds (the size the window snaps to when un-maximized) get updated. The other
  axis (Height) of the restored bounds is preserved.
- **Minimized**: same as Maximized. The *restored* bounds get updated and the
  window stays minimized (it snaps to the new size when restored). The other
  axis of the restored bounds is preserved.
- **Fullscreen** (via `AppWindowPresenterKind.FullScreen`): the live window stays
  fullscreen. The restored size is updated and applied when the window returns
  to the `Overlapped` presenter.
- **CompactOverlay** (picture-in-picture, via
  `AppWindowPresenterKind.CompactOverlay`): same as Fullscreen. The live window is
  unchanged; the restored size is updated and applied on return to `Overlapped`.
- **Closed**: valid assignments are ignored. The getter returns its preserved pre-teardown value.
  No pending resize or size-change event is created. A canceled close leaves normal behavior intact.

### 4.1.2. Remarks

**Units and DPI.** The setter converts the requested logical-pixel value to physical
pixels using the window's current per-monitor DPI:

```
physicalPixels = round(logicalPixels * GetDpiForWindow(hwnd) / 96.0)
```

Because of integer-pixel rounding, the value you read back from the getter may
differ from the value you passed to the setter by up to a couple of logical pixels. The
scenario tests allow a 2-logical-pixel tolerance when comparing.

**DPI changes during the window's lifetime.** If the window moves to a monitor
with a different scale factor after you call the setter, the OS re-scales the
window per its normal rules. The client-area size in logical pixels is preserved across
the move.

**Interaction with ExtendsContentIntoTitleBar.** Setting `Width` alone does not opt into preserving
client height when the title-bar configuration changes. Only an explicit assignment to `Height`
enables that behavior while the window is overlapped and restored. See
[How Window sizing works with Window.ExtendsContentIntoTitleBar](#22-how-window-sizing-works-with-windowextendscontentintotitlebar).

**XAML markup.** Width and Height are settable from code-behind AND from XAML on the `<Window>` object.

**Bindings.** Width and Height are not `DependencyProperty`-backed. They do not
take part in data binding and do not raise change notifications. (This matches the
rest of `Window`'s API surface).  You can, however, use `x:Bind` to set them
in XAML markup.

**Threading.** You must read and write this property on the thread that owns the Window
(the dispatcher thread), including after close. Calls from other threads return the standard
`RPC_E_WRONG_THREAD` error from the XAML framework.

### 4.1.3. Errors

For all errors the runtime calls RoOriginateError with a specific error message to help the app developer diagnose the problem.

Argument validation applies both before and after close. Closing alone does not cause an error.
The resizing effects below apply only while the window is open; valid post-close assignments are ignored.

| Input                         | Result                              |
| ----------------------------- | ----------------------------------- |
| `value < 0`                   | Returns `E_INVALIDARG`              |
| `Double.NaN`                  | Returns `E_INVALIDARG`              |
| `Double.PositiveInfinity`     | Returns `E_INVALIDARG`              |
| `Double.NegativeInfinity`     | Returns `E_INVALIDARG`              |
| `0`                           | Allowed. The window resizes so the client area has zero width. The OS may apply a minimum size. |
| Very large value (> screen)   | Allowed. The OS clamps to its tracking-size maximum; the window grows as large as the OS allows. |

In .NET an `E_INVALIDARG` error is thrown as an `ArgumentException`.

## 4.2. Window.Height property

Gets or sets the height of the Window's **client area** in logical pixels.

```csharp
public double Height { get; set; }
```

**Getter**: returns the restored client-area height in logical pixels.

- Before first activation, a pending `Height` request is returned as supplied. Without a pending
  request, the getter uses the current presenter's restored-height behavior described below.
- In the **Restored** state after activation this equals `Window.Bounds.Height`.
- In the **Maximized** or **Minimized** state it returns the *restored* height -- the
  height the window will have when it returns to the restored state. The OS tracks this
  restored size, so it works even if you never set `Height`.
- When the AppWindow is using a non-default presenter (`FullScreen`, `CompactOverlay`),
  it returns the restored height -- the last height the window had in the restored state
  (whether set by your code or by the user resizing), or a value you set on
  `Height` while in the non-default presenter.
- After the window closes, it returns the height preserved immediately before native teardown.
  Subsequent assignments do not change this value.

**Setter**: while the window is open, requests a client height of `value` logical pixels.
The window's non-client chrome (caption, borders, and so on) gets added on top of
`value` to work out the window rect, using the current per-monitor DPI. Width is
left unchanged. After close, a valid assignment has no effect.

Returns `E_INVALIDARG` for negative, `NaN`, or `Infinity` values.
In .NET an `E_INVALIDARG` error is thrown as an `ArgumentException`.

### 4.2.1. Behavior by window state

These behaviors split by presenter. The default `Overlapped` presenter honors
the setter and maps to restored / maximized / minimized based on its show state.
The non-default presenters, `FullScreen` and `CompactOverlay`, remember the
requested size and apply it when the window returns to `Overlapped`, without
affecting the live window in the meantime.

- **Before first activation**: the requested height is stored and returned by the getter. It is
  applied on activation if the presenter supports sizing, or when it subsequently does.
- **Restored, after activation**: the window resizes right away to the requested client height.
  Position is preserved.
- **Maximized**: the live (maximized) window does not resize. The *restored*
  bounds (the size the window snaps to when un-maximized) get updated. The other
  axis (Width) of the restored bounds is preserved.
- **Minimized**: same as Maximized. The *restored* bounds get updated and the
  window stays minimized (it snaps to the new size when restored). The other
  axis of the restored bounds is preserved.
- **Fullscreen** (via `AppWindowPresenterKind.FullScreen`): the live window stays
  fullscreen. The restored size is updated and applied when the window returns
  to the `Overlapped` presenter.
- **CompactOverlay** (picture-in-picture, via
  `AppWindowPresenterKind.CompactOverlay`): same as Fullscreen. The live window is
  unchanged; the restored size is updated and applied on return to `Overlapped`.
- **Closed**: valid assignments are ignored. The getter returns its preserved pre-teardown value.
  No pending resize or size-change event is created. A canceled close leaves normal behavior intact.

### 4.2.2. Remarks

**Units and DPI.** The setter converts the requested logical-pixel value to physical
pixels using the window's current per-monitor DPI:

```
physicalPixels = round(logicalPixels * GetDpiForWindow(hwnd) / 96.0)
```

Because of integer-pixel rounding, the value you read back from the getter may
differ from the value you passed to the setter by up to a couple of logical pixels. The
scenario tests allow a 2-logical-pixel tolerance when comparing.

**DPI changes during the window's lifetime.** If the window moves to a monitor
with a different scale factor after you call the setter, the OS re-scales the
window per its normal rules. The client-area size in logical pixels is preserved across
the move.

**Interaction with ExtendsContentIntoTitleBar.** If your app has explicitly set `Height`, toggling
`Window.ExtendsContentIntoTitleBar` while the window is overlapped and restored preserves its current
client height, including subsequent user resizes. If your app has set only `Width`, or neither
property, the outer window size stays unchanged and the client height changes instead. See
[How Window sizing works with Window.ExtendsContentIntoTitleBar](#22-how-window-sizing-works-with-windowextendscontentintotitlebar)
for behavior before activation and in other window states.

**XAML markup.** Width and Height are settable from code-behind
AND from XAML on the `<Window>` object.

**Bindings.** Width and Height are not `DependencyProperty`-backed. They do not
take part in data binding and do not raise change notifications. (This matches the
rest of `Window`'s API surface).  You can, however, use `x:Bind` to set them
in XAML markup.

**Threading.** You must read and write this property on the thread that owns the Window
(the dispatcher thread), including after close. Calls from other threads return the standard
`RPC_E_WRONG_THREAD` error from the XAML framework.

### 4.2.3. Errors

For all errors the runtime calls RoOriginateError with a specific error message to help the app developer diagnose the problem.

| Input                         | Result                              |
| ----------------------------- | ----------------------------------- |
| `value < 0`                   | Returns `E_INVALIDARG`               |
| `Double.NaN`                  | Returns `E_INVALIDARG`               |
| `Double.PositiveInfinity`     | Returns `E_INVALIDARG`               |
| `Double.NegativeInfinity`     | Returns `E_INVALIDARG`               |
| `0`                           | Allowed. The window resizes so the client area has zero height; the OS may apply a minimum size. |
| Very large value (> screen)   | Allowed. The OS clamps to its tracking-size maximum; the window grows as tall as the OS allows. |

In .NET an `E_INVALIDARG` error is thrown as an `ArgumentException`.

# 5. API Details

```c# (but really MIDL3)
namespace Microsoft.UI.Xaml
{
  [contractversion(12)]
  apicontract WinUIContract{};

  [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
  [webhosthidden]
  [contentproperty("Content")]
  unsealed runtimeclass Window
  {
    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [feature(Feature_ExperimentalApi)]
    [interface_name("Microsoft.UI.Xaml.IWindowFeature_ExperimentalApi")]
    {
      Double Width;
      Double Height;
    }
  }
}
```

# 6. Appendix

_(This section will not be part of public docs)_

## 6.1. Release plan

We plan to make the API experimental in WinUI3 main.  We plan to later make it stable for the WinAppSDK 3.0
release, and possibly service it to WinAppSDK 2.x.

## 6.2. Implementation notes

These are implementation details, not part of the public contract. They are
here for posterity, not for the public docs.

**Applying the size.** The setter computes the matching *window rect* by adding the
window's non-client chrome to the requested client size. How it applies depends on
state:

- **Before first activation**: retain the request until activation with a sizing-capable presenter.
- **Restored, after activation**: `SetWindowPos` on the live window.
- **Maximized / Minimized**: updates `rcNormalPosition` via `SetWindowPlacement`, so
  the window snaps to the new size when it is restored.
- **FullScreen / CompactOverlay** (presenters that don't support sizing): the value
  is stashed in a pending field rather than applied. The implementation listens to
  `AppWindow.Changed`; when the presenter changes back to one that supports sizing,
  it applies the pending value through the restored-state path above. If no value was
  ever set there is nothing pending, so a presenter change does nothing.
- **Closed**: validate the argument and return success without changing sizing state or accessing
  native window resources.

**User resize clears pending state.** A value set via `Width`/`Height` in the
restored state is applied immediately and has no further effect.  If the user then
resizes the window (or the window changes size for any other reason), the
previously-set value is not remembered or re-applied.  In particular, entering
and exiting a non-default presenter after a user resize must restore to the
user's size, not a stale app-set value.  The implementation achieves this because
the restored-state setter calls `SetWindowPos` once and does not stash a pending
value; the OS's own `WINDOWPLACEMENT` restored rect tracks whatever the window's
last restored geometry was.

**Where the chrome comes from.** For desktop (HWND) windows in the restored state, the
chrome is taken from the window's *observed* window-rect-minus-client delta rather than
from a purely style-based calculation (`AdjustWindowRectExForDpi`). This is what
makes `ExtendsContentIntoTitleBar` work correctly. When the window is maximized
or minimized, the live rects do not represent normal chrome, so the style-based
calculation is used instead, with a correction that zeros out the top chrome when
`ExtendsContentIntoTitleBar` is active.

**Reading the size back.** While the window is open, the getter returns the restored client-area
size in logical pixels. After close, it returns the preserved pre-teardown value.

- In the pre-activated state, before ShowWindow is called, the getters return the
  restored size (same as `Window.Bounds` at that point, unless the app set a new value).
- In the restored state, it returns the same size as `Window.Bounds`.
- In the **Maximized / Minimized** state it computes the restored client size from the
  `rcNormalPosition` stored in `WINDOWPLACEMENT`, subtracting the same non-client
  chrome and DPI scale the setter used.
- When the AppWindow is using a non-default Presenter, it returns the restored size:
  either the last size it had in the restored state (including user resizes), or a new value
  the app set on Width/Height while in the non-default presenter -- whichever is more recent.

User resizes in the restored state update the tracked restored size. The runtime deliberately does *not* read
`rcNormalPosition` in the FullScreen/CompactOverlay presenters: those overwrite it with
the live (monitor or PiP) rect and keep the true restored rect private, so it would not
be a meaningful restored size.

**UWP host.** The implementation will provide basic UWP support to keep
existing UWP tests running.

## 6.3. Design rationale

The big decision was **client area vs window rect** as the unit. WinUI picked
client area so `Width`/`Height` round-trip with `Window.Bounds`, which already
ships as the client area. The alternative (the window rect, like WPF) would have put
two different units on one Window object.

## 6.4. FAQ

These came up as questions during design.

**Will `Width` and `Height` be dependency properties so I can bind to them in XAML markup?**
No. `Window` is not a `DependencyObject` (unlike WPF), so these are
plain WinRT properties that you can't set with classic Bindings.
You *can* however use `x:Bind` to set the properties from markup.

**Can we change `Window` to be a `FrameworkElement` to allow binding and other scenarios?**
Not in the short term.  We may consider it later if it unblocks a lot of scenarios and we
can find a compat-friendly way to do it.

**Will `Window.Bounds` change to report the *desired* size (the most recent setter
value) instead of the actual current size?** No. `Window.Bounds` has shipped as
the actual size, and changing it would be a breaking change.

**Are you adding matching `MinWidth` / `MaxWidth` properties?** Not in this spec.
It is out of scope for now, but we expect to come back to it soon.

**Is there a "size to content" behavior, like setting WPF's `Width` to `NaN`?** No,
that is out of scope. Setting `NaN` returns `E_INVALIDARG`.

**Is it required the DPI mode to be per-mon-v2?** The Width and Height properties are implemented
by calling win32 functions that are virtualized for DPI mode for the underlying HWND, so they will
honor the DPI mode just as those functions do.

**If I set `Width` and then `Height`, will the window flicker between the two sizes?**
For an open, activated window in the restored state, each setter resizes the window right away,
so setting both is technically two
resizes: one frame at the new width with the old height, then the final size. But both calls run in
the same synchronous turn, before the next frame is drawn, so that in-between size doesn't actually
show up on screen. This matches WPF, which also resizes immediately on each setter and has worked
this way for years.

If you want a single, guaranteed atomic resize -- for example, if you set the two values across an
`await` -- you have a couple of options:

- Set `Width` and `Height` before you call `Activate`. The window isn't shown yet, so there's
  nothing to flicker.
- Use `AppWindow.ResizeClient(...)`, which takes both dimensions in one call (physical pixels).

## 6.5. Acknowledgements

This API is based on community contribution
[microsoft/microsoft-ui-xaml#11052](https://github.com/microsoft/microsoft-ui-xaml/pull/11052)
from [dotMorten](https://github.com/dotMorten). Thanks!
