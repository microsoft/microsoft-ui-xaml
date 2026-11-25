// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

// This class is the proxy of TEAF's ETWWatier(http://tfmhelp/index.html?page=source%2Fhtml%2Fproducts%2Ftaef%2Ftaef.htm)
// Underhook, it invokes test_infra::ETWWaiterHelper client helper function and forward RPC call to server process, where the
// actual ETWWaiter object will be created and used. Manifest and TraceLogging events support payload matching.
// In the future, MultiplicityWaiter, CompositeWaiter and CachingWaiter can be added if need arise.

    class ETWWaiterProxy
    {
    public:

        ETWWaiterProxy()
        {
        }

        ETWWaiterProxy(GUID providerGuid, unsigned long eventId)
        {
            Start(providerGuid, eventId);
        }

        ETWWaiterProxy(GUID providerGuid, unsigned long eventId, Platform::String ^payloadCriteria)
        {
            Start(providerGuid, eventId, payloadCriteria);
        }

        void Start(GUID providerGuid, unsigned long eventId)
        {
            Stop();
            m_providerGUID = providerGuid;
            m_eventId = eventId;

            test_infra::ETWWaiterHelper::Start(providerGuid, eventId);
            m_started = true;
        }

        void Start(GUID providerGuid, unsigned long eventId, Platform::String ^payloadCriteria)
        {
            Stop();
            m_providerGUID = providerGuid;
            m_eventId = eventId;

            test_infra::ETWWaiterHelper::StartWithPayload(providerGuid, eventId, payloadCriteria);
            m_started = true;
        }

        void Start(GUID providerGuid, Platform::String ^taskName, Platform::String ^payloadCriteria)
        {
            Stop();
            m_providerGUID = providerGuid;
            m_taskName = taskName;

            test_infra::ETWWaiterHelper::StartWithTaskNameAndPayload(providerGuid, taskName, payloadCriteria);
            m_started = true;
        }

        void Stop()
        {
            if (m_started)
            {
                if (m_taskName != nullptr)
                {
                    test_infra::ETWWaiterHelper::StopTaskName(m_providerGUID, m_taskName);
                }
                else
                {
                    test_infra::ETWWaiterHelper::Stop(m_providerGUID, m_eventId);
                }
                m_started = false;
                m_taskName = nullptr;
            }
        }

        void WaitForDefault()
        {
            WaitFor(4000); //default wait set to 4 seconds
        }

        void WaitFor(uint32 timeout)
        {
            if (m_taskName != nullptr)
            {
                test_infra::ETWWaiterHelper::WaitForTaskName(m_providerGUID, m_taskName, timeout);
            }
            else
            {
                test_infra::ETWWaiterHelper::Wait(m_providerGUID, m_eventId, timeout);
            }
        }

        ~ETWWaiterProxy()
        {
            Stop();
        }

    private:

        GUID m_providerGUID{};
        unsigned int m_eventId = 0;
        Platform::String ^m_taskName = nullptr;
        bool m_started = false;
    };

}}}}}
