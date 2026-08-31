XamlRoot.Host
===

# Background

Controls sometimes need to interact with the object that hosts their XAML tree. For
example, a control might need to reach its `Window` to use `AppWindow`, while a control
running in an island might need to coordinate with the `XamlIsland` or
`DesktopWindowXamlSource` that owns the tree.

The control can get its `XamlRoot`, but today there is no supported path from that
`XamlRoot` to its XAML host. Apps commonly work around this by passing the host through
constructors, services, static properties, or element tags. Those approaches couple the
control to the app's object model and become difficult when the same control is used in
multiple windows or in both windowed and island-hosted XAML.

This spec adds the read-only `XamlRoot.Host` property. It returns the public object that
hosts the XAML tree:

| XAML hosting scenario | `XamlRoot.Host` value |
|---|---|
| Content of a WinUI `Window` | That `Window` |
| Content of a `XamlIsland` | That `XamlIsland` |
| Content of a `DesktopWindowXamlSource` | That `DesktopWindowXamlSource` |

The property type is the new marker interface `IXamlHost`, which is implemented by each
supported host type. This constrains the value to a XAML host while allowing callers to
pattern-match to the concrete host that provides the APIs for their scenario.

## Media playback window modes

A concrete framework scenario is restoring full-window and compact-overlay support to
`MediaPlayerElement`. UWP exposes the following media APIs:

| Capability | UWP | WinUI |
|---|---|---|
| Render media in full-window mode | [`MediaPlayerElement.IsFullWindow`](https://learn.microsoft.com/uwp/api/windows.ui.xaml.controls.mediaplayerelement.isfullwindow) | The property exists, but its full-window state handling is compiled out. |
| Show and enable the full-window transport button | [`MediaTransportControls.IsFullWindowButtonVisible`](https://learn.microsoft.com/uwp/api/windows.ui.xaml.controls.mediatransportcontrols.isfullwindowbuttonvisible) and [`IsFullWindowEnabled`](https://learn.microsoft.com/uwp/api/windows.ui.xaml.controls.mediatransportcontrols.isfullwindowenabled) | The properties are not in the public surface. |
| Show and enable the compact-overlay transport button | [`MediaTransportControls.IsCompactOverlayButtonVisible`](https://learn.microsoft.com/uwp/api/windows.ui.xaml.controls.mediatransportcontrols.iscompactoverlaybuttonvisible) and [`IsCompactOverlayEnabled`](https://learn.microsoft.com/uwp/api/windows.ui.xaml.controls.mediatransportcontrols.iscompactoverlayenabled) | The properties are not in the public surface. |

The corresponding WinUI implementation and default-template code is present in the
source tree but disabled. In particular, the compact-overlay implementation uses the
UWP `ApplicationView.GetForCurrentView` model, which does not identify a WinUI desktop
`Window` and does not support WinUI's multi-window and island-hosting model.

`XamlRoot.Host` gives media controls a supported way to find the host of their specific
XAML tree. When the host is a `Window`, the controls can coordinate full-window layout
with that window and use its `AppWindow` for compact-overlay or full-screen presentation.
When the host is an island, the controls can use host-appropriate behavior or disable
window-level commands rather than acting on an unrelated process-global window. This API
does not itself restore the media buttons, but it supplies the missing host association
needed for a WinUI-native implementation.

# Conceptual pages (How To)

## Access the host of a XAML element

Use an element's `XamlRoot` to obtain the object hosting that element's XAML tree. The
element must be connected to a XAML tree before its `XamlRoot` is available.

The same control can be used in different hosting scenarios, so code that supports more
than one scenario should test the concrete host type:

```csharp
IXamlHost? host = myControl.XamlRoot?.Host;

switch (host)
{
    case Window window:
        // Use Window or Window.AppWindow APIs.
        break;

    case XamlIsland xamlIsland:
        // Coordinate with a ContentIsland-based XAML host.
        break;

    case DesktopWindowXamlSource xamlSource:
        // Coordinate with an HWND-based XAML host.
        break;
}
```

`XamlRoot.Host` does not keep the host alive. Apps remain responsible for retaining and
closing their host according to the lifetime rules of `Window`, `XamlIsland`, or
`DesktopWindowXamlSource`.

# API Pages

## XamlRoot.Host property

Gets the public object that hosts this XAML tree.

```csharp
public IXamlHost Host { get; }
```

### Property value

[IXamlHost](#ixamlhost-interface)

The object that hosts the XAML tree, or `null` if the host is no longer available.

For content in a WinUI `Window`, this property returns the `Window`, even though WinUI
uses an island object internally to implement the window's XAML tree. Internal hosting
objects are not exposed in place of the public host.

For island-hosted content, the property returns the `XamlIsland` or
`DesktopWindowXamlSource` that owns that specific tree. If multiple island hosts exist
on the same thread, each `XamlRoot` returns its own host.

The XAML framework keeps only a weak reference from the XAML root to its host. Reading
the property does not extend the host's lifetime. The property returns `null` after the
host is closed or after it is released and collected. A caller that stores the returned
host in a strong reference extends that host's lifetime in the usual way.

Access this property on the thread that owns the XAML tree, consistent with other
`XamlRoot` APIs.

## IXamlHost interface

Identifies an object that can host a XAML tree.

```csharp
public interface IXamlHost
{
}
```

`IXamlHost` is a marker interface and has no members. The following WinUI types implement
it:

* [Window](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.window)
* [XamlIsland](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.xamlisland)
* [DesktopWindowXamlSource](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.hosting.desktopwindowxamlsource)

Use type matching or a cast to access APIs on the concrete host.

# Examples

## Use the Window that hosts a control

The following code maximizes the window containing a button. It works without passing a
`Window` reference into the page or control:

```csharp
private void MaximizeButton_Click(object sender, RoutedEventArgs e)
{
    if (sender is Button button &&
        button.XamlRoot.Host is Window window &&
        window.AppWindow.Presenter is OverlappedPresenter presenter)
    {
        presenter.Maximize();
    }
}
```

## Build full-screen and mini-player transport controls

A custom media transport control can find the exact window containing its
`MediaPlayerElement` and change that window's presentation mode. This avoids passing a
`Window` reference into the control and works correctly when the app has multiple
windows. The example shows the window-presentation portion; a complete implementation
would also update the media layout and restore any previous presenter configuration:

```csharp
private void EnterFullScreen()
{
    if (mediaPlayerElement.XamlRoot.Host is Window window)
    {
        window.AppWindow.SetPresenter(AppWindowPresenterKind.FullScreen);
    }
}

private void EnterMiniPlayer()
{
    if (mediaPlayerElement.XamlRoot.Host is Window window)
    {
        window.AppWindow.SetPresenter(AppWindowPresenterKind.CompactOverlay);
    }
}

private void ExitSpecialPresentation()
{
    if (mediaPlayerElement.XamlRoot.Host is Window window)
    {
        window.AppWindow.SetPresenter(AppWindowPresenterKind.Default);
    }
}
```

The control can omit or disable these commands when `Host` is an `XamlIsland` or
`DesktopWindowXamlSource`, unless the containing framework provides equivalent
host-specific behavior.

## Support both windowed and island-hosted content

A reusable control can adapt its behavior to its current host:

```csharp
private void UpdateForHost()
{
    IXamlHost? host = XamlRoot?.Host;

    if (host is Window window)
    {
        ConfigureForWindow(window);
    }
    else if (host is XamlIsland xamlIsland)
    {
        ConfigureForContentIsland(xamlIsland.ContentIsland);
    }
    else if (host is DesktopWindowXamlSource xamlSource)
    {
        ConfigureForDesktopSource(xamlSource);
    }
}
```

## Get the correct host when several islands share a thread

Each XAML root is associated with its own host:

```csharp
DesktopWindowXamlSource firstSource = CreateXamlSource(firstParentWindowId);
DesktopWindowXamlSource secondSource = CreateXamlSource(secondParentWindowId);

IXamlHost firstHost = firstSource.Content.XamlRoot.Host;
IXamlHost secondHost = secondSource.Content.XamlRoot.Host;

Debug.Assert(ReferenceEquals(firstSource, firstHost));
Debug.Assert(ReferenceEquals(secondSource, secondHost));
Debug.Assert(!ReferenceEquals(firstHost, secondHost));
```

# API Details

```c# (but really MIDL3)
namespace Microsoft.UI.Xaml
{
    // ===== NEW =====

    [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
    [feature(Feature_ExperimentalApi)]
    [webhosthidden]
    interface IXamlHost
    {
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
    [webhosthidden]
    runtimeclass XamlRoot
    {
        // ...existing members...

        [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
        [feature(Feature_ExperimentalApi)]
        Microsoft.UI.Xaml.IXamlHost Host{ get; };
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 1)]
    [webhosthidden]
    [contentproperty("Content")]
    unsealed runtimeclass Window
        : [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
          Microsoft.UI.Xaml.IXamlHost
    {
        // ...existing members...
    };

    [contract(Microsoft.UI.Xaml.WinUIContract, 8)]
    [webhosthidden]
    unsealed runtimeclass XamlIsland
        : [contract(Microsoft.UI.Xaml.WinUIContract, 8)]
          Windows.Foundation.IClosable
        , [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
          Microsoft.UI.Xaml.IXamlHost
    {
        // ...existing members...
    };
}

namespace Microsoft.UI.Xaml.Hosting
{
    [contract(Microsoft.UI.Xaml.WinUIContract, 5)]
    [webhosthidden]
    unsealed runtimeclass DesktopWindowXamlSource
        : [contract(Microsoft.UI.Xaml.WinUIContract, 5)]
          Windows.Foundation.IClosable
        , [contract(Microsoft.UI.Xaml.WinUIContract, 12)]
          Microsoft.UI.Xaml.IXamlHost
    {
        // ...existing members...
    };
}
```

# Appendix

## Host identity and lifetime

`XamlRoot.Host` returns the existing public host object; it does not create a wrapper.
Object identity is preserved, including for projected or composed `Window` types.

The relationship from `XamlRoot` to its host is weak. This avoids a cycle in which
retaining a `XamlRoot` would unintentionally retain an otherwise unused window or island.
It also allows a previously obtained `XamlRoot` to report that its host is no longer
available:

| Host lifecycle event | Result from a previously obtained `XamlRoot.Host` |
|---|---|
| Host is alive | The original host object |
| `Window.Close()` | `null` |
| `XamlIsland.Close()` | `null` |
| `DesktopWindowXamlSource.Close()` | `null` |
| Host is released and collected | `null` |

## Relationship to XamlRoot.ContentIsland

`XamlRoot.ContentIsland` exposes the lower-level object that connects XAML to the
Windows App SDK scene graph. `XamlRoot.Host` instead exposes the public WinUI object that
owns the XAML tree.

These values intentionally represent different abstraction levels. In particular, a
WinUI `Window` uses island infrastructure internally, but its `XamlRoot.Host` is the
public `Window`. Callers that need scene-graph services should use `ContentIsland`;
callers that need window or host APIs should use `Host`.

## Alternatives considered

### Return Object instead of IXamlHost

Returning `Object` would permit the same concrete values, but would not communicate or
enforce that the result is a supported XAML host. A marker interface provides a common,
extensible identity for host types without adding a wrapper or putting unrelated APIs
on `XamlRoot`.

### Return Window

Returning `Window` would address only standard WinUI windows. It could not represent
XAML trees hosted by `XamlIsland` or `DesktopWindowXamlSource`, preventing reusable
controls from supporting all WinUI hosting scenarios.

### Add separate properties for each host type

Properties such as `Window`, `XamlIsland`, and `DesktopWindowXamlSource` would make the
surface grow whenever WinUI adds another host type. They would also produce several
mostly-null properties for a single relationship. One `IXamlHost` property models the
relationship directly and remains extensible.

### Expose WindowId or HWND

A window identifier or HWND is not the XAML host and is not available for every hosting
configuration. It also would not provide access to host-specific APIs or preserve the
identity of the object that owns the XAML tree.
