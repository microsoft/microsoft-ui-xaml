TeachingTip.Opened event
===

> **STATUS: DRAFT - EXPERIMENTAL - NOT APPROVED**
>
> This spec covers an experimental API. It has not completed Windows API review.
> The API surface and behavior may change before it is made stable.

Public review period: September 4, 2026 through October 4, 2026.

# Background

The `TeachingTip` control provides `Closing` and `Closed` events so an app can react
when a teaching tip is dismissed. It does not currently provide a stable event that
indicates when the teaching tip has finished opening.

Apps may need to know when a teaching tip is fully open to move focus, start an
animation, record telemetry, or coordinate other UI. Listening for a property change
to `IsOpen` does not provide the same information because `IsOpen` changes before the
opening transition completes.

This spec adds an `Opened` event. The API is already available experimentally and is
being specified for review before it can be included in stable Windows App SDK
releases.

The original request is tracked by
[microsoft-ui-xaml issue #1607](https://github.com/microsoft/microsoft-ui-xaml/issues/1607).

# Examples

The following example moves keyboard focus into a teaching tip after it is fully open:

```xml
<TeachingTip
    x:Name="SaveTeachingTip"
    Title="Save your changes"
    Subtitle="Select Save before closing this page."
    Opened="SaveTeachingTip_Opened">
    <Button x:Name="SaveButton" Content="Save" />
</TeachingTip>
```

```csharp
private void SaveTeachingTip_Opened(
    TeachingTip sender,
    TeachingTipOpenedEventArgs args)
{
    SaveButton.Focus(FocusState.Programmatic);
}
```

# API Pages

## TeachingTip.Opened event

Occurs after the teaching tip has opened and its opening transition has completed.

```csharp
public event TypedEventHandler<TeachingTip, TeachingTipOpenedEventArgs> Opened;
```

The event is raised when the teaching tip reaches its fully open state. If opening
animations are disabled or no opening animation is required, the event is raised
after the teaching tip is opened without waiting for an animation.

The event is not raised if a closing transition begins before the opening transition
completes. Setting `IsOpen` to `true` begins opening the teaching tip, but does not
guarantee that `Opened` will be raised if the operation is superseded by a close.

Use this event when an action must occur after the teaching tip is ready for
interaction. Use the `IsOpen` property when an action only depends on the requested
open state.

## TeachingTipOpenedEventArgs class

Provides data for the `TeachingTip.Opened` event.

```csharp
public sealed class TeachingTipOpenedEventArgs
```

`TeachingTipOpenedEventArgs` does not currently expose additional data.

_Spec note: A dedicated event-arguments type is proposed instead of `Object` or
`RoutedEventArgs`. This is consistent with `TeachingTipClosedEventArgs` and
`ContentDialogOpenedEventArgs`, and permits properties to be added in the future
without changing the event delegate type._

# API Details

```c# (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls
{
    runtimeclass TeachingTipOpenedEventArgs
    {
    };

    unsealed runtimeclass TeachingTip : Microsoft.UI.Xaml.Controls.ContentControl
    {
        event Windows.Foundation.TypedEventHandler<
            TeachingTip,
            TeachingTipOpenedEventArgs> Opened;
    };
}
```

# Appendix

## Event timing

`Opened` represents completion of the opening operation rather than the change to the
requested `IsOpen` state. This distinction is important when transitions are enabled.
The event should be raised once for each opening operation that reaches the fully open
state.

The following cases should be covered by automated tests:

- Opening with transitions enabled raises `Opened` after the opening transition.
- Opening with transitions disabled raises `Opened` after the control is open.
- Closing before the opening transition completes does not raise `Opened`.
- Reopening a teaching tip raises one new `Opened` event.
- Changing `IsOpen` to `true` when the teaching tip is already open does not raise an
  additional `Opened` event.

## Alternatives considered

### Use `Object` as the event data

Several existing WinUI lifecycle events use `Object` when they do not provide event
data. This would avoid introducing a new type, but would make it difficult to add data
later without changing the event's public signature.

### Use `RoutedEventArgs`

Some existing controls use `RoutedEventHandler` for opened events. `TeachingTip.Opened`
is a direct event on the control rather than a routed event, so using
`RoutedEventArgs` would imply routing behavior that this API does not provide.

### Add an `Opening` event in the same change

An `Opening` event could support actions before the teaching tip begins its opening
transition. Cancellation or deferral would introduce additional behavioral and API
design questions. It is therefore outside the scope of this proposal and can be
considered separately.
