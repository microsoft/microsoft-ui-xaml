// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Hack to work around differences between RS5 and Win11+ versions of TraceLoggingActivity.h.  Since RS5 the _TlgActivityDecl was renamed to _tlgActivityDecl
// (the T is now lowercase).  We are using a recent version of WIL but are locked on the RS5 SDK.  Use a define to map one to the other to make the compiler happy.
// This list of re-defines is the minimal set needed to get our usage of tracelogging to compile.
//
// First include TraceLoggingActivity.h and TraceLoggingProvider.h to get the implmented macros.  Then redefine.  Lastly include <wil/TraceLogging.h> to provide
// the API that everyone actually uses.
#include <TraceLoggingActivity.h>
#include <TraceLoggingProvider.h>
#define _tlgActivityDecl _TlgActivityDecl
#define _tlgActivity_Keyword _TlgActivity_Keyword
#define _tlgActivity_Level _TlgActivity_Level
#define _tlgActivityPrivacyTag _TlgActivityPrivacyTag
#define _tlgActivityRef _TlgActivityRef
#define _tlg_FOREACH _TLG_FOREACH
#define _tlgKeywordVal _TlgKeywordVal
#define _tlgLevelVal _TlgLevelVal
#define _tlg_DefineProvider_annotation _TlgDefineProvider_annotation

#include <telemetry\MicrosoftTelemetry.h>

// There are some conflicting definitions between the internal <MicrosoftTelemetry.h> and <wil/TraceLoggingConfig.h>.  The OSS-friendly <MicrosoftTelemetry.h>
// does not conflict because the defines are empty everywhere.  This conflict can be resolved by defining the include-guard for <wil/TraceLoggingConfig.h>.  That
// is how the internal version of WIL handles this conflict.
#define __WIL_TRACELOGGING_CONFIG_H

#include <wil/TraceLogging.h>