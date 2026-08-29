AnnotatedScrollBar API Spec
===

# Background

Xaml
[ScrollBar](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.Primitives.ScrollBar)s
are used to scroll content vertically or horizontally.

The new `AnnotatedScrollBar` extends a regular scrollbar's functionality by providing an easy way 
to navigate through a large collection of items. This is achieved via a clickable rail 
with labels which act as markers. It also allows for a more granular understanding of the 
scrollable content by displaying a tooltip when hovering over the clickable rail.

It can be used to navigate a large collection of photos in a manner which can be seen in FileExplorer and PhotoGallery.

![Example of a `AnnotatedScrollBar` used in File Explorer.](./images/annotatedscrollbar-fileexplorer.jpg)

The `AnnotatedScrollBar` can be connected to a scrollable container via the `IScrollController` interface, 
such as with a `ScrollView` [_`IScrollController` and `ScrollPresenter` API specs upcoming_].
An adapter for `IScrollController` must be written for all other scenarios.

# Conceptual pages (How To)

An `AnnotatedScrollBar` is a scroll bar that's used with a large scrolling region, and shows
labels alongside to help the user find a target offset.

It's typically used with a `ScrollView` or an `ItemsView` (which internally uses a `ScrollView`).
For example, this places an `AnnotatedScrollBar` next to an `ItemsView`,
and connects the two via their respective `IScrollController` properties.
App code behind uses event handlers to populate the labels.

```xml
<Grid ColumnDefinitions="*,Auto">
    <ItemsView VerticalScrollController="{x:Bind annotatedScrollBar.ScrollController}"/>
    <AnnotatedScrollBar
        x:Name="annotatedScrollBar"
        Grid.Column="1"
        Loaded="AnnotatedScrollBar_Loaded"
        DetailLabelRequested="AnnotatedScrollBar_DetailLabelRequested"/>
</Grid>
```

Use the `AnnotatedScrollBar.ScrollController` property to connect to a scroller,
or you can equivalently type cast the `AnnotatedScrollBar` to `IScrollController`.
`ItemsView` and `ScrollView` have built in support to consume this interface,
but any control or app can use it.
For example you can write code to connect an `AnnotatedScrollBar` to a 
[ScrollViewer](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.ScrollViewer)
control.
For `ScrollView`, you can put `AnnotatedScrollBar` into a custom `ScrollView` control template
(as the `PART_VerticalScrollBar`).


_Navigation Affordances:_\
The `AnnotatedScrollBar` allows for navigating content via the arrow buttons or clicking/tapping on the rail
(the space between the arrow buttons).
The arrow buttons scroll by the amount defined in the `SmallChange` property.\
The thumb is a non-interactable element that is only for visualizing the current viewport position.\
_Note: The thumb has a fixed height which is not representative of the viewport height._

_Visualization Elements:_\
The control exposes the ability to display two types of labels. 

Labels (if specified) are always visible on the scrollbar and are populated by using the `Labels` property.
Each Label is represented by the `AnnotatedScrollBarLabel` class and requires two pieces of information:
- `Content`: content that will be displayed in the `AnnotatedScrollBarLabel` on the scrollbar
- `ScrollOffset`: offset value at which the `AnnotatedScrollBarLabel` will be placed

_Note: The `AnnotatedScrollBar` does not display Labels that collide with other labels or extend past the bounds of
the rail._

A detail label is an element that is shown only on user interaction (hover and scroll).
You create this element by listening to the `DetailLabelRequested` event.

The example below highlights how you can use the `AnnotatedScrollBar` to scroll `ItemsView` content via the `IScrollController` interface.
It also demonstrates how you can populate the `Labels` and detail labels. In this example, the `Labels` are the years and the detail label
is the month (shown as 'August' in the screenshot).

![Illustration of the `AnnotatedScrollBar` sample below.](./images/annotatedscrollbar-sample.png)

```xml
<Grid ColumnDefinitions="*,Auto">
    <ItemsView 
        VerticalScrollController="{x:Bind annotatedScrollBar.ScrollController}"
        Loaded="ItemsView_Loaded">
        <ItemsView.Layout>
            <LinedFlowLayout 
                x:Name="linedFlowLayout" 
                ItemsStretch="Fill" 
                MinItemSpacing="4" 
                LineSpacing="4" 
                LineHeight="140"/>
        </ItemsView.Layout>
    </ItemsView>
    <AnnotatedScrollBar
        x:Name="annotatedScrollBar"
        Grid.Column="1"
        DetailLabelRequested="AnnotatedScrollBar_DetailLabelRequested"/>
</Grid>
```

```cs
private void ItemsView_Loaded(object sender, RoutedEventArgs args)
{
    PopulateAnnotatedScrollBar();
}

private void PopulateAnnotatedScrollBar()
{    
    annotatedScrollBar.Labels.Clear();

    annotatedScrollBar.Labels.Add(new AnnotatedScrollBarLabel("2023", GetScrollOffsetForYear(2023)));
    annotatedScrollBar.Labels.Add(new AnnotatedScrollBarLabel("2022", GetScrollOffsetForYear(2022)));
    annotatedScrollBar.Labels.Add(new AnnotatedScrollBarLabel("2021", GetScrollOffsetForYear(2021)));
    annotatedScrollBar.Labels.Add(new AnnotatedScrollBarLabel("2020", GetScrollOffsetForYear(2020)));
}

private double GetScrollOffsetForYear(int year)
{
    double lineAndSpacingSize = linedFlowLayout.ActualLineHeight + linedFlowLayout.LineSpacing;

    int itemIndex = GetLastItemIndexForYear(year);
    int lineIndex = linedFlowLayout.LockItemToLine(itemIndex);

    return lineIndex * lineAndSpacingSize;
}

public void AnnotatedScrollBar_DetailLabelRequested(AnnotatedScrollBar sender, AnnotatedScrollBarDetailLabelRequestedEventArgs args)
{
    // Defines the content that will be displayed in the tooltip
    args.Content = GetMonthLabelForOffset(args.ScrollOffset);
}
```





# API pages

## `AnnotatedScrollBar` class

Represents a control that allows for vertical scrolling of a scrollable container's content using
the `IScrollController` interface.
It allows for navigation to explicit positions which are made clear via
always visible labels at definable increments.
It can also display labels in a tooltip on interaction with the scrollbar rail.

This class also implements the `IScrollController` interface,
equivalent to getting the `ScrollController` property.
This means that you can type cast for it, though the type information does not reflect this.
[_Not sure if "cloaked interface" is a public term?_]
This opens up the ability to drop the `AnnotatedScrollBar` into templates that expect
the `IScrollController` interface  such as replacing the 
template part `PART_VerticalScrollBar` in `ScrollView`.


Namespace: Microsoft.UI.Xaml.Controls

C#
```cs
public class AnnotatedScrollBar : Control
```

_See earlier example code_


## AnnotatedScrollBar.ScrollController property

This property exposes the `IScrollController` interface. 
Use this property in order to connect the `AnnotatedScrollBar` to properties of type `IScrollController`.

```xml
<Grid ColumnDefinitions="*,Auto">
    <ItemsView VerticalScrollController="{x:Bind annotatedScrollBar.ScrollController}"/>
    <AnnotatedScrollBar 
        x:Name="annotatedScrollBar"
        Grid.Column="1"/>
</Grid>
```

You can also get this interface value by type casting the class itself.


## AnnotatedScrollBar.Labels property

Gets or sets a collection of `AnnotatedScrollBarLabel` objects which define the labels to be displayed 
by the `AnnotatedScrollBar`.

C#
```cs 
public IList<AnnotatedScrollBarLabel> Labels { get; set; }
```

### Property Value

The default value is an empty collection.


## AnnotatedScrollBar.LabelTemplate property

Gets or sets an
[IElementFactory](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.IElementFactory)
(such as a DataTemplate) used for displaying `AnnotatedScrollBarLabel`s.

C#
```cs
public IElementFactory LabelTemplate { get; set; }
```

If not set a default template is used.
This example shows a custom template:

```xml
<AnnotatedScrollBar>
    <AnnotatedScrollBar.LabelTemplate>
        <DataTemplate  x:DataType="AnnotatedScrollBarLabel">
            <Border MinWidth="{StaticResource LabelsGridMinWidth}">
                <TextBlock
                    Text="{x:Bind Content}"
                    Style="{StaticResource BodyTextBlockStyle}"
                    HorizontalTextAlignment="Right"
                    HorizontalAlignment="Right"
                    TextWrapping="NoWrap"/>
            </Border>
        </DataTemplate>
    </AnnotatedScrollBar.LabelTemplate>
</AnnotatedScrollBar>
```

## AnnotatedScrollBar.DetailLabelTemplate property

Gets or sets an
[IElementFactory](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.IElementFactory)
(such as a DataTemplate) used for displaying detail labels (labels shown in tooltips on user interaction).

C#
```cs
public IElementFactory DetailLabelTemplate { get; set; }
```

## AnnotatedScrollBar.DetailLabelRequested event

Raised upon user interaction with the `AnnotatedScrollBar` track when content for tooltip is required.

C#
```cs
public event EventHandler<AnnotatedScrollBarDetailLabelRequestedEventArgs> DetailLabelRequested;
```

### Remarks

The user interactions that raise this event include mouse hover, mouse click, tap, and scroll (via scrollable container or the `AnnotatedScrollBar` arrow buttons).

## AnnotatedScrollBar.SmallChange property

Represents the offset delta by which the `AnnotatedScrollBar` scrolls when using the up and down arrow buttons.

```cs
public double SmallChange{ get; set; };
```

## AnnotatedScrollBar.Scrolling event

Raised when a user scrolls using the buttons or by interacting with the track.
This event can be cancelled by setting the `AnnotatedScrollBarScrollingEventArgs.Cancel` property to true.
Cancelling this event will prevent `IScrollController` scroll requests from being raised.

C#
```cs
public event TypedEventHandler<AnnotatedScrollBar,AnnotatedScrollBarScrollingEventArgs> Scrolling;
```

_Spec note:_
_We considered naming this event `AnnotatedScrollBar.Scroll` to align with `ScrollBar.Scroll`,
however decided to go with Scrolling for correctness as this event is raised before
`IScrollController` makes a request to scroll._

## AnnotatedScrollBarLabel class

Used to define the content and scroll offset value of a label that is to be displayed by the `AnnotatedScrollBar`.

Namespace: Microsoft.UI.Xaml.Controls

C#
```cs
public class AnnotatedScrollBarLabel
{
    public Object Content { get; set; };
    public Double ScrollOffset { get; set; };
}
```

### Remarks

The `ScrollOffset` property is an offset in the scrollable container frame of reference. 
It should be in the scrollable range of the container.
For example, for `ScrollView` the range is between 0 and `ScrollView.ScrollableHeight`.
If the `ScrollOffset` is not in the scrollable range then the label will not be shown.

## AnnotatedScrollBarDetailLabelRequestedEventArgs class

Provides data for the AnnotatedScrollBar.DetailLabelRequested event.
The `Content` property is used to set the element that will be displayed in the tooltip shown on interaction.
The `ScrollOffset` property specifies the offset at which the tooltip will be displayed. 

Namespace: Microsoft.UI.Xaml.Controls

```cs
public class AnnotatedScrollBarDetailLabelRequestedEventArgs
{
    public Object Content { get; set; }
    public Double ScrollOffset { get; }
}
```

### Remarks

The provided Label will be displayed in a tooltip.

The `ScrollOffset` property is an offset in the scrollable container frame of reference. It will be in the 
scrollable range of the container. 
For example, for `ScrollView` the range is between 0 and `ScrollView.ScrollableHeight`.

## AnnotatedScrollBarScrollingEventArgs class

Provides data for the AnnotatedScrollBar.Scrolling event.

Namespace: Microsoft.UI.Xaml.Controls

```cs
public class AnnotatedScrollBarScrollingEventArgs
{
	public Double ScrollOffset { get; }
    public AnnotatedScrollBarScrollingEventKind ScrollingEventKind{ get; };
    public Boolean Cancel { get; set; }
}
```

## AnnotatedScrollBarScrollingEventKind enum

Defines the event type that triggered the Scrolling event.

Namespace: Microsoft.UI.Xaml.Controls

| Name | Value | Description |
|-|-|-|
| Click | 0 | Scroll was raised via user clicking/tapping on the `AnnotatedScrollBar` rail |
| Drag | 1 | Scroll was raised by a drag action on the rail |
| IncrementButton | 2 | Scroll was raised by invoking the increment button |
| DecrementButton | 3 | Scroll was raised by invoking the decrement button |





# API Details

```cs (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls
{

enum AnnotatedScrollBarScrollingEventKind
{
    Click = 0,
    Drag = 1,
    IncrementButton = 2,
    DecrementButton = 3,
};

runtimeclass AnnotatedScrollBarDetailLabelRequestedEventArgs
{
    Object Content{ get; set; };
    Double ScrollOffset{ get; };
}

runtimeclass AnnotatedScrollBarScrollingEventArgs
{
    Double ScrollOffset{ get; };
    AnnotatedScrollBarScrollingEventKind ScrollingEventKind{ get; };
    Boolean Cancel { get; set; }
}

runtimeclass AnnotatedScrollBarLabel
{ 
    AnnotatedScrollBarLabel(Object content, Double scrollOffset);

    Object Content{ get; };
    Double ScrollOffset{ get; };
}

unsealed runtimeclass AnnotatedScrollBar : Microsoft.UI.Xaml.Controls.Control
{
    AnnotatedScrollBar();

    Microsoft.UI.Xaml.Controls.Primitives.IScrollController ScrollController{ get; };
    Windows.Foundation.Collections.IVector<AnnotatedScrollBarLabel> Labels{ get; set; };
    IElementFactory LabelTemplate{ get; set; };
    IElementFactory DetailLabelTemplate{ get; set; };
    Double SmallChange{ get; set; };

    event Windows.Foundation.TypedEventHandler<AnnotatedScrollBar, 
        AnnotatedScrollBarScrollingEventArgs>
        Scrolling;
    event Windows.Foundation.TypedEventHandler<AnnotatedScrollBar,
        AnnotatedScrollBarDetailLabelRequestedEventArgs>
        DetailLabelRequested;

    static Microsoft.UI.Xaml.DependencyProperty LabelsProperty{ get; };
    static Microsoft.UI.Xaml.DependencyProperty LabelTemplateProperty{ get; };
    static Microsoft.UI.Xaml.DependencyProperty DetailLabelTemplateProperty{ get; };
    static Microsoft.UI.Xaml.DependencyProperty SmallChangeProperty{ get; };
}

}

```

# XAML markup considerations

## Theme resources

### Brushes

- ScrollButtonBackground
- ScrollButtonForeground
- ScrollButtonForegroundPointerOver
- ScrollButtonForegroundPressed
- ScrollButtonForegroundDisabled
- ScrollButtonBorderBrush
- VerticalThumbBrush

### Other Resources

- `CornerRadius` ThumbCornerRadius
- `Double` ThumbHeight
- `Double` ThumbWidth
- `Double` LabelsGridMinWidth
- `Double` AnnotatedScrollBarTooltipMaxWidth
- `Double` AnnotatedScrollBarTooltipMinHeight
- `Double` ScrollButtonFontSize
- `Thickness` ScrollButtonStyleBorderThickness

# Input handling

## Mouse handling

### Mouse buttons handling

Scrolling is initiated on PointerPressed. The pointer is captured while the mouse left click is being pressed which allows for the `AnnotatedScrollBar` to continue scrolling on mouse move.

## Touch handling

Touch handling behaves the same as mouse handling. The only exception is that the tooltip is raised higher on tap to keep it visible.

# Accessibility considerations

The `AnnotatedScrollBar` currently does not expose any AutomationPeer. 
The ability to scroll should be exposed via the scrollable container to which the AnnotatedScrollBar
is attached to.

## Tooltip usage

The XAML part `PART_ToolTipRail` spans the space between the arrow buttons and displays tooltips on hover.









# Appendix

## ScrollViewer example

To use the `AnnotatedScrollBar` with controls that do not have support for the `IScrollController` interface,
you can write a custom adapter.
Below is an example adapter which connects the `AnnotatedScrollBar` with a
[ScrollViewer](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.ScrollViewer).


```xml
<Grid ColumnDefinitions="*,Auto">
    <ScrollViewer x:Name="scrollViewer"/>
    <AnnotatedScrollBar
        x:Name="annotatedScrollBar"
        Grid.Column="1"/>
</Grid>
```

```cs
public sealed partial class ScrollViewerPage
{
    private IScrollControllerAdapter adapter = null;

    public ScrollViewerPage()
    {
        this.InitializeComponent();
        adapter = new IScrollControllerAdapter(scrollViewer, annotatedScrollBar.ScrollController);
    }
}

```

```cs
public class IScrollControllerAdapter
{
    private ScrollViewer _scrollViewer;
    private IScrollController _scrollController;
    static private double s_velocityNeededPerPixel = 7.600855902349023;

    public IScrollControllerAdapter(ScrollViewer sv, IScrollController scrollController)
    {
        _scrollViewer = sv;
        _scrollController = scrollController;
        SetUpBridge();
    }

    private void SetUpBridge()
    {
        _scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;
        _scrollViewer.ViewChanged += pageScrollViewer_ViewChanged;
        _scrollViewer.SizeChanged += pageScrollViewer_SizeChanged;

        _scrollController.ScrollToRequested += _scrollController_ScrollToRequested;
        _scrollController.ScrollByRequested += _scrollController_ScrollByRequested;
        _scrollController.AddScrollVelocityRequested += _scrollController_AddScrollVelocityRequested;
    }

    private void _scrollController_AddScrollVelocityRequested(IScrollController sender, ScrollControllerAddScrollVelocityRequestedEventArgs args)
    {
        var offsetDelta = args.OffsetVelocity / s_velocityNeededPerPixel;
        var newOffset = _scrollViewer.VerticalOffset + offsetDelta;
        newOffset = Math.Max(0, newOffset);
        newOffset = Math.Min(_scrollViewer.ScrollableHeight, newOffset);
        _scrollViewer.ChangeView(null, newOffset, null, false /*disableAnimation*/);
    }

    private void _scrollController_ScrollByRequested(IScrollController sender, ScrollControllerScrollByRequestedEventArgs args)
    {
        var newOffset = _scrollViewer.VerticalOffset + args.OffsetDelta;
        newOffset = Math.Max(0, newOffset);
        newOffset = Math.Min(_scrollViewer.ScrollableHeight, newOffset);
        _scrollViewer.ChangeView(null, newOffset, null, false /*disableAnimation*/);
    }

    private void _scrollController_ScrollToRequested(IScrollController sender, ScrollControllerScrollToRequestedEventArgs args)
    {
        _scrollViewer.ChangeView(null, args.Offset, null, true /*disableAnimation*/);
    }

    private void pageScrollViewer_SizeChanged(object sender, object e)
    {
        _scrollController.SetValues(0, _scrollViewer.ScrollableHeight, _scrollViewer.VerticalOffset, _scrollViewer.ViewportHeight);
    }

    private void pageScrollViewer_ViewChanged(object sender, object e)
    {
        _scrollController.SetValues(0, _scrollViewer.ScrollableHeight, _scrollViewer.VerticalOffset, _scrollViewer.ViewportHeight);
    }
}
```

