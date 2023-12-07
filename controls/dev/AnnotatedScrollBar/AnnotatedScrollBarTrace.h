// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "TraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsAnnotatedScrollBarTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_ANNOTATEDSCROLLBAR || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsAnnotatedScrollBarVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_ANNOTATEDSCROLLBAR || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsAnnotatedScrollBarPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_ANNOTATEDSCROLLBAR || g_PerfProviderMatchAnyKeyword == 0);
}

#define ANNOTATEDSCROLLBAR_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
AnnotatedScrollBarTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define ANNOTATEDSCROLLBAR_TRACE_INFO(sender, message, ...) \
if (IsAnnotatedScrollBarTracingEnabled()) \
{ \
    ANNOTATEDSCROLLBAR_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (AnnotatedScrollBarTrace::s_IsDebugOutputEnabled || AnnotatedScrollBarTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    ANNOTATEDSCROLLBAR_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define ANNOTATEDSCROLLBAR_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
AnnotatedScrollBarTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define ANNOTATEDSCROLLBAR_TRACE_VERBOSE(sender, message, ...) \
if (IsAnnotatedScrollBarVerboseTracingEnabled()) \
{ \
    ANNOTATEDSCROLLBAR_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (AnnotatedScrollBarTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    ANNOTATEDSCROLLBAR_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define ANNOTATEDSCROLLBAR_TRACE_PERF(info) \
if (IsAnnotatedScrollBarPerfTracingEnabled()) \
{ \
    AnnotatedScrollBarTrace::TracePerfInfo(info); \
} \

#ifdef DBG
#define ANNOTATEDSCROLLBAR_TRACE_INFO_DBG(sender, message, ...)    ANNOTATEDSCROLLBAR_TRACE_INFO(sender, message, __VA_ARGS__)
#define ANNOTATEDSCROLLBAR_TRACE_VERBOSE_DBG(sender, message, ...) ANNOTATEDSCROLLBAR_TRACE_VERBOSE(sender, message, __VA_ARGS__)
#define ANNOTATEDSCROLLBAR_TRACE_PERF_DBG(info)                    ANNOTATEDSCROLLBAR_TRACE_PERF(info)
#else
#define ANNOTATEDSCROLLBAR_TRACE_INFO_DBG(sender, message, ...)
#define ANNOTATEDSCROLLBAR_TRACE_VERBOSE_DBG(sender, message, ...)
#define ANNOTATEDSCROLLBAR_TRACE_PERF_DBG(info)
#endif // DBG

class AnnotatedScrollBarTrace
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
                    "AnnotatedScrollBarInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_ANNOTATEDSCROLLBAR),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"AnnotatedScrollBar") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
                    "AnnotatedScrollBarVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_ANNOTATEDSCROLLBAR),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"AnnotatedScrollBar") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
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
            "AnnotatedScrollBarPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_ANNOTATEDSCROLLBAR),
            TraceLoggingWideString(info, "Info"));
    }
};
