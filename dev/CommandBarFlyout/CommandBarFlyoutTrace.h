// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MUXControlsTestHooks.h"
#include "TraceLogging.h"
#include "Utils.h"
#include "common.h"

inline bool IsCommandBarFlyoutTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (((g_LoggingProviderMatchAnyKeyword & KEYWORD_COMMANDBARFLYOUT) != 0U) || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsCommandBarFlyoutVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (((g_LoggingProviderMatchAnyKeyword & KEYWORD_COMMANDBARFLYOUT) != 0U) || g_LoggingProviderMatchAnyKeyword == 0);
}

#define COMMANDBARFLYOUT_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
CommandBarFlyoutTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define COMMANDBARFLYOUT_TRACE_INFO(sender, message, ...) \
if (IsCommandBarFlyoutTracingEnabled()) \
{ \
    COMMANDBARFLYOUT_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (CommandBarFlyoutTrace::s_IsDebugOutputEnabled || CommandBarFlyoutTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    COMMANDBARFLYOUT_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define COMMANDBARFLYOUT_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
CommandBarFlyoutTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define COMMANDBARFLYOUT_TRACE_VERBOSE(sender, message, ...) \
if (IsCommandBarFlyoutVerboseTracingEnabled()) \
{ \
    COMMANDBARFLYOUT_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (CommandBarFlyoutTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    COMMANDBARFLYOUT_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define COMMANDBARFLYOUT_TRACE_PERF(info) \
if (IsCommandBarFlyoutPerfTracingEnabled()) \
{ \
    CommandBarFlyoutTrace::TracePerfInfo(info); \
} \

class CommandBarFlyoutTrace
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
                    "CommandBarFlyoutInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_COMMANDBARFLYOUT),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"CommandBarFlyout") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
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
                    "CommandBarFlyoutVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_COMMANDBARFLYOUT),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"CommandBarFlyout") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
            {
                globalTestHooks->LogMessage(sender, buffer, true /*isVerboseLevel*/);
            }
        }
        va_end(args);
    }
};
