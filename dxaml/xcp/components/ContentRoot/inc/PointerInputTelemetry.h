// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TraceLoggingInterop.h>

// GUID for "Microsoft-Windows-Xaml": {531a35ab-63ce-4bcf-aa98-f88c7a89e455}
DECLARE_TRACELOGGING_CLASS(PointerInputTelemetryLogging, "Microsoft-Windows-XAML", (0x531a35ab, 0x63ce, 0x4bcf, 0xaa, 0x98, 0xf8, 0x8c, 0x7a, 0x89, 0xe4, 0x55));

class PointerInputTelemetry final : public TelemetryBase
{
    IMPLEMENT_TELEMETRY_CLASS(PointerInputTelemetry, PointerInputTelemetryLogging);

public:

    // Fired when we detect that we're receiving pointer input on a thread where we're already processing pointer
    // input.  This can happen, for example, when an app subscribes to a PointerPressed event and runs a message
    // pump in that event handler.  New pointer input will be sent to Xaml on that inner message pump.
    DEFINE_TRACELOGGING_EVENT_PARAM2(PointerInputReentrancyDetected,
        uint32_t, SupersededMessageId,
        uint32_t, NewMessageId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
};
