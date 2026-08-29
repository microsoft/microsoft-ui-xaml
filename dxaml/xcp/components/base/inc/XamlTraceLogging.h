// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <winmeta.h> // WINEVENT_LEVEL constants

// We need to undefine DECLSPEC_SAFEBUFFERS for Telemetry. __declspec(safebuffers)
// disables the stack protector compiler switch, /GS. For more info on this switch,
// see msdn: https://msdn.microsoft.com/en-us/library/8dbf701c.aspx
#ifdef DECLSPEC_SAFEBUFFERS
#undef DECLSPEC_SAFEBUFFERS
#endif

#include <TraceLoggingProvider.h>
#include <telemetry\microsofttelemetry.h>
#include <TraceLoggingActivity.h>

// This will forward-declare the TraceLoggingProvider handle g_hTraceProvider.
// This handle needs to be accessible to any class that wishes to use TraceLogging.
TRACELOGGING_DECLARE_PROVIDER(g_hTraceProvider);

