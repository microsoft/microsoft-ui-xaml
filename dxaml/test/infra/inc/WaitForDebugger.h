// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

inline void WaitForDebugger()
{
    WEX::Common::String value;
    if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"WaitForDebugger", value)))
    {
        while (true)
        {
            WCHAR szMessage[256];
            ::StringCchPrintfW(szMessage, _countof(szMessage), L"Waiting for a debugger or tttracer to attach - PID %d", ::GetCurrentProcessId());
            WEX::Logging::Log::Comment(szMessage);

            if (IsDebuggerPresent())
            {
                DebugBreak(); 
                break;
            }

            // This module is injected by tttracer -- if it's loaded, we know tttracer is attached.
            if (GetModuleHandle(L"TTDRecordCPU.dll"))
            {
                break;
            }

            Sleep(1000);
        }
    }
}
