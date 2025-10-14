SystemBackdropHost
===

# Background

System backdrop materials such as Mica, Acrylic etc are delivered through
`Microsoft.UI.Xaml.Media.SystemBackdrop`. When you want to expose those materials inside a specific region of your UI
rather than across an entire window, you need an element that can connect the backdrop to the visual tree, manage its
lifetime, and clip it to the layout bounds. Today, you can only host a system backdrop at the window level, which makes
it awkward to layer the effect behind individual sections of content or inside islands. There is no public stable API available
in composition layer as well which allow apps to add backdrops in a specific area.

`SystemBackdropHost` is a lightweight `FrameworkElement` that bridges between the XAML tree and the composition
infrastructure required by `SystemBackdrop`. It creates the composition link a backdrop needs, keeps the placement
visual sized to the arranged bounds, and applies the element's `CornerRadius` to the backdrop clip so rounded corners
appear as expected.

## Goals

* Provide an intuitive, XAML-friendly way to place a system backdrop anywhere inside application's visual tree.
* Handle connection, disconnection, and sizing so application only have to set a backdrop and position the element.
* Allow to round the hosted backdrop without writing custom composition code.

## Non-goals

* Supporting backdrop independently on all controls.
* Provide a content container; `SystemBackdropHost` is purely a visual effect surface and does not host child content.

# Conceptual pages (How To)

The guidance in the below examples can be followed by developers for adopting
`SystemBackdropHost`.

# API Pages

_(Each level-two section below maps to a docs.microsoft.com API page.)_

## SystemBackdropHost class

Use `SystemBackdropHost` to place a system-provided material anywhere within your XAML layout.

```csharp
public sealed class SystemBackdropHost : FrameworkElement
```

### Examples

You can place the host behind header content while keeping the rest of the page unchanged:

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

```csharp
var host = new SystemBackdropHost
{
    SystemBackdrop = new MicaBackdrop(),
    CornerRadius = new CornerRadius(12)
};
rootGrid.Children.Add(host);
```

```cpp
winrt::Microsoft::UI::Xaml::Controls::SystemBackdropHost host;
host.SystemBackdrop(winrt::Microsoft::UI::Xaml::Media::MicaBackdrop());
host.CornerRadius(winrt::CornerRadius{ 12, 12, 12, 12 });
rootGrid().Children().Append(host);
```

In both snippets, `rootGrid` represents the panel that hosts the backdrop surface just behind your content.

### Remarks

* _Spec note: This type is currently marked `[MUX_PREVIEW]`; the API surface may still change before it is finalized._
* The host creates a `ContentExternalBackdropLink` when it is loaded and a `SystemBackdrop` is assigned. When you
    remove the backdrop or the element unloads, the host releases the composition resources.
* The element does not render visuals of its own. Place it behind other content (for example as the first child inside a
    panel) to expose the backdrop surface.
* The host only connects to a backdrop while it has a `XamlRoot`. If the element is not in the live tree, the backdrop
    remains disconnected until it is loaded again.

## SystemBackdropHost.SystemBackdrop property

Gets or sets the `SystemBackdrop` instance that renders within this host. The default value is `null`.

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
    host.

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
