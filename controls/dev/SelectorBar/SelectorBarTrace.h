// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "MuxcTraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsSelectorBarTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_SELECTORBAR || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsSelectorBarVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_SELECTORBAR || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsSelectorBarPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_SELECTORBAR || g_PerfProviderMatchAnyKeyword == 0);
}

#define SELECTORBAR_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
SelectorBarTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SELECTORBAR_TRACE_INFO(sender, message, ...) \
if (IsSelectorBarTracingEnabled()) \
{ \
    SELECTORBAR_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (SelectorBarTrace::s_IsDebugOutputEnabled || SelectorBarTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    SELECTORBAR_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SELECTORBAR_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
SelectorBarTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SELECTORBAR_TRACE_VERBOSE(sender, message, ...) \
if (IsSelectorBarVerboseTracingEnabled()) \
{ \
    SELECTORBAR_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (SelectorBarTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    SELECTORBAR_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SELECTORBAR_TRACE_PERF(info) \
if (IsSelectorBarPerfTracingEnabled()) \
{ \
    SelectorBarTrace::TracePerfInfo(info); \
} \

#ifdef DBG
#define SELECTORBAR_TRACE_INFO_DBG(sender, message, ...)    SELECTORBAR_TRACE_INFO(sender, message, __VA_ARGS__)
#define SELECTORBAR_TRACE_VERBOSE_DBG(sender, message, ...) SELECTORBAR_TRACE_VERBOSE(sender, message, __VA_ARGS__)
#define SELECTORBAR_TRACE_PERF_DBG(info)                    SELECTORBAR_TRACE_PERF(info)
#else
#define SELECTORBAR_TRACE_INFO_DBG(sender, message, ...)
#define SELECTORBAR_TRACE_VERBOSE_DBG(sender, message, ...)
#define SELECTORBAR_TRACE_PERF_DBG(info)
#endif // DBG

class SelectorBarTrace
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
                    "SelectorBarInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_SELECTORBAR),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"SelectorBar") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
                    "SelectorBarVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_SELECTORBAR),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"SelectorBar") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
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
            "SelectorBarPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_SELECTORBAR),
            TraceLoggingWideString(info, "Info"));
    }
};
