// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "macros.h"
#include "wil_resource.h"
#include <unordered_set>
#include <windowscollections.h>
#include "helpers.h"
#include "EtwEventInfo.h"
#include "EtwProvider.h"
#include "EtwEventWatcher.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    typedef UINT64 InstanceHandle;

#define BEGIN_PROVIDERS(Rule) \
    ProviderInfo Rule::ETWConsumerType::m_Providers[] =\
{\

#define DECLARE_MANIFEST_PROVIDER(ProviderId, ManifestName) \
 { ProviderId, ManifestName, 0, appanalysis::ProviderType_Manifest }, \

#define DECLARE_KERNEL_PROVIDER(ProviderId, EnableFlags) \
 { ProviderId, NULL, EnableFlags, appanalysis::ProviderType_Kernel }, \

#define END_PROVIDERS() \
}; \

#define BEGIN_CALLBACKS(Rule) \
    Rule::ProcessEventCallback Rule::ETWConsumerType::m_EventCallbacks[] =\
{\

#define DECLARE_EVENT_CALLBACK(ProviderId, EventId, Version, Handler) \
  { ProviderId, EventId, Version, Handler }, \

#define EventVersion_0 0
#define EventVersion_1 1

#define END_CALLBACKS() \
}; \

    template <class T, typename ...Interfaces>
    class ETWConsumer
        : public wrl::RuntimeClass<Interfaces..., wrl::FtmBase>
    {
    public:
        using ETWConsumerType = ETWConsumer;

        ETWConsumer()
        {
        }

        virtual ~ETWConsumer()
        {
        }

        struct ProcessEventCallback
        {
            GUID ProviderId;
            EventInfo EventInfo;
            HRESULT(T::*Callback)(_In_ appanalysis::IEtwEventRecord* pEvent);
        };

        HRESULT RegisterEvents(_COM_Outptr_ appanalysis::IEtwEventWatcher** watcher)
        {
            ARG_VALIDRETURNPOINTER(watcher);
            *watcher = nullptr;

            wrl::ComPtr<EtwEventWatcher> eventWatcher;
            IFC_RETURN(wrl::MakeAndInitialize<EtwEventWatcher>(&eventWatcher));

            // A rule may not specify the rules in the same order, make sure that we
            // don't add providers more than once
            AppAnalysisHelpers::GuidSet providersAdded;

            for (UINT i = 0; i < _countof(m_Providers); i++)
            {
                GUID currentProviderId = m_Providers[i].ProviderId;

                if (providersAdded.find(currentProviderId) == providersAdded.end())
                {
                    wrl::ComPtr<appanalysis::IEtwProvider> provider;
                    IFC_RETURN(EtwProvider::CreateInstance(m_Providers[i], &provider));
                    IFCSTL_RETURN(providersAdded.emplace(currentProviderId));

                    for (UINT i = 0; i < _countof(m_EventCallbacks); i++)
                    {
                        if (IsEqualGUID(m_EventCallbacks[i].ProviderId, currentProviderId))
                        {
                            wrl::ComPtr<EtwEvent> eventInfo;
                            IFC_RETURN(EtwEvent::CreateInstance(m_EventCallbacks[i].EventInfo, provider.Get(), &eventInfo));
                            auto callback = wrl::Callback<appanalysis::IEtwEventRecordCallback>(static_cast<T*>(this), m_EventCallbacks[i].Callback);
                            IFC_RETURN(eventWatcher->RegisterEvent(eventInfo.Get(), callback.Get()));
                        }
                    }
                }
            }

            *watcher = eventWatcher.Detach();
            return S_OK;
        }

    protected:
        static ProcessEventCallback m_EventCallbacks[];
        static ProviderInfo m_Providers[];
    };

} } }
