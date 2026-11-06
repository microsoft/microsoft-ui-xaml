// Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project
// root for license information.

#pragma once

#include <TraceLoggingInterop.h>
#include <cstdint>
#include <evntrace.h>
#include <unknwn.h>

namespace XcpAllocation
{
    struct AllocationSnapshot
    {
        int64_t allocCount = -1; // Tracked allocation calls; -1 when unavailable.
        int64_t allocSize = -1; // Cumulative requested bytes, excluding reallocations; -1 when unavailable.
        int64_t allocatedMemory = -1; // Managed-runtime GC byte estimate, independent of profiling; -1 when unavailable.
        int64_t outstandingAllocCount = -1; // Tracked successful allocations minus non-null frees; -1 when unavailable.
        int64_t outstandingAllocSize = -1; // Net actual heap bytes, including realloc deltas; -1 when unavailable.
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

// Instrumentation is compiled in all configurations.
// Event emission is controlled by runtime ETW enablement.

#include "XamlTelemetry.h"

class CDependencyObject;

uint64_t XamlProfilerGetPeerHandle(_In_opt_ const CDependencyObject* obj) noexcept;

class XamlProfilerTracing final : public TelemetryBase
{
    #pragma warning (suppress : 6387)
    IMPLEMENT_TELEMETRY_CLASS(XamlProfilerTracing, XamlProfilerLogging);

public:

    // Shared cached ETW state; querying must not initialize the provider on allocation paths.
    static void SetProfilerEnabled(bool enabled) noexcept;
    static bool IsProfilerEnabled() noexcept;

    static void NotifyChildInserted(_In_opt_ IUnknown* parentVisual, _In_opt_ IUnknown* childVisual);
    static void NotifyChildRemoved(_In_opt_ IUnknown* parentVisual, _In_opt_ IUnknown* childVisual);
    static void NotifyChildrenCleared(_In_opt_ IUnknown* parentVisual);
    static void NotifyRootSet(_In_opt_ IUnknown* visual, uint64_t targetId);
    static void NotifyRootCleared(uint64_t targetId);

    DEFINE_TRACELOGGING_EVENT_PARAM2(PeerAssociated,
        uint64_t, ElementId,
        uint64_t, PeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Reports successful native surface creation before WinRT wrapping. Width/Height exclude
    // gutters; Format/AlphaMode are the actual creation arguments, not backing-texture attributes.
    DEFINE_TRACELOGGING_EVENT_PARAM10(DCompSurfaceInit,
        uint64_t, SurfaceId,
        uint64_t, NativeSurfaceId,
        uint32_t, Width,
        uint32_t, Height,
        uint32_t, Format,
        uint32_t, AlphaMode,
        bool,     IsVirtual,
        bool,     IncludesGutters,
        bool,     RequestAtlas,
        bool,     IsAlphaMask,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM3(GraphicsResourceCreated,
        uint32_t, ResourceKind,
        PCWSTR,   ResourceName,
        bool,     HasLifetime,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM3(GraphicsResourceDestroyed,
        uint32_t, ResourceKind,
        PCWSTR,   ResourceName,
        bool,     HasLifetime,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM5(ChildAdded,
        uint64_t, ParentId,
        uint64_t, ChildId,
        uint32_t, Index,
        bool,     IsTemplateChild,
        uint64_t, ChildPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM5(ChildInserted,
        uint64_t, ParentId,
        uint64_t, ChildId,
        uint32_t, Index,
        bool,     IsTemplateChild,
        uint64_t, ChildPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM2(ChildRemoved,
        uint64_t, ParentId,
        uint64_t, ChildId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM5(ElementEnteredTree,
        uint64_t, ElementId,
        uint64_t, ParentId,
        bool,     IsLive,
        bool,     IsTemplateChild,
        uint64_t, PeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a UIElement leaves the live tree.
    DEFINE_TRACELOGGING_EVENT_PARAM3(ElementLeftTree,
        uint64_t, ElementId,
        uint64_t, ParentId,
        bool, IsLive,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM4(ContentChanged,
        uint64_t, ParentId,
        uint64_t, OldContentId,
        uint64_t, NewContentId,
        uint64_t, NewContentPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM3(PopupOpened,
        uint64_t, PopupId,
        uint64_t, ChildId,
        uint64_t, ChildPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM2(PopupClosed,
        uint64_t, PopupId,
        uint64_t, ChildId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM4(CompPeerLinked,
        uint64_t, UIElementId,
        uint64_t, CompNodeId,
        uint64_t, ParentCompNodeId,
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
    DEFINE_TRACELOGGING_EVENT_PARAM3(WucVisualChildInserted,
        uint64_t, ParentVisualId,
        uint64_t, ChildVisualId,
        PCWSTR,   VisualType,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a WUC visual is removed from its parent's children collection.
    DEFINE_TRACELOGGING_EVENT_PARAM2(WucVisualChildRemoved,
        uint64_t, ParentVisualId,
        uint64_t, ChildVisualId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM1(WucVisualChildrenCleared,
        uint64_t, ParentVisualId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    DEFINE_TRACELOGGING_EVENT_PARAM3(WucVisualRootSet,
        uint64_t, VisualId,
        uint64_t, TargetId,
        PCWSTR,   VisualType,
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

    DEFINE_TRACELOGGING_EVENT_PARAM3(VisualSurfaceChanged,
        uint64_t, VisualId,
        uint64_t, SurfaceId,
        uint32_t, Role,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

};

inline void NTAPI XamlProfilerLogging::Callback(
    const GUID*, ULONG control, UCHAR, ULONGLONG, ULONGLONG, EVENT_FILTER_DESCRIPTOR*, void*)
{
    if (control == EVENT_CONTROL_CODE_ENABLE_PROVIDER || control == EVENT_CONTROL_CODE_DISABLE_PROVIDER)
    {
        XamlProfilerTracing::SetProfilerEnabled(control == EVENT_CONTROL_CODE_ENABLE_PROVIDER);
    }
}

inline XamlProfilerLogging::~XamlProfilerLogging()
{
    XamlProfilerTracing::SetProfilerEnabled(false);
}
