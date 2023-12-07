// Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project
// root for license information.

#pragma once

#include <TraceLoggingInterop.h>

// GUID for "Microsoft-Windows-Xaml": {531a35ab-63ce-4bcf-aa98-f88c7a89e455}
DECLARE_TRACELOGGING_CLASS(XamlTelemetryLogging, "Microsoft-Windows-XAML", (0x531a35ab, 0x63ce, 0x4bcf, 0xaa, 0x98, 0xf8, 0x8c, 0x7a, 0x89, 0xe4, 0x55));

class XamlTelemetry final : public TelemetryBase
{
    IMPLEMENT_TELEMETRY_CLASS(XamlTelemetry, XamlTelemetryLogging);

public:

    // Calling into a public API
    DEFINE_TRACELOGGING_EVENT_PARAM4(PublicApiCall,
        bool, IsStart,
        uint64_t, ObjectPointer,
        PCSTR, MethodName,
        uint32_t, HR,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
};
