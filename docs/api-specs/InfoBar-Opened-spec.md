InfoBar.Opened event
===

> **STATUS: DRAFT - EXPERIMENTAL - NOT APPROVED**
>
> This spec covers an experimental API. It has not completed Windows API review.
> The API surface and behavior may change before it is made stable.

Public review period: September 4, 2026 through October 4, 2026.

# Background

The `InfoBar` control provides `Closing` and `Closed` events so an app can react
when an InfoBar is dismissed. It does not currently provide a stable event that
indicates when the InfoBar has opened.

Apps may need to react after an InfoBar becomes visible to account for layout
changes, update related UI or state, record telemetry, or start other work.
Observing `IsOpen` does not clearly express that the control has updated its
visible state.

This spec adds an `Opened` event. The API is already available experimentally and
is being specified for review before it can be included in stable Windows App SDK
releases.

The original request is tracked by
[microsoft-ui-xaml issue #3761](https://github.com/microsoft/microsoft-ui-xaml/issues/3761).

# Examples

The following example updates related UI after an InfoBar becomes visible:

```xml
<InfoBar
    x:Name="ConnectionInfoBar"
    Title="Connection restored"
    Message="Your changes are now being synchronized."
    IsOpen="False"
    Opened="ConnectionInfoBar_Opened" />
```

```csharp
private void ConnectionInfoBar_Opened(
    InfoBar sender,
    InfoBarOpenedEventArgs args)
{
    StatusText.Text = "Synchronization resumed";
}
```

# API Pages

## InfoBar.Opened event

Occurs after the InfoBar has updated to its open visual state.

```csharp
public event TypedEventHandler<InfoBar, InfoBarOpenedEventArgs> Opened;
```

The event is raised after setting `IsOpen` to `true` causes the control to update
its visibility. It is raised once for each transition from closed to open.

If `IsOpen` is set to `true` before the control template has been applied, the
`Opened` event is deferred until the template is applied and the open visual
state has been established.

The event does not indicate completion of an animation. The default InfoBar
visual-state transition is immediate, and handlers should depend on the updated
open state rather than animation timing.

Use this event when an action must occur after the InfoBar becomes visible. Use
the `IsOpen` property when an action only depends on the requested open state.

InfoBars communicate important app-level status. Apps should not use `Opened` to
automatically dismiss an InfoBar before users, including users of assistive
technologies, have enough time to perceive and act on its content.

## InfoBarOpenedEventArgs class

Provides data for the `InfoBar.Opened` event.

```csharp
public class InfoBarOpenedEventArgs
```

`InfoBarOpenedEventArgs` does not currently expose additional data.

_Spec note: A dedicated event-arguments type is proposed instead of `Object` or
`RoutedEventArgs`. This is consistent with `InfoBarClosedEventArgs`,
`TeachingTipOpenedEventArgs`, and `ContentDialogOpenedEventArgs`, and permits
properties to be added in the future without changing the event delegate type._

_Spec note: The experimental MIDL declaration uses `default_interface` because
the unsealed event-arguments class has no members from which MIDL can infer a
default interface._

_Spec note: The current experimental implementation raises `Opened` immediately
when `IsOpen` is set before template application. It must defer the event until
the template is applied to meet the behavior specified here._

# API Details

```c# (but really MIDL3)
namespace Microsoft.UI.Xaml.Controls
{
    [default_interface]
    unsealed runtimeclass InfoBarOpenedEventArgs
    {
    };

    unsealed runtimeclass InfoBar : Microsoft.UI.Xaml.Controls.Control
    {
        event Windows.Foundation.TypedEventHandler<
            InfoBar,
            InfoBarOpenedEventArgs> Opened;
    };
}
```

# Appendix

## Event timing

`Opened` represents the InfoBar updating to its open visual state after an
`IsOpen` change. It does not wait for an animation to complete.

The following cases should be covered by automated tests:

- Changing `IsOpen` from `false` to `true` updates the visible state before
  raising `Opened`.
- One `Opened` event is raised for each transition from closed to open.
- Setting `IsOpen` to `true` when the InfoBar is already open does not raise an
  additional `Opened` event.
- Closing and reopening the InfoBar raises one new `Opened` event.
- Setting `IsOpen` before the template is applied raises `Opened` once and does
  not raise another event when the template is applied.

## Alternatives considered

### Observe changes to `IsOpen`

An app can observe changes to the `IsOpen` dependency property, but that reports
the requested state and does not provide a direct lifecycle event after the
control updates its visible state. A named event is also consistent with the
existing `Closing` and `Closed` lifecycle events.

### Use `Object` as the event data

Several existing WinUI lifecycle events use `Object` when they do not provide
event data. This would avoid introducing a new type, but would make it difficult
to add data later without changing the event's public signature.

### Use `RoutedEventArgs`

Some existing controls use `RoutedEventHandler` for opened events.
`InfoBar.Opened` is a direct event on the control rather than a routed event, so
using `RoutedEventArgs` would imply routing behavior that this API does not
provide.

### Add an `Opening` event in the same change

An `Opening` event could support actions before the InfoBar changes its visual
state. Cancellation or deferral would introduce additional behavioral and API
design questions. It is therefore outside the scope of this proposal and can be
considered separately.
