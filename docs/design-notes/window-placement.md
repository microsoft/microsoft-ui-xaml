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
3. Phase 3: Roundouts as needed.
   - WinUI automatically evict old placement entries
   - App can easily change monitor fallback logic
   - Finalize roadmap for updating PlacementEx

## Using automatic window placement

To enable in an app with package identity, just set `Window.PlacementId` and call `Activate`.

```cs
window w = new Window() { PlacementId="MyWindow" };

// During activate, WinUI looks up the saved position, size, state for "MyWindow" and applies it.
w.Activate(); 
```

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

WinUI uses `Microsoft.Windows.Storage.ApplicationData.GetDefault().LocalSettings`, in the
`Microsoft.UI.Xaml.WindowPlacement` container. ApplicationData scopes the settings to the current
user and package. This is a private format, not a public API; apps should not edit the records.

Each key is `P_` followed by 16 lowercase hexadecimal digits: the 64-bit FNV-1a hash of the id's
exact UTF-16LE bytes, without a terminating null. This keeps keys short without relying on
case-sensitive settings keys. WinUI also compares the full id using ordinal comparison:
colliding ids can replace each other's records, but cannot restore each other's placement.

### Version 1 record

Each value is a `Windows.Storage.ApplicationDataCompositeValue` with the following boxed fields.
All rectangles use physical screen pixels, not workspace coordinates or DIPs.

| Field | Type | Meaning |
|---|---|---|
| `Version` | Int32 | `1`. |
| `PlacementId` | String | The exact, case-sensitive id. |
| `NormalX`, `NormalY`, `NormalWidth`, `NormalHeight` | Int32 | Normal outer bounds, including invisible resize borders. |
| `WorkAreaX`, `WorkAreaY`, `WorkAreaWidth`, `WorkAreaHeight` | Int32 | The saved display's usable area. |
| `Dpi` | Int32 | Window DPI at capture. |
| `State` | Int32 | `0` Normal, `1` Maximized, `2` Minimized, `3` Snapped, `4` MinimizedFromMaximized, `5` MinimizedFromSnapped. |
| `SnapX`, `SnapY`, `SnapWidth`, `SnapHeight` | Int32 | Snapped visible bounds, excluding invisible resize borders. Required for states 3 and 5. |
| `DisplayDeviceName` | String | Optional GDI display name. WinUI writes it but accepts records without it. |

Phase 1: WinUI does not store virtual-desktop ids, timestamps, presenter kinds, visibility, activation,
or Z-order. It derives sizing policy from the destination HWND.

### Validation and compatibility

WinUI ignores records with an unsupported version, a different id, missing required fields, or
incorrect boxed types. It ignores field order and unknown fields. WinUI also requires:

- Rectangle dimensions of 1-65535 pixels and endpoints that fit in Int32.
- DPI of 96-9600 and normal bounds that intersect the saved work area.
- A complete snap rectangle intersecting that work area if any snap field exists or the state is 3 or 5.
- A display name, if present, of at most 31 UTF-16 code units with no embedded null.

WinUI falls back to requested dimensions, then existing bounds, when it cannot read a valid record.
Keep version 1 for new optional fields only if existing readers can safely ignore them. Use a new
version for changes to required fields, types, units, or meanings.

WinUI replaces the whole record with one insertion after filling every field. It does not preserve
unknown fields; a successful save can replace an unsupported version with version 1. ApplicationData
size and storage limits apply. When windows or processes share an id, the last successful write wins.
Phase 1 has no cross-instance coordination, record-count limit, or automatic eviction.

## Implementation guide

The WinUI files below live in `dxaml\xcp\dxaml\lib`; PlacementEx is under `external\inc\PlacementEx`.

| Code | Responsibility |
|---|---|
| `DesktopWindowImpl.cpp` | Choose initial placement and coordinate activation, presenter checks, and shutdown. |
| `WindowPlacement.h/.cpp` | Own the id and storage objects; load, validate, apply, and save placement through PlacementEx. |
| `PlacementEx` | Handle monitor selection, DPI, work-area adjustment, and native placement calls. |

## Tests

- Automated tests: `WindowIntegrationTests_Placement.cpp` 
- Manual tests app: `WinUICsDesktopSampleApp`

`WindowPlacementCapturesMinimizedSnappedBounds` documents the current capture limitation.
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
> (`SPI_GETWINARRANGING`) is off, the fallback path restores normal bounds, as tested on
> Windows 10. The `ApplyWindowAction` path still restores the saved snap, as tested on
> Windows 11. Phase 1 leaves this difference unchanged.

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
| Windows 10 | The minimized snap bounds do not overlap the work area. WinUI skips saving and preserves any earlier record. | WinUI uses the earlier record, if any; otherwise, it uses normal initial-placement fallback. |
| Windows 11 | Native capture no longer reports the window as snapped. WinUI saves the normal restore bounds as `MinimizedFromNormal`, replacing any earlier record. | The window opens in normal state at its normal restore bounds, not snapped. |

Neither result guarantees restoring the last snapped layout. Closing and reopening a snapped window
without minimizing it remains supported; see `restore-snapped`.

Scenario `fit-offscreen-bounds`

WinUI prioritizes a usable on-screen window over an exact off-screen position. It adjusts
saved bounds to fit the current work area when size constraints allow.

> **Known limitation: `IsResizable=false`.** The `ApplyWindowAction` path, as tested on
> Windows 11, preserves the destination window's current size. The fallback path, as tested
> on Windows 10, can resize it to the saved dimensions, subject to native tracking limits.
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