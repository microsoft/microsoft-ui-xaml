#pragma once

#include <TraceLoggingActivity.h>
#include <TraceLoggingProvider.h>
#include <telemetry\MicrosoftTelemetry.h>

// There are some conflicting definitions between the internal <MicrosoftTelemetry.h> and <wil/TraceLoggingConfig.h>.  The OSS-friendly <MicrosoftTelemetry.h>
// does not conflict because the defines are empty everywhere.  This conflict can be resolved by defining the include-guard for <wil/TraceLoggingConfig.h>.  That
// is how the internal version of WIL handles this conflict.
#define __WIL_TRACELOGGING_CONFIG_H

#include <wil/TraceLogging.h>