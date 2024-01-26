// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WinEventLogLevels.h"
#include <TraceLoggingProvider.h>

// In the OS repo, we want to pick up the published version of MicrosoftTelemetry.h,
// so we'll use the "telemetry/" prefix to differentiate that version from the one
// in our repo.
#include <MicrosoftTelemetry.h>

#pragma warning(push)
#pragma warning(disable: 6387 6553 26400 26401 26409 26439)
#include <wil/TraceLogging.h>
#pragma warning(pop)

// Keywords
#define KEYWORD_ITEMSREPEATER      0x0000000000000001
#define KEYWORD_SCROLLPRESENTER    0x0000000000000002
#define KEYWORD_PTR                0x0000000000000004
#define KEYWORD_SCROLLVIEW         0x0000000000000008
#define KEYWORD_SWIPECONTROL       0x0000000000000010
#define KEYWORD_COMMANDBARFLYOUT   0x0000000000000020
#define KEYWORD_WEBVIEW2           0x0000000000000040
#define KEYWORD_TABVIEW            0x0000000000000080
#define KEYWORD_ITEMSVIEW          0x0000000000000100
#define KEYWORD_ITEMCONTAINER      0x0000000000000200
#define KEYWORD_LINEDFLOWLAYOUT    0x0000000000000400
#define KEYWORD_ANNOTATEDSCROLLBAR 0x0000000000000800
#define KEYWORD_SELECTORBAR            0x0000000000001000

// Common output formats
#define TRACE_MSG_METH L"%s[0x%p]()\n"
#define TRACE_MSG_METH_DBL L"%s[0x%p](%lf)\n"
#define TRACE_MSG_METH_DBL_DBL L"%s[0x%p](%lf, %lf)\n"
#define TRACE_MSG_METH_DBL_INT L"%s[0x%p](%lf, %d)\n"
#define TRACE_MSG_METH_DBL_DBL_INT L"%s[0x%p](%lf, %lf, %d)\n"
#define TRACE_MSG_METH_DBL_DBL_FLT L"%s[0x%p](%lf, %lf, %f)\n"
#define TRACE_MSG_METH_DBL_DBL_STR L"%s[0x%p](%lf, %lf, %s)\n"
#define TRACE_MSG_METH_FLT L"%s[0x%p](%f)\n"
#define TRACE_MSG_METH_FLT_FLT L"%s[0x%p](%f, %f)\n"
#define TRACE_MSG_METH_FLT_FLT_FLT L"%s[0x%p](%f, %f, %f)\n"
#define TRACE_MSG_METH_FLT_FLT_FLT_FLT L"%s[0x%p](%f, %f, %f, %f)\n"
#define TRACE_MSG_METH_FLT_FLT_STR_INT L"%s[0x%p](%f, %f, %s, %d)\n"
#define TRACE_MSG_METH_INT L"%s[0x%p](%d)\n"
#define TRACE_MSG_METH_INT_INT L"%s[0x%p](%d, %d)\n"
#define TRACE_MSG_METH_PTR L"%s[0x%p](0x%p)\n"
#define TRACE_MSG_METH_PTR_PTR L"%s[0x%p](0x%p, 0x%p)\n"
#define TRACE_MSG_METH_PTR_DBL L"%s[0x%p](0x%p, %lf)\n"
#define TRACE_MSG_METH_PTR_INT L"%s[0x%p](0x%p, %d)\n"
#define TRACE_MSG_METH_PTR_STR L"%s[0x%p](0x%p, %s)\n"
#define TRACE_MSG_METH_STR L"%s[0x%p](%s)\n"
#define TRACE_MSG_METH_IND_STR L"%s[0x%p](%*s)\n"
#define TRACE_MSG_METH_IND_STR_STR L"%s[0x%p](%*s, %s)\n"
#define TRACE_MSG_METH_IND_STR_STR_INT L"%s[0x%p](%*s, %s, %d)\n"
#define TRACE_MSG_METH_IND_STR_STR_FLT L"%s[0x%p](%*s, %s, %f)\n"
#define TRACE_MSG_METH_IND_STR_STR_FLT_FLT L"%s[0x%p](%*s, %s, %f, %f)\n"
#define TRACE_MSG_METH_IND_STR_STR_FLT_FLT_FLT_FLT L"%s[0x%p](%*s, %s, %f, %f, %f, %f)\n"
#define TRACE_MSG_METH_IND_STR_STR_INT_FLT_FLT_FLT_FLT L"%s[0x%p](%*s, %s, %d, %f, %f, %f, %f)\n"
#define TRACE_MSG_METH_STR_STR L"%s[0x%p](%s, %s)\n"
#define TRACE_MSG_METH_STR_DBL L"%s[0x%p](%s, %lf)\n"
#define TRACE_MSG_METH_STR_DBL_DBL L"%s[0x%p](%s, %lf, %lf)\n"
#define TRACE_MSG_METH_STR_FLT L"%s[0x%p](%s, %f)\n"
#define TRACE_MSG_METH_STR_INT L"%s[0x%p](%s, %d)\n"
#define TRACE_MSG_METH_STR_STR_INT L"%s[0x%p](%s, %s, %d)\n"
#define TRACE_MSG_METH_STR_STR_STR L"%s[0x%p](%s, %s, %s)\n"
#define TRACE_MSG_METH_STR_INT_INT L"%s[0x%p](%s, %d, %d)\n"
#define TRACE_MSG_METH_STR_FLT_FLT L"%s[0x%p](%s, %f, %f)\n"
#define TRACE_MSG_METH_STR_STR_FLT_FLT L"%s[0x%p](%s, %s, %f, %f)\n"
#define TRACE_MSG_METH_STR_STR_FLT_FLT_FLT_FLT L"%s[0x%p](%s, %s, %f, %f, %f, %f)\n"
#define TRACE_MSG_METH_STR_STR_FLT L"%s[0x%p](%s, %s, %f)\n"
#define TRACE_MSG_METH_STR_STR_INT_INT L"%s[0x%p](%s, %s, %d, %d)\n"

#define TRACE_MSG_METH_METH L"%s[0x%p] - calls %s()\n"
#define TRACE_MSG_METH_METH_INT L"%s[0x%p] - calls %s(%d)\n"
#define TRACE_MSG_METH_METH_STR L"%s[0x%p] - calls %s(%s)\n"
#define TRACE_MSG_METH_METH_STR_STR L"%s[0x%p] - calls %s(%s, %s)\n"
#define TRACE_MSG_METH_METH_FLT_STR L"%s[0x%p] - calls %s(%f, %s)\n"
#define TRACE_MSG_METH_METH_FLT_FLT_FLT L"%s[0x%p] - calls %s(%f, %f, %f)\n"

// Current method name
#define METH_NAME StringUtil::Utf8ToUtf16(__FUNCTION__).c_str()

// TraceLogging provider name for telemetry.
#define TELEMETRY_PROVIDER_NAME "Microsoft.UI.Xaml.Controls"

TRACELOGGING_DECLARE_PROVIDER(g_hTelemetryProvider);
extern bool g_IsTelemetryProviderEnabled;
extern UCHAR g_TelemetryProviderLevel;
extern ULONGLONG g_TelemetryProviderMatchAnyKeyword;
extern GUID g_TelemetryProviderActivityId;

// TraceLogging provider name for performance.
#define PERF_PROVIDER_NAME "Microsoft.UI.Xaml.Controls.Perf"

TRACELOGGING_DECLARE_PROVIDER(g_hPerfProvider);
extern bool g_IsPerfProviderEnabled;
extern UCHAR g_PerfProviderLevel;
extern ULONGLONG g_PerfProviderMatchAnyKeyword;
extern GUID g_PerfProviderActivityId;

// TraceLogging provider name for debugging.
#define DEBUG_PROVIDER_NAME "Microsoft.UI.Xaml.Controls.Debug"

TRACELOGGING_DECLARE_PROVIDER(g_hLoggingProvider);
extern bool g_IsLoggingProviderEnabled;
extern UCHAR g_LoggingProviderLevel;
extern ULONGLONG g_LoggingProviderMatchAnyKeyword;
extern GUID g_LoggingProviderActivityId;

extern void RegisterTraceLogging();
extern void UnRegisterTraceLogging();

#pragma warning(push)
#pragma warning(disable: 6387)
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

    // An event that the perf_xaml plugin cares about
    //
    // perf_xaml is a WPA plugin that looks at Xaml events and picks out important ones to provide an overview of Xaml
    // UI thread performance. One of its most helpful features is the ability to match start and stop events and
    // automatically calculate a duration for event pairs. This lets you quickly see information like the durations of
    // all frames that Xaml rendered without having to comb through events and manually subtract timestamps. Xaml logs
    // too many kinds of events for the plugin to automatically match up and calculate everything, so the plugin has a
    // hardcoded list of important events that it looks for. The hardcoded list means that if Xaml wanted to log new
    // events that are relevant to perf, the plugin must also be updated to teach it about those new events.
    //
    // This PerfXamlEvent is meant to solve that problem. The PerfXamlEvent is part of the hardcoded list that the
    // plugin looks for. The plugin also knows that this event is special, and carries a string (the "EventName" param)
    // that describes the operation being traced. The plugin then matches up PerfXamlEvent pairs and displays the string
    // directly in the table that it produces. For Xaml this means adding a new row in the plugin's Xaml Frame Analysis
    // table just involves logging a pair of PerfXamlEvent events and providing a string for the operation that the
    // event is meant to trace.
    //
    // This event also carries an "IsInteresting" bool param. The perf_xaml plugin has a concept of regions of interest,
    // which span from some triggering event to the end of the next UI thread frame. Regions of interest are meant to
    // mark large changes in the visual tree, which produce long-running frames that are the most susceptible to perf
    // problems. One example is page navigation - navigating to a new page brings in an entire tree of elements that
    // need to run layout and render, so the next UI thread frame is one that we'll want to pay particularly close
    // attention to. A PerfXamlEvent with "IsInteresting" true will tell perf_xaml to start a region of interest, if
    // we're not already in one.
    DEFINE_TRACELOGGING_EVENT_PARAM4(PerfXamlEvent,
        bool, IsStart,
        uint64_t, ObjectPointer,
        PCSTR, EventName,
        bool, IsInteresting,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
};
#pragma warning(pop)

struct PerfXamlEvent_RAII
{
public:
    PerfXamlEvent_RAII(uint64_t objectPointer, PCSTR eventName, bool isInteresting)
        : m_objectPointer(objectPointer)
        , m_eventName(eventName)
        , m_isInteresting(isInteresting)
    {
        XamlTelemetry::PerfXamlEvent(true, m_objectPointer, m_eventName, m_isInteresting);
    }

    ~PerfXamlEvent_RAII()
    {
        XamlTelemetry::PerfXamlEvent(false, m_objectPointer, m_eventName, m_isInteresting);
    }

    // Disallow copying/moving
    PerfXamlEvent_RAII(const PerfXamlEvent_RAII&) = delete;
    PerfXamlEvent_RAII(PerfXamlEvent_RAII&&) = delete;
    PerfXamlEvent_RAII& operator=(const PerfXamlEvent_RAII&) = delete;
    PerfXamlEvent_RAII& operator=(PerfXamlEvent_RAII&&) = delete;

private:
    uint64_t m_objectPointer;
    PCSTR m_eventName;
    bool m_isInteresting;
};
