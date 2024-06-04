// Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project
// root for license information.

#pragma once

#include <wil/TraceLogging.h>

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