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
#include "XamlTelemetry.h"

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

    while (!m_loadedEventObjects.empty())
    {
        CDependencyObjectVector currentLoadedEventObjects;
        std::swap(currentLoadedEventObjects, m_loadedEventObjects);
    }

    IFC(FlushQueue());

Cleanup:
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
//  Method:   CEventManager::EnableEvents
//
//  Synopsis:
//      Walks through the list of events and enables events for the given object.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CEventManager::EnableEvents(
    _In_ CDependencyObject* pObject,
    _In_ std::vector<REQUEST>* pEventList)
{
    ASSERT(pObject && pEventList);

    // The events should be fired in this order:
    // -----------------------------------------
    // (1) Events registered in the constructor.
    // (2) Events hooked up in XAML.
    // (3) Elsewhere! (Page_Loaded, LayoutUpdated, ...etc), using the "+=" syntax.
    // Events are appended to the vector via push_back, so iterating
    // forward gives us the correct registration order.

    for (auto& request : *pEventList)
    {
        AddToLoadedEventListIfNeeded(pObject, request.m_hEvent);
        request.m_bActive = TRUE;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::DisableEvents
//
//  Synopsis:
//      Walks through the list of events and disables events for the given object.
//
//      This is intended to be used as a mirror image to EnableEvents()
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CEventManager::DisableEvents(
    _In_ CDependencyObject* pObject,
    _In_ std::vector<REQUEST>* pEventList)
{
    bool removedLoadedEvent = false;
    bool hasLoadedEvent = false;

    for (auto& request : *pEventList)
    {
        if (IsLoadedEvent(request.m_hEvent))
        {
            if (request.m_bCanFireWhenInactive)
            {
                hasLoadedEvent = true;
            }
            else if (request.m_bActive)
            {
                removedLoadedEvent = true;
            }
        }

        if (!request.m_bCanFireWhenInactive)
        {
            request.m_bActive = false;
        }

        // If/When it becomes reenabled, the event can fire again
        request.m_bFired = FALSE;
    }

    if (removedLoadedEvent && !hasLoadedEvent)
    {
        RemoveFromLoadedEventList(pObject);
    }

    return S_OK;
}

_Check_return_ HRESULT CEventManager::RaiseLoadedEventForObject(_In_ CDependencyObject* pLoadedEventObject, _In_ CEventArgs* loadedArgs)
{
    TraceRaiseLoadedEventBegin((UINT64)pLoadedEventObject);

    TraceLoggingProviderWrite(
        XamlTelemetry, "EventManager_RaiseLoadedEvent",
        TraceLoggingBoolean(true, "IsStart"),
        TraceLoggingUInt32(pLoadedEventObject->GetContext()->GetFrameNumber(), "FrameNumber"),
        TraceLoggingUInt64(reinterpret_cast<uint64_t>(pLoadedEventObject), "ObjectPointer"),
        TraceLoggingWideString(pLoadedEventObject->OfTypeByIndex<KnownTypeIndex::FrameworkElement>() ? static_cast<CFrameworkElement*>(pLoadedEventObject)->GetStrClassName().GetBuffer() : nullptr, "ClassName"),
        TraceLoggingWideString(pLoadedEventObject->m_strName.GetBuffer(), "Name"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    auto scopeGuard = wil::scope_exit([pLoadedEventObject]
    {
        TraceRaiseLoadedEventEnd();

        TraceLoggingProviderWrite(
            XamlTelemetry, "EventManager_RaiseLoadedEvent",
            TraceLoggingBoolean(false, "IsStart"),
            TraceLoggingUInt64(reinterpret_cast<uint64_t>(pLoadedEventObject), "ObjectPointer"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
    });

    auto handlers = pLoadedEventObject->GetEventHandlers();
    if (!handlers.empty())
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

        // Make a copy of the handlers to be invoked because the callback can cause reentrancy which can
        // mutate the event list on the DO itself. Pre-allocate 2 items in the match collection,
        // assuming we'll find an internal and a CLR handler.
        TempRequests matches;
        FilterEligibleHandlers(handlers, KnownEventIndex::FrameworkElement_Loaded, false /*refire*/, &matches);

        if (!matches.empty())
        {
            IFC_RETURN(RaiseHelper(
                matches,
                pLoadedEventObject,         // pSender
                loadedArgs,
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

    // If m_loadedEventObjects is, no Loaded event listeners have been registered, so there's nothing to do.
    if (!m_fRaiseLoadedEventNeeded || m_loadedEventObjects.empty())
    {
        return S_OK;
    }

    // we must have a Core at this point - this check is needed because a call to Disable() can null it out
    if (!m_pCore)
    {
        IFC_RETURN(E_FAIL);
    }

    xref_ptr<CEventArgs> spLoadedArgs = make_xref<CRoutedEventArgs>();

    // Firing loaded events may add new objects to the loaded event list. To maintain historical
    // behavior, we must raise events on them synchronously, in the current invocation of RaiseLoadedEvent.
    // This loop continues until no new loaded event objects are added.
    while (!m_loadedEventObjects.empty())
    {
        CDependencyObjectVector currentLoadedEventObjects;
        std::swap(currentLoadedEventObjects, m_loadedEventObjects);

        // Raise the Loaded event on the current set of objects.
        for (auto& obj : currentLoadedEventObjects)
        {
            IFC_RETURN(RaiseLoadedEventForObject(obj.get(), spLoadedArgs.get()));
        }

        // Current batch of loaded event DOs released here.
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
    bool bFired = false;
    gsl::span<REQUEST> handlers;

    // Verify the sync input event.
    ASSERT((IsValidSyncInputEvent(hEvent, fInputEvent, fRaiseSync)) || !IsSyncInputEvent(hEvent));

    if (pSender)
    {
        if (pSender->GetContext()->IsShuttingDown())
        {
            // Do not raise events when shutting down.
            return;
        }
        if (!pSender->ShouldRaiseEvent(hEvent, fInputEvent, pArgs))
        {
            // Do not raise events when there are no listeners, unless there are implicit
            // event listeners, such as RichTextBlock::OnTapped.
            return;
        }

        handlers = pSender->GetEventHandlers();
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

        // If sender didn't provide any handlers, then ostensibly we've handled it as a
        // static/control/uielement event above.
        if (!handlers.empty())
        {
            // Make a copy of the handlers to be invoked because the callback can cause reentrancy which can
            // mutate the event list on the DO itself. Pre-allocate 2 items in the match collection,
            // assuming we'll find an internal and a CLR handler.
            TempRequests matches;
            FilterEligibleHandlers(handlers, hEvent, !!bRefire, &matches);

            if (!matches.empty())
            {
                IFC(RaiseHelper(matches, pSender, pArgs, pfnScriptCallback, bFired, pSenderOverride));
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

HRESULT CEventManager::RaiseHelper(const gsl::span<const REQUEST>& requests,
                                _In_ CDependencyObject *pSender,
                                CEventArgs *pArgs,
                                EVENTPFN pfnScriptCallback,
                                bool& bFired,
                                _In_opt_ CDependencyObject *pSenderOverride)
{
    bFired = FALSE;

    for (const auto& request : requests)
    {
#ifdef TRACE_EVENTS
        const auto* pSenderTypeName = pSenderOverride ? 
        DirectUI::MetadataAPI::GetClassInfoByIndex(pSenderOverride->GetTypeIndex())->GetFullName().GetBuffer() : 
            pSender ? DirectUI::MetadataAPI::GetClassInfoByIndex(pSender->GetTypeIndex())->GetFullName().GetBuffer() : L"null";

        TraceLoggingProviderWrite(
            XamlTelemetry, "CEventManager::RaiseHelper",
            TraceLoggingUInt32(static_cast<uint32_t>(request.m_hEvent.index), "EventIndex"),
            TraceLoggingUInt64(reinterpret_cast<uint64_t>(pSenderOverride ? pSenderOverride : pSender), "Sender Pointer"),
            TraceLoggingWideString(pSenderTypeName, "Sender TypeName"),
            TraceLoggingBoolean(true, "IsStart"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        auto endEvent = wil::scope_exit([request, pSender, pSenderOverride]
        {
            TraceLoggingProviderWrite(
                XamlTelemetry, "CEventManager::RaiseHelper",
                TraceLoggingUInt32(static_cast<uint32_t>(request.m_hEvent.index), "EventIndex"),
                TraceLoggingUInt64(reinterpret_cast<uint64_t>(pSenderOverride ? pSenderOverride : pSender), "Sender Pointer"),
                TraceLoggingBoolean(false, "IsStart"),
                TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
        });
#endif
        // here we checking assuming we are in the scenario 2.
        if (m_pfnScriptCallbackAsync)
        {
            XUINT32 flags = request.m_bHandledEventsToo ? EVENT_HANDLEDEVENTSTOO : 0;
                    flags |= IsSyncInputEvent(request.m_hEvent) ? EVENT_SYNC_INPUT : 0;
                    flags |= (IsLoadedEvent(request.m_hEvent) || IsLoadingEvent(request.m_hEvent)) ? EVENT_SYNC_LOADED : 0;

            if (request.m_pfnInternalEventDelegate != NULL)
            {
                // if we are here we don't need to check pfnScriptCallback
                // it will be at least m_pfnScriptCallbackAsync
                (pfnScriptCallback)(
                    m_pControl,
                    NULL,
                    request.m_hEvent,
                    pSender,
                    pArgs,
                    flags,
                    NULL,
                    request.m_pfnInternalEventDelegate);
            }
            else
            {
                bool isSenderPegged = false;

                // Some events, such as VisualStateChanging/Changed need to arrive with a different sender specified.
                // Callers can optionally set pSenderOverride to specify this different sender.
                CDependencyObject* pActualSender = (pSenderOverride != nullptr) ? pSenderOverride : pSender;

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
                        pSender,
                        request.m_hEvent,
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
            bFired = TRUE;
        }
   }

   return S_OK;
}

/*static*/
void CEventManager::FilterEligibleHandlers(
    const gsl::span<REQUEST>& handlers,
    EventHandle hEvent,
    bool bRefire,
    _Inout_ TempRequests* pMatches)
{
    for (auto& handler : handlers)
    {
        if (handler.m_bActive && (hEvent == handler.m_hEvent))
        {
            // Prevent double firing of events
            if (!handler.m_bFired || bRefire)
            {
                handler.m_bFired = TRUE;
                pMatches->push_back(handler);
            }
        }
    }
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
CEventManager::ProcessQueue(_In_ bool fProcessFastQueue)
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
    _Inout_ std::unique_ptr<std::vector<REQUEST>>& pEventList,
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _In_ bool fHandledEventsToo,
    _In_ bool fSkipIsActiveCheck)
{
    IFCPTR_RETURN(pDO);

    if ((iListenerType != EVENTLISTENER_CLR) && (iListenerType != EVENTLISTENER_INTERNAL))
    {
        IFC_RETURN(E_FAIL);
    }

    if ((pValue->GetType() != valueAny) &&
        (pValue->GetType() != valueInternalHandler))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // Some managed peers, such as Timelines, don't need to be protected from GC until
    // they have meaningful managed state.  For such types, attaching a managed event listener
    // is meaningful state, so mark it to require protection.

    if (pDO->HasManagedPeer()
        && iListenerType == EVENTLISTENER_CLR)
    {
        IFC_RETURN(pDO->SetParticipatesInManagedTreeDefault());
    }

    if (!pEventList)
    {
        pEventList = std::make_unique<std::vector<REQUEST>>();
    }

    pEventList->emplace_back(hEvent, fHandledEventsToo);
    if (pValue->GetType() == valueInternalHandler)
    {
        pEventList->back().m_pfnInternalEventDelegate = pValue->AsInternalHandler();
    }

    bool fIgnoreIsActive = pDO->AllowsHandlerWhenNotLive(iListenerType, hEvent.index);
    //popup does not need to be in the tree for its events to be fired.
    //adding a one off special case. for any more such types, consider adding
    //a flag to metadata instead. there is a corresponding check in RemoveEventListener.

    if (fSkipIsActiveCheck || fIgnoreIsActive || pDO->IsFiringEvents())
    {
        auto* pEventManager = pDO->GetContext()->GetEventManager();
        if (pEventManager != nullptr)
        {
            pEventManager->AddToLoadedEventListIfNeeded(pDO, hEvent);
        }

        // If we are active (can fire events), mark the handler so
        pEventList->back().m_bActive = TRUE;
        if (fIgnoreIsActive)
        {
            // The passed-in fSkipIsActiveCheck applies only to
            // this addition, it does not mean the handler should remain
            // active if handlers for the DO are disabled.
            pEventList->back().m_bCanFireWhenInactive = TRUE;
        }
    }

    return S_OK;
}

_Check_return_
HRESULT
CEventManager::RemoveEventListener(
    _In_ CDependencyObject *pDO,
    _In_ std::vector<REQUEST> *pEventList,
    _In_ EventHandle hEvent,
    _In_ CValue *pValue)
{
    IFCPTR_RETURN(pDO);

    // Remove this from our internal list...
    if (pEventList == nullptr)
    {
        // Trying to remove something that isn't there is treated as a success.
        return S_OK;
    }

    // Look through the entire list to determine whether we should remove the DO
    // from the list of loaded event objects.
    bool hasLoadedEvent = false;
    bool removedLoadedEvent = false;
    auto eraseIt = pEventList->end();

    for (auto it = pEventList->begin(); it != pEventList->end(); ++it)
    {
        const REQUEST& request = *it;
        bool shouldErase = false;

        // We will remove at most one listener but need to examine the entire list to determine
        // whether we need to remove from the loaded event list.
        if (eraseIt == pEventList->end())
        {
            if (request.m_hEvent == hEvent)
            {
                if (pValue->GetType() == valueInternalHandler)
                {
                    // This REQUEST was placed from internal code to call back to a
                    //  static method pointer.
                    if (pValue->AsInternalHandler() == request.m_pfnInternalEventDelegate)
                    {
                        shouldErase = true;
                    }
                }
                else if (pValue->GetType() == valueAny)
                {
                    if (request.m_pfnInternalEventDelegate == nullptr)
                    {
                        shouldErase = true;
                    }
                }
                else
                {
                    XAML_FAIL_FAST();
                    break;
                }
            }
        }

        if (shouldErase)
        {
            eraseIt = it;
            removedLoadedEvent = IsLoadedEvent(request.m_hEvent);
        }
        else
        {
            hasLoadedEvent |= IsLoadedEvent(request.m_hEvent);
        }
    }

    if (eraseIt != pEventList->end())
    {
        pEventList->erase(eraseIt);
    }

    if (removedLoadedEvent && !hasLoadedEvent)
    {
        auto* pEventManager = pDO->GetContext()->GetEventManager();
        if (pEventManager != nullptr)
        {
            pEventManager->RemoveFromLoadedEventList(pDO);
        }
    }

    return S_OK;
}

void CEventManager::AddToLoadedEventListIfNeeded(
    _In_ CDependencyObject* pDO,
    _In_ EventHandle hEvent)
{
    // Loaded events are not fired on a per-element basis, rather on the entire tree at once.
    // Also, Loaded events should be fired in the order in which their handlers were added.
    // Each DO maintains its event handlers in the order added, but does not maintain
    // the order among the elements themselves. As a result, we need to maintain a separate
    // list for the order among elements for which Loaded event is pending.
    if (IsLoadedEvent(hEvent) && !IsLoadedEventPending(pDO))
    {
        m_loadedEventObjects.emplace_back(pDO);
    }
}

//------------------------------------------------------------------------
//
//  Method:   CEventManager::RemoveFromLoadedEventList
//
//  Synopsis:
//      Remove from list of objects on which Loaded event is yet to be fired.
//
//------------------------------------------------------------------------
void CEventManager::RemoveFromLoadedEventList(_In_ CDependencyObject* pElement)
{
    auto it = std::find(m_loadedEventObjects.begin(), m_loadedEventObjects.end(), pElement);
    if (it != m_loadedEventObjects.end())
    {
        m_loadedEventObjects.erase(it);
   }
}

bool CEventManager::IsLoadedEventPending(_In_ CDependencyObject* pElement)
{
    return std::find(m_loadedEventObjects.begin(), m_loadedEventObjects.end(), pElement) != m_loadedEventObjects.end();
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