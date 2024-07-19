﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "MuxcTraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsPTRTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_PTR || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsPTRVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_PTR || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsPTRPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_PTR || g_PerfProviderMatchAnyKeyword == 0);
}

#define PTR_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
PTRTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define PTR_TRACE_INFO(sender, message, ...) \
if (IsPTRTracingEnabled()) \
{ \
    PTR_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (PTRTrace::s_IsDebugOutputEnabled || PTRTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    PTR_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define PTR_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
PTRTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define PTR_TRACE_VERBOSE(sender, message, ...) \
if (IsPTRVerboseTracingEnabled()) \
{ \
    PTR_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (PTRTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    PTR_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define PTR_TRACE_PERF(info) \
if (IsPTRPerfTracingEnabled()) \
{ \
    PTRTrace::TracePerfInfo(info); \
} \

#ifdef DBG
#define PTR_TRACE_INFO_DBG(sender, message, ...)    PTR_TRACE_INFO(sender, message, __VA_ARGS__)
#define PTR_TRACE_VERBOSE_DBG(sender, message, ...) PTR_TRACE_VERBOSE(sender, message, __VA_ARGS__)
#define PTR_TRACE_PERF_DBG(info)                    PTR_TRACE_PERF(info)
#else
#define PTR_TRACE_INFO_DBG(sender, message, ...)
#define PTR_TRACE_VERBOSE_DBG(sender, message, ...)
#define PTR_TRACE_PERF_DBG(info)
#endif // DBG

class PTRTrace
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
                    "PTRInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_PTR),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"PTR") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
        if SUCCEEDED((StringCchVPrintfW(buffer, ARRAYSIZE(buffer), message, args)))
        {
            if (includeTraceLogging)
            {
                // TraceViewers
                // http://toolbox/pef
                // http://fastetw/index.aspx
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "PTRVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_PTR),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"PTR") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
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
            "PTRPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_PTR),
            TraceLoggingWideString(info, "Info"));
    }
};
