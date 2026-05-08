Layout API spec
===

# Background

One way to implement Xaml layout is with a subclass of
[Layout](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.Layout),
for example
[StackLayout](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.StackLayout).

Currently `Layout` is only used by
[ItemsRepeater](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Controls.ItemsRepeater)
and the upcoming `ItemsView`. For example:

```xml
<ItemsRepeater ItemsSource="{x:Bind People}" ItemTemplate="{StaticResource PersonTemplate}">
    <ItemsRepeater.Layout>
        <StackLayout Spacing="20" />
    </ItemsRepeater.Layout>
</ItemsRepeater>
```

This spec adds a `Layout.IndexBasedLayoutOrientation` property that you can call to
determine if the item at index `N+1` is to the right or below the item at index `N` (or neither).
This enables a control such as `ItemsView` to correctly implement keyboard navigation with
the arrow keys.
Custom layouts set this property by calling the new (protected) `SetIndexBasedLayoutOrientation` method.

This spec also adds a `VisibleRect` properties for virtualizing layouts to indicate what's visible,
thereby enabling controls to determine what items to load.


# API Pages

## IndexBasedLayoutOrientation enum

Defines constants that specify whether there is a correlation between the items indices and how they are laid out.

Namespace: Microsoft.UI.Xaml.Controls

| **Name** | **Value** | **Description** |
|-|-|-|
| None | 0 | There is no correlation between the items' layout and their index number. |
| TopToBottom | 1 | The items are laid out vertically by increasing index number. |
| LeftToRight | 2 | The items are laid out horizontally by increasing index number. |


## Layout.IndexBasedLayoutOrientation property

Gets the orientation, if any, in which items are laid out based on their index in the source collection.
The default value is `IndexBasedLayoutOrientation.None`.

_Spec note:_  
_That's because both `VirtualizingLayout` and `NonVirtualizingLayout` specify `IndexBasedLayoutOrientation.None`._

C#
```cs
public IndexBasedLayoutOrientation IndexBasedLayoutOrientation { get; }
```

### Remark

The `IndexBasedLayoutOrientation` property has no effect on the `Layout` per se. Instead, it can have an effect on controls 
that consume the `Layout`, like the `ItemsView` control. 
The `ItemsView` uses this property in its internal implementation of its `TryGetItemIndex` method and its built-in handling 
of keyboard-based navigation. 

For example, the `LinedFlowLayout` layout returns `LeftToRight`. As a result, the right arrow key's handling is navigating to the 
next index (moving from index I to index I+1). Likewise, the left arrow key's handling is navigating to the previous index 
(moving from index I to index I-1). Left arrow and right arrow keys are moving to an item based on its index. On the other hand, 
the up arrow and down arrow keys are moving to an item based on its physical position. The behaviors are reversed when 
`Layout.IndexBasedLayoutOrientation` returns `TopToBottom`. When `Layout.IndexBasedLayoutOrientation` returns `None`, all four arrows 
move to an item based on its physical location, rather than index.

### Example

| **`IndexBasedLayoutOrientation` value** | **Description** | **Illustration** |
|-|-|-|
| None | There is no correlation between the items' layout and their index number. | ![Illustration of IndexBasedLayoutOrientation's None value](Images/Layout_NoneIndexBasedLayoutOrientation.png) |
| TopToBottom | Items are laid out vertically with increasing indices. | ![Illustration of IndexBasedLayoutOrientation's TopToBottom value](Images/Layout_TopToBottomIndexBasedLayoutOrientation.png) |
| LeftToRight | Items are laid out horizontally with increasing indices. | ![Illustration of IndexBasedLayoutOrientation's LeftToRight value](Images/Layout_LeftToRightIndexBasedLayoutOrientation.png) |


## Layout.SetIndexBasedLayoutOrientation protected method

Call this method to set the value of the `IndexBasedLayoutOrientation` property.

### Example

C#
```cs
public class MyHorizontalLayout : NonVirtualizingLayout
{
    public class MyHorizontalLayout()
    {
        SetIndexBasedLayoutOrientation(IndexBasedLayoutOrientation.LeftToRight);
        Debug.Assert(this.IndexBasedLayoutOrientation == IndexBasedLayoutOrientation.LeftToRight);
    }
}
```


## VirtualizingLayoutContext.VisibleRect property

> Background note: a `VirtualizingLayout` is a `Layout` that doesn't require all elements to be created.
So a layout being used to display 10,000 images doesn't have to create 10,000 item containers.

Gets the visible viewport rectangle within the `FrameworkElement` associated with the `Layout`.

C#
```cs
public Windows.Foundation.Rect VisibleRect { get; }
```

_Spec note:_
_This visible viewport comes from the 
[FrameworkElement.EffectiveViewportChanged](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.frameworkelement.effectiveviewportchanged?view=winrt-22621) event_
_raised for the `FrameworkElement` owning the `Layout`: EffectiveViewportChangedEventArgs.EffectiveViewport_
_The "protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)" 
implementation of a `Layout` in particular can then consume that Rect._

_Similar existing VirtualizingLayoutContext property following that naming:_
_Windows.Foundation.Rect RealizationRect { get; }_

_The LinedFlowLayout layout makes use of that effective/visible viewport to freeze the layout of the lines_
_that are in the user's sight. The alternative is to have the LinedFlowLayout get a hold of the owning_
_ItemsRepeater and listen to its EffectiveViewportChanged event which is an undesirable dependency._


## VirtualizingLayoutContext.VisibleRectCore protected virtual method

Override this method in your subclass of `VirtualizingLayoutContext` to provide the value
that will be returned from the `VisibleRect` property.

C#
```cs
protected virtual Windows.Foundation.Rect VisibleRectCore();
```

_Spec note:_
_Similar existing VirtualizingLayoutContext method following that naming:_
_overridable Windows.Foundation.Rect RealizationRectCore();_


# API Details

```cs (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls
{
    [contract(Microsoft.UI.Xaml.XamlContract, 5)]
    enum IndexBasedLayoutOrientation
    {
        None = 0,
        TopToBottom = 1,
        LeftToRight = 2,
    }

    unsealed runtimeclass Layout : Microsoft.UI.Xaml.DependencyObject
    {
        // The following two APIs are just additions to the existing Layout class.

        [contract(Microsoft.UI.Xaml.XamlContract, 5)]
        {
            IndexBasedLayoutOrientation IndexBasedLayoutOrientation { get; };
            protected void SetIndexBasedLayoutOrientation(IndexBasedLayoutOrientation orientation);
        }
    }

    unsealed runtimeclass VirtualizingLayoutContext : LayoutContext
    {
        // The following two APIs are just additions to the existing VirtualizingLayoutContext class.

        [contract(Microsoft.UI.Xaml.XamlContract, 5)]
        {
            Windows.Foundation.Rect VisibleRect { get; };

            overridable Windows.Foundation.Rect VisibleRectCore();
        }
    }
}
```
