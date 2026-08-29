// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include <chrono>
#include <TestEvent.h>
#include <vector>
#include <map>
#include "etwrealtimeconsumer.h"
#include "EtwWaiter.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        class ETWWaiterKey
        {
        public:
            ETWWaiterKey(GUID providerGUID, unsigned int eventID)
                : m_providerGUID(providerGUID)
                , m_eventID(eventID)
                , m_taskName()
            { }

            ETWWaiterKey(GUID providerGUID, const wchar_t* taskName)
                : m_providerGUID(providerGUID)
                , m_eventID(0)
                , m_taskName(taskName)
            { }

            // two equal events, one created with eventId and one with taskName will not be equal - no way to retrieve event id.
            bool operator<(const ETWWaiterKey& k) const // provide simple comparison operation for std::map
            {
                int guidCmp = memcmp(&m_providerGUID, &k.m_providerGUID, sizeof(GUID));
                if (guidCmp == 0)
                {
                    int taskNameCmp = m_taskName.compare(k.m_taskName);
                    if (taskNameCmp == 0)
                    {
                        if (m_taskName.empty())
                        {
                            return m_eventID < k.m_eventID;
                        }
                        return false;
                    }
                    return taskNameCmp < 0;
                }
                return guidCmp < 0;
            }

        private:
            GUID m_providerGUID;
            unsigned int m_eventID;
            std::wstring m_taskName;
        };

        class ETWWaiterServerHelper
        {
        public:
            static void Start(GUID providerGuid, unsigned long eventId);
            static void StartWithTaskName(GUID providerGuid, const wchar_t* taskName);
            static void StartWithPayloadMatch(GUID providerGuid, unsigned long eventId, const wchar_t* payloadCriteria);
            static void Wait(GUID providerGuid, unsigned long eventId, unsigned int timeoutMs);
            static void WaitForTaskName(GUID providerGuid, const wchar_t* taskName, unsigned int timeoutMs);
            static void Stop(GUID providerGuid, unsigned long eventId);
            static void StopTaskName(GUID providerGuid, const wchar_t* taskName);
            static void GetActiveWaiterCount(unsigned int *waiterCount);

        private:
            static std::map<ETWWaiterKey, std::unique_ptr<Etw::Processor::EtwWaiter>> s_WaiterMap;

            static bool AddKeyValue(ETWWaiterKey* key, Etw::Processor::EtwWaiter* waiter);
            static void WaitForKey(ETWWaiterKey* key, unsigned int timeoutMs);
            static bool StopForKey(ETWWaiterKey* key);
        };
    }
} } } }
