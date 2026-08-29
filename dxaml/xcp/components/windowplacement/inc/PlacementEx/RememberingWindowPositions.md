# Remembering Window Positions

This README is paired with the header files in this directory.

- [PlacementEx.h](PlacementEx.h)
- [MiscUser32.h](MiscUser32.h)
- [MonitorData.h](MonitorData.h)
- [CurrentMonitorTopology.h](CurrentMonitorTopology.h)
- [VirtualDesktopIds.h](VirtualDesktopIds.h)
- [WindowActions.h](VirtualDesktopIds.h)

The intended audience of this file is developers who are using or modifying
these headers in apps or frameworks.

Please read the [Win32Concepts](Win32Concepts.md) page before reading this one.
This readme heavily refers to the concepts discussed on the Win32 Concepts page.

These header files use only public APIs. In some cases, these APIs were added
in recent releases, but PlacementEx does not require these APIs, offering
fallbacks that use APIs supported all the way back to Win7.

 - [CreateWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa)
 - [SetWindowPos](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos)
 - [GetMonitorInfo](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmonitorinfoa)
 - [GetWindowPlacement](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowplacement)
 - [SetWindowPlacement](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowplacement)
 - [GetDpiForWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdpiforwindow)
 - [IsWindowArranged](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowarranged)
 - [GetDpiForMonitor](https://learn.microsoft.com/en-us/windows/win32/api/shellscalingapi/nf-shellscalingapi-getdpiformonitor)
 - [DwmGetWindowAttribute](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmgetwindowattribute)
 - [IVirtualDesktopManager](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ivirtualdesktopmanager)
 - [ApplyWindowAction](https://learn.microsoft.com/en-us/windows/win32/winmsg/winuser/nf-winuser-applywindowaction)
 - [GetCurrentMonitorTopologyId](https://learn.microsoft.com/en-us/windows/win32/winmsg/winuser/nf-winuser-getcurrentmonitortopologyid)

## Overview

PlacementEx defines an object that stores a window's position and additional
state. This simplifies the surprisingly complicated process of restoring windows
to these stored positions. For example, storing the position when a window
closes and using it to pick an initial position when the app launches. As
described in detail later in this document, PlacementEx is a modern replacement
for the APIs
[SetWindowPlacement](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowplacement)
and
[GetWindowPlacement](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowplacement),
which served this purpose in the past, but are now no longer sufficient.

While most Windows apps do appear to remember their positions, the system does
not do this automatically. Apps that remember Maximized state or positions on
secondary monitors are storing positions themselves. This is often done by
application frameworks, and using (in part) the APIs Get and SetWindowPlacement.

Unfortunately, many applications today pick unexpected positions, especially when
the monitors change between closing and re-opening the application, or when an
application is closed while Arranged. Multiple monitors, Arrangement, Virtual
Desktops, and other concepts were added to Windows years (decades) after the
Get and SetWindowPlacement APIs were created.

This README, and [PlacementEx.h](PlacementEx.h), introduce fresh guidance for
storing window positions. Apps of all sizes and complexity can copy these headers
into their code bases, and use them to store window positions. Like the time when
Get/SetWindowPlacement were new (circa Win3.1), apps can simply Get/SetPlacementEx
and not worry about Arranged positions, monitor changes, virtual desktops, and
the other 'problems' that apps using only Get/SetWindowPlacement have hit in the
past.

## Picking a Monitor

If an app is closed on a secondary monitor and immediately relaunches, the new
window should be in the same place, on the same monitor. If that monitor is
removed before the app relaunches, the new window should be created on the
nearest monitor. (Note: Using the primary monitor instead is also fine.)

```cpp
HMONITOR hMonitor = MonitorFromRect(&rcNormal, MONITOR_DEFAULTTONEAREST);
```

Using the nearest monitor to a stored position will work in most common cases,
but it has problems. For example, if the user changes the primary monitor, or
re-arranges the monitors (changing the relative position of the secondary
monitors) between closing and opening an app. Using the position alone might
cause the window to launch on a different monitor than it was closed on
(because the right monitor is in a different position).

To account for monitors changing positions, it is recommended for apps that
store past positions to store the device name for the window's nearest monitor.

```cpp
MONITORINFOEX mi = { sizeof(mi) };
if (!GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi))
{
    // Handle failure.
}
std::wstring deviceName = mi.szDevice;
```

When picking the monitor to create a window, first use EnumDisplayMonitors
to loop over each monitor, and GetMonitorInfo on each monitor, to see if any
monitor has a matching device ID. If no monitors have a matching device ID, fall
back to the nearest monitor to the stored position.

See `PlacementEx::FindClosestMonitor` in [PlacementEx.h](PlacementEx.h), and
`MonitorData::FromDeviceName` in [MonitorData.h](MonitorData.h) for an
implementation of this algorithm.

## Migrating Normal Positions

The normal rect is the name for the window position and size, not including the
special Maximize/Minimize/Arranged positions. For Maximized, Minimized, or
Arranged windows, the normal position is the restore position, where the window
would go if restored.

Because Maximized/Arranged windows should always restore to their current
monitor, migrating a Maximized/Arranged window requires migrating the normal
rect.

When storing a window position, it is important to store the normal rect along
with the work area of the nearest monitor and the DPI of the window. See
[GetMonitorInfo](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmonitorinfoa)
and
[GetDpiForWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdpiforwindow).

Given a stored normal rect, work area, and DPI, and given a work area and DPI
for a current monitor, the normal rect can be moved, or migrated, to a monitor
in the following steps:

1. Offset the normal rect by the new and old work areas.
2. Resize the normal rect (moving bottom/right sides) to scale the size from
  the old DPI to the new DPI.
3. Adjust the position if the rect is outside the new work area.
 - If smaller than the work area, move the position left/right, up/down as needed.
 - If larger than the work area, move each edge to maintain the same relative
 distance to the work area.

See `PlacementEx::AdjustNormalRect` in PlacementEx.h for an implementation of
this algorithm.

Note that the third step can draw different opinions between users. Some users
might expect a window closed off-screen to re-launch off-screen. Or for a large
window on a large monitor that later launches on a small monitor to be larger
than the monitor. But, while apps can remember precise positions from the past,
users often do NOT remember exactly where every window was last, and an app
ending up mostly off-screen or larger than the monitor is often perceived as a
bug (even if the app was just restoring its last position).

## Migrating Arranged Positions

If a window is Arranged (Snapped), it is fit to some section of the work area.
For example, left half, columns, corners, etc. An arrange rect migrated between
monitors should remain left half, corners, etc. (Unlike normal rects, arrange
rects always retain their relative distance to each edge of the work area.)

See `PlacementEx::AdjustArrangeRect` in PlacementEx.h for an implementation of
this algorithm.

IMPORTANT: Arranged windows should visibly align with the monitor edges. Because
some windows have partially transparent resize borders, it is necessary to store
arranged rects in frame bounds
([DwmGetWindowAttribute](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmgetwindowattribute)
, DWMWA_EXTENDED_FRAME_BOUNDS). The arrange rect should be migrated while in
frame bounds, and then extended by the window margins before moving the window.

Note that it is not necessary to handle the Maximize/Minimize positions when
moving the other RECTs to a different monitor. These positions are chosen
depending on the monitor a window is on when it is Maximized/Minimized, and apps
can handle messages like `WM_GETMINMAXINFO` to override these default positions.

## The STARTUPINFO

The API [GetStartupInfo](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getstartupinfow)
returns a
[STARTUPINFO](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfoa),
which contains values set by whoever launched the process. For example,
if the user clicks on the taskbar to launch an app, the app is launched with a
STARTUPINFO set by the taskbar.

The STARTUPINFO has some flags/values related to the position of the app's main
window. While some, `STARTF_USEPOSITION` and `STARTF_USESIZE`, are not
recommended to use, there are two that are important:

1. The ShowWindow Command

- `STARTF_USESHOWWINDOW` is set if the caller requested that the window launch
`SW_MAXIMIZE` or `SW_MINIMIZE`. Note that apps should ignore this when the
value is `SW_NORMAL`. If closed while Maximized and launched 'normal', the app
should ignore the startup info and launch Maximized. But if closed while
restored and launched with 'start /Maximized', the app should use the startup
info and launch Maximized.

- For example, from cmd:

```cmd
> start /min notepad
```

From PowerShell:
```powershell
> start -WindowStyle Maximized notepad
```

2. The Monitor Hint

- If launched by the taskbar or start menu, the monitor the user clicked on is
provided to the app in the startup info, aka the 'Monitor Hint'. Ideally, the
app's main window should launch on this requested monitor.

- The monitor hint is provided as the `hStdOutput`. (There is no flag indicating
it is set.) Note that the monitor hint is only available for non-console apps.
For console windows, this field is the handle to the standard output stream.

For an example, let's say a window is closed while Maximized on a secondary
monitor. If launched with 'start /min', the app should launch Minimized, but
restore to Maximized, `WPF_RESTORETOMAXIMIZED`. Or, if launched from taskbar on
the primary monitor, the app should launch Maximized on the primary.

## Restarting after System Reboot

In the Settings app, search for 'Restart apps after signing in'. If this
setting is enabled, apps opened while the system reboots can be restarted. This
is not done automatically - apps must register for restart.

The API to register for restart is
[RegisterApplicationRestart](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-registerapplicationrestart).
Apps call this when launching, providing a command line argument that is
provided back to the app when the system launches the app, after rebooting.
Note that the app is responsible for remembering a position for each window and
restoring the window(s) when restarted.

```cpp
PCWSTR restartCmdLine = L"restart";
RegisterApplicationRestart(restartCmdLine, 0);
```

If the system reboots, the window will not receive a WM_DESTROY.
(This happens if the window is still valid when the thread is destroyed.) To
ensure that the window saves it's last position when the system reboots, the
window should handle `WM_ENDSESSION`. This message is sent prior to the thread
closing because the system is rebooting, and offers a time to either destroy
the window (DestroyWindow) or call the same handler as WM_DESTROY to save the
window position and other state.

```cpp
   case WM_ENDSESSION:
   case WM_DESTROY:
      SaveWindowPositionToRegistry();
      break;
```

Restarting is similar to a regular launch, but there are some small recommended
differences in behavior:

- Normally, apps should Activate their main window, but not when restarting.
- Normally, apps should not launch Minimized. If closed while Minimized, the app
  should launch to the restore position. If restarting, however, a window that
  was Minimized previously should should launch Minimized.
- Normally, apps should launch on the active virtual desktop. If closed while
  on a background virtual desktop and restarting, the app should move the window
  to the same virtual desktop.

More info about application restart on [this msdn page](https://learn.microsoft.com/en-us/windows/win32/recovery/registering-for-application-restart).

## Launching Minimized

If an app is closed while the window is Maximized, it is ideal to launch
Maximized. But, if closed while Minimized, for example via Taskbar/Alt-Tab, the
window should typically launch to its restore position. (As a bonus, you can use
`WPF_RESTORETOMAXIMIZED` to launch Maximized if the window was Maximized prior
to Minimizing and closing.)

But if closed while Minimized, it is NOT recommended to launch Minimized. There
are two caveats:

 - If restarting (because system reboot or app restarting itself to take an update).
 - If explicitly launched as Minimized, using the Startup Info (start /min).

Put another way, if a user launches an app from the taskbar, it should show up
somewhere on screen (not Minimized). But, if the system reboots, Minimized apps
should not be restored from Minimized (they should 'stay Minimized' while the
system reboots).

## Launching on a Background Virtual Desktop

Similar to launching Minimized, apps should normally launch on the active
virtual desktop. (This is the default if you simply create a new window.) If an
app is closed while on a background virtual desktop, apps should restore the
virtual desktop only when being restarted. This is done using the
[IVirtualDesktopManager](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ivirtualdesktopmanager)
APIs.

See [VirtualDesktopIds.h](VirtualDesktopIds.h) for examples and more information
about virtual desktops.

## Cascading

Consider an app like notepad, or photos, where a user may launch several
instances (multiple files, each in a separate window). If the app always
launches a new window where the last one closed, launching multiple text files,
or pictures would launch multiple windows in *exactly* the same place. This may
happen very quickly, and the user might not realize multiple windows have opened
until one is moved, 'revealing' the window immediately below.

Instead of 'stacking' windows directly on top of each other, it is recommended
to 'cascade', meaning move the next window down and right a bit, leaving the
title bar of the previous window visible.

For single window apps, this can be done by using FindWindow with the class
name *before* creating the main window. This will find another instance of the
same app that is already running (assuming the class name is sufficiently unique).

The size of the 'nudge' is ideally the height of the window's caption bar.
This can be queried using system metrics:

```cpp
UINT captionheight =
    GetSystemMetricsForDpi(SM_CYCAPTION, dpi) +
    (2 * GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi));
```

Adding `2 * SM_CXSIZEFRAME` ensures the entire title bar from the previous
window is visible, since it includes the invisible resize area (which is part of
the title bar when not Maximized).

> Note: The similar function
[GetSystemMetrics](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsystemmetrics)
returns values for the *process* DPI. If an app is Per-Monitor DPI aware, it
should use [GetSystemMetricsForDpi](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsystemmetricsfordpi)
and the DPI of the window,
[GetDpiForWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdpiforwindow).

## CreateWindow parameters

[**`CreateWindow`**](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowa)
takes an: `x`, `y`, `nWidth`, `nHeight`, and styles. (And other parameters not
discussed here.)

If the position and size parameters are ALL `CW_USEDEFAULT` (a special value),
the window will be in the default position:

- On the primary, unless a monitor hint is set in `STARTUPINFO`.
- Cascading: at a position a bit down/right from the last window to get the default
 position on that monitor, see above.
- A size picked from the size of the monitor (likely too large on very large
 monitors).

Note that if the size parameters are set, the window will *always* launch on the
primary and at the requested size (which is assumed scaled to the DPI of the
primary monitor).

If a process is launched with the `STARTUPINFO` show command set, that is applied
to the first window the app makes visible (matching some criteria, like `WS_CAPTION`).

Since these parameters must all be `CW_USEDEFAULT` for the magic behavior to
occur, when restoring window positions (including Maximize state, size, and
monitor), it is recommended to create the window hidden (without `WS_VISIBLE`)
and in the default position (`CW_USEDEFAULT`). Then, after the window is
created, the app should move the window to its correct position (which in some
cases requires moving multiple times), and only show the window when it is in
the final position.

Providing a size or position to `CreateWindow` requires querying the monitor
information and pre-scaling the size to the DPI of the monitor (and adjusting
it to ensure the position is on screen). To create a window that starts Maximized
or Minimized, create the window with the `WS_MAXIMIZE` or `WS_MINIMIZE` style.

Apps can also position themselves within `WM_CREATE`. Apps can pick an initial
size here (and call `SetWindowPos`), but calling `ShowWindow` or
`SetWindowPlacement` to Maximize the window can cause problems! After
`WM_CREATE` returns, if the window has `WS_MAXIMIZE`/`WS_MINIMIZE` styles, it is
assumed that the window was created with these styles (and its current position
is the normal position). If the window has been Maximized already at this point,
restoring it will not move the window (its normal position was set to the
Maximize position). This is (much) worse if done for Minimize, since it could
cause the window to be stuck off screen.


# Putting it all together

Previous sections describe individual problems that must be considered when
saving window positions and using them later to position windows. This section
puts these pieces together, and offers a complete list of steps recommended for
developers when implementing this logic in application and framework code.

See `PlacementParams`, defined in [PlacementEx.h](PlacementEx.h), which
simplifies these steps when using PlacementEx.

## Saving window state

Apps need a way to store persisted data, for example in a registry key with a
name unique to the application. See [RegistryHelpers.h](RegistryHelpers.h).

When storing a window position, the following fields are needed:

- Normal position and Maximize state ([GetWindowPlacement](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowplacement))
- Monitor work area and device ID ([GetMonitorInfo](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmonitorinfoa))
- Window DPI ([GetDpiForWindow]())
- If arranged, the frame bounds ([DwmGetWindowAttribute](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmgetwindowattribute))
- The window's Virtual Desktop ID ([IVirtualDesktopManager](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ivirtualdesktopmanager))

Apps can choose how many placements to store, and what to name them. For
example, a mail client may store a position for the last 10 emails opened in a
separate window, as well as the main window and the notifications window. For
this example, we'll assume the app has only one window, and stores its position
in the registry under a 'last close position' key.

Each top level window should store its state when it is destroyed, `WM_DESTROY`.
Apps should also handle `WM_ENDSESSION` and either destroy the window explicitly
or simply save the position information from that message too. When the system
reboots, apps that are opened will be forcefully closed, and each window will
not receive a WM_DESTROY, but they will get a WM_ENDSESSION in that case.

## Picking the initial window position

This example assumes the app has only one window, which should launch by default
where it was when last closed. The window has a class name that is a hard coded
string unique to this app (not a string that other apps would be likely to use).

### Read last close position from the registry

As described in the previous section, the app stores a 'last close position' in
persisted data (the registry) when it is closed. When launching the app, it
reads this persisted data, and uses it to decide how to position the main window.

This stored position (which may not be found) is the default for the initial
position, but it may be overridden in several ways before the final position is
picked.

### Detect another running instance (Cascade)

Before creating the window, the app should use the window class name and the
API [FindWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-findwindowa)
to see if another instance of the app is already running.

If another window is found, use that window's placement instead of the one
stored in persisted data. Cascade this position, moving it down/right by the
height of the title bar, and wrap to the opposite side of the monitor if this
moves the placement partially off-screen.

This ensures that if the user launches several instances of this app at the
same time, each of those windows doesn't perfectly cover the previous one
(because each one used the same stored position in the registry).

Don't cascade over the window from another instance if it is cloaked,
(DwmGetWindowAttribute, DWMWA_CLOAKED).

### Detect Restart

Apps should handle a command line argument that indicates the app is being
restarted, as opposed to a 'normal launch'. This command line is used with the
API [RegisterApplicationRestart](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-registerapplicationrestart)
, called during launch. If the application is running while the system reboots,
it will be launched after the reboot with the provided command line.

```c++
PCWSTR restartCmdLine = L"/restart";
RegisterApplicationRestart(restartCmdLine, 0);
const bool isRestartLaunch = (wcsstr(cmdLine, restartCmdLine) != nullptr);
```

The restart launch changes behavior in a few ways:

- Normal launch should Activate the window. Restart should show the window but
  not activate.

- Normal launch modify a Minimized position, switching to Restored or Maximized
  (if `WPF_RESTORETOMAXIMIZED`).

- Normal launch should ignore the Virtual Desktop ID (always launch on active
  desktop).

- Normal launch should call `GetStartupInfo` and read the flags to see if the
  app is being launched with the 'start Maximized/Minimized' option (from
  command line), or the 'Monitor Hint' (from taskbar/start menu).

See `PlacementEx::AdjustForMainWindow` in [PlacementEx.h](PlacementEx.h) where
these restart modifications are done.

### Create the window (CW_USEDEFAULT, and not Visible)

It is recommended for apps to create their window using CreateWindow's
`CW_USEDEFAULT` values for the x, y, cx, cy parameters. This will create the
window at a position and size chosen by the system, generally on the primary
monitor.

It is not recommended to create the window with WS_VISIBLE, WS_MAXIMIZE, or
WS_MINIMIZE, or show the window WM_CREATE. Instead, allow CreateWindow to
return with the window in the default position, and then move the window to the
ideal position, and only show the window once at the final position. This
avoids the user seeing the window jump to multiple places.

### Pick a default size (first run fallback)

While the default size on each launch depends on the previous size when last
closed (or the size of an already running instance), the very first time each
user runs the app, there will be no default size at this point.

It is recommended for apps to choose a default logical (not scaled for DPI)
size for the window. This size should be a relatively small size (which fits on
very small monitors) that the app looks reasonable at. Like '700 x 500'.

If the app launches and finds no position from the registry, or previous
instance to launch cascaded above, the app should use start with the window's
current position (the default chosen by CreateWindow), and change the size to
the hard coded default size, scaled to the window's DPI (GetDpiForWindow).

See `PlacementEx::SetLogicalSize` in [PlacementEx.h](PlacementEx.h), which
applies this default size and adjusts it as needed to keep the position within
the work area.

### Moving a window to a stored position

We now have a chosen position, which may be from the registry, another window,
or the current position adjusted to a fixed default size. The window is created
but is not visible yet. The time has come to move the window to this position
and show it (make it visible).

This section describes `PlacementEx::SetPlacement`, the function in PlacementEx
that applies a window placement, moving the window and normally making it
visible and active.

If any step fails, and the operation is canceled, fall back to showing the window:

```c++
ShowWindow(hwnd, SW_SHOW);
```

1. Pick a final monitor and adjust the position as needed.

  - If querying the monitor info fails, fall back to showing the window (SW_SHOW).

  - Use the display ID of the monitor stored with the placement first to pick
  a monitor, falling back to the nearest monitor to the stored work area.

  - The normal rect must be adjusted depending on the work area and DPI stored
  in the placement and the values for the monitor. If the placement is arranged,
  also migrate the arrange rect (which uses slightly different logic).

  - See `PlacementEx::FindClosestMonitor` and `PlacementEx::MoveToMonitor` for
  these steps in [PlacementEx.h](PlacementEx.h).

2. Cloak the window

  - In some cases, like arranging, moving between monitors, etc, we're going to
  have to move the window multiple times. To guarantee that the user does not
  see the window become visible until the final position, you can cloak the
  window now, and uncloak at the very end.

  - See TempCloakWindowIf in [MiscUser32.h](MiscUser32.h), which calls
  [DwmSetWindowAttribute](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmsetwindowattribute)
  to cloak and unclock a window.

  - This can be scoped only to cases like Minimized/Maximized/Arranged windows,
  monitor changes, virtual desktop (restart), but it is needed in several cases
  and can be done unconditionally. Note that it is especially important when
  Arranging a window, because cloaking disables animations (which might
  otherwise show the window animating from an unexpected previous position).

3. Restore from Max/Min/Arranged (if needed)

  - If the window is Maximized, Minimized, or Arranged, it must be restored
  before setting the normal position. Use `SW_SHOWNOACTIVATE` to avoid activating
  the window here.

```c++
ShowWindow(hwnd, SW_SHOWNOACTIVATE);
```

4. Move 'Preemptively' if changing monitors

  - If the window is not on the same monitor it is moving to, call SetWindowPos
  to move the window to the normal rect. This is skipped (and overwritten by)
  the next step.

  - This preemptive move handles cases where a window is moving between monitors
  with different DPIs. (If the app picks an unusual size after the DPI change,
  this is considered if later the position is saved because the window is
  becoming Maximized/Minimized/arranged.

5. Call SetWindowPlacement

 - Convert screen coordinates to workspace using the target monitor's work area
 and monitor rect. Then set the requested normal rect and show state (if
 Maximize/Minimize, not if Arranged).

6. Set Arrangement position, if Arranging

 - If arranging, the SetPlacementCall should use SW_NORMAL, which restores the
 window and sets the normal rect. If the desired state is Arranged, we need to
 'manually' set the Arrange position, using the 'WM_NCLBUTTONDBLCLK workaround'
 described in the Arranged (snapped) section of [Win32Concepts](Win32Concepts.md).

 - The arranged rect should be stored in frame bounds. Now that the window is
 on the target monitor, restored and at the final DPI, query the window's frame
 margins and expand the arrange rect by the margins.

 - Note: In a very narrow case where an app picks an Arranged position, and
 then overrules that with 'Minimize', because of a StartupInfo override,
 PlacementEx has an additional step here to Minimize the Arranged window. See
 the RestoreToArranged flag in PlacementEx.h.

7. Set virtual desktop (restart scenarios only)

   - If restarting, set the window's virtual desktop ID,
   [VirtualDesktopIds.h](VirtualDesktopIds.h).

## Using PlacementEx

The previous section described each recommended step for applications to
implement storing a window position when closing and using it to pick an
initial position when launching. This section provides the syntax to do this
using PlacementEx.h.

```cpp
// Include PlacementEx.h and associated headers.
#include "User32Utils.h"

// Define a unique name in the registry for information for this app,
// and a last close registry key name.
PCWSTR appRegKeyName = L"SOFTWARE\\UniqueNameForThisApp";
PCWSTR lastCloseRegKeyName = L"LastClosePosition";

// Define a unique class name and pick an (arbitrary) default size for the app,
// to use if no previous size is stored yet in the registry.
PCWSTR wndClassName = L"UniqueWindowClassNameForThisApp";
constexpr SIZE defaultSize = { 600, 400 };

// Save the window position in the registry when the window closes.
LRESULT CALLBACK WndProc ...
{
    switch (message)
    {
        case WM_ENDSESSION:
        case WM_DESTROY:
            PlacementEx::StorePlacementInRegistry(
                hwnd,
                appRegKeyName,
                lastCloseRegKeyName);
          break;
    }
...
}

bool CreateMainWindow(HINSTANCE hInstance, PWSTR cmdLine)
{
    // Before creating the window, initialize a PlacementParams (defined in
    // PlacementEx.h). This stored position information from the registry,
    // and finds any existing instance of this app that is already opened.
    PlacementParams pp(defaultSize, appRegKeyName, lastCloseRegKeyName);
    pp.FindPrevWindow(wndClassName);

    // Create the window in the default location, and not visible.
    HWND hwnd = CreateWindowEx(
        0,
        wndClassName,
        L"MyWindowTitle",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hwnd)
    {
        return false;
    }

    // Register for application restart. If the restart command line is set on
    // this instance, the initial position is adjusted slightly: We won't
    // activate the window, and if it closed while Minimized we'll launch this
    // window Minimized.
    PCWSTR restartCmdLine = L"/restart";
    RegisterApplicationRestart(restartCmdLine, 0);
    if (wcsstr(cmdLine, restartCmdLine) != nullptr)
    {
        pp.SetIsRestart();
    }

    // Move the window to the initial position and show it (make it visible).
    pp.PositionAndShow(hwnd);
    return true;
}
```
