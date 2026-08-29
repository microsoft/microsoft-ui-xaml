Window MinWidth, MinHeight, MaxWidth, MaxHeight
===

> **STATUS: DRAFT - EXPERIMENTAL - NOT APPROVED**
>
> This spec covers an **experimental** API. It has **NOT** been through the Windows API review board and
> is **NOT approved**. The API surface, behavior, and defaults may all change. Experimental APIs ship only
> in experimental releases of the Windows App SDK, never in a stable release. See
> [api-review-process.md](./api-review-process.md) for how an experimental API becomes stable.
>
> In IDL these properties live behind `[feature(Feature_ExperimentalApi)]` on `Microsoft.UI.Xaml.Window`.

> **AI GENERATED - NEEDS THOROUGH HUMAN REVIEW**
>
> This document was drafted by an AI assistant. It has **not** been fully vetted by a human yet. Do not
> treat anything here as authoritative until a human has carefully reviewed every claim against the actual
> implementation. Expect mistakes.

# Background

_Spec Note: this Background section is for reviewers and is not meant for the public docs._

WinUI 3 apps can already set a window's restored size with the experimental `Window.Width` and
`Window.Height` properties (see [PR #11171](https://github.com/microsoft/microsoft-ui-xaml/pull/11171)).
The natural next ask is "let me stop the user from making the window too small or too large." WPF developers
already know `MinWidth`, `MinHeight`, `MaxWidth`, and `MaxHeight`, so we mirror that shape on
`Microsoft.UI.Xaml.Window`.

Guiding principles:

- Familiar to folks coming from WPF.
- Consistent with the existing experimental `Window.Width` / `Window.Height`, including the idea of a
  "restored size" that the constraints act on.
- Implemented on top of `AppWindow`'s `OverlappedPresenter` (`PreferredMinimumWidth` and friends). We do
  not invent a new windowing model.
- The headline scenario is setting the values in XAML markup: `<Window MinWidth="320" MinHeight="240" />`.
  That has to feel good.

Non-goals:

- No special snap-layout behavior. Snapping flows through the normal platform sizing path.
- No cap on the maximized size. `MaxWidth` / `MaxHeight` only limit the restored size.
- No WPF-style unit suffixes in XAML (`px`, `in`, `cm`, `pt`).
- No new property-change notification. `Window` is not a `DependencyObject`, so these stay plain properties.

# API Pages

## Meaning

These four properties describe the window's **restored client-size constraints**.

- Values are in DIPs (device-independent pixels), the same unit as `Window.Width` and `Window.Height`.
- They apply to the **client area** (the content area inside the window frame), matching `Window.Width` and
  `Window.Height`.
- They change the live window only when the window's `AppWindow` is using an `OverlappedPresenter` and the
  window is in the restored state (not minimized, not maximized).
- In any other state (minimized, maximized, full screen, compact overlay, or a different presenter), the
  value is remembered but does not resize the live window right then. When the window comes back to the
  restored overlapped state, the restored size is clamped by these constraints.

`MaxWidth` and `MaxHeight` do **not** limit the maximized size. A maximized window is allowed to be larger
than its `MaxWidth` / `MaxHeight`.

## Important: this overwrites the OverlappedPresenter constraints

**If you set any of these Window properties, do NOT also set the matching constraint directly on the
`AppWindow`'s `OverlappedPresenter`.** Once you set `Window.MinWidth` (or `MinHeight` / `MaxWidth` /
`MaxHeight`), the `Window` treats itself as the single source of truth and will **overwrite** the
presenter's `PreferredMinimumWidth` (or the matching `PreferredMinimumHeight` / `PreferredMaximumWidth` /
`PreferredMaximumHeight`) with its own value.

This overwrite ("clobber") happens at several times, not just when you set the property:

- Right when you set the `Window` property.
- Every time the window returns to the overlapped presenter after using another presenter (full screen,
  compact overlay, etc.).
- On every DPI change (the value is re-scaled and re-pushed to the presenter).

So any value you write to `OverlappedPresenter.PreferredMinimumWidth` and friends yourself will be silently
replaced the next time one of the above happens. Pick one owner: either drive the constraint through these
`Window` properties, or drive it through the `OverlappedPresenter` directly. Do not mix the two for the same
dimension.

## Defaults and valid values

Defaults are deliberately liberal so existing apps keep working unchanged:

| Property | Default | Valid values |
| --- | --- | --- |
| `MinWidth`, `MinHeight` | `0` | any finite value `>= 0` |
| `MaxWidth`, `MaxHeight` | `PositiveInfinity` | any finite value `>= 0`, or `PositiveInfinity` |

Setting an invalid value throws `ArgumentException`:

- a negative value
- `NaN`
- `NegativeInfinity`
- `PositiveInfinity` for `MinWidth` / `MinHeight` (a minimum of "infinity" makes no sense)

To remove a constraint, set it back to its default: `0` for a minimum, `PositiveInfinity` for a maximum.

`PositiveInfinity` cannot be written in XAML, so `MaxWidth` / `MaxHeight` are effectively "set to a finite
value" from markup. Leave the attribute off to keep the default of "no maximum."

## Constraint conflicts

If the minimum and maximum disagree, the **minimum wins** for the effective size. Both app-set values are
kept as-is (we don't rewrite one to match the other). This matches how `OverlappedPresenter` behaves.

```csharp
window.MinWidth = 500;
window.MaxWidth = 300;
// Effective restored width is 500.
```

## Relationship with Window.Width and Window.Height

`Window.Width` and `Window.Height` are the restored client size. The min/max constraints clamp that restored
size, and the clamped value is what the getter reports.

```csharp
window.Width = 300;
window.MinWidth = 500;
// window.Width now reports 500.
// If the window is restored and overlapped, the live client width also becomes 500.
// Otherwise the stored restore size becomes 500 and is applied when the window next returns to
// restored overlapped state.
```

The order in XAML does not matter. Both of these produce an effective restored width of 500:

```xaml
<Window Width="300" MinWidth="500" />
<Window MinWidth="500" Width="300" />
```

## AppWindow and Win32 behavior

_Spec Note: implementation detail, useful for reviewers, trim as needed for public docs._

These APIs layer over `AppWindow` / `OverlappedPresenter` / Win32. The implementation converts:

```text
client DIPs -> client physical pixels -> outer-window physical pixels -> OverlappedPresenter tracking constraint
```

It uses the same DPI and client-to-window conversion rules as `Window.Width` / `Window.Height`: DIPs are
scaled to physical pixels at the current DPI and rounded to the nearest integer pixel.

```text
physicalPixels = round(dips * GetDpiForWindow(hwnd) / 96.0)
```

The OS can still win. For example `MaxWidth = 0` is a legal value, but the live window may still be wider
because of the system minimum tracking size and the non-client frame. Very large finite values are passed
straight through to the windowing layer; WinUI does not add its own cap.

## Events

Changing one of these properties fires the normal `Window.SizeChanged` event **only if the live size
actually changes**.

- If the live window resizes, size events behave exactly as they do for any other resize.
- If only the stored restore size changes (window not currently restored/overlapped), no size event fires
  until a real size change happens later.
- `Window` is not a `DependencyObject`, so there is no dependency-property change notification and no
  `INotifyPropertyChanged` here.

## Threading

Call these on the same thread that created the `Window`. A call from another thread fails with an illegal
thread error (`RPC_E_WRONG_THREAD`).

## Platform support

These properties are supported for WinUI Desktop apps. On UWP they return `E_NOTIMPL`.

# API Details

```csharp
namespace Microsoft.UI.Xaml
{
    // NOTE: Experimental. Shown as the delta added to the existing Window runtimeclass.
    unsealed runtimeclass Window
    {
        // ... existing members ...

        // [feature(Feature_ExperimentalApi)]
        Double MinWidth;
        Double MinHeight;
        Double MaxWidth;
        Double MaxHeight;
    }
}
```

# Open questions

- Get/set behavior after `Window.Close()` is still being finalized.
