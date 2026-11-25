# Loading / Loaded / Unloaded events

The FrameworkElement Loading, Loaded, and Unloaded events have some unusual/unexpected behaviors.

* **FrameworkElement.Loading:** The Loading event is **synchronous**, during the first Measure() call for an element.
This event is raised by `RaiseLoadingEventIfNeeded()`, and is raised the first time the function is called for an
element, regardless of whether the element is live in the tree.
* **FrameworkElement.Loaded:** The Loaded event is **special asynchronous**, sometime after the element has been
inserted into the live tree. The event is queued as soon as the element goes live, and is raised from
`CEventManager::RaiseLoadedEvent()` on the next Tick. The call sequence to raise this event goes like this:
  * Someone adds a handler to the Loaded event, which adds to `CUIElement::m_eventList`.
  * In `CUIElement::EnterImpl()`, if the element is going live its `m_eventList` is passed to
  `CEventManager::EnableEvents()`, which special-cases the Loaded event by calling
  `CEventManager::AddToLoadedEventList()`.
  That adds the element to `m_pLoadedEventList`.
  * On the next Tick, `CCoreServices::NWDrawTree()` calls `CEventManager::RaiseLoadedEvent()` to raise the event.
  * NOTE: If addition elements are added to the live tree during a Loaded event handler, those elements will be added to
  the end of `m_pLoadedEventList` and get fired during the current `RaiseLoadedEvent()` process. This provides an
  opportunity for the asynchronous Loaded event to be delivered before the Loading event on those elements.
* **FrameworkElement.Unloaded:** The Unloaded event is **(regular) asynchronous**. This event is queued by
`CFrameworkElement::LeaveImpl()` into the `CDeferredInvoke` queue, and fires whenever
`CDeferredInvoke::DispatchQueuedMessage()` gets called. `DispatchQueuedMessage` is called on a special CoreMessaging
timer which is inserted into the input queue. Because it is in the input queue, the event won't raise until after other
high priority work which happens before processing input.

### One summary of the above:

The Loaded and Unloaded events are both asynchronous, but in different ways (Loaded raises as part of tick, Unloaded is
posted to the UI thread). The end result is that if you have an element going back/forth in and out of the tree quickly,
you can start to get these two events out of order and in un-paired counts.

We need a unique Issue to consolidate this, because I think I’ve heard multiple workarounds. I think it’s
https://github.com/microsoft/microsoft-ui-xaml/issues/1900. The last comment there is to listen to both events, but to
look at the sender’s Parent property to see what’s really going on (non-null means that it’s loaded).

It can be problematic to use the IsLoaded property to disambiguate, because it’s synchronized with the events:

``` c++
_Check_return_ HRESULT FrameworkElement::get_IsLoadedImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = FALSE;
    CFrameworkElement* frameworkObj = static_cast<CFrameworkElement*>(GetHandle());
    if (frameworkObj != nullptr)
    {
        // if the Loaded event is still pending in the queue, we should report IsLoaded as false until event fires.
        if (frameworkObj->IsActive() && !frameworkObj->IsLoadedEventPending())
        {
            *pValue = TRUE;
        }
    }
    return S_OK;
}
```
Because of that, it is typically better to check Parent for non-null rather than checking IsLoaded.

# Loaded2 / Unloaded2

This is a proposal for a new pair of events, `Loaded2` and `Unloaded2`, that are meant to address the inconsistent
behaviors of the current `Loaded` and `Unloaded` events.

## Timing - delayed synchronous

Both `Loaded2` and `Unloaded2` will be delayed-synchronous events. They're synchronous in the sense that WinUI calls the
event handlers directly from a tick, rather than post an asynchronous event, but they're delayed in that WinUI does not
immediately call the handler from Enter/Leave, instead waiting until the tick has finished with layout first.

Calling out directly during Enter/Leave was considered, but was rejected because it can cause reentrancy problems.
Unloaded2, for example, will fire while we're still in the middle of `CFrameworkElement::LeaveImpl`. For cases like a
closing popup, this is before popup has finished cleaning up its bookkeeping, and will cause problems if the app's
handler responded by reopening the same popup. Delaying the events until the tick will allow all the Enter/Leave methods
to exit and put WinUI in a more consistent state.

As for the timing of the event within the tick, these events will fire when `Loaded` fires today, after the layout pass
and before rendering. This makes it easier for current consumers of `Loaded` to move to `Loaded2`, and makes layout
information and expanded template parts potentially available to handlers (collapsed elements will get `Loaded2` without
run layout). `Unloaded2` should be fired at the same place as `Loaded2` to maintain relative event order. An element can
enter the tree and leave it in the same tick, in which case it must get `Loaded2` before `Unloaded2`.

## Differences with Loaded / Unloaded

`Loaded2` is mostly identical to `Loaded`, except apps can now see a `Loaded2` event when there was no `Loaded` event
before. If an element enters and leaves the tree in the same tick, the act of leaving the tree disables event handlers
and cancels any pending `Loaded` events that are going to be raised. We don't do that for `Loaded2`. If an element
enters and leaves the tree in the same tick, the app will get a `Loaded2` event followed by an `Unloaded2` event.
> todo: attaching to Loaded after entering can sometimes still raise the event. Is that intentional? Should Loaded2 do
> it?

`Unloaded2` differs from `Unloaded` in its timing. `Unloaded` is an asynchronous event that's queued by an element
leaving the tree and can be raised at some later time after the tick is done. This creates problems when an element
leaves and re-enters the tree, since that second `Loaded` event will be raised synchronously during the tick, and  the
first `Unloaded` event can be raised asynchronously after the tick. `Unloaded2` is a delayed synchronous event just like
`Loaded2`, and will also raise during the tick. Apps should never see an out-of-order `Unloaded2` event. The sequence of
`Loaded2` and `Unloaded2` events should exactly match the sequence of the element leaving and entering the live tree.

## Potential future expansion

`Loaded2` and `Unloaded2`'s event args are currently missing some information about the state of the tree at the time
the event was queued. Current event handlers in WinUI controls don't need this information, but they could be added
later if there are scenarios that need them:
* The current parent element at the time of entering/leaving the tree
* The current XamlRoot at the time of entering/leaving the tree
* A generation counter tracking how many times an element entered or left the tree

Right now the element's parent and XamlRoot are available as properties, but they only show the state at the time the
event is raised, rather than at the time the event is queued. If an element moves from one part of the tree to another,
then there's no way to know where in the tree it used to be.

Similarly, if an element leaves and reenters the tree at the same place multiple times, there's no way to know which
leave and enter corresponds to the current `Unloaded2` or `Loaded2` event. A generation counter on the events as well as
the element can be used to track this.

## Tests

Attach an element. L2 should be raised on the next tick. Detach an element after the frame. U2 should be raised on the
next tick.
Attach and detach an element in the same frame. L2 and U2 should both be raised, in that order.
Reparent an element, U2 and L2 are raised in the that order.
Reparent an element multiple times before a tick. U2 and L2 should be correctly interleaved.
Detach from tree in L2 handler, verify U2 is raised in the same frame. Reattach in U2 handler, verify L2 is raised in
same frame.
Attach a collapsed element. L2 should be raised. Detach it. U2 should be raised.
Detach an element without any other action, U2 should be raised.
Reparent an entire subtree. U2 and L2 should be raised on the subtree root as well as a leaf element.
Attach an element, then subscribe to L2. Should an event be raised? Loaded can raise an event here. Detach the element,
then subscribe to U2. Should an event be raised?
L2 handler triggers layout work that attaches another element. That element should raise L2 in the same frame.
Close a Popup. U2 on the popup child should be raised after transition completes. Reopen the popup from that U2 handler.
L2 should be raised on the child.

## Existing Controls

Existing controls should be migrated to using `Loaded2` and `Unloaded2`.

The lists below are the consumers identified in the [subscriber
audit](loading-loaded-unloaded-events-investigation.md#control-subscriber-and-compatibility-audit-2026-11-19), including
subscriptions on parts and associated helpers. Inherited handlers also cover derived controls; this is not an exhaustive
inventory.

Unloaded2 annotations outline proposed handler adaptations beyond switching the event subscription, based on the static
audit. "No additional handler change identified" is not a validated migration result. Remove only stale-event
workarounds, not checks that legitimately gate live-tree-dependent work.

### MUX

* ButtonBase, ListBox, ListBoxItem, ComboBoxItem, ListViewBaseItem
  * **Loaded:** Refreshes visual states without transitions.
* Page, MenuFlyoutPresenter, WindowChrome
  * **Loaded:** Establishes or repairs initial focus; Page also notifies accessibility clients.
  * **Unloaded:** Page removes a layout-bounds callback. (Unloaded2: no additional handler change identified.)
* AppBar, SplitView
  * **Loaded:** Sets up host/layout subscriptions, registration, focus, overlays, and dismiss layers.
  * **Unloaded:** Tears down overlays/dismiss layers and registrations. (Unloaded2: no additional handler change
    identified.)
* AutoSuggestBox
  * **Loaded:** Part handlers discover the query button, hook Click, set its accessibility name, and size/position
    suggestions.
  * **Unloaded:** Removes query-button subscriptions or overlay state, with live-tree guards. (Unloaded2: review each
    guard before changing it; ordered delivery does not mean the control is currently detached, so preserve checks
    needed to avoid dismissing a currently live overlay.)
* ComboBox, CommandBar
  * **Loaded:** Part handlers realize/scroll selection, discover nested parts, hook keyboard handling, and set focus.
  * **Unloaded:** Closes dropdown/overflow; CommandBar also restores compact mode. (Unloaded2: no additional handler
    change identified.)
* ContentDialog
  * **Loaded:** Performs presentation setup such as focus.
  * **Unloaded:** Popup-child handler finishes closing or handles unexpected popup removal; close completion raises
    Closed, restores temporary interaction policies, and completes ShowAsync. (Unloaded2: no additional handler change
    identified; close completion must remain intact.)
* FlyoutBase
  * **Loaded:** Presenter handler handles focus, positioning, and open notification.
  * **Unloaded:** Presenter handler processes closure, clears open-flyout metadata, and can open a staged flyout.
    (Unloaded2: no additional handler change identified; delayed delivery permits the current Hide-then-stage ordering.)
* ApplicationBarService
  * **Loaded:** Uses appbar notifications for popup lifetime management.
  * **Unloaded:** Removes appbars from the pending-unload set and reevaluates whether the popup can close. (Unloaded2:
    no additional handler change identified.)
* ItemsPresenter
  * **Loaded:** Finds its ScrollViewer, reconciles viewport subscriptions, and delay-loads the footer. Also removes old
    subscriptions because Unloaded can be missing.
  * **Unloaded:** Detaches subscriptions. (Unloaded2: no additional handler change identified.)
* ListViewBase
  * **Loaded:** Resumes phased work and deferred scroll commands.
* ModernCollectionBasePanel
  * **Unloaded:** Detaches scroller/presenter components, removes sticky headers, and invalidates measure. (Unloaded2:
    no additional handler change identified.)
* ScrollViewer
  * **Loaded:** Maintains its loaded flag and synchronizes DirectManipulation transforms after viewport sizing.
  * **Unloaded:** Clears the flag and resets input/indicator state. (Unloaded2: no additional handler change
    identified.)
* ScrollContentPresenter
  * **Unloaded:** Unparents headers. (Unloaded2: no additional handler change identified.)
* Popup
  * **Loaded:** Child handler positions the popup.
* MediaTransportControls
  * **Loaded:** Part handlers update progress UI and discover/measure CommandBar parts.
* TimePicker
  * **Loaded:** Subscribes to host-window activation.
* MenuFlyoutItem, CascadingMenuHelper
  * **Loaded:** Clears menu interaction state flags. The helper also covers MenuFlyoutSubItem and SplitMenuFlyoutItem.
* Hub, HubSection
  * **Loaded:** Updates loaded flags and processes deferred/default-section scrolling, focus, and header styling.
  * **Unloaded:** Updates loaded flags. (Unloaded2: no additional handler change identified.)
* Pivot and slide-in support
  * **Loaded:** Synchronizes parallax after ScrollViewer load and registers slide-in elements with their parent
    PivotItem.
  * **Unloaded:** Completes animations and unregisters slide-in elements. (Unloaded2: no additional handler change
    identified.)

Framework support to review separately: ElementName binding's Loaded retry, FrameworkElement's automatic-tooltip
Unloaded handling, and EventTrigger's explicit Loaded support. EventTrigger exposes the legacy event contract, so it is
not a mechanical migration.

### MUXC

* AnimatedIcon, ProgressRing, NumberBox, TabViewItem
  * **Loaded:** Applies bound/inherited icon state and fallback, refreshes animation/VSM state, updates spin-button
    placement, or updates tab separators, respectively.
* BreadcrumbBar, BreadcrumbBarItem, AutoSuggestBoxHelper, TwoPaneView
  * **Loaded:** Refreshes repeater items, discovers/configures child parts, hooks Click/popup handlers, or adjusts
    nested ScrollContentPresenter behavior.
* RadioButtons, SelectorBar
  * **Loaded:** Reconciles initial selection with realized containers/selection models.
* NavigationView (including its repeaters), NavigationViewItemBase, NavigationViewItem, TabView
  * **Loaded:** Establishes/rebinds ancestor and host relationships and performs deferred visual updates.
    NavigationView's repeaters reconcile selection/animation; NavigationView manages title-bar subscriptions, and
    TabView manages tab content, tear-out registration, and nonclient regions.
  * **Unloaded:** Where subscribed, tears down those relationships/registrations. (Unloaded2: no additional handler
    change identified.)
* CommandBarFlyoutCommandBar, TextCommandBarFlyout
  * **Loaded:** Updates command states/geometry, sets initial focus, connects backdrop, or invokes the proofing button
    to open its menu, sometimes after a delay.
* AnimatedVisualPlayer
  * **Loaded:** Restores animated content/playback resources.
  * **Unloaded:** Stops playback, closes/releases the animated visual, and clears exposed state, with current-state
    defenses. (Unloaded2: remove the stale-event IsLoaded gate around teardown; retain the m_isUnloaded/UnloadContent
    path and ensure the following Loaded2 restores content.)
* WebView2
  * **Loaded:** Establishes/rebinds host subscriptions and its visual target.
  * **Unloaded:** Disconnects host/visual-target state, with current-state guards. (Unloaded2: remove the IsLoaded early
    return that skips historical departures; disconnect the stored host subscriptions/visual target and retain
    reconnection from the following Loaded2. Preserve unrelated visibility/readiness checks.)
* InkCanvas
  * **Loaded:** Attaches host/composition state.
  * **Unloaded:** Detaches subscriptions/visual links and reports session telemetry, with timing defenses. (Unloaded2:
    remove the stale-event IsLoaded early return; retain subscription/visual-link cleanup and telemetry deduplication,
    with Loaded2 restoring the visual link.)
* SystemBackdropElement
  * **Loaded:** Connects backdrop/composition resources.
  * **Unloaded:** Disconnects the target and releases resources, guarded against stale unloads. (Unloaded2: remove the
    !IsLoaded gate around ReleaseCompositionResources; retain reconnection from Loaded2 and keep the separate live-state
    check in UpdatePlacementVisual.)
* ScrollPresenter
  * **Loaded:** Initializes interaction/scroll-controller plumbing.
  * **Unloaded:** Interrupts pending operations, removes rendering callbacks, and zeroes viewport state, guarded by
    IsLoaded. (Unloaded2: remove the stale-event IsLoaded gate and revise the zero-RenderSize assertion; if teardown
    clears a viewport already populated by re-entry layout, request fresh layout or otherwise restore it before
    rendering. Keep operation interruption and rendering-hook cleanup.)
* ItemsView
  * **Loaded:** Initializes its attachment-dependent state.
  * **Unloaded:** Clears queued keyboard-navigation/bring-into-view work and rendering callbacks, guarded by IsLoaded.
    (Unloaded2: remove the !IsLoaded gate so each ordered departure performs rendering-hook and queued-navigation
    cleanup.)
* ItemsRepeater
  * **Loaded:** Updates lifecycle counters and provides recovery for unbalanced delivery, resetting scrollers and
    realization-window caches when needed.
  * **Unloaded:** Updates lifecycle counters and resets scrollers and realization-window caches when the counts indicate
    an in-sync unload. (Unloaded2: replace Loaded/Unloaded count matching with teardown on each ordered departure;
    replace Loaded's unbalanced-count recovery with an explicit re-entry InvalidateMeasure so measure reconnects
    scrollers after teardown.)
* ParallaxView, ScrollInputHelper, pull-to-refresh ScrollViewer adapter, SwipeControl
  * **Loaded:** Discovers source/target relationships and initializes composition interactions. SwipeControl initializes
    once but closes on every reload, including recycling.
* ColorPicker, ColorSpectrum, PersonPicture
  * **Unloaded:** Cancels outstanding bitmap generation/image reads. ColorSpectrum guards cancellation with IsLoaded;
    the others do not. (Unloaded2: for ColorSpectrum, removing the stale-event gate must be paired with restart or
    retention of needed bitmap work; template/property/size changes can start it before Loaded2, and there is no Loaded
    restart today. The restart/retention policy remains to be chosen. No additional handler change identified for
    ColorPicker or PersonPicture.)
* TeachingTip
  * **Loaded:** Target handler repositions the tip.
  * **Unloaded:** Handler on the tip or target closes it. (Unloaded2: no additional handler change identified.)
* RadioMenuFlyoutItem
  * **Loaded:** Restores radio-group membership. Submenu handler updates checked visual state.
  * **Unloaded:** Removes radio-group membership. (Unloaded2: no additional handler change identified.)
* ScrollView
  * **Unloaded:** Resets indicators, clears bring-into-view work/rendering callbacks, and resets its timer. (Unloaded2:
    no additional handler change identified.)
* ResizeGripper
  * **Unloaded:** Cancels an active drag, releases pointer capture, and raises canceled DragCompleted, guarded against
    stale unloads. (Unloaded2: remove the stale-event IsLoaded early return; retain the active-drag check, cancellation,
    pointer-capture release, and canceled DragCompleted notification.)
* InkToolbar
  * **Unloaded:** Flushes session telemetry. (Unloaded2: no additional handler change identified.)
* AnnotatedScrollBar
  * **Loaded:** Produces debug-only tracing.

### MUXTabular

* TableView (including its rows repeater)
  * **Loaded:** Acquires source ownership, restores repeater/scroller state, and hooks host/theme notifications.
  * **Unloaded:** Releases ownership, drains repeater state, detaches subscriptions, cancels pending work/interactions,
    and ends the telemetry session, guarded by IsLoaded. (Unloaded2: remove the IsLoaded early return in
    OnTableViewUnloaded; keep source release, repeater/subscription teardown, cancellation, and session completion, and
    ensure the following Loaded2 reacquires ownership and restores attachment state.)
* TableViewRow
  * **Loaded:** Updates geometry/visual states.
  * **Unloaded:** Restores temporarily suppressed cell-content tab stops. (Unloaded2: no additional handler change
    identified.)
* TableViewColumnHeader
  * **Loaded:** Updates geometry/visual states.
* TableViewTextColumn
  * **Loaded:** Editor handler discovers TextBox internals and refreshes editing metrics.
* TableViewCellAutomationPeer
  * **Unloaded:** Releases a temporarily held pre-edit accessibility name. (Unloaded2: no additional handler change
    identified.)
