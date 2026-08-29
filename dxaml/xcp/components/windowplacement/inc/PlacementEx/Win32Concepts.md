# Win32 Windowing Concepts

This README is paired with the RememberingWindowPositions readme and the header
files located in this directory.

- [RememberingWindowPositions.md](RememberingWindowPositions.md)
- [PlacementEx.h](PlacementEx.h)
- [MiscUser32.h](MiscUser32.h)
- [MonitorData.h](MonitorData.h)
- [CurrentMonitorTopology.h](CurrentMonitorTopology.h)
- [VirtualDesktopIds.h](VirtualDesktopIds.h)
- [WindowActions.h](VirtualDesktopIds.h)

## HWNDs

Apps create Windows using the API
[CreateWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa)
and receive a handle to the window, an HWND. The window has a position, size,
a parent, and many other fields.

A [**Top-Level Window**](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-windows#parent-or-owner-window-handle)
is a window that is parented to the Desktop Window (specifying null as the
parent for CreateWindow). These windows are 'free floating' windows (they are
not parented to another window). Apps can also create child windows, or ones
parented to the special HWND_MESSAGE window (which is not under the Desktop
Window and therefore never visible).

This readme focuses primarily on Top Level Windows.

## SetWindowPos - HWNDs, position, size, ZOrder, activation, visibility

HWNDs have several [window styles](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-styles),
including `WS_VISIBLE`. Windows that do not have the visible style are not seen
(on screen) and they do not receive input.

Every top-level window has a position and size, also called the window RECT
([GetWindowRect](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect)).
This indicates where the window is, relative to the primary monitor.

Each window also has a position in Z-Order, meaning its position above and below
other top level windows. Each window's list of children is ordered, and the first
child of the Desktop window is at the top of Z-Order, and above all other windows.
If two windows overlap, the one above generally occludes the one below. (Note that
this isn't always the case - some windows can be partially or completely
transparent, or cloaked.)

Every thread has a single Active window, which is one of its top level windows.
(Note: two or more threads can be 'joined', meaning the threads are synchronized
for input. There is only one active window for each input 'queue', or group of
joined threads.) Clicking on a window, or alt-tab to navigate to a window,
generally causes that window to be activated.

The entire session (all apps) share a single **Foreground**. This is the
'active app' (or input queue). Generally, keyboard input goes to the Active
window on the Foreground queue (aka the Foreground Window). See
[GetForegroundWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getforegroundwindow)
and
[SetForegroundWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setforegroundwindow).

IMPORTANT: Apps do not have a 'right to foreground', and should not 'steal' it.
User input and other things outside of the control for each app can cause
foreground to change. Apps should not expect to always be in foreground, and
attempts to set foreground can fail.

[SetWindowPos](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos)
sets a window's position, size, zorder, activation and visibility. Most of
these fields (all but visibility) are set by default, and opted out by flags.
For example, these are some of the SetWindowPos flags:
- SWP_NOMOVE
- SWP_NOSIZE
- SWP_NOZORDER
- SWP_NOACTIVATE
- SWP_SHOWWINDOW

## ShowWindow, Maximize, Minimize, and Restore

The API `ShowWindow` can **Maximize**, **Minimize**, and **Restore** a window.

**Maximized** windows fit the monitor. By default, this moves the window's
resize borders *outside* the work area. Moving the cursor to the top of the
monitor and dragging should *move* a Maximized window, not resize, and moving
the cursor to the top-right corner of the monitor should put it over the close
button.

**Minimized** windows are normally off-screen and can be restored by clicking on
the Taskbar, or pressing Alt-Tab.

If the window is not in a special state like Maximized/Minimized, the window is
'normal', or **Restored**. When a normal window becomes Maximized/Minimized, its
previous position is saved (as the normal position). Restoring a window means
moving it back to this stored position.

The window styles `WS_MAXIMIZED` and `WS_MINIMIZED` are set when a window is in
these states. These styles can be set directly using `SetWindowLongPtr`, but
doing so does not also move the window, which can lead to unexpected behavior,
like a Maximized window not restoring properly to a previous size.

To check if a window is Maximized or Minimized, use:
- [`IsZoomed`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iszoomed)
- [`IsIconic`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-isiconic)

## SetWindowPlacement (and GetWindowPlacement)

[GetWindowPlacement](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowplacement)
returns a
[WINDOWPLACEMENT](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-windowplacement)
, which contains these fields:

 - `rcNormalPosition` is the normal position (the restore position if Maximized, Minimized, or Arranged).
 - `showCmd` is a `ShowWindow` command, `SW_MAXIMIZE`, `SW_MINIMIZE`, or `SW_NORMAL`.
 - `flags` can be `WPF_RESTORETOMAXIMIZED` (and others).
 - **Note:** It is not recommended to use the other fields, like ptMinPosition.

If a window is Maximized, Minimized, or Arranged, the normal position
`GetWindowPlacement` returns does NOT match `GetWindowRect`. Windows in special
states, Maximize/Minimize/Arrange, have a separate position and normal position.

For windows that are restored, the normal rect GetWindowPlacement returns will
match `GetWindowRect`, unless the taskbar is in the top or left.
Get/SetWindowPlacement use workspace coordinates, which are offset from screen
by the distance between the monitor's work area and monitor rect.

[SetWindowPlacement](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowplacement)
takes a `WINDOWPLACEMENT`, and calls `SetWindowPos` and `ShowWindow`, to set the
window's normal position and maximize state.

When these APIs were new, apps could store the result of GetWindowPlacement in
the registry and read that value when launching to pick an initial position,
and the result was an app that always launched where it was closed.

Since these APIs were new, the OS has added support for multiple monitors, DPI
scaling, Arrangement, and the other concepts, which have made these old APIs,
when used alone, increasingly obsolete. See
[RememberingWindowPositions.md](RememberingWindowPositions.md) for more info.

> ![NOTE]
> The normal rect in WINDOWPLACEMENT is in 'workspace coordinates'. These are
  similar to screen coordinates, but they are offset by the space between the
  monitor's work area and monitor rect. These are NOT monitor relative
  coordinates - secondary monitors will still be offset by roughly the position
  of the monitor. Converting these to screen coordinates requires querying the
  monitor's work area and DPI.

## Monitors (`HMONITOR`s)

The coordinates for top-level windows are called **Screen Coordinates**. These
are defined to be relative to the primary monitor, whose origin is 0, 0.
Monitors to the left of the primary have negative X values, and ones above the
primary have negative Y values.

The 'Screen' is split into one or more sections (RECTs), the monitors. These
monitor rects NEVER overlap, and exactly one monitor rect is always positioned
at 0, 0 (that monitor is the primary monitor).

APIs like [MonitorFromPoint](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-monitorfrompoint) and
[MonitorFromRect](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-monitorfromrect)
return a handle to a **monitor**, an `HMONITOR`. This handle can be used with APIs
like [GetMonitorInfo](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmonitorinfoa)
and [GetDpiForMonitor](https://learn.microsoft.com/en-us/windows/win32/api/shellscalingapi/nf-shellscalingapi-getdpiformonitor)
to read information about a monitor:

- **Monitor Rect**. This is the position and size (resolution) of the monitor.
- **Work Area**. This is a subset of the monitor rect that isn't covered by the
 taskbar (or other docked toolbars, like Voice Access).
- **DPI**. This is the scale for content on the monitor. See **DPI**, below.
- **Display Name**. This is a string uniquely identifying the monitor. This is
 available in `MONITORINFOEX` as `szDevice`.

When picking positions, apps should pick positions that are within the bounds
of some monitor's work area. Everywhere else is 'off-screen' (not on a monitor
or covered by a top-most window like the taskbar).

In most cases, each HMONITOR maps 1 to 1 with a physical display, but this is
not always true. Displays can be removed from the desktop, which means there is
no HMONITOR, and two (or more) physical displays can be in duplicate mode, where
a single HMONITOR represents the part of the screen shown on each display in the
duplicate group.

See [MonitorData.h](MonitorData.h), which defines helpers to query the monitor
data, and [CurrentMonitorTopology.h](CurrentMonitorTopology.h), which defines
a cache of all connected monitors (the monitor topology).

### Display Changes

After an app creates a window, the monitors can change at any time! This means
that screen coordinates that are 'on-screen' (within the bounds of some monitor)
could become off-screen if that monitor is removed, or changes size.

Any time any of the fields change about a monitor, the whole system adjusts the
positions of each top level window, as needed. These 'display changes' can
change the window's position by a large amount, without there being a 'perceived'
difference to the user. For example when the user changes the primary monitor,
every window is moved by the difference in the old and new monitor origin (and
the user observes that every window stayed in the same place - while the monitors
changed positions).

There are two window messages broadcasted to all windows when the monitors change:

* `WM_DISPLAYCHANGE`: sent to all top-level windows when monitors change.
* `WM_SETTINGCHANGE` (for `SPI_SETWORKAREA`): sent when work area changes.
  This is important: if a new docked toolbar appears (e.g. Voice Access), only
  this event is fired, not `WM_DISPLAYCHANGE`.

IMPORTANT: Apps should NOT move their windows in response to WM_DISPLAYCHANGE!
Doing so can cause a window to be off screen or in unexpected places.

 - A user has two monitors connected to a laptop, using two ports on the
 laptop. The user unplugs one cable, then immediately unplugs the other.

 - It is possible that the first display change moves the window to the monitor
 that is about to be removed. If this happens, the window may be moved twice.

 - It is possible that the window receives the first WM_DISPLAYCHANGE after
 being moved once, but after that monitor it was moved to was removed. During
 this time, the window moving itself would cause the system to NOT move the
 window the second time.

 - The final state of the window becomes hard to define. If the user expects
 the system's behavior (like moving the window back to a monitor it was on when
 the monitor was unplugged), the window might end up on the wrong monitor. And
 if the app only sizes itself, but does not check that it's position is on
 screen, this could cause the window to end up off screen.

It is important to handle failures when using HMONITOR APIs, even when using
`MONITOR_DEFAULTTONEAREST`. The handle this call returns will never be null, but
it could become invalid before it is used.

```cpp
MONITORINFOEX mi { sizeof(mi) };
if (!GetMonitorInfo(MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST), &mi))
{
    // Return without moving the window. The monitors are changing.
    return;
}
```

Apps sometimes query the monitors very frequently, which can make these
edge error cases difficult to handle, or expensive to compute. See
[CurrentMonitorTopology.h](CurrentMonitorTopology.h), which defines a cache
for the monitor data. Caching the monitors removes the need to handle errors
when querying the monitors, moving these errors to display changes (where the
app can more easily wait for the next display change). This header also uses a
new API [GetCurrentMonitorTopologyId](https://learn.microsoft.com/en-us/windows/win32/winmsg/winuser/nf-winuser-getcurrentmonitortopologyid)
to avoid extra work when the monitors haven't actually changed.

Starting in Win11, the system will remember where apps are when monitors are
disconnected and move apps back if the monitor is reconnected. This 'memory'
is currently not exposed to apps. This means that if an app restarts (or the
system reboots), this previous position data is lost.

## DPI

Since Vista, Windows supports custom **DPIs** (Dots Per Inch) and scale factors,
so that apps scale properly on high-resolution displays. In 8.1, this was
extended to allow each monitor to have its own DPI, and this has improved a few
times since. Windows scales 'DPI-unaware' apps by stretching them, while
'DPI-aware' apps/windows must scale themselves. This is typically done via a
**scale factor**, which is `(DPI) / 96`; a logical DPI of 144 would have a scale
factor of 144 / 96 = 150%.

Today, there are 3 types of
[DPI awareness](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows).
A window can be any one of these:

 - **DPI Unaware**. The window renders itself at the Windows default of 96 DPI
   (scale factor 100%). Windows stretches this window bitmap to the actual DPI
   of the monitor.

 - **"System DPI" Aware**. Previously called 'Aware'. The window receives the
   primary monitor's DPI at the time the process was launched, and must scale
   its UI appropriately. Windows stretches this window bitmap if the window
   moves to a monitor with a different DPI or if the primary monitor's DPI
   changes.

 - **"Per-Monitor DPI" Aware**. The window is expected to handle
    [`WM_DPICHANGED`](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged),
    which tells the window when its DPI scale should change (e.g. when moved
    between monitors). This provides the window a `RECT` that must be used to
    resize the window to the new DPI.

It is possible for a thread to change its awareness, using
[`SetThreadDpiAwarenessContext`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setthreaddpiawarenesscontext).
When a window is created, it is 'stamped' with the thread awareness at the
time, and the thread will automatically switch back to that awareness when
dispatching messages to the window. This allows a thread to create two windows
with different awareness, which means the coordinates seen by the two windows
will be different.

### DPI Virtualization

Virtualized apps can get scaled up or down by the system when the app's DPI
does not match the monitor DPI. When the app is scaled like this, its content
becomes blurry. This is most noticeable for text, because this stretching can
make the text harder to read.

Windows that are Virtualized for DPI (not Per-Monitor DPI Aware) do not share
the same screen coordinates as other apps!

For example, if an Unaware and Per-Monitor DPI aware window are both on a 200%
DPI monitor, and each one queries the cursor position
([GetCursorPos](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getcursorpos)),
the two windows will get different answers! The output of the Unaware window is
being scaled by a 2x transform, so it's cursor position, window position, monitor
rects, and EVERYTHING the window observes is scaled DOWN by the same transform.
If each window calls GetWindowRect on the other window, each one would see a
value scaled to its own DPI, which doesn't match the RECT the other app sees.
(The two apps have different screen coordinates.)

IMPORTANT: Virtualized apps should not use coordinates on other monitors! When
a virtualized app moves between monitors with different scale factors, the
window's screen coordinates change (because the app's output is being scaled by
a different amount). For example, an unaware app may move from one monitor to
another, and find that this causes one both monitors to change size! Apps that
need to consider positions on other monitors should always run as Per-Monitor
DPI Aware (not virtualized).

IMPORTANT: Do not scale screen coordinates by a DPI! If two windows or apps
running at a different DPI awareness send coordinates directly to each other,
it may be necessary to transform between virtualized screen coordinates. This
should be done using [SetThreadDpiAwarenessContext](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setthreaddpiawarenesscontext),
which allows apps to switch between DPI awareness values. (Querying the same
thing, like a window rect, from different awareness values is enough to know
the relative transform between two windows.) Scaling by a DPI is not sufficient!
The transform applied by the system to virtualized apps can contain an offset
in addition to a scale. Values computed only by scaling by a DPI might put
windows in unexpected positions on screen.

The example below assumes that an app has multiple windows with different
awareness values, and needs to convert screen coordinates for a virtualized
window to physical (Per-Monitor Aware). While this does work with apps in
different processes, note that System DPI Aware uses a per-process DPI value,
the primary monitor when the process launched. If the primary monitor changes
DPI between two processes launching, system aware windows in the different
apps will have different coordinate spaces.

```cpp
POINT TransformPointBetweenRects(
    const POINT& pt,
    const RECT& rcFrom,
    const RECT& rcTo)
{
    const int scaleFrom = rcFrom.right - rcFrom.left;
    const int scaleTo = rcTo.right - rcTo.left;
    const POINT originFrom = { rcFrom.left, rcFrom.top };
    const POINT originTo = { rcTo.left, rcTo.top };

    return {
        originTo.x + MulDiv(ppt->x - originFrom.x, scaleTo, scaleFrom),
        originTo.y + MulDiv(ppt->y - originFrom.y, scaleTo, scaleFrom)
    };
}

POINT LogicalToPhysicalPointForWindow(HWND hwnd, const POINT& pt)
{
    DPI_AWARENESS_CONTEXT windowDpiContext = GetWindowDpiAwarenessContext(hwnd);

    if (GetDpiFromDpiAwarenessContext(windowDpiContext) == 0)
    {
        return pt;
    }

    RECT rcLogical{};
    RECT rcPhysical{};
    DPI_AWARENESS_CONTEXT dpiPrev =
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    GetWindowRect(hwnd, &rcPhysical);
    SetThreadDpiAwarenessContext(windowDpiContext);
    GetWindowRect(hwnd, &rcLogical);
    SetThreadDpiAwarenessContext(dpiPrev);

    // Transform the point from logical to physical.
    // Note: Reversing the RECT params would convert physical to logical.
    return TransformPointBetweenRects(pt, rcLogical, rcPhysical);
}
```

### Per-Monitor DPI Aware

Windows that are Per-Monitor DPI aware are not virtualized for DPI. This means
that they must scale themselves for DPI.

Consider a browser window showing a web page, where a picture and paragraph of
text is visible, but if you scroll there is much more text. When moving from a
200% DPI monitor to a 100% DPI monitor, the window should appear to the user to
stay the same size. (The window should be the same size and it should fit the
same amount of content.) Because the monitors have different DPIs, the physical
size of the window and the content scale must change to account for the different
in pixel density on the two displays.

If the window is initially size 1000 x 1000 on the 200% monitor, it should be
500 x 500 on the 100% monitor. Similarly, the content scale should be decreased
by half. (If the user is zoomed to 150% zoom, that value should not change when
changing monitors, but internally the app may apply the DPI by adjusting the
same transform.)

Changing only the window size or content scale would cause the window to grow or
shrink in size (either becoming larger or zoomed in/out). Furthermore, changing
these things at different times would cause the window to appear to 'jump' twice
(showing an intermediate state where either the window size or content is at the
wrong size).

Per-Monitor DPI aware apps must respond to DPI changes,
[`WM_DPICHANGED`](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged).
This message is sent when a window's DPI changes, and contains a RECT with the
new window position and size. The window is expected to move itself (call
`SetWindowPos`) with this RECT, after updating any state necessary for DPI
changes. (For example, prior to moving the window may need to update it's fonts
or other assets, so that they are scaled to the new window DPI.)

Apps that want to control their size on DPI changes should use
[`WM_GETDPISCALEDSIZE`](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-getdpiscaledsize),
which is sent prior to WM_DPICHANGED and offers the app a chance to scale a size
provided at the current DPI to a different DPI.

> IMPORTANT: Apps cannot move from `WM_DPICHANGED` to any position other than the
RECT provided as the lParam parameter. Doing so risks 'bouncing' back to the
previous monitor. (A slightly different size can cause the window to be mostly
on the previous monitor, which changes the size again and can get stuck in a loop.)

It is common for apps or frameworks to 'normalize' coordinates within their
window, by dividing by the DPI. This creates a 'logical' coordinate space:

- **Logical** units refer to units that have been normalized by the DPI. Someone
needs to scale these values by a DPI scale before they end up on screen.
- **Physical** units refer to units that are already scaled for the appropriate DPI.
For example, 12 logical pixels on a 150% scale monitor correspond to 18 physical
pixels.

IMPORTANT: Screen coordinates are *always* measured in in physical pixels. This
is important because screen coordinates span all the monitors, and each monitor
can have a different DPI. This makes screen coordinates 'non-uniform'. This
means they cannot be scaled by a single DPI value. Computing 'logical screen
coordinates' risks using different DPIs when scaling to/from physical, and can
lead to bogus coordinates and windows in unexpected positions. This is different
from client coordinates, or 'monitor relative' (which isn't commonly used),
because top level windows and monitors always have a single DPI (these
coordinate spaces are 'uniform', so you can unambiguously scale to/from logical
values.

The API that returns the DPI a window is currently scaling to is
[`GetDpiForWindow`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdpiforwindow).

IMPORTANT: Do not use `MonitorFromWindow`/`GetDpiForMonitor` to determine a
window's DPI! In most cases, the DPI of a monitor that a window is mostly on
will match the window's DPI, but this is not a guarantee. Using GetDpiForMonitor
to scale content in a window can cause the window to 'jump' (rescale) twice when
dragging between monitors, or get stuck scaling to the wrong DPI.

## Arranged (Snapped)

**Arranged** (aka **Snapped**) is a 'fourth state', similar to Maximized,
Minimized, and Restored. Arranged is most similar to Maximized windows, because
the window is 'fit' in some way to the monitor, and maintains a separate normal
position. (Restoring a window from either Maximized or Arranged moves the window
back to its normal position.)

While Maximized windows fit the entire monitor, Arranged windows generally align
with 2 or 3 edges of the work area, for example left-half, top-right corner,
columns, etc.

To check if a window is Arranged use:
- [`IsWindowArranged`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowarranged)

Users can Arrange windows in many ways:
 - Drag a window and hit the edges of the monitor with the cursor
 - Drag a window and drop it onto the Snap Flyout that appears at the top of the screen
 - Hotkeys like Win+Left/Right
 - Snap another window and choose this window from Snap Assist
 - See [this MSDN page](https://support.microsoft.com/en-us/windows/snap-your-windows-885a9b1e-a983-a3b1-16cd-c531795e6241) for more info.

The APIs `GetWindowPlacement` and `ShowWindow` do NOT have a way to Arrange the
window. Making a window Arranged programmatically was added by the API
`ApplyWindowAction`, which was added in recent releases.

There is a 'supported workaround' that allows for arranging windows without
using `ApplyWindowAction`, but it is not trivial. It's based on 2 factors:

 - Double-clicking on a window's top (or bottom) resize area will Arrange
 the window. This aligns the top and bottom borders with the top/bottom of the
 work area.

 - Apps can move themselves while Arranged.

    This is true of Maximized as well, though it is not generally advised.

    You can call `SetWindowPos` to move a Maximized, Minimized, or Arranged
    window, but this will not change the window's state (styles) or normal
    position. Doing this without consulting the monitor information can leave
    the window in an unexpected position, possibly off-screen!

This means that you can do something like this to Arrange a window
and set its position:

```cpp
DefWindowProc(hwnd, WM_NCLBUTTONDBLCLK, HTTOP, 0);
SetWindowPos(hwnd, nullptr, x, y, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
```

## FullScreen windows

Maximize, Minimize, and Arrange are official 'states'. Windows in these three
states have a 'normal' rect stored by the system. This creates 4 possible states
(including Restored).

FullScreen windows are ones that fit the monitor rect (not the work area, like
Maximized windows), and do not have the caption or resize border window styles,
`WS_CAPTION` or `WS_THICKFRAME`. FullScreen is not a true state, though it
operates similarly to Maximized. Enter FullScreen moves the window, and exiting
returns the window to its previous position.

For example, pressing F11 in a browser window generally makes it FullScreen.

- Try Maximizing the window prior to FullScreen. Exiting should exit first to
  Maximize (staying sized to the screen but adding the title bar and revealing
  the taskbar). A FullScreen window can **also** be Maximized.

- Try Win+M hotkey, which Minimizes all windows. The FullScreen browser window
  should Minimize. Restoring it (clicking the taskbar) should restore to
  FullScreen. A FullScreen window can **also** be Minimized.

- With multiple monitors, try Win+Shift+Left while the FullScreen window is
  in foreground (click on it first). This should migrate the FullScreen window
  to another monitor. F11 to exit FullScreen after changing monitors should
  keep the window on the same monitor, returning to a similar position from
  before entering FullScreen.

There is no API to make a window FullScreen, the way ShowWindow can for
Maximized and Minimized. FullScreen must be implemented by the app:

- When entering FullScreen, the app must store the window position, maximize
  state, and monitor information.

- When exiting FullScreen, this stored position must be adjusted to fit the
  monitor the window is on at the time.

Storing and restoring window positions is NOT trivial! Handling cases where the
new and old monitors have different sizes and DPI scale factors, and when the
window is Maximized (or Arranged) require several steps. These steps are what
the helpers in [PlacementEx.h](PlacementEx.h) are intended to make simpler!

More info in the related readme: [RememberingWindowPositions.md](RememberingWindowPositions.md)

## Extended Frame Bounds

Windows with default resize borders may (in default themes, but not in high
contrast) have invisible area on the left/right/bottom edges. The resize borders
on these sides of the window are partially transparent. (You can resize from ~7
px outside the visible bounds of the window, but the visible resize border is
only 1 px.)

This is especially important when moving Arranged windows. Arranged windows
should align, visibly, with the edges of the monitor, which requires accounting
for the invisible areas on each side of the window, otherwise there will be gaps
between the window and the monitor edges, or between arranged windows.

You can query a window's visible bounds using `DwmGetWindowAttribute`. This
returns a rect that represents the visible bounds, or frame bounds, which is
the 'real' window rect (from GetWindowRect), reduced by the size of the invisible
resize borders.

```cpp
RECT rcArrange = /* requested arrange rect, like left half of the work area */

// Query the extended frame bounds (window rect reduced by invisible resize borders).
RECT rcFrame;
HRESULT hr = DwmGetWindowAttribute(hwnd,
    DWMWA_EXTENDED_FRAME_BOUNDS, &rcFrame, sizeof(rcFrame));

if (SUCCEEDED(hr))
{
    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);

    // Extend the arrange rect by the size of the window's margins.
    rcArrange.left -= (rcFrame.left - rcWindow.left);
    rcArrange.top -= (rcFrame.top - rcWindow.top);
    rcArrange.right += (rcWindow.right - rcFrame.right);
    rcArrange.bottom += (rcWindow.bottom - rcFrame.bottom);

    // Make the window Arranged and move it to the (extended) arrange rect
    // picked above.
    DefWindowProc(hwnd, WM_NCLBUTTONDBLCLK, HTTOP, 0);
    SetWindowPos(hwnd, nullptr,
        rcArrange.left, rcArrange.top,
        RECTWIDTH(rcArrange), RECTHEIGHT(rcArrange),
        SWP_NOZORDER | SWP_NOACTIVATE);
}
```

Caveats:
 - The window should not be Maximized/Minimized when getting the frame
    bounds (the values are different in those states).
 - Changing a window's DPI (moving it between monitors) can change the
    size of the invisible area! Apps moving windows between monitors
    should move the window to the target monitor before querying the
    window's frame bounds.
 - This API does not handle DPI virtualization. If an app or window is
    virtualized for DPI (for example, a DPI unaware window), these values
    may not match those from GetWindowRect.

## ApplyWindowAction

The API [ApplyWindowAction](https://learn.microsoft.com/en-us/windows/win32/winmsg/winuser/nf-winuser-applywindowaction)
was added in Windows circa 2025.10B (26100). It takes a
[WINDOW_ACTION](https://learn.microsoft.com/en-us/windows/win32/winmsg/winuser/ns-winuser-windowaction)
, which describes changes to a window's position, size, state, monitor, etc.

For example, a WINDOW_ACTION can Maximize a window, make it visible, and
activate it, which is the same as ShowWindow(hwnd, SW_MAXIMIZE).

ApplyWindowAction can do several things that the older APIs cannot do. For
example, it can move (migrate) a window to another monitor, make the window
Arranged, and restore a position and state from the past (specifying the work
area and DPI that the position was from).

```cpp
WINDOW_ACTION action{};

//
// Example 1: Maximize the window (same as SW_MAXIMIZE).
//
// Note: You can remove visibility or activate, and Maximize without also
// showing or activating, which is not supported by ShowWindow.
//
action.kinds = WAK_PLACEMENT_STATE | WAK_VISIBILITY | WAK_ACTIVATE;
action.visible = true;
action.placementState = WPS_MAXIMIZED;

// Maximize the window (same as SW_MAXIMIZE).
if (!ApplyWindowAction(hwnd, &action))
{
    // handle error, GetLastError().
}

//
// Example 2: Arrange the window
//
RECT rcArrange = /* arrange rect, visible bounds (frame bounds) fit to the work area */
action.kinds = WAK_PLACEMENT_STATE | WAK_POSITION | WAK_SIZE;
action.position = { rcArrange.left, rcArrange.top };
action.size = { RECTWIDTH(rcArrange), RECTHEIGHT(rcArrange) };
action.placementState = WPS_ARRANGED;

if (!ApplyWindowAction(hwnd, &action))
{
    // handle error, GetLastError().
}

//
// Example 2: Migrate the window to another monitor
//
// Note: If the window is Maximized/Arranged, the window's position and normal
// position are updated as needed to fit the target monitor. (Maximized and
// Arranged windows should always restore to their same monitor.)
//
action.kinds = WAK_MOVE_TO_MONITOR;
action.pointOnMonitor = /* point in screen coordinates on the monitor to move to */;

if (!ApplyWindowAction(hwnd, &action))
{
    // handle error, GetLastError().
}

//
// Example 3: Restoring past state
//
// Note: This requires specifying the work area and DPI of the monitor when
// this position was stored. If the monitor has changed, ApplyWindowAction will
// adjust the position as needed.
//
RECT rcNormal = /* from stored state */
RECT rcArrange = /* from stored state, only if state is arranged */
RECT rcWork = /* from stored state */
UINT dpi = /* from stored state */
WINDOW_PLACEMENT_STATE state = /* from stored state */

action.kinds = WAK_PLACEMENT_STATE | WAK_NORMAL_RECT;
action.modifiers = WAM_WORK_AREA | WAM_DPI;
action.normalRect = rcNormal;
action.placementState = state;
action.workArea = rcWork;
action.dpi = dpi;

if (state == WPS_ARRANGED)
{
    action.kinds |= WAK_POSITION | WAK_SIZE;
    action.modifiers |= WAM_FRAME_BOUNDS;
    action.position = { rcArrange.left, rcArrange.top };
    action.size = { RECTWIDTH(rcArrange), RECTHEIGHT(rcArrange) };
}

if (!ApplyWindowAction(hwnd, &action))
{
    // handle error, GetLastError().
}
```

## Virtual Desktops

Win+Tab and Taskview button on the taskbar allow the user to create
multiple [**Virtual Desktops**](https://support.microsoft.com/en-us/windows/configure-multiple-desktops-in-windows-36f52e38-5b4a-557b-2ff9-e1a60c976434)
(sometimes called **Desktops**). These are groups of windows that the user can
switch between. There is an active virtual desktop, and all other virtual
desktops are considered 'background'. When a window is on a background virtual
desktop, it is not visible ('cloaked'), but it is still in the same position.

To see if a window is cloaked, use [DwmGetWindowAttribute](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmgetwindowattribute),
`DWMWA_CLOAKED`.

```cpp
bool IsCloaked(HWND hwnd)
{
    DWORD dwCloak = 0;
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &dwCloak, sizeof(dwCloak));
    return dwCloak != 0;
}
```

To see which specific virtual desktop (which is identified with a GUID) a
window is on, use [IVirtualDesktopManager](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ivirtualdesktopmanager).
IVirtualDesktopManager can query the virtual desktop ID and move a window to a
specific virtual desktop.
 - [`GetWindowDesktopId`](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ivirtualdesktopmanager-getwindowdesktopid)
 - [`MoveWindowToDesktop`](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ivirtualdesktopmanager-movewindowtodesktop)

Apps generally do not need to worry about virtual desktops, including for most
scenarios related to restoring window positions. In a normal app launch, it is
always the case that the window should launch on the current virtual desktop
(even if closed while on a background desktop).

But, when restarted (automatic app/sytstem update, etc), apps should ideally
'keep' their windows on the same virtual desktops. Consider an app like a
browser that creates many windows, and a user has organized them onto different
virtual desktops. If the system reboots and the app is restarted, it should
restore all window positions, *and* it should move each window to its previous
virtual desktop.

Note that moving the foreground window to a background virtual desktop switches
the active virtual desktop. (If an app is attempting to move a window to a
background virtual desktop, it must SetForegroundWindow on some other window if
the window is foreground, to ensure the active virtual desktop doesn't change.)
