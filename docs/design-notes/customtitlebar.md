# Custom Title Bar

## Table of Contents

- [Under the hood](#under-the-hood)
  - [Client area and top border](#client-area-and-top-border)
  - [Min/Max/Close buttons and dragging](#minmaxclose-buttons-and-dragging)
  - [NCHITTEST behavior](#nchittest-behavior)
  - [Files](#files)

WinUI lets an app use a XAML element as a custom title bar instead of the system-provided title bar. See the public
documentation for
[Window.SetTitleBar()](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window.settitlebar).
[Spec document](./customtitlebar-spec.md)

## Under the hood

The implementation divides responsibility between WinUI and the Windows App SDK windowing layer:

1. AppWindow hides the system-drawn title bar and extends the client area.
2. WinUI registers caption drag regions through `Microsoft.UI.Input.InputNonClientPointerSource`.
3. AppWindow provides the system min/max/close caption controls.

AppWindow and `InputNonClientPointerSource` are Windows App SDK components whose implementations are outside this repository.
This note describes WinUI's use of those APIs without relying on their internal HWND layout or input-routing implementation.

### Client area and top border

Windows normally divides a top-level window into a non-client area containing
the caption and resize frame, and a client area containing app content.

Windows sends
[`WM_NCCALCSIZE`](https://learn.microsoft.com/windows/win32/winmsg/wm-nccalcsize)
when it needs the client rectangle. When `wParam` is `TRUE`,
`NCCALCSIZE_PARAMS.rgrc[0]` is an input/output value. On entry it contains the
proposed outer window rectangle. Before returning, the handler replaces it with
the rectangle that should become the client area.

The return value does not contain the client rectangle. Returning zero selects
the default preservation behavior, which aligns the old client area with the
upper-left corner of the new client area. Windows reads the new client rectangle
from the updated `rgrc[0]`.

When `ExtendsContentIntoTitleBar` (ECITB) is enabled, AppWindow extends the
client area into the former caption region. AppWindow owns the non-client
calculation; WinUI positions its XAML child HWND within the resulting client
rectangle.

```text
Normal window

+---------------------------+
| native caption            | non-client
+---------------------------+
| app client area           | client
+---------------------------+

Content extended into title bar

+---------------------------+ client y=0
| former caption area       | client
| app client area           |
+---------------------------+
```

`DesktopWindowImpl` hosts the XAML tree in a private
`Microsoft.UI.Content.DesktopChildSiteBridge` HWND. For a restored,
non-fullscreen window with `Window.ExtendsContentIntoTitleBar` enabled,
`CWindowChrome` positions that child HWND at client y=1 and reduces its height
by one physical pixel:

```text
+---------------------------+ client y=0
| row reserved for DWM      | 1 physical pixel
+---------------------------+ client y=1
| DesktopChildSiteBridge    |
| WinUI content             |
+---------------------------+
```

The reserved row leaves space for the native top border outside the XAML
content. On Windows 11, the DWM-rendered top border appears in this row,
followed immediately by WinUI content. On Windows 10, when the top-level HWND
has a GDI redirection surface, the legacy background erase paints this row
with the window background color. The row can then differ in color from the
side borders.

The visible one-pixel row is not the complete resize target. Windows uses its
DPI-aware resize-frame metrics to provide a larger top resize target.

#### Optional top-border alignment

The optional change `AlignTitleBarTopBorderBehavior` is disabled by default.
Apps enable it before starting the XAML application. With the change enabled,
`CWindowChrome::GetAlignedTopBorderHeight` uses
`AppWindow.TitleBar.ExtendsContentIntoTitleBar` and the current presenter to
decide whether to reserve a row. For non-minimized windows:

| AppWindow ECITB | Window state | Child HWND top offset |
| --- | --- | --- |
| Disabled | Any | 0 |
| Enabled | Maximized or using the `FullScreen` presenter | 0 |
| Enabled | Neither maximized nor using `FullScreen` | 1 physical pixel |

The child HWND spans the client width, and its height is the client height
minus this offset. These geometry rules apply on both Windows 10 and Windows
11; the DWM workaround below has an additional platform check.

In fullscreen, XAML starts at client y=0 and fills the client area, including
when fullscreen is selected before the first activation. When the presenter
changes, `DesktopWindowImpl::OnAppWindowChanged` refreshes the geometry through
`WindowChrome::MoveContainer`, even if the client size did not change. Returning
to a restored Overlapped window reserves the row again if ECITB is still enabled.

The change aligns geometry, not the two ECITB properties. Changes through the
Window setter update AppWindow, but direct AppWindow assignments do not update
the cached Window getter. Presenter changes do not change either property.

With the optional change disabled, the border calculation continues to use
the cached Window ECITB state and the maximized state, without checking the
presenter. This preserves the legacy fullscreen behavior, including the
one-pixel gap when Window ECITB is enabled.

#### Windows 10 frame workaround

With the optional change enabled, WinUI also applies a frame and painting
workaround when `DwmGetWindowAttribute` returns `E_INVALIDARG` for
`DWMWA_VISIBLE_FRAME_BORDER_THICKNESS`. This check selects older DWM versions,
including Windows 10, rather than using an OS version comparison.

**Compatibility note:** On Windows 10, enabling this change can change the
app's window border colors, including the side and bottom borders, not just
the reserved top row. In the observed light-frame configuration, active
borders become white. Apps that depend on the previous border appearance
should account for this visual change before opting in.

While the top row is reserved, WinUI calls `DwmExtendFrameIntoClientArea`.
It sets `MARGINS.cyTopHeight` to the caption/resize-frame height calculated by
`AdjustWindowRectExForDpi` from the window's styles, menu state, and DPI, with
a minimum of one physical pixel. The other three margins are zero. This
calculated height is not the visible border height or an additional offset
for XAML.

WinUI clears its margin set when the reserved row is removed: when ECITB is
disabled, the window is maximized, or the presenter changes to `FullScreen`.
It reapplies the margins when the row is needed again. The DWM call writes
all four margins and has no getter. WinUI only clears a set it previously
applied; it does not restore earlier app-supplied margins. Windows for which
WinUI never applies margins are left alone.

This follows Windows Terminal's
[`_UpdateFrameMargins` workaround](https://github.com/microsoft/terminal/blob/0b94a7ea041a0b67f13ac281a645a82e077e4578/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L884-L943).
On Windows 10 1809, extending only one pixel exposes the untinted backdrop in
the inactive top row; extending the standard top-frame height makes that row
match the side borders. This is precedent for the selected margin, not a claim
that it is the minimum working value. The border-color change noted above
occurs even though only the top margin is nonzero.

Within this workaround, a top-level HWND with a GDI redirection surface and
a reserved row uses two separate `WM_ERASEBKGND` fills. WinUI retains the normal
background fill below the row and paints the row with the stock `BLACK_BRUSH`.
The two fills do not overlap, so the background erase does not temporarily
overwrite the frame strip. This follows the black-background technique in
[Custom Window Frame Using DWM](https://learn.microsoft.com/windows/win32/dwm/customframe).
`BLACK_BRUSH` is an ordinary stock brush, not an alpha-aware brush.

On Windows 10, that row remains visibly black in High Contrast, so WinUI uses the
configured `COLOR_WINDOWFRAME` brush for the reserved row instead. Moving the
composition island does not always trigger another background erase, so WinUI
also paints that High Contrast row when it updates the island position. Neither
paint path requires a buffered-paint bitmap or explicit alpha writes. HWNDs with
`WS_EX_NOREDIRECTIONBITMAP` skip the background-erase workaround; this does not
disable the DWM margin updates.

### Min/Max/Close buttons and dragging

`Window.SetTitleBar` stores the custom title-bar element and subscribes to its
size changes. `CWindowChrome` converts its bounds from XAML logical coordinates
to physical client coordinates and registers a caption rectangle through
`InputNonClientPointerSource.SetRegionRects`. Geometry updates also refresh
this rectangle. When replacing its rectangle, WinUI retains other registered
caption rectangles.

The Windows App SDK provides the caption dragging behavior and the
min/max/close controls. Apps can register multiple caption rectangles and
leave interactive XAML elements outside those regions. See the public
[`InputNonClientPointerSource`](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.input.inputnonclientpointersource)
documentation for the supported API behavior.

### NCHITTEST behavior

XAML renders through the child `DesktopChildSiteBridge`, not directly into the
top-level HWND. WinUI identifies caption regions through
`InputNonClientPointerSource` rather than relying on XAML element hit testing
to provide top-level non-client behavior. The Windows App SDK handles that
input and coordinates it with AppWindow caption controls, including
maximize-button Snap Layouts on Windows 11.

![Snap flyout example](./images/customtitlebar-snapflyout.png)

![Minimize button Mouse over example](./images/customtitlebar-minimize-mouse-over.png)

![Close tooltip](./images/customtitlebar-close-tooltip.png)

### Files

The WinUI side of the custom title-bar feature is internally represented by a control named **`WindowChrome`**.
Drag regions are registered through `Microsoft.UI.Input.InputNonClientPointerSource`; AppWindow and the Windows App SDK
own the underlying non-client input and caption-control implementation.

* `WindowChrome` Control:
  * Dxaml layer: [`dxaml/xcp/dxaml/lib/WindowChrome_Partial.cpp`](../../dxaml/xcp/dxaml/lib/WindowChrome_Partial.cpp)
  * Core layer: [`dxaml/xcp/components/WindowChrome/CWindowChrome.cpp`](../../dxaml/xcp/components/WindowChrome/CWindowChrome.cpp)
* Top-level HWND and window messages:
  [`dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp`](../../dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp)
* Workaround detection and coordinate helpers:
  [`dxaml/xcp/components/WindowChrome/WindowHelpers.cpp`](../../dxaml/xcp/components/WindowChrome/WindowHelpers.cpp)
* Border geometry regression and compatibility tests:
  [`dxaml/test/native/external/controls/window/WindowIntegrationTests.cpp`](../../dxaml/test/native/external/controls/window/WindowIntegrationTests.cpp)
* InputNonClientPointerSource:
  [Windows App SDK API documentation](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.input.inputnonclientpointersource)
* AppWindowTitleBar:
  [Windows App SDK API documentation](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.windowing.appwindowtitlebar)
