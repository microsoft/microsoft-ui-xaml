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
- [4. API Pages](#4-api-pages)
- [5. API Details](#5-api-details)
- [6. Appendix](#6-appendix)


# 1. Background

_Note: This section is background to help you read the spec. It does NOT go to
the online docs._

**TL;DR**: The plan in this doc is:
- Add **TrySetForeground()**  (works like WPF's "Activate")
- Keep **Activate()** exactly as it is today

(An earlier draft also added `Show()` and a `ShowActivated` property to match
WPF. We cut those to keep this change small and low-risk. See the appendix.)


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

There is no WinUI API that asks Windows to bring an already-shown window to the
foreground across the process boundary. `Activate()` only does thread-level
activation, and `AppWindow` has no foreground method either.

This proposal adds one explicit, fallible foreground request,
`TrySetForeground()`, and leaves the existing `Activate()` behavior exactly as
it is today.

## 1.1. WPF behavior

WPF has three relevant members:

| API | Behavior |
| --- | --- |
| [`Window.Show()`](https://learn.microsoft.com/dotnet/api/system.windows.window.show) | Shows a modeless window. |
| [`Window.ShowActivated`](https://learn.microsoft.com/dotnet/api/system.windows.window.showactivated) | Controls whether the window is activated when it is first shown. The default is `true`. Apps set it to `false` before `Show()` to show without activation. |
| [`Window.Activate()`](https://learn.microsoft.com/dotnet/api/system.windows.window.activate) | Attempts to bring an existing window to the foreground and activate it. It returns whether activation succeeded and follows the Win32 `SetForegroundWindow` rules. |

Only `Window.Activate()` is in scope for this proposal. `Show()` and
`ShowActivated` are listed under future directions in the appendix.

## 1.2. Win32 behavior

Win32 also separates showing, thread activation, and foreground activation:

| API | Behavior |
| --- | --- |
| [`ShowWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-showwindow) | Changes the window's show state. Flags such as `SW_SHOW` activate the window, while `SW_SHOWNOACTIVATE` and `SW_SHOWNA` show it without activation. |
| [`SetActiveWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setactivewindow) | Selects the active window for the calling thread's message queue. It brings the window to the top only when the calling app is already foreground. |
| [`SetForegroundWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setforegroundwindow) | Asks Windows to make the window foreground and active, even if another app is currently foreground. Windows can deny the request. |

Windows limits foreground changes so a background app cannot freely interrupt
the user. Roughly, the request is allowed when the calling app already has some
claim to the foreground (for example it is already foreground, it launched the
current foreground app, or it received the last input event) and no app has
locked the foreground. The `SetForegroundWindow` documentation has the full list
of conditions.

Even when the conditions look right, Windows can still deny the request, and it
may flash the app's taskbar button instead. So a WinUI API that requests
foreground activation must follow these rules, must not promise success, and
should treat denial as a normal outcome rather than an error.

## 1.3. API summary

The new API names the foreground request as its own operation, separate from the
existing thread-level activate:

| API | Win32 call | What it does | Can Windows deny it? |
| --- | --- | --- | --- |
| `TrySetForeground()` | `SetForegroundWindow` | Requests system-wide foreground for an already-shown window | Yes, returns `false` if denied |
| `Activate()` (unchanged) | `ShowWindow` + `SetActiveWindow` | Shows and selects the thread's active window | No, but cannot take foreground from another app |

## 1.4. Goals

- Give WinUI apps a clear way to request foreground activation.
- Preserve Windows foreground-lock behavior.
- Preserve the existing `Window.Activate()` behavior for compatibility.
- Keep the change small and low-risk.

## 1.5. Non-goals

- Do not bypass Windows foreground-lock rules.
- Do not guarantee that a background window becomes foreground.
- Do not change the behavior of `Window.Activate()`.
- Do not add `Show()` or a `ShowActivated` property in this proposal.
  `AppWindow.Show(Boolean)` already shows a window with or without activation, so
  that story is deferred. See the appendix.

# 2. Conceptual pages (How To)

_Note: This is conceptual documentation that WILL go to the online "how to"
page on learn.microsoft.com. It may extend, for example, the
[Manage app windows](https://learn.microsoft.com/windows/apps/develop/ui/manage-app-windows)
conceptual page rather than starting a new one._

A WinUI `Window` represents a top-level window on the desktop. Showing a window
and bringing it to the foreground are two separate operations.

- Call `Activate()` to show and activate a window, the same as always.
- Call `TrySetForeground()` to ask Windows to bring an already-shown window to
  the foreground. Windows can say no, so treat the return value as "maybe". This
  is what you want when a background window needs the user's attention, which is
  the case WinUI's thread-level `Activate()` cannot handle on its own.

# 3. Examples

_Note: These examples WILL go to the online "how to" page on
learn.microsoft.com._

Show and activate a window the normal way:

```csharp
Window window = new();
window.Activate();
```

Later, ask Windows to bring that window to the foreground, and handle the case
where Windows keeps the current foreground window:

```csharp
if (!window.TrySetForeground())
{
    // Windows kept the current foreground window.
    // For example, you could flash the taskbar button instead.
}
```

# 4. API Pages

_Note: Each of the following sections WILL become a page on
learn.microsoft.com._

## 4.1. Window.TrySetForeground

Asks Windows to make the window the foreground window.

WinUI calls `SetForegroundWindow` and returns `true` when Windows accepts the
request. It returns `false` when Windows denies the request under its normal
foreground-lock rules.

`TrySetForeground()` does not bypass Windows restrictions and does not treat a
denied request as an exception. It does not show or restore the window. Apps
should show a hidden window or restore a minimized window through the
appropriate window API before calling it.

```csharp
if (!window.TrySetForeground())
{
    // Windows kept the current foreground window.
}
```

## 4.2. Window.Activate

`Window.Activate()` keeps its existing `ShowWindow`, `UpdateWindow`, and
`SetActiveWindow` behavior. This proposal does not change it and does not
deprecate it.

Use `TrySetForeground()` when you need to request foreground for a window that
is already shown.


# 5. API Details

_Note: This is the API definition (MIDL). The doc comments in it feed
Intellisense and the online docs, but the block itself is a spec reference and
does NOT go to the online docs as-is._

```csharp
namespace Microsoft.UI.Xaml
{
    unsealed runtimeclass Window
    {
        Boolean TrySetForeground();

        // Existing.  Behavior does not change and it is not deprecated.
        void Activate();
    }
}
```

# 6. Appendix

_Note: This section is a spec aid for design notes. It does NOT go to the
online docs._

## 6.1. Alternatives considered

- **Change `Window.Activate()` to call `SetForegroundWindow`.** This is the
  tempting one, because WPF's `Window.Activate()` already calls
  `SetForegroundWindow` and returns a `bool`. Matching WPF here would fix
  [#7595](https://github.com/microsoft/microsoft-ui-xaml/issues/7595) with no new
  API. We rejected it because it silently changes the behavior of every existing
  WinUI `Activate()` call, including the one in every default project template,
  and because `Activate()` returns `void` so it still could not report a denied
  request. `TrySetForeground()` honors the same intent (a fallible foreground
  request) without changing a shipped method.

- **Keep `TryActivate()`.** This stays closest to WPF's `Window.Activate()`
  name, but it collides with WinUI's existing `Activate()` (which only does
  thread-level `SetActiveWindow`). Developers would read `TryActivate()` as a
  bool-returning `Activate()`, when it actually adds the fallible cross-process
  `SetForegroundWindow`. Same word, stronger behavior, right next to each other.

We chose `TrySetForeground()` because it names the exact Win32 operation
(`SetForegroundWindow`), the `Try`/`bool` shape makes denial a normal outcome,
and it keeps 'foreground' distinct from the thread-level 'activate' vocabulary.

## 6.2. Future directions

Out of scope here, but natural follow-ups:

- **A `Show()` method and a `ShowActivated` property on `Window` (match WPF).**
  This would give a WPF-style way to show a window and choose whether it
  activates. Setting `ShowActivated = false` before `Show()` would show a window
  without stealing focus, which suits overlays, toasts, splash screens, and tool
  windows:

  ```csharp
  Window toolWindow = new();
  toolWindow.ShowActivated = false;
  toolWindow.Show();
  ```

  We left it out for now because `AppWindow.Show(Boolean)` already covers most of
  this. If there is demand for it directly on `Window`, it can be its own
  proposal.

## 6.3. API review status

This API is not approved. This draft is meant to gather public feedback before
the implementation or API shape is finalized.