// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "wil\resource.h"
#include <unordered_set>
#include "helpers.h"
#include "EtwEventInfo.h"
#include "EtwProvider.h"
#include "AppAnalysisETWEventRecord.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    ////////////////////////////////////////////////////////////////////////////////
    //
    // The EventProcessor is the implementation of IEventProcessor. A client hosting
    // AppAnalysis will use this object to add rules to be Processed (see AddRule)
    // and to feed ETW events for the processor to feed into those rules that are
    // registered for those specific providers/eventids. The EventProcessor is not
    // thread safe, but does ensure that rules are called in a thread safe manner.
    //
    class EventProcessor :
        public wrl::ActivationFactory<appanalysis::IEventProcessorStatics, IEventProcessorPrivate>
    {
        InspectableClassStatic(
            RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EventProcessor, TrustLevel::FullTrust
            );

    public:

        EventProcessor();
        virtual ~EventProcessor();

        static HRESULT
        ProcessEventStatic(
            _In_ PEVENT_RECORD eventRecord
            );

        static HRESULT
        RegisterEventStatic(
            _In_ appanalysis::IEtwEvent* etwEent,
            _In_ appanalysis::IEtwEventRecordCallback* callback
            );

        static HRESULT
        UnregisterEventStatic(
            _In_ appanalysis::IEtwEvent* etwEent,
            _In_ appanalysis::IEtwEventRecordCallback* callback
            );

        HRESULT RuntimeClassInitialize();

    private:
        //
        // IEventProcessorPrivate
        //

        IFACEMETHOD(ProcessEvent)(
            _In_ PEVENT_RECORD eventRecord
            );

        //
        // IEventProcessorStatics
        //

        IFACEMETHOD(RegisterEvent)(
            _In_ appanalysis::IEtwEvent* etwEent,
            _In_ appanalysis::IEtwEventRecordCallback* callback
            ) override;

        IFACEMETHOD(UnregisterEvent)(
            _In_ appanalysis::IEtwEvent* etwEent,
            _In_ appanalysis::IEtwEventRecordCallback* callback
            ) override;



    private:
        struct RULE_DISPATCH_INFO
        {
            wrl::ComPtr<appanalysis::IEtwEventRecordCallback> spEventCallback;
            RULE_DISPATCH_INFO*             pNext{};
            RULE_DISPATCH_INFO*             pPrevious;
        };

        struct EVENT_DISPATCH_INFO
        {
            EventInfo                       eventInfo;
            RULE_DISPATCH_INFO*             pRules;
            EVENT_DISPATCH_INFO*            pNext;
        };

        struct PROVIDER_DISPATCH_INFO
        {
            GUID                            ProviderId;
            EVENT_DISPATCH_INFO*            pEvents;
            PROVIDER_DISPATCH_INFO*         pNext;
        };

    private:

        HRESULT AddProviderDispatch(
            _In_ appanalysis::IEtwProvider* etwProvider,
            _Out_ PROVIDER_DISPATCH_INFO** ppProviderDispatchInfo
            );

        HRESULT AddEventDispatch(
            _In_ PROVIDER_DISPATCH_INFO* pProviderDispatchInfo,
            _In_ appanalysis::IEtwEvent* eventInfo,
            _Out_ EVENT_DISPATCH_INFO** ppEventDispatchInfo
            );

        HRESULT AddRuleDispatch(
            _In_ EVENT_DISPATCH_INFO* pEventDispatchInfo,
            _In_ appanalysis::IEtwEventRecordCallback* pEventCallback
            );

        EVENT_DISPATCH_INFO* FindEventDispatchInfo(
            _In_ const GUID& providerId,
            _In_ const EventInfo& eventInfo
            );

        static EventInfo GetEventInfoFromRecord(_In_ PEVENT_RECORD record);

        static GUID GetProviderFromRecord(_In_ PEVENT_RECORD record);

    private:

        TRACE_EVENT_INFO* m_pTraceEventInfo;
        // TODO: see about switching to a standard container.
        PROVIDER_DISPATCH_INFO* m_pProviderDispatchInfo;
        wil::critical_section m_processSection;
        wrl::ComPtr<EtwEventRecord> m_eventRecord;
    };
} } }
