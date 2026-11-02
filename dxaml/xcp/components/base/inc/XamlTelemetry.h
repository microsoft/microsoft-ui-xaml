// Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project
// root for license information.

#pragma once

#include <TraceLoggingInterop.h>

// Uncomment to trace allocations via ETW
//#define TRACE_ALLOC 1

// Uncomment to trace parser commands via ETW
//#define TRACE_PARSER_COMMANDS 1

// Uncomment to trace classic Bindings via ETW
//#define TRACE_BINDINGS 1

// Uncomment to trace events via ETW
//#define TRACE_EVENTS 1

// Uncomment to trace ResourceDictionary lookups via ETW
//#define TRACE_RESOURCELOOKUPS 1

// GUID for "Microsoft-Windows-Xaml": {531a35ab-63ce-4bcf-aa98-f88c7a89e455}
#pragma warning (suppress : 6387) // param 1 could be null warning from inside wil\Telemetry.
DECLARE_TRACELOGGING_CLASS(XamlTelemetryLogging, "Microsoft-Windows-XAML", (0x531a35ab, 0x63ce, 0x4bcf, 0xaa, 0x98, 0xf8, 0x8c, 0x7a, 0x89, 0xe4, 0x55));

class CDependencyObject;

class XamlTelemetry final : public TelemetryBase
{
    #pragma warning (suppress : 6387) // param 1 could be null warning from inside wil\Telemetry.
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

    // Creating a DesktopWindowImpl. This is separate from PerfXamlEvent because perf_xaml uses it to define Xaml
    // desktop apps (as opposed to island apps). Desktop apps should measure startup from process start, whereas island
    // apps should measure startup from when the app initialized Xaml. An island app has non-Xaml content and might run
    // for a long time before it needs Xaml, whereas a Xaml desktop app has only Xaml content.
    //
    // Note that the CreateWindow event isn't good enough. It's hooked up to DXamlCore::ConfigureJupiterWindow, which
    // exists for both island and desktop apps.
    DEFINE_TRACELOGGING_EVENT_PARAM2(CreateDesktopWindow,
        bool, IsStart,
        uint64_t, ObjectPointer,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Element-scoped events retain their operation payload and add typed object identities.
    enum : uint64_t
    {
        CoreKeyword = 0x4,
        LayoutKeyword = 0x40,
        ControlsKeyword = 0x80,
        AppmodelKeyword = 0x100
    };

    struct Identity
    {
        uint64_t uiElement = 0;
        uint64_t objectPointer = 0;
    };

    static Identity GetIdentity(
        _In_opt_ CDependencyObject* object,
        _In_opt_ CDependencyObject* relatedUIElement);

    // Resolve relationships only when the event can actually be emitted at its original level.
#define TRACE_ELEMENT_EVENT(OpName, Object, RelatedUIElement, Level, Keyword, ...) \
    do \
    { \
        if (TraceLoggingProviderEnabled(TraceLoggingType::Provider(), Level, Keyword)) \
        { \
            const auto identity = GetIdentity(Object, RelatedUIElement); \
            if (identity.uiElement != 0 && identity.objectPointer == 0) \
            { \
                TraceLoggingWrite(TraceLoggingType::Provider(), OpName, \
                    TraceLoggingLevel(Level), \
                    TraceLoggingKeyword(Keyword), \
                    TraceLoggingValue(identity.uiElement, "ElementId"), \
                    __VA_ARGS__); \
            } \
            else if (identity.uiElement != 0) \
            { \
                TraceLoggingWrite(TraceLoggingType::Provider(), OpName, \
                    TraceLoggingLevel(Level), \
                    TraceLoggingKeyword(Keyword), \
                    TraceLoggingValue(identity.uiElement, "ElementId"), \
                    TraceLoggingValue(identity.objectPointer, "objectpointer"), \
                    __VA_ARGS__); \
            } \
            else \
            { \
                TraceLoggingWrite(TraceLoggingType::Provider(), OpName, \
                    TraceLoggingLevel(Level), \
                    TraceLoggingKeyword(Keyword), \
                    TraceLoggingValue(identity.objectPointer, "objectpointer"), \
                    __VA_ARGS__); \
            } \
        } \
    } while (false)

    // Emits a Begin/End activity pair using ETW Start/Stop opcodes under one task (== OpName),
    // so the consumer pairs them into a duration exactly like the retail Begin/End.
    // End always takes a subject; GenerateContainer also has a parameterless Begin.
#define DEFINE_ELEMENT_ACTIVITY(OpName, Level, Keyword) \
    static void OpName##Begin(CDependencyObject* object, CDependencyObject* relatedUIElement = nullptr) \
    { \
        TRACE_ELEMENT_EVENT(#OpName, object, relatedUIElement, Level, Keyword, \
            TraceLoggingOpcode(WINEVENT_OPCODE_START)); \
    } \
    static void OpName##End(CDependencyObject* object, CDependencyObject* relatedUIElement = nullptr) \
    { \
        TRACE_ELEMENT_EVENT(#OpName, object, relatedUIElement, Level, Keyword, \
            TraceLoggingOpcode(WINEVENT_OPCODE_STOP)); \
    }

    // Items virtualization (dxaml projection layer): the subject is the container/panel.
    // GenerateContainer carries an identity on its End: the container is only realized near the
    // end of the generate call; CopyTo keeps it live through Cleanup.
    DEFINE_ELEMENT_ACTIVITY(GenerateContainer, WINEVENT_LEVEL_INFO, LayoutKeyword);
    static void GenerateContainerBegin()
    {
        TraceLoggingWrite(TraceLoggingType::Provider(), "GenerateContainer",
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(LayoutKeyword));
    }
    DEFINE_ELEMENT_ACTIVITY(GenerateItems, WINEVENT_LEVEL_INFO, CoreKeyword);
    // GenerateMCContainer carries the retail item-collection index on Begin and the container
    // identity on End, once the container is available.
    DEFINE_ELEMENT_ACTIVITY(GenerateMCContainer, WINEVENT_LEVEL_INFO, CoreKeyword);
    static void GenerateMCContainerBegin(int32_t Index)
    {
        TraceLoggingWrite(TraceLoggingType::Provider(), "GenerateMCContainer",
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(CoreKeyword),
            TraceLoggingValue(Index, "ItemIndex"));
    }
    DEFINE_ELEMENT_ACTIVITY(PlaceElement, WINEVENT_LEVEL_INFO, CoreKeyword);
    // PlaceElement adds the retail Begin's data index on the Begin edge.
    static void PlaceElementBegin(CDependencyObject* object, int32_t DataIndex)
    {
        TRACE_ELEMENT_EVENT("PlaceElement", object, nullptr, WINEVENT_LEVEL_INFO, CoreKeyword,
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingValue(DataIndex, "ItemIndex"));
    }
    DEFINE_ELEMENT_ACTIVITY(PrepareContainer, WINEVENT_LEVEL_INFO, LayoutKeyword);
    DEFINE_ELEMENT_ACTIVITY(MeasureChild, WINEVENT_LEVEL_INFO, LayoutKeyword);
    DEFINE_ELEMENT_ACTIVITY(VirtualizationMeasure, WINEVENT_LEVEL_INFO, LayoutKeyword);
    // OrientedVirtualizingPanel-scoped virtualization bookkeeping (CleanupContainers,
    // UpdateLogicalScrollData: the subject is the panel itself). VirtualizationAdd is
    // per-container: the subject is the container being added.
    DEFINE_ELEMENT_ACTIVITY(VirtualizationAdd, WINEVENT_LEVEL_INFO, LayoutKeyword);
    DEFINE_ELEMENT_ACTIVITY(VirtualizationCleanup, WINEVENT_LEVEL_INFO, LayoutKeyword);
    DEFINE_ELEMENT_ACTIVITY(UpdateLogicalScrollData, WINEVENT_LEVEL_INFO, LayoutKeyword);
    DEFINE_ELEMENT_ACTIVITY(GetElementCount, WINEVENT_LEVEL_VERBOSE, CoreKeyword);

    // Hub section count (Hub_Partial.cpp): the subject is the Hub whose section count changed.
    static void HubSectionCount(CDependencyObject* object, uint32_t SectionCount)
    {
        TRACE_ELEMENT_EVENT("HubSectionCount", object, nullptr, WINEVENT_LEVEL_INFO, ControlsKeyword,
            TraceLoggingValue(SectionCount, "CurrentSectionCount"));
    }

    // Frame navigation (Frame_Partial.cpp): the subject is the Frame, not its content.
    static void FrameNavigated(CDependencyObject* object, PCWSTR Descriptor, uint8_t NavigationMode)
    {
        TRACE_ELEMENT_EVENT("FrameNavigated", object, nullptr, WINEVENT_LEVEL_INFO, AppmodelKeyword,
            TraceLoggingValue(Descriptor, "PageDescriptor"),
            TraceLoggingValue(NavigationMode, "NavigationMode"));
    }

    static void FrameNavigating(CDependencyObject* object, PCWSTR Descriptor, uint8_t NavigationMode)
    {
        TRACE_ELEMENT_EVENT("FrameNavigating", object, nullptr, WINEVENT_LEVEL_INFO, AppmodelKeyword,
            TraceLoggingValue(Descriptor, "PageDescriptor"),
            TraceLoggingValue(NavigationMode, "NavigationMode"));
    }

    // Container-content-changing setup (ListViewBase_Partial_ContainerPhase.cpp): the subject is
    // the container being set up.
    DEFINE_ELEMENT_ACTIVITY(SetupCCC, WINEVENT_LEVEL_INFO, ControlsKeyword);
    // SetupCCC adds the retail Begin's item index on the Begin edge.
    static void SetupCCCBegin(CDependencyObject* object, int32_t ItemIndex, CDependencyObject* relatedUIElement = nullptr)
    {
        TRACE_ELEMENT_EVENT("SetupCCC", object, relatedUIElement, WINEVENT_LEVEL_INFO, ControlsKeyword,
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingValue(ItemIndex, "ItemIndex"));
    }

#undef DEFINE_ELEMENT_ACTIVITY
#undef TRACE_ELEMENT_EVENT

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