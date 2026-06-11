# TraceLogging telemetry events

## Table of Contents

- [Syntax](#syntax)
- [Viewing/monitoring telemetry events](#viewingmonitoring-telemetry-events)

## Syntax

A typical event in source will look like this:
```
TraceLoggingWrite(
    g_hTraceProvider,
    "NameOfEvent",
    TraceLoggingDescription("Description of Event"),
    TraceLoggingUInt32(value, "FieldName"[, Optional description]),
    TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
    TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
```

The parameters are:

`g_hTraceProvider`: This is a reference to the provider.  Defined in the module by `TRACELOGGING_DEFINE_PROVIDER` macro.

`"NameOfEvent"`: simply the name of the event as a string literal.  Keep it simple, use alpha-numeric characters.

`TraceLoggingDescription()`: Macro for event description.

`TraceLoggingUInt32()`: (and other types).  Includes the value, string literal for field name, and optional 
description.  You may have multiple fields per event.

`TelemetryPrivacyDataTag()`: The data tag for the event.  This describes how the data will be interpreted.  For this 
project, as a platform and NuGet package, PDT_ProductAndServicePerformance is used.

`TraceLoggingKeyword()`: Ordered by increasing population Telemetry (Insider population), Measures (small percentage 
of retail population), Critical (all retail machines that opt in to telemetry)

## Viewing/monitoring telemetry events

You may use 'Diagnostic Data Viewer' available in the Windows Store to view the telemetry events from this and other 
components.