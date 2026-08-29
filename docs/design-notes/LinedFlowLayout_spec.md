LinedFlowLayout API spec
===

# Background

The [ItemsRepeater](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.ItemsRepeater)
takes in a list of object items and generates a list of elements (item containers).
The [ItemsRepeater.Layout](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.ItemsRepeater.Layout)
property lets you specify a layout implementation for how the elements are laid out.

For example this lays out a list of people in a vertical stack with 20px spacing between the elements:

```xml
<ItemsRepeater ItemsSource="{x:Bind People}" ItemTemplate="{StaticResource PersonTemplate}">
    <ItemsRepeater.Layout>
        <StackLayout Spacing="20" />
    </ItemsRepeater.Layout>
</ItemsRepeater>
```

(`ItemsRepeater` is a primitive used by the new `ItemsView` control,
which adds higher level features such as scrolling.)

The new `LinedFlowLayout` in this spec is a `Layout` derived class that can be assigned to
the `ItemsRepeater` or `ItemsView`'s `Layout` property.
It lays out UI elements from left to right and top to bottom in horizontal lines of equal height.

_Spec note:_

_In the future, LinedFlowLayout may expose an Orientation property which would add the support for vertical 
lines of equal width. LinedFlowLayout API names use the term Line instead of Row for that reason._

Its typical usage being in the context of an `ItemsRepeater` or `ItemsView`, the UI elements laid out represent items 
coming from a data collection. Being a `VirtualizingLayout` subclass, it supports UI virtualization. Therefore when 
used within a scrolling control such as the `ScrollView`, the UI elements are only created for a subset of the data 
collection - the subset that corresponds to the scrolling control's viewport and its neighborhood.

The `UniformGridLayout` is a similar virtualizing layout class, but it lays out the UI elements in rows and columns,
using a uniform width and height. With the `LinedFlowLayout` only one dimension is uniform. Also note that some elements 
may be cropped to fit in their line.

The canonical usage for the `LinedFlowLayout` is to display picture collections. 

![Illustration of canonical LinedFlowLayout usage](Images/LinedFlowLayout_Background.png)

# Conceptual pages (How To)

The `LinedFlowLayout` class is typically used to lay out the items of the `ItemsView` collection control. 
It is particularly useful for displaying collection of pictures. It does so by laying them out from left 
to right, and top to bottom, in lines of equal height. The pictures fill a horizontal line and then wrap 
to a next line. Pictures may be cropped at the left and right edges to fit into a line. They may also be 
expanded horizontally and cropped at the top and bottom edges to fill a line when the stretching mode is 
employed.

## Example with default property values

In this example, no property is set on the `LinedFlowLayout`.
In particular, its [`LineHeight`](#riverflowlayoutlineheight-property) 
property uses the default `NaN` double value.
This means that the first photo in the collection dictates how tall all lines are going to be. If the 
natural desired size of the first `Image` is 250 x 150 pixels, all lines will be 150 pixels tall, and 
the [`ActualLineHeight`](#riverflowlayoutactuallineheight-property) property retuns 150.

Images (except the first one) may be expanded or shrunk to adopt the `ActualLineHeight` of 150. 
Because their `Stretch` property is set to `UniformToFill`, their aspect ratio is preserved, but 
they may be cropped (left and right edges) to make them fit in a line.

C#
```cs
namespace PhotosApp
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();

            this.Photos = new ObservableCollection<Photo>();
            PopulatePhotos();
        }

        public ObservableCollection<Photo> Photos
        {
            get;
            private set;
        }

        private void PopulatePhotos()
        {
            // Populates the this.Photos collection 
        }
    }
    
    public class Photo
    {
        public BitmapImage PhotoBitmapImage
        {
            get;
            set;
        }
    }
}
```

XAML
```xml
<Page
    x:Class="PhotosApp.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PhotosApp">

    <ItemsView ItemsSource="{x:Bind Photos}">
        <ItemsView.ItemTemplate>
            <DataTemplate x:DataType="local:Photo">
                <ItemContainer>
                    <Image 
                        Source="{x:Bind PhotoBitmapImage, Mode=OneWay}"
                        Stretch="UniformToFill"
                        HorizontalAlignment="Center"
                        VerticalAlignment="Center"/>
                </ItemContainer>
            </DataTemplate>
        </ItemsView.ItemTemplate>
        <ItemsView.Layout>
            <LinedFlowLayout/>
        </ItemsView.Layout>
    </ItemsView>
</Page>
```

![LinedFlowLayout example with default property values](Images/LinedFlowLayout_Conceptual1.png)

There is no spacing between the lines and the items within a line. The items are packed to the left by default.

## Example with explicit line height and item spacing

In this example, an explicit height is given to the lines through the [`LineHeight`](#riverflowlayoutlineheight-property) 
property, lines are spaced out through the [`LineSpacing`](#riverflowlayoutlinespacing-property) property, 
and items are spaced out through the [`MinItemSpacing`](#riverflowlayoutminitemspacing-property) property. 
The [`ActualLineHeight`](#riverflowlayoutactuallineheight-property) property retuns 100.

XAML
```xml
<Page
    x:Class="PhotosApp.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PhotosApp">

    <ItemsView ItemsSource="{x:Bind Photos}">
        <ItemsView.ItemTemplate>
            <DataTemplate x:DataType="local:Photo">
                <ItemContainer>
                    <Image
                        Source="{x:Bind PhotoBitmapImage, Mode=OneWay}"
                        Stretch="UniformToFill"
                        HorizontalAlignment="Center"
                        VerticalAlignment="Center"/>
                </ItemContainer>
            </DataTemplate>
        </ItemsView.ItemTemplate>
        <ItemsView.Layout>
            <LinedFlowLayout
                LineHeight="100"
                LineSpacing="10"
                MinItemSpacing="5"/>
        </ItemsView.Layout>
    </ItemsView>
</Page>
```

![LinedFlowLayout example with explicit sizing and spacing](Images/LinedFlowLayout_Conceptual2.png)

The items are packed to the left by default. This is due to the default `LinedFlowLayout.ItemsJustification` 
value of `Start`, and default `LinedFlowLayout.ItemsStretch` value of `None`. Changing the `ItemsJustification` 
property results in a different horizontal distribution.

## Example with empty space evenly distributed between items

In this example, the empty space is evenly distributed between a line's items using 
the [`ItemsJustification`](#riverflowlayoutitemsjustification-enum) property.

XAML
```xml
<Page
    x:Class="PhotosApp.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PhotosApp">

    <ItemsView ItemsSource="{x:Bind Photos}">
        <ItemsView.ItemTemplate>
            <DataTemplate x:DataType="local:Photo">
                <ItemContainer>
                    <Image
                        Source="{x:Bind PhotoBitmapImage, Mode=OneWay}"
                        Stretch="UniformToFill"
                        HorizontalAlignment="Center"
                        VerticalAlignment="Center"/>
                </ItemContainer>
            </DataTemplate>
        </ItemsView.ItemTemplate>
        <ItemsView.Layout>
            <LinedFlowLayout
                LineHeight="100"
                LineSpacing="10"
                MinItemSpacing="5"
                ItemsJustification="SpaceBetween"/>
        </ItemsView.Layout>
    </ItemsView>
</Page>
```

![LinedFlowLayout example with empty space evenly distributed between items](Images/LinedFlowLayout_Conceptual3.png)

## Example with items stretching to fill the lines

In this example, the items are potentially stretched to fill the lines, which involves 
both their top and bottom edges being cropped because the `Image.Stretch` property is 
set to `UniformToFill`. This is done with the [`ItemsStretch`](#riverflowlayoutitemsstretch-property) 
property.

XAML
```xml
<Page
    x:Class="PhotosApp.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PhotosApp">

    <ItemsView ItemsSource="{x:Bind Photos}">
        <ItemsView.ItemTemplate>
            <DataTemplate x:DataType="local:Photo">
                <ItemContainer>
                    <Image
                        Source="{x:Bind PhotoBitmapImage, Mode=OneWay}"
                        Stretch="UniformToFill"
                        HorizontalAlignment="Center"
                        VerticalAlignment="Center"/>
                </ItemContainer>
            </DataTemplate>
        </ItemsView.ItemTemplate>
        <ItemsView.Layout>
            <LinedFlowLayout
                LineHeight="100"
                LineSpacing="10"
                MinItemSpacing="5"
                ItemsStretch="Fill"/>
        </ItemsView.Layout>
    </ItemsView>
</Page>
```

![LinedFlowLayout example with explicit sizing and spacing](Images/LinedFlowLayout_Conceptual4.png)


# API pages

## LinedFlowLayout class

Positions elements sequentially from left to right, then top to bottom, in a wrapping layout, using 
the same height for all elements.  

Namespace: Microsoft.UI.Xaml.Controls

C#
```cs
public class LinedFlowLayout : VirtualizingLayout
```

### Remark

Unlike some other `Layout` derived types, a single `LinedFlowLayout` instance cannot be used by 
multiple `ItemsRepeater` instances at the same time. So for example, each `ItemsView` instance must 
use its own `LinedFlowLayout` instance.

### Example

C#
```cs
void AddPhotosView(Panel panel)
{
    var myLinedFlowLayout = new LinedFlowLayout() {};
    var myItemsView = new ItemsView() { ItemsSource = this.Photos; Layout = myLinedFlowLayout };

    panel.Children.Add(myItemsView);
}
```


## LinedFlowLayout.ItemsJustification property

Gets or sets a value that indicates how items are aligned on the horizontal axis.

C#
```cs
public LinedFlowLayoutItemsJustification ItemsJustification { get; set; }
```

_Spec note:_
_These values are a copy of [UniformGridLayout.ItemsJustification](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.UniformGridLayout.ItemsJustification)_


### Property Value

An enumeration value that indicates how items are aligned. The default is `Start`.

![Illustration of LinedFlowLayout's ItemsJustification values](Images/LinedFlowLayout_ItemsJustification.png)

The `ItemsJustification` property affects the _arrange_ pass of layout.

### Example

XAML
```xml
<Page
    x:Class="PhotosApp.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PhotosApp">

    <ItemsView
        x:Name="photosView"
        ItemsSource="{x:Bind Photos}">
        <ItemsView.Layout>
            <LinedFlowLayout ItemsJustification="SpaceAround"/>
        </ItemsView.Layout>
    </ItemsView>
</Page>
```


## LinedFlowLayout.ItemsStretch property

Gets or sets a value that indicates how items are sized to fill the available space.

C#
```cs
public LinedFlowLayoutItemsStretch ItemsStretch { get; set; }
```

_Spec note:_
_Property name and enum values are copied from [UniformGridLayout.ItemsStretch](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.controls.uniformgridlayout.itemsstretch?view=winui-2.8)_


### Property Value

An enumeration value that indicates how items are sized to fill the available space. The default is `None`.

![Illustration of LinedFlowLayout's ItemsStretch values](Images/LinedFlowLayout_ItemsStretch.png)

The `ItemsStretch` property affects the _measure_ pass of layout.

### Examples

XAML
```xml
<Page
    x:Class="PhotosApp.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PhotosApp">

    <ItemsView>
        <ItemsView.Layout>
            <LinedFlowLayout ItemsStretch="Fill"/>
        </ItemsView.Layout>
    </ItemsView>
</Page>
```

In the two examples below, the `ItemsStretch` property is set to `None` and `Fill` respectively.
Note that the last line is not affected by the property value.

![Illustration of ItemsStretch's None value](Images/LinedFlowLayout_ItemsStretchNone.png)

![Illustration of ItemsStretch's Fill value](Images/LinedFlowLayout_ItemsStretchFill.png)


## LinedFlowLayout.MinItemSpacing property

Gets or sets the minimum space between items on the horizontal axis.

C#
```cs
public double MinItemSpacing { get; set; }
```

_Spec note:_
_Property name is inspired from [UniformGridLayout.MinColumnSpacing](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.controls.uniformgridlayout.mincolumnspacing?view=winui-2.8)_
_and [UniformGridLayout.LineSpacing](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.controls.uniformgridlayout.LineSpacing?view=winui-2.8)_

### Property Value

The minimum space (in pixels) between items on the horizontal axis. Default is zero.

In the example below, the `MinItemSpacing` property is set to 6.

![Illustration of LinedFlowLayout's MinItemSpacing property](Images/LinedFlowLayout_MinItemSpacing.png)

### Remarks

The spacing may exceed this minimum value when `ItemsStretch` is set to `None` and 
`ItemsJustification` is set to `SpaceEvenly`, `SpaceAround`, or `SpaceBetween`.


### Example

XAML
```xml
<Page
    x:Class="PhotosApp.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PhotosApp">

    <ItemsView>
        <ItemsView.Layout>
            <LinedFlowLayout MinItemSpacing="6"/>
        </ItemsView.Layout>
    </ItemsView>
</Page>
```


## LinedFlowLayout.LineSpacing property

Gets or sets the vertical space between items.

C#
```cs
public double LineSpacing { get; set; }
```

_Spec note:_
_Property name is inspired from [StackLayout.Spacing](https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.controls.stacklayout.spacing?view=winui-2.8)_

### Property Value

The space (in pixels) between items on the vertical axis. Default is zero.

In the example below, the `LineSpacing` property is set to 10.

![Illustration of LinedFlowLayout's LineSpacing property](Images/LinedFlowLayout_LineSpacing.png)


_Spec note:_
_Because the Orientation property see [LinedFlowLayout.Orientation](#riverflowlayoutorientation-property) may be added at a later time,_
_the LineSpacing property for now represents the row spacing. Later, when `Orientation` is set to `Horizontal`, it will represent the column spacing._


### Example

XAML
```xml
<Page
    x:Class="PhotosApp.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PhotosApp">

    <ItemsView>
        <ItemsView.Layout>
            <LinedFlowLayout LineSpacing="6"/>
        </ItemsView.Layout>
    </ItemsView>
</Page>
```


## LinedFlowLayout.LineHeight property

Gets or sets the lines fixed height.

C#
```cs
public double LineHeight { get; set; }
```

### Property Value

Represents the common height of all lines, and therefore of all items. Default is `NaN`.
When the default `NaN` value is set, the `LinedFlowLayout` uses the desired height (`UIElement.DesiredSize.Height`) 
of the item at index 0 as a fallback value. 

In the example below, the `LineHeight` property is set to 96.

![Illustration of LinedFlowLayout's LineHeight property](Images/LinedFlowLayout_LineHeight.png)

_Spec note:_
_Because the Orientation property see [LinedFlowLayout.Orientation](#riverflowlayoutorientation-property) may be added at a later time,_
_the LineHeight property always represents the fixed height of the rows for now. Later, when `Orientation` is set to `Horizontal`, the_
_LineHeight will represent the fixed columns width._
_Thus, when LineHeight is `NaN`, UIElement.DesiredSize.Height for the first item in the collection is used as the fixed line height._


### Example

XAML
```xml
<Page
    x:Class="PhotosApp.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PhotosApp">

    <ItemsView>
        <ItemsView.Layout>
            <LinedFlowLayout LineHeight="96"/>
        </ItemsView.Layout>
    </ItemsView>
</Page>
```


## LinedFlowLayout.ActualLineHeight property

Gets the lines fixed height.

C#
```cs
public double ActualLineHeight { get; }
```

### Property Value

Because the `LineHeight` property may be set to its default `NaN` value, the `ActualLineHeight` property 
can be used to retrieve the effective height of the lines. That fallback value is based on the desired 
size (`UIElement.DesiredSize`) of the item at index 0. 
Returns zero when the `LinedFlowLayout`'s consuming `ItemsRepeater` has not been loaded or has not yet 
been involved in a layout pass that renders the UI.

## LinedFlowLayout.ItemsInfoRequested event

The `LinedFlowLayout` raises this event to gather sizing information for items in and around the current scrolling viewport.
You can optionally provide the requested information to improve the responsiveness of the user interface.

C#
```cs
public event TypedEventHandler<LinedFlowLayout, LinedFlowLayoutItemsInfoRequestedEventArgs> ItemsInfoRequested;
```

### Remarks

The `ItemsInfoRequested` event is raised with the `LinedFlowLayoutItemsInfoRequestedEventArgs` argument 
which provides details about the requested item sizing information.

Handling of this event is optional and influences the layout algorithm employed by the `LinedFlowLayout` 
in the following ways:

**Not handling this event, or not providing sizing information for the requested items range:**

- A contiguous range of items is created. By default, those items fill up a buffer of up to 5 
scrolling viewports, centered around the currently visible viewport. All other items are virtualized.
- Only that range of items is laid out. Items outside the range, i.e. virtualized items, do not 
influence the layout.
- Scrolling through the items, and more generally each layout pass, triggers the `ItemsInfoRequested` 
event in order to attempt gathering sizing information again.
- Recently created items are used to compute an average items-per-line value. That number determines 
how many lines are needed to display the entire collection, and the approximate location of each item.
- A width change of the hosting `ItemsRepeater` may or may not trigger an average items-per-line change.
The greater that width change, the greater the chance of an average items-per-line change.
- A complete reflow of the items is performed when that average items-per-line evaluates to a new value 
for any reason.
- The number of items created and laid out in the buffer (up to 5 scrolling viewports) is strictly 
enforced by the computed average items-per-line.

**Providing sizing information exactly for the requested items range:**

Results in the same charateristics as in the case above, except:
- Only up to 3 scrolling viewports worth of items are created instead of 5. Fewer created items 
translates into improved performance. Thus it is recommended to handle the `ItemsInfoRequested` event 
and provide sizing information for at least the requested range which covers up to 5 scrolling 
viewports.
- The average items-per-line is computed based on the provided sizing information and not the 
created items' desired size.
- Scrolling through the items triggers the `ItemsInfoRequested` event in order to gather sizing 
information for the buffer (up to 5 scrolling viewports) centered around the new visible viewport.

**Providing sizing information for a greater items range than the requested one:**

Results in the same charateristics as in the case above, except:
- Because sizing information was provided for a superset of the requested items range, scrolling 
does not necessarily raise the `ItemsInfoRequested` event. As long as the 5 scrolling viewports 
centered around the visible viewport are within the known sizes, the `ItemsInfoRequested` does not
need to be raised.
- Thus the larger the items range of sizing information provided by the `ItemsInfoRequested` event 
handler, the fewer its occurrences. This improves the overall performance.

**Providing sizing information for the entire items collection:**

This case results in significantly different characteristics from the 3 cases above:
- Up to 3 scrolling viewports worth of items are created.
- Bounding rectangles are evaluated for _all_ items, whether created or virtualized. 
This takes full advantage of the sizing information provided for the entire collection.
- There is no enforcement about how many items are laid out around the visible viewport.
Not being constrained to fulfill an average items-per-line may result in less item cropping.
The total number of lines is also not enforced by the average items-per-line.
- Scrolling through the items never triggers the `ItemsInfoRequested` event because no 
additional sizing information needs to be retrieved. Calling the `InvalidateItemsInfo` method 
is a way to trigger that event though. 
- A width change of the hosting `ItemsRepeater` always triggers a reflow of the items.

_Spec note:_

_The three first cases are known as the 'regular path', whereas the last case is known as the 'fast path'. Here's Fast path algorithm overview:_

_- aspect ratios, min and max widths gathered for the entire source collection are used to do a layout of all items._

_- items are cumulated on the current line as long as their desired width does not exceed the available width._

_- once the next item goes beyond the available size, a key decision needs to be made: should that item be added 
to the current line or should it create a new line?_

_- two scale factors are then computed:_

_* the scale factor < 1 required to shrink the items in the current line to fit the available size, were the next item appended._

_* the scale factor > 1 required to grow the items in the current line to fill the available size, were the next item create a new line._

_- whichever resulting scale factor is closest to 1 wins. The next item is appended or creates a new line accordingly._


_It is a simple algorithm which allows to go through the entire collection rather fast._

_After a certain collection size threshold (which has not been determined yet through performance analysis), the time required to do 
the full collection layout and its memory requirements negate the efficiency of the algorithm. After that threshold, the regular path 
becomes more efficient.
The regular path algorithm is more complexe but operates on fewer items._

### Example

In the following example, the `ItemsInfoRequested` handler always provides the requested sizing 
information, even if it means using temporary placeholder values of 0.0 for unknown aspect ratios.

If there are additional known aspect ratios around the requested range, they are also returned up 
to a total of 8K items.

XAML
```xml
<Page
    x:Class="PhotosApp.MainPage"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:PhotosApp">

    <ItemsView 
        x:Name="photosView"
        ItemsSource="{x:Bind Photos}">
        <ItemsView.Layout>
            <LinedFlowLayout LineHeight="120" ItemsInfoRequested="LinedFlowLayout_ItemsInfoRequested"/>
        </ItemsView.Layout>
    </ItemsView>
</Page>
```

C#
```cs
namespace PhotosApp
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        // ...

        private bool ItemsHaveMinWidth
        {
            get
            {
                // returns True when items must use a minimum width
                ...
            }
        }

        private bool ItemsHaveMaxWidth
        {
            get
            {
                // returns True when items must use a maximum width
                ...
            }
        }

        private bool ItemsHaveUniformMinWidths
        {
            get
            {
                if (ItemsHaveMinWidth)
                    // returns True when items use a uniform minimum width
                    // returns False when they use varied minimum widths
                    ...
                else
                    return false;
            }
        }

        private bool ItemsHaveUniformMaxWidths
        {
            get
            {
                if (ItemsHaveMaxWidth)
                    // returns True when items use a uniform maximum width
                    // returns False when they use varied maximum widths
                    ...
                else
                    return false;
            }
        }

        private double UniformItemMinWidth
        {
            get
            {
                Debug.Assert(ItemsHaveUniformMinWidths);
                // returns the uniform minimum width for all items in the collection
                ...
            }
        }

        private double UniformItemMaxWidth
        {
            get
            {
                Debug.Assert(ItemsHaveUniformMaxWidths);
                // returns the uniform maximum width for all items in the collection
                ...
            }
        }

        private bool IsItemAspectRatioKnown(int index)
        {
            // returns True when the aspect ratio for the provided 
            // index is known. The minimum and maximum widths, if
            // applicable, are known as well then.
            ...
        }

        private int GetSmallestItemIndexWithKnownRatio(int minIndex, int maxIndex)
        {
            // returns the smallest index in the provided range with a known aspect ratio.
            // returns -1 if there is none.
            ...
        }

        private int GetLargestItemIndexWithKnownRatio(int minIndex, int maxIndex)
        {
            // returns the largest index in the provided range with a known aspect ratio.
            // returns -1 if there is none.
            ...
        }

        private double GetItemAspectRatio(int index)
        {
            Debug.Assert(IsItemAspectRatioKnown(index));
            // returns the known aspect ratio for the provided index.
            ...
        }

        private double GetItemMinWidth(int index)
        {
            Debug.Assert(ItemsHaveMinWidth && !ItemsHaveUniformMinWidths && IsItemAspectRatioKnown(index));
            // returns the known minimum width for the provided index.
            ...
        }

        private double GetItemMaxWidth(int index)
        {
            Debug.Assert(ItemsHaveMaxWidth && !ItemsHaveUniformMaxWidths && IsItemAspectRatioKnown(index));
            // returns the known maximum width for the provided index.
            ...
        }

        private void LinedFlowLayout_ItemsInfoRequested(LinedFlowLayout sender, LinedFlowLayoutItemsInfoRequestedEventArgs args)
        {
            const int maxReturnedArrayLength = 8192;

            if (ItemsHaveMinWidth && ItemsHaveUniformMinWidths)
                args.MinWidth = UniformItemMinWidth;

            if (ItemsHaveMaxWidth && ItemsHaveUniformMaxWidths)
                args.MaxWidth = UniformItemMaxWidth;                

            int itemsRangeStartIndex = GetSmallestItemIndexWithKnownRatio(
                minIndex: args.ItemsRangeStartIndex + (args.ItemsRangeRequestedLength - maxReturnedArrayLength) / 2,
                maxIndex: args.ItemsRangeStartIndex);

            if (itemsRangeStartIndex == -1)
                itemsRangeStartIndex = args.ItemsRangeStartIndex;
            else
                args.ItemsRangeStartIndex = itemsRangeStartIndex;

            int itemsRangeEndIndex = GetLargestItemIndexWithKnownRatio(
                minIndex: itemsRangeStartIndex + args.ItemsRangeRequestedLength - 1,
                maxIndex: itemsRangeStartIndex + (args.ItemsRangeRequestedLength + maxReturnedArrayLength) / 2 - 1);

            if (itemsRangeEndIndex == -1)
                itemsRangeEndIndex = itemsRangeStartIndex + args.ItemsRangeRequestedLength - 1;

            int returnedArrayLength = itemsRangeEndIndex - itemsRangeStartIndex + 1;

            double[] desiredAspectRatios = new double[returnedArrayLength];
            double[] minWidths = (!ItemsHaveMinWidth || ItemsHaveUniformMinWidths) ? null : new double[returnedArrayLength];
            double[] maxWidths = (!ItemsHaveMaxWidth || ItemsHaveUniformMaxWidths) ? null : new double[returnedArrayLength];

            for (int index = 0; index < returnedArrayLength; index++)
            {
                if (IsItemAspectRatioKnown(itemsRangeStartIndex + index))
                {
                    desiredAspectRatios[index] = GetItemAspectRatio(itemsRangeStartIndex + index);

                    if (minWidths != null)
                        minWidths[index] = GetItemMinWidth(itemsRangeStartIndex + index);
                        
                    if (maxWidths != null)
                        maxWidths[index] = GetItemMaxWidth(itemsRangeStartIndex + index);
                }
                else
                {
                    desiredAspectRatios[index] = 0.0;

                    if (minWidths != null)
                        minWidths[index] = 0.0;
                        
                    if (maxWidths != null)
                        maxWidths[index] = double.MaxValue;
                }
            }

            args.SetDesiredAspectRatios(desiredAspectRatios);

            if (minWidths != null)
                args.SetMinWidths(minWidths);
                    
            if (maxWidths != null)
                args.SetMaxWidths(maxWidths);
        }
    }
}
```


## LinedFlowLayout.RequestedRangeStartIndex property

Gets the smallest index of the items currently using sizing information provided through the `ItemsInfoRequested` event. 
A value of -1 indicates that no sizing information from `ItemsInfoRequested` is being used.

C#
```cs
public int RequestedRangeStartIndex { get; }
```

### Property Value

This property returns -1 when no item is using sizing information provided through the `ItemsInfoRequested` event.
Otherwise it returns a value greater than or equal to 0 and less than the source collection size.

This property is typically used to determine whether the `InvalidateItemsInfo` method must be called or not when
temporary sizing information was provided by the `ItemsInfoRequested` event handler and has become known.

### Example

Assuming the `ItemsInfoRequested` event was raised with the following values, 
and the application provided the requested information, while the user scrolls 
and jumps down the items collection with a `ScrollBar`:

| **LinedFlowLayoutItemsInfoRequestedEventArgs.ItemsRangeStartIndex** | **LinedFlowLayoutItemsInfoRequestedEventArgs.ItemsRangeRequestedLength** |
|-|-|
|   0 | 50 | 
| 195 | 40 | 
| 235 | 21 |
| 256 |  6 |
| 303 | 65 |

The `ItemsView.RequestedRangeStartIndex` and `ItemsView.RequestedRangeLength` property values after each 
`ItemsInfoRequested` event occurrence are:

| **ItemsView.RequestedRangeStartIndex** | **ItemsView.RequestedRangeLength** |
|-|-|
|   0 | 50 | 
| 195 | 40 | 
| 195 | 61 |
| 195 | 67 |
| 303 | 65 |

![Illustration of LinedFlowLayout RequestedRangeStartIndex and RequestedRangeLength](Images/LinedFlowLayout_RequestedRange.png)

After the 5 `ItemsInfoRequested` event occurrences, the `ItemsView.RequestedRangeStartIndex` property returns the value 303,
while the `ItemsView.RequestedRangeLength` property returns the value 65.

See code for [InvalidateItemsInfo](#riverflowlayoutinvalidateitemsinfo-method) example.


## LinedFlowLayout.RequestedRangeLength property

Gets the number of items currently using sizing information provided through the `ItemsInfoRequested` event. 
The default value is 0.

C#
```cs
public int RequestedRangeLength { get; }
```

### Property Value

This property returns 0 when no item is using sizing information provided through the `ItemsInfoRequested` event.
Otherwise it returns a value greater than 0 and less than or equal to the source collection size.

This property is typically used to determine whether the `InvalidateItemsInfo` method must be called or not when
temporary sizing information was provided by the `ItemsInfoRequested` event handler and has become known.

### Example

See code for [InvalidateItemsInfo](#riverflowlayoutinvalidateitemsinfo-method) example.


## LinedFlowLayout.InvalidateItemsInfo method

Causes the `LinedFlowLayout` to do a reflow of its items. As part of it, any potential item sizing information 
previously collected through the `ItemsInfoRequested` event is discarded. That event is then raised to retrieve 
the latest item sizing information given the current scrolling viewport position.

C#
```cs
public void InvalidateItemsInfo();
```

### Remarks

A temporary item aspect ratio can be provided by an `ItemsInfoRequested` event handler because it is unknown at the time
the event is processed. Once that aspect ratio becomes known, the `InvalidateItemsInfo` is meant to be called if the item's
index is within the range established by `ItemsView.RequestedRangeStartIndex` and `ItemsView.RequestedRangeLength`.
The item's index may have fallen out of that relevant range if the scrolling viewport has sufficently moved between the time 
the temporary aspect ratio was provided and the time its real value was established.

### Example

C#
```cs
namespace PhotosApp
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        // ...

        // Called when the aspect ratio of item at index `itemIndex` is discovered.
        private void OnItemAspectRatioEstablished(int itemIndex)
        {
            if (itemIndex >= myLinedFlowLayout.RequestedRangeStartIndex &&
                itemIndex < myLinedFlowLayout.RequestedRangeStartIndex + myLinedFlowLayout.RequestedRangeLength)
            {
                // Index `itemIndex` is still relevant and InvalidateItemsInfo must be called.
                myLinedFlowLayout.InvalidateItemsInfo();
            }
        }
    }
}
```


## LinedFlowLayout.LockItemToLine method

Returns the index of the line the item with the provided index belongs to. 
That item is then guaranteed to appear in the returned line until the
[`ItemsUnlocked`](#riverflowlayoutitemsunlocked-event) 
event is raised.

C#
```cs
public int LockItemToLine(int itemIndex);
```

### Example

C#
```cs
namespace PhotosApp
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        // ...

        double GetAnnotatedScrollBarLabelValueFromItemIndex(int itemIndex)
        {
            LinedFlowLayout myLinedFlowLayout = photosView.Layout as LinedFlowLayout;
            int lineIndex = myLinedFlowLayout.LockItemToLine(itemIndex);
            return (myLinedFlowLayout.LineSpacing + myLinedFlowLayout.ActualLineHeight) * lineIndex;
        }
    }
}
```

_Spec note:_
_This method is used in the AnnotatedScrollBar scenario to make sure that when the user drags the blue glyph right next to a 
predefined Label, the ItemsView content ends up at the exact offset associated with that Label. Otherwise the scroll offset 
may be off by a few lines._

![Illustration of LinedFlowLayout LockItemToLine usage](Images/LinedFlowLayout_LockItemToLine.png)

_Application code can call this method to determine Label values to be used by an AnnotatedScrollBar. This guarantees that when 
a user scrolls to an AnnotatedScrollBar label, the user lands exactly with the item represented by that label at the top of the 
viewport._

_For example, if the item represents the first photo of the year 2019 (from the end of December 2019), the ItemsView will scroll
the line with that item to the top of the viewport as the AnnotatedScrollBar glyph reaches the 2019 Label.
Let's say the LinedFlowLayout predicts that item 1056, which is that first photo of 2019, will belong to line 155 based on its 
internal average items-per-line value. That's an approximation. When scrolling down line by line to line 155, item 1056 may
actually not belong to that line if the layout algorithm had no concept of locked item. Item 1056 may appear on line 154 or 156
for example._


## LinedFlowLayout.ItemsUnlocked event

The `LinedFlowLayout` raises this event whenever items that had been locked into specific lines
through `LockItemToLine` method calls are unlocked again.

C#
```cs
public event TypedEventHandler<LinedFlowLayout, object> ItemsUnlocked;
```

### Remarks

Locked items become unlocked again, triggering this `ItemsUnlocked` event when:
- the available width for the `LinedFlowLayout` changed.
- the `ItemsSource` data source has changed. For example an item is added, removed or resized.
- the `ActualLineHeight` property changed. 

_Spec note:_
_Application code can listen to this event to update an AnnotatedScrollBar's label values._


### Example

C#
```cs
namespace PhotosApp
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();

            (this.PhotosView.Layout as LinedFlowLayout).ItemsUnlocked += OnItemsUnlocked;            
        }

        // ...

        private void OnItemsUnlocked(LinedFlowLayout linedFlowLayout, object e)
        {
            PopulateAnnotatedScrollBarLabels();
        }
    }
}
```


## LinedFlowLayoutItemsInfoRequestedEventArgs class

The `ItemsInfoRequested` event is raised with the `LinedFlowLayoutItemsInfoRequestedEventArgs` argument which provides details about 
the requested item sizing information.

Namespace: Microsoft.UI.Xaml.Controls

C#
```cs
public class LinedFlowLayoutItemsInfoRequestedEventArgs
```

### Example

See example for [ItemsInfoRequested](#riverflowlayoutitemsinforequested-event) event.


## LinedFlowLayoutItemsInfoRequestedEventArgs.ItemsRangeStartIndex property

Gets or sets the start of the item range with sizing information. 

C#
```cs
public int ItemsRangeStartIndex { get; set; }
```

### Property Value

When the `ItemsInfoRequested` event is raised, this property returns the index of the first item for which sizing 
information is requested. The requested range starts at index `ItemsRangeStartIndex` and ends at index 
`ItemsRangeStartIndex` + `ItemsRangeRequestedLength` - 1.

If sizing information is available for a larger range than the requested one, this property can be updated with 
a smaller value indicating the actual start of the provided range. The new value must be greater than or equal to 0.

### Example

See example for [ItemsInfoRequested](#riverflowlayoutitemsinforequested-event) event.


## LinedFlowLayoutItemsInfoRequestedEventArgs.ItemsRangeRequestedLength property

Gets the number of items for which sizing information is requested.  

C#
```cs
public int ItemsRangeRequestedLength { get; }
```

### Property Value

Returns a value greater than 0 indicating the range length of the requested item sizing information.
When the `ItemsInfoRequested` event is raised, the requested range starts at index `ItemsRangeStartIndex` 
and ends at index `ItemsRangeStartIndex` + `ItemsRangeRequestedLength` - 1.

### Example

See example for [ItemsInfoRequested](#riverflowlayoutitemsinforequested-event) event.


## LinedFlowLayoutItemsInfoRequestedEventArgs.MinWidth property

Gets or sets a minimum width applicable to all items in the source collection. The default value is 0. 

C#
```cs
public double MinWidth{ get; set; }
```

### Property Value

This property is meant to be used when all items use the same minimum width. The smaller the value
the more flexibility is given to the algorithm which lays out the items in lines.

When items do not use a uniform minimum width, the `LinedFlowLayoutItemsInfoRequestedEventArgs.SetMinWidths` 
is meant to be called instead. When they do use a uniform minimum width, setting the `LinedFlowLayoutItemsInfoRequestedEventArgs.MinWidth` 
property is more efficient than calling the method.

If both the `MinWidth` property and the `SetMinWidths` method are used, the effective minimum width
applied for each item is the largest of the two values provided. 

_Example:_

| **Item index** | **MinWidth value** | **SetMinWidths value** | **Effective min width** |
|-|-|-|-|
| 0 | 200 | 150 | 200
| 1 | 200 | 200 | 200
| 2 | 200 | 250 | 250
| 3 | 200 | 100 | 200

_Spec note:_
_Once the LinedFlowLayout supports an Orientation property, the MinWidth will represent the minimum item height in a horizontal layout._

### Example

See example for [ItemsInfoRequested](#riverflowlayoutitemsinforequested-event) event.


## LinedFlowLayoutItemsInfoRequestedEventArgs.MaxWidth property

Gets or sets a maximum width applicable to all items in the source collection. The default value is `PositiveInfinity`. 

C#
```cs
public double MaxWidth{ get; set; }
```

### Property Value

This property is meant to be used when all items use the same maximum width. The larger the value
the more flexibility is given to the algorithm which lays out the items in lines.

When items do not use a uniform maximum width, the `LinedFlowLayoutItemsInfoRequestedEventArgs.SetMaxWidths` 
is meant to be called instead. When they do use a uniform maximum width, setting the `LinedFlowLayoutItemsInfoRequestedEventArgs.MaxWidth` 
property is more efficient than calling the method.

If both the `MaxWidth` property and the `SetMaxWidths` method are used, the effective maximum width
applied for each item is the smallest of the two values provided. 

_Example:_

| **Item index** | **MaxWidth value** | **SetMaxWidths value** | **Effective max width** |
|-|-|-|-|
| 0 | 800 | 850 | 800
| 1 | 800 | 800 | 800
| 2 | 800 | 750 | 750
| 3 | 800 | 900 | 800

In order to give optimal flexibility to the algorithm which lays out the items, it is preferable to 
not use the `MaxWidth` property or the `SetMaxWidths` method.

_Spec note:_
_Once the LinedFlowLayout supports an Orientation property, the MaxWidth will represent the maximum 
item height in a horizontal layout._

### Example

See example for [ItemsInfoRequested](#riverflowlayoutitemsinforequested-event) event.


## LinedFlowLayoutItemsInfoRequestedEventArgs.SetDesiredAspectRatios method

This method is called to provide desired aspect ratios for an item range at least as large as the requested one. 

C#
```cs
public void SetDesiredAspectRatios(double[] values);
```

### Remarks

Several options are possible when handling the `ItemsInfoRequested` event:
- the event handler does not provide any aspect ratios,
- the event handler provides aspect ratios for exactly the requested range, defined by the `ItemsRangeStartIndex` 
and `ItemsRangeRequestedLength` properties,
- the event handler provides aspect ratios for more items than the requested range (i.e. a superset of the requested range).

Providing aspect ratios for a subset of the requested range results in an error.
The first call to any of the three methods `SetDesiredAspectRatios`, `SetMinWidths` and `SetMaxWidths` establishes 
the range length that must be returned by those methods. For that particular occurrence of the `ItemsInfoRequested` 
event, any subsequent use of those three methods with a different range length results in an error.

When the aspect ratio for an item is still unknown, a temporary placeholder value can be provided in the `values` 
array instead of the real aspect ratio. A placeholder value could be 1.0 and assume that the item is square. 
It could be 2.0 if the items are known to have that aspect ratio on average. Or it could be 0.0 to let the `LinedFlowLayout` 
compute an average aspect ratio based on the known ratios and use it as the placeholder. 
In any case, once the real aspect ratio is determined, the `LinedFlowLayout`'s [`InvalidateItemsInfo`](#riverflowlayoutinvalidateitemsinfo-method) 
method may need to be called.

### Example

See example for [ItemsInfoRequested](#riverflowlayoutitemsinforequested-event) event.


## LinedFlowLayoutItemsInfoRequestedEventArgs.SetMinWidths method

This method is called to provide minimum widths for an item range at least as large as the requested one.

C#
```cs
public void SetMinWidths(double[] values);
```

### Remarks

When items do not use a uniform minimum width and therefore `LinedFlowLayoutItemsInfoRequestedEventArgs`'s `MinWidth` 
property is not applicable, the `SetMinWidths` method can be used instead. It allows to specify a different minimum 
width for each item.

Providing minimum widths is only useful when aspect ratios are also provided with `SetDesiredAspectRatios`. Otherwise 
the provided information is unused.

As with the `MinWidth` property, the smaller the values provided the more flexibility is given to the algorithm which 
lays out the items in lines.

Providing minimum widths for a subset of the requested range results in an error.
The first call to any of the three methods `SetDesiredAspectRatios`, `SetMinWidths` and `SetMaxWidths` establishes 
the range length that must be returned by those methods. For that particular occurrence of the `ItemsInfoRequested` 
event, any subsequent use of those three methods with a different range length results in an error.

_Spec note:_
_Once the LinedFlowLayout supports an Orientation property, the `values` array will represent the minimum item heights 
in a horizontal layout._

### Example

See example for [ItemsInfoRequested](#riverflowlayoutitemsinforequested-event) event.


## LinedFlowLayoutItemsInfoRequestedEventArgs.SetMaxWidths method

This method is called to provide maximum widths for an item range at least as large as the requested one.

C#
```cs
public void SetMaxWidths(double[] values);
```

### Remarks

When items do not use a uniform maximum width and therefore `LinedFlowLayoutItemsInfoRequestedEventArgs`'s `MaxWidth` 
property is not applicable, the `SetMaxWidths` method can be used instead. It allows to specify a different maximum 
width for each item.

Providing maximum widths is only useful when aspect ratios are also provided with `SetDesiredAspectRatios`. Otherwise 
the provided information is unused.

As with the `MaxWidth` property, the larger the values provided the more flexibility is given to the algorithm which 
lays out the items in lines.
For optimal flexibility, it is preferable to not use the `MaxWidth` property or the `SetMaxWidths` method.

Providing maximum widths for a subset of the requested range results in an error.
The first call to any of the three methods `SetDesiredAspectRatios`, `SetMinWidths` and `SetMaxWidths` establishes 
the range length that must be returned by those methods. For that particular occurrence of the `ItemsInfoRequested` 
event, any subsequent use of those three methods with a different range length results in an error.

_Spec note:_
_Once the LinedFlowLayout supports an Orientation property, the `values` array will represent the maximum item heights 
in a horizontal layout._

### Example

See example for [ItemsInfoRequested](#riverflowlayoutitemsinforequested-event) event.


## LinedFlowLayoutItemsJustification enum

Defines constants that specify how items are aligned on the horizontal axis.

_Spec note: this is a copy of
[UniformGridLayoutItemsJustification](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.UniformGridLayoutItemsJustification)_

Namespace: Microsoft.UI.Xaml.Controls

| **Name** | **Value** | **Description** |
|-|-|-|
| Start | 0 | Items are aligned with the start of the row or column, with extra space at the end. Spacing between items does not change.
| Center | 1 | Items are aligned in the center of the row or column, with extra space at the start and end. Spacing between items does not change.
| End | 2 | Items are aligned with the end of the row or column, with extra space at the start. Spacing between items does not change.
| SpaceAround | 3 | Items are aligned so that extra space is added evenly before and after each item.
| SpaceBetween | 4 | Items are aligned so that extra space is added evenly between adjacent items. No space is added at the start or end.
| SpaceEvenly | 5 | Items are aligned so that extra space is equal before and after each item.

_Spec note:_
_Equivalent to UniformGridLayoutItemsJustification enum: https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.controls.uniformgridlayoutitemsjustification?view=winui-2.7_

![Illustration of LinedFlowLayout's ItemsJustification property](Images/LinedFlowLayout_ItemsJustification.png)

## LinedFlowLayoutItemsStretch enum

Defines constants that specify how items are sized to fill the available space in the horizontal axis.

Namespace: Microsoft.UI.Xaml.Controls

Irrespective of the `LinedFlowLayoutItemsStretch` enum value, the items in a line are sized according 
to the `ActualLineHeight` property, minimum and maximum item widths, and the available line width. 
The `LinedFlowLayout` layout algorithm always attempts to keep the total items width within the available 
line width.

The difference between the `None` and `Fill` enum values lays in how the potential extra horizontal space 
is handled. In the `None` case, that extra space is preserved and distributed around the items in the 
line according to the `ItemsJustification` property. In the `Fill` case, the items in the line are sized 
wider in order to eliminate that extra space.

| **Name** | **Value** | **Description** |
|-|-|-|
| None | 0 | The potential extra horizontal space on a line is distributed around its items according to the `ItemsJustification` property. |
| Fill | 1 | The items on a line are sized wider to eliminate any potential extra horizontal space. |

_Spec note:_
_Similar to UniformGridLayoutItemsStretch enum: https://docs.microsoft.com/en-us/windows/winui/api/microsoft.ui.xaml.controls.uniformgridlayoutitemsstretch?view=winui-2.7_



# API Details

```cs (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls
{
    enum LinedFlowLayoutItemsStretch
    {
        None = 0,
        Fill = 1
    }

    enum LinedFlowLayoutItemsJustification
    {
        Start = 0,	
        Center = 1,	
        End = 2,	
        SpaceAround = 3,	
        SpaceBetween = 4,	
        SpaceEvenly = 5	
    }

    runtimeclass LinedFlowLayout : VirtualizingLayout
    {
        LinedFlowLayout();

        event Windows.Foundation.TypedEventHandler<LinedFlowLayout, LinedFlowLayoutItemsInfoRequestedEventArgs> ItemsInfoRequested;
        event Windows.Foundation.TypedEventHandler<LinedFlowLayout, Object> ItemsUnlocked;

        // Default value: LinedFlowLayoutItemsJustification.Start
        LinedFlowLayoutItemsJustification ItemsJustification { get; set; };

        // Default value: LinedFlowLayoutItemsStretch.None
        LinedFlowLayoutItemsStretch ItemsStretch { get; set; };

        Double MinItemSpacing { get; set; };
        Double LineSpacing { get; set; };

        Double LineHeight { get; set; };
        Double ActualLineHeight { get; };

        Int32 RequestedRangeStartIndex{ get; };        
        Int32 RequestedRangeLength{ get; };

        void InvalidateItemsInfo();
        Int32 LockItemToLine(Int32 itemIndex);

        static Microsoft.UI.Xaml.DependencyProperty ItemsJustificationProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty ItemsStretchProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty MinItemSpacingProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty LineSpacingProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty LineHeightProperty { get; };
        static Microsoft.UI.Xaml.DependencyProperty ActualLineHeightProperty { get; };
    }

    runtimeclass LinedFlowLayoutItemsInfoRequestedEventArgs
    {
        Int32 ItemsRangeStartIndex{ get; set; };
        Int32 ItemsRangeRequestedLength{ get; };
        Double MinWidth{ get; set; };
        Double MaxWidth{ get; set; };
        void SetDesiredAspectRatios(Double[] values);
        void SetMinWidths(Double[] values);
        void SetMaxWidths(Double[] values);
    }
}
```


# Appendix

## Future APIs

### LinedFlowLayout.LineCount property

Gets the total number of lines in the layout.

C#
```cs
public int LineCount { get; }
```

#### Property Value

Represents the total number of lines used to display all items in the source collection. Default is zero.


_The C# LinedFlowLayout prototype implementation is:_
```cs
public int LineCount
{
    get
    {
        if (m_itemCount > 0)
        {
            double averageItemsPerLine = RefreshAverageItemsPerLine(m_lastAvailableWidth);
            int lineCount = m_itemCount / averageItemsPerLine;

            if (m_itemCount / averageItemsPerLine - lineCount > 0.2)
            {
                lineCount++;
            }

            return lineCount;
        }

        return 0;
    }
}
```


### LinedFlowLayout.Orientation property

Gets or sets the axis along which items are laid out.

C#
```cs
public Orientation Orientation { get; set; }
```

_Spec note:_  
_If we indeed add an Orientation property in the future, then we can document that LineHeight
actually represents a column width.
An alternative would be to create a VerticalLinedFlowLayout with a LineWidth property._

### LinedFlowLayout.AverageItemAspectRatio property

Gets or sets the average aspect ratio to use to estimate how many items to lay out on the lines.

C#
```cs
public double AverageItemAspectRatio { get; set; }
```

#### Property Value

Default is `NaN`. When this property has its default `NaN` value, the `LinedFlowLayout` computes an average aspect ratio based on its realized items.
The average aspect ratio, combined with the `LinedFlowLayout.ActualLineHeight` property, determines the average number of items displayed per line.

#### Remarks

This property affects the value returned by `LinedFlowLayout.LineCount`.
The `LinedFlowLayout` consumer can override the default `NaN` value when it has better knowledge of the average aspect ratio than the LinedFlowLayout
based on its small buffer of realized items.
