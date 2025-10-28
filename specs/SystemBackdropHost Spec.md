SystemBackdropHost
===

# Background

There are backdrop materials provided in WinUI such as Mica, Acrylic that are subclass of 
[Microsoft.UI.Xaml.Media.SystemBackdrop](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.media.systembackdrop). Currently, it is possible to host a system backdrop only at the window level or on flyouts, but not in a specific area in the visual tree. This have been a major limitation on WinUI3 compared to WinUI2 in achieving the acrylic / mica effects, especially for achieving various animations.

`SystemBackdropHost` is a lightweight `FrameworkElement` that bridges between the XAML tree and the composition
infrastructure required by `SystemBackdrop`. It creates the required composition components to host the systembackdrop on a specific container, keeps the placement
visual sized to the arranged bounds, and applies the element's `CornerRadius` to the backdrop clip so rounded corners
appear as expected. This control abstracts lot of details for the composition layer and hence make it easy
for WinUI3 developers to implement the acrylic effect in the applications.

## Goals

* Provide an intuitive, XAML-friendly way to place a system backdrop anywhere inside application's visual tree.
* Handle connection, disconnection, and sizing so application only have to set a backdrop and position the element.
* Allow to round the hosted backdrop without writing custom composition code.

## Non-goals

* Adding a SystemBackdrop property independently on all controls.
* Provide a content container; `SystemBackdropHost` is purely a visual effect surface and does not host child content.

# Conceptual pages (How To)

The guidance in the below examples can be followed by developers for adopting
`SystemBackdropHost`.

# API Pages

_(Each level-two section below maps to a docs.microsoft.com API page.)_

## SystemBackdropHost class

Use `SystemBackdropHost` to place a SystemBackdrop](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.media.systembackdrop) anywhere within the XAML layout.

```csharp
public sealed class SystemBackdropHost : FrameworkElement
```

### Examples

Keep the `SystemBackdropHost` in the bottom of the stack below other contents to achieve the backdrop effect:

```xml
<Grid x:Name="AnimatedGrid" Height="100" Width="100"> 
    <Grid.Resources> 
        <Storyboard x:Name="SizeAnimation" RepeatBehavior="Forever"> 
            <DoubleAnimation Storyboard.TargetName="AnimatedGrid"  
                           Storyboard.TargetProperty="Width" 
                           From="100" To="200" Duration="0:0:2"  
                           RepeatBehavior="Forever" 
                           EnableDependentAnimation="True" 
                           AutoReverse="True"/> 
            <DoubleAnimation Storyboard.TargetName="AnimatedGrid"  
                           Storyboard.TargetProperty="Height" 
                           From="100" To="200" Duration="0:0:2"  
                           EnableDependentAnimation="True" 
                           RepeatBehavior="Forever" 
                           AutoReverse="True"/> 
        </Storyboard> 
    </Grid.Resources> 
    <SystemBackdropHost CornerRadius="4"> 
        <SystemBackdropHost.SystemBackdrop> 
            <DesktopAcrylicBackdrop /> 
        </SystemBackdropHost.SystemBackdrop> 
    </SystemBackdropHost> 
    <Button Content="Test Button"/> 
</Grid> 
```

![Example of SystemBackdropHost with animation](images/Acrylic.gif)

The same pattern works from code:

* C#:
```csharp
var host = new SystemBackdropHost
{
    SystemBackdrop = new MicaBackdrop(),
    CornerRadius = new CornerRadius(12)
};
rootGrid.Children.Add(host);
```

* C++:
```cpp
winrt::Microsoft::UI::Xaml::Controls::SystemBackdropHost host;
host.SystemBackdrop(winrt::Microsoft::UI::Xaml::Media::MicaBackdrop());
host.CornerRadius(winrt::CornerRadius{ 12, 12, 12, 12 });
rootGrid().Children().Append(host);
```

In both snippets, `rootGrid` represents the panel that hosts the backdrop surface just behind your content.
If a `CornerRadius` is applied on the parent `rootGrid`, that would clip the `SystemBackdropHost` as well.

### Remarks

* _Spec note: This API is currently `experimental`; the API surface may still change before it is finalized._
* The element have to be placed as first element in the container (for example as the first child inside a
    panel) for having the backdrop below the contents.
* The host only connects to a backdrop while it has a `XamlRoot`. If the element is not in the live tree, the backdrop
    remains disconnected until it is loaded again.

## SystemBackdropHost.SystemBackdrop property

Gets or sets the `SystemBackdrop` instance that renders in the host area. The default value is `null`.

* When you assign a non-null backdrop and the host is loaded, it calls `SystemBackdrop.OnTargetConnected` with a
    `ContentExternalBackdropLink` that matches the element's arranged size. If the host is not yet loaded, the connection
    happens the next time it loads.
* Changing the property disconnects the previous backdrop (through `OnTargetDisconnected`) before connecting the new
    value. Setting the property to `null` releases the composition resources and removes the child visual from the
    element.
* You can data bind or animate this property. Typical values include `MicaBackdrop`, `DesktopAcrylicBackdrop`, or a
    custom subclass of `SystemBackdrop`.
* If the host does not yet have a `XamlRoot`, the connection is postponed until one becomes available.

## SystemBackdropHost.CornerRadius property

Gets or sets the `CornerRadius` applied to the hosted backdrop surface. The default value is `CornerRadius(0)`.

* The host applies a `RectangleClip` on the placement visual to achieve the rounded corners.
* Updating the property while the element is loaded immediately refreshes the clip. Setting the property to `null` (for
    example through a binding) clears the stored corner radius and restores square corners.
* This property only affects the backdrop clip. It does not change layout or round other content layered above the
    `SystemBackdropHost`.
* `SystemBackdropHost` would get clipped to the container as well, So the `CornerRadius` on the parent container also would be having same behavior.

# API Details

```csharp
namespace Microsoft.UI.Xaml.Controls
{
    [MUX_PREVIEW]
    [webhosthidden]
    public sealed class SystemBackdropHost : Microsoft.UI.Xaml.FrameworkElement
    {
        public SystemBackdropHost();

        public Microsoft.UI.Xaml.Media.SystemBackdrop SystemBackdrop { get; set; }
        public static Microsoft.UI.Xaml.DependencyProperty SystemBackdropProperty { get; }

        public Microsoft.UI.Xaml.CornerRadius CornerRadius { get; set; }
        public static Microsoft.UI.Xaml.DependencyProperty CornerRadiusProperty { get; }
    }
}
```

# Appendix

Additional property for `SystemBackdropHost` to hold a child content can be considered at a later point based on requirement.
