Window showing and activation
===

- [Window showing and activation](#window-showing-and-activation)
- [1. Background](#1-background)
  - [1.1. WPF behavior](#11-wpf-behavior)
  - [1.2. Win32 behavior](#12-win32-behavior)
  - [1.3. API summary](#13-api-summary)
  - [1.4. Goals](#14-goals)
  - [1.5. Non-goals](#15-non-goals)
- [2. Conceptual pages (How To)](#2-conceptual-pages-how-to)
- [3. Examples](#3-examples)


# 1. Background

_Note: This section is background to help you read the spec. It does NOT go to
the online docs._

**TL;DR**: The plan in this doc is:
- Add **Show()**  (match WPF)
- Add **ShowActivated** prop  (match WPF)
- Add **TrySetForeground()**  (works like WPF's "Activate")
- Deprecate **Activate()**


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

## 1.1. WPF behavior

WPF has three relevant members:

| API | Behavior |
| --- | --- |
| [`Window.Show()`](https://learn.microsoft.com/dotnet/api/system.windows.window.show) | Shows a modeless window. |
| [`Window.ShowActivated`](https://learn.microsoft.com/dotnet/api/system.windows.window.showactivated) | Controls whether the window is activated when it is first shown. The default is `true`. Apps set it to `false` before `Show()` to show without activation. |
| [`Window.Activate()`](https://learn.microsoft.com/dotnet/api/system.windows.window.activate) | Attempts to bring an existing window to the foreground and activate it. It returns whether activation succeeded and follows the Win32 `SetForegroundWindow` rules. |


## 1.2. Win32 behavior

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

## 1.3. API summary

The new surface keeps activation and foreground as separate, clearly named operations:

| API | Win32 call | What it does | Can Windows deny it? |
| --- | --- | --- | --- |
| `Show()` with `ShowActivated = true` | `ShowWindow` (`SW_SHOW` family) | Shows the window and activates it at show time | No explicit foreground request |
| `Show()` with `ShowActivated = false` | `ShowWindow` (`SW_SHOWNOACTIVATE` family) | Shows the window without activating it | n/a |
| `TrySetForeground()` | `SetForegroundWindow` | Requests system-wide foreground for an already-shown window | Yes, returns `false` if denied |
| `Activate()` (legacy) | `ShowWindow` + `SetActiveWindow` | Shows and selects the thread's active window | No, but cannot take foreground from another app |

## 1.4. Goals

- Give WinUI apps a clear way to show a window without taking focus.
- Give WinUI apps a clear way to request foreground activation.
- Align with WPF where that produces a usable and compatible WinUI API.
- Preserve Windows foreground-lock behavior.
- Preserve the existing `Window.Activate()` behavior for compatibility.

## 1.5. Non-goals

- Do not bypass Windows foreground-lock rules.
- Do not guarantee that a background window becomes foreground.
- Do not change the behavior of `Window.Activate()`.

# 2. Conceptual pages (How To)

_Note: This is conceptual documentation that WILL go to the online "how to"
page on learn.microsoft.com. It may extend, for example, the 
[Manage app windows](https://learn.microsoft.com/windows/apps/develop/ui/manage-app-windows)
conceptual page rather than starting a new one._

A WinUI `Window` represents a top-level window on the desktop. Showing that
window and bringing it to the foreground are two separate operations, and these
new APIs keep them apart so you can pick the behavior you want.

- Call `Show()` to display a window. By default it also activates the window.
- Set `ShowActivated = false` before `Show()` when you do not want the window
  to steal focus. This is the right choice for non-interrupting windows such as
  overlays, toasts, splash screens, search palettes, and tool windows.
- Call `TrySetForeground()` to ask Windows to bring an already-shown window to the
  foreground. Windows can say no, so treat the return value as "maybe".
- `Activate()` is the old call that showed the window and activated it together.
  It still works, but it is deprecated. New code should use `Show()`.

# 3. Examples

_Note: These examples WILL go to the online "how to" page on
learn.microsoft.com._

Show a window the normal way. It activates by default:

````csharp
Window window = new();
window.Show();
```

Show a tool window without stealing focus:

```csharp
Window toolWindow = new();
toolWindow.ShowActivated = false;
toolWindow.Show();
```

Ask Windows to bring an existing window to the foreground, and handle the case
where Windows keeps the current foreground window:

```csharp
window.Show();

if (!window.TrySetForeground())
{
    // Windows kept the current foreground window.
    // For example, you could flash the taskbar button instead.
}
```

# 4. API Pages

_Note: Each of the following sections WILL become a page on
learn.microsoft.com._

## 4.1. Window.Show

Shows the window and returns immediately.

When `Show()` changes a hidden window to visible, WinUI uses the current
`ShowActivated` value to decide whether the operation should activate it.

If the window is already visible, `Show()` does nothing. This includes a
minimized window, which is still visible and must be restored through the
window-state APIs.

Calling `Show()` after the window has closed produces the same error behavior
as calling `Activate()` after close.

## 4.2. Window.ShowActivated

Gets or sets whether `Show()` activates the window.

The default value is `true`. Set it to `false` before calling `Show()` to show
the window without activation. Changing the property does not retroactively
activate or deactivate a window that is already visible, but the new value is
used after a later `Hide()` and `Show()` sequence. As in WPF, the value is also
used when a minimized window is restored through the window-state APIs.

ShowActivated only chooses the `ShowWindow` activation flag (an `SW_SHOW`-family
flag when true, an `SW_SHOWNOACTIVATE`-family flag when false). It does not call
`SetForegroundWindow`, which is why showing and foreground activation are named
differently.

```csharp
Window toolWindow = new();
toolWindow.ShowActivated = false;
toolWindow.Show();
```

## 4.3. Window.TrySetForeground

Asks Windows to make the window the foreground window.

WinUI calls `SetForegroundWindow` and returns `true` when Windows accepts the
request. It returns `false` when Windows denies the request under its normal
foreground-lock rules.

`TrySetForeground()` does not bypass Windows restrictions and does not treat a
denied request as an exception. It does not show or restore the window. Apps
should show a hidden window or restore a minimized window through the
appropriate window API before calling it.

```csharp
window.Show();

if (!window.TrySetForeground())
{
    // Windows kept the current foreground window.
}
```

## 4.4. Window.Activate

`Window.Activate()` keeps its existing `ShowWindow`, `UpdateWindow`, and
`SetActiveWindow` behavior.

The API is deprecated. New code can safely switch to use `Show()`.


# 5. API Details

_Note: This is the API definition (MIDL). The doc comments in it feed
Intellisense and the online docs, but the block itself is a spec reference and
does NOT go to the online docs as-is._

```csharp
namespace Microsoft.UI.Xaml
{
    unsealed runtimeclass Window
    {
        Boolean ShowActivated { get; set; };
        void Show();
        Boolean TrySetForeground();

        // Existing, will be deprecated.  Behavior does not change.
        void Activate();
    }
}
```

# 6. Appendix

_Note: This section is a spec aid for design notes. It does NOT go to the
online docs._

## 6.1. Alternatives considered

- **Keep `TryActivate()`.** This stays closest to WPF's `Window.Activate()`
  name, but it collides with WinUI's existing `Activate()` (which only does
  thread-level `SetActiveWindow`). Developers would read `TryActivate()` as a
  bool-returning `Activate()`, when it actually adds the fallible cross-process
  `SetForegroundWindow`. Same word, stronger behavior, right next to each other.

- **Change `Window.Activate()` to call `SetForegroundWindow`.** This matches WPF
  behavior, but it silently changes shipped WinUI behavior and removes the
  ability to show a window quietly.

- **Add only `Window.Show()`.** Simpler, but it gives apps no framework-level
  way to request foreground for a window that is already shown.

We chose `TrySetForeground()` because it names the exact Win32 operation
(`SetForegroundWindow`), the `Try`/`bool` shape makes denial a normal outcome,
and it keeps 'foreground' distinct from the thread-level 'activate' vocabulary.

## 6.2. API review status

This API is not approved. This draft is meant to gather public feedback before
the implementation or API shape is finalized.
````