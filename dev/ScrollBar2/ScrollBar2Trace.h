// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "TraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsScrollBar2TracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_SCROLLBAR2 || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsScrollBar2VerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_SCROLLBAR2 || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsScrollBar2PerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_SCROLLBAR2 || g_PerfProviderMatchAnyKeyword == 0);
}

#define SCROLLBAR2_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
ScrollBar2Trace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SCROLLBAR2_TRACE_INFO(sender, message, ...) \
if (IsScrollBar2TracingEnabled()) \
{ \
    SCROLLBAR2_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ScrollBar2Trace::s_IsDebugOutputEnabled || ScrollBar2Trace::s_IsVerboseDebugOutputEnabled) \
{ \
    SCROLLBAR2_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SCROLLBAR2_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
ScrollBar2Trace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SCROLLBAR2_TRACE_VERBOSE(sender, message, ...) \
if (IsScrollBar2VerboseTracingEnabled()) \
{ \
    SCROLLBAR2_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ScrollBar2Trace::s_IsVerboseDebugOutputEnabled) \
{ \
    SCROLLBAR2_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SCROLLBAR2_TRACE_PERF(info) \
if (IsScrollBar2PerfTracingEnabled()) \
{ \
    ScrollBar2Trace::TracePerfInfo(info); \
} \

class ScrollBar2Trace
{
public:
    static bool s_IsDebugOutputEnabled;
    static bool s_IsVerboseDebugOutputEnabled;

    static void TraceInfo(bool includeTraceLogging, const winrt::IInspectable& sender, PCWSTR message, ...) noexcept
    {
        va_list args;
        va_start(args, message);
        WCHAR buffer[256]{};
        if (SUCCEEDED(StringCchVPrintfW(buffer, ARRAYSIZE(buffer), message, args)))
        {
            if (includeTraceLogging)
            {
                // TraceViewers
                // http://toolbox/pef 
                // http://fastetw/index.aspx
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "ScrollBar2Info" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_SCROLLBAR2),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ScrollBar2") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
            {
                globalTestHooks->LogMessage(sender, buffer, false /*isVerboseLevel*/);
            }
        }
        va_end(args);
    }

    static void TraceVerbose(bool includeTraceLogging, const winrt::IInspectable& sender, PCWSTR message, ...) noexcept
    {
        va_list args;
        va_start(args, message);
        WCHAR buffer[256]{};
        if (SUCCEEDED(StringCchVPrintfW(buffer, ARRAYSIZE(buffer), message, args)))
        {
            if (includeTraceLogging)
            {
                // TraceViewers
                // http://toolbox/pef 
                // http://fastetw/index.aspx
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "ScrollBar2Verbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_SCROLLBAR2),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ScrollBar2") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
            {
                globalTestHooks->LogMessage(sender, buffer, true /*isVerboseLevel*/);
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
            "ScrollBar2Perf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_SCROLLBAR2),
            TraceLoggingWideString(info, "Info"));
    }
};
