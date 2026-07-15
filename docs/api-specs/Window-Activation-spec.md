Window showing and activation
===

> **STATUS: DRAFT FOR API REVIEW**
>
> This proposal is intentionally open for design feedback. The API shape and
> behavior are not approved.

# Background

[Issue #7595](https://github.com/microsoft/microsoft-ui-xaml/issues/7595)
reports that `Window.Activate()` does not bring a background WinUI window to
the foreground unless the window is minimized.

The current WinUI implementation uses this sequence:

1. `ShowWindow` to make the HWND visible or restore it.
2. `UpdateWindow` to paint it.
3. `SetActiveWindow` to activate it within the calling thread.

This works when the app is already the foreground app, but
[`SetActiveWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setactivewindow)
does not bring a background app to the foreground.

The first implementation considered changing `Window.Activate()` to call
`SetForegroundWindow`, then pivoted to a separate `Window.SetForeground()`
method. Neither choice fully accounts for the fact that WinUI currently uses
`Activate()` for two different jobs:

- Showing a window for the first time.
- Asking an existing window to become active.

Those jobs are separate in WPF and in Win32.

## WPF behavior

WPF has three relevant members:

| API | Behavior |
| --- | --- |
| [`Window.Show()`](https://learn.microsoft.com/dotnet/api/system.windows.window.show) | Shows a modeless window. |
| [`Window.ShowActivated`](https://learn.microsoft.com/dotnet/api/system.windows.window.showactivated) | Controls whether the window is activated when it is first shown. The default is `true`. Apps set it to `false` before `Show()` to show without activation. |
| [`Window.Activate()`](https://learn.microsoft.com/dotnet/api/system.windows.window.activate) | Attempts to bring an existing window to the foreground and activate it. It returns whether activation succeeded and follows the Win32 `SetForegroundWindow` rules. |

This split lets a WPF app show a window without activation while keeping
`Activate()` focused on foreground activation.

WinUI has no public `Window.Show()` API or `ShowActivated` property. Apps call
`Window.Activate()` to show a newly created window. Changing that method to
always request foreground activation could cause existing secondary windows,
tool windows, and palettes to take focus when an app only meant to show them.

## Win32 behavior

Win32 also separates showing, thread activation, and foreground activation:

| API | Behavior |
| --- | --- |
| [`ShowWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-showwindow) | Changes the window's show state. Flags such as `SW_SHOW` activate the window, while `SW_SHOWNOACTIVATE` and `SW_SHOWNA` show it without activation. |
| [`SetActiveWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setactivewindow) | Activates a window attached to the calling thread's message queue. It does not bring a background application to the foreground. |
| [`SetForegroundWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setforegroundwindow) | Requests foreground activation. Windows can deny the request under its foreground-lock rules. |

An app cannot force foreground activation while the user is working in
another app. If Windows denies `SetForegroundWindow`, it can flash the taskbar
button instead. A WinUI API that requests foreground activation must preserve
these rules and must not promise success.

# Goals

- Give WinUI apps a clear way to show a window without taking focus.
- Give WinUI apps a clear way to request foreground activation.
- Align with WPF where that produces a usable and compatible WinUI API.
- Preserve Windows foreground-lock behavior.
- Avoid changing existing app behavior without an explicit compatibility plan.

# Non-goals

- Do not bypass Windows foreground-lock rules.
- Do not guarantee that a background window becomes foreground.
- Do not silently change `Window.Activate()` until the compatibility impact is
  understood.
- Do not approve a final API shape in this initial draft.

# Proposed direction

The current direction is to add the missing WPF-style show surface before
changing foreground activation behavior:

```csharp
namespace Microsoft.UI.Xaml
{
    unsealed runtimeclass Window
    {
        Boolean ShowActivated { get; set; };
        void Show();
    }
}
```

`ShowActivated` would default to `true`. Apps could set it to `false` before
the first call to `Show()` to display a window without activating it.

This gives WinUI a dedicated show operation and removes the main reason that
apps must use `Activate()` for first-time display.

The exact future of `Window.Activate()` remains an API review question. Options
include:

1. Keep its current behavior for compatibility and add a separate fallible
   foreground API.
2. Change it in a future compatibility boundary to follow WPF and
   `SetForegroundWindow`.
3. Deprecate it after adding clearer show and foreground APIs.

# Alternatives considered

## Change Window.Activate

Change the existing method to call `SetForegroundWindow`.

This matches the name and WPF behavior, but it can break apps that currently
call `Activate()` only because WinUI lacks `Show()`. Those apps could start
taking focus unexpectedly.

## Add Window.SetForeground

Add a method that directly represents `SetForegroundWindow` and returns
whether Windows accepted the request.

```csharp
Boolean SetForeground();
```

This is explicit and preserves the current `Activate()` behavior. However,
WPF does not expose this API shape, and adding it without first addressing
WinUI's missing show API moves the frameworks further apart.

If this option is selected, `TrySetForeground` may be a clearer name because
failure is a normal result under Windows foreground-lock rules.

## Add only Window.Show

Add `Show()` but no `ShowActivated` property.

This is simpler, but it does not give apps a framework-level way to show tool
windows, palettes, or notifications without activation. WPF and Win32 both
support that scenario.

# API behavior

## Window.Show

Shows the window and returns immediately.

On the first show, WinUI uses `ShowActivated` to decide whether to request
activation. Later calls show or restore the window using the same setting.

Calling `Show()` after the window has closed produces the same error behavior
as calling `Activate()` after close.

## Window.ShowActivated

Gets or sets whether `Show()` activates the window.

The default value is `true`. The property must be set before the first call to
`Show()`. Changing it after the window is first shown has no effect.

```csharp
Window toolWindow = new();
toolWindow.ShowActivated = false;
toolWindow.Show();
```

# Open questions

- Should WinUI match WPF exactly by adding both `Show()` and `ShowActivated`?
- Should `ShowActivated` apply only to the first show, like WPF, or to every
  call to `Show()`?
- Should `Window.Activate()` eventually use `SetForegroundWindow`, remain
  unchanged, or be deprecated?
- If compatibility prevents changing `Activate()`, should the foreground API
  be named `SetForeground`, `TrySetForeground`, or `TryActivate`?
- How should the API report foreground denial without treating normal Windows
  behavior as an exception?
- Should a foreground API restore or show a hidden or minimized window, or
  only operate on an already visible window?
- Are `Show()` and `ShowActivated` desktop-only, or can they have meaningful
  behavior for UWP?

# API review status

This API is not approved. This draft is meant to gather public feedback before
the implementation or API shape is finalized.
