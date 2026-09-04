Window showing and activation
===

> **Status: Placeholder**.  This API has not yet been reviewed and approved.

- [Window showing and activation](#window-showing-and-activation)
- [1. Background](#1-background)
  - [1.1. WPF behavior](#11-wpf-behavior)
  - [1.2. Win32 behavior](#12-win32-behavior)
  - [1.3. API summary](#13-api-summary)
- [2. Conceptual pages (How To)](#2-conceptual-pages-how-to)
- [3. Examples](#3-examples)
- [4. API Pages](#4-api-pages)
  - [4.1. Window.TrySetForeground](#41-windowtrysetforeground)
  - [4.2. Window.Activate](#42-windowactivate)
- [5. API Details](#5-api-details)
- [6. Appendix](#6-appendix)
  - [6.1. Alternatives considered](#61-alternatives-considered)
  - [6.2. Future directions](#62-future-directions)


# 1. Background

_Note: This section is background to help you read the spec. It does NOT go to
the online docs._

**TL;DR**: The plan in this doc is:
- Add **TrySetForeground()** for the case where an already-shown window needs to
  come forward, such as a single-instance relaunch. It asks Windows for the
  foreground, and Windows can still say no.
- Keep **Activate()** exactly as it is today, because existing apps rely on how
  it behaves now.

(An earlier draft also added `Show()` and a `ShowActivated` property to match
WPF. We cut those to keep this change small and low-risk. See the appendix.)

[Issue #7595](https://github.com/microsoft/microsoft-ui-xaml/issues/7595)
reports that `Window.Activate()` does not bring a background WinUI window to
the foreground unless the window is minimized. It is a long-standing request,
open since 2022.

The main scenario is **single-instancing**. A single-instance app wants a second
launch to bring the already-running window forward. The Windows App SDK already
has most of the plumbing for this. The second instance calls
[`AppInstance.RedirectActivationToAsync`](https://learn.microsoft.com/windows/apps/windows-app-sdk/applifecycle/applifecycle-instancing),
which is designed to grant foreground rights to the running instance (it calls
`AllowSetForegroundWindow` under the hood) before it raises that instance's
`Activated` event. (You can see this in the WindowsAppSDK source,
`dev/AppLifecycle/AppInstance.cpp`.)

That makes the running instance's `Activated` handler the right place to come
forward. The missing piece is a WinUI call that actually does it. `Activate()`
will not: it calls `SetActiveWindow`, which only activates a window on the
calling thread and cannot pull the foreground from another thread or process,
which is exactly the single-instance case. There is no WinUI API that crosses
that boundary, and `AppWindow` has no foreground method either. So Microsoft's
own
[single-instance walkthrough](https://learn.microsoft.com/windows/apps/windows-app-sdk/applifecycle/applifecycle-single-instance)
drops to a raw Win32 `SetForegroundWindow` P/Invoke to finish the job.
Sections 1.1 and 1.2 have the details.

This proposal adds that one missing call: `TrySetForeground()`, the managed API
for that same `SetForegroundWindow`. Put it in the `Activated` handler instead of
the P/Invoke. It is still a request that Windows can deny, so it returns a
`bool`. `Activate()` stays exactly as it is today.

## 1.1. WPF behavior

WPF has three relevant members:

| API | Behavior |
| --- | --- |
| [`Window.Show()`](https://learn.microsoft.com/dotnet/api/system.windows.window.show) | Shows a modeless window. |
| [`Window.ShowActivated`](https://learn.microsoft.com/dotnet/api/system.windows.window.showactivated) | Controls whether the window is activated when it is first shown. The default is `true`. Apps set it to `false` before `Show()` to show without activation. |
| [`Window.Activate()`](https://learn.microsoft.com/dotnet/api/system.windows.window.activate) | Attempts to bring an existing window to the foreground and activate it. It returns whether activation succeeded and follows the Win32 `SetForegroundWindow` rules. |

WPF's naming is a little odd: `Activate()` actually calls `SetForegroundWindow`,
so the method named "activate" is the one that pulls the window to the front.

WinUI's `Activate()` looks the same but only does thread-level activation
(`SetActiveWindow`), so it cannot pull the foreground from another thread or
process. Same name as WPF, less power. That gap is what `TrySetForeground()`
fills. Section 1.2 has the two levels of "activate" in Win32.

## 1.2. Win32 behavior

Win32 also separates showing, thread activation, and foreground activation:

| API | Behavior |
| --- | --- |
| [`ShowWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-showwindow) | Changes the window's show state. Flags such as `SW_SHOW` activate the window, while `SW_SHOWNOACTIVATE` and `SW_SHOWNA` show it without activation. |
| [`SetActiveWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setactivewindow) | Selects the active window for the calling thread's message queue. It cannot take the foreground for a background app; the window comes to the top only when the app is already in the foreground. |
| [`SetForegroundWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setforegroundwindow) | Asks Windows to make the window foreground and active, even when the current foreground window is on another thread or process. Windows can deny the request. |

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

| API | Win32 call | What it does | Foreground request? |
| --- | --- | --- | --- |
| `TrySetForeground()` | `SetForegroundWindow` | Requests system-wide foreground for an already-shown window | Yes. Windows can deny it, and it returns `false`. |
| `Activate()` (unchanged) | `ShowWindow` + `SetActiveWindow` | Shows the window and sets it active on its own thread | No. It does not request foreground, so it cannot pull the window in front of another app. |


# 2. Conceptual pages (How To)

_Note: This is conceptual documentation that WILL go to the online "how to"
page on learn.microsoft.com. It may extend, for example, the
[Manage app windows](https://learn.microsoft.com/windows/apps/develop/ui/manage-app-windows)
conceptual page rather than starting a new one._

A WinUI `Window` represents a top-level window on the desktop. Showing a window
and bringing it to the foreground are two separate operations, and neither one
is guaranteed to leave your window active and in front. Windows has the final
say.

**`Activate()`** calls the Win32
[`ShowWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-showwindow)
and
[`SetActiveWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setactivewindow)
functions to show the window and mark it active on its own thread. What
the user sees depends on who has the foreground:

- If your app already has the foreground, the window comes up front and active,
  the way you would expect.
- If another process has the foreground, `Activate()` does not pull your window
  in front of it. The window still shows, but it stays behind the active app and
  does not steal focus. So "activate" does not always mean "comes to the front."

**`TrySetForeground()`** calls the Win32
[`SetForegroundWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setforegroundwindow)
function to ask Windows to make the window the foreground window.
Windows grants it only when your app has a claim to the foreground, for example
it is already foreground, it launched the current foreground app, it got the
last input event, or a single-instance redirect just handed it foreground
rights.

When Windows says no:

- The foreground does not change and your window stays where it was.
- Windows flashes your taskbar button to signal that you want attention.
- The call returns `false`.

`TrySetForeground()` also does nothing to a hidden or minimized window on its
own. Call `Activate()` to show a hidden window, or use the window's
[`OverlappedPresenter.Restore()`](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.windowing.overlappedpresenter.restore)
to bring back a minimized one, then call `TrySetForeground()`.

A key scenario for `TrySetForeground()` is when an app is a single-instance app:
after a second launch redirects to it, the running instance calls
`TrySetForeground()` to come forward. See the
[single-instance walkthrough](https://learn.microsoft.com/windows/apps/windows-app-sdk/applifecycle/applifecycle-single-instance)
for the redirect pattern.

These behaviors come straight from the Win32 functions underneath. See the
[`SetForegroundWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setforegroundwindow)
and
[`SetActiveWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setactivewindow)
reference pages for more detail.

# 3. Examples

_Note: These examples WILL go to the online "how to" page on
learn.microsoft.com._

**Bring a single-instance app forward.** A single-instance app redirects a
second launch to the instance that is already running (see the
[single-instance walkthrough](https://learn.microsoft.com/windows/apps/windows-app-sdk/applifecycle/applifecycle-single-instance)).
The redirect grants foreground rights to the running instance, then raises its
`Activated` event. That handler is the right place to come forward. Restore the
window first if it is minimized, then call `TrySetForeground()`. Windows can
still deny it, so check the return value:

```csharp
private void OnActivated(object sender, AppActivationArguments args)
{
    // The user launched a second copy, and it redirected here.
    // The redirect granted us foreground rights, but Windows can still say no.
    if (!_mainWindow.TrySetForeground())
    {
        // Windows kept the current foreground window.
    }
}
```

**Handle a denied request.** Windows can keep the current foreground window, so
check the return value:

```csharp
if (!window.TrySetForeground())
{
    // Windows kept the current foreground window and flashed our taskbar
    // button to signal that we want attention. There is usually nothing
    // more to do here.
}
```

# 4. API Pages

_Note: Each of the following sections WILL become a page on
learn.microsoft.com._

## 4.1. Window.TrySetForeground

Asks Windows to make the window the foreground window. You can call it on any
window that is already shown, but calling it does not guarantee success. Whether
Windows grants the request is up to the
[`SetForegroundWindow`](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setforegroundwindow)
rules: your app needs a current claim to the foreground, so a call out of the
blue while another app is in front will be denied. A common case where the
request is granted is right after `AppInstance.RedirectActivationToAsync`, which
hands the running instance foreground rights before it raises the `Activated`
event.

WinUI calls `SetForegroundWindow` and returns `true` when Windows accepts the
request. It returns `false` when Windows denies it under the normal
foreground-lock rules, the same rules any `SetForegroundWindow` caller follows.
A denied request is a normal outcome, not an exception.

`TrySetForeground()` does not show or restore the window. Show a hidden window or
restore a minimized one through the appropriate window API before calling it.

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
  API. We rejected it because (a) it's complicated to change the behavior
  of an existing API and (b) aligning the concepts of "Activate" and "Foreground"
  with Win32 is the simplest model long-term.

- **Keep `TryActivate()`.** This stays closest to WPF's `Window.Activate()`
  name, and avoids the compat issue, but again doesn't match the function at the
  Win32 level.

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

  Open question if we ever build it: match WPF with a `ShowActivated` property, or
  take the activated flag as a parameter (`Show(Boolean activated)`)? A parameter
  reads more clearly than a separate property you have to set first, and
  `AppWindow.Show(Boolean)` already uses that shape. WPF likely used a property so
  the common case (`ShowActivated = true`) needs no argument at the `Show()` call.
