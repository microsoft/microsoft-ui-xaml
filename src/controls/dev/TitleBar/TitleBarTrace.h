// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "MuxcTraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsTitleBarTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_TITLEBAR || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsTitleBarVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_TITLEBAR || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsTitleBarPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_TITLEBAR || g_PerfProviderMatchAnyKeyword == 0);
}

#define TITLEBAR_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
TitleBarTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define TITLEBAR_TRACE_INFO(sender, message, ...) \
if (IsTitleBarTracingEnabled()) \
{ \
    TITLEBAR_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (TitleBarTrace::s_IsDebugOutputEnabled || TitleBarTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    TITLEBAR_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define TITLEBAR_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
TitleBarTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define TITLEBAR_TRACE_VERBOSE(sender, message, ...) \
if (IsTitleBarVerboseTracingEnabled()) \
{ \
    TITLEBAR_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (TitleBarTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    TITLEBAR_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define TITLEBAR_TRACE_PERF(info) \
if (IsTitleBarPerfTracingEnabled()) \
{ \
    TitleBarTrace::TracePerfInfo(info); \
} \

#ifdef DBG
#define TITLEBAR_TRACE_INFO_DBG(sender, message, ...)    TITLEBAR_TRACE_INFO(sender, message, __VA_ARGS__)
#define TITLEBAR_TRACE_VERBOSE_DBG(sender, message, ...) TITLEBAR_TRACE_VERBOSE(sender, message, __VA_ARGS__)
#define TITLEBAR_TRACE_PERF_DBG(info)                    TITLEBAR_TRACE_PERF(info)
#else
#define TITLEBAR_TRACE_INFO_DBG(sender, message, ...)
#define TITLEBAR_TRACE_VERBOSE_DBG(sender, message, ...)
#define TITLEBAR_TRACE_PERF_DBG(info)
#endif // DBG

class TitleBarTrace
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
                    "TitleBarInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_TITLEBAR),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"TitleBar") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
                    "TitleBarVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_TITLEBAR),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"TitleBar") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
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
            "TitleBarPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_TITLEBAR),
            TraceLoggingWideString(info, "Info"));
    }
};
