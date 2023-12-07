// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "TraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsItemsRepeaterTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_ITEMSREPEATER || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsItemsRepeaterVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_ITEMSREPEATER || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsItemsRepeaterPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_ITEMSREPEATER || g_PerfProviderMatchAnyKeyword == 0);
}

#define ITEMSREPEATER_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
ItemsRepeaterTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define ITEMSREPEATER_TRACE_INFO(sender, message, ...) \
if (IsItemsRepeaterTracingEnabled()) \
{ \
    ITEMSREPEATER_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ItemsRepeaterTrace::s_IsDebugOutputEnabled || ItemsRepeaterTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    ITEMSREPEATER_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define ITEMSREPEATER_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
ItemsRepeaterTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define ITEMSREPEATER_TRACE_VERBOSE(sender, message, ...) \
if (IsItemsRepeaterVerboseTracingEnabled()) \
{ \
    ITEMSREPEATER_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ItemsRepeaterTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    ITEMSREPEATER_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define ITEMSREPEATER_TRACE_PERF(info) \
if (IsItemsRepeaterPerfTracingEnabled()) \
{ \
    ItemsRepeaterTrace::TracePerfInfo(info); \
} \

#ifdef DBG
#define ITEMSREPEATER_TRACE_INFO_DBG(sender, message, ...)    ITEMSREPEATER_TRACE_INFO(sender, message, __VA_ARGS__)
#define ITEMSREPEATER_TRACE_VERBOSE_DBG(sender, message, ...) ITEMSREPEATER_TRACE_VERBOSE(sender, message, __VA_ARGS__)
#define ITEMSREPEATER_TRACE_PERF_DBG(info)                    ITEMSREPEATER_TRACE_PERF(info)
#else
#define ITEMSREPEATER_TRACE_INFO_DBG(sender, message, ...)
#define ITEMSREPEATER_TRACE_VERBOSE_DBG(sender, message, ...)
#define ITEMSREPEATER_TRACE_PERF_DBG(info)
#endif // DBG

class ItemsRepeaterTrace
{
public:
    static bool s_IsDebugOutputEnabled;
    static bool s_IsVerboseDebugOutputEnabled;

    static void TraceInfo(bool includeTraceLogging, const winrt::IInspectable& sender, PCWSTR message, ...) noexcept
    {
        va_list args;
        va_start(args, message);
        WCHAR buffer[384]{};
        if (SUCCEEDED(StringCchVPrintfW(buffer, ARRAYSIZE(buffer), message, args)))
        {
            if (includeTraceLogging)
            {
                // TraceViewers
                // http://toolbox/pef 
                // http://fastetw/index.aspx
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "ItemsRepeaterInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_ITEMSREPEATER),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ItemsRepeater") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
        WCHAR buffer[1024]{};
        const HRESULT hr = StringCchVPrintfW(buffer, ARRAYSIZE(buffer), message, args);
        if (SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            if (includeTraceLogging)
            {
                // TraceViewers
                // http://toolbox/pef 
                // http://fastetw/index.aspx
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "ItemsRepeaterVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_ITEMSREPEATER),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ItemsRepeater") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
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
            "ItemsRepeaterPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_ITEMSREPEATER),
            TraceLoggingWideString(info, "Info"));
    }
};
