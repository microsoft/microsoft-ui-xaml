# Custom Title Bar

## Table of Contents

- [Custom Title Bar](#custom-title-bar)
  - [Table of Contents](#table-of-contents)
  - [Under the hood](#under-the-hood)
    - [Client area and top border](#client-area-and-top-border)
      - [Optional Window top-border fix](#optional-window-top-border-fix)
      - [Windows 10 frame workaround](#windows-10-frame-workaround)
    - [Min/Max/Close buttons and dragging](#minmaxclose-buttons-and-dragging)
    - [NCHITTEST behavior](#nchittest-behavior)
    - [Files](#files)

WinUI lets an app use a XAML element as a custom title bar instead of the system-provided title bar.
See [Window.SetTitleBar()](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window.settitlebar)
and the [spec document](./customtitlebar-spec.md).

## Under the hood

The implementation divides responsibility between WinUI and the Windows App SDK windowing layer:

1. AppWindow hides the system-drawn title bar and extends the client area.
2. WinUI registers caption drag regions through `Microsoft.UI.Input.InputNonClientPointerSource`.
3. AppWindow provides the system min/max/close caption controls.

AppWindow and `InputNonClientPointerSource` are Windows App SDK components implemented outside this repository.

### Client area and top border

Windows normally separates the non-client caption and resize frame from the client area used for app content.
Enabling `ExtendsContentIntoTitleBar` makes the former caption region part of the client area:

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

`DesktopWindowImpl`, the implementation of `Window`, hosts XAML in a child `DesktopChildSiteBridge` HWND.
With `Window.ExtendsContentIntoTitleBar` enabled and the HWND not maximized, WinUI leaves one physical pixel above that child.
This legacy offset can remain in fullscreen and borderless presenters. In a restored window with a native frame,
it keeps XAML from covering the top border; it does not limit the resize target to one pixel.
On Windows 10 1809, disabling `Window.ExtendsContentIntoTitleBar` while fullscreen or borderless can also leave the child at its
previous offset until a later frame or size change. The optional fix preserves this existing geometry.

#### Optional Window top-border fix

The optional change `FixWindowTopBorder` fixes how that existing row is drawn on Windows 10 when the app uses
`Window.ExtendsContentIntoTitleBar`. It does not move or resize XAML content, align the AppWindow entry point,
or change fullscreen behavior. Direct `AppWindow.TitleBar.ExtendsContentIntoTitleBar` assignments remain outside its scope.

The optional change is disabled by default and must be enabled before starting the XAML application.
The plan is to make it the default behavior with an opt-out in the future.
Test-only optional-change resets clear the process flags, not existing HWND margins. Tests must close their
custom-titlebar windows before resetting the flags. The production initialization restriction is unchanged.

#### Windows 10 frame workaround

On Windows 10, leaving a row above XAML is not enough: the existing background erase can cover it,
and the top border can differ in color from the side borders. The same opt-in enables a DWM frame workaround
and paints the row separately from the normal background. High Contrast uses the system window-frame color instead.

The workaround calls `DwmExtendFrameIntoClientArea` with a top margin based on the caption/resize-frame height,
following Windows Terminal's
[`_UpdateFrameMargins` workaround](https://github.com/microsoft/terminal/blob/0b94a7ea041a0b67f13ac281a645a82e077e4578/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L884-L943).
On Windows 10 1809, extending only one pixel left the inactive top row untinted; the larger margin made it match
the side borders. This is an observed result and an implementation precedent, not a claim about the minimum
working margin. The larger margin does not move XAML farther down.

**Compatibility:** This workaround can change all four border colors, even though only the top margin is nonzero.
WinUI takes ownership of the DWM margin set and clears it when the reserved row is removed, such as when
`Window.ExtendsContentIntoTitleBar` is disabled or the window is maximized. WinUI also clears its margins and uses the legacy
background erase when a fullscreen or borderless presenter removes the HWND's native frame, even if the
legacy XAML offset remains one pixel. A style change updates these margins without requiring a size change.
The eligibility check also excludes the FullScreen presenter and an OverlappedPresenter with `HasBorder` false.
A presenter-change notification rechecks the settled presenter state. Restoring the native frame reapplies
the workaround. Earlier app-supplied margins are not restored because DWM provides no getter.

The eligibility query is best-effort. If an AppWindow or presenter query fails, WinUI logs the HRESULT
and uses the normal background erase. A margin refresh then requests clearing any WinUI-owned margins.
If the DWM update fails, WinUI traces that failure and retains its last successfully applied margin value.

Closing detaches WindowChrome from its DesktopWindow before raising `VisibilityChanged(false)`, while the
HWND is still alive. Detached chrome is not eligible for the native top-border treatment. A synchronous
background erase during that notification fills the complete client area with the ordinary background;
a margin update clears any WinUI-owned margins without querying AppWindow.

### Min/Max/Close buttons and dragging

WinUI registers the custom title-bar element's bounds as a caption region; the Windows App SDK provides
dragging behavior and the min/max/close controls. Apps can define multiple caption regions and leave interactive
XAML elements outside them. See
[`InputNonClientPointerSource`](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.input.inputnonclientpointersource)
for the supported API behavior.

### NCHITTEST behavior

XAML renders in a child HWND, so ordinary XAML hit testing is not sufficient for top-level non-client behavior.
The Windows App SDK handles caption input and coordinates it with the caption controls, including
maximize-button Snap Layouts on Windows 11.

![Snap flyout example](./images/customtitlebar-snapflyout.png)

![Minimize button Mouse over example](./images/customtitlebar-minimize-mouse-over.png)

![Close tooltip](./images/customtitlebar-close-tooltip.png)

### Files

The WinUI side of the feature is implemented by **`WindowChrome`** and **`DesktopWindowImpl`**:

* `WindowChrome` Control:
  * Dxaml layer: [`dxaml/xcp/dxaml/lib/WindowChrome_Partial.cpp`](../../dxaml/xcp/dxaml/lib/WindowChrome_Partial.cpp)
  * Core layer: [`dxaml/xcp/components/WindowChrome/CWindowChrome.cpp`](../../dxaml/xcp/components/WindowChrome/CWindowChrome.cpp)
* Top-level HWND and window messages:
  [`dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp`](../../dxaml/xcp/dxaml/lib/DesktopWindowImpl.cpp)
