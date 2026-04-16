// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

// This class is the proxy of TEAF's ETWWatier(http://tfmhelp/index.html?page=source%2Fhtml%2Fproducts%2Ftaef%2Ftaef.htm)
// Underhook, it invokes test_infra::ETWWaiterHelper client helper function and forward RPC call to server process, where the
// actual ETWWaiter object will be created and used. Right now, only simple ETWWaiter and ETWWaiter with payload match(on manifested event) are supported.
// In the future, MultiplicityWaiter, CompositeWaiter and CachingWaiter can be added if need arise.

    class ETWWaiterProxy
    {
    public:

        ETWWaiterProxy()
        {
        }

        ETWWaiterProxy(GUID providerGuid, unsigned long eventId)
            : m_providerGUID(providerGuid)
            , m_eventId(eventId)
        {
            test_infra::ETWWaiterHelper::Start(providerGuid, eventId);
        }

        ETWWaiterProxy(GUID providerGuid, unsigned long eventId, Platform::String ^payloadCriteria)
            : m_providerGUID(providerGuid)
            , m_eventId(eventId)
        {
            test_infra::ETWWaiterHelper::StartWithPayload(providerGuid, eventId, payloadCriteria);
        }

        void Start(GUID providerGuid, unsigned long eventId)
        {
            m_providerGUID = providerGuid;
            m_eventId = eventId;

            test_infra::ETWWaiterHelper::Start(providerGuid, eventId);
        }

        void Start(GUID providerGuid, unsigned long eventId, Platform::String ^payloadCriteria)
        {
            m_providerGUID = providerGuid;
            m_eventId = eventId;

            test_infra::ETWWaiterHelper::StartWithPayload(providerGuid, eventId, payloadCriteria);
        }

        void Stop()
        {
            test_infra::ETWWaiterHelper::Stop(m_providerGUID, m_eventId);
        }

        void WaitForDefault()
        {
            WaitFor(4000); //default wait set to 4 seconds
        }

        void WaitFor(uint32 timeout)
        {
            test_infra::ETWWaiterHelper::Wait(m_providerGUID, m_eventId, timeout);
        }

        ~ETWWaiterProxy()
        {
            test_infra::ETWWaiterHelper::Stop(m_providerGUID, m_eventId);
        }

    private:

        GUID m_providerGUID;
        unsigned int m_eventId;
    };

}}}}}
