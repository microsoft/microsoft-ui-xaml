# Issue #9524: Window.Visible throws when the window is closed

## Symptom

After `Window.Close()`, reading `Window.Visible` throws `E_INVALID_OPERATION` (the test saw HRESULT `0x800710DD`).
There's no public "is closed" property, so apps can't check the window state before reading `Visible`.
They have to track the closed state themselves or catch the exception.

It also throws inside the close-time `VisibilityChanged` handler. `Close` raises `VisibilityChanged(false)`,
but if the handler reads `sender.Visible` to check, it gets an exception instead of `false`.

## Root cause

| Step | Evidence |
|---|---|
| `get_VisibleImpl` called `CheckIsWindowClosed()` first. | `dxaml\xcp\dxaml\lib\DesktopWindowImpl.cpp:263` (before the fix) |
| `CheckIsWindowClosed` fails with `E_INVALID_OPERATION` / `ERROR_WINDOW_DESKTOP_ALREADY_CLOSED` once `m_bIsClosed` is set. | `DesktopWindowImpl.cpp:2177-2181` |
| `Close` sets `m_bIsClosed = true` before it raises `VisibilityChanged(FALSE)`. So the handler also hits the throw. | `DesktopWindowImpl.cpp:591` (closed), `:594-599` (event) |
| `Close` then calls `Shutdown()`, which destroys the HWND. | `DesktopWindowImpl.cpp:609` |

## Fix

In `DesktopWindowImpl::get_VisibleImpl`, drop the `CheckIsWindowClosed()` call and return false when closed:

```cpp
*pValue = !m_bIsClosed && !IsIconic(m_hwnd.get()) && IsWindowVisible(m_hwnd.get());
```

`m_bIsClosed` is checked first. That way the getter never calls `IsIconic`/`IsWindowVisible` on an HWND that is
being destroyed or is already gone.

## Why this fixes it

- After `Close`, `m_bIsClosed` is true, so the getter returns `S_OK` with `false`. No throw.
- Inside the close-time `VisibilityChanged(false)` handler, `m_bIsClosed` is already true (`:591` runs before `:598`).
  So the handler sees `Visible == false`, which matches the event args.
- The new test shows this. Before the fix, it failed in all 3 states with `Window.Visible threw hr=0x800710dd`.
  After the fix, it passes.

## Why it's safe

- **Only the closed case changes.** For an open window, the expression is the same as before:
  `!IsIconic && IsWindowVisible`.
- **Closing but not yet closed is unchanged.** `m_bIsClosing` is set at `:545`, and `Closed` handlers run before
  `m_bIsClosed` is set. A handler can cancel the close (`:553-559`). During that time the window is still live, so
  `Visible` still reports the real HWND state. That's the same as before, because `CheckIsWindowClosed` only
  checks `m_bIsClosed`.
- **There's precedent.** Several `DesktopWindowImpl` getters already work after close without
  `CheckIsWindowClosed`: `Dispatcher` (`:331`), `Width`/`Height` (`:704`, tested by the existing
  `WindowWidthHeightAfterClose`), and `AppWindow` (`:2280`).
- **Compat risk is low.** The only code that breaks is code that relied on the exception to detect a closed window.
  Returning false is what the issue asks for, and it matches the last `VisibilityChanged` value the app saw.
- **UWP path is not touched.** `UWPWindowImpl::get_VisibleImpl` is unchanged. UWP windows aren't used in WinUI 3.

## Test

`WindowIntegrationTests::WindowVisibleAfterClose` in
`dxaml\test\native\external\controls\window\WindowIntegrationTests.cpp`.

It covers 3 states: never activated, activated, and minimized. For each one it:

1. Checks `Visible` before close (true only when activated).
2. Closes the window, with a `VisibilityChanged` handler that reads `sender.Visible`.
3. Checks that `Visible` on the closed window returns false without throwing.
4. Checks the close-time event count: 1 for activated, 0 for minimized. A never-activated window also gets the
   event (`m_bMinimizedOrHidden` starts false, `DesktopWindowImpl.h:228`). That's existing behavior, so the test
   doesn't check that count.
5. Checks that the handler did not throw and saw `Visible == false`.

| Run | Result |
|---|---|
| Pre-fix, `*WindowIntegrationTests::WindowVisibleAfterClose` | Failed in all 3 states: `Window.Visible threw hr=0x800710dd` |
| Post-fix, `*WindowIntegrationTests::Window*AfterClose*` | 4/4 passed (new test + 3 existing after-close tests) |
