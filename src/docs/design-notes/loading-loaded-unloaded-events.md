# Loading / Loaded / Unloaded events

The FrameworkElement Loading, Loaded, and Unloaded events have some unusual/unexpected behaviors.

* **FrameworkElement.Loading:** The Loading event is **synchronous**, during the first Measure() call for an element. 
This event is raised by `RaiseLoadingEventIfNeeded()`, and is raised the first time the function is called for an 
element, regardless of whether the element is live in the tree.
* **FrameworkElement.Loaded:** The Loaded event is **special asynchronous**, sometime after the element has been 
inserted into the live tree. The event is queued as soon as the element goes live, and is raised from 
`CEventManager::RaiseLoadedEvent()` on the next Tick. The call sequence to raise this event goes like this:
  * Someone adds a handler to the Loaded event, which adds to `CUIElement::m_pEventList`.
  * In `CUIElement::EnterImpl()`, if the element is going live its `m_pEventList` is passed to 
  `CEventManager::AddRequestsInOrder()`, which in turn calls `CEventManager::AddRequest()` for each queued event registration.
  * `CEventManager::AddRequest()` special-cases the Loaded event by calling `CEventManager::AddToLoadedEventList()`. 
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
