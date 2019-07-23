// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "TraceLogging.h"
#include <pch.h>

// GUID_NULL is not defined for us in Windows, so we'll define it here in that case.
#ifdef BUILD_WINDOWS
#ifndef GUID_NULL
struct __declspec(uuid("00000000-0000-0000-0000-000000000000")) GUID_NULL;
#define GUID_NULL __uuidof(struct GUID_NULL)
#endif
#endif

// GUID for Microsoft.UI.Xaml.Controls : {21e0ae07-56a7-55b5-12f9-011e6bc08cca}
// GUID for Windows.UI.Xaml.Controls :{21e0ae07-56a7-55b5-12f9-011e6bc08ccb}
#ifndef BUILD_WINDOWS
TRACELOGGING_DEFINE_PROVIDER(
    g_hTelemetryProvider,
    TELEMETRY_PROVIDER_NAME,
    (0x21e0ae07, 0x56a7, 0x55b5, 0x12, 0xf9, 0x01, 0x1e, 0x6b, 0xc0, 0x8c, 0xca),
    TraceLoggingOptionMicrosoftTelemetry());
#else
TRACELOGGING_DEFINE_PROVIDER(
    g_hTelemetryProvider,
    TELEMETRY_PROVIDER_NAME,
    (0x21e0ae07, 0x56a7, 0x55b5, 0x12, 0xf9, 0x01, 0x1e, 0x6b, 0xc0, 0x8c, 0xcb),
    TraceLoggingOptionMicrosoftTelemetry());
#endif

bool g_IsTelemetryProviderEnabled{};
UCHAR g_TelemetryProviderLevel{};
ULONGLONG g_TelemetryProviderMatchAnyKeyword{};
GUID g_TelemetryProviderActivityId{};

void WINAPI TelemetryProviderEnabledCallback(
    _In_      LPCGUID /*sourceId*/,
    _In_      ULONG isEnabled,
    _In_      UCHAR level,
    _In_      ULONGLONG matchAnyKeyword,
    _In_      ULONGLONG /*matchAllKeywords*/,
    _In_opt_  PEVENT_FILTER_DESCRIPTOR /*filterData*/,
    _In_opt_  PVOID /*callbackContext*/)
{
    g_IsTelemetryProviderEnabled = !(isEnabled == 0u);
    g_TelemetryProviderLevel = level;
    g_TelemetryProviderMatchAnyKeyword = matchAnyKeyword;
}

// GUID for Microsoft.UI.Xaml.Controls.Perf : {f55f7011-988d-4674-a724-e01b39dc7af6}
// GUID for Windows.UI.Xaml.Controls.Perf : {f55f7011-988d-4674-a724-e01b39dc7af7}
#ifndef BUILD_WINDOWS
TRACELOGGING_DEFINE_PROVIDER(
    g_hPerfProvider,
    PERF_PROVIDER_NAME,
    (0xf55f7011, 0x988d, 0x4674, 0xa7, 0x24, 0xe0, 0x1b, 0x39, 0xdc, 0x7a, 0xf6));
#else
TRACELOGGING_DEFINE_PROVIDER(
    g_hPerfProvider,
    PERF_PROVIDER_NAME,
    (0xf55f7011, 0x988d, 0x4674, 0xa7, 0x24, 0xe0, 0x1b, 0x39, 0xdc, 0x7a, 0xf7));
#endif

bool g_IsPerfProviderEnabled{};
UCHAR g_PerfProviderLevel{};
ULONGLONG g_PerfProviderMatchAnyKeyword{};
GUID g_PerfProviderActivityId{};

void WINAPI PerfProviderEnabledCallback(
    _In_      LPCGUID /*sourceId*/,
    _In_      ULONG isEnabled,
    _In_      UCHAR level,
    _In_      ULONGLONG matchAnyKeyword,
    _In_      ULONGLONG /*matchAllKeywords*/,
    _In_opt_  PEVENT_FILTER_DESCRIPTOR /*filterData*/,
    _In_opt_  PVOID /*callbackContext*/)
{
    g_IsPerfProviderEnabled = !(isEnabled == 0u);
    g_PerfProviderLevel = level;
    g_PerfProviderMatchAnyKeyword = matchAnyKeyword;
}

// GUID for Microsoft.UI.Xaml.Controls.Debug : {afe0ae07-66a7-55bb-12ff-01116bc08c1a}
// GUID for Windows.UI.Xaml.Controls.Debug :{afe0ae07-66a7-55bb-12ff-01116bc08c1b}
#ifndef BUILD_WINDOWS
TRACELOGGING_DEFINE_PROVIDER(
    g_hLoggingProvider,
    DEBUG_PROVIDER_NAME,
    (0xafe0ae07, 0x66a7, 0x55bb, 0x12, 0xff, 0x01, 0x11, 0x6b, 0xc0, 0x8c, 0x1a));
#else
TRACELOGGING_DEFINE_PROVIDER(
    g_hLoggingProvider,
    DEBUG_PROVIDER_NAME,
    (0xafe0ae07, 0x66a7, 0x55bb, 0x12, 0xff, 0x01, 0x11, 0x6b, 0xc0, 0x8c, 0x1b));
#endif

bool g_IsLoggingProviderEnabled{};
UCHAR g_LoggingProviderLevel{};
ULONGLONG g_LoggingProviderMatchAnyKeyword{};
GUID g_LoggingProviderActivityId{};

void WINAPI LoggingProviderEnabledCallback(
    _In_      LPCGUID /*sourceId*/,
    _In_      ULONG isEnabled,
    _In_      UCHAR level,
    _In_      ULONGLONG matchAnyKeyword,
    _In_      ULONGLONG /*matchAllKeywords*/,
    _In_opt_  PEVENT_FILTER_DESCRIPTOR /*filterData*/,
    _In_opt_  PVOID /*callbackContext*/)
{
    g_IsLoggingProviderEnabled = !(isEnabled == 0u);
    g_LoggingProviderLevel = level;
    g_LoggingProviderMatchAnyKeyword = matchAnyKeyword;
}

void RegisterTraceLogging()
{
    HRESULT hr = S_OK;

#ifdef EMIT_TELEMETRY_EVENTS
    TraceLoggingRegisterEx(g_hTelemetryProvider, TelemetryProviderEnabledCallback, nullptr);
    //Generate the ActivityId used to track the session
    hr = CoCreateGuid(&g_TelemetryProviderActivityId);
    if (FAILED(hr))
    {
        TraceLoggingWriteActivity(
            g_hTelemetryProvider,
            "CreateGuidError",
            nullptr,
            nullptr,
            TraceLoggingHResult(hr),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

        g_TelemetryProviderActivityId = GUID_NULL;
    };
#endif // EMIT_TELEMETRY_EVENTS

    TraceLoggingRegisterEx(g_hPerfProvider, PerfProviderEnabledCallback, nullptr);
    //Generate the ActivityId used to track the session
    hr = CoCreateGuid(&g_PerfProviderActivityId);
    if (FAILED(hr))
    {
        TraceLoggingWriteActivity(
            g_hPerfProvider,
            "CreateGuidError",
            nullptr,
            nullptr,
            TraceLoggingHResult(hr));

        g_PerfProviderActivityId = GUID_NULL;
    };

    TraceLoggingRegisterEx(g_hLoggingProvider, LoggingProviderEnabledCallback, nullptr);
    //Generate the ActivityId used to track the session
    hr = CoCreateGuid(&g_LoggingProviderActivityId);
    if (FAILED(hr))
    {
        TraceLoggingWriteActivity(
            g_hLoggingProvider,
            "CreateGuidError",
            nullptr,
            nullptr,
            TraceLoggingHResult(hr));

        g_LoggingProviderActivityId = GUID_NULL;
    };
}

void UnRegisterTraceLogging()
{
#ifdef EMIT_TELEMETRY_EVENTS
    TraceLoggingUnregister(g_hTelemetryProvider);
#endif // EMIT_TELEMETRY_EVENTS

    TraceLoggingUnregister(g_hPerfProvider);
    TraceLoggingUnregister(g_hLoggingProvider);
}
