﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "MuxcTraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsScrollViewTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_SCROLLVIEW || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsScrollViewVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_SCROLLVIEW || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsScrollViewPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_SCROLLVIEW || g_PerfProviderMatchAnyKeyword == 0);
}

#define SCROLLVIEW_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
ScrollViewTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SCROLLVIEW_TRACE_INFO(sender, message, ...) \
if (IsScrollViewTracingEnabled()) \
{ \
    SCROLLVIEW_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ScrollViewTrace::s_IsDebugOutputEnabled || ScrollViewTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    SCROLLVIEW_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SCROLLVIEW_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
ScrollViewTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SCROLLVIEW_TRACE_VERBOSE(sender, message, ...) \
if (IsScrollViewVerboseTracingEnabled()) \
{ \
    SCROLLVIEW_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ScrollViewTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    SCROLLVIEW_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SCROLLVIEW_TRACE_PERF(info) \
if (IsScrollViewPerfTracingEnabled()) \
{ \
    ScrollViewTrace::TracePerfInfo(info); \
} \

#ifdef DBG
#define SCROLLVIEW_TRACE_INFO_DBG(sender, message, ...)    SCROLLVIEW_TRACE_INFO(sender, message, __VA_ARGS__)
#define SCROLLVIEW_TRACE_VERBOSE_DBG(sender, message, ...) SCROLLVIEW_TRACE_VERBOSE(sender, message, __VA_ARGS__)
#define SCROLLVIEW_TRACE_PERF_DBG(info)                    SCROLLVIEW_TRACE_PERF(info)
#else
#define SCROLLVIEW_TRACE_INFO_DBG(sender, message, ...)
#define SCROLLVIEW_TRACE_VERBOSE_DBG(sender, message, ...)
#define SCROLLVIEW_TRACE_PERF_DBG(info)
#endif // DBG

class ScrollViewTrace
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
                    "ScrollViewInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_SCROLLVIEW),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ScrollView") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
                    "ScrollViewVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_SCROLLVIEW),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ScrollView") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
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
            "ScrollViewPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_SCROLLVIEW),
            TraceLoggingWideString(info, "Info"));
    }
};
