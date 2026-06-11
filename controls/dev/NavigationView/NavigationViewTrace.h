// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "MuxcTraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsNavigationViewTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_NAVIGATIONVIEW || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsNavigationViewVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_NAVIGATIONVIEW || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsNavigationViewPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_NAVIGATIONVIEW || g_PerfProviderMatchAnyKeyword == 0);
}

#define NAVIGATIONVIEW_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
NavigationViewTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define NAVIGATIONVIEW_TRACE_INFO(sender, message, ...) \
if (IsNavigationViewTracingEnabled()) \
{ \
    NAVIGATIONVIEW_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (NavigationViewTrace::s_IsDebugOutputEnabled || NavigationViewTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    NAVIGATIONVIEW_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define NAVIGATIONVIEW_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
NavigationViewTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define NAVIGATIONVIEW_TRACE_VERBOSE(sender, message, ...) \
if (IsNavigationViewVerboseTracingEnabled()) \
{ \
    NAVIGATIONVIEW_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (NavigationViewTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    NAVIGATIONVIEW_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define NAVIGATIONVIEW_TRACE_PERF(info) \
if (IsNavigationViewPerfTracingEnabled()) \
{ \
    NavigationViewTrace::TracePerfInfo(info); \
} \

#ifdef DBG
#define NAVIGATIONVIEW_TRACE_INFO_DBG(sender, message, ...)    NAVIGATIONVIEW_TRACE_INFO(sender, message, __VA_ARGS__)
#define NAVIGATIONVIEW_TRACE_VERBOSE_DBG(sender, message, ...) NAVIGATIONVIEW_TRACE_VERBOSE(sender, message, __VA_ARGS__)
#define NAVIGATIONVIEW_TRACE_PERF_DBG(info)                    NAVIGATIONVIEW_TRACE_PERF(info)
#else
#define NAVIGATIONVIEW_TRACE_INFO_DBG(sender, message, ...)
#define NAVIGATIONVIEW_TRACE_VERBOSE_DBG(sender, message, ...)
#define NAVIGATIONVIEW_TRACE_PERF_DBG(info)
#endif // DBG

class NavigationViewTrace
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
                    "NavigationViewInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_NAVIGATIONVIEW),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"NavigationView") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
                    "NavigationViewVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_NAVIGATIONVIEW),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"NavigationView") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
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
            "NavigationViewPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_NAVIGATIONVIEW),
            TraceLoggingWideString(info, "Info"));
    }
};
