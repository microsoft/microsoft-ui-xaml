// Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project
// root for license information.

#pragma once

#include <TraceLoggingInterop.h>
#include <atomic>
#include <cstdint>
#include <evntrace.h>

namespace XcpAllocation
{
    struct AllocationSnapshot
    {
        uint64_t allocCount; // Profiled allocation calls, including failed non-fail-fast calls; excludes reallocations.
        uint64_t allocSize; // Cumulative requested bytes for those allocation calls; excludes reallocations.
        int64_t allocatedMemory; // Nonnegative byte estimate used by the managed-runtime GC heuristic, independent of profiling.
        uint64_t outstandingAllocCount; // Successful profiled allocations minus non-null profiled frees; transitions can cause underflow.
        uint64_t outstandingAllocSize; // Net actual heap bytes (HeapSize), including realloc deltas; transitions can cause underflow.
        bool allocationCountsAvailable; // Profiling was enabled when read; otherwise allocCount and allocSize are zero.
        bool outstandingAllocationCountsAvailable; // Outstanding fields are populated; does not imply a complete live-heap baseline.
        bool memoryTrackingEnabled; // The independent managed-runtime GC allocation heuristic is enabled.
    };

    AllocationSnapshot GetAllocationSnapshot() noexcept;
}

class XamlProfilerLogging : public wil::TraceLoggingProvider
{
    #pragma warning (suppress : 6387)
    IMPLEMENT_TRACELOGGING_CLASS_WITH_MICROSOFT_TELEMETRY_AND_CALLBACK(
        XamlProfilerLogging, "Microsoft-Windows-XAML-Profiler",
        (0xa1b2c3d4, 0xe5f6, 0x4a5b, 0x9c, 0x8d, 0x7e, 0x6f, 0x5a, 0x4b, 0x3c, 0x2d));

public:
    ~XamlProfilerLogging();
};

#ifdef XAMLPROFILER_ENABLED

#include "XamlTelemetry.h"

class CDependencyObject;

uint64_t XamlProfilerGetPeerHandle(_In_opt_ const CDependencyObject* obj) noexcept;

uint64_t XamlProfilerGetCoreSize(_In_opt_ const CDependencyObject* obj) noexcept;

#endif // XAMLPROFILER_ENABLED

class XamlProfilerTracing final : public TelemetryBase
{
    inline static std::atomic<bool> g_profilerEnabled{false};

    #pragma warning (suppress : 6387)
    IMPLEMENT_TELEMETRY_CLASS(XamlProfilerTracing, XamlProfilerLogging);

public:

    static void EnableAllocationProfiling(bool enabled) noexcept
    {
        g_profilerEnabled.store(enabled, std::memory_order_relaxed);
    }

    static bool IsAllocationProfilingEnabled() noexcept
    {
        return g_profilerEnabled.load(std::memory_order_relaxed);
    }

#ifdef XAMLPROFILER_ENABLED

    DEFINE_TRACELOGGING_EVENT_PARAM6(ChildAdded,
        uint64_t, ParentId,
        uint64_t, ChildId,
        uint32_t, Index,
        PCWSTR,   ChildLabel,
        bool,     IsTemplateChild,
        uint64_t, ChildPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM6(ChildInserted,
        uint64_t, ParentId,
        uint64_t, ChildId,
        uint32_t, Index,
        PCWSTR,   ChildLabel,
        bool,     IsTemplateChild,
        uint64_t, ChildPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM2(ChildRemoved,
        uint64_t, ParentId,
        uint64_t, ChildId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM10(ElementEnteredTree,
        uint64_t, ElementId,
        uint64_t, ParentId,
        bool,     IsLive,
        PCWSTR,   Label,
        bool,     IsTemplateChild,
        uint64_t, PeerHandle,
        uint64_t, CoreSize,
        PCWSTR,   SourceFileUri,
        uint32_t, SourceLine,
        uint32_t, SourceColumn,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a UIElement leaves the live tree.
    DEFINE_TRACELOGGING_EVENT_PARAM3(ElementLeftTree,
        uint64_t, ElementId,
        uint64_t, ParentId,
        bool, IsLive,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM2(PeerAssociated,
        uint64_t, ElementId,
        uint64_t, PeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM5(ContentChanged,
        uint64_t, ParentId,
        uint64_t, OldContentId,
        uint64_t, NewContentId,
        PCWSTR,   NewContentLabel,
        uint64_t, NewContentPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM5(PopupOpened,
        uint64_t, PopupId,
        uint64_t, ChildId,
        PCWSTR,   PopupLabel,
        PCWSTR,   ChildLabel,
        uint64_t, ChildPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM2(PopupClosed,
        uint64_t, PopupId,
        uint64_t, ChildId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM5(CompPeerLinked,
        uint64_t, UIElementId,
        uint64_t, CompNodeId,
        uint64_t, ParentCompNodeId,
        PCWSTR,   ElementLabel,
        uint64_t, UIElementPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a UIElement's composition peer is unlinked.
    DEFINE_TRACELOGGING_EVENT_PARAM2(CompPeerUnlinked,
        uint64_t, UIElementId,
        uint64_t, CompNodeId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a comp node child is inserted into a parent comp node.
    DEFINE_TRACELOGGING_EVENT_PARAM2(CompNodeChildInserted,
        uint64_t, ParentCompNodeId,
        uint64_t, ChildCompNodeId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a comp node child is removed from a parent comp node.
    DEFINE_TRACELOGGING_EVENT_PARAM2(CompNodeChildRemoved,
        uint64_t, ParentCompNodeId,
        uint64_t, ChildCompNodeId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a WUC visual is inserted as a child of another WUC visual.
    DEFINE_TRACELOGGING_EVENT_PARAM6(WucVisualChildInserted,
        uint64_t, ParentVisualId,
        uint64_t, ChildVisualId,
        uint64_t, OwnerCompNodeId,
        int32_t,  Index,
        PCWSTR,   VisualType,
        PCWSTR,   Properties,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a WUC visual is removed from its parent's children collection.
    DEFINE_TRACELOGGING_EVENT_PARAM2(WucVisualChildRemoved,
        uint64_t, ParentVisualId,
        uint64_t, ChildVisualId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM1(WucVisualChildrenCleared,
        uint64_t, ParentVisualId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM5(WucVisualRootSet,
        uint64_t, VisualId,
        uint64_t, TargetId,
        uint64_t, OwnerCompNodeId,
        PCWSTR,   VisualType,
        PCWSTR,   Properties,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a composition target/island root is cleared (Root set to null).
    DEFINE_TRACELOGGING_EVENT_PARAM1(WucVisualRootCleared,
        uint64_t, TargetId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM7(DCompSurfaceMemoryChanged,
        uint64_t, SurfaceId,
        bool,     IsAllocation,
        uint64_t, EstimatedSizeBytes,
        uint32_t, Width,
        uint32_t, Height,
        uint32_t, PixelStride,
        bool,     IsVirtual,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM10(DCompSurfaceTextureObserved,
        uint64_t, SurfaceId,
        uint64_t, ResourceId,
        uint32_t, Width,
        uint32_t, Height,
        uint32_t, MipLevels,
        uint32_t, ArraySize,
        uint32_t, Format,
        uint32_t, SampleCount,
        int32_t,  OffsetX,
        int32_t,  OffsetY,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM3(VisualSurfaceChanged,
        uint64_t, VisualId,
        uint64_t, SurfaceId,
        uint32_t, Role,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

#endif // XAMLPROFILER_ENABLED
};

inline void NTAPI XamlProfilerLogging::Callback(
    const GUID*, ULONG control, UCHAR, ULONGLONG, ULONGLONG, EVENT_FILTER_DESCRIPTOR*, void*)
{
    if (control == EVENT_CONTROL_CODE_ENABLE_PROVIDER || control == EVENT_CONTROL_CODE_DISABLE_PROVIDER)
    {
        XamlProfilerTracing::EnableAllocationProfiling(control == EVENT_CONTROL_CODE_ENABLE_PROVIDER);
    }
}

inline XamlProfilerLogging::~XamlProfilerLogging()
{
    XamlProfilerTracing::EnableAllocationProfiling(false);
}

#ifdef XAMLPROFILER_ENABLED

class XamlElementTracing final : public TelemetryBase
{
    #pragma warning (suppress : 6387)
    IMPLEMENT_TELEMETRY_CLASS(XamlElementTracing, XamlTelemetryLogging);

public:

    struct Identity
    {
        uint64_t uiElement = 0;
        uint64_t objectPointer = 0;
    };

    static Identity GetIdentity(
        _In_opt_ CDependencyObject* object,
        _In_opt_ CDependencyObject* relatedUIElement);

    // Resolve relationships only when the event can actually be emitted at its original level.
#define TRACE_ELEMENT_EVENT(OpName, Object, RelatedUIElement, Level, ...) \
    do \
    { \
        if (TraceLoggingProviderEnabled(TraceLoggingType::Provider(), Level, 0)) \
        { \
            const auto identity = GetIdentity(Object, RelatedUIElement); \
            if (identity.uiElement != 0 && identity.objectPointer == 0) \
            { \
                TraceLoggingWrite(TraceLoggingType::Provider(), OpName, \
                    TraceLoggingLevel(Level), \
                    TraceLoggingValue(identity.uiElement, "ElementId"), \
                    __VA_ARGS__); \
            } \
            else if (identity.uiElement != 0) \
            { \
                TraceLoggingWrite(TraceLoggingType::Provider(), OpName, \
                    TraceLoggingLevel(Level), \
                    TraceLoggingValue(identity.uiElement, "ElementId"), \
                    TraceLoggingValue(identity.objectPointer, "objectpointer"), \
                    __VA_ARGS__); \
            } \
            else \
            { \
                TraceLoggingWrite(TraceLoggingType::Provider(), OpName, \
                    TraceLoggingLevel(Level), \
                    TraceLoggingValue(identity.objectPointer, "objectpointer"), \
                    __VA_ARGS__); \
            } \
        } \
    } while (false)

#define DEFINE_ELEMENT_ACTIVITY(OpName, Level) \
    static void OpName##Begin(CDependencyObject* object, CDependencyObject* relatedUIElement = nullptr) \
    { \
        TRACE_ELEMENT_EVENT(#OpName, object, relatedUIElement, Level, \
            TraceLoggingOpcode(WINEVENT_OPCODE_START)); \
    } \
    static void OpName##End(CDependencyObject* object, CDependencyObject* relatedUIElement = nullptr) \
    { \
        TRACE_ELEMENT_EVENT(#OpName, object, relatedUIElement, Level, \
            TraceLoggingOpcode(WINEVENT_OPCODE_STOP)); \
    }

    // Layout-manager passes (CLayoutManager): the subject is the layout root.
    DEFINE_ELEMENT_ACTIVITY(FireLayoutUpdated, WINEVENT_LEVEL_INFO);
    DEFINE_ELEMENT_ACTIVITY(FireSizeChanged, WINEVENT_LEVEL_INFO);
    // Per-element size-changed notification: the subject is the element whose size changed.
    DEFINE_ELEMENT_ACTIVITY(IndividualSizeChanged, WINEVENT_LEVEL_INFO);
    // Root visual set on a visual tree: the subject is the root element.
    DEFINE_ELEMENT_ACTIVITY(PutRootVisual, WINEVENT_LEVEL_INFO);

    // Top-level layout passes (CLayoutManager::UpdateLayout): the subject is the layout root.
    DEFINE_ELEMENT_ACTIVITY(Layout, WINEVENT_LEVEL_INFO);
    DEFINE_ELEMENT_ACTIVITY(Measure, WINEVENT_LEVEL_INFO);
    DEFINE_ELEMENT_ACTIVITY(Arrange, WINEVENT_LEVEL_INFO);

    static void RealizeTransition(CDependencyObject* object)
    {
        TRACE_ELEMENT_EVENT("RealizeTransition", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingOpcode(WINEVENT_OPCODE_INFO));
    }
    DEFINE_ELEMENT_ACTIVITY(CancelTransitions, WINEVENT_LEVEL_INFO);
    DEFINE_ELEMENT_ACTIVITY(ProcessLayoutForTransition, WINEVENT_LEVEL_VERBOSE);

    DEFINE_ELEMENT_ACTIVITY(UpdateFocus, WINEVENT_LEVEL_INFO);
    DEFINE_ELEMENT_ACTIVITY(XYFocusEntered, WINEVENT_LEVEL_INFO);
    // XYFocusEntered adds the retail Begin's focus-direction string on the Begin edge.
    static void XYFocusEnteredBegin(CDependencyObject* object, PCWSTR Direction)
    {
        TRACE_ELEMENT_EVENT("XYFocusEntered", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingValue(Direction, "Direction"));
    }

    DEFINE_ELEMENT_ACTIVITY(GenerateContainer, WINEVENT_LEVEL_INFO);
    static void GenerateContainerBegin()
    {
        TraceLoggingWrite(TraceLoggingType::Provider(), "GenerateContainer",
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingLevel(WINEVENT_LEVEL_INFO));
    }
    DEFINE_ELEMENT_ACTIVITY(GenerateItems, WINEVENT_LEVEL_INFO);
    DEFINE_ELEMENT_ACTIVITY(GenerateMCContainer, WINEVENT_LEVEL_INFO);
    static void GenerateMCContainerBegin(int32_t Index)
    {
        TraceLoggingWrite(TraceLoggingType::Provider(), "GenerateMCContainer",
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingValue(Index, "ItemIndex"));
    }
    DEFINE_ELEMENT_ACTIVITY(PlaceElement, WINEVENT_LEVEL_INFO);
    // PlaceElement adds the retail Begin's data index on the Begin edge.
    static void PlaceElementBegin(CDependencyObject* object, int32_t DataIndex)
    {
        TRACE_ELEMENT_EVENT("PlaceElement", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingValue(DataIndex, "ItemIndex"));
    }
    DEFINE_ELEMENT_ACTIVITY(PrepareContainer, WINEVENT_LEVEL_INFO);
    DEFINE_ELEMENT_ACTIVITY(MeasureChild, WINEVENT_LEVEL_INFO);
    DEFINE_ELEMENT_ACTIVITY(VirtualizationMeasure, WINEVENT_LEVEL_INFO);
    DEFINE_ELEMENT_ACTIVITY(VirtualizationAdd, WINEVENT_LEVEL_INFO);
    DEFINE_ELEMENT_ACTIVITY(VirtualizationCleanup, WINEVENT_LEVEL_INFO);
    DEFINE_ELEMENT_ACTIVITY(UpdateLogicalScrollData, WINEVENT_LEVEL_INFO);
    static void VirtualizationIsEnabledByLayout(CDependencyObject* object, bool IsVirtualizationActive, PCWSTR ElementName, PCWSTR ClassName, PCWSTR ParentClassName)
    {
        TRACE_ELEMENT_EVENT("VirtualizationIsEnabledByLayout", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingValue(IsVirtualizationActive, "IsEnabled"),
            TraceLoggingValue(ElementName, "Name"),
            TraceLoggingValue(ClassName, "ClassType"),
            TraceLoggingValue(ParentClassName, "PropertyType"));
    }
    DEFINE_ELEMENT_ACTIVITY(GetElementCount, WINEVENT_LEVEL_VERBOSE);

    // Control template/style operations (CControl): the subject is the control, not the style.
    DEFINE_ELEMENT_ACTIVITY(GetBuiltInStyle, WINEVENT_LEVEL_INFO);
    // GetBuiltInStyle adds the retail End's resolved style target-type name on the End edge.
    static void GetBuiltInStyleEnd(CDependencyObject* object, PCWSTR StyleName)
    {
        TRACE_ELEMENT_EVENT("GetBuiltInStyle", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingOpcode(WINEVENT_OPCODE_STOP),
            TraceLoggingValue(StyleName, "ClassName"));
    }

    // Hit-testing (CUIElement::BoundsTestEntry): the subject is the subtree being hit-tested.
    DEFINE_ELEMENT_ACTIVITY(HitTest, WINEVENT_LEVEL_INFO);
    static void OuterBoundsStats(CDependencyObject* object, uint32_t RecalculatedCount, uint32_t ReusedCount)
    {
        TRACE_ELEMENT_EVENT("OuterBoundsStats", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingValue(RecalculatedCount, "RecalculatedCount"),
            TraceLoggingValue(ReusedCount, "ReusedCount"));
    }

    static void InnerBoundsStats(CDependencyObject* object, uint32_t RecalculatedCount, uint32_t ReusedCount)
    {
        TRACE_ELEMENT_EVENT("InnerBoundsStats", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingValue(RecalculatedCount, "RecalculatedCount"),
            TraceLoggingValue(ReusedCount, "ReusedCount"));
    }

    static void ContentBoundsStats(CDependencyObject* object, uint32_t RecalculatedCount, uint32_t ReusedCount)
    {
        TRACE_ELEMENT_EVENT("ContentBoundsStats", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingValue(RecalculatedCount, "RecalculatedCount"),
            TraceLoggingValue(ReusedCount, "ReusedCount"));
    }

    static void ChildBoundsStats(CDependencyObject* object, uint32_t RecalculatedCount, uint32_t ReusedCount)
    {
        TRACE_ELEMENT_EVENT("ChildBoundsStats", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingValue(RecalculatedCount, "RecalculatedCount"),
            TraceLoggingValue(ReusedCount, "ReusedCount"));
    }

    // Hub section count (Hub_Partial.cpp): the subject is the Hub whose section count changed.
    static void HubSectionCount(CDependencyObject* object, uint32_t SectionCount)
    {
        TRACE_ELEMENT_EVENT("HubSectionCount", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingValue(SectionCount, "CurrentSectionCount"));
    }

    // Frame navigation (Frame_Partial.cpp): the subject is the Frame, not its content.
    static void FrameNavigated(CDependencyObject* object, PCWSTR Descriptor, uint8_t NavigationMode)
    {
        TRACE_ELEMENT_EVENT("FrameNavigated", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingValue(Descriptor, "PageDescriptor"),
            TraceLoggingValue(NavigationMode, "NavigationMode"));
    }

    static void FrameNavigating(CDependencyObject* object, PCWSTR Descriptor, uint8_t NavigationMode)
    {
        TRACE_ELEMENT_EVENT("FrameNavigating", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingValue(Descriptor, "PageDescriptor"),
            TraceLoggingValue(NavigationMode, "NavigationMode"));
    }

    // Text input scope/keyboard skin (CPasswordBox): the subject is the PasswordBox.
    static void SendInputScopeToRichEdit(CDependencyObject* object, uint32_t InputScope)
    {
        TRACE_ELEMENT_EVENT("SendInputScopeToRichEdit", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingValue(InputScope, "InputScopeValue"));
    }

    static void SetIhmKeyboardSkin(CDependencyObject* object, uint32_t KeyboardSkin)
    {
        TRACE_ELEMENT_EVENT("SetIhmKeyboardSkin", object, nullptr, WINEVENT_LEVEL_INFO,
            TraceLoggingValue(KeyboardSkin, "IKSkinValue"));
    }

    DEFINE_ELEMENT_ACTIVITY(SetupCCC, WINEVENT_LEVEL_INFO);
    // SetupCCC adds the retail Begin's item index on the Begin edge.
    static void SetupCCCBegin(CDependencyObject* object, int32_t ItemIndex, CDependencyObject* relatedUIElement = nullptr)
    {
        TRACE_ELEMENT_EVENT("SetupCCC", object, relatedUIElement, WINEVENT_LEVEL_INFO,
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingValue(ItemIndex, "ItemIndex"));
    }

#undef DEFINE_ELEMENT_ACTIVITY
#undef TRACE_ELEMENT_EVENT
};

#endif // XAMLPROFILER_ENABLED
