Window.MinWidth, MinHeight, MaxWidth, and MaxHeight
===

- [Window.MinWidth, MinHeight, MaxWidth, and MaxHeight](#windowminwidth-minheight-maxwidth-and-maxheight)
- [1. Background](#1-background)
- [2. Conceptual pages (How-to)](#2-conceptual-pages-how-to)
  - [2.1. The Window size constraint properties (MinWidth, MinHeight, MaxWidth, MaxHeight)](#21-the-window-size-constraint-properties-minwidth-minheight-maxwidth-maxheight)
  - [2.2. Relationship to AppWindow and OverlappedPresenter](#22-relationship-to-appwindow-and-overlappedpresenter)
    - [2.2.1. Switching presenters](#221-switching-presenters)
    - [2.2.2. Limits of AppWindow integration](#222-limits-of-appwindow-integration)
  - [2.3. Relationship to other Window properties](#23-relationship-to-other-window-properties)
  - [2.4. Window SizeChanged event and reentrancy](#24-window-sizechanged-event-and-reentrancy)
  - [2.5. Use in XAML markup](#25-use-in-xaml-markup)
  - [2.6. Errors and threading](#26-errors-and-threading)
- [3. Examples](#3-examples)
  - [3.1. Set initial size and constraints](#31-set-initial-size-and-constraints)
  - [3.2. Change or remove constraints](#32-change-or-remove-constraints)
- [4. API pages](#4-api-pages)
  - [4.1. Window.MinWidth property](#41-windowminwidth-property)
  - [4.2. Window.MinHeight property](#42-windowminheight-property)
  - [4.3. Window.MaxWidth property](#43-windowmaxwidth-property)
  - [4.4. Window.MaxHeight property](#44-windowmaxheight-property)
- [5. API details](#5-api-details)
- [6. Appendix](#6-appendix)

# 1. Background

_Spec Note: Background for reviewers; not intended for public documentation._

`Window.MinWidth`, `MinHeight`, `MaxWidth`, and `MaxHeight` let apps constrain a window's size
in XAML markup or code.

These properties have the same names and similar semantics as the size constraint properties on WinUI's `FrameworkElement`
and WPF's `Window`.

The `MinWidth`/`MinHeight` properties are in more demand than the maximum properties.

_Spec Note: This document follows the
[API spec template](https://github.com/microsoft/WindowsAppSDK/blob/main/specs/spec_template.md).
These APIs are experimental. Implementation work to match this spec is tracked in
[#11965](https://github.com/microsoft/microsoft-ui-xaml/issues/11965)._

# 2. Conceptual pages (How-to)

_Spec Note: Intended for a learn.microsoft.com how-to page._

## 2.1. The Window size constraint properties (MinWidth, MinHeight, MaxWidth, MaxHeight)

The `Window` size constraint properties `MinWidth`, `MinHeight`, `MaxWidth`, and `MaxHeight` let you put limits on the
size of your window. The values describe the width or height of the client area in logical pixels.
These properties use the same units as the `Width` and `Height` properties.

`MinWidth` and `MinHeight` have a default value of `0`. `MaxWidth` and `MaxHeight` have a default value of `Double.PositiveInfinity`.

WinUI does not adjust the size constraint values to keep each minimum less than or equal to its corresponding maximum.
If a minimum exceeds its corresponding maximum, the minimum takes precedence.

The order in which you set these properties generally doesn't matter.  The one exception: if you've previously set a minimum value
to be greater than its corresponding maximum, and then relax both constraints (to their defaults, for example), the order
in which you set the properties _can_ result in a different window size. 

## 2.2. Relationship to AppWindow and OverlappedPresenter

WinUI implements these size constraints by configuring the underlying `AppWindow` object, specifically by
setting properties on the `AppWindow`'s `OverlappedPresenter`. This is the same object you get by querying
`Window.AppWindow.Presenter` in your own code. By default, this property returns an `OverlappedPresenter` object,
but the active presenter can be another type, such as `FullScreenPresenter`. The `Window` size constraint
properties only have an effect when the active presenter is an `OverlappedPresenter`.

Each `Window` size constraint property maps to an `OverlappedPresenter` property:

| `Window` property (logical pixels, client area only) | `OverlappedPresenter` property (physical pixels, includes window frame) |
| --- | --- |
| `Window.MinWidth` | `OverlappedPresenter.PreferredMinimumWidth` |
| `Window.MinHeight` | `OverlappedPresenter.PreferredMinimumHeight` |
| `Window.MaxWidth` | `OverlappedPresenter.PreferredMaximumWidth` |
| `Window.MaxHeight` | `OverlappedPresenter.PreferredMaximumHeight` |

Set either a `Window` size constraint
property or the corresponding `OverlappedPresenter` size constraint property, not both, because WinUI overwrites the
`OverlappedPresenter` value.
When you set a `Window` size constraint property to a non-default value,
WinUI sets and manages the corresponding `OverlappedPresenter` property until the `Window` property is set
back to the default value. When WinUI is managing a particular `OverlappedPresenter` property, it updates that property
when the window DPI changes, when `Window.ExtendsContentIntoTitleBar` changes, and when the `AppWindow.Presenter` is set
to a new `OverlappedPresenter` object.

When you set a `Window` size constraint property back to its default, WinUI clears the corresponding
`OverlappedPresenter` property and then stops managing it. If you assign a size constraint property the same value it already has
(whether default or non-default), nothing happens.

### 2.2.1. Switching presenters

While the window is open, you can set the size constraint properties even if the active presenter is not an
`OverlappedPresenter`. This is not an error. WinUI stores the values to apply them later.
When an `OverlappedPresenter` becomes active, WinUI updates the new presenter with all the non-default size
constraints.  This also happens when one `OverlappedPresenter` replaces another.

If you reset a size constraint property to its default while no `OverlappedPresenter` is active, WinUI defers
clearing the matching presenter constraint until an `OverlappedPresenter` becomes active. WinUI clears that
constraint once, then stops managing it. Clearing does not restore an earlier presenter value.

WinUI detects presenter changes through the `AppWindow.Changed` event. Your app can call `AppWindow.SetPresenter`
to set a new presenter.  WinUI does not guarantee it will configure the new presenter before the
call to `SetPresenter` returns.

### 2.2.2. Limits of AppWindow integration

Changes to the `OverlappedPresenter` size constraint properties do not change the corresponding `Window`
property values. The values also don't change in response to the user resizing the window, pixel rounding,
or operating system limits.

WinUI does not detect every change that requires recalculating the size constraints. For example, WinUI does not detect
changes made through `OverlappedPresenter.SetBorderAndTitleBar` or other AppWindow or native HWND APIs that could change
the size of the client area.

## 2.3. Relationship to other Window properties

Unlike `Width` and `Height`, the `MinWidth`, `MinHeight`, `MaxWidth`, and `MaxHeight` properties **do** constrain the size
of a window in the maximized state. By contrast, `Width` and `Height` only represent the size of the window in the restored state
(not maximized or minimized).

WinUI does not guarantee that the `Width` and `Height` properties always return values within the size constraints.
Here are some scenarios where they may be outside the constraints:

- The `Window` has not yet been shown.
- The `Window` is minimized or maximized.
- The `AppWindow.Presenter` property is set to a presenter that's not an `OverlappedPresenter`.

When `Window.ExtendsContentIntoTitleBar` is `true`, WinUI removes the system caption bar and expands the client area so that WinUI
content fills the caption area. When this property changes, WinUI recalculates and updates the constraints it manages
on the `OverlappedPresenter`, which can change the window's size.

## 2.4. Window SizeChanged event and reentrancy

If you set a size constraint on a restored or maximized window, the window can resize before the setter returns.
A maximized window stays maximized even if a constraint resizes it. If you set a constraint while the window is
minimized, WinUI applies the constraint without resizing the window.

If setting a constraint causes the window to resize, WinUI raises `Window.SizeChanged` as normal. Storing a constraint
alone does not raise this event. Your `SizeChanged` event handler may run before the constraint setter returns.
Setting a constraint does not guarantee a particular number or order of size-change events.

In some cases, for example during a DPI change, WinUI sets all the `OverlappedPresenter` properties it's managing
one by one. This can trigger window resizes, which can raise the `SizeChanged` event. If you close the window
during one of these `SizeChanged` events, WinUI stops updating the remaining presenter constraints.

## 2.5. Use in XAML markup

You can set these properties directly in XAML markup, as shown in [Set initial size and constraints](#31-set-initial-size-and-constraints).
Use `MaxWidth="Infinity"` or `MaxHeight="Infinity"` to specify no app-requested maximum for that dimension.
You can also omit either maximum to use its default value of `Double.PositiveInfinity`.

Compiled `x:Bind` can assign these properties. Classic `Binding` is not supported.
These properties are not dependency properties and do not provide property-change notifications.

## 2.6. Errors and threading

The following table lists accepted values and the errors returned for invalid values:

| Input                         | Result                              |
| ----------------------------- | ----------------------------------- |
| `value < 0`                   | Returns `E_INVALIDARG`.             |
| `Double.NaN`                  | Returns `E_INVALIDARG`.             |
| `Double.PositiveInfinity`     | Allowed for `MaxWidth` and `MaxHeight` only. Otherwise, returns `E_INVALIDARG`. |
| `Double.NegativeInfinity`     | Returns `E_INVALIDARG`.             |
| `0`                           | Allowed.                            |
| Very large value (> screen)   | Allowed. The OS may enforce a limit. |

In .NET, `E_INVALIDARG` is reported as an `ArgumentException`.

Read and write these properties only on the thread that owns the window. Calls from other threads return `RPC_E_WRONG_THREAD`.
This requirement still applies after the window closes.

When you set a size constraint property to a valid value, WinUI stores the value before updating the `OverlappedPresenter`.
If that update fails, the setter returns the error, but the property retains the value you assigned. The property also retains
its value if a later update, such as one triggered by a presenter change, fails.

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

All sizes describe the client area in logical pixels. The window frame is not included.
Omit `MaxWidth` or `MaxHeight` from the XAML if you do not want to set that maximum.

## 3.2. Change or remove constraints

```csharp
window.ExtendsContentIntoTitleBar = true;
window.MinHeight = 320; // Includes the custom title-bar region.

window.MinWidth = 0;
window.MaxWidth = double.PositiveInfinity;
```

If `MinWidth` and `MaxWidth` had non-default values, the last two assignments clear the
matching `OverlappedPresenter` constraints once, and then WinUI stops updating those presenter
properties. If the `MinWidth` and `MaxWidth` properties were already at their defaults,
these assignments do nothing. Assigning a non-default value to either
`Window` property makes WinUI manage that constraint again.

# 4. API pages

_Spec Note: Each subsection corresponds to a learn.microsoft.com property page.
Shared behavior is documented above rather than repeated on all four pages._

All four properties remain readable and writable after the window is closed. Each getter
returns the latest value your app assigned to that property, or its default if never assigned.
When you set a property after the window is closed, WinUI updates only the stored value. There are
no other side effects.

## 4.1. Window.MinWidth property

Gets or sets the minimum width of the window's client area, in logical pixels.

```csharp
public double MinWidth { get; set; }
```

Accepts finite values greater than or equal to zero. The default is `0` (no app-requested minimum).
Negative values, `NaN`, and either infinity are invalid.

## 4.2. Window.MinHeight property

Gets or sets the minimum height of the window's client area, in logical pixels.

```csharp
public double MinHeight { get; set; }
```

Accepts finite values greater than or equal to zero. The default is `0` (no app-requested minimum).
Negative values, `NaN`, and either infinity are invalid.

## 4.3. Window.MaxWidth property

Gets or sets the maximum width of the window's client area, in logical pixels.

```csharp
public double MaxWidth { get; set; }
```

Accepts finite values greater than or equal to zero, or `Double.PositiveInfinity`.
The default is `Double.PositiveInfinity` (no app-requested maximum).
Negative values, `NaN`, and negative infinity are invalid.

## 4.4. Window.MaxHeight property

Gets or sets the maximum height of the window's client area, in logical pixels.

```csharp
public double MaxHeight { get; set; }
```

Accepts finite values greater than or equal to zero, or `Double.PositiveInfinity`.
The default is `Double.PositiveInfinity` (no app-requested maximum).
Negative values, `NaN`, and negative infinity are invalid.

# 5. API details

_Spec Note: API declarations for review; public reference pages document these members individually._

```idl
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

Design changes from past iterations:

- Earlier designs reapplied all non-default constraints when an app changed any one constraint.
  The current design updates only the corresponding presenter constraint.