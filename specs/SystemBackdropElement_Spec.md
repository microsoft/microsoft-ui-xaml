SystemBackdropElement
===

# Background

There are backdrop materials provided in WinUI such as Mica, Acrylic that are subclass of
[Microsoft.UI.Xaml.Media.SystemBackdrop](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.media.systembackdrop). Currently, it is possible to add a system backdrop only at the window level or on flyouts, but not in a specific area in the visual tree. This has been a major limitation on WinUI 3 compared to WinUI 2 in achieving the acrylic / mica effects, especially for achieving various animations.

`SystemBackdropElement` is a lightweight `FrameworkElement` that bridges between the XAML tree and the composition
infrastructure required by `SystemBackdrop`. It creates the required composition components to add the systembackdrop on a specific area, resizes the systembackdrop to fill the given area, and applies the clip on the backdrop visual based on `CornerRadius` values applied on `SystemBackdropElement` which helps for rounded corners to
appear as expected. This control abstracts lot of details for the composition layer and hence make it easy
for WinUI 3 developers to implement the acrylic effect in the applications.

In WinUI 2, it was possible to achieve the backdrop using `BackgroundSource` property of [AcrylicBrush](https://learn.microsoft.com/uwp/api/windows.ui.xaml.media.acrylicbrush?view=winrt-26100), However in WinUI 3, [AcrylicBrush](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.media.acrylicbrush) doesn't provide `BackgroundSource` property leaving it capable of achieving only in-app acrylic. This is due to the limitation of WinUI 3 compositor which is running in-proc, and so can't fetch buffers outside the application window. In this design, the solution is to leverage the [ContentExternalBackdropLink](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.content.contentexternalbackdroplink) API. It provides `PlacementVisual` that would be used for rendering the backdrop in the WinUI 3 visual tree. `SystemBackdropElement` control takes care of resizing and positioning this `PlacementVisual` as per the position, size and Z-order of the control.

## Goals

- Provide an intuitive, XAML-friendly way to place a system backdrop anywhere inside application's visual tree.
- Handle connection, disconnection, and sizing so application only have to set a backdrop and position the element.
- Allow to put rounded corners on the backdrop without writing custom composition code.

## Non-goals

- Adding a SystemBackdrop property independently on all controls.
- Provide a content container; `SystemBackdropElement` is purely a visual effect surface and is not a container.

# Conceptual pages (How To)

The guidance in the below examples can be followed by developers for adopting
`SystemBackdropElement`.

# API Pages

_Each of the following level-two sections correspond to a page that will be on learn.microsoft.com_

## SystemBackdropElement class

Use `SystemBackdropElement` to place a [SystemBackdrop](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.media.systembackdrop) anywhere within the XAML layout.

```csharp
public sealed class SystemBackdropElement : FrameworkElement
```

### Examples

Keep the `SystemBackdropElement` in the bottom of the stack below other contents to achieve the backdrop effect:

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
    <SystemBackdropElement CornerRadius="4"> 
        <SystemBackdropElement.SystemBackdrop> 
            <DesktopAcrylicBackdrop /> 
        </SystemBackdropElement.SystemBackdrop> 
    </SystemBackdropElement> 
    <Button Content="Test Button"/> 
</Grid> 
```

![Example of SystemBackdropElement with animation](images/Acrylic.gif)

The same pattern works from code:

- C#:

```csharp
var backdropElement = new SystemBackdropElement
{
    SystemBackdrop = new MicaBackdrop(),
    CornerRadius = new CornerRadius(12)
};
rootGrid.Children.Add(backdropElement);
```

- C++:

```cpp
winrt::Microsoft::UI::Xaml::Controls::SystemBackdropElement backdropElement;
backdropElement.SystemBackdrop(winrt::Microsoft::UI::Xaml::Media::MicaBackdrop());
backdropElement.CornerRadius(winrt::CornerRadius{ 12, 12, 12, 12 });
rootGrid().Children().Append(backdropElement);
```

In both snippets, `rootGrid` represents the panel that holds the backdrop surface just behind your content.
If a `CornerRadius` is applied on the parent `rootGrid`, that would clip the `SystemBackdropElement` as well.

### Remarks

- _Spec note: This API is currently `experimental`; the API surface may still change before it is finalized._
- It is recommended to be used as first element for background effect to work appropriately, for example as the first child inside a
    panel. (First element added to tree gets rendered first and goes in the bottom of stack)
- The `SystemBackdropElement` only connects to a backdrop while it has a `XamlRoot`. If the element is not in the live tree, the backdrop
    remains disconnected until it is loaded.
- As `SystemBackdropElement` control punches a hole in the layout to show the SystemBackdrop, properties like Shadow, Lights, Clip & Transform may not behave as intended.

## SystemBackdropElement.SystemBackdrop property

Gets or sets the `SystemBackdrop` instance that renders in the `SystemBackdropElement` area. The default value is `null`.

- You can data bind this property. Typical values include `MicaBackdrop`, `DesktopAcrylicBackdrop`, or a
    custom subclass of `SystemBackdrop`.
- If the `SystemBackdropElement` does not yet have a `XamlRoot`, the connection is postponed until one becomes available.

## SystemBackdropElement.CornerRadius property

Gets or sets the `CornerRadius` applied to the backdrop surface. The default value of `CornerRadius` is 0.

- The `SystemBackdropElement` applies a `RectangleClip` on the composition visual of SystemBackdrop to achieve the rounded corners.
- Default `CornerRadius` value will be 0 aligning with [control templates](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.control.cornerradius#remarks) behavior.

# API Details

```csharp
namespace Microsoft.UI.Xaml.Controls
{
    [MUX_PREVIEW]
    [webhosthidden]
    public sealed class SystemBackdropElement : Microsoft.UI.Xaml.FrameworkElement
    {
        public SystemBackdropElement();

        public Microsoft.UI.Xaml.Media.SystemBackdrop SystemBackdrop { get; set; }
        public static Microsoft.UI.Xaml.DependencyProperty SystemBackdropProperty { get; }

        public Microsoft.UI.Xaml.CornerRadius CornerRadius { get; set; }
        public static Microsoft.UI.Xaml.DependencyProperty CornerRadiusProperty { get; }
    }
}
```

# Appendix

Additional property for `SystemBackdropElement` to hold a child content can be considered at a later point based on requirement.
