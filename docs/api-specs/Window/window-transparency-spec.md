# Window creation options for transparency (prototype)

# Background

This spike adds an opt-in path for non-rectangular desktop WinUI windows. It separates visual
transparency from input routing. WinUI enables a transparent composition background at window
creation. The app supplies its shape and uses Win32 `SetWindowRgn` to restrict input.

The APIs are experimental. This proposal does not define a shipping commitment.

# Conceptual pages (How To)

Create a `WindowOptions` object, set `SupportsTransparency` to `true`, and pass it to the new
`Window` constructor. Draw only the content that should be visible. An opaque root background
still covers the window.

The option does not remove the title bar or border. Use the existing
`OverlappedPresenter.SetBorderAndTitleBar(false, false)` API for a borderless window. Provide
your own close and move affordances.

Transparent pixels do not automatically pass input to windows underneath. For this prototype,
use `SetWindowRgn` on the HWND obtained through `IWindowNative`. The region also clips rendering:
content outside it is not displayed. A region has hard pixel boundaries, not partial opacity.
Rebuild the region when the window size or DPI changes. Its coordinates are physical pixels
relative to the window's upper-left corner, not the XAML content's origin.
Size the XAML shape from the client area. Use the client-to-window offset when building the
region, because a borderless window can still have nonclient insets.

Windows owns an HRGN after a successful `SetWindowRgn` call. Do not delete that handle. If the
call fails, the app still owns the handle and must delete it. Temporary regions used to construct
the final region remain the app's responsibility.

This API does not create an input region, derive a region from XAML, or add alpha-based hit
testing. Popups and other separate HWNDs do not inherit the option or the window region.

# Examples

```csharp
public sealed class ShapedWindow : Microsoft.UI.Xaml.Window
{
    public ShapedWindow()
        : base(new Microsoft.UI.Xaml.WindowOptions { SupportsTransparency = true })
    {
        var presenter = (Microsoft.UI.Windowing.OverlappedPresenter)AppWindow.Presenter;
        presenter.SetBorderAndTitleBar(false, false);

        // Set Content to the app's shape. Keep the remaining background transparent.
        // Use IWindowNative and SetWindowRgn to define the native input region.
    }
}
```

The complete sample is in `Samples\NonRectangularWindow`. From an initialized repository,
run the following command at the repository root:

```powershell
.\BuildNonRectangularWindow.ps1
```

The script builds x64 Release WinUI without profile-guided optimization, packs a uniquely
versioned local component package, and
publishes the sample to `BuildOutput\bin\amd64fre\Samples\NonRectangularWindow`.
Use `-Configuration Debug` for an `amd64chk` build. The script requires the repository's normal
build prerequisites. It restores dependencies for the selected flavor.

Copy the entire published folder, not just the EXE, to an x64 Windows machine. Run
`NonRectangularWindow.exe`. The app is unpackaged and includes both the .NET and WinAppSDK
runtimes. The default Release build avoids the debug CRT dependency of a Debug build.
The destination still needs the supported Windows version and the normal native runtime
prerequisites of a WinAppSDK app; this spike does not provide an installer.

The sample opens an ordinary rectangular input target and a borderless oval window with a
circular hole. The oval uses the new constructor. The target uses the existing constructor.
The oval has XAML controls and a draggable heading. It uses the existing
`InputNonClientPointerSource` caption-region API for dragging. Closing it closes the target.

| Check | Expected result |
|---|---|
| Inspect the oval and its hole | The target is visible outside the oval and through the hole. |
| Click the XAML test button | The oval's counter increases. |
| Click the hole with the region enabled | The target's counter increases. The target may activate and cover the oval. |
| Clear "Use Win32 input region" | The visual hole remains, but input is no longer excluded by a window region. |
| Drag the heading between monitors | The visual shape and input region update for the new DPI. |
| Inspect the ordinary target | Its background, border, and title bar retain ordinary WinUI behavior. |

# API Pages

## WindowOptions class

Specifies options applied when a desktop WinUI window is created.

The class is sealed and has a parameterless constructor. Create and use the options on the
window's UI thread.

## WindowOptions.SupportsTransparency property

Gets or sets whether the window supports a transparent composition background.

The default value is `false`. A `true` value selects a creation path without an HWND redirection
bitmap and without WinUI's opaque island background or GDI erase fill.

The constructor reads the value once. Changing the options object later does not change an
existing window. The same options object can be reused when creating another window.
There is no API to change this option on an existing window.

The option does not set content opacity, change chrome, or configure a system backdrop.
An explicitly assigned system backdrop can cover otherwise transparent pixels. Removing that
backdrop restores the transparent island background for an opted-in window.

## Window(WindowOptions options) constructor

Creates a desktop WinUI window with the specified creation options.

`options` must not be `null`. A null argument fails with `E_INVALIDARG`, projected as an
argument exception in .NET.

Use this constructor on the UI thread after WinUI application initialization, like the existing
constructor. A derived `Window` can pass options through its base constructor.

`new Window()` and `new Window(new WindowOptions())` preserve the existing opaque-window
behavior. The feature is not a setting for the internal UWP compatibility window or for XAML
islands hosted by an app.

# API Details

The following MIDL3 excerpt omits existing `Window` members:

```idl
namespace Microsoft.UI.Xaml
{
    [experimental]
    runtimeclass WindowOptions
    {
        WindowOptions();
        Boolean SupportsTransparency;
    };

    unsealed runtimeclass Window : Microsoft.UI.Xaml.DependencyObject
    {
        Window();

        [experimental]
        Window(WindowOptions options);
    };
}
```

# Appendix

## Implementation

The XamlOM model generates the experimental options type and constructor factory. The factory
copies the flag onto the backing `CWindow` before creating its peer and HWND.
Desktop implementation creation moves from the native `Window` constructor into peer
initialization so that the creation flag is available. The internally created compatibility
window retains its existing path.

For opted-in windows, `CreateWindowEx` uses `WS_EX_NOREDIRECTIONBITMAP`. The XAML island
starts with a transparent background, and `WM_ERASEBKGND` does not paint a GDI background.
Clearing a system backdrop restores the creation-time background choice.
Other windows retain their existing style and background settings.

Chrome and input remain app responsibilities. This keeps the spike out of WinUI's hit-testing
and visual-tree routing code.

## Limits and follow-up questions

- The sample uses a fixed-size, borderless, x64 desktop window. Regions can have jagged edges.
  Regions also clip effects such as shadows that extend beyond the shape.
- Resize interactions, maximize/snap behavior, HDR, remote sessions, device loss, and all
  accessibility tools need broader validation before defining a supported shipping feature.
- A future input API could avoid app-side region bookkeeping. It is not part of this spike.
- The options object leaves room for future creation-time settings without adding them now.
  A constructor boolean would be smaller, but would not provide that extension point.

## References

- [SetWindowRgn](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setwindowrgn)
- [Extended window styles](https://learn.microsoft.com/windows/win32/winmsg/extended-window-styles)
