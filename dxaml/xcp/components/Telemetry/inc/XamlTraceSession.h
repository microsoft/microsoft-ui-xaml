// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Instrumentation {

    class __declspec(uuid("3d4543e2-8792-4b95-8d0a-0f7b5c1260fa")) XamlTraceSession
    {
    public:
        XamlTraceSession();
        static void GetUniqueId(std::wstring& uniqueId);
        const wchar_t* GetSessionId();
        const wchar_t* GetAppId();
        DWORD GetProcessId();

    private:
        std::wstring m_appId;
        std::wstring m_sessionId;
        DWORD m_processId;

        std::mutex m_mutex;
    };

    std::shared_ptr<XamlTraceSession> GetXamlTraceSession();
}
