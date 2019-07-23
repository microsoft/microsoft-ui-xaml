// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MUXControlsTestHooks.h"
#include "TraceLogging.h"
#include "Utils.h"
#include "common.h"

inline bool IsSwipeControlTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (((g_LoggingProviderMatchAnyKeyword & KEYWORD_SWIPECONTROL) != 0U) || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsSwipeControlVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (((g_LoggingProviderMatchAnyKeyword & KEYWORD_SWIPECONTROL) != 0U) || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsSwipeControlPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (((g_PerfProviderMatchAnyKeyword & KEYWORD_SWIPECONTROL) != 0U) || g_PerfProviderMatchAnyKeyword == 0);
}

#define SWIPECONTROL_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
SwipeControlTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SWIPECONTROL_TRACE_INFO(sender, message, ...) \
if (IsSwipeControlTracingEnabled()) \
{ \
    SWIPECONTROL_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (SwipeControlTrace::s_IsDebugOutputEnabled || SwipeControlTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    SWIPECONTROL_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SWIPECONTROL_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
SwipeControlTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SWIPECONTROL_TRACE_VERBOSE(sender, message, ...) \
if (IsSwipeControlVerboseTracingEnabled()) \
{ \
    SWIPECONTROL_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (SwipeControlTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    SWIPECONTROL_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SWIPECONTROL_TRACE_PERF(info) \
if (IsSwipeControlPerfTracingEnabled()) \
{ \
    SwipeControlTrace::TracePerfInfo(info); \
} \

class SwipeControlTrace
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
                    "SwipeControlInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_SWIPECONTROL),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"SwipeControl") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
                    "SwipeControlVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_SWIPECONTROL),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"SwipeControl") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
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
            "SwipeControlPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_SWIPECONTROL),
            TraceLoggingWideString(info, "Info"));
    }
};
