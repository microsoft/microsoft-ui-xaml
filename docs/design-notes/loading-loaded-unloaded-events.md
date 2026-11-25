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
  `CEventManager::EnableEvents()`, which special-cases the Loaded event by calling `CEventManager::AddToLoadedEventList()`.
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

This is a proposal for a new pair of events, `Loaded2` and `Unloaded2`, that are meant to address the inconsistent behaviors of the current `Loaded` and `Unloaded` events.

## Timing - delayed synchronous

Both `Loaded2` and `Unloaded2` will be delayed-synchronous events. They're synchronous in the sense that WinUI calls the event handlers directly from a tick, rather than post an asynchronous event, but they're delayed in that WinUI does not immediately call the handler from Enter/Leave, instead waiting until the tick has finished with layout first.

Calling out directly during Enter/Leave was considered, but was rejected because it can cause reentrancy problems. Unloaded2, for example, will fire while we're still in the middle of `CFrameworkElement::LeaveImpl`. For cases like a closing popup, this is before popup has finished cleaning up its bookkeeping, and will cause problems if the app's handler responded by reopening the same popup. Delaying the events until the tick will allow all the Enter/Leave methods to exit and put WinUI in a more consistent state.

As for the timing of the event within the tick, these events will fire when `Loaded` fires today, after the layout pass and before rendering. This makes it easier for current consumers of `Loaded` to move to `Loaded2`, and makes layout information and expanded template parts potentially available to handlers (collapsed elements will get `Loaded2` without run layout). `Unloaded2` should be fired at the same place as `Loaded2` to maintain relative event order. An element can enter the tree and leave it in the same tick, in which case it must get `Loaded2` before `Unloaded2`.

## Differences with Loaded / Unloaded

`Loaded2` is mostly identical to `Loaded`, except apps can now see a `Loaded2` event when there was no `Loaded` event before. If an element enters and leaves the tree in the same tick, the act of leaving the tree disables event handlers and cancels any pending `Loaded` events that are going to be raised. We don't do that for `Loaded2`. If an element enters and leaves the tree in the same tick, the app will get a `Loaded2` event followed by an `Unloaded2` event.
> todo: attaching to Loaded after entering can sometimes still raise the event. Is that intentional? Should Loaded2 do it?

`Unloaded2` differs from `Unloaded` in its timing. `Unloaded` is an asynchronous event that's queued by an element leaving the tree and can be raised at some later time after the tick is done. This creates problems when an element leaves and re-enters the tree, since that second `Loaded` event will be raised synchronously during the tick, and  the first `Unloaded` event can be raised asynchronously after the tick. `Unloaded2` is a delayed synchronous event just like `Loaded2`, and will also raise during the tick. Apps should never see an out-of-order `Unloaded2` event. The sequence of `Loaded2` and `Unloaded2` events should exactly match the sequence of the element leaving and entering the live tree.

## Potential future expansion

`Loaded2` and `Unloaded2`'s event args are currently missing some information about the state of the tree at the time the event was queued. Current event handlers in WinUI controls don't need this information, but they could be added later if there are scenarios that need them:
* The current parent element at the time of entering/leaving the tree
* The current XamlRoot at the time of entering/leaving the tree
* A generation counter tracking how many times an element entered or left the tree

Right now the element's parent and XamlRoot are available as properties, but they only show the state at the time the event is raised, rather than at the time the event is queued. If an element moves from one part of the tree to another, then there's no way to know where in the tree it used to be.

Similarly, if an element leaves and reenters the tree at the same place multiple times, there's no way to know which leave and enter corresponds to the current `Unloaded2` or `Loaded2` event. A generation counter on the events as well as the element can be used to track this.

## Tests

Attach an element. L2 should be raised on the next tick. Detach an element after the frame. U2 should be raised on the next tick.
Attach and detach an element in the same frame. L2 and U2 should both be raised, in that order.
Reparent an element, U2 and L2 are raised in the that order.
Reparent an element multiple times before a tick. U2 and L2 should be correctly interleaved.
Detach from tree in L2 handler, verify U2 is raised in the same frame. Reattach in U2 handler, verify L2 is raised in same frame.
Attach a collapsed element. L2 should be raised. Detach it. U2 should be raised.
Detach an element without any other action, U2 should be raised.
Reparent an entire subtree. U2 and L2 should be raised on the subtree root as well as a leaf element.
Attach an element, then subscribe to L2. Should an event be raised? Loaded can raise an event here. Detach the element, then subscribe to U2. Should an event be raised?
L2 handler triggers layout work that attaches another element. That element should raise L2 in the same frame.
Close a Popup. U2 on the popup child should be raised after transition completes. Reopen the popup from that U2 handler. L2 should be raised on the child.

## Existing Controls

Existing controls should be migrated to using `Loaded2` and `Unloaded2`.

[drop list here]