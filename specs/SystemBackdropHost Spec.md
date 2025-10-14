SystemBackdropHost
===

# Background

System backdrop materials such as Mica, Acrylic, and future platform-provided surfaces are delivered through the
`Microsoft.UI.Xaml.Media.SystemBackdrop` API surface. When apps want to expose these surfaces inside specific areas in the UI (as opposed to applying them to the full window), they need a XAML element that can connect the backdrop to the
visual tree, manage its lifetime, and clip it to the intended layout bounds. Today, the only supported way to host a
system backdrop is at the window level, which makes it awkward to layer the effect behind individual sections of UI or
inside islands.

`SystemBackdropHost` is a lightweight `FrameworkElement` that bridges between the XAML tree and the composition
infrastructure required by `SystemBackdrop`. It automatically creates the composition link that a backdrop needs,
maintains the placement visual at the arranged size of the element, and translates the element's `CornerRadius` to the
backdrop clip so that rounded surfaces are respected as required.

## Goals

* Provide an intuitive, XAML-friendly way to place a system backdrop anywhere inside an app's visual tree.
* Ensure the host handles connection, disconnection, and sizing so that apps only need to set a backdrop and place the
  element.
* Allow apps to round the hosted backdrop without writing custom composition code.

## Non-goals

* Replace the window-level backdrop APIs on `Window` or `AppWindow`.
* Provide a content container; `SystemBackdropHost` is purely a visual effect surface and does not host child content.

# API Pages

## SystemBackdropHost class

```csharp
public sealed class SystemBackdropHost : FrameworkElement
```

### Remarks

* The control is marked `[MUX_PREVIEW]`; the API may change before it is finalized.
* When the element is loaded, it creates a `ContentExternalBackdropLink` and connects it to the assigned
  `SystemBackdrop`. When unloaded, or when the backdrop is removed, it releases composition resources and disconnects the backdrop.
* The control does not render any visuals of its own. Place it behind content (for example as the first child in a `Grid`) to expose the backdrop surface.
* The host only connects to a backdrop while it has a `XamlRoot`. If the element is not in the live tree, the backdrop will remain disconnected until it is loaded again.

### Example

XAML markup that layers a Acrylic material behind a page header with rounded corners and animation:

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


The control can be configured in code behind as well like below:

C#
```csharp
var backdropHost = new SystemBackdropHost
{
    SystemBackdrop = new DesktopAcrylicBackdrop(),
    CornerRadius = new CornerRadius(4),
};
MainGrid.Children.Add(backdropHost);
```

C++
```cpp
auto backdrop = winrt::Microsoft::UI::Xaml::Controls::SystemBackdropHost();
backdrop.SystemBackdrop(winrt::Microsoft::UI::Xaml::Media::DesktopAcrylicBackdrop());
backdrop.CornerRadius(winrt::Microsoft::UI::Xaml::CornerRadius({4}));
AnimatedGrid().Children().Append(backdrop);
```

## SystemBackdropHost.SystemBackdrop property

Gets or sets the `SystemBackdrop` instance that should render within this host. The default value is `null`.

* When set to a non-null systembackdrop and the host is loaded, the host invokes `SystemBackdrop.OnTargetConnected` with a `ContentExternalBackdropLink` that matches the arranged size of the element. The connection occurs lazily the next time the element is loaded if it is not already in the live tree.
* Changing the property disconnects the previously assigned backdrop (via `OnTargetDisconnected`) before connecting the new value. Setting the property back to `null` releases the composition resources and removes the child visual from the element.
* The property supports data binding and can be animated. Applications typically assign platform-provided backdrop types such as `MicaBackdrop`, `DesktopAcrylicBackdrop`, or custom subclasses of `SystemBackdrop`.
* If the element does not yet have a `XamlRoot`, the connection is postponed until one becomes available.

## SystemBackdropHost.CornerRadius property

Gets or sets the `CornerRadius` that should be applied to the hosted backdrop surface. The default value is
`CornerRadius(0)`.

* The host uses a `RectangleClip` applied to the backdrop's placement visual so that the rounded corner can be applied on all four edges.
* Setting the property while the element is loaded immediately updates the clip. Setting it to `null` (from markup or a binding) clears the corner radius and restores square corners.
* This property only affects the clip applied to the backdrop; it does not change layout or apply corner rounding to other child elements that may be layered on top.

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
