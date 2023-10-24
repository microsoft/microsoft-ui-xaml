// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "TraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsItemContainerTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_ITEMCONTAINER || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsItemContainerVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_ITEMCONTAINER || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsItemContainerPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_ITEMCONTAINER || g_PerfProviderMatchAnyKeyword == 0);
}

#define ITEMCONTAINER_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
ItemContainerTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define ITEMCONTAINER_TRACE_INFO(sender, message, ...) \
if (IsItemContainerTracingEnabled()) \
{ \
    ITEMCONTAINER_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ItemContainerTrace::s_IsDebugOutputEnabled || ItemContainerTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    ITEMCONTAINER_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define ITEMCONTAINER_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
ItemContainerTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define ITEMCONTAINER_TRACE_VERBOSE(sender, message, ...) \
if (IsItemContainerVerboseTracingEnabled()) \
{ \
    ITEMCONTAINER_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ItemContainerTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    ITEMCONTAINER_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define ITEMCONTAINER_TRACE_PERF(info) \
if (IsItemContainerPerfTracingEnabled()) \
{ \
    ItemContainerTrace::TracePerfInfo(info); \
} \

class ItemContainerTrace
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
                    "ItemContainerInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_ITEMCONTAINER),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ItemContainer") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
                    "ItemContainerVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_ITEMCONTAINER),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ItemContainer") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
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
            "ItemContainerPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_ITEMCONTAINER),
            TraceLoggingWideString(info, "Info"));
    }
};
