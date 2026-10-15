// Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project
// root for license information.

#pragma once

#include <TraceLoggingInterop.h>

// XamlProfiler tree-tracking instrumentation is compiled into chk/Debug builds only
// (XAMLPROFILER_ENABLED is defined alongside DBG when Configuration==Debug; see
// Xaml.Cpp.Targets / LibraryCompile.props). In retail/fre builds this entire header is
// empty, and the matching WucVisualTreeProfiler.cpp is excluded from the build, so the
// profiler adds zero code/size to shipping binaries. Every consumer #includes this header
// (and its call sites) inside its own #ifdef XAMLPROFILER_ENABLED, matching the
// SwipeTestHooks convention.
#ifdef XAMLPROFILER_ENABLED

#include "XamlTelemetry.h"

class CDependencyObject;

// Returns the IInspectable identity handle of the object's DXaml peer - the same value
// XAML Diagnostics (and the WinUISnoop tap) use as an InstanceHandle to identify a live
// element - or 0 when the object is null or has no peer yet. This lets an out-of-process
// profiler bridge a tree node back to the live element so the element can be highlighted
// in the target app. The core pointer IDs emitted alongside it are kept as-is, since those
// are what stitch the logical/visual/composition trees together. Defined in uielement.cpp.
uint64_t XamlProfilerGetPeerHandle(_In_opt_ const CDependencyObject* obj) noexcept;

// Returns the actual heap-allocation size (in bytes) of the core object — its concrete,
// most-derived C++ instance footprint (NOT sizeof(CUIElement), which is the base-class size and
// identical for every element). Core objects go through the global operator new, so the allocator
// records the true block size; the size is recovered from the debug allocator's validation header
// in debug builds (or HeapSize in retail) for every creation path. This is only the fixed per-type
// "core block"; the element's sparse property table and its DXaml peer are separate allocations,
// measured on demand. 0 when obj is null. Defined in uielement.cpp alongside XamlProfilerGetPeerHandle.
uint64_t XamlProfilerGetCoreSize(_In_opt_ const CDependencyObject* obj) noexcept;

// TraceLogging provider for XAML Profiler tree-tracking events.
// These events allow an out-of-process profiler to reconstruct and diff the logical tree,
// visual tree, and composition tree over the lifetime of the host process.
//
// Provider GUID: {A1B2C3D4-E5F6-4A5B-9C8D-7E6F5A4B3C2D}
// Provider Name: "Microsoft-Windows-XAML-Profiler"

#pragma warning (suppress : 6387)
DECLARE_TRACELOGGING_CLASS(XamlProfilerLogging, "Microsoft-Windows-XAML-Profiler",
    (0xa1b2c3d4, 0xe5f6, 0x4a5b, 0x9c, 0x8d, 0x7e, 0x6f, 0x5a, 0x4b, 0x3c, 0x2d));

class XamlProfilerTracing final : public TelemetryBase
{
    #pragma warning (suppress : 6387)
    IMPLEMENT_TELEMETRY_CLASS(XamlProfilerTracing, XamlProfilerLogging);

public:

    // =====================================================================
    // Logical Tree Events
    // =====================================================================

    // Fired when a child UIElement is added to a parent's children collection.
    // ChildLabel is the child's GetDebugLabel() — x:Name if set, else class name (e.g. "Button", "OkButton").
    // IsTemplateChild is true when the child was created by a ControlTemplate (GetTemplatedParent() != null).
    // ChildPeerHandle is the child's DXaml-peer InstanceHandle (0 when no peer) for live highlighting.
    DEFINE_TRACELOGGING_EVENT_PARAM6(ChildAdded,
        uint64_t, ParentId,
        uint64_t, ChildId,
        uint32_t, Index,
        PCWSTR,   ChildLabel,
        bool,     IsTemplateChild,
        uint64_t, ChildPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a child UIElement is inserted at a specific index.
    // ChildLabel is the child's GetDebugLabel() — x:Name if set, else class name.
    // IsTemplateChild is true when the child was created by a ControlTemplate (GetTemplatedParent() != null).
    // ChildPeerHandle is the child's DXaml-peer InstanceHandle (0 when no peer) for live highlighting.
    DEFINE_TRACELOGGING_EVENT_PARAM6(ChildInserted,
        uint64_t, ParentId,
        uint64_t, ChildId,
        uint32_t, Index,
        PCWSTR,   ChildLabel,
        bool,     IsTemplateChild,
        uint64_t, ChildPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a child UIElement is removed from a parent's children collection.
    // No label — the consumer already has it from the original ChildAdded/ChildInserted event.
    DEFINE_TRACELOGGING_EVENT_PARAM2(ChildRemoved,
        uint64_t, ParentId,
        uint64_t, ChildId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a UIElement enters the live tree.
    // Label is the element's GetDebugLabel().
    // IsTemplateChild is true when the element was created by a ControlTemplate (GetTemplatedParent() != null).
    // PeerHandle is the element's DXaml-peer InstanceHandle (0 when no peer) for live highlighting.
    // CoreSize is the fixed core-object allocation size; sparse properties and the DXaml peer are separate.
    // SourceFileUri/Line/Column are populated when source-info storage is enabled and the peer has source info.
    // Missing source info uses an empty URI and 0 coordinates; XBF without line info also uses 0 coordinates.
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

    // Snapshot of XAML allocator-owned CPU heap blocks. This excludes GPU texture backing and
    // process allocations owned by other components/runtimes.
    DEFINE_TRACELOGGING_EVENT_PARAM7(XamlHeapSnapshot,
        uint64_t, HeapHandle,
        bool,     IsPrivateHeap,
        uint64_t, OutstandingBytes,
        uint64_t, OutstandingAllocationCount,
        uint64_t, CumulativeAllocatedBytes,
        uint64_t, CumulativeAllocationCount,
        uint64_t, CumulativeDeallocationCount,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // =====================================================================
    // Peer Association Events
    // =====================================================================

    // Fired when a DependencyObject's DXaml framework peer is created and associated with the
    // core object (DXamlCore::CreateAggregableDO). Lazy-peer elements (e.g. ContentPresenter,
    // ClientAreaPresenter) have no peer when ElementEnteredTree/ChildAdded fire, so their
    // PeerHandle is traced as 0 there; this event back-fills the real InstanceHandle the moment
    // the peer exists, letting the consumer make those nodes live-highlightable after the fact.
    // ElementId is the core CDependencyObject* (the same id stitched across all other events);
    // PeerHandle is the peer's IInspectable identity (the XAML Diagnostics InstanceHandle).
    DEFINE_TRACELOGGING_EVENT_PARAM2(PeerAssociated,
        uint64_t, ElementId,
        uint64_t, PeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // =====================================================================
    // Content Change Events
    // =====================================================================

    // Fired when ContentControl's content is replaced.
    // NewContentLabel is the new content's GetDebugLabel() (when it is a CDependencyObject;
    // empty for non-DO content such as raw strings).
    // NewContentPeerHandle is the new content's DXaml-peer InstanceHandle (0 when no peer) for live highlighting.
    DEFINE_TRACELOGGING_EVENT_PARAM5(ContentChanged,
        uint64_t, ParentId,
        uint64_t, OldContentId,
        uint64_t, NewContentId,
        PCWSTR,   NewContentLabel,
        uint64_t, NewContentPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // =====================================================================
    // Popup Events
    // =====================================================================

    // Fired when a Popup is opened and its child enters the visual tree.
    // PopupLabel is the popup's own GetDebugLabel(); ChildLabel is the popup child's GetDebugLabel().
    // ChildPeerHandle is the popup child's DXaml-peer InstanceHandle (0 when no peer) for live highlighting.
    DEFINE_TRACELOGGING_EVENT_PARAM5(PopupOpened,
        uint64_t, PopupId,
        uint64_t, ChildId,
        PCWSTR,   PopupLabel,
        PCWSTR,   ChildLabel,
        uint64_t, ChildPeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a Popup is closed and its child leaves the visual tree.
    // No label — consumer already has it from PopupOpened.
    DEFINE_TRACELOGGING_EVENT_PARAM2(PopupClosed,
        uint64_t, PopupId,
        uint64_t, ChildId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // =====================================================================
    // Composition Tree Events
    // =====================================================================

    // Fired when a UIElement's composition peer is linked.
    // ElementLabel is the UIElement's GetDebugLabel() — also used as the comp node's label
    // in the profiler, since the comp node is otherwise anonymous.
    // UIElementPeerHandle is the UIElement's DXaml-peer InstanceHandle (0 when no peer); it lets
    // a comp-tree node be highlighted live by routing through its owning UIElement.
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

    // =====================================================================
    // WUC (Windows.UI.Composition) Visual Tree Events
    //
    // These let an out-of-process profiler reconstruct the *full* IVisual tree
    // (the dense tree that DCompTreeHelper dumps as <VisualTree>), which is finer
    // grained than the comp-node tree: each comp node expands into a spine of
    // several IVisuals (prepend -> primary -> [clip] -> content), plus leaf
    // SpriteVisuals for drawn content.
    //
    // Identity: all *VisualId values are the raw WUComp::IVisual* pointer (QI'd to
    // IVisual so the id is stable regardless of which interface the call site held),
    // matching the "wucVisual" attribute emitted by DCompTreeHelper's live dump.
    //
    // OwnerCompNodeId: the HWCompNode that owns the visual (0 when unknown, e.g. for
    // some leaf SpriteVisuals). Join it against CompPeerLinked (CompNode -> UIElement)
    // to bridge the WUC tree to the logical/visual trees.
    //
    // Properties: the full property set of the inserted visual, serialized in the
    // exact same XML-attribute format DCompTreeHelper uses for <WucVisual> nodes, so
    // a consumer can reuse the same attribute parser. Type is also surfaced as its
    // own field for convenience (SpriteVisual / LayerVisual / ShapeVisual /
    // RedirectVisual / ContainerVisual / Visual).
    // =====================================================================

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

    // Fired when *all* children of a WUC container are removed at once (RemoveAll),
    // e.g. when an element re-renders its content. The consumer should drop every
    // child currently recorded under ParentVisualId.
    DEFINE_TRACELOGGING_EVENT_PARAM1(WucVisualChildrenCleared,
        uint64_t, ParentVisualId,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a WUC visual is set as the root of a composition target/island
    // (ContentIsland::Root or XamlIslandRoot::SetRootVisual). This is how the very
    // top of a tree - and each windowed-popup subtree - gets attached; it never goes
    // through a children collection, so it must be reported separately.
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

    // =====================================================================
    // GPU Memory Events
    // =====================================================================

    // Reports the lifecycle and logical dimensions of a DComp surface. EstimatedSizeBytes
    // is width-with-gutters * height-with-gutters * pixel stride. It is useful for attribution,
    // but is not exact committed VRAM because DComp may atlas, share, or sparsely tile surfaces.
    DEFINE_TRACELOGGING_EVENT_PARAM7(DCompSurfaceMemoryChanged,
        uint64_t, SurfaceId,
        bool,     IsAllocation,
        uint64_t, EstimatedSizeBytes,
        uint32_t, Width,
        uint32_t, Height,
        uint32_t, PixelStride,
        bool,     IsVirtual,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Reports the transient D3D11 texture exposed by DComp during BeginDraw. For atlased
    // surfaces, Width/Height describe the shared backing texture and OffsetX/OffsetY locate
    // this surface's update region within it. The resource must not be retained after EndDraw.
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

    // Associates a rendered IVisual with the DComp surface used by one of its brush roles.
    // SurfaceId == 0 clears the role. Role 0 is the brush texture; role 1 is the mask texture.
    // VisualId joins the existing WUC events, whose OwnerCompNodeId joins CompPeerLinked.
    DEFINE_TRACELOGGING_EVENT_PARAM3(VisualSurfaceChanged,
        uint64_t, VisualId,
        uint64_t, SurfaceId,
        uint32_t, Role,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Driver-reported memory usage for this process on the D3D device's adapter. This is the
    // closest available view of committed GPU memory, but allocations owned by DWM/DComp may
    // be charged outside the app process.
    DEFINE_TRACELOGGING_EVENT_PARAM6(GpuMemorySnapshot,
        uint64_t, LocalCurrentUsage,
        uint64_t, LocalBudget,
        bool,     LocalAvailable,
        uint64_t, NonLocalCurrentUsage,
        uint64_t, NonLocalBudget,
        bool,     NonLocalAvailable,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
};

// Element-scoped events are TraceLogging events on the existing Microsoft-Windows-XAML provider
// {531a35ab-63ce-4bcf-aa98-f88c7a89e455}, through the same XamlTelemetryLogging registration that
// XamlTelemetry uses, rather than on Microsoft-Windows-XAML-Profiler. TraceLogging events are
// self-describing, so WPA/TraceProcessor decode their names and typed identity fields without a
// registered manifest.
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

    // =====================================================================
    // Element-Scoped Activity Events
    //
    // Each of these mirrors a retail Microsoft-Windows-XAML operation (layout,
    // transition, focus, selection, virtualization, ...) that previously emitted
    // no element identity, so an out-of-process consumer could only attribute it
    // to an element by scope-nesting inference. Core pointers are classified at
    // runtime, including objects reached through a projection's GetHandle():
    //   * UI element, including controls: cuielement.
    //   * Other dependency object: objectpointer, plus cuielement only when a
    //     containing text control or an explicitly supplied UI owner is known.
    // A null subject produces objectpointer=0, without a cuielement field.
    // cuielement uses the Visual tree's CUIElement identity. An associated text
    // host is not necessarily the particular overflow element rendering a run.
    // No owner is inferred for arbitrary or shared dependency objects.
    //
    // Routing model (see XAMLPROFILER_ENABLED): the manifest and TraceLogging forms are mutually
    // exclusive at each call site. A retail (shipping) build never defines XAMLPROFILER_ENABLED
    // and emits only the retail manifest event, exactly as before — existing xperf/WPA consumers
    // are byte-for-byte unaffected. A profiler-enabled (chk/debug) build emits only the
    // TraceLogging copy below IN PLACE OF the retail manifest event, on the same
    // Microsoft-Windows-XAML provider, so a profiler build never double-logs the same operation.
    // Each call site selects one branch with #ifdef XAMLPROFILER_ENABLED (profiler) / #else
    // (retail). The consumer keys a scope by (provider + task) and pairs Start/Stop by opcode, so
    // each activity below reuses the retail operation's name as its event name and preserves the
    // Begin/End timing while adding typed identities.
    //
    // Exceptions (stay additive): a few sites keep the pre-existing retail Begin/End AND also emit
    // the profiler marker, because the retail marker is not a same-location duplicate — its
    // Begin/End brackets a whole pass/loop while the profiler marker is a per-iteration point event
    // at a different line (RealizeTransition, GenerateMCContainer). Gating the pre-existing retail
    // bracket there would change retail behavior, so it is left intact.
    //
    // Payload parity: where the retail marker carried extra payload (style name, focus
    // direction, gripper coordinates, virtualization state, container index), the profiler
    // copy carries the same payload PLUS typed identities, so a profiler build — which emits in place
    // of the retail marker — loses no information the retail provider would have supplied.
    //
    // Two shapes are used:
    //   * Activity events  (DEFINE_ELEMENT_ACTIVITY): a Start/Stop pair mirroring the
    //     retail Begin/End so per-operation duration is preserved. Both edges carry
    //     their subject's identity, so each Start and Stop row stands on its own. The only
    //     exception is GenerateContainerStart, whose container does not exist until the
    //     operation ends. An operation whose retail marker carried extra payload adds a
    //     matching overload on the edge that carried it (e.g. GetBuiltInStyleStop with the
    //     style name).
    //   * Point events: a single marker for
    //     operations whose retail form is win:Info, or whose element identity is
    //     per-iteration inside a pass-level Begin/End (RealizeTransition). The field
    //     payload preserves the retail fields alongside the typed identities.
    // =====================================================================

    // Resolve relationships only when this verbose event can actually be emitted.
#define TRACE_ELEMENT_EVENT(OpName, Object, RelatedUIElement, ...) \
    do \
    { \
        if (TraceLoggingProviderEnabled(TraceLoggingType::Provider(), WINEVENT_LEVEL_VERBOSE, 0)) \
        { \
            const auto identity = GetIdentity(Object, RelatedUIElement); \
            if (identity.uiElement != 0 && identity.objectPointer == 0) \
            { \
                TraceLoggingWrite(TraceLoggingType::Provider(), OpName, \
                    TraceLoggingValue(identity.uiElement, "cuielement"), \
                    __VA_ARGS__); \
            } \
            else if (identity.uiElement != 0) \
            { \
                TraceLoggingWrite(TraceLoggingType::Provider(), OpName, \
                    TraceLoggingValue(identity.uiElement, "cuielement"), \
                    TraceLoggingValue(identity.objectPointer, "objectpointer"), \
                    __VA_ARGS__); \
            } \
            else \
            { \
                TraceLoggingWrite(TraceLoggingType::Provider(), OpName, \
                    TraceLoggingValue(identity.objectPointer, "objectpointer"), \
                    __VA_ARGS__); \
            } \
        } \
    } while (false)

    // Emits a Start/Stop activity pair on this provider under one ETW task (== OpName),
    // so the consumer pairs them into a duration exactly like the retail Begin/End.
    // Stop always takes a subject; the parameterless Start exists only for
    // GenerateContainer (see above).
#define DEFINE_ELEMENT_ACTIVITY(OpName) \
    static void OpName##Start(CDependencyObject* object, CDependencyObject* relatedUIElement = nullptr) \
    { \
        TRACE_ELEMENT_EVENT(#OpName, object, relatedUIElement, \
            TraceLoggingOpcode(WINEVENT_OPCODE_START), \
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE)); \
    } \
    static void OpName##Stop(CDependencyObject* object, CDependencyObject* relatedUIElement = nullptr) \
    { \
        TRACE_ELEMENT_EVENT(#OpName, object, relatedUIElement, \
            TraceLoggingOpcode(WINEVENT_OPCODE_STOP), \
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE)); \
    }

    // Layout-manager passes (CLayoutManager): the subject is the layout root.
    DEFINE_ELEMENT_ACTIVITY(FireLayoutUpdated);
    DEFINE_ELEMENT_ACTIVITY(FireSizeChanged);
    // Per-element size-changed notification: the subject is the element whose size changed.
    DEFINE_ELEMENT_ACTIVITY(IndividualSizeChanged);
    // Root visual set on a visual tree: the subject is the root element.
    DEFINE_ELEMENT_ACTIVITY(PutRootVisual);

    // Top-level layout passes (CLayoutManager::UpdateLayout): the subject is the layout root.
    DEFINE_ELEMENT_ACTIVITY(Layout);
    DEFINE_ELEMENT_ACTIVITY(Measure);
    DEFINE_ELEMENT_ACTIVITY(Arrange);

    // Layout transitions: the subject is the element the transition targets.
    // RealizeTransition stays a point event: its retail Begin/End brackets a whole
    // pass while the element identity is per-iteration inside the loop.
    static void RealizeTransition(CDependencyObject* object)
    {
        TRACE_ELEMENT_EVENT("RealizeTransition", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
    }
    DEFINE_ELEMENT_ACTIVITY(CancelTransitions);
    DEFINE_ELEMENT_ACTIVITY(ProcessLayoutForTransition);

    // Focus (CFocusManager): the subject can be a UI element or a non-UI Hyperlink.
    // UpdateFocus Start carries the requested target and Stop carries pNewFocus; they can
    // differ after redirection. Stop also runs on cancellation/error, not just successful focus.
    DEFINE_ELEMENT_ACTIVITY(UpdateFocus);
    DEFINE_ELEMENT_ACTIVITY(XYFocusEntered);
    // XYFocusEntered adds the retail Begin's focus-direction string on the Start edge.
    static void XYFocusEnteredStart(CDependencyObject* object, PCWSTR Direction)
    {
        TRACE_ELEMENT_EVENT("XYFocusEntered", object, nullptr,
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(Direction, "Direction"));
    }

    // Text selection (TextSelectionManager): the subject is the owning text control.
    DEFINE_ELEMENT_ACTIVITY(ChangeSelection);
    DEFINE_ELEMENT_ACTIVITY(ExtendSelectionRange);

    // Touch-selection grippers (CTextSelectionGripper): the subject is the gripper element.
    // IsStartGripper/X/Y mirror the retail *Info markers (the gripper's start-vs-end role and its
    // center world coordinate) so a profiler-only session keeps the gripper's placement.
    static void TouchSelectionGripperShowBegin(CDependencyObject* object, bool IsStartGripper, int32_t X, int32_t Y)
    {
        TRACE_ELEMENT_EVENT("TouchSelectionGripperShowBegin", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(IsStartGripper, "IsStartGripper"),
            TraceLoggingValue(X, "X"),
            TraceLoggingValue(Y, "Y"));
    }

    static void TouchSelectionGripperShowEnd(CDependencyObject* object, bool IsStartGripper, int32_t X, int32_t Y)
    {
        TRACE_ELEMENT_EVENT("TouchSelectionGripperShowEnd", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(IsStartGripper, "IsStartGripper"),
            TraceLoggingValue(X, "X"),
            TraceLoggingValue(Y, "Y"));
    }

    static void TouchSelectionGripperHideBegin(CDependencyObject* object, bool IsStartGripper, int32_t X, int32_t Y)
    {
        TRACE_ELEMENT_EVENT("TouchSelectionGripperHideBegin", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(IsStartGripper, "IsStartGripper"),
            TraceLoggingValue(X, "X"),
            TraceLoggingValue(Y, "Y"));
    }

    static void TouchSelectionGripperHideEnd(CDependencyObject* object, bool IsStartGripper, int32_t X, int32_t Y)
    {
        TRACE_ELEMENT_EVENT("TouchSelectionGripperHideEnd", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(IsStartGripper, "IsStartGripper"),
            TraceLoggingValue(X, "X"),
            TraceLoggingValue(Y, "Y"));
    }

    static void TouchSelectionGripperReposition(CDependencyObject* object, bool IsStartGripper, int32_t X, int32_t Y)
    {
        TRACE_ELEMENT_EVENT("TouchSelectionGripperReposition", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(IsStartGripper, "IsStartGripper"),
            TraceLoggingValue(X, "X"),
            TraceLoggingValue(Y, "Y"));
    }

    // Items virtualization (dxaml projection layer): the subject is the container/panel.
    // GenerateContainer carries an identity on its Stop: the container is only realized near the
    // end of the generate call (inverted Start/Stop pair; CopyTo keeps it live through Cleanup).
    DEFINE_ELEMENT_ACTIVITY(GenerateContainer);
    static void GenerateContainerStart()
    {
        TraceLoggingWrite(TraceLoggingType::Provider(), "GenerateContainer",
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
    }
    DEFINE_ELEMENT_ACTIVITY(GenerateItems);
    // GenerateMCContainer is a point event: its container is moved out (MoveTo) before Cleanup,
    // so it emits a single marker at realization instead of an inverted Start/Stop pair. Index is
    // the retail Begin's item-collection index.
    static void GenerateMCContainer(CDependencyObject* object, int32_t Index)
    {
        TRACE_ELEMENT_EVENT("GenerateMCContainer", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(Index, "Index"));
    }
    DEFINE_ELEMENT_ACTIVITY(PlaceElement);
    // PlaceElement adds the retail Begin's data index on the Start edge.
    static void PlaceElementStart(CDependencyObject* object, int32_t DataIndex)
    {
        TRACE_ELEMENT_EVENT("PlaceElement", object, nullptr,
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(DataIndex, "DataIndex"));
    }
    DEFINE_ELEMENT_ACTIVITY(PrepareContainer);
    DEFINE_ELEMENT_ACTIVITY(MeasureChild);
    DEFINE_ELEMENT_ACTIVITY(VirtualizationMeasure);
    // OrientedVirtualizingPanel-scoped virtualization bookkeeping (CleanupContainers,
    // UpdateLogicalScrollData: the subject is the panel itself). VirtualizationAdd is
    // per-container: the subject is the container being added.
    DEFINE_ELEMENT_ACTIVITY(VirtualizationAdd);
    DEFINE_ELEMENT_ACTIVITY(VirtualizationCleanup);
    DEFINE_ELEMENT_ACTIVITY(UpdateLogicalScrollData);
    // VirtualizationIsEnabledByLayout stays a point event (retail form is win:Info).
    // IsVirtualizationActive/ElementName/ClassName/ParentClassName mirror the retail Info1 payload.
    static void VirtualizationIsEnabledByLayout(CDependencyObject* object, bool IsVirtualizationActive, PCWSTR ElementName, PCWSTR ClassName, PCWSTR ParentClassName)
    {
        TRACE_ELEMENT_EVENT("VirtualizationIsEnabledByLayout", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(IsVirtualizationActive, "IsVirtualizationActive"),
            TraceLoggingValue(ElementName, "ElementName"),
            TraceLoggingValue(ClassName, "ClassName"),
            TraceLoggingValue(ParentClassName, "ParentClassName"));
    }
    DEFINE_ELEMENT_ACTIVITY(GetElementCount);

    // Control template/style operations (CControl): the subject is the control, not the style.
    DEFINE_ELEMENT_ACTIVITY(GetBuiltInStyle);
    // GetBuiltInStyle adds the retail End's resolved style target-type name on the Stop edge.
    static void GetBuiltInStyleStop(CDependencyObject* object, PCWSTR StyleName)
    {
        TRACE_ELEMENT_EVENT("GetBuiltInStyle", object, nullptr,
            TraceLoggingOpcode(WINEVENT_OPCODE_STOP),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(StyleName, "StyleName"));
    }

    // Hit-testing (CUIElement::BoundsTestEntry): the subject is the subtree being hit-tested.
    DEFINE_ELEMENT_ACTIVITY(HitTest);
    // Per-hit-test bounds cache stats, emitted once BoundsTestEntry completes. The subject is the
    // same element as the enclosing HitTest activity; RecalculatedCount/ReusedCount mirror the
    // retail Info payload.
    static void OuterBoundsStats(CDependencyObject* object, uint32_t RecalculatedCount, uint32_t ReusedCount)
    {
        TRACE_ELEMENT_EVENT("OuterBoundsStats", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(RecalculatedCount, "RecalculatedCount"),
            TraceLoggingValue(ReusedCount, "ReusedCount"));
    }

    static void InnerBoundsStats(CDependencyObject* object, uint32_t RecalculatedCount, uint32_t ReusedCount)
    {
        TRACE_ELEMENT_EVENT("InnerBoundsStats", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(RecalculatedCount, "RecalculatedCount"),
            TraceLoggingValue(ReusedCount, "ReusedCount"));
    }

    static void ContentBoundsStats(CDependencyObject* object, uint32_t RecalculatedCount, uint32_t ReusedCount)
    {
        TRACE_ELEMENT_EVENT("ContentBoundsStats", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(RecalculatedCount, "RecalculatedCount"),
            TraceLoggingValue(ReusedCount, "ReusedCount"));
    }

    static void ChildBoundsStats(CDependencyObject* object, uint32_t RecalculatedCount, uint32_t ReusedCount)
    {
        TRACE_ELEMENT_EVENT("ChildBoundsStats", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(RecalculatedCount, "RecalculatedCount"),
            TraceLoggingValue(ReusedCount, "ReusedCount"));
    }

    // Hub section count (Hub_Partial.cpp): the subject is the Hub whose section count changed.
    static void HubSectionCount(CDependencyObject* object, uint32_t SectionCount)
    {
        TRACE_ELEMENT_EVENT("HubSectionCount", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(SectionCount, "SectionCount"));
    }

    // Frame navigation (Frame_Partial.cpp): the subject is the Frame, not its content.
    static void FrameNavigated(CDependencyObject* object, PCWSTR Descriptor, uint8_t NavigationMode)
    {
        TRACE_ELEMENT_EVENT("FrameNavigated", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(Descriptor, "Descriptor"),
            TraceLoggingValue(NavigationMode, "NavigationMode"));
    }

    static void FrameNavigating(CDependencyObject* object, PCWSTR Descriptor, uint8_t NavigationMode)
    {
        TRACE_ELEMENT_EVENT("FrameNavigating", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(Descriptor, "Descriptor"),
            TraceLoggingValue(NavigationMode, "NavigationMode"));
    }

    // Text input scope/keyboard skin (CPasswordBox): the subject is the PasswordBox.
    static void SendInputScopeToRichEdit(CDependencyObject* object, uint32_t InputScope)
    {
        TRACE_ELEMENT_EVENT("SendInputScopeToRichEdit", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(InputScope, "InputScope"));
    }

    static void SetIhmKeyboardSkin(CDependencyObject* object, uint32_t KeyboardSkin)
    {
        TRACE_ELEMENT_EVENT("SetIhmKeyboardSkin", object, nullptr,
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(KeyboardSkin, "KeyboardSkin"));
    }

    // Container-content-changing setup (ListViewBase_Partial_ContainerPhase.cpp): the subject is
    // the container being set up.
    DEFINE_ELEMENT_ACTIVITY(SetupCCC);
    // SetupCCC adds the retail Begin's item index on the Start edge.
    static void SetupCCCStart(CDependencyObject* object, int32_t ItemIndex, CDependencyObject* relatedUIElement = nullptr)
    {
        TRACE_ELEMENT_EVENT("SetupCCC", object, relatedUIElement,
            TraceLoggingOpcode(WINEVENT_OPCODE_START),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingValue(ItemIndex, "ItemIndex"));
    }

#undef DEFINE_ELEMENT_ACTIVITY
#undef TRACE_ELEMENT_EVENT
};

#endif // XAMLPROFILER_ENABLED
