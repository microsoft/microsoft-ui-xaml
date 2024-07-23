// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <UIThreadScheduler.h>
#include "ReentrancyGuard.h"
#include <IHwndComponentHost.h>
#include <DXamlServices.h>
#include <DependencyObject.h>
#include <xcpwindow.h>
#include <optional>

#include <FocusSelection.h>

//------------------------------------------------------------------------
//
//  Method:   CEventManager::Create
//
//  Synopsis:
//      Creates an instance of an event manager object
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CEventManager::Create(
    _In_ CCoreServices* pCore,
    _Outptr_ CEventManager **ppObject
)
{
    HRESULT hr = S_OK;
    CEventManager *pMgr = NULL;

    pMgr = new CEventManager();

    IFC(gps->QueueCreate(&pMgr->m_pSlowQueue));
    IFC(gps->QueueCreate(&pMgr->m_pFastQueue));

    pMgr->m_pCore = pCore;

    *ppObject = pMgr;
    pMgr = NULL;

Cleanup:
    if (pMgr)
    {
        delete pMgr;
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CEventManager::~CEventManager()
{
    VERIFYHR(ClearRequests());

    ASSERT(NULL != m_pSlowQueue);
    ASSERT(NULL != m_pFastQueue);

    VERIFYHR(m_pSlowQueue->Close());
    m_pSlowQueue = NULL;
    VERIFYHR(m_pFastQueue->Close());
    m_pFastQueue = NULL;

    delete m_pRequest;
    m_pRequest = NULL;
    delete m_pLoadedEventList;
    m_pLoadedEventList = NULL;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::Disable
//
//  Synopsis:
//      Notify the EventManager that the core is going away
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEventManager::Disable()
{
    m_pCore = NULL;
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CEventManager::ClearRequests
//
//  Synopsis:
//      removes all requests and flushes the queue
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CEventManager::ClearRequests()
{
    HRESULT hr = S_OK;

    // Re-entrancy guard for this method.
    ++m_uInClearRequests;
    if(m_uInClearRequests != 1)
    {
        ASSERT(FALSE);
        goto Cleanup;
    }

    if(m_pRequest)
    {
        CEventRequestMap::iterator mapEndItr = m_pRequest->end();
        for (CEventRequestMap::iterator mapItr = m_pRequest->begin();
             mapItr != mapEndItr;
             ++mapItr)
        {
            CRequestsForObjectList* pRequests = (*mapItr).second;
            if(pRequests)
            {
                XUINT32 uRequestCtr = 0;
                while(uRequestCtr < pRequests->size())
                {
                    // Set the request to NULL in the list before deleting it. Deleting the request
                    // can release the contained DO, causing re-entrancy in the list and a double delete.
                    REQUEST* pTemp = NULL;
                    IFC(pRequests->get_item(uRequestCtr, pTemp));
                    IFC(pRequests->set_item(uRequestCtr, NULL));
                    delete pTemp;
                    ++uRequestCtr;
                }
                pRequests->clear();
                (*mapItr).second = NULL;
                delete pRequests;
                pRequests = NULL;
            }
        }
        m_pRequest->Clear();
    }

    if(m_pLoadedEventList)
    {
        XUINT32 uElementCtr = 0;
        while(uElementCtr < m_pLoadedEventList->size())
        {
            CDependencyObject* pTemp = NULL;
            IFC(m_pLoadedEventList->get_item(uElementCtr, pTemp));
            IFC(m_pLoadedEventList->set_item(uElementCtr, NULL));
            ReleaseInterface(pTemp);
            ++uElementCtr;
        }
        m_pLoadedEventList->clear();
    }

    IFC(FlushQueue());

Cleanup:
    --m_uInClearRequests;
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Flushes the thread safe queue, and deletes any remaining items.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEventManager::FlushQueue()
{
    HRESULT hr = S_OK;
    RaiseArguments *pArgs = NULL;

    ASSERT(NULL != m_pSlowQueue);
    ASSERT(NULL != m_pFastQueue);

    pArgs = NULL;
    while (SUCCEEDED(m_pSlowQueue->Get((void**)&pArgs, 0)))
    {
        ASSERT(NULL != pArgs);
        delete pArgs;
        pArgs = NULL;
    }

    pArgs = NULL;
    while (SUCCEEDED(m_pFastQueue->Get((void**)&pArgs, 0)))
    {
        ASSERT(NULL != pArgs);
        delete pArgs;
        pArgs = NULL;
    }

    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   AddRef
//
//  Synopsis:
//      Raises the reference count on the object.
//
//------------------------------------------------------------------------

XUINT32
CEventManager::AddRef()
{
    return ++m_cRef;
}

//------------------------------------------------------------------------
//
//  Method:   Release
//
//  Synopsis:
//      Lowers the reference count on the object.  Will delete it when there
// are no remaining references.
//
//------------------------------------------------------------------------

XUINT32
CEventManager::Release()
{
    XUINT32 cRef = --m_cRef;

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::AddRequestsInOrder
//
//  Synopsis:
//      Walks through the list of events and adds a request for every event.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CEventManager::AddRequestsInOrder(
                               _In_ CDependencyObject *pObject,
                               _In_ CXcpList<REQUEST> *pEventList
                               )
{
    HRESULT hr = S_OK;
    CXcpList<REQUEST>* pEventListInCorrectOrder = NULL;
    CXcpList<REQUEST>::XCPListNode *pTemp       = NULL;
    REQUEST * pRequest                          = NULL;


    ASSERT(pObject && pEventList);

    // The events should be fired in this order:
    // -----------------------------------------
    // (1) Events registered in the constructor.
    // (2) Events hooked up in XAML.
    // (3) Elsewhere! (Page_Loaded, LayoutUpdated, ...etc), using the "+=" syntax.
    // But, When registered, an event is being added to the list of events
    // (m_pEventList) at the head (This is how CXcpList behaves), we are reversing them
    // here, so that, the event manager fires them in the correct order.

    pEventListInCorrectOrder = new CXcpList<REQUEST>;
    pEventList->GetReverse(pEventListInCorrectOrder);

    // Get the head of the list.
    pTemp = pEventListInCorrectOrder->GetHead();

    // Walk the list, and AddRequest for every event.
    while (pTemp)
    {
        pRequest = (REQUEST *)pTemp->m_pData;

        // Do not add requests that have the added flag set.
        if(pRequest->m_bAdded == FALSE)
        {
            IFC( this->AddRequest(pObject, pRequest));
        }

        pTemp = pTemp->m_pNext;
    }

Cleanup:
    // Get rid of the list, if it's there.
    if(pEventListInCorrectOrder)
    {
        // Don't "delete" the data, we still need them.
        pEventListInCorrectOrder->Clean(FALSE);
        delete pEventListInCorrectOrder;
        pEventListInCorrectOrder = NULL;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::RemoveRequests
//
//  Synopsis:
//      Walks through the list of events and removes a request for every event.
//
//      This is intended to be used as a mirror image to AddRequestsInOrder()
//  but without the "InOrder" in the name because while add ordering matters,
//  removal ordering does not.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CEventManager::RemoveRequests(
                               _In_ CDependencyObject *pObject,
                               _In_ CXcpList<REQUEST> *pEventList
                               )
{
    CXcpList<REQUEST>::XCPListNode *pTemp = pEventList->GetHead();
    while (pTemp)
    {
        REQUEST * pRequest = (REQUEST *)pTemp->m_pData;
        IFC_RETURN( RemoveRequest(pObject, pRequest));
        pTemp = pTemp->m_pNext;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::RemoveObject
//
//  Synopsis:
//      Removes the given DependencyObject from the EventRequestMap
//
//      This is different from RemoveRequests which does not remove the map entry
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CEventManager::RemoveObject(_In_ CDependencyObject *pObject)
{
    CRequestsForObjectList* pObjRequests = NULL;

    if (m_uInClearRequests > 0)
    {
        // We need to no-op here when ClearRequests() is called because it could clean up
        // a contained DO.  In that case, we would delete could requests here that ClearRequests()
        // is still operating on.  ClearRequests() will clear all of this out anyway.
        // We should be ok in this ClearRequests(CDependencyProperty*,CDependencyObject*)
        // since the DO is passed in so presumably there is still something keeping it alive
        return S_OK;
    }

    if (m_pRequest && pObject)
    {
        IFC_RETURN(m_pRequest->Remove(pObject, pObjRequests));
        if (pObjRequests)
        {
            // There shouldn't be any requests left because they should be removed by CUIElement::LeaveImpl
            // or CPopup::Release but delete any existing requests just in case
            XUINT32 uRequestCtr = 0;
            while (uRequestCtr < pObjRequests->size())
            {
                // Set the request to NULL in the list before deleting it. Deleting the request
                // can release the contained DO, causing re-entrancy in the list and a double delete.
                REQUEST* pTemp = NULL;
                IFC_RETURN(pObjRequests->get_item(uRequestCtr, pTemp));
                IFC_RETURN(pObjRequests->set_item(uRequestCtr, NULL));
                delete pTemp;
                ++uRequestCtr;
            }
            pObjRequests->clear();
            delete pObjRequests;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::AddRequest
//
//  Synopsis:
//      Add an request to the event manager
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CEventManager::AddRequest(
                          _In_ CDependencyObject *pObject,
                          _In_ REQUEST *pRequest
                          )
{
    HRESULT hr = S_OK;
    REQUEST* request = NULL;
    CRequestsForObjectList* pObjRequests = NULL;
    CRequestsForObjectList* pObjRequestsTemp = NULL;

    ASSERT(pRequest);

    // create and initialize a REQUEST object
    request = new REQUEST();
    request->m_pListener = pObject;
    request->m_hEvent = pRequest->m_hEvent;
    request->m_pObject = pObject;
    request->m_pfnInternalEventDelegate = pRequest->m_pfnInternalEventDelegate;
    request->m_iToken = pRequest->m_iToken;
    request->m_bFired = FALSE;
    request->m_bAdded = FALSE;
    request->m_bHandledEventsToo = pRequest->m_bHandledEventsToo;

    // create the event handler map if necessary
    if(!m_pRequest)
    {
        m_pRequest = new CEventRequestMap();
    }

    // get the list of event handlers for this object, create if one doesn't exist
    IFC(m_pRequest->Get(pObject, pObjRequests));
    if(!pObjRequests)
    {
        pObjRequestsTemp = new CRequestsForObjectList();
        IFC(m_pRequest->Add(pObject, pObjRequestsTemp));
        pObjRequests = pObjRequestsTemp;
        pObjRequestsTemp = NULL;
    }
    // Add the request to the list of event handlers for this object
    IFC(pObjRequests->push_back(request));

    AddRefInterface(request->m_pListener);
    AddRefInterface(request->m_pObject);

    // Loaded events are not fired on a per-element basis, rather on the entire tree at once.
    // Also, Loaded events should be fired in the order in which their handlers were added.
    // m_pRequest map maintains the order of events for each element but does not maintain
    // the order among the elements themselves. As a result, we need to maintain a separate
    // list for the order among elements for which Loaded event is pending.
    if (IsLoadedEvent(pRequest->m_hEvent))
    {
        IFC(AddToLoadedEventList(pObject));
    }

    request = NULL;

Cleanup:
    delete pObjRequestsTemp;
    delete request;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::CreateRequest
//
//  Synopsis:
//      Create a request  ... does NOT add it to the list ....
//------------------------------------------------------------------------
_Check_return_
HRESULT
CEventManager::CreateRequest(
    _Outptr_ REQUEST **ppRequest,
    _In_ EventHandle hEvent,
    _In_ CDependencyObject *pObject,
    _In_ CValue *pValue,
    _In_ XINT32 iToken,
    _In_ CCoreServices *pContext /* = NULL */,
    _In_ XINT32 fHandledEventsToo
   )
{
    REQUEST *pRequest = NULL;

    ASSERT( pObject == NULL || pContext == NULL || pObject->GetContext() == pContext );

    if (ppRequest == NULL || pValue == NULL)
        IFC_RETURN(E_INVALIDARG);

    *ppRequest = NULL;

    if ((pValue->GetType() != valueObject) &&
        (pValue->GetType() != valueAny) &&
        (pValue->GetType() != valueInternalHandler))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    pRequest = new REQUEST;


    // Place the request in the array.

    pRequest->m_hEvent = hEvent;
    pRequest->m_pListener = NULL; // don't store the listener object till it is added to the eventmanager (AddRequest);
    pRequest->m_pObject = NULL; // don't store the target object till it is added to the eventmanager (AddRequest);

    pRequest->m_iToken = iToken;
    pRequest->m_bHandledEventsToo = (XUINT8)fHandledEventsToo; // The requested event will be invoked even though event is handled

    if (pValue->GetType() == valueInternalHandler)
    {
        pRequest->m_pfnInternalEventDelegate = pValue->AsInternalHandler();
    }

    *ppRequest = pRequest;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::ClearRequests
//
//  Synopsis:
//      Remove all the specified requests associated with this object
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CEventManager::ClearRequests(
    _In_ EventHandle hEvent,
    _In_ CDependencyObject *pObject
)
{
    CRequestsForObjectList* pRequests = NULL;

    if(m_pRequest)
    {
        // Get the list of REQUESTs for this object
        IFC_RETURN(m_pRequest->Get(pObject, pRequests));

        if(pRequests)
        {
            // Iterate through the list, removing entries corresponding to hEvent
            XUINT32 uRequestCtr = 0;
            while(uRequestCtr < pRequests->size())
            {
                REQUEST* pNodeRequest = NULL;
                IFC_RETURN(pRequests->get_item(uRequestCtr, pNodeRequest));

                if (pNodeRequest
                 && pNodeRequest->m_pObject == pObject
                 && pNodeRequest->m_hEvent == hEvent)
                {
                    // Remove the request from the list before deleting it. Deleting the request
                    // can release the contained DO, causing re-entrancy in the list and a double delete.
                    IFC_RETURN(pRequests->erase(uRequestCtr));
                    delete pNodeRequest;
                    pNodeRequest = NULL;
                }
                else
                {
                    ++uRequestCtr;
                }
            }

            // Do not delete the list even though it might be empty. This method call could be a re-entrant call
            // in the event manager i.e. there could be code on the stack iterating over this list.


            // All Loaded event have been removed. Remove the object from the loaded event list.
            if(IsLoadedEvent(hEvent))
            {
                IFC_RETURN(RemoveFromLoadedEventList(pObject));
            }
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CEventManager::RemoveRequest
//
//  Synopsis:
//      Remove the specific request associated with this object
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CEventManager::RemoveRequest(
    _In_ CDependencyObject *pObject,
    _In_ REQUEST *pRequest
)
{
    CRequestsForObjectList* pRegisteredRequests = NULL;
    bool fHasLoadedEvent = false;
    bool fRemovedLoadedEvent = false;

    ASSERT(pRequest);

    if(pObject && pObject->GetContext()->IsShuttingDown())
    {
        return S_OK;
    }

    if(m_pRequest)
    {
        // Get the list of REQUESTs for this object
        IFC_RETURN(m_pRequest->Get(pObject, pRegisteredRequests));

        // Ordinarily, pRegisteredRequests will not be NULL. However, this RemoveRequest call could be a
        // re-entrant call with ClearRequests on the stack. In such a case, ClearRequests could have set some of
        // the entries to NULL.
        if(pRegisteredRequests)
        {
            // Iterate through the list, removing entries corresponding to the REQUEST passed in
            XUINT32 uRequestCtr = 0;
            while(uRequestCtr < pRegisteredRequests->size())
            {
                REQUEST* pNodeRequest = NULL;
                IFC_RETURN(pRegisteredRequests->get_item(uRequestCtr, pNodeRequest));

                // Similar to the check for the pRegisteredRequests above, some nodes in the list may also be NULL
                // when we re-enter this CEventManager method during execution of CEventManager::ClearRequests.
                if (pNodeRequest != NULL)
                {
                    if ((pObject == pNodeRequest->m_pObject) &&
                        (pNodeRequest->m_hEvent == pRequest->m_hEvent) &&
                        (
                            (pNodeRequest->m_pListener != NULL && pNodeRequest->m_pListener == pObject) ||
                            (pNodeRequest->m_iToken >= 0 && pNodeRequest->m_iToken == pRequest->m_iToken) ||
                            (pNodeRequest->m_pfnInternalEventDelegate != NULL && pNodeRequest->m_pfnInternalEventDelegate == pRequest->m_pfnInternalEventDelegate)
                        )
                       )
                    {
                        if(IsLoadedEvent(pNodeRequest->m_hEvent))
                        {
                            fRemovedLoadedEvent = TRUE;
                        }
                        // Remove the request from the list before deleting it. Deleting the request
                        // can release the contained DO, causing re-entrancy in the list and a double delete.
                        IFC_RETURN(pRegisteredRequests->erase(uRequestCtr));
                        delete pNodeRequest;
                        pNodeRequest = NULL;
                    }
                    else
                    {
                        if(IsLoadedEvent(pNodeRequest->m_hEvent))
                        {
                            fHasLoadedEvent = TRUE;
                        }
                        ++uRequestCtr;
                    }
                }
                else
                {
                    ++uRequestCtr;
                }
            }

            // Do not delete the list even though it might be empty. This method call could be a re-entrant call
            // in the event manager i.e. there could be code on the stack iterating over this list.


            // If we removed a Loaded event handler and there are no more Loaded event handlers, then
            // remove the object from the Loaded event list.
            if(fRemovedLoadedEvent && !fHasLoadedEvent)
            {
                IFC_RETURN(RemoveFromLoadedEventList(pObject));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CEventManager::RaiseLoadedEventForObject(_In_ CDependencyObject* pLoadedEventObject, _In_ CEventArgs* loadedArgs)
{
    TraceRaiseLoadedEventBegin((UINT64)pLoadedEventObject);
    auto scopeGuard = wil::scope_exit([]
    {
        TraceRaiseLoadedEventEnd();
    });

    CRequestsForObjectList* pRegisteredRequests = nullptr;
    // look up the registered requests for the current object
    if (m_pRequest)
    {
        IFC_RETURN(m_pRequest->Get(pLoadedEventObject, pRegisteredRequests));
        if (pRegisteredRequests)
        {
            bool bFired;
            std::optional<bool> previousMessageReentrancyGuard;
            CXcpDispatcher* dispatcher = nullptr;

            auto messageReentrancyGuard = wil::scope_exit([&]
            {
                if (dispatcher && previousMessageReentrancyGuard.has_value())
                {
                    dispatcher->SetMessageReentrancyGuard(previousMessageReentrancyGuard.value());
                }
            });

            // WebView2.OnLoaded makes a call to EBWebView's put_isVisible,
            // which calls ShowWindow and can cause reentrancy. Here, we check
            // if the loaded event object is a WV2, and if so, we exempt it from
            // reentrancy checks.
            if (pLoadedEventObject->GetTypeIndex() == KnownTypeIndex::Panel)
            {
                ctl::ComPtr<DirectUI::DependencyObject> peer;
                IFC_RETURN(DirectUI::DXamlServices::TryGetPeer(pLoadedEventObject, &peer));
                if (peer)
                {
                    ctl::ComPtr<IHwndComponentHost> host = peer.AsOrNull<IHwndComponentHost>();
                    if (host != nullptr)
                    {
                        dispatcher = GetXcpDispatcher(pLoadedEventObject);
                        if (dispatcher)
                        {
                            previousMessageReentrancyGuard = dispatcher->GetMessageReentrancyGuard();
                            dispatcher->SetMessageReentrancyGuard(false);
                        }
                    }
                }
            }

            // raise the event for all registered requests for the current object
            IFC_RETURN(RaiseHelper(
                pRegisteredRequests,
                EventHandle(KnownEventIndex::FrameworkElement_Loaded),
                nullptr,                    // pSender
                loadedArgs,
                FALSE,                      // bRefire
                m_pfnScriptCallbackSync,    // synchronous raise
                bFired));

        }
    }

    return S_OK;
}

CXcpDispatcher* CEventManager::GetXcpDispatcher(_In_ CDependencyObject* pLoadedEventObject)
{
    CCoreServices *pCore = pLoadedEventObject->GetContext();
    IXcpHostSite *pHostSite = pCore->GetHostSite();
    CXcpDispatcher* dispatcher = nullptr;

    if (pHostSite)
    {
        auto iXcpDispatcher = pHostSite->GetXcpDispatcher();
        if (iXcpDispatcher)
        {
            dispatcher = static_cast<CXcpDispatcher*>(iXcpDispatcher);
        }
    }

    return dispatcher;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::RaiseLoadedEvent
//
//  Synopsis: Called to synchronously raise the Loaded event for any
//            Loaded listeners that have been registered up to this point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CEventManager::RaiseLoadedEvent()
{
    TraceRaiseAllLoadedEventsBegin();

    auto scopeGuard = wil::scope_exit([]
    {
        TraceRaiseAllLoadedEventsEnd();
    });
    // Callers should check ShouldRaiseLoadedEvent() before calling RaiseLoadedEvent()
    ASSERT(m_fRaiseLoadedEventNeeded);

    // If m_pLoadedEventList is NULL, no Loaded event listeners have been registered, so there's nothing to do.
    if (!m_fRaiseLoadedEventNeeded || !m_pLoadedEventList)
    {
        return S_OK;
    }

    // we must have a Core at this point - this check is needed because a call to Disable() can null it out
    if (!m_pCore)
    {
        IFC_RETURN(E_FAIL);
    }

    if (m_pLoadedEventList->size() > 0)
    {
        xref_ptr<CEventArgs> spLoadedArgs = make_xref<CRoutedEventArgs>();

        for (XUINT32 iLoadedEventObject = 0;
            iLoadedEventObject < m_pLoadedEventList->size();
            ++iLoadedEventObject)
        {
            xref_ptr<CDependencyObject> spLoadedEventObject;
            IFC_RETURN(m_pLoadedEventList->get_item(iLoadedEventObject, *spLoadedEventObject.ReleaseAndGetAddressOf()));
            if (!spLoadedEventObject)
            {
                continue;
            }
            // NULL out the list entry first to guard against reentrant removal.
            IFC_RETURN(m_pLoadedEventList->set_item(iLoadedEventObject, nullptr));
            IFC_RETURN(RaiseLoadedEventForObject(spLoadedEventObject, spLoadedArgs.get()));
        }
        m_pLoadedEventList->clear();
    }

    // reset the loaded event needed flag - we've now finished raising the event
    m_fRaiseLoadedEventNeeded = FALSE;

    TraceRaiseAllLoadedEventsEnd();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::Raise
//
//  Synopsis:
//      Scan the array of requests and call those register for the specified
//  event.
//  Sender is the current element to fire the event on, Source is the element that initiated this
//------------------------------------------------------------------------

void
CEventManager::Raise(
    _In_ EventHandle hEvent,
    _In_ XINT32 bRefire,
    _In_opt_ CDependencyObject *pSender,
    _In_opt_ CEventArgs *pArgs,
    _In_ bool fRaiseSync,
    _In_ bool fInputEvent,
    _In_ bool bAllowErrorFallback,
    _In_opt_ CDependencyObject *pSenderOverride)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    CRequestsForObjectList* pRegisteredRequests = NULL;
    bool bFired = false;

    // Verify the sync input event.
    ASSERT((IsValidSyncInputEvent(hEvent, fInputEvent, fRaiseSync)) || !IsSyncInputEvent(hEvent));

    if (pSender && !pSender->ShouldRaiseEvent(hEvent, fInputEvent, pArgs))
    {
        // Do not raise events when there are no listeners, unless there are implicit
        // event listeners, such as RichTextBlock::OnTapped.
        return;
    }

    if (pSender && pSender->GetContext()->IsShuttingDown())
    {
        // Do not raise events when shutting down.
        return;
    }

    // Do not use Raise to raise the Loaded event - see RequestRaiseLoadedEventOnNextTick() and RaiseLoadedEvent().
    ASSERT(!IsLoadedEvent(hEvent));

    // Callbacks can be as following:
    // 1. Both m_pfnScriptCallbackAsync and m_pfnScriptCallbackSync are NULL.
    // It can be
    // a)when CCoreServices::RegisterScriptCallback haven't been invoked yet or
    // b)when CCoreServices::RegisterScriptCallback invoked with both parameter are NULL
    //
    // 2. m_pfnScriptCallbackAsync is NOT NULL and m_pfnScriptCallbackSync is NULL
    // It can be on MAC scenario
    //
    // 3. Both m_pfnScriptCallbackAsync and m_pfnScriptCallbackSync are valid.
    // last case is invalid scenario. ASSERT it.

    ASSERT(!(m_pfnScriptCallbackAsync == NULL && m_pfnScriptCallbackSync != NULL));

    EVENTPFN pfnScriptCallback = NULL;
    if (fRaiseSync && m_pfnScriptCallbackSync)
    {
        //   Synchronous call to delegate of internal event listener
        pfnScriptCallback = m_pfnScriptCallbackSync;
    }
    else
    {
        // Schedule an asynchronous call to delegate of internal event listener
        pfnScriptCallback = m_pfnScriptCallbackAsync;
    }

    if ((EventEnabledInputEventsBegin() || EventEnabledKeyEventsBegin()) && fInputEvent)
    {
        TraceInputEventBegin(pSender, pArgs, hEvent);
    }

    CUIElement* pUIElement = do_pointer_cast<CUIElement>(pSender);
    CControl * pSenderControl = nullptr;
    bool skipRaiseEvent = false;

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    if (static_cast<unsigned int>(hEvent.index) <= static_cast<unsigned int>(LastControlEvent) &&
        pUIElement && pUIElement->HasAttachedInteractions())
    {
        bool hasInteractionForEvent = false;
        IFC(FxCallbacks::InteractionCollection_HasInteractionForEvent(hEvent.index, pUIElement, hasInteractionForEvent));
        if (hasInteractionForEvent)
        {
            IFC(FxCallbacks::InteractionCollection_DispatchInteraction(hEvent.index, pUIElement, pArgs));
            skipRaiseEvent = true;
        }
    }
#endif

    if (!skipRaiseEvent)
    {
        // here pfnScriptCallback might be NULL. See scenario 1a.
        if (IsValidStaticEventDelegate(hEvent) && pSender == nullptr)
        {
            RaiseStaticEvents(hEvent, pArgs, pfnScriptCallback);
        }
        else if (pfnScriptCallback && SUCCEEDED(DoPointerCast(pSenderControl, pSender)))
        {
            RaiseControlEvents(hEvent, pSenderControl, pArgs, pfnScriptCallback);
        }
        else if (pUIElement)
        {
            RaiseUIElementEvents(hEvent, pUIElement, pArgs, pfnScriptCallback);
        }

        if(m_pRequest)
        {
            // If sender is known, get the list of event handlers for that sender. Else, raise the event for all objects.
            if(pSender)
            {
                IFC(m_pRequest->Get(pSender, pRegisteredRequests));
                if(pRegisteredRequests)
                {
                    IFC(RaiseHelper(pRegisteredRequests, hEvent, pSender, pArgs, bRefire, pfnScriptCallback, bFired, pSenderOverride));
                }
            }
            else
            {
                CEventRequestMap::const_iterator endItr = m_pRequest->end();
                for (CEventRequestMap::const_iterator mapItr = m_pRequest->begin();
                        mapItr != endItr;
                        ++mapItr)
                {
                    pRegisteredRequests = (*mapItr).second;
                    if(pRegisteredRequests)
                    {
                        IFC(RaiseHelper(pRegisteredRequests, hEvent, pSender, pArgs, bRefire, pfnScriptCallback, bFired, pSenderOverride));
                    }
                }
            }
        }
    }

    if ((EventEnabledInputEventsEnd() || EventEnabledKeyEventsEnd()) && fInputEvent)
    {
        TraceInputEventEnd(pArgs, hEvent);
    }

    // If this is a error event and we did not fire it yet, report it via the error service.
    if (bAllowErrorFallback && !bFired && pArgs != NULL && pArgs->IsErrorEventArgs())
    {
        // We can guarantee pArgs is an ErrorEventArgs.
        //
        // Generate ErrorService to hold error information, and report the error message.
        //
        ASSERT(pSender);
        CCoreServices *pCore = pSender->GetContext();
        CErrorEventArgs *pErrorArgs = static_cast<CErrorEventArgs*>(pArgs);

        if (pCore != NULL && pErrorArgs != NULL)
        {
            IErrorService *pErrorService = NULL;
            VERIFYHR(pCore->getErrorService(&pErrorService));

            if(pErrorService)
            {
                SuspendFailFastOnStowedException suspender; // http://osgvsowi/6897785 it is possible to have invalid URI from toast payload, ignore error

                VERIFYHR(pErrorService->ReportGenericError(pErrorArgs->m_hResult, pErrorArgs->m_eType, pErrorArgs->m_iErrorCode, 1, 0, 0, NULL, 0, pArgs, pSender));
                pCore->ReportAsyncErrorToBrowserHost( );
            }
        }
    }

Cleanup:

    return;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::RaiseHelper
//
//  Synopsis:
//      Scan the list of requests of a given sender and call those register for the specified  event.
//------------------------------------------------------------------------

HRESULT CEventManager::RaiseHelper(CRequestsForObjectList* pRegisteredRequests,
                                _In_ EventHandle hEvent,
                                _In_ CDependencyObject *pSender,
                                CEventArgs *pArgs,
                                XINT32 bRefire,
                                EVENTPFN pfnScriptCallback,
                                bool& bFired,
                                _In_opt_ CDependencyObject *pSenderOverride)
{
    bFired = FALSE;

    for (XUINT32 requestItr = 0; requestItr < pRegisteredRequests->size(); requestItr++)
    {
        REQUEST* pRequest = NULL;

        IFC_RETURN(pRegisteredRequests->get_item(requestItr, pRequest));

        if (pRequest && hEvent == pRequest->m_hEvent)
        {
            // Prevent double firing of events
            if (!bRefire && pRequest->m_bFired)
            {
                continue;
            }

            // Some events, such as VisualStateChanging/Changed need to arrive with a different sender specified.
            // Callers can optionally set pSenderOverride to specify this different sender.
            CDependencyObject* pActualSender = NULL;
            if(pSenderOverride)
            {
                pActualSender = pSenderOverride;
            }
            else
            {
                pActualSender = pRequest->m_pObject == NULL ? pSender : pRequest->m_pObject;
            }

            pRequest->m_bFired = TRUE;

            // here we checking assuming we are in the scenario 2.
            if (m_pfnScriptCallbackAsync)
            {
                XUINT32 flags = pRequest->m_bHandledEventsToo ? EVENT_HANDLEDEVENTSTOO : 0;
                        flags |= IsSyncInputEvent(hEvent) ? EVENT_SYNC_INPUT : 0;
                        flags |= (IsLoadedEvent(hEvent) || IsLoadingEvent(hEvent)) ? EVENT_SYNC_LOADED : 0;

                if (pRequest->m_pfnInternalEventDelegate != NULL)
                {
                    // if we are here we don't need to check pfnScriptCallback
                    // it will be at least m_pfnScriptCallbackAsync
                    (pfnScriptCallback)(
                        m_pControl,
                        NULL,
                        hEvent,
                        pRequest->m_pObject,
                        pArgs,
                        flags,
                        NULL,
                        pRequest->m_pfnInternalEventDelegate);
                }
                else
                {
                    WCHAR* temp = NULL;

                    if (!pRequest->m_pListener)
                    {
                        (m_pfnScriptCallbackAsync)(
                            m_pControl,
                            NULL,
                            hEvent,
                            pActualSender,
                            pArgs,
                            flags,
                            NULL,
                            NULL);
                    }
                    else
                    {
                        bool isSenderPegged = false;

                        // These pegs are legitimate shutdown exceptions because we peg the peers but shutdown could occur before
                        // the event is fired on the managed side.  In those cases, there are not actual leaks
                        if (pActualSender && pActualSender->HasManagedPeer())
                        {
                            if (pActualSender->PegManagedPeer(TRUE/*isShutdownException*/) == S_OK)
                            {
                                isSenderPegged = true;
                            }
                        }

                        flags |= isSenderPegged ? EVENT_SENDER_PEGGED : 0;

                        if (FAILED((pfnScriptCallback)(
                                m_pControl,
                                pRequest->m_pListener,
                                hEvent,
                                pActualSender,
                                pArgs,
                                flags,
                                nullptr,
                                nullptr)) &&
                            isSenderPegged)
                        {
                            pActualSender->UnpegManagedPeer(TRUE /*isShutdownException*/);
                        }

                    }
                    delete [] temp;
                }
                bFired = TRUE;
            }
       }
   }
    return S_OK;
}



//------------------------------------------------------------------------
//
//  Inline Method:   CEventManager::RaiseClassEvents
//
//  Synopsis:
//      Invokes CLR event for a controls which have managed peer
//      otherwise invokes Event handler with Native callback delegate.
//------------------------------------------------------------------------
inline void CEventManager::RaiseControlEvents(
    _In_ EventHandle hEvent,
    _In_opt_ CControl *pSender,
    _In_opt_ CEventArgs *pArgs,
    _In_ EVENTPFN pfnScriptCallback)
{
    if (!CControl::IsValidDelegate(hEvent.index))
    {
        return;
    }

    if (pSender &&
        pSender->HasManagedPeer())
    {
        bool isSenderPegged = false;
        XUINT32 flags = 0;

        // Add the control to the list of controls with a pending event
        // so we don't lose it until the event gets fired
        // These pegs are legitimate shutdown exceptions because we peg the peers but shutdown could occur before
        // the event is fired on the managed side.  In those cases, there are not actual leaks
        if (pSender->PegManagedPeer(TRUE/*isShutdownException*/) == S_OK)
        {
            isSenderPegged = true;
        }

        flags |= isSenderPegged ? EVENT_SENDER_PEGGED : 0;
        flags |= IsSyncInputEvent(hEvent) ? EVENT_SYNC_INPUT : 0;

        if (FAILED((pfnScriptCallback)(
                m_pControl,
                nullptr,
                hEvent,
                pSender,
                pArgs,
                flags,
                nullptr,
                nullptr)) &&
            isSenderPegged)
        {
            pSender->UnpegManagedPeer(TRUE /*isShutdownException*/);
        }
    }
    else
    {
        (pfnScriptCallback)(
            m_pControl,
            nullptr,
            hEvent,
            pSender,
            pArgs,
            0,
            nullptr,
            CControl::Delegates[static_cast<XUINT32>(hEvent.index) - 1 /* UnknownEvent */]);
    }
}

//------------------------------------------------------------------------
//
//  Inline Method:   CEventManager::RaiseUIElementEvents
//
//  Synopsis:
//      For UIElements that are not controls we will bypass the reverse
//      PInvoke for performance and directly call the event handlers
//      on the core CUIElement.
//------------------------------------------------------------------------
inline void CEventManager::RaiseUIElementEvents(
    _In_ EventHandle hEvent,
    _In_opt_ CUIElement *pSender,
    _In_opt_ CEventArgs *pArgs,
    _In_ EVENTPFN pfnScriptCallback
    )
{
    if (!CUIElement::IsValidDelegate(hEvent.index))
    {
        return;
    }

    (pfnScriptCallback)(
        m_pControl,
        NULL,
        hEvent,
        pSender,
        pArgs,
        IsSyncInputEvent(hEvent) ? EVENT_SYNC_INPUT : 0,
        NULL,
        CControl::Delegates[static_cast<XUINT32>(hEvent.index) - 1 /* UnknownEvent */]);
}

//------------------------------------------------------------------------------------
//
//  Inline Method:   CEventManager::RaiseStaticEvents
//
//  Synopsis:
//      For FocusManager we will directly call the event handlers on the FocusManager.
//-------------------------------------------------------------------------------------
inline void CEventManager::RaiseStaticEvents(
    _In_ EventHandle hEvent,
    _In_opt_ CEventArgs *pArgs,
    _In_ EVENTPFN pfnScriptCallback
)
{
    ASSERT(IsValidStaticEventDelegate(hEvent));
    (pfnScriptCallback)(
        m_pControl,
        NULL,
        hEvent,
        nullptr, /*pSender, passing null because this is a static event*/
        pArgs,
        EVENT_ASYNC_STATIC,
        NULL,
        NULL /*handler*/);
}

inline bool CEventManager::IsValidStaticEventDelegate(_In_ EventHandle hEvent)
{

    return (hEvent.index == KnownEventIndex::FocusManager_LostFocus
        || hEvent.index == KnownEventIndex::FocusManager_GotFocus);

}
//------------------------------------------------------------------------
//
//  Method:   CEventManager::RaiseRoutedEvent
//
//  Synopsis:
//          Raises the event on elements up the tree or down the tree
//          depending on routing strategy.
//
//------------------------------------------------------------------------

bool
CEventManager::RaiseRoutedEvent(
    _In_ EventHandle hEvent,
    _In_ CDependencyObject *pSource,
    _In_ CRoutedEventArgs *pArgs,
    _In_ bool bIgnoreVisibility,
    _In_ bool fRaiseSync,
    _In_ bool fInputEvent,
    _In_opt_ CDependencyObject* coerceToHandledAtElement,
    _In_ RoutingStrategy strategy)
{
    switch (strategy)
    {
    case RoutingStrategy::Bubbling:
        return RaiseRoutedEventBubbling(
            hEvent,
            pSource,
            pArgs,
            bIgnoreVisibility,
            fRaiseSync,
            fInputEvent,
            coerceToHandledAtElement);
        break;
    case RoutingStrategy::Tunnelling:
        //Tunnelling has no concept of coercing to handled
        ASSERT(coerceToHandledAtElement == nullptr);
          RaiseRoutedEventTunnelling(
            hEvent,
            pSource,
            pArgs,
            bIgnoreVisibility,
            fRaiseSync,
            fInputEvent);
        break;
    default:
        IFCFAILFAST(E_INVALIDARG);
    }

    return false;
}


//------------------------------------------------------------------------
//
//  Method:   CEventManager::RaiseRoutedEventBubbling
//
//  Synopsis:
//      Scan the array of requests and call those register for the specified
//  event.It bubbles the event through all parents of the event. Please note
//  this uses hit-testing semantics to detect if the event needs to go up the
//  chain. When we have keyboard semantics we should consolidate to a common check
//
//------------------------------------------------------------------------
bool CEventManager::RaiseRoutedEventBubbling(
    _In_ EventHandle hEvent,
    _In_opt_ CDependencyObject* pSource,
    _In_opt_ CRoutedEventArgs* pArgs,
    _In_ bool bIgnoreVisibility,
    _In_ bool fRaiseSync,
    _In_ bool fInputEvent,
    _In_opt_ CDependencyObject* coerceToHandledAtElement)
{
    if (!pSource || !pArgs)
    {
        return false;
    }

    // Peg the args object to avoid re-creating the managed peer
    // for each object we raise an event on.

    if (FAILED(pArgs->PegManagedPeerForRoutedEventArgs()))
    {
        return false;
    }

    auto guard = wil::scope_exit([&pArgs]()
    {
        pArgs->UnpegManagedPeerForRoutedEventArgs();
    });

    const bool isKeyDown = hEvent == KnownEventIndex::UIElement_KeyDown;
    DirectUI::FocusNavigationDirection focusDirection = DirectUI::FocusNavigationDirection::None;

    if (isKeyDown)
    {
        focusDirection = Focus::FocusSelection::GetNavigationDirectionForKeyboardArrow(static_cast<CKeyEventArgs*>(pArgs)->m_originalKeyCode);
    }

    bool focusCandidateFound = false;
    bool directionalFocusEnabled = false;
    bool shouldProcessDirectionalFocus = true;
    bool wasEventCoercedToHandled = false;
    CDependencyObject *pVisual = pSource;
    CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(pVisual);

    while (pVisual && !pVisual->IsDestructing())
    {
        CUIElement *pUIE = do_pointer_cast<CUIElement>(pVisual);

        if (bIgnoreVisibility || (pUIE && pUIE->IsHitTestVisible()))
        {
            Raise(hEvent, TRUE, pVisual, pArgs, fRaiseSync, fInputEvent);
        }

        if (coerceToHandledAtElement == pVisual)
        {
            // There are cases (e.g. mouse mode and engagement) where we don't want events to route all the way to the
            // top since they might do things that conflict the normal operation of the feature.  In this
            // case we will mark the event as handled automatically when we reach the element.  This is
            // the equivalent of adding an event handler that does nothing other than mark the event as
            // handled, but without all the overhead.
            wasEventCoercedToHandled = pArgs->m_bHandled == FALSE;
            pArgs->m_bHandled = TRUE;
        }

        // We want to attempt to change focus with arrow keys when:
        // 1) When the current element is not equal to the source.
        // 2) When the event is a KeyDown
        // 3) When the event has yet to be handled.
        // 4) The KeyDown originated from an arrow key
        // Note: if we do find a candidate to move focus to, we will set handled to true
        if (shouldProcessDirectionalFocus
            && pSource != pVisual
            && focusDirection != DirectUI::FocusNavigationDirection::None
            && pArgs->m_bHandled == false
            && isKeyDown)
        {
            const auto& directionalFocusInfo = Focus::FocusSelection::TryDirectionalFocus(focusManager, focusDirection, pVisual);
            pArgs->m_bHandled = directionalFocusInfo.handled;
            shouldProcessDirectionalFocus = directionalFocusInfo.shouldBubble;
            focusCandidateFound |= directionalFocusInfo.focusCandidateFound;
            directionalFocusEnabled |= directionalFocusInfo.directionalFocusEnabled;
        }

        CDependencyObject* parentVisual = nullptr;
        IFCFAILFAST(GetParentForRoutedEventBubbling(hEvent, pVisual, &parentVisual));
        pVisual = parentVisual;
    }

    //Only raise NoFocusCandidateFound if XYDirectionalFocus was ever set to enabled and a
    // focus candidate was never found.
    if (directionalFocusEnabled && !focusCandidateFound)
    {
        focusManager->RaiseNoFocusCandidateFoundEvent(focusDirection);
    }

    return wasEventCoercedToHandled;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::RaiseRoutedEventTunnelling
//
//  Synopsis:
//      Scan the array of requests and call those register for the specified
//  event. It tunnels the event down the tree to the event source. Please note
//  this uses hit-testing semantics to determine the routing path.
//------------------------------------------------------------------------

void CEventManager::RaiseRoutedEventTunnelling(
    _In_ EventHandle hEvent,
    _In_ CDependencyObject *pSource,
    _In_ CRoutedEventArgs *pArgs,
    _In_ bool bIgnoreVisibility,
    _In_ bool fRaiseSync,
    _In_ bool fInputEvent)
{
    ASSERT(pArgs != nullptr);

    m_topDownPathToSource.reserve(50);

    CDependencyObject *pVisual = pSource;
    bool bPegged = false;

    auto scopeGuard = wil::scope_exit([this, pArgs, &bPegged]
    {
        if (bPegged)
        {
            pArgs->UnpegManagedPeerForRoutedEventArgs();
        }

        this->m_topDownPathToSource.clear();
    });

    while (pVisual)
    {
        xref_ptr<CUIElement> pUIE(do_pointer_cast<CUIElement>(pVisual));

        // Since Tunnelling events are only defined on UIElements, only add these to the m_topDownPathToSource
        if (pUIE)
        {
            m_topDownPathToSource.push_back(pUIE);
        }

        CDependencyObject* parentVisual = nullptr;
        IFCFAILFAST(GetParentForRoutedEventBubbling(hEvent, pVisual, &parentVisual));
        pVisual = parentVisual;
    }

    // Peg the args object to avoid re-creating the managed peer
    // for each object on which we raise an event.
    bPegged = SUCCEEDED(pArgs->PegManagedPeerForRoutedEventArgs());

    if (bPegged)
    {
        for (auto it = m_topDownPathToSource.rbegin(); it != m_topDownPathToSource.rend(); ++it)
        {
            xref_ptr<CUIElement>& currentElement = *it;
            if (bIgnoreVisibility || ((currentElement != nullptr) && currentElement->IsHitTestVisible()))
            {
                Raise(hEvent, TRUE /*bRefire*/, currentElement.get(), pArgs, fRaiseSync, fInputEvent);
            }
        }
    }
}

bool CEventManager::IsRegisteredForEvent(_In_ CDependencyObject *pListener, _In_ EventHandle hEvent)
{
    CRequestsForObjectList* pRegisteredRequests = nullptr;

    if (SUCCEEDED(m_pRequest->Get(pListener, pRegisteredRequests)))
    {
        if (pRegisteredRequests)
        {
            XUINT32 uRequestCtr = 0;
            while (uRequestCtr < pRegisteredRequests->size())
            {
                REQUEST* pNodeRequest = NULL;
                VERIFYHR(pRegisteredRequests->get_item(uRequestCtr, pNodeRequest));

                if (pNodeRequest && pNodeRequest->m_hEvent == hEvent)
                {
                    return true;
                }
                ++uRequestCtr;
            }
        }
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Post a message to the threadsafe queue to trigger an event from
//      any thread.
//
//  Notes:
//      Ideally everything would use the fast path, but there seem to be various timing
//      assumptions throughout the codebase that prevent us from doing that right now.
//      For examples, several drts assume that an event will be raised after the next tick.
//      For now, callers should opt-in to using the fast path whenever possible.
//
//------------------------------------------------------------------------
void
CEventManager::ThreadSafeRaise(
    _In_ EventHandle hEvent,
    _In_ XINT32 bRefire,
    _In_opt_ CDependencyObject *pSender,
    _Inout_opt_ CEventArgs **ppArgs,
    _In_ bool fAllowErrorFallback,
    _In_ bool fUseFastRaise
    )
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    // Assemble an arguments object
    RaiseArguments *pRaiseArgs = new RaiseArguments(hEvent, bRefire, pSender, ppArgs, fAllowErrorFallback);

    // EventArgs lifetime has been handed off to the RaiseArguments
    // so we clear the pp so the caller can Release safely in both success
    // and error cases.  This asserts that RaiseArguments did the right thing
    ASSERT((ppArgs == NULL) || (*ppArgs == NULL));

    // Post the arguments object to one of the threadsafe queues.
    if (fUseFastRaise)
    {
        ASSERT(NULL != m_pFastQueue);
        m_pFastQueue->Post(reinterpret_cast<void*>(pRaiseArgs));
        if (m_pCore)
        {
            IGNOREHR(m_pCore->ExecuteOnUIThread(this, ReentrancyBehavior::CrashOnReentrancy));
        }

    }
    else
    {
        ASSERT(NULL != m_pSlowQueue);
        m_pSlowQueue->Post(reinterpret_cast<void*>(pRaiseArgs));

        // TODO: TICK: It seems unfortunate that we have to tick to raise events with no listeners.
        IFC(RequestAdditionalFrame());
    }

    pRaiseArgs = NULL;

Cleanup:
    delete pRaiseArgs;
    return;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::RaiseCallback
//
//  Synopsis:
//      Scan the array of requests and call those register for the specified
//  event.
//  Sender is the current element to fire the event on, Source is the element that initiated this
//------------------------------------------------------------------------

void
CEventManager::RaiseCallback(
    _In_ CEventArgs *pArgs)
{
    if (m_pfnScriptCallbackAsync && pArgs)
    {
        // Schedule an asynchronous call to delegate of internal event listener
        (m_pfnScriptCallbackAsync)(
            m_pControl,
            NULL,
            EventHandle(KnownEventIndex::UnknownType_UnknownEvent),
            m_pCore->getVisualRoot() /* pSender */,
            pArgs,
            0,
            NULL,
            &CCoreServices::CallbackEventListener);
    }
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::Execute
//
//  Synopsis:
//      IPALExecuteOnUIThread::Execute callback.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEventManager::Execute()
{
    RRETURN(ProcessQueue(TRUE /* fProcessFastQueue */));
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::ProcessQueue
//
//  Synopsis:
//      Process the queue that ThreadSafeRaise uses calling Raise for
//      each available set of arguments
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEventManager::ProcessQueue(_In_opt_ bool fProcessFastQueue)
{
    if (!fProcessFastQueue)
    {
        ASSERT(NULL != m_pSlowQueue);
        IFC_RETURN(ProcessQueueImpl(m_pSlowQueue));
    }

    ASSERT(NULL != m_pFastQueue);
    IFC_RETURN(ProcessQueueImpl(m_pFastQueue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::ProcessQueueImpl
//
//  Synopsis:
//      Process the specified queue, calling Raise for each available set
//      of arguments
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEventManager::ProcessQueueImpl(_In_ IPALQueue *pQueue)
{
    RaiseArguments *pArgs = NULL;

    // If queue has objects, process them
    // If the queue timeouts, it will not
    // return S_OK and the loop will break
    while (SUCCEEDED(pQueue->Get((void**)&pArgs, 0)))
    {
        IFCPTR_RETURN(pArgs);

        if (pArgs->m_hEvent.index == KnownEventIndex::DependencyObject_RaiseAsyncCallback)
        {
            RaiseCallback(pArgs->m_pArgs);
        }
        else
        {
            Raise(
                pArgs->m_hEvent,
                !!pArgs->m_bRefire,
                pArgs->m_pSender,
                pArgs->m_pArgs,
                false,
                false,
                !!pArgs->m_fAllowErrorFallback);
        }

        delete pArgs;
        pArgs = NULL;
    }

    return S_OK;
}

///
/// Static methods
///

_Check_return_
HRESULT
CEventManager::AddEventListener(
_In_ CDependencyObject *pDO,
_In_ CXcpList<REQUEST> **pEventList,
_In_ EventHandle hEvent,
_In_ CValue *pValue,
_In_ XINT32 iListenerType,
_Out_opt_ CValue *pResult,
_In_ bool fHandledEventsToo,
_In_ bool fSkipIsActiveCheck)
{
    REQUEST *p = NULL;
    XINT32 iToken = 0;
    CEventManager* pEventManager = NULL;

    IFCPTR_RETURN(pDO);

    // Some managed peers, such as Timelines, don't need to be protected from GC until
    // they have meaningful managed state.  For such types, attaching a managed event listener
    // is meaningful state, so mark it to require protection.

    if (pDO->HasManagedPeer()
        && iListenerType == EVENTLISTENER_CLR)
    {
        IFC_RETURN(pDO->SetParticipatesInManagedTreeDefault());
    }

    // Obtain an instance of CEventManager matching the DependencyObject under discussion.
    pEventManager = pDO->GetContext()->GetEventManager();

    // Determine the token value appropriate to the listener type.
    if (iListenerType == EVENTLISTENER_CLR)
    {
        // Same token value is used for all listeners attached via the CLR.
        iToken = REQUEST_CLR;
    }
    else if (iListenerType == EVENTLISTENER_INTERNAL)
    {
        iToken = REQUEST_INTERNAL;
    }
    else
    {
        // Not a known listener type - unable to assign a token until this code
        //  knows what to do with this listener type.
        IFC_RETURN(E_FAIL);
    }

    // Create a new REQUEST to store information about the new event listener.
    IFC_RETURN(pEventManager->CreateRequest(&p, hEvent, pDO, pValue, iToken, NULL, fHandledEventsToo));

    IFCEXPECT_RETURN(p);

    if (*pEventList == NULL)
    {
        *pEventList = new CXcpList<REQUEST>;
    }
    // Add it to our list of Events...
    (*pEventList)->Add(p);

    fSkipIsActiveCheck |= pDO->AllowsHandlerWhenNotLive(iListenerType, hEvent.index);

    //popup does not need to be in the tree for its events to be fired.
    //adding a one off special case. for any more such types, consider adding
    //a flag to metadata instead. there is a corresponding check in RemoveEventListener.

    // We should add a request for this listener if the DO currently has requests
    if (pDO->IsFiringEvents() || fSkipIsActiveCheck)
    {
        // if we are active add it to the EventManager
        IFC_RETURN(pEventManager->AddRequest(pDO, p));

        if (fSkipIsActiveCheck)
        {
            p->m_bAdded = TRUE;
        }
    }

    // Copy the REQUEST token value when requested.
    if (NULL != pResult)
    {
        pResult->SetSigned(iToken);
    }

    return S_OK;
}

_Check_return_
HRESULT
CEventManager::RemoveEventListener(
    _In_ CDependencyObject *pDO,
    _In_ CXcpList<REQUEST> *pEventList,
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ bool fSkipIsActiveCheck)
{
    CXcpList<REQUEST>::XCPListNode *pTemp = NULL;

    IFCPTR_RETURN(pDO);

    // Remove this from our internal list...
    if (pEventList == NULL)
    {
        // Trying to remove something that isn't there is treated as a success.
        return S_OK;
    }

    // Find the right one
    pTemp = pEventList->GetHead();

    while (pTemp)
    {
        REQUEST * pRequest = (REQUEST *)pTemp->m_pData;
        IFCEXPECT_ASSERT_RETURN(pRequest);

        if( pRequest->m_hEvent == hEvent)
        {
            if (pValue->GetType() == valueSigned)
            {
                // Removal specified by token value.  Code upstream is responsible for
                //  verifying that the value is valid here.  (For example, negative
                //  values may not be passed in via browser script APIs.)
                if (pValue->AsSigned() == pRequest->m_iToken)
                {
                    // We should remove a request for this listener if the DO currently has requests
                    if (pDO->IsFiringEvents())
                    {
                        IFC_RETURN(pDO->GetContext()->GetEventManager()->RemoveRequest(pDO, pRequest));
                    }

                    pEventList->Remove(pRequest);
                    break;
                }
            }
            else if (pRequest->m_pListener == pValue->AsObject())
            {
                // RS5 Bug #17784006:
                // The policy to not call CEventManager::RemoveRequest() if pDO->IsFiringEvents() returns false will leak
                // the REQUEST object for this DO and along with it two strong references on the DO, causing the DO to leak as well.
                // The fix is to always call RemoveRequest() to avoid this memory leak.
                // Note that the other blocks of code in this function continue to check for IsFiringEvents(), to reduce risk.
                if (pDO->GetContext()->GetEventManager() != nullptr)
                {
                    IFC_RETURN(pDO->GetContext()->GetEventManager()->RemoveRequest(pDO, pRequest));
                }
                pEventList->Remove(pRequest);
                break;
            }
            else if (pRequest->m_pfnInternalEventDelegate && pValue->AsInternalHandler())
            {
                // This REQUEST was placed from internal code to call back to a
                //  static method pointer.
                if (pValue->AsInternalHandler() == pRequest->m_pfnInternalEventDelegate)
                {
                    // We should remove a request for this listener if the DO currently has requests
                    if (pDO->IsFiringEvents() || pDO->OfTypeByIndex<KnownTypeIndex::RootVisual>())
                    {
                        IFC_RETURN(pDO->GetContext()->GetEventManager()->RemoveRequest(pDO, pRequest));
                    }

                    pEventList->Remove(pRequest);
                    break;
                }
            }
        }
        pTemp = pTemp->m_pNext;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CEventManager::AddToLoadedEventList
//
//  Synopsis:
//      Add to list of objects on which Loaded event is yet to be fired.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CEventManager::AddToLoadedEventList(_In_ CDependencyObject* pElement)
{
    IFCPTR_RETURN(pElement);

    if(!m_pLoadedEventList)
    {
        m_pLoadedEventList = new CDependencyObjectVector();
    }
    IFC_RETURN(m_pLoadedEventList->push_back(pElement));
    AddRefInterface(pElement);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::RemoveFromLoadedEventList
//
//  Synopsis:
//      Remove from list of objects on which Loaded event is yet to be fired.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CEventManager::RemoveFromLoadedEventList(_In_ CDependencyObject* pElement, _Inout_opt_ bool *pLoadedEventRemoved)
{
    IFCPTR_RETURN(pElement);

    if (pLoadedEventRemoved)
    {
        *pLoadedEventRemoved = false;
    }

    if (m_pLoadedEventList)
    {
        XUINT32 uObjCtr = 0;
        while (uObjCtr < m_pLoadedEventList->size())
        {
            xref_ptr<CDependencyObject> spCurObject;
            CDependencyObject *pCurObject = nullptr;
            IFC_RETURN(m_pLoadedEventList->get_item(uObjCtr, pCurObject));
            if (pCurObject == pElement)
            {
                spCurObject.attach(pCurObject);
                IFC_RETURN(m_pLoadedEventList->erase(uObjCtr));
                if (pLoadedEventRemoved)
                {
                    *pLoadedEventRemoved = true;
                }
            }
            else
            {
                uObjCtr++;
            }
        }
    }

    return S_OK;
}

bool CEventManager::IsLoadedEventPending(_In_ CDependencyObject* pElement)
{
    if (m_pLoadedEventList)
    {
        XUINT32 uObjCtr = 0;
        while (uObjCtr < m_pLoadedEventList->size())
        {
            CDependencyObject* pCurObject = nullptr;
            IFCFAILFAST(m_pLoadedEventList->get_item(uObjCtr, pCurObject));
            if (pCurObject == pElement)
            {
                return true;
            }
            else
            {
                uObjCtr++;
            }
        }
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis: Schedules an additional tick. This is used when we add events
//            that are raised during the next tick.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEventManager::RequestAdditionalFrame()
{
    // The browser host and/or frame scheduler can be NULL during shutdown.
    IXcpBrowserHost *pBH = m_pCore->GetBrowserHost();
    if (pBH != NULL)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
        if (pFrameScheduler != NULL)
        {
            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::EventManager));
        }
    }

    return S_OK;
}

void CEventManager::TraceInputEventBegin(
    _In_ const CDependencyObject *pSender,
    _In_ CEventArgs *pInputArgs,
    _In_ const EventHandle &eventHandle)
{
    // The sender should never be null

    ASSERT(pSender);

    int keyCode = 0;
    int pointerType = 0;
    int manipulationMode = 0;
    bool isInertial = false;
    UINT64 container = 0;

    // Get information about the event. A lot of this is similar for each trace
    // so gather first before firing the event.
    auto pArgs = static_cast<CRoutedEventArgs*>(pInputArgs);

    auto senderAddress = reinterpret_cast<UINT64>(pSender);
    auto sourceAddress = pArgs ? reinterpret_cast<UINT64>(pArgs->m_pSource) : 0;

    CKeyEventArgs* keyArgs = nullptr;
    CPointerEventArgs *pointerArgs = nullptr;
    CTappedEventArgs *tappedArgs = nullptr;
    CManipulationStartingEventArgs* manipStartingArgs = nullptr;
    CManipulationEventArgs* manipArgs = nullptr;
    CManipulationDeltaEventArgs* manipDeltaArgs = nullptr;
    CManipulationCompletedEventArgs* manipCompletedArgs = nullptr;

    switch (eventHandle.index)
    {
    case KnownEventIndex::UIElement_KeyUp:
    case KnownEventIndex::UIElement_KeyDown:
        keyArgs = static_cast<CKeyEventArgs*>(pArgs);
        keyCode = keyArgs ? keyArgs->m_platformKeyCode : -1;
        break;
    case KnownEventIndex::UIElement_PointerPressed:
    case KnownEventIndex::UIElement_PointerMoved:
    case KnownEventIndex::UIElement_PointerReleased:
    case KnownEventIndex::UIElement_PointerEntered:
    case KnownEventIndex::UIElement_PointerExited:
    case KnownEventIndex::UIElement_PointerCaptureLost:
    case KnownEventIndex::UIElement_PointerCanceled:
    case KnownEventIndex::UIElement_PointerWheelChanged:
        pointerArgs = static_cast<CPointerEventArgs*>(pArgs);
        pointerType = pointerArgs ? static_cast<XUINT32>(pointerArgs->m_pPointer->m_pointerDeviceType) : -1;
        break;
    case KnownEventIndex::UIElement_Tapped:
    case KnownEventIndex::UIElement_DoubleTapped:
    case KnownEventIndex::UIElement_Holding:
    case KnownEventIndex::UIElement_RightTapped:
        tappedArgs = static_cast<CTappedEventArgs*>(pArgs);
        pointerType = tappedArgs ? static_cast<XUINT32>(tappedArgs->m_pointerDeviceType) : -1;
        break;
    case KnownEventIndex::UIElement_ManipulationStarting:
        manipStartingArgs = static_cast<CManipulationStartingEventArgs*>(pArgs);
        container = manipStartingArgs ? reinterpret_cast<XUINT64>(manipStartingArgs->m_pManipulationContainer) : 0;
        manipulationMode = manipStartingArgs ?  static_cast<int>(manipStartingArgs->m_uiManipulationMode) : -1;
        break;
    case KnownEventIndex::UIElement_ManipulationStarted:
    case KnownEventIndex::UIElement_ManipulationInertiaStarting:
        manipArgs = static_cast<CManipulationEventArgs*>(pArgs);
        container = manipArgs ? reinterpret_cast<XUINT64>(manipArgs->m_pManipulationContainer) : 0;
        break;
    case KnownEventIndex::UIElement_ManipulationDelta:
        manipDeltaArgs = static_cast<CManipulationDeltaEventArgs*>(pArgs);
        container = manipDeltaArgs ? reinterpret_cast<XUINT64>(manipDeltaArgs->m_pManipulationContainer) : 0;
        isInertial = manipDeltaArgs ? manipDeltaArgs->m_bInertial != 0 : false;
        break;
    case KnownEventIndex::UIElement_ManipulationCompleted:
        manipCompletedArgs = static_cast<CManipulationCompletedEventArgs*>(pArgs);
        container = manipCompletedArgs ? reinterpret_cast<XUINT64>(manipCompletedArgs->m_pManipulationContainer) : 0;
        isInertial = manipCompletedArgs ? manipCompletedArgs->m_bInertial != 0 : false;
        break;
    // Don't need more information for drag events. Here to show they weren't forgotten
    case KnownEventIndex::UIElement_DragEnter:
    case KnownEventIndex::UIElement_DragLeave:
    case KnownEventIndex::UIElement_DragOver:
    case KnownEventIndex::UIElement_Drop:
    default:
        break;
    }

    switch (eventHandle.index)
    {
    case KnownEventIndex::UIElement_KeyUp:
        TraceKeyUpHandlerBegin(senderAddress, keyCode, sourceAddress);
        break;
    case KnownEventIndex::UIElement_KeyDown:
        TraceKeyDownHandlerBegin(senderAddress, keyCode, sourceAddress);
        break;
    case KnownEventIndex::UIElement_PointerPressed:
        TracePointerPressedBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_PointerMoved:
        TracePointerMovedBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_PointerReleased:
        TracePointerReleasedBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_PointerEntered:
        TracePointerEnteredBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_PointerExited:
        TracePointerExitedBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_PointerCaptureLost:
        TracePointerCaptureLostBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_PointerCanceled:
        TracePointerCanceledBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_PointerWheelChanged:
        TracePointerWheelChangedBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_DragEnter:
        TraceDragEnterBegin(senderAddress, sourceAddress);
        break;
    case KnownEventIndex::UIElement_DragLeave:
        TraceDragLeaveBegin(senderAddress, sourceAddress);
        break;
    case KnownEventIndex::UIElement_DragOver:
        TraceDragOverBegin(senderAddress, sourceAddress);
        break;
    case KnownEventIndex::UIElement_Drop:
        TraceDropBegin(senderAddress, sourceAddress);
        break;
    case KnownEventIndex::UIElement_Tapped:
        TraceTappedBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_DoubleTapped:
        TraceDoubleTappedBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_Holding:
        TraceHoldingBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_RightTapped:
        TraceRightTappedBegin(senderAddress, pointerType, sourceAddress);
        break;
    case KnownEventIndex::UIElement_ManipulationStarting:
        TraceManipulationStartingBegin(senderAddress, sourceAddress, container, manipulationMode);
        break;
    case KnownEventIndex::UIElement_ManipulationStarted:
        TraceManipulationStartedBegin(senderAddress, sourceAddress, container);
        break;
    case KnownEventIndex::UIElement_ManipulationInertiaStarting:
        TraceManipulationInertiaStartingBegin(senderAddress, sourceAddress, container);
        break;
    case KnownEventIndex::UIElement_ManipulationDelta:
        TraceManipulationDeltaBegin(senderAddress, sourceAddress, container, isInertial);
        break;
    case KnownEventIndex::UIElement_ManipulationCompleted:
        TraceManipulationCompletedBegin(senderAddress, sourceAddress, container, isInertial);
    default:
        break;
    }
}
void CEventManager::TraceInputEventEnd(
    _In_ CEventArgs *pInputArgs,
    _In_ const EventHandle &eventHandle)
{
    if (!pInputArgs)
    {
        return;
    }

    auto pArgs = static_cast<CRoutedEventArgs*>(pInputArgs);
    auto handled = pArgs ? pArgs->m_bHandled : FALSE;


    switch (eventHandle.index)
    {
    case KnownEventIndex::UIElement_KeyUp:
        TraceKeyUpHandlerEnd(handled);
        break;
    case KnownEventIndex::UIElement_KeyDown:
        TraceKeyDownHandlerEnd(handled);
        break;
    case KnownEventIndex::UIElement_PointerPressed:
        TracePointerPressedEnd(handled);
        break;
    case KnownEventIndex::UIElement_PointerMoved:
        TracePointerMovedEnd(handled);
        break;
    case KnownEventIndex::UIElement_PointerReleased:
        TracePointerReleasedEnd(handled);
        break;
    case KnownEventIndex::UIElement_PointerEntered:
        TracePointerEnteredEnd(handled);
        break;
    case KnownEventIndex::UIElement_PointerExited:
        TracePointerExitedEnd(handled);
        break;
    case KnownEventIndex::UIElement_PointerCaptureLost:
        TracePointerCaptureLostEnd(handled);
        break;
    case KnownEventIndex::UIElement_PointerCanceled:
        TracePointerCanceledEnd(handled);
        break;
    case KnownEventIndex::UIElement_PointerWheelChanged:
        TracePointerWheelChangedEnd(handled);
        break;
    case KnownEventIndex::UIElement_DragEnter:
        TraceDragEnterEnd(handled);
        break;
    case KnownEventIndex::UIElement_DragLeave:
        TraceDragLeaveEnd(handled);
        break;
    case KnownEventIndex::UIElement_DragOver:
        TraceDragOverEnd(handled);
        break;
    case KnownEventIndex::UIElement_Drop:
        TraceDropEnd(handled);
        break;
    case KnownEventIndex::UIElement_Tapped:
        TraceTappedEnd(handled);
        break;
    case KnownEventIndex::UIElement_DoubleTapped:
        TraceDoubleTappedEnd(handled);
        break;
    case KnownEventIndex::UIElement_Holding:
        TraceHoldingEnd(handled);
        break;
    case KnownEventIndex::UIElement_RightTapped:
        TraceRightTappedEnd(handled);
        break;
    case KnownEventIndex::UIElement_ManipulationStarting:
        TraceManipulationStartingEnd(handled);
        break;
    case KnownEventIndex::UIElement_ManipulationStarted:
        TraceManipulationStartedEnd(handled);
        break;
    case KnownEventIndex::UIElement_ManipulationInertiaStarting:
        TraceManipulationInertiaStartingEnd(handled);
        break;
    case KnownEventIndex::UIElement_ManipulationDelta:
        TraceManipulationDeltaEnd(handled);
        break;
    case KnownEventIndex::UIElement_ManipulationCompleted:
        TraceManipulationCompletedEnd(handled);
        break;
    default:
        break;
    }
}

_Check_return_ HRESULT CEventManager::GetParentForRoutedEventBubbling(EventHandle hEvent, _In_ CDependencyObject* currentObject, _In_ CDependencyObject** parentObject)
{
    CDependencyObject* parentObjectLocal = nullptr;

    // In case of Key events, we routed the key events from the popup root to the visual root that
    // supports the invoking the key event handler on the visual root.
    // Alternatively, if we're a flyout presenter that's been opened in transitive mode,
    // we want to route keyboard and character events from there up to the flyout's target
    // in order to allow users to interact with both the flyout and the target
    // with the keyboard so the flyout doesn't get in the user's way.
    const bool isKeyEvent = IsKeyEvent(hEvent);

    CPopupRoot* popupRootNoRef = nullptr;

    if (isKeyEvent && SUCCEEDED(VisualTree::GetPopupRootForElementNoRef(currentObject, &popupRootNoRef)) && currentObject == popupRootNoRef)
    {
        parentObjectLocal = currentObject->GetPublicRootVisual();
        // if window chrome the is top most element on a visual tree, return its content as
        // the real root element of the visual tree
        // TODO: Task 38535248
        if (parentObjectLocal->OfTypeByIndex<KnownTypeIndex::WindowChrome>())
        {
            CContentControl* pWindowChrome = do_pointer_cast<CContentControl>(parentObjectLocal);
            CValue windowContent;
            IFC_RETURN(pWindowChrome->Content(pWindowChrome, 0, nullptr, nullptr, &windowContent));
            parentObjectLocal = windowContent.AsObject();
        }
    }
    else if ((isKeyEvent || hEvent == KnownEventIndex::UIElement_CharacterReceived) && currentObject->OfTypeByIndex<KnownTypeIndex::FlyoutPresenter>())
    {
        IFC_RETURN(FxCallbacks::FlyoutPresenter_GetTargetIfOpenedAsTransient(currentObject, &parentObjectLocal));
    }

    if (!parentObjectLocal)
    {
        // There are scenarios, like Hyperlink, where there is a non public parent. If we only limit ourselves to this,
        // we will not fully bubble up events. Therefore, we explicitly get all parents, whether it is public or non-public
        parentObjectLocal = currentObject->GetParentInternal(/*fPublic*/ false);
    }

    *parentObject = parentObjectLocal;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  ctor
//
//------------------------------------------------------------------------
CEventManager::RaiseArguments::RaiseArguments(
    EventHandle hEvent,
    XINT32 bRefire,
    _In_opt_ CDependencyObject *pSender,
    _Inout_opt_ CEventArgs **ppArgs,
    _In_ bool fAllowErrorFallback
    )
{
    m_hEvent = hEvent;
    m_bRefire = bRefire;
    m_pSender = pSender;
    m_fAllowErrorFallback = fAllowErrorFallback;
    AddRefInterface(m_pSender);

    // We're handing off EventArgs to the other thread,
    // so we don't AddRef it in the caller
    if (ppArgs)
    {
        m_pArgs = *ppArgs;
        // NULL out the args to indicate the Ref
        // is now owned by these RaiseArguments
        *ppArgs = NULL;
    }
    else
    {
        m_pArgs = NULL;
    }
}

//------------------------------------------------------------------------
//
//  dtor
//
//------------------------------------------------------------------------
CEventManager::RaiseArguments::~RaiseArguments()
{
    ReleaseInterface(m_pSender);
    ReleaseInterface(m_pArgs);
}