# Automatic window placement (Window.PlacementId)

Automatic window placement (also called "persistence") is WinUI feature that lets the app author
easily configure the WinUI window so that it opens where the user left it.

- [API spec](https://github.com/microsoft/microsoft-ui-xaml/pull/12371) currently in progress.

This feature is built on [PlacementEx](../../external/inc/PlacementEx/RememberingWindowPositions.md), a C++
header library to support this scenario.  We expect to update PlacementEx occasionally if there are new changes,
there's no roadmap for that yet.  Note that PlacementEx uses an API that's on Win11 but not Win10 (ApplyWindowAction),
so be sure to test changes in this area downlevel.

## Staging plan

We're working on the implementation in multiple phases, though there's no guarantee we'll get to every item:

1. Phase 1: Minimum Viable Product (MVP) (near parity with UWP)
   - App can set `Window.PlacementId` and call `Window.Activate` to enable WinUI's automatic placement.
   - WinUI doesn't evict old placement entries.
   - App must have package identity.
   - Special handling for app launch and restart scenarios not supported.
   - No support for window cascading.
   - WinUI only saves window placement when the AppWindow is using an OverlappedPresenter (no FullScreen support)
   - No support for preserving snapped placement when a window is snapped, minimized, and closed without being restored.
2. Phase 2: Parity with PlacementEx.
   - Support special handling for app-restart and app-launch.
   - App can run their own placement logic before and after WinUI's.
   - App can be unpackaged (have no package identity)
   - Supports window cascading.
   - Supports AppWindow FullScreen presenter.
   - Allow more mixing of Window APIs with AppWindow and HWND, and harden against reentrancy scenarios: https://github.com/microsoft/microsoft-ui-xaml/issues/12512
3. Phase 3: Roundouts as needed.
   - WinUI automatically evict old placement entries
   - App can easily change monitor fallback logic
   - Finalize roadmap for updating PlacementEx

## Using automatic window placement

To enable in an app with package identity, just set `Window.PlacementId` and call `Activate`.

If the app has no package identity, WinUI skips automatic placement and reports the reason through
`RoOriginateError` and `OutputDebugString`. The window still opens normally.

```cs
window w = new Window() { PlacementId="MyWindow" };

// During activate, WinUI looks up the saved position, size, state for "MyWindow" and applies it.
w.Activate(); 
```

A placement id can contain at most `MAX_PATH` (260) UTF-16 code units. If you set a longer id,
WinUI returns `E_INVALIDARG` and leaves the previous id unchanged. WinUI does not truncate ids.

The first `Window.Activate` opts the window into automatic placement and attempts to restore its saved
placement. Later `Activate` calls don't restore it again. WinUI doesn't restore, track, or save placement
for a window shown only through `AppWindow.Show`. If the app calls `Window.Activate` afterward, that
first call still runs initial placement even though the window is already visible.

After `Window.Activate`, later `AppWindow.Show` or hide calls don't rerun placement.

## When do we save the placement data?

- Save on the `WM_DESTROY` and `WM_ENDSESSION` messages
- WinUI saves only after the first `Window.Activate` finishes with the window still open. Closing from
  a `SizeChanged` or `Activated` handler during that call doesn't create or overwrite a saved placement,
  even if the handler calls `Activate` again before closing.
- Saving requires a non-empty id and available storage.
- Phase 1: WinUI captures the current HWND only when its AppWindow uses an `OverlappedPresenter`.
- Save and restore are best effort; storage failures don't prevent the window from opening or closing.

## Where and how data is saved

WinUI uses `Microsoft.Windows.Storage.ApplicationData.GetDefault().LocalSettings`, in a container
named `Microsoft.UI.Xaml.WindowPlacement`. ApplicationData scopes the settings to the current
user and package. It stores in the registry.  This is a private format, not a public API.

Each key is `P_` followed by 16 lowercase hexadecimal digits representing a hash of the placement ID.
This keeps keys short and also allows us to  have case-sensitive IDs (the keys are case insensitive).
WinUI also stores and compares the full ID string, so we detect if a hash collision happens.
If WinUI does hit a hash collision, it calls `RoOriginateError` with a hash-collision message and continues.
Restoring skips the other ID's record; saving replaces it with the current ID's placement.

Internally, `TryLoadAndApply` and `Save` return an HRESULT and `PlacementResult` flags. The HRESULT reports
failures. `LoadedAndApplied` means the saved placement was loaded and applied. `HashCollision` means the
stored ID differs from the requested ID. The flags support bitwise combinations, but a colliding record is never applied.

### Version 1 record

Each value is a boxed `UInt8Array`: a 152-byte fixed header followed by the exact placement id string.
All integers and UTF-16 code units use little-endian byte order. Rectangles contain four signed
32-bit edges (`left`, `top`, `right`, `bottom`) in physical screen pixels, not workspace coordinates
or DIPs.

| Byte offset | Field | Type | Meaning |
|---|---|---|---|
| 0 | `Version` | UInt32 | `1`. |
| 4 | `SavedAt` | FILETIME (8 bytes) | UTC save time, in 100-nanosecond intervals since January 1, 1601. |
| 12 | `VirtualDesktopId` | GUID (16 bytes) | Reserved. WinUI writes all zeros and ignores this field when restoring. |
| 28 | `NormalRect` | RECT (16 bytes) | Normal outer bounds, including invisible resize borders. |
| 44 | `WorkArea` | RECT (16 bytes) | The saved display's usable area. |
| 60 | `SnapRect` | RECT (16 bytes) | Snapped visible bounds, excluding invisible resize borders. All zeros when not snapped. |
| 76 | `Dpi` | UInt32 | Window DPI at capture. |
| 80 | `State` | UInt32 | `0` Normal, `1` Maximized, `2` Minimized, `3` Snapped, `4` MinimizedFromMaximized, `5` MinimizedFromSnapped. |
| 84 | `DisplayDeviceName` | WCHAR[32] (64 bytes) | Null-terminated GDI display name with zero padding. An empty name is allowed. |
| 148 | `IdLength` | UInt32 | Placement id length in UTF-16 code units. |
| 152 | `PlacementId` | UTF-16LE | The exact, case-sensitive id, with no terminating null. |

`SavedAt` is metadata only; WinUI does not use it to evict records or decide whether to restore.
Phase 1 does not capture or restore virtual-desktop ids. It also does not store presenter kinds,
visibility, activation, or Z-order. It derives sizing policy from the destination HWND.

## Implementation guide

The WinUI files below live in `dxaml\xcp\dxaml\lib`; PlacementEx is under `external\inc\PlacementEx`.

| Code | Responsibility |
|---|---|
| `DesktopWindowImpl.cpp` | Choose initial placement and coordinate activation, presenter checks, and shutdown. |
| `WindowPlacement.h/.cpp` | Own the id and storage objects; load, validate, apply, and save placement through PlacementEx. |
| `PlacementEx` | Handle monitor selection, DPI, work-area adjustment, and native placement calls. |

## Open issues and reentrancy

There's more work to harden against reentrancy and Window/AppWindow/win32 interop scenarios,
tracked by [#12512](https://github.com/microsoft/microsoft-ui-xaml/issues/12512).

**Reentrancy from DesktopWindowImpl::TryApplySavedWindowPlacement to PlacementEx.**

See the TODOs in `DesktopWindowImpl::TryApplySavedWindowPlacement`.

**PlacementEx's downlevel restore path can show the window too early.**

During `Window.Activate`,
WinUI asks PlacementEx to restore the saved position and size before WinUI shows the window.
When `ApplyWindowAction` is unavailable or fails, PlacementEx uses its downlevel path, which calls
`SetWindowPlacement`. That call can show the window and raise `Window.VisibilityChanged` before
`Window.Activate` reaches its own call to ShowWindow. If the app hides the window in that VisibilityChanged event handler,
the later show in `Window.Activate` can make it visible again.

WinUI passes `KeepHidden` to PlacementEx,
but the downlevel path only hides the window again at
the end of restoration. It does not prevent the earlier visibility event. We still need to avoid
this extra show and preserve the app's decision to hide the window.

## Tests

- Automated tests: `WindowIntegrationTests_Placement.cpp` 
- Manual tests app: `WinUICsDesktopSampleApp`

`WindowPlacementDocumentsMinimizedSnappedCaptureLimitations` documents the current capture limitation.
`WindowPlacementRestoreMinimizedSnapped` tests restoration from an edited saved record with complete
snap data. It does not demonstrate support for capturing the snap, minimize, then close sequence.

## Phase 1 Scenarios

For phase 1, WinUI doesn't support any customization, cascading, or AppWindow presenters other than OverlappedPresenter.
Here are some specific ways WinUI handles window placement given a windowing scenario.

Scenario `restore-normal`

WinUI uses saved placement to preserve continuity between app sessions. With unchanged
displays and size constraints, a normal window keeps its position and size.

1. Open window in normal state
2. Close window in normal state
3. Re-open window
4. Assert: Window state should be normal
5. Assert: Window bounds should be same as after (1)

Scenario `persist-moved-bounds`

WinUI remembers the user's updated position and size, not just the window's initial
layout. App defaults do not override a usable saved placement.

1. Open window in normal state
2. Move and resize window to different bounds, fully inside the work area
3. Close window
4. Re-open window
5. Assert: Window state should be normal
6. Assert: Window bounds should be same as after (2), not its initial bounds

Scenario `restore-maximized`

WinUI remembers window state separately from normal restore bounds, so maximizing
does not discard the user's normal layout.

1. Open window in normal state
2. Move and resize window to different bounds, fully inside the work area
3. Maximize window
4. Close window
5. Re-open window
6. Assert: Window should be maximized
7. Restore window
8. Assert: Window bounds should be same as after (2)

Scenario `restore-minimized-normal`

Minimization does not replace the state and bounds a window should return to. On an
ordinary launch, WinUI brings a minimized normal window back as a normal window.

1. Open window in normal state
2. Move and resize window to different bounds, fully inside the work area
3. Minimize window
4. Close window without restoring it
5. Re-open window
6. Assert: Window should be normal, not minimized
7. Assert: Window bounds should be same as after (2)

Scenario `restore-minimized-maximized`

WinUI preserves the state a minimized window should return to, including maximization.
Minimizing must not erase the normal bounds used when leaving maximized state.

1. Open window in normal state
2. Move and resize window to different bounds, fully inside the work area
3. Maximize window
4. Minimize window
5. Close window without restoring it
6. Re-open window
7. Assert: Window should be maximized, not minimized
8. Restore window
9. Assert: Window bounds should be same as after (2)

Scenario `restore-snapped`

WinUI keeps snapped placement and normal restore bounds separately, so a saved snap
does not become the window's normal size.

> **Known limitation: snapping disabled.** When Windows' window-arrangement setting
> (`SPI_GETWINARRANGING`) is off, PlacementEx's downlevel `SetWindowPlacement` path restores
> normal bounds, as tested on Windows 10. The `ApplyWindowAction` path still restores the
> saved snap, as tested on Windows 11. Phase 1 leaves this difference unchanged.

1. Open window in normal state with Windows snapping enabled
2. Move and resize window to different bounds, fully inside the work area
3. Snap window to the left side of the display
4. Close window
5. Re-open window
6. Assert: Window should be snapped to left side of the display, approximately the same coordinates as after (3)
7. Restore window out of snap (double-click the title bar)
8. Assert: Normal window bounds should be same as after (2), not the snapped bounds

Scenario `restore-minimized-snapped`

> **Not supported yet in Phase 1.** WinUI does not reliably capture the snapped layout when
> a snapped window is minimized and then closed without being restored.

1. Open window in normal state
2. Move and resize window to different bounds, fully inside the work area
3. Snap window to the left side of the display
4. Minimize window
5. Close window without restoring it
6. Re-open window with the same `PlacementId`

Actual behavior on the tested Windows versions:

| OS | On close | On re-open |
|---|---|---|
| Windows 10 | The minimized snap bounds do not overlap the work area. WinUI doesn't save its placement. | WinUI uses the previous saved placement, if any; otherwise, it uses normal initial-placement fallback. |
| Windows 11 | PlacementEx doesn't report the window as snapped. WinUI saves the normal restore bounds as `MinimizedFromNormal`, replacing any saved placement. | The window opens in normal state at its normal restore bounds, not snapped. |

Neither result guarantees restoring the last snapped layout. Closing and reopening a snapped window
without minimizing it remains supported; see `restore-snapped`.

Scenario `fit-offscreen-bounds`

WinUI prioritizes a usable on-screen window over an exact off-screen position. It adjusts
saved bounds to fit the current work area when size constraints allow.

> **Known limitation: `IsResizable=false`.** The `ApplyWindowAction` path, as tested on
> Windows 11, preserves the destination window's current size. PlacementEx's downlevel
> `SetWindowPlacement` path, as tested on Windows 10, can resize it to the saved dimensions,
> subject to native tracking limits.
> Fixed-size windows may extend beyond the work area. Phase 1 leaves this difference unchanged.

1. Open a resizable window in normal state
2. Move and resize window to different bounds, partly off-screen
3. Close window
4. Re-open window
5. Assert: Window state should be normal
6. Assert: Window should have positive width and height and fit entirely inside the work area

Scenario `prefer-same-name-display`

WinUI uses the saved display name to find the target before falling back to old screen
coordinates. That match identifies a current display name, not permanent hardware.

1. Open window in normal state on display A
2. Move and resize window to different bounds, fully inside the work area
3. Close window
4. Rearrange displays so that display A now has different screen coordinates
5. Open window
6. Assert: Window should be entirely on display A, with the same offset to display A's work area it had after (2)

Scenario `preserve-logical-size`

WinUI adapts physical-pixel size to preserve approximately the same logical space for
content when scaling changes. Work-area fitting and size constraints can limit that preservation.

1. Open window in normal state
2. Move and resize window to different bounds, fully inside the work area
3. Close window
4. Increase scaling on the same display
5. Re-open window
6. Assert: Window should have approximately the same logical size as after (2)
7. Assert: Window should fit inside the current work area

Scenario `adapt-to-replacement-display`

WinUI can adapt saved placement to an available display instead of requiring the original
display to remain connected. It uses the target's work area and DPI rather than treating
the old screen coordinates as a fixed location.

1. Open window in normal state on display A
2. Move and resize window to different bounds, fully inside the work area
3. Close window
4. Remove display A, attach display B
5. Open window
6. Assert: Window should be entirely in display B's work area, with the same work-area offset and accounting for DPI

Scenario `fallback-to-nearest-display`

When the saved display name has no match and no connected display intersects the saved
normal bounds, WinUI chooses the nearest display. Primary-display status does not take precedence.

1. Open window in normal state on display B
2. Move and resize window to different bounds, fully inside B's work area
3. Close window
4. Disconnect B, leaving no display overlapping the bounds from (2)
5. Arrange the remaining displays so non-primary display C is uniquely nearest to those bounds
6. Re-open window
7. Assert: Window should appear on C, not on the primary display
8. Assert: Window outer bounds should fit inside C's current work area
9. Assert: Window size should be same as after (2)

Scenario `fallback-without-saved-placement`

Saved placement is optional history, not a prerequisite for opening a window. With no
usable record, WinUI uses the app's requested size or existing or OS defaults.

1. Open window with a PlacementId for which there is no saved record
2. Assert: Window should open using the app's initial size (set by `Width`/`Height`) or OS defaults
3. Move and resize window to normal bounds different from those defaults, fully inside the work area
4. Close window
5. Re-open window using the same PlacementId
6. Assert: Window should open in normal state with the same bounds as after (3)