Tab Tear-Out API spec
===

# Background

Applications such as Edge have an existing design paradigm where users can click and drag on a tab to tear the tab out into its own
window, or to connect the tab to another set of tabs in an existing window, in a smooth fashion that allows the user to continue to
drag the detached window without any delay or interruptions.  This is a highly intuitive way to interact with tabs and
windows that users have come to expect and desire, and multiple different Microsoft applications such as File Explorer and Terminal
would like to have the same pattern as well.  We are generalizing this pattern in XAML in a way that enables this behavior and handles
a maximal amount of the tab tear-out logic.

There currently exist tab tear-out implementations in place in File Explorer and WinUI Gallery, but those are using the WinRT
drag-and-drop APIs, which require a drop on pointer released before any operations can be performed.  This is a significantly
worse experience in comparison - in Edge, for example, a user can tear out a tab and then drag it to the top to maximize it
in one smooth motion, whereas with the drag-and-drop APIs, a user must first drag and drop to even create the new window
in the first place, and then must perform a completely separate pointer action to interact with the new window in any way.

# Conceptual pages (how to)

## Microsoft.UI.Input.InputNonClientPointerSource

Smooth tab tear-out is implemented using the move/size loop, which requires the application to designate part of its window as a
non-client caption region, forward `WM_NCLBUTTONDOWN` messages to the top-level window, and then handle the set of
`WM_ENTERSIZEMOVE`, `WM_WINDOWPOSCHANGING`, `WM_WINDOWPOSCHANGED`, and `WM_EXITSIZEMOVE` messages that represent the progress
of the move/size loop. `InputNonClientPointerSource` already provides a way for applications to designate a non-client caption region,
which we are augmenting by adding additional events for these four move/size loop window messages.  This allows any application
to add custom handling to the move/size loop by adding event handlers to these new `InputNonClientPointerSource` APIs.

These APIs do not apply only to XAML.  They can be used in an application that is designing its own tab control, as well, or in
scenarios unrelated to tab tear-out entirely - for example, an app can delay regenerating resources that depend on the window size
until a resize is complete.

Below is an example of how a non-XAML application can use these APIs to implement a custom tab control.

C#
```cs
public class ApplicationWindow
{
    public AppWindow AppWindow { get; }
    public static List<ApplicationWindow> Windows { get; } = new();

    public void ApplicationWindow()
    {
        this.AppWindow = CreateAppWindow();

        // We'll keep each window in a list to keep them alive until the user closes them.
        Windows.Add(this);

        this.AppWindow.Destroying += (AppWindow sender, object args) =>
        {
            Windows.Remove(this);
        };

        InputNonClientPointerSource nonClientPointerSource = InputNonClientPointerSource.GetForWindowId(this.AppWindow.Id);

        nonClientPointerSource.SetRegionRects(NonClientRegionKind.Caption, GetTabRegion());

        // EnteringMoveSize doesn't correspond to a move-size window message - it's raised on WM_NCLBUTTONDOWN when we're about to enter the move-size loop.
        nonClientPointerSource.EnteringMoveSize += OnEnteringMoveSize;
        nonClientPointerSource.EnteredMoveSize += OnEnteredMoveSize;
        nonClientPointerSource.WindowRectChanging += OnWindowRectChanging;
        nonClientPointerSource.ExitedMoveSize += OnExitedMoveSize;

        // Note that InputNonClientPointerSource also has a WindowRectChanged event that corresponds to WM_WINDOWPOSCHANGED,
        // but we don't need to handle it for the purposes of smooth tab tear-out.
    }

    private void OnEnteringMoveSize(InputNonClientPointerSource sender, EnteringMoveSizeEventArgs args)
    {
        if (this.SelectedTabs.Count == this.Tabs.Count)
        {
            // If all tabs are selected, then we aren't tearing out any tab, so we'll just allow the window to be dragged normally
            // instead of creating a new window to host torn-out tabs.
            return;
        }

        // Create a new initially hidden window to host the torn-out tab.
        ApplicationWindow tornOutWindow = new();
        tornOutWindow.AppWindow.IsVisible = false;
        args.MoveSizeWindowId(tornOutWindow.AppWindow.Id);
    }

    private void OnEnteredMoveSize(InputNonClientPointerSource sender, EnteredMoveSizeEventArgs args)
    {
        // Initialize the tab tear-out state machine.  This is necessary even if we didn't create a new window in OnEnteringMoveSize,
        // because we still need to handle the case where the user drags the tabs on top of another tab control to merge them into
        // that other tab control.
    }

    private void OnWindowRectChanging(InputNonClientPointerSource sender, WindowRectChangingEventArgs args)
    {
        //
        // Update the tab tear-out state machine:
        //
        //    1. If the user has dragged the selected tabs out of the tab control's bounds, show the new window created to host them
        //       and move the selected tabs to the new window's tab control.
        //    2. If the user is dragging on a tab control in a window where all the tabs are selected, allow the window to be dragged
        //       as normal.
        //    3. If the user has dragged a tab control window on top of another window's tab control, hide the window being dragged
        //       and move the tabs being dragged to the other window's tab control.
        //
    }

    private void OnExitedMoveSize(InputNonClientPointerSource sender, ExitedMoveSizeEventArgs args)
    {
        // Finalize the tab tear-out state machine.  Destroy any hidden windows.
    }

    private AppWindow CreateAppWindow()
    {
        // Create an AppWindow and set up its content.
    }
}
```

## Microsoft.UI.Xaml.Controls.TabView

The `TabView` control in XAML makes use of the new `InputNonClientPointerSource` APIs to implement smooth tab tear-out, and exposes
APIs to application developers to provide hooks for the application to shuttle `TabViewItems` or data items between `TabView` controls
when tabs are being torn out from or dropped onto a `TabView` control.

XAML
```xml
<!-- Note that the TabView does not necessarily need to be in the title bar; the non-client area can be anywhere. -->
<MainWindow ExtendsContentIntoTitleBar="True">
  <Grid>
    <TabView x:Name="TabView"
        TabItemsSource="{x:Bind StringList}"
        CanTearOutTabs="True"
        TabTearOutWindowRequested="OnTabTearOutWindowRequested"
        TabTearOutRequested="OnTabTearOutRequested"
        ExternalTornOutTabsDropping="OnExternalTornOutTabsDropping"
        ExternalTornOutTabsDropped="OnExternalTornOutTabsDropped">
        <!-- TabViewItems -->
    </TabView>
    <!-- ... -->
  </Grid>
</MainWindow>
```

C#
```cs
Dictionary<WindowId, MainWindow> windowIdToWindowDictionary = new();

// TabTearOutWindowRequested and TabTearOutRequested will only be raised if we're tearing out a tab. If a user clicks and drags on a TabView control
// where all tabs are selected,  then we'll drag the window instead of tearing out any tabs. However, ExternalTornOutTabsDropping and ExternalTornOutTabsDropped will still
// be raised in that case if the user drags those tabs on top of another TabView control.
//
// Note that the following events will be raised only when CanTearOutTabs is true.
void OnTabTearOutWindowRequested(TabView sender, TabViewTabTearOutWindowRequestedEventArgs args)
{
    // We'll create a new window that will later host torn-out tabs. The window will only be shown if tabs are actually torn out.
    MainWindow newWindow = new();
    args.MoveSizeWindowId = newWindow.AppWindow.Id;

    windowIdToWindowDictionary.Add(newWindow.AppWindow.Id, newWindow);

    newWindow.AppWindow.Destroying += (AppWindow sender, object args) =>
    {
        windowIdToWindowDictionary.Remove(newWindow.AppWindow.Id);
    };
}

void OnTabTearOutRequested(TabView sender, TabViewTabTearOutRequestedEventArgs args)
{
    // When tabs have been torn out of this TabView, we'll remove their data items from the TabView's TabItemsSource.
    foreach (object item in args.Items)
    {
        int index = this.StringList.IndexOf(item);
        if (index >= 0)
        {
            this.StringList.RemoveAt(index);
        }
    }

    // We'll now have the new window contain the items being torn out.
    if (windowIdToWindowDictionary.TryGetValue(args.MoveSizeWindowId, out MainWindow newWindow))
    {
        newWindow.StringList.AddRange(args.Items);
    }

    // The TabView will show the window we created in OnTabTearOutWindowRequested after this event handler returns.
}

void OnExternalTornOutTabsDropping(TabView sender, TabViewExternalTornOutTabsDroppingEventArgs args)
{
    // This sample app only has the one TabView, so we know we'll always allow tabs to be dropped
    // because tab drag/drop is permitted only between TabViews of the same process.
    // However, an app with multiple TabViews can check the contents of the new tabs in order
    // to determine whether the drop should be accepted.
    args.AllowDrop = true;
}

void OnExternalTornOutTabsDropped(TabView sender, TabViewExternalTornOutTabsDroppedEventArgs args)
{
    // When tabs being dragged out of another TabView have been dropped onto this TabView,
    // we'll add their data items to our TabView's TabItemsSource.
    for (uint i = 0; i < args.Items.Size; i++)
    {
        this.StringList.Insert(args.DropIndex + i, args.Items[i]);
    }

    // The TabView will hide the window hosting those tabs, and will destroy the window if the user ends the tab tear-out operation while
    // the window is still hidden.
}
```

## Microsoft.UI.Xaml.XamlRoot

Interactions between `TabView` controls in multiple different windows requires the ability to convert from XAML coordinates to screen or
app window coordinates.  The `CoordinateConverter` property on XamlRoot gives XAML controls the ability to know how to do that.

C#
```cs
WindowId appWindowId = XamlRoot.ContentIslandEnvironment.AppWindowId;
ContentCoordinateConverter appWindowContentCoordinateConverter = ContentCoordinateConverter.CreateForWindowId(appWindowId);

auto hostBounds = control.TransformToVisual(nullptr).TransformBounds({ 0, 0, control.ActualWidth(), control.ActualHeight() });
auto screenBounds = XamlRoot.CoordinateConverter.ConvertLocalToScreen(hostBounds);
auto appWindowBounds = appWindowContentCoordinateConverter.ConvertScreenToLocal(screenBounds);
```

# API pages

## MoveSizeOperation enum

Specifies what sort of window move-size operation is being performed.

| **Name** | **Value** | **Description** |
|-|-|-|
| Move | 0 | The window is being moved.
| SizeBottom | 1 | The window is being sized with the bottom border.
| SizeBottomLeft | 2 | The window is being sized with the bottom-left corner.
| SizeBottomRight | 3 | The window is being sized with the bottom-right corner.
| SizeLeft | 4 | The window is being sized with the left border.
| SizeRight | 5 | The window is being sized with the right border.
| SizeTop | 6 | The window is being sized with the top border.
| SizeTopLeft | 7 | The window is being sized with the top-left corner.
| SizeTopRight | 8 | The window is being sized with the top-right corner.

C#
```cs
enum MoveSizeOperation
{
    Move,
    SizeBottom,
    SizeBottomLeft,
    SizeBottomRight,
    SizeLeft,
    SizeRight,
    SizeTop,
    SizeTopLeft,
    SizeTopRight,
};
```

## EnteringMoveSizeEventArgs class

Represents the args for the [`InputNonClientPointerSource.EnteringMoveSize` event](#inputnonclientpointersourceEnteringMoveSize-event).

C#
```cs
public sealed class EnteringMoveSizeEventArgs
```

## EnteringMoveSizeEventArgs.PointerScreenPoint property

Gets the pointer position prior to entering the move-size loop.

C#
```cs
public PointInt32 PointerScreenPoint{ get; };
```

## EnteringMoveSizeEventArgs.MoveSizeOperation property

Gets the type of operation being performed in this move-size loop.

C#
```cs
public MoveSizeOperation MoveSizeOperation{ get; }
```

## EnteringMoveSizeEventArgs.MoveSizeWindowId property

Gets or sets the ID of the window for which to enter the move-size loop.  The application can create a new window and set this value to its ID
if it wants to supply an alternate window for the move-size loop to target.  Defaults to the window ID of the window that received the pointer
message that initiated the move-size loop.

C#
```cs
public Microsoft.UI.WindowId MoveSizeWindowId { get; set; }
```

*NOTE:* The new window, if created, must be on the same thread as the window that received the pointer message that initiated the move-size loop.

## InputNonClientPointerSource.EnteringMoveSize event

Raised by `InputNonClientPointerSource` when the window is about to enter its move-size loop to allow the app to provide an alternate window instead.

C#
```cs
public event TypedEventHandler<InputNonClientPointerSource, EnteringMoveSizeEventArgs> EnteringMoveSize;
```

**Note:** This corresponds to the `WM_NCLBUTTONDOWN` windows message prior to entering the move-size loop.

## EnteredMoveSizeEventArgs class

Represents the args for the [`InputNonClientPointerSource.EnteredMoveSize` event](#inputnonclientpointersourceenteredmovesize-event).

C#
```cs
public sealed class EnteredMoveSizeEventArgs
```

## EnteredMoveSizeEventArgs.PointerScreenPoint property

Gets the pointer position upon entering the move-size loop.

C#
```cs
public PointInt32 PointerScreenPoint{ get; };
```

## EnteredMoveSizeEventArgs.MoveSizeOperation property

Gets the type of operation being performed in this move-size loop.

C#
```cs
public MoveSizeOperation MoveSizeOperation{ get; }
```

## InputNonClientPointerSource.EnteredMoveSize event

Raised by `InputNonClientPointerSource` when the window has entered its move-size loop.

C#
```cs
public event TypedEventHandler<InputNonClientPointerSource, EnteredMoveSizeEventArgs> EnteredMoveSize;
```

**Note:** This corresponds to the `WM_ENTERSIZEMOVE` windows message.

## WindowRectChangingEventArgs class

Represents the args for the [`InputNonClientPointerSource.WindowRectChanging` event](#inputnonclientpointersourcewindowrectchanging-event).

C#
```cs
public sealed class WindowRectChangingEventArgs
```

## WindowRectChangingEventArgs.PointerScreenPoint property

Gets the pointer position when the window position is about to change.

C#
```cs
public PointInt32 PointerScreenPoint{ get; };
```

## WindowRectChangingEventArgs.MoveSizeOperation property

Gets the type of operation being performed in this move-size loop.

C#
```cs
public MoveSizeOperation MoveSizeOperation{ get; }
```

## WindowRectChangingEventArgs.OldWindowRect property

Gets the old rect that the window will be changing from to if nothing prevents it from changing.

C#
```cs
public RectInt32 OldWindowRect { get; }
```

## WindowRectChangingEventArgs.NewWindowRect property

Gets or sets the new rect that the window will change to if nothing prevents it from changing.
Can be set to supply an alternate rect instead.

C#
```cs
public RectInt32 NewWindowRect { get; set; }
```

## WindowRectChangingEventArgs.AllowRectChange property

Gets or sets a value indicating whether or not the window rect should be allowed to change.
Defaults to `true`; can be set to `false` to prevent the window rect change from happening.

C#
```cs
public bool AllowRectChange { get; set; }
```

## WindowRectChangingEventArgs.ShowWindow property

Gets or sets a value indicating whether or not the window should be shown.
Defaults to the current state of the window; can be changed from `false` to `true`
to show the window, or from `true` to `false` to hide the window.

C#
```cs
public bool ShowWindow { get; set; }
```

## InputNonClientPointerSource.WindowRectChanging event

Raised by `InputNonClientPointerSource` when the window rect is about to change.

C#
```cs
public event TypedEventHandler<InputNonClientPointerSource, WindowRectChangingEventArgs> WindowRectChanging;
```

**Note:** This corresponds to the `WM_WINDOWPOSCHANGING` windows message.

## WindowRectChangedEventArgs class

Represents the args for the [`InputNonClientPointerSource.WindowRectChanged` event](#inputnonclientpointersourcewindowrectchanged-event).

C#
```cs
public sealed class WindowRectChangedEventArgs
```

## WindowRectChangedEventArgs.PointerScreenPoint property

Gets the pointer position when the window position has finished changing.

C#
```cs
public PointInt32 PointerScreenPoint{ get; }
```

## WindowRectChangedEventArgs.MoveSizeOperation property

Gets the type of operation being performed in this move-size loop.

C#
```cs
public MoveSizeOperation MoveSizeOperation{ get; }
```

## WindowRectChangedEventArgs.OldWindowRect property

Gets the old rect that the window changed from.

C#
```cs
public RectInt32 OldWindowRect { get; }
```

## WindowRectChangedEventArgs.NewWindowRect property

Gets the new position that the window changed to.

C#
```cs
public RectInt32 NewWindowRect { get; }
```

## InputNonClientPointerSource.WindowRectChanged event

Raised by `InputNonClientPointerSource` when the window position has finished changing.

C#
```cs
public event TypedEventHandler<InputNonClientPointerSource, WindowRectChangedEventArgs> WindowRectChanged;
```

**Note:** This corresponds to the `WM_WINDOWPOSCHANGED` windows message.

## ExitedMoveSizeEventArgs class

Represents the args for the [`InputNonClientPointerSource.ExitedMoveSize` event](#inputnonclientpointersourceexitedmovesize-event).

C#
```cs
public sealed class ExitedMoveSizeEventArgs
```

## ExitedMoveSizeEventArgs.PointerScreenPoint property

Gets the pointer position upon exiting the move-size loop.

C#
```cs
public PointInt32 PointerScreenPoint{ get; }
```

## ExitedMoveSizeEventArgs.MoveSizeOperation property

Gets the type of operation being performed in this move-size loop.

C#
```cs
public MoveSizeOperation MoveSizeOperation{ get; }
```

## InputNonClientPointerSource.ExitedMoveSize event

Raised by `InputNonClientPointerSource` when the window has exited its move-size loop.

C#
```cs
public event TypedEventHandler<InputNonClientPointerSource, ExitedMoveSizeEventArgs> ExitedMoveSize;
```

**Note:** This corresponds to the `WM_EXITSIZEMOVE` windows message.

## TabView.CanTearOutTabs property

Gets or sets a value indicating whether or not tearing out tabs is supported on this `TabView`.
If set to `true`, causes `TabTearOutWindowRequested` and `TabTearOutRequested` to be raised instead of
`TabDragStarting`, and causes `ExternalTornOutTabsDropping` and `ExternalTornOutTabsDropped` to be raised
instead of `TabStripDragOver`, `TabStripDrop`, `TabDragCompleted`, and `TabDroppedOutside`.
Defaults to `false`.

C#
```cs
public bool CanTearOutTabs { get; set; }
```

## TabViewTabTearOutWindowRequestedEventArgs class

Represents the args for the [`TabView.TabTearOutWindowRequested` event](#tabviewtabtearoutwindowrequested-event).

C#
```cs
public sealed class TabViewTabTearOutWindowRequestedEventArgs
```

## TabViewTabTearOutWindowRequestedEventArgs.Items property

Gets the data items held by the selected tabs on which the user is dragging.

C#
```cs
public IReadOnlyList<object> Items { get; }
```

## TabViewTabTearOutWindowRequestedEventArgs.Tabs property

Gets the selected tabs on which the user is dragging.

C#
```cs
public IReadOnlyList<UIElement> Tabs { get; }
```

## TabViewTabTearOutWindowRequestedEventArgs.NewWindowId property

Gets or sets the `WindowId` for the new window, if the application created one, that will house torn-out tabs.
If not set, the tab tear-out will be cancelled.  This window represented by the window ID can be any HWND,
not necessarily a XAML window. It must be on the same thread as the TabView.

C#
```cs
public Microsoft.UI.WindowId NewWindowId { get; set; }
```

## TabView.TabTearOutWindowRequested event

Raised for `TabView`s where the user has started to drag on its selected tabs when `CanTearOutTabs` is set to `true`.
Gives the application the option to create a new window in which to host the torn-out tabs.

C#
```cs
public event TypedEventHandler<TabView, TabViewTabTearOutWindowRequestedEventArgs> TabTearOutWindowRequested;
```

## TabViewTabTearOutRequestedEventArgs class

Represents the args for the [`TabView.TabTearOutRequested` event](#tabviewTabTearOutRequested-event).

C#
```cs
public sealed class TabViewTabTearOutRequestedEventArgs
```

## TabViewTabTearOutRequestedEventArgs.Items property

Gets the data items held by the tabs to be torn out.

C#
```cs
public IReadOnlyList<object> Items { get; }
```

## TabViewTabTearOutRequestedEventArgs.Tabs property

Gets the tabs to be torn out.

C#
```cs
public IReadOnlyList<UIElement> Tabs { get; }
```

## TabViewTabTearOutRequestedEventArgs.NewWindowId property

Gets the `WindowId` for the window that the application created that should house the torn-out tabs.

C#
```cs
public object Microsoft.UI.WindowId NewWindowId { get; }
```

## TabView.TabTearOutRequested event

Raised for `TabView`s where their selected tabs need to be torn out. The event handler should transfer
the torn-out tab data items to the window specified by `NewWindowId`. This is usually done by adding them
to the `TabItemsSource` of a `TabView` control in that new window.

C#
```cs
public event TypedEventHandler<TabView, TabViewTabTearOutRequestedEventArgs> TabTearOutRequested;
```

## TabViewExternalTornOutTabsDroppingEventArgs class

Represents the args for the [`TabView.ExternalTornOutTabsDropping` event](#tabviewExternalTornOutTabsDropping-event).

C#
```cs
public sealed class TabViewExternalTornOutTabsDroppingEventArgs
```

## TabViewExternalTornOutTabsDroppingEventArgs.Items property

Gets the data items held by the tabs that to be dropped.

C#
```cs
public IReadOnlyList<object> Items { get; }
```

## TabViewExternalTornOutTabsDroppingEventArgs.Tabs property

Gets the tabs to be dropped.

C#
```cs
public IReadOnlyList<UIElement> Tabs { get; }
```

## TabViewExternalTornOutTabsDroppingEventArgs.DropIndex property

Gets the index at which the system proposes to insert the dropped tabs into the `TabView`.

C#
```cs
public int DropIndex { get; }
```

## TabViewExternalTornOutTabsDroppingEventArgs.AllowDrop property

Gets or sets a value indicating whether or not to accept the tabs being dropped.
Defaults to `false`; can be set to `true` to allow the drop.  If set to `false`, the
`ExternalTornOutTabsDropped` event will not be raised and the user will continue to
be dragging the torn-out tabs.

C#
```cs
public bool AllowDrop { get; set; }
```

## TabView.ExternalTornOutTabsDropping event

Raised for `TabView`s where external tabs are being dropped onto this `TabView`.
Allows applications to specify whether they want to allow the drop to occur.
External tabs are accepted only from other `TabViews` in the same process.

C#
```cs
public event TypedEventHandler<TabView, TabViewExternalTornOutTabsDroppingEventArgs> ExternalTornOutTabsDropping;
```

## TabViewExternalTornOutTabsDroppedEventArgs class

Represents the args for the [`TabView.ExternalTornOutTabsDropped` event](#tabviewExternalTornOutTabsDropped-event).

C#
```cs
public sealed class TabViewExternalTornOutTabsDroppedEventArgs
```

## TabViewExternalTornOutTabsDroppedEventArgs.Items property

Gets the data items held by the tabs that have been dropped.

C#
```cs
public IReadOnlyList<object> Items { get; }
```

## TabViewExternalTornOutTabsDroppedEventArgs.Tabs property

Gets the tabs that have been dropped.

C#
```cs
public IReadOnlyList<UIElement> Tabs { get; }
```

## TabViewExternalTornOutTabsDroppedEventArgs.DropIndex property

Gets the index at which the dropped tabs should be inserted into the `TabView`.

C#
```cs
public int DropIndex { get; }
```

## TabView.ExternalTornOutTabsDropped event

Raised for `TabView`s where external tabs have been dropped onto this `TabView`.
Allows applications to add the new tabs' data items to this `TabView`.

C#
```cs
public event TypedEventHandler<TabView, TabViewExternalTornOutTabsDroppedEventArgs> ExternalTornOutTabsDropped;
```

## XamlRoot.CoordinateConverter property

Represents a `ContentCoordinateConverter` corresponding to the island that hosts the `XamlRoot`'s content.

C#
```cs
public Microsoft.UI.Content.ContentCoordinateConverter CoordinateConverter { get; }
```

# API Details

```cs (but really MIDL3)
namespace Microsoft.UI.Input
{
    enum MoveSizeOperation
    {
        Move,
        SizeBottom,
        SizeBottomLeft,
        SizeBottomRight,
        SizeLeft,
        SizeRight,
        SizeTop,
        SizeTopLeft,
        SizeTopRight,
    };

    runtimeclass EnteringMoveSizeEventArgs
    {
        PointInt32 PointerScreenPoint{ get; };
        MoveSizeOperation MoveSizeOperation{ get; };
        Microsoft.UI.WindowId MoveSizeWindowId;
    };

    runtimeclass EnteredMoveSizeEventArgs
    {
        PointInt32 PointerScreenPoint{ get; };
        MoveSizeOperation MoveSizeOperation{ get; };
    };

    runtimeclass WindowRectChangingEventArgs
    {
        PointInt32 PointerScreenPoint{ get; };
        MoveSizeOperation MoveSizeOperation{ get; };
        RectInt32 OldWindowRect{ get; };
        RectInt32 NewWindowRect;
        Boolean AllowRectChange;
        Boolean ShowWindow;
    };

    runtimeclass WindowRectChangedEventArgs
    {
        PointInt32 PointerScreenPoint{ get; };
        MoveSizeOperation MoveSizeOperation{ get; };
        RectInt32 OldWindowRect{ get; };
        RectInt32 NewWindowRect{ get; };
    };

    runtimeclass ExitingMoveSizeEventArgs
    {
        PointInt32 PointerScreenPoint{ get; };
        MoveSizeOperation MoveSizeOperation{ get; };
    };

    runtimeclass InputNonClientPointerSource
    {
        // Existing APIs snipped
        
        event TypedEventHandler<InputNonClientPointerSource, EnteringMoveSizeEventArgs> EnteringMoveSize;
        event TypedEventHandler<InputNonClientPointerSource, MoveSizeEnteredEventArgs> EnteredMoveSize;
        event TypedEventHandler<InputNonClientPointerSource, WindowRectChangingEventArgs> WindowRectChanging;
        event TypedEventHandler<InputNonClientPointerSource, WindowRectChangedEventArgs> WindowRectChanged;
        event TypedEventHandler<InputNonClientPointerSource, MoveSizeExitedEventArgs> ExitedMoveSize;
    };
}

namespace Microsoft.UI.Xaml.Controls
{
    runtimeclass TabViewTabTearOutWindowRequestedEventArgs
    {
        Object[] Items { get; };
        UIElement[] Tabs { get; };

        Microsoft.UI.WindowId NewWindowId;
    };

    runtimeclass TabViewTabTearOutRequestedEventArgs
    {
        Object[] Items { get; };
        UIElement[] Tabs { get; };

        Microsoft.UI.WindowId NewWindowId { get; };
    };

    runtimeclass TabViewExternalTornOutTabsDroppingEventArgs
    {
        Object[] Items { get; };
        UIElement[] Tabs { get; };

        Int32 DropIndex { get; };
        Boolean AllowDrop;
    };

    runtimeclass TabViewExternalTornOutTabsDroppedEventArgs
    {
        Object[] Items { get; };
        UIElement[] Tabs { get; };

        Int32 DropIndex { get; };
    };

    unsealed runtimeclass TabView : Microsoft.UI.Xaml.Controls.Control
    {
        // Existing APIs snipped.

        Boolean CanTearOutTabs;

        event TypedEventHandler<TabView, TabViewTabTearOutWindowRequestedEventArgs> TabTearOutWindowRequested;
        event TypedEventHandler<TabView, TabViewTabTearOutRequestedEventArgs> TabTearOutRequested;
        event TypedEventHandler<TabView, TabViewExternalTornOutTabsDroppingEventArgs> ExternalTornOutTabsDropping;
        event TypedEventHandler<TabView, TabViewExternalTornOutTabsDroppedEventArgs> ExternalTornOutTabsDropped;
            
        static Microsoft.UI.Xaml.DependencyProperty CanTearOutTabsProperty{ get; };
    };
}

namespace Microsoft.UI.Xaml
{
    runtimeclass XamlRoot
    {
        // Existing APIs snipped.

        Microsoft.UI.Content.ContentCoordinateConverter CoordinateConverter { get; }
    };
}
```
