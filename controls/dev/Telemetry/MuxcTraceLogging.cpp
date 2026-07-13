// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include "MuxcTraceLogging.h"

#ifdef MUXCONTROLS_TABULAR
// GUID for Microsoft.UI.Xaml.Controls.Tabular : {80a0b321-304c-576e-c0f8-c593c3e7cf86} (TraceLogging name-hash)
TRACELOGGING_DEFINE_PROVIDER(
    g_hTelemetryProvider,
    TELEMETRY_PROVIDER_NAME,
    (0x80a0b321, 0x304c, 0x576e, 0xc0, 0xf8, 0xc5, 0x93, 0xc3, 0xe7, 0xcf, 0x86));
#else
// GUID for Microsoft.UI.Xaml.Controls : {21e0ae07-56a7-55b5-12f9-011e6bc08cca}
// GUID for Microsoft.UI.Xaml.Controls : {21e0ae07-56a7-55b5-12f9-011e6bc08ccb}
TRACELOGGING_DEFINE_PROVIDER(
    g_hTelemetryProvider,
    TELEMETRY_PROVIDER_NAME,
    (0x21e0ae07, 0x56a7, 0x55b5, 0x12, 0xf9, 0x01, 0x1e, 0x6b, 0xc0, 0x8c, 0xca));
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
    g_IsTelemetryProviderEnabled = !!isEnabled;
    g_TelemetryProviderLevel = level;
    g_TelemetryProviderMatchAnyKeyword = matchAnyKeyword;
}

#ifdef MUXCONTROLS_TABULAR
// GUID for Microsoft.UI.Xaml.Controls.Tabular.Perf : {6e45f683-063c-5216-d430-45381807d781} (TraceLogging name-hash)
TRACELOGGING_DEFINE_PROVIDER(
    g_hPerfProvider,
    PERF_PROVIDER_NAME,
    (0x6e45f683, 0x063c, 0x5216, 0xd4, 0x30, 0x45, 0x38, 0x18, 0x07, 0xd7, 0x81));
#else
// GUID for Microsoft.UI.Xaml.Controls.Perf : {f55f7011-988d-4674-a724-e01b39dc7af6}
// GUID for Microsoft.UI.Xaml.Controls.Perf : {f55f7011-988d-4674-a724-e01b39dc7af7}
TRACELOGGING_DEFINE_PROVIDER(
    g_hPerfProvider,
    PERF_PROVIDER_NAME,
    (0xf55f7011, 0x988d, 0x4674, 0xa7, 0x24, 0xe0, 0x1b, 0x39, 0xdc, 0x7a, 0xf6));
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
    g_IsPerfProviderEnabled = !!isEnabled;
    g_PerfProviderLevel = level;
    g_PerfProviderMatchAnyKeyword = matchAnyKeyword;
}

// GUID for Microsoft.UI.Xaml.Controls.Debug : {afe0ae07-66a7-55bb-12ff-01116bc08c1a}
#ifdef MUXCONTROLS_TABULAR
// GUID for Microsoft.UI.Xaml.Controls.Tabular.Debug : {03dc0588-a8a2-5562-b209-03918bcc6792} (TraceLogging name-hash)
TRACELOGGING_DEFINE_PROVIDER(
    g_hLoggingProvider,
    DEBUG_PROVIDER_NAME,
    (0x03dc0588, 0xa8a2, 0x5562, 0xb2, 0x09, 0x03, 0x91, 0x8b, 0xcc, 0x67, 0x92));
#else
// GUID for Microsoft.UI.Xaml.Controls.Debug :{afe0ae07-66a7-55bb-12ff-01116bc08c1b}
TRACELOGGING_DEFINE_PROVIDER(
    g_hLoggingProvider,
    DEBUG_PROVIDER_NAME,
    (0xafe0ae07, 0x66a7, 0x55bb, 0x12, 0xff, 0x01, 0x11, 0x6b, 0xc0, 0x8c, 0x1a));
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
    g_IsLoggingProviderEnabled = !!isEnabled;
    g_LoggingProviderLevel = level;
    g_LoggingProviderMatchAnyKeyword = matchAnyKeyword;
}

void RegisterTraceLogging()
{
    HRESULT hr = S_OK;

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
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

        g_TelemetryProviderActivityId = GUID_NULL;
    };

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
    TraceLoggingUnregister(g_hTelemetryProvider);

    TraceLoggingUnregister(g_hPerfProvider);
    TraceLoggingUnregister(g_hLoggingProvider);
}
