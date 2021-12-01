// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "TraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsRepeaterTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_REPEATER || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsRepeaterPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_REPEATER || g_PerfProviderMatchAnyKeyword == 0);
}

#define REPEATER_TRACE_INFO(message, ...) \
if (IsRepeaterTracingEnabled()) \
{ \
    RepeaterTrace::TraceInfo(true /*includeTraceLogging*/, message, __VA_ARGS__); \
} \
else if (RepeaterTrace::s_IsDebugOutputEnabled) \
{ \
    RepeaterTrace::TraceInfo(false /*includeTraceLogging*/, message, __VA_ARGS__); \
} \

#define REPEATER_TRACE_PERF(info) \
if(IsRepeaterPerfTracingEnabled()) \
{ \
    RepeaterTrace::TracePerfInfo(info); \
} \

class RepeaterTrace
{
public:
    static bool s_IsDebugOutputEnabled;

    static void TraceInfo(bool includeTraceLogging, PCWSTR message, ...) noexcept
    {
        va_list args;
        va_start(args, message);
        WCHAR buffer[128]{};

        if (SUCCEEDED(StringCchVPrintfW(buffer, ARRAYSIZE(buffer), message, args)))
        {
            if (includeTraceLogging)
            {
                // TraceViewers
                // http://toolbox/pef 
                // http://fastetw/index.aspx
                // GUID for Microsoft.UI.Xaml.Controls.Debug : {afe0ae07-66a7-55bb-12ff-01116bc08c1a}
                // GUID for Windows.UI.Xaml.Controls.Debug :{afe0ae07-66a7-55bb-12ff-01116bc08c1b}
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "RepeaterInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_REPEATER),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (RepeaterTrace::s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks && globalTestHooks->GetLoggingLevelForType(L"Repeater") >= WINEVENT_LEVEL_INFO)
            {
                globalTestHooks->LogMessage(nullptr, buffer, false /*isVerboseLevel*/);
            }
        }
        va_end(args);
    }

    static void TracePerfInfo(PCWSTR info) noexcept
    {
        // TraceViewers
        // http://toolbox/pef 
        // http://fastetw/index.aspx
        TraceLoggingWrite(
            g_hPerfProvider,
            "RepeaterPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_REPEATER), 
            TraceLoggingWideString(info, "Info"));
    }
};
