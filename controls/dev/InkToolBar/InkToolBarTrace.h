// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "MuxcTraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

// REVIEW:  Not sure how to shoehorn in the Error and warning trace info used by inking.  The controls stuff
//          seems to only deal with INFO and VERBOSE.
#define INKTOOLBAR_TRACE_ERROR INKTOOLBAR_TRACE_INFO
#define INKTOOLBAR_TRACE_WARNING INKTOOLBAR_TRACE_INFO

inline bool IsInkToolBarTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_INKTOOLBAR || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsInkToolBarVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_INKTOOLBAR || g_LoggingProviderMatchAnyKeyword == 0);
}

#define INKTOOLBAR_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
InkToolBarTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define INKTOOLBAR_TRACE_INFO(sender, message, ...) \
if (IsInkToolBarTracingEnabled()) \
{ \
    INKTOOLBAR_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (InkToolBarTrace::s_IsDebugOutputEnabled || InkToolBarTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    INKTOOLBAR_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define INKTOOLBAR_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
InkToolBarTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define INKTOOLBAR_TRACE_VERBOSE(sender, message, ...) \
if (IsInkToolBarVerboseTracingEnabled()) \
{ \
    INKTOOLBAR_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (InkToolBarTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    INKTOOLBAR_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define INKTOOLBAR_TRACE_PERF(info) \
if (IsInkToolBarPerfTracingEnabled()) \
{ \
    InkToolBarTrace::TracePerfInfo(info); \
} \

#ifdef DBG
#define INKTOOLBAR_TRACE_INFO_DBG(sender, message, ...)    INKTOOLBAR_TRACE_INFO(sender, message, __VA_ARGS__)
#define INKTOOLBAR_TRACE_VERBOSE_DBG(sender, message, ...) INKTOOLBAR_TRACE_VERBOSE(sender, message, __VA_ARGS__)
#define INKTOOLBAR_TRACE_PERF_DBG(info)                    INKTOOLBAR_TRACE_PERF(info)
#else
#define INKTOOLBAR_TRACE_INFO_DBG(sender, message, ...)
#define INKTOOLBAR_TRACE_VERBOSE_DBG(sender, message, ...)
#define INKTOOLBAR_TRACE_PERF_DBG(info)
#endif // DBG

class InkToolBarTrace
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
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "InkToolBarInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_INKTOOLBAR),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"InkToolBar") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
        WCHAR buffer[384]{};
        if (SUCCEEDED(StringCchVPrintfW(buffer, ARRAYSIZE(buffer), message, args)))
        {
            if (includeTraceLogging)
            {
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "InkToolBarVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_INKTOOLBAR),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"InkToolBar") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
            {
                globalTestHooks->LogMessage(sender, buffer, true /*isVerboseLevel*/);
            }
        }
        va_end(args);
    }
};
