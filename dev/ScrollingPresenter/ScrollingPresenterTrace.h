// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "TraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsScrollingPresenterTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_SCROLLER || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsScrollingPresenterVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_SCROLLER || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsScrollingPresenterPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_SCROLLER || g_PerfProviderMatchAnyKeyword == 0);
}

#define SCROLLER_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
ScrollingPresenterTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SCROLLER_TRACE_INFO(sender, message, ...) \
if (IsScrollingPresenterTracingEnabled()) \
{ \
    SCROLLER_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ScrollingPresenterTrace::s_IsDebugOutputEnabled || ScrollingPresenterTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    SCROLLER_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SCROLLER_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
ScrollingPresenterTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SCROLLER_TRACE_VERBOSE(sender, message, ...) \
if (IsScrollingPresenterVerboseTracingEnabled()) \
{ \
    SCROLLER_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ScrollingPresenterTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    SCROLLER_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SCROLLER_TRACE_PERF(info) \
if (IsScrollingPresenterPerfTracingEnabled()) \
{ \
    ScrollingPresenterTrace::TracePerfInfo(info); \
} \

class ScrollingPresenterTrace
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
                    "ScrollingPresenterInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_SCROLLER),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ScrollingPresenter") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
        HRESULT hr = StringCchVPrintfW(buffer, ARRAYSIZE(buffer), message, args);
        if (SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            if (includeTraceLogging)
            {
                // TraceViewers
                // http://toolbox/pef 
                // http://fastetw/index.aspx
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "ScrollingPresenterVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_SCROLLER),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"ScrollingPresenter") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
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
            "ScrollingPresenterPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_SCROLLER),
            TraceLoggingWideString(info, "Info"));
    }
};
