// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <winmeta.h> // WINEVENT_LEVEL constants
#include <TraceLoggingProvider.h>
#include <telemetry\microsofttelemetry.h>

#include <TraceLoggingActivity.h>

// This will forward-declare the TraceLoggingProvider handle g_hTraceProvider.
// This handle needs to be accessible to any class that wishes to use TraceLogging.
TRACELOGGING_DECLARE_PROVIDER(g_hExtTraceProvider);

