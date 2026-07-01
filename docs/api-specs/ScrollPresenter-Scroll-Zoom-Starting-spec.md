ScrollPresenter.ScrollStarting / ScrollView.ScrollStarting events and ScrollPresenter.ZoomStarting / ScrollView.ZoomStarting
===

# Background

These are events for the `ScrollPresenter` & `ScrollView` elements that indicate that their view is about to change.
More precisely, they are raised when a non-animated view change request is about to be fulfilled.

Note that a `ScrollPresenter` & `ScrollView` view is defined by their three `HorizontalOffset`, `VerticalOffset` and 
`ZoomFactor` read-only properties.

The `ScrollStarting` event is raised when a non-animated scroll is starting, while the `ZoomStarting` event is raised
when a non-animated zoom is starting. Their event arguments specify the anticipated view.

These events are particularly useful when the `ScrollPresenter` & `ScrollView` `Content` includes an `ItemsRepeater`. 
When it hosts virtualized items, it can generate UI elements for the anticipated view and thus avoid momentarily 
showing blank content.

_Spec note:_  
_Why are these additions considered? To address a blank content issue when scrolling virtualized lists.
This issue shows up for example when scrolling with the ScrollBar a very long list of virtualized items hosted by an ItemsRepeater.
For large lists, each ScrollBar Thumb movement represents a large jump in the ScrollPresenter.Content. The ScrollPresenter 
receives a request to jump to a new offset from the ScrollBar, via the IScrollController interface. The ScrollPresenter fulfills
that request by handing it off to the underlying InteractionTracker. It turn, the InteractionTracker fulfills the request asynchronously
by updating the Content's Visual position on the compositor thread and then raising its IInteractionTrackerOwner.ValuesChanged event
on the UI thread. On the XAML side, the UI thread reacts by updating the ScrollPresenter HorizontalOffset, VerticalOffset and
ZoomFactor properties (which are not dependency properties), raising the FrameworkElement.EffectiveViewChanged event, processing
that event in ItemsRepeater by generating UI for the new offset. But by the time that UI thread processing occurs, the independent
Visual will already have moved to a region where no UI items were generated. This causes the ScrollPresenter to temporarily
show blank content. Note that the effective viewport feature uses UIElement.TransformToVisual which incorporates the transform driven 
by the InteractionTracker._
_The same bug can be observed when simply calling scrollPresenter.ScrollTo(...) to jump to a far away offset. This results in a single
blank frame too._
_The new ScrollPresenter.ScrollStarting event, raised when handing off the request to the InteractionTracker, allows the ItemsRepeater
to generate UI items at the destination offsets ahead of the compositor's updates._

_Spec note:_  
_Alternatives considered include:_
* _To avoid the tight coupling between the `ItemsRepeater` and `ScrollPresenter` types, we could introduce a new interface,
a sibling of the existing `IScrollAnchorProvider`, which would be implemented only by the `ScrollPresenter`:
The interface could be called Microsoft.UI.Xaml.Controls.IScrollPreviewProvider and expose the ScrollStarting/ZoomStarting 
events with their anticipated view. The `ItemsRepeater` would detect that implementation just like it already uses the 
`IScrollAnchorProvider` interface. Again, this would avoid the dependency on the `ScrollPresenter` type, but requires the 
introduction of a new interface.
Being only implemented and consumed by MuxC components, that interface could live in the MuxC dll.
However, the `ItemsRepeater` already has a tight dependency on the `ScrollViewer` control and needs event more dependency
on `ScrollPresenter` than these two new events - all those dependencies are implemented in the ViewportManager
class. So an IScrollPreviewProvider interface would not preserve
a strict decoupling of `ItemsRepeater` and the scrollers._
* _The `ItemsRepeater` virtualized items generation is based on the `FrameworkElement`'s `EffectiveViewportChanged` event. 
With an additional EffectiveViewportChanging event, the `ItemsRepeater` could listen to that new event and do 
whatever it would do in its `ScrollPresenter`.ScrollStarting event.
Advantages: No tight coupling betweent the `ScrollPresenter` and `ItemsRepeater`.
            No risk of generating too many UI items for viewports that are partially obfuscated.
This would require an expensive work item in Mux though._
* _A single ViewChanging event for `ScrollPresenter` and `ScrollView` raised whenever `ScrollStarting` and `ZoomStarting` are
raised was originally considered. But given the existing `ViewChanged` event, it was too misleading because the expectation
was that ViewChanging would be raised before each `ViewChanged` which is not the case.
There would be no coupling of the ViewChanging and ViewChanged events. ViewChanged is raised whenever the ScrollPresenter's 
HorizontalOffset, VerticalOffset or ZoomFactor changes. This is drastically more than the ScrollStarting/ZoomStarting occurrences._


# Example - generating virtualized UI items in time for an offset shift

C#
```cs
public class MyPanel : Panel
{
  private ScrollPresenter _parentScrollPresenter;

  private void OnLoaded(object sender, RoutedEventArgs args)
  {
    _parentScrollPresenter = FindParentByType<ScrollPresenter>();

    if (_parentScrollPresenter != null)
    {
      _parentScrollPresenter.ScrollStarting += ParentScrollPresenter_ScrollStarting;
    }
  }

  private void ParentScrollPresenter_ScrollStarting(ScrollPresenter sender, ScrollingScrollStartingEventArgs args)
  {
    GenerateItemsForView(args.HorizontalOffset, args.VerticalOffset, args.ZoomFactor);
  }
}
```


# API Pages

## ScrollPresenter.ScrollStarting event

The `ScrollStarting` event is raised when the `ScrollPresenter`'s `HorizontalOffset`, `VerticalOffset` or 
`ZoomFactor` properties, which constitute the view, are about to be affected by a non-animated scroll request.

Those requests in question originate from:
- a `ScrollPresenter` `ScrollTo` or `ScrollBy` method call that results in a non-animated view change,
- a `ScrollView` `ScrollTo` or `ScrollBy` method call that results in a non-animated view change.
  The `ScrollView` forwards those calls to the `ScrollPresenter` in its control template,
- the `ScrollPresenter`'s `HorizontalScrollController` or `VerticalScrollController` `IScrollController` implementation 
  raising its `ScrollByRequested` or `ScrollToRequested` event that results in a non-animated offset change. A concrete
  example is the user dragging the `Thumb` of a `ScrollBar` resulting in an `IScrollController` `ScrollToRequested`
  notification.

_Spec note:_  
_The event is raised when the request is handed off to the underlying InteractionTracker._

Note that `ScrollTo` and `ScrollBy` calls can result in animated or non-animated view changes depending on the 
`ScrollingAnimationMode` argument being used and the Windows' "Animation effects" setting in the Settings application.

The `ScrollStarting` event provides data through its `ScrollingScrollStartingEventArgs` argument. It exposes `HorizontalOffset`, 
`VerticalOffset` and `ZoomFactor` read-only properties representing the anticipated view, or an approximation of it. 

The anticipated view is an approximation and not an exact one when the `ScrollPresenter` is in the middle of animating its 
view and the view change request is for a view relative to the current one. 
Such an ongoing animation can be the result for example of user input (touch panning or mouse-wheel scrolling),
calls to `ScrollTo`, `ScrollBy`, `ZoomTo` or `ZoomBy` with a `ScrollingAnimationMode.Enabled` argument,
calls to `AddScrollVelocity`, `AddZoomVelocity`, or `HorizontalScrollController`/`VerticalScrollController`
`IScrollController` implementation raising its `ScrollToRequested`, `ScrollByRequested` or `AddScrollVelocityRequested`
event.

_Spec note:_  
_Indeed, when the ScrollPresenter is performing an animation, the compositor thread is ahead of the UI thread. As a result,
the UI thread can not anticipate the exact effect of a relative view change.
For example, if the current (HorizontalOffset, VerticalOffset, ZoomFactor) view is (100.0, 200.0, 2.0f) while being animated,
a ScrollBy(horizontalOffsetDelta: 10.0, verticalOffsetDelta: 50.0, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled))
call will not result in a view (HorizontalOffset, VerticalOffset, ZoomFactor) equal to (110.0, 250.0, 2.0f). There will be a
ScrollPresenter.ViewChanged event for a view in that neighborhood though._

_Spec note:_  
_Existing ScrollPresenter events:_  
    _event Windows.Foundation.TypedEventHandler<ScrollPresenter, Object> ExtentChanged;_  
    _event Windows.Foundation.TypedEventHandler<ScrollPresenter, Object> StateChanged;_  
    _event Windows.Foundation.TypedEventHandler<ScrollPresenter, Object> ViewChanged;_  
    _event Windows.Foundation.TypedEventHandler<ScrollPresenter, ScrollingScrollAnimationStartingEventArgs> ScrollAnimationStarting;_  
    _event Windows.Foundation.TypedEventHandler<ScrollPresenter, ScrollingZoomAnimationStartingEventArgs> ZoomAnimationStarting;_  
    _event Windows.Foundation.TypedEventHandler<ScrollPresenter, ScrollingScrollCompletedEventArgs> ScrollCompleted;_  
    _event Windows.Foundation.TypedEventHandler<ScrollPresenter, ScrollingZoomCompletedEventArgs> ZoomCompleted;_  
    _event Windows.Foundation.TypedEventHandler<ScrollPresenter, ScrollingBringingIntoViewEventArgs> BringingIntoView;_  
    _event Windows.Foundation.TypedEventHandler<ScrollPresenter, ScrollingAnchorRequestedEventArgs> AnchorRequested;_  

_A ScrollStarting or ZoomStarting notification to (x, y, z) is not necessarily followed by a ViewChanged notification to (x, y, z).
Indeed, if the ScrollPresenter is in the middle of an animation, the ViewChanged notification for an approximation of (x, y, z) is 
likely the second one after the ScrollStarting/ZoomStarting occurrence._


## ScrollView.ScrollStarting event

The `ScrollView.ScrollStarting` event is raised when its inner `ScrollPresenter` raises its own `ScrollStarting` event. 
The same `ScrollingScrollStartingEventArgs` argument instance is used for both events.

_Spec note:_  
_This pattern applies to all events raised by the ScrollView. It simply surfaces the same events raised by the 
ScrollPresenter present in its control template._


## ScrollingScrollStartingEventArgs class

Provides data for the `ScrollPresenter.ScrollStarting` and `ScrollView.ScrollStarting` events.

Namespace: Microsoft.UI.Xaml.Controls

C#
```cs
public class ScrollingScrollStartingEventArgs
```

### Remarks

Used by the `ScrollStarting` event which is raised when a `ScrollPresenter` is starting an asynchronous
and non-animated view change. That same event is replicated in the potential owning `ScrollView` control.

_Spec note:_  
_The name ScrollingScrollStartingEventArgs follows the existing pattern for other ScrollPresenter/ScrollView
event args: ScrollingScrollAnimationStartingEventArgs, ScrollingZoomAnimationStartingEventArgs, 
ScrollingScrollCompletedEventArgs, ScrollingZoomCompletedEventArgs, ScrollingBringingIntoViewEventArgs, 
ScrollingAnchorRequestedEventArgs._


## ScrollingScrollStartingEventArgs.CorrelationId

Gets the correlation ID associated with the view change.

C#
```cs
public int CorrelationId { get; }
```

### Remark

When the `ScrollStarting` event is triggered by a `ScrollTo` or `ScrollBy` call, the `CorrelationId` returned by 
`ScrollingScrollStartingEventArgs` is the same as the one returned by that method call.
However when the event is the result of an `IScrollController` request, `ScrollingScrollStartingEventArgs.CorrelationId` is the
same as `ScrollControllerScrollToRequestedEventArgs.CorrelationId` or `ScrollControllerScrollByRequestedEventArgs.CorrelationId`.

## ScrollingScrollStartingEventArgs.HorizontalOffset

Gets the exact or approximated anticipated value for the `ScrollPresenter.HorizontalOffset` property,
when raised by a `ScrollPresenter`. 

Likewise, when the event is raised by a `ScrollView`, it represents its anticipated or approximated 
`ScrollView.HorizontalOffset` property.

C#
```cs
public double HorizontalOffset { get; }
```

## ScrollingScrollStartingEventArgs.VerticalOffset

Gets the exact or approximated anticipated value for the `ScrollPresenter.VerticalOffset` property,
when raised by a `ScrollPresenter`. 

Likewise, when the event is raised by a `ScrollView`, it represents its anticipated or approximated 
`ScrollView.VerticalOffset` property.

C#
```cs
public double VerticalOffset { get; }
```

## ScrollingScrollStartingEventArgs.ZoomFactor

Gets the exact or approximated anticipated value for the `ScrollPresenter.ZoomFactor` property,
when raised by a `ScrollPresenter`. 

Likewise, when the event is raised by a `ScrollView`, it represents its anticipated or approximated 
`ScrollView.ZoomFactor` property.

C#
```cs
public float ZoomFactor { get; }
```


## ScrollPresenter.ZoomStarting event

The `ZoomStarting` event is raised when the `ScrollPresenter`'s `HorizontalOffset`, `VerticalOffset` or 
`ZoomFactor` properties, which constitute the view, are about to be affected by a non-animated zoom request.

Those requests in question originate from:
- a `ScrollPresenter` `ZoomTo` or `ZoomBy` method call that results in a non-animated view change,
- a `ScrollView` `ZoomTo` or `ZoomBy` method call that results in a non-animated view change.
  The `ScrollView` forwards those calls to the `ScrollPresenter` in its control template.

_Spec note:_  
_The event is raised when the request is handed off to the underlying InteractionTracker._

Note that `ZoomTo` and `ZoomBy` calls can result in animated or non-animated view changes depending on the 
`ScrollingAnimationMode` argument being used and the Windows' "Animation effects" setting in the Settings application.

The `ZoomStarting` event provides data through its `ScrollingZoomStartingEventArgs` argument. It exposes `HorizontalOffset`, 
`VerticalOffset` and `ZoomFactor` read-only properties representing the anticipated view, or an approximation of it. 

The anticipated view is an approximation and not an exact one when the `ScrollPresenter` is in the middle of animating its 
view and the view change request is for a view relative to the current one. 
Such an ongoing animation can be the result for example of user input (touch panning or mouse-wheel scrolling),
calls to `ScrollTo`, `ScrollBy`, `ZoomTo` or `ZoomBy` with a `ScrollingAnimationMode.Enabled` argument,
calls to `AddScrollVelocity`, `AddZoomVelocity`, or `HorizontalScrollController`/`VerticalScrollController`
`IScrollController` implementation raising its `ScrollToRequested`, `ScrollByRequested` or `AddScrollVelocityRequested`
event.


## ScrollView.ZoomStarting event

The `ScrollView.ZoomStarting` event is raised when its inner `ScrollPresenter` raises its own `ZoomStarting` event. 
The same `ScrollingZoomStartingEventArgs` argument instance is used for both events.


## ScrollingZoomStartingEventArgs class

Provides data for the `ScrollPresenter.ZoomStarting` and `ScrollView.ZoomStarting` events.

Namespace: Microsoft.UI.Xaml.Controls

C#
```cs
public class ScrollingZoomStartingEventArgs
```

### Remarks

Used by the `ZoomStarting` event which is raised when a `ScrollPresenter` is starting an asynchronous
and non-animated view change. That same event is replicated in the potential owning `ScrollView` control.


## ScrollingZoomStartingEventArgs.CorrelationId

Gets the correlation ID associated with the view change.

C#
```cs
public int CorrelationId { get; }
```

### Remark

The `CorrelationId` returned by `ScrollingZoomStartingEventArgs` is the same as the one returned by 
the `ZoomTo` or `ZoomBy` call which triggered the event.


## ScrollingZoomStartingEventArgs.HorizontalOffset

Gets the exact or approximated anticipated value for the `ScrollPresenter.HorizontalOffset` property,
when raised by a `ScrollPresenter`. 

Likewise, when the event is raised by a `ScrollView`, it represents its anticipated or approximated 
`ScrollView.HorizontalOffset` property.

C#
```cs
public double HorizontalOffset { get; }
```

## ScrollingZoomStartingEventArgs.VerticalOffset

Gets the exact or approximated anticipated value for the `ScrollPresenter.VerticalOffset` property,
when raised by a `ScrollPresenter`. 

Likewise, when the event is raised by a `ScrollView`, it represents its anticipated or approximated 
`ScrollView.VerticalOffset` property.

C#
```cs
public double VerticalOffset { get; }
```

## ScrollingZoomStartingEventArgs.ZoomFactor

Gets the exact or approximated anticipated value for the `ScrollPresenter.ZoomFactor` property,
when raised by a `ScrollPresenter`. 

Likewise, when the event is raised by a `ScrollView`, it represents its anticipated or approximated 
`ScrollView.ZoomFactor` property.

C#
```cs
public float ZoomFactor { get; }
```


# API Details

## `ScrollingScrollStartingEventArgs` class

```cs (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls
{
    runtimeclass ScrollingScrollStartingEventArgs
    {
        Int32 CorrelationId { get; };
        Double HorizontalOffset { get; };
        Double VerticalOffset { get; };
        Single ZoomFactor { get; };
    }
}
```

## `ScrollingZoomStartingEventArgs` class

```cs (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls
{
    runtimeclass ScrollingZoomStartingEventArgs
    {
        Int32 CorrelationId { get; };
        Double HorizontalOffset { get; };
        Double VerticalOffset { get; };
        Single ZoomFactor { get; };
    }
}
```

## `ScrollView` class

```cs (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls
{
    unsealed runtimeclass ScrollView : Control
    {
        event Windows.Foundation.TypedEventHandler<ScrollView, Microsoft.UI.Xaml.Controls.ScrollingScrollStartingEventArgs> ScrollStarting;
        event Windows.Foundation.TypedEventHandler<ScrollView, Microsoft.UI.Xaml.Controls.ScrollingZoomStartingEventArgs> ZoomStarting;
    }
}
```

## `ScrollPresenter` class

```cs (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls.Primitives
{
    unsealed runtimeclass ScrollPresenter
        : Microsoft.UI.Xaml.FrameworkElement,
        Microsoft.UI.Xaml.Controls.IScrollAnchorProvider
    {
        event Windows.Foundation.TypedEventHandler<ScrollPresenter, Microsoft.UI.Xaml.Controls.ScrollingScrollStartingEventArgs> ScrollStarting;
        event Windows.Foundation.TypedEventHandler<ScrollPresenter, Microsoft.UI.Xaml.Controls.ScrollingZoomStartingEventArgs> ZoomStarting;
    }
}
```


# Appendix

## Related Documents

| Document                            | URL                                                                 |
|-------------------------------------|---------------------------------------------------------------------|
| Functional Specification            |                                                                     |
| `ScrollPresenter` API Specification | [ScrollPresenter API spec](..\design-notes\ScrollPresenter-spec.md) |
| `ScrollView` API Specification      | [ScrollView API spec](..\design-notes\ScrollView-spec.md)           |
