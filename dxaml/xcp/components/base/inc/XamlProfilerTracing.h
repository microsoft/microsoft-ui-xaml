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

class CDependencyObject;

// Returns the IInspectable identity handle of the object's DXaml peer - the same value
// XAML Diagnostics (and the WinUISnoop tap) use as an InstanceHandle to identify a live
// element - or 0 when the object is null or has no peer yet. This lets an out-of-process
// profiler bridge a tree node back to the live element so the element can be highlighted
// in the target app. The core pointer IDs emitted alongside it are kept as-is, since those
// are what stitch the logical/visual/composition trees together. Defined in uielement.cpp.
uint64_t XamlProfilerGetPeerHandle(_In_opt_ const CDependencyObject* obj) noexcept;

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
    DEFINE_TRACELOGGING_EVENT_PARAM6(ElementEnteredTree,
        uint64_t, ElementId,
        uint64_t, ParentId,
        bool,     IsLive,
        PCWSTR,   Label,
        bool,     IsTemplateChild,
        uint64_t, PeerHandle,
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

    // Fired when a UIElement leaves the live tree.
    DEFINE_TRACELOGGING_EVENT_PARAM3(ElementLeftTree,
        uint64_t, ElementId,
        uint64_t, ParentId,
        bool, IsLive,
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
};

#endif // XAMLPROFILER_ENABLED
