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

Today, an app creates a WinUI `Window` and calls `Window.Activate()` to show
it. In response, WinUI calls `ShowWindow` to show or restore the backing HWND,
`UpdateWindow` to paint it, and `SetActiveWindow` to make it the active window
for the calling thread.

The important distinction is between an *active window* and the *foreground
window*.

[`SetActiveWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setactivewindow)
only activates a window attached to the calling thread's message queue. If the
app is already the foreground app, Windows also brings that window to the top
of the foreground app's Z-order. If another app is foreground,
`SetActiveWindow` does not move the calling app to the foreground. This is why
the current WinUI `Window.Activate()` can select a window within the WinUI app,
but cannot take foreground from another process.

[`SetForegroundWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setforegroundwindow)
is the Win32 request that can cross that process boundary. If Windows allows
the request, it makes the target window foreground, activates it, and directs
keyboard input to it. The request is not guaranteed to succeed.

WinUI currently uses `Activate()` for two different jobs:

- Showing a window for the first time.
- Asking an existing window to become active.

This proposal separates those jobs. It adds WPF-style APIs for showing a
window, adds an explicit fallible foreground request, and keeps the existing
`Activate()` behavior for compatibility.

## WPF behavior

WPF has three relevant members:

| API | Behavior |
| --- | --- |
| [`Window.Show()`](https://learn.microsoft.com/dotnet/api/system.windows.window.show) | Shows a modeless window. |
| [`Window.ShowActivated`](https://learn.microsoft.com/dotnet/api/system.windows.window.showactivated) | Controls whether the window is activated when it is first shown. The default is `true`. Apps set it to `false` before `Show()` to show without activation. |
| [`Window.Activate()`](https://learn.microsoft.com/dotnet/api/system.windows.window.activate) | Attempts to bring an existing window to the foreground and activate it. It returns whether activation succeeded and follows the Win32 `SetForegroundWindow` rules. |


## Win32 behavior

Win32 also separates showing, thread activation, and foreground activation:

| API | Behavior |
| --- | --- |
| [`ShowWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-showwindow) | Changes the window's show state. Flags such as `SW_SHOW` activate the window, while `SW_SHOWNOACTIVATE` and `SW_SHOWNA` show it without activation. |
| [`SetActiveWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setactivewindow) | Selects the active window for the calling thread's message queue. It brings the window to the top only when the calling app is already foreground. |
| [`SetForegroundWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setforegroundwindow) | Asks Windows to make the window foreground and active, even if another app is currently foreground. Windows can deny the request. |

Windows limits foreground changes so a background app cannot freely interrupt
the user. According to the `SetForegroundWindow` documentation, all of these
conditions must be true:

- The caller is a desktop app.
- The foreground process has not called `LockSetForegroundWindow` to block the
  request.
- No menus are active.

At least one of these conditions must also be true:

- The foreground-lock timeout has expired.
- The calling process is already foreground.
- The foreground process started the calling process.
- There is no foreground process.
- The calling process received the last input event.
- Either the calling process or foreground process is being debugged.

Windows may still deny the request even when these conditions appear to be
met. When foreground activation is denied, Windows can flash the app's taskbar
button instead. A WinUI API that requests foreground activation must preserve
these rules, must not promise success, and should expose denial as a normal
outcome rather than an exceptional failure.

# Goals

- Give WinUI apps a clear way to show a window without taking focus.
- Give WinUI apps a clear way to request foreground activation.
- Align with WPF where that produces a usable and compatible WinUI API.
- Preserve Windows foreground-lock behavior.
- Preserve the existing `Window.Activate()` behavior for compatibility.

# Non-goals

- Do not bypass Windows foreground-lock rules.
- Do not guarantee that a background window becomes foreground.
- Do not change the behavior of `Window.Activate()`.

# Proposed API

Add the missing WPF-style show surface and a separate foreground request:

```csharp
namespace Microsoft.UI.Xaml
{
    unsealed runtimeclass Window
    {
        Boolean ShowActivated { get; set; };
        void Show();
        Boolean TryActivate();

        // Existing, will be deprecated.  Behavior does not change.
        void Activate();
    }
}
```

These APIs handle two different points in the window lifecycle:

- `ShowActivated` controls whether showing a hidden window also activates it.
- `TryActivate()` asks Windows to activate a window after it has been shown.

The show-time option is needed for non-interrupting windows such as overlays,
toasts, splash screens, search palettes, and tool windows. Public WPF apps use
`ShowActivated = false` for these scenarios.

# API behavior

## Window.Show

Shows the window and returns immediately.

When `Show()` changes a hidden window to visible, WinUI uses the current
`ShowActivated` value to decide whether the operation should activate it.

If the window is already visible, `Show()` does nothing. This includes a
minimized window, which is still visible and must be restored through the
window-state APIs.

Calling `Show()` after the window has closed produces the same error behavior
as calling `Activate()` after close.

## Window.ShowActivated

Gets or sets whether `Show()` activates the window.

The default value is `true`. Set it to `false` before calling `Show()` to show
the window without activation. Changing the property does not retroactively
activate or deactivate a window that is already visible, but the new value is
used after a later `Hide()` and `Show()` sequence. As in WPF, the value is also
used when a minimized window is restored through the window-state APIs.

```csharp
Window toolWindow = new();
toolWindow.ShowActivated = false;
toolWindow.Show();
```

## Window.TryActivate

Asks Windows to make the window foreground and active.

WinUI calls `SetForegroundWindow` and returns `true` when Windows accepts the
request. It returns `false` when Windows denies the request under its normal
foreground-lock rules.

`TryActivate()` does not bypass Windows restrictions and does not treat a
denied request as an exception. It does not show or restore the window. Apps
should show a hidden window or restore a minimized window through the
appropriate window API before calling it.

```csharp
window.Show();

if (!window.TryActivate())
{
    // Windows kept the current foreground window.
}
```

## Window.Activate

`Window.Activate()` keeps its existing `ShowWindow`, `UpdateWindow`, and
`SetActiveWindow` behavior.

The API is deprecated. New code can safely switch to use `Show()`.


# Appendix: Alternatives considered

- **Change `Window.Activate()` to call `SetForegroundWindow`.** This matches
  the name and WPF behavior, but then WinUI apps do not have the ability to
  show a window normally or quietly without jumping to the foreground.

- **Add `Window.SetForeground()`.** This directly represents
  `SetForegroundWindow`, but the operation can be denied during normal use and
  "foreground" exposes Win32 terminology that WPF presents as activation. This
  proposal uses `TryActivate()` to make the fallibility clear while staying
  closer to WPF naming.

- **Add only `Window.Show()`.** This is simpler, but it does not give apps a
  framework-level way to show tool windows, palettes, or notifications without
  activation. WPF and Win32 both support that scenario.

# API review status

This API is not approved. This draft is meant to gather public feedback before
the implementation or API shape is finalized.
