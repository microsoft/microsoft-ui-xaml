// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ETWWaiterServerHelper.h"
#include <wexassert.h>
#include <cwctype>

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Etw::Processor;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

    std::map<ETWWaiterKey, std::unique_ptr<EtwWaiter>> ETWWaiterServerHelper::s_WaiterMap;
    
    void ETWWaiterServerHelper::Start(GUID providerGuid, unsigned long eventId)
    {
        ETWWaiterKey key(providerGuid, eventId);
        if (AddKeyValue(&key, new EtwWaiter(providerGuid, eventId)) == true)
        {
            LOG_OUTPUT(L"Event %d has already been enabled.", eventId);
        }
    }

    void ETWWaiterServerHelper::StartWithTaskName(GUID providerGuid, const wchar_t* taskName)
    {
        ETWWaiterKey key(providerGuid, taskName);
        if (AddKeyValue(&key, new EtwWaiter(providerGuid, taskName)) == true)
        {
            LOG_OUTPUT(L"Event with TaskName %ls has already been enabled.", taskName);
        }
    }

    //returns true if key already existed in map
    // not going to throw exception if true, just remove and recreate a new ETWWaiter object
    bool ETWWaiterServerHelper::AddKeyValue(ETWWaiterKey* key, EtwWaiter* waiter)
    {
        bool keyAlreadyEnabled = false;
        if (s_WaiterMap.find(*key) != s_WaiterMap.end())
        {
            keyAlreadyEnabled = true;
            s_WaiterMap.erase(*key);
        }
        std::unique_ptr<EtwWaiter> spWaiter(waiter);
        s_WaiterMap[*key] = std::move(spWaiter);
        return keyAlreadyEnabled;
    }

    void ETWWaiterServerHelper::StartWithPayloadMatch(GUID providerGuid, unsigned long eventId, const wchar_t* payloadCriteria)
    {
        ETWWaiterKey key(providerGuid, eventId);
        if(s_WaiterMap.find(key) != s_WaiterMap.end())
        {
            LOG_OUTPUT(L"Event %d has already been enabled.", eventId);
            s_WaiterMap.erase(key);
        }
        
        std::unique_ptr<EtwWaiter> spWaiter(new EtwWaiter(providerGuid, eventId, payloadCriteria));
        s_WaiterMap[key] = std::move(spWaiter);
    }

    void ETWWaiterServerHelper::Wait(GUID providerGuid, unsigned long eventId, unsigned int timeoutMs)
    {
        ETWWaiterKey key(providerGuid, eventId);
        WaitForKey(&key, timeoutMs);
    }

    void ETWWaiterServerHelper::WaitForTaskName(GUID providerGuid, const wchar_t* taskName, unsigned int timeoutMs)
    {
        ETWWaiterKey key(providerGuid, taskName);
        WaitForKey(&key, timeoutMs);
    }

    void ETWWaiterServerHelper::WaitForKey(ETWWaiterKey* key, unsigned int timeoutMs)
    {
        LogThrow_IfFailedWithMessage(s_WaiterMap[*key]->Wait(timeoutMs), L"ETW event waiter failed.");
        LOG_OUTPUT(L"Wait succeeded.");
        s_WaiterMap[*key]->Reset();
    }

    void ETWWaiterServerHelper::Stop(GUID providerGuid, unsigned long eventId)
    {
        ETWWaiterKey key(providerGuid, eventId);
        if (StopForKey(&key) == false)
        {
            LOG_OUTPUT(L"Test is trying to stop a nonexistent event %d.", eventId);
        }
    }

    void ETWWaiterServerHelper::StopTaskName(GUID providerGuid, const wchar_t* taskName)
    {
        ETWWaiterKey key(providerGuid, taskName);
        if (StopForKey(&key) == false)
        {
            LOG_OUTPUT(L"Test is trying to stop a nonexistent event with TaskName %ls", taskName);
        }
    }

    //returns true if event was found and stopped
    bool ETWWaiterServerHelper::StopForKey(ETWWaiterKey* key)
    {
        if (s_WaiterMap.find(*key) != s_WaiterMap.end())
        {
            s_WaiterMap.erase(*key);
            return true;
        }
        return false;
    }

    void ETWWaiterServerHelper::GetActiveWaiterCount(unsigned int *waiterCount)
    {
        *waiterCount =  static_cast<unsigned int>(s_WaiterMap.size());
    }
    }
} } } } 
