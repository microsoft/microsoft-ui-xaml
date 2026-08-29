// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlTraceSession.h"
#include <DependencyLocator.h>
#include <appmodel.h>
#include <minappmodel.h>

namespace Instrumentation {

    std::shared_ptr<XamlTraceSession> GetXamlTraceSession()
    {
        static PROVIDE_DEPENDENCY(XamlTraceSession, DependencyLocator::StoragePolicyFlags::None);
        static DependencyLocator::Dependency<Instrumentation::XamlTraceSession> s_traceSession;
        return s_traceSession.Get();
    }

    XamlTraceSession::XamlTraceSession()
    { }

    // get a unique id for tracing scenarios.  as an example, use this when logging tracelogging events need to track
    // all events across a unique control instances 
    /*static*/
    void XamlTraceSession::GetUniqueId(std::wstring& uniqueId)
    {
        GUID guid = { 0 };

        if (SUCCEEDED(CoCreateGuid(&guid)))
        {
            RPC_WSTR guidstr = nullptr;
            if (SUCCEEDED(UuidToString(&guid, &guidstr)))
            {
                uniqueId = guidstr;
            }
            else
            {
                // not able to convurt guid to string. clear any existing output
                uniqueId.clear();
            }
            RpcStringFree(&guidstr);
        }
        else
        {
            // not able to create a guid. clear any existing output
            uniqueId.clear();
        }
    }

    // get an unique session id for current xaml instance.  use this when logging tracelogging events to track
    // all events across an app instance
    const wchar_t* XamlTraceSession::GetSessionId()
    {
        std::lock_guard<std::mutex> lock(m_mutex); // ensure multiple UI threads don't both attempt to initialize m_sessionId

        if (m_sessionId.empty())
        {
            GetUniqueId(m_sessionId);
        }

        return m_sessionId.c_str();
    }

    // get app id of current xaml application
    const wchar_t* XamlTraceSession::GetAppId()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        WCHAR wszAppId[APPLICATION_USER_MODEL_ID_MAX_LENGTH + 1];
        UINT32 cchAppId = ARRAY_SIZE(wszAppId);

        if (m_appId.empty())
        {
            // if we successfully get an appid return that value, otherwise set m_appId to a
            // string to identify a xaml instance running without an application package (explorer start screen search) 
            if (ERROR_SUCCESS == GetCurrentApplicationUserModelId(&cchAppId, wszAppId))
            {
                m_appId = wszAppId;
            }
            else
            {
                m_appId = L"NO_APPLICATION_IDENTITY";
            }
        }

        return m_appId.c_str();
    }

    // get the process id currently running 
    DWORD XamlTraceSession::GetProcessId()
    {
        return GetCurrentProcessId();
    }
}
