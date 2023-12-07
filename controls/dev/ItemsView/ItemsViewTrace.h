// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "TraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsItemsViewTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_ITEMSVIEW || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsItemsViewVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_ITEMSVIEW || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsItemsViewPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_ITEMSVIEW || g_PerfProviderMatchAnyKeyword == 0);
}

#define ITEMSVIEW_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
ItemsViewTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define ITEMSVIEW_TRACE_INFO(sender, message, ...) \
if (IsItemsViewTracingEnabled()) \
{ \
    ITEMSVIEW_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ItemsViewTrace::s_IsDebugOutputEnabled || ItemsViewTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    ITEMSVIEW_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define ITEMSVIEW_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
ItemsViewTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define ITEMSVIEW_TRACE_VERBOSE(sender, message, ...) \
if (IsItemsViewVerboseTracingEnabled()) \
{ \
    ITEMSVIEW_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ItemsViewTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    ITEMSVIEW_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define ITEMSVIEW_TRACE_PERF(info) \
if (IsItemsViewPerfTracingEnabled()) \
{ \
    ItemsViewTrace::TracePerfInfo(info); \
} \

#ifdef DBG
#define ITEMSVIEW_TRACE_INFO_DBG(sender, message, ...)    ITEMSVIEW_TRACE_INFO(sender, message, __VA_ARGS__)
#define ITEMSVIEW_TRACE_VERBOSE_DBG(sender, message, ...) ITEMSVIEW_TRACE_VERBOSE(sender, message, __VA_ARGS__)
#define ITEMSVIEW_TRACE_PERF_DBG(info)                    ITEMSVIEW_TRACE_PERF(info)
#else
#define ITEMSVIEW_TRACE_INFO_DBG(sender, message, ...)
#define ITEMSVIEW_TRACE_VERBOSE_DBG(sender, message, ...)
#define ITEMSVIEW_TRACE_PERF_DBG(info)
#endif // DBG

class ItemsViewTrace
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
                    "ItemsViewInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_ITEMSVIEW),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ItemsView") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
                    "ItemsViewVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_ITEMSVIEW),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ItemsView") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
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
            "ItemsViewPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_ITEMSVIEW),
            TraceLoggingWideString(info, "Info"));
    }
};
