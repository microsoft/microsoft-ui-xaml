// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include <memory>
#include "Pathcch.h"
#include "EventProcessor.h"
#include "RuleServiceProvider.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
#define MAX_TRACE_EVENT_INFO 8192 // this value is about 4 times the amount seen in traces (it should be enough)

////////////////////////////////////////////////////////////////////////////////
//
EventProcessor::EventProcessor()
    : m_pTraceEventInfo(nullptr)
    , m_pProviderDispatchInfo(nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////
//
EventProcessor::~EventProcessor()
{
    delete[](BYTE*)m_pTraceEventInfo;
    m_pTraceEventInfo = nullptr;

    PROVIDER_DISPATCH_INFO* pProvider = m_pProviderDispatchInfo;
    while (pProvider)
    {
        EVENT_DISPATCH_INFO* pEvent = pProvider->pEvents;
        while (pEvent)
        {
            RULE_DISPATCH_INFO* pRule = pEvent->pRules;
            while (pRule)
            {
                RULE_DISPATCH_INFO* pPrev = pRule;
                pRule = pRule->pNext;
                delete pPrev;
            }

            EVENT_DISPATCH_INFO* pPrev = pEvent;
            pEvent = pEvent->pNext;
            delete pPrev;
        }
        PROVIDER_DISPATCH_INFO* pPrev = pProvider;
        pProvider = pProvider->pNext;
        delete pPrev;
    }

    m_pProviderDispatchInfo = nullptr;
}

HRESULT
EventProcessor::ProcessEventStatic(
    _In_ PEVENT_RECORD eventRecord
    )
{
    static wrl::ComPtr<IEventProcessorPrivate> singleton;
    if (!singleton)
    {
        wrl::ComPtr<IActivationFactory> factory;
        IFC_RETURN(wrl::Module< wrl::InProc >::GetModule().GetActivationFactory(StringRef(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EventProcessor), &factory));
        IFC_RETURN(factory.As(&singleton));
    }

    IFC_RETURN(singleton->ProcessEvent(eventRecord));

    return S_OK;
}

HRESULT
EventProcessor::RegisterEventStatic(
    _In_ appanalysis::IEtwEvent* etwEvent,
    _In_ appanalysis::IEtwEventRecordCallback* callback)
{
    static wrl::ComPtr<IEventProcessorStatics> singleton;
    if (!singleton)
    {
        wrl::ComPtr<IActivationFactory> factory;
        IFC_RETURN(wrl::Module< wrl::InProc >::GetModule().GetActivationFactory(StringRef(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EventProcessor), &factory));
        IFC_RETURN(factory.As(&singleton));
    }

    IFC_RETURN(singleton->RegisterEvent(etwEvent, callback));

    return S_OK;
}

HRESULT
EventProcessor::UnregisterEventStatic(
    _In_ appanalysis::IEtwEvent* etwEvent,
    _In_ appanalysis::IEtwEventRecordCallback* callback)
{
    static wrl::ComPtr<IEventProcessorStatics> singleton;
    if (!singleton)
    {
        wrl::ComPtr<IActivationFactory> factory;
        IFC_RETURN(wrl::Module< wrl::InProc >::GetModule().GetActivationFactory(StringRef(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EventProcessor), &factory));
        IFC_RETURN(factory.As(&singleton));
    }

    IFC_RETURN(singleton->UnregisterEvent(etwEvent, callback));

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//
HRESULT
EventProcessor::RuntimeClassInitialize(
    )
{
    // Allocate a buffer large enough to hold all events that we care about
    m_pTraceEventInfo = (PTRACE_EVENT_INFO) new BYTE[MAX_TRACE_EVENT_INFO];

    IFC_RETURN(wrl::MakeAndInitialize<EtwEventRecord>(&m_eventRecord));

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// This method registers an ETW consumer with the EventProcessor engine such that
// it will receieve the ETW events that it wants in order. The consumer can be
// either an ETWRule, or ETWRuleService, but the processing engine is indifferent 
// and treats them the same.
//
HRESULT
EventProcessor::RegisterEvent(
    _In_ appanalysis::IEtwEvent* etwEvent,
    _In_ appanalysis::IEtwEventRecordCallback* callback)
{
    IFCPTR_RETURN(etwEvent);
    IFCPTR_RETURN(callback);

    wrl::ComPtr<appanalysis::IEtwProvider> provider;
    IFC_RETURN(etwEvent->get_Provider(&provider));

    PROVIDER_DISPATCH_INFO* pProviderDispatch = nullptr;
    IFC_RETURN(AddProviderDispatch(provider.Get(), &pProviderDispatch));

    EVENT_DISPATCH_INFO* pEventDispatch = nullptr;

    IFC_RETURN(AddEventDispatch(pProviderDispatch, etwEvent, &pEventDispatch));
    IFC_RETURN(AddRuleDispatch(pEventDispatch, callback));

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// This method unregisters an ETW consumer with the EventProcessor engine such that
// it will no longer receive events
//
HRESULT
EventProcessor::UnregisterEvent(
    _In_ appanalysis::IEtwEvent* etwEvent,
    _In_ appanalysis::IEtwEventRecordCallback* callback)
{
    IFCPTR_RETURN(etwEvent);
    IFCPTR_RETURN(callback);

    wrl::ComPtr<appanalysis::IEtwProvider> provider;
    IFC_RETURN(etwEvent->get_Provider(&provider));

    GUID providerId = GUID_NULL;
    IFC_RETURN(provider->get_ID(&providerId));

    EventInfo info = { 0 };
    IFC_RETURN(etwEvent->get_EventVersion(&info.EventVersion));
    IFC_RETURN(etwEvent->get_EventId(&info.EventId));

    EVENT_DISPATCH_INFO* eventDispatch = FindEventDispatchInfo(providerId, info);

    if (!eventDispatch)
    {
        return S_OK;
    }

    RULE_DISPATCH_INFO* callbackFinder = eventDispatch->pRules;
    while (callbackFinder && callbackFinder->spEventCallback.Get() != callback)
    {
        callbackFinder = callbackFinder->pNext;
    }

    if (!callbackFinder)
    {
        return S_OK;
    }

    auto unlockOnExit = m_processSection.lock();

    // has both previous and next, need to remove and connect the two
    if (callbackFinder->pPrevious && callbackFinder->pNext)
    {
        eventDispatch->pRules = callbackFinder->pPrevious;
        callbackFinder->pPrevious->pNext = callbackFinder->pNext;
        callbackFinder->pNext->pPrevious = callbackFinder->pPrevious;
    }
    // at the end of the list
    else if (callbackFinder->pPrevious)
    {
        eventDispatch->pRules = callbackFinder->pPrevious;
        callbackFinder->pPrevious->pNext = nullptr;
    }
    // at the head
    else if (callbackFinder->pNext)
    {
        eventDispatch->pRules = callbackFinder->pNext;
        callbackFinder->pNext->pPrevious = nullptr;
    }
    else
    {
        // removing the rule from this event dispatch
        eventDispatch->pRules = nullptr;
    }

    delete callbackFinder;
    callbackFinder = nullptr;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
EventProcessor::AddProviderDispatch(
    _In_ appanalysis::IEtwProvider* provider,
    _Out_ PROVIDER_DISPATCH_INFO** ppProviderDispatchInfo
    )
{
    ARG_VALIDRETURNPOINTER(ppProviderDispatchInfo);

    HRESULT hr = S_OK;

    *ppProviderDispatchInfo = nullptr;

    GUID ProviderId = GUID_NULL;
    IFC_RETURN(provider->get_ID(&ProviderId));
    if (m_pProviderDispatchInfo == nullptr)
    {
        // We're empty, create the first entry.
        m_pProviderDispatchInfo = new PROVIDER_DISPATCH_INFO();

        m_pProviderDispatchInfo->ProviderId = ProviderId;
        m_pProviderDispatchInfo->pEvents = nullptr;
        m_pProviderDispatchInfo->pNext = nullptr;

        *ppProviderDispatchInfo = m_pProviderDispatchInfo;
    }
    else
    {
        PROVIDER_DISPATCH_INFO* pProvider = m_pProviderDispatchInfo;
        while (pProvider)
        {
            if (IsEqualGUID(pProvider->ProviderId, ProviderId))
            {
                // Found existing dispatch information for this provider.
                *ppProviderDispatchInfo = pProvider;
                break;
            }

            if (!pProvider->pNext)
            {
                // No matches were found, add a new entry at the end.
                PROVIDER_DISPATCH_INFO* pNew = new PROVIDER_DISPATCH_INFO();

                pNew->ProviderId = ProviderId;
                pNew->pEvents = nullptr;
                pNew->pNext = nullptr;

                pProvider->pNext = pNew;

                *ppProviderDispatchInfo = pNew;
                break;
            }

            pProvider = pProvider->pNext;
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
EventProcessor::AddEventDispatch(
    _In_ PROVIDER_DISPATCH_INFO* pProviderDispatchInfo,
    _In_ appanalysis::IEtwEvent* etwEventInfo,
    _Out_ EVENT_DISPATCH_INFO** ppEventDispatchInfo
    )
{
    *ppEventDispatchInfo = nullptr;
    IFCPTR_RETURN(pProviderDispatchInfo);

    EventInfo eventInfo = { 0 };
    IFC_RETURN(etwEventInfo->get_EventId(&eventInfo.EventId));
    IFC_RETURN(etwEventInfo->get_EventVersion(&eventInfo.EventVersion));

    if (pProviderDispatchInfo->pEvents == nullptr)
    {
        // We're empty, create the first entry.
        pProviderDispatchInfo->pEvents = new EVENT_DISPATCH_INFO();

        pProviderDispatchInfo->pEvents->eventInfo = eventInfo;
        pProviderDispatchInfo->pEvents->pRules = nullptr;
        pProviderDispatchInfo->pEvents->pNext = nullptr;

        *ppEventDispatchInfo = pProviderDispatchInfo->pEvents;
    }
    else
    {
        EVENT_DISPATCH_INFO* pEvent = pProviderDispatchInfo->pEvents;
        while (pEvent)
        {
            if (pEvent->eventInfo.EventId == eventInfo.EventId &&
                pEvent->eventInfo.EventVersion == eventInfo.EventVersion)
            {
                // Found existing dispatch information for this event.
                *ppEventDispatchInfo = pEvent;
                break;
            }

            if (!pEvent->pNext)
            {
                // No matches were found, add a new entry at the end.
                EVENT_DISPATCH_INFO* pNew = new EVENT_DISPATCH_INFO();

                pNew->eventInfo = eventInfo;
                pNew->pRules = nullptr;
                pNew->pNext = nullptr;

                pEvent->pNext = pNew;

                *ppEventDispatchInfo = pNew;
                break;
            }

            pEvent = pEvent->pNext;
        }
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
EventProcessor::AddRuleDispatch(
    _In_ EVENT_DISPATCH_INFO* pEventDispatchInfo,
    _In_ appanalysis::IEtwEventRecordCallback* pEventCallback
    )
{
    HRESULT hr = S_OK;
    IFCPTR(pEventDispatchInfo);
    IFCPTR(pEventCallback);
    
    RULE_DISPATCH_INFO* pNew = new RULE_DISPATCH_INFO();

    pNew->spEventCallback = pEventCallback;
    pNew->pNext = nullptr;
    pNew->pPrevious = nullptr;

    if (pEventDispatchInfo->pRules == nullptr)
    {
        // We're empty, create the first entry.
        pEventDispatchInfo->pRules = pNew;
    }
    else
    {
        RULE_DISPATCH_INFO* pRule = pEventDispatchInfo->pRules;
        while (pRule->pNext)
        {
            pRule = pRule->pNext;
            pNew->pPrevious = pRule;
        }

        pRule->pNext = pNew;
    }
    
Cleanup:
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// This is the callback from the trace session where we process events. This
// method filters the event and searches through the linked list to find
// a Rule/RuleService that is interested in this event. If found, the
// EventProcessor ensures the rule gets the event in a threadsafe way since
// this method can be called from multiple threads.
//
IFACEMETHODIMP
EventProcessor::ProcessEvent(
    _In_ PEVENT_RECORD eventRecord
    )
{
    PROVIDER_DISPATCH_INFO* pProviderInfo = m_pProviderDispatchInfo;

    // Parameter check.
    IFCPTR_RETURN(eventRecord);

    // There should be at least one rule scheduled to run.
    IFCPTR_RETURN(pProviderInfo);

    // Get provider and event information from the event
    GUID ProviderId = EventProcessor::GetProviderFromRecord(eventRecord);

    EventInfo eventInfo = EventProcessor::GetEventInfoFromRecord(eventRecord);

    //
    // Dispatch the event.
    //

    EVENT_DISPATCH_INFO* eventDispatch = FindEventDispatchInfo(ProviderId, eventInfo);

    // if we don't find the dispatch info for this event, return
    if (!eventDispatch)
    {
        return S_OK;
    }

    // wil::critical_section::lock() returns an object that
    // will unlock once it goes out of scope. we lock here so that another
    // thread won't come in and interrupt the current thread dispatching the
    // event to the rules.
    auto unlockOnExit = m_processSection.lock();

    m_eventRecord->SetEventRecord(eventRecord);
                    
    RULE_DISPATCH_INFO* pRule = eventDispatch->pRules;
    while (pRule)
    {
        (void) pRule->spEventCallback->Invoke(m_eventRecord.Get());
        pRule = pRule->pNext;
    }

    m_eventRecord->SetEventRecord(nullptr);

    return S_OK;
}

EventProcessor::EVENT_DISPATCH_INFO*
EventProcessor::FindEventDispatchInfo(
    _In_ const GUID & providerId, 
    _In_ const EventInfo & eventInfo)
{
    PROVIDER_DISPATCH_INFO* pProviderInfo = m_pProviderDispatchInfo;

    bool foundProviderInfo = false;
    while (pProviderInfo && !foundProviderInfo)
    {
        // Find dispatch information for this provider.
        if (InlineIsEqualGUID(pProviderInfo->ProviderId, providerId))
        {
            // we have found the dispath information for this provider, we 
            // won't need to walk through the list anymore, now we just want to
            // find the event information.
            foundProviderInfo = true;
            EVENT_DISPATCH_INFO* pEventInfo = pProviderInfo->pEvents;

            while (pEventInfo)
            {
                // Find dispatch information for this event.
                if (pEventInfo->eventInfo.EventId == eventInfo.EventId)
                {
                    return pEventInfo;
                }
                pEventInfo = pEventInfo->pNext;
            }
        }
        pProviderInfo = pProviderInfo->pNext;
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
//
EventInfo
EventProcessor::GetEventInfoFromRecord(
    _In_ PEVENT_RECORD eventRecord
    )
{
    EventInfo info = { 0 };

    // Classic (MOF) providers store the Event ID in the Opcode.
    if ((eventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_CLASSIC_HEADER) == EVENT_HEADER_FLAG_CLASSIC_HEADER)
    {
        info.EventId = eventRecord->EventHeader.EventDescriptor.Opcode;
    }
    else
    {
        info.EventId = eventRecord->EventHeader.EventDescriptor.Id;
    }

    info.EventVersion = eventRecord->EventHeader.EventDescriptor.Version;

    return info;
}

////////////////////////////////////////////////////////////////////////////////
//
GUID
EventProcessor::GetProviderFromRecord(
    _In_ PEVENT_RECORD eventRecord
    )
{
    return eventRecord->EventHeader.ProviderId;
}

} } }
