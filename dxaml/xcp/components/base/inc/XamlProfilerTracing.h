// Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project
// root for license information.

#pragma once

// XamlProfiler tree-tracking instrumentation. XAMLPROFILER_ENABLED is defined for every build
// configuration from the XamlProfilerEnabled MSBuild property (dxaml\Xaml.Cpp.Props; applied by
// Xaml.Cpp.Targets / LibraryCompile.props). Every consumer #includes this header (and its call
// sites) inside its own #ifdef XAMLPROFILER_ENABLED, matching the SwipeTestHooks convention, so
// building with /p:XamlProfilerEnabled=false still compiles all of it out. The one exception is
// the WUC visual property sites, which use the XAMLPROFILER_WUC_* macros from
// WucVisualTreeProfiler.h; those macros compile to nothing without XAMLPROFILER_ENABLED.
//
// Cost with no profiler session: every instrumented site is gated by the inline
// XamlProfilerTracing::IsEnabled(<keyword>) check below - one load, compare and branch. Argument
// work (labels, peer handles, type info) lives in out-of-line helpers next to the call sites,
// and every event is written by an out-of-line emitter in XamlProfilerTracing.cpp, so no
// TraceLoggingWrite expansion lands in a hot function.
//
// Cost with a session: the per-change tree events ElementEnteredTree, WucVisualChildInserted and
// WucVisualRootSet carry scalar payloads only. Anything that is a property of a type rather than
// an instance (type name, core size) is emitted once per type per session (ElementTypeInfo), and
// per-instance details (x:Name, DXaml peer, XAML source location, WUC visual properties) come from
// granular events emitted when that detail changes. The consumer joins them back into the same
// trees.
#ifdef XAMLPROFILER_ENABLED

#include <cstdint>
#include <winmeta.h> // WINEVENT_LEVEL constants
#include <TraceLoggingProvider.h>
#include <XcpAllocation.h> // XCP_COUNT_OUTSTANDING_ALLOC and the allocator counters

class CDependencyObject;

// Returns the IInspectable identity handle of the object's DXaml peer - the same value
// XAML Diagnostics (and the WinUISnoop tap) use as an InstanceHandle to identify a live
// element - or 0 when the object is null or has no peer yet. This lets an out-of-process
// profiler bridge a tree node back to the live element so the element can be highlighted
// in the target app. The core pointer IDs emitted alongside it are kept as-is, since those
// are what stitch the logical/visual/composition trees together. Defined in uielement.cpp.
// It QIs the peer, and for a composed peer (e.g. a C# subclass) that QI runs app code, so
// never call it while holding the PeerTable lock (AutoReentrantReferenceLock).
uint64_t XamlProfilerGetPeerHandle(_In_opt_ const CDependencyObject* obj) noexcept;

// Returns the actual heap-allocation size (in bytes) of the core object — its concrete,
// most-derived C++ instance footprint (NOT sizeof(CUIElement), which is the base-class size and
// identical for every element). In debug builds the size is recovered from the debug allocator's
// validation header for every creation path. Other builds return 0 (unknown): HeapSize needs the
// exact block start, which a base-class pointer isn't guaranteed to be, and retail blocks have no
// header to validate. This is only the fixed per-type "core block"; the element's sparse property
// table and its DXaml peer are separate allocations, measured on demand. 0 when obj is null.
// Defined in uielement.cpp alongside XamlProfilerGetPeerHandle.
uint64_t XamlProfilerGetCoreSize(_In_opt_ const CDependencyObject* obj) noexcept;

// TraceLogging provider for XAML Profiler tree-tracking events.
// These events allow an out-of-process profiler to reconstruct and diff the logical tree,
// visual tree, and composition tree over the lifetime of the host process.
//
// Provider Name: "Microsoft-Windows-XAML-Profiler"
// Provider GUID: {26f28a5c-7d30-54c5-40f4-fff6d3beda7d} (EventSource name hash of the provider
// name, so tools can also enable it as "*Microsoft-Windows-XAML-Profiler").
//
// Defined in XamlProfilerTracing.cpp with no provider-group option; registered and unregistered
// with the DLL in dllentry.cpp. Before registration and after unregistration IsEnabled() is false
// and the emitters do nothing.
TRACELOGGING_DECLARE_PROVIDER(g_hXamlProfilerProvider);

// One ETW keyword per marker group. A session enables only the groups it needs, and the work
// behind a group runs only when that group is on. Every event is written at
// WINEVENT_LEVEL_VERBOSE with exactly one of these keywords. A session that enables the provider
// without keywords (MatchAnyKeyword 0) receives every group. Only low bits are used: bits 44-47
// are reserved for Microsoft telemetry and bits 48-63 by ETW.
namespace XamlProfilerKeywords
{
    // ChildAdded, ChildInserted, ChildRemoved, ElementEnteredTree, ElementTypeInfo,
    // ElementLeftTree, PeerAssociated, ContentChanged, PopupOpened, PopupClosed.
    constexpr ULONGLONG XamlTree = 0x01;

    // CompPeerLinked, CompPeerUnlinked, CompNodeChildInserted, CompNodeChildRemoved.
    constexpr ULONGLONG CompTree = 0x02;

    // WucVisualCreated, WucVisualChildInserted, WucVisualChildRemoved, WucVisualChildrenCleared,
    // WucVisualRootSet, WucVisualRootCleared, and the visual property events
    // WucVisualPropertyChanged, WucVisualTransformChanged, WucVisualObjectChanged.
    constexpr ULONGLONG WucTree = 0x04;

    // DCompSurfaceMemoryChanged, VisualSurfaceChanged.
    constexpr ULONGLONG Surfaces = 0x08;

    // DCompSurfaceTextureObserved (QueryInterface chain to the D3D texture on each draw).
    constexpr ULONGLONG SurfaceTextures = 0x10;

    // XamlHeapSnapshot (allocator counter reads on each element enter/leave and surface change).
    constexpr ULONGLONG HeapSnapshots = 0x20;

    // GpuMemorySnapshot (D3D device lock and DXGI video-memory queries on each surface change).
    constexpr ULONGLONG GpuSnapshots = 0x40;
}

namespace XamlProfilerTracing
{
    // True when a session has enabled the provider at verbose level for any of 'keywords'.
    // Sessions below WINEVENT_LEVEL_VERBOSE would drop every event, so they don't turn it on.
    __forceinline bool IsEnabled(ULONGLONG keywords) noexcept
    {
        return TraceLoggingProviderEnabled(g_hXamlProfilerProvider, WINEVENT_LEVEL_VERBOSE, keywords) != 0;
    }

    // ETW enable callback, passed to TraceLoggingRegisterEx in dllentry.cpp (runs on an ETW
    // thread, after TraceLogging has updated the state IsEnabled reads). It only resets the
    // session-scoped type table below, whenever a session enables or disables the provider or
    // requests capture state, so every session receives ElementTypeInfo for the types it sees.
    void NTAPI ProviderEnableCallback(
        _In_ LPCGUID sourceId,
        ULONG controlCode,
        UCHAR level,
        ULONGLONG matchAnyKeyword,
        ULONGLONG matchAllKeyword,
        _In_opt_ PEVENT_FILTER_DESCRIPTOR filterData,
        _Inout_opt_ PVOID callbackContext) noexcept;

    // Session-scoped set of the KnownTypeIndex values whose ElementTypeInfo has been written (a
    // bitset of this many entries). ElementEnteredTree carries only a TypeIndex; the first enter
    // of each type in a session is preceded by an ElementTypeInfo row.
    constexpr uint32_t c_typeTableCapacity = 2048;

    // True when typeIndex's ElementTypeInfo has been written since the table was last reset, and
    // always false for an index beyond the table. When it's false, the caller writes
    // ElementTypeInfo and then calls MarkTypeDescribed, so a type is marked only after its row is
    // written. UI threads that race on a new type may each write the row.
    bool IsTypeDescribed(uint32_t typeIndex) noexcept;

    // Records that typeIndex's ElementTypeInfo has been written. No-op for an index beyond the table.
    void MarkTypeDescribed(uint32_t typeIndex) noexcept;

    // Out-of-line emitters, defined in XamlProfilerTracing.cpp. Event names, field names, field
    // types and field order are the wire contract with the profiler tools.

    // =====================================================================
    // Logical Tree Events (XamlTree)
    // =====================================================================

    // Fired when a child UIElement is added to a parent's children collection.
    // ChildLabel is the child's GetDebugLabel() — x:Name if set, else class name (e.g. "Button", "OkButton").
    // IsTemplateChild is true when the child was created by a ControlTemplate (GetTemplatedParent() != null).
    // ChildPeerHandle is the child's DXaml-peer InstanceHandle (0 when no peer) for live highlighting.
    void ChildAdded(
        uint64_t parentId,
        uint64_t childId,
        uint32_t index,
        PCWSTR childLabel,
        bool isTemplateChild,
        uint64_t childPeerHandle) noexcept;

    // Fired when a child UIElement is inserted at a specific index.
    // ChildLabel is the child's GetDebugLabel() — x:Name if set, else class name.
    // IsTemplateChild is true when the child was created by a ControlTemplate (GetTemplatedParent() != null).
    // ChildPeerHandle is the child's DXaml-peer InstanceHandle (0 when no peer) for live highlighting.
    void ChildInserted(
        uint64_t parentId,
        uint64_t childId,
        uint32_t index,
        PCWSTR childLabel,
        bool isTemplateChild,
        uint64_t childPeerHandle) noexcept;

    // Fired when a child UIElement is removed from a parent's children collection.
    // No label — the consumer already has it from the original ChildAdded/ChildInserted event.
    void ChildRemoved(
        uint64_t parentId,
        uint64_t childId) noexcept;

    // Fired when a UIElement enters the tree (live or non-live). Scalar-only by design, since it
    // fires for every element: the consumer resolves the remaining node details from granular
    // events that are emitted once, when the detail is established:
    //   - type name and core size: ElementTypeInfo (below), normally once per TypeIndex per session;
    //   - x:Name: Microsoft-Windows-XAML ElementSetNameInfo (CDependencyObject::SetName);
    //   - DXaml peer InstanceHandle: PeerAssociated (below);
    //   - XAML source location: Microsoft-Windows-XAML-Diagnostics ElementCreatedWithSourceInfo1;
    //   - per-address reset for the above: Microsoft-Windows-XAML ElementCreatedInfo (CUIElement ctor).
    // IsTemplateChild is true when the element was created by a ControlTemplate (GetTemplatedParent() != null).
    // TypeIndex is the core object's KnownTypeIndex.
    void ElementEnteredTree(
        uint64_t elementId,
        uint64_t parentId,
        bool isLive,
        bool isTemplateChild,
        uint32_t typeIndex) noexcept;

    // Emitted before the first ElementEnteredTree of each TypeIndex in a session (see
    // IsTypeDescribed). Delivery is best-effort: a type can be described more than once (racing
    // UI threads, overlapping sessions), and when a session starts while elements are entering,
    // the first enters of a type can precede its row. Consumers therefore apply a late row to the
    // nodes they already hold with that TypeIndex. TypeName is the core class name (what
    // GetDebugLabel() falls back to when there's no x:Name). CoreSize is the heap-block size of
    // the concrete core object, which is fixed per type (0 when unknown, see
    // XamlProfilerGetCoreSize); sparse properties and the DXaml peer are separate allocations.
    void ElementTypeInfo(
        uint32_t typeIndex,
        PCWSTR typeName,
        uint64_t coreSize) noexcept;

    // Fired when a UIElement leaves the live tree.
    void ElementLeftTree(
        uint64_t elementId,
        uint64_t parentId,
        bool isLive) noexcept;

    // =====================================================================
    // Peer Association Events (XamlTree)
    // =====================================================================

    // Fired when a UIElement's DXaml framework peer is created and associated with the core
    // object (DXamlCore::GetPeerPrivate, after SetDXamlPeer, once the PeerTable lock is released).
    // This is the source of the InstanceHandle for ElementEnteredTree nodes: it fires before the
    // element enters the tree for peer-first objects and after it for lazy-peer elements (e.g.
    // ContentPresenter, ClientAreaPresenter), so the consumer holds it per ElementId until the
    // element enters. Other DependencyObjects aren't reported.
    // ElementId is the core CDependencyObject* (the same id stitched across all other events);
    // PeerHandle is the peer's IInspectable identity (the XAML Diagnostics InstanceHandle).
    void PeerAssociated(
        uint64_t elementId,
        uint64_t peerHandle) noexcept;

    // =====================================================================
    // Content Change Events (XamlTree)
    // =====================================================================

    // Fired when ContentControl's content is replaced.
    // NewContentLabel is the new content's GetDebugLabel() (when it is a CDependencyObject;
    // empty for non-DO content such as raw strings).
    // NewContentPeerHandle is the new content's DXaml-peer InstanceHandle (0 when no peer) for live highlighting.
    void ContentChanged(
        uint64_t parentId,
        uint64_t oldContentId,
        uint64_t newContentId,
        PCWSTR newContentLabel,
        uint64_t newContentPeerHandle) noexcept;

    // =====================================================================
    // Popup Events (XamlTree)
    // =====================================================================

    // Fired when a Popup is opened and its child enters the visual tree.
    // PopupLabel is the popup's own GetDebugLabel(); ChildLabel is the popup child's GetDebugLabel().
    // ChildPeerHandle is the popup child's DXaml-peer InstanceHandle (0 when no peer) for live highlighting.
    void PopupOpened(
        uint64_t popupId,
        uint64_t childId,
        PCWSTR popupLabel,
        PCWSTR childLabel,
        uint64_t childPeerHandle) noexcept;

    // Fired when a Popup is closed and its child leaves the visual tree.
    // No label — consumer already has it from PopupOpened.
    void PopupClosed(
        uint64_t popupId,
        uint64_t childId) noexcept;

    // =====================================================================
    // Composition Tree Events (CompTree)
    // =====================================================================

    // Fired when a UIElement's composition peer is linked.
    // ElementLabel is the UIElement's GetDebugLabel() — also used as the comp node's label
    // in the profiler, since the comp node is otherwise anonymous.
    // UIElementPeerHandle is the UIElement's DXaml-peer InstanceHandle (0 when no peer); it lets
    // a comp-tree node be highlighted live by routing through its owning UIElement.
    void CompPeerLinked(
        uint64_t uiElementId,
        uint64_t compNodeId,
        uint64_t parentCompNodeId,
        PCWSTR elementLabel,
        uint64_t uiElementPeerHandle) noexcept;

    // Fired when a UIElement's composition peer is unlinked.
    void CompPeerUnlinked(
        uint64_t uiElementId,
        uint64_t compNodeId) noexcept;

    // Fired when a comp node child is inserted into a parent comp node.
    void CompNodeChildInserted(
        uint64_t parentCompNodeId,
        uint64_t childCompNodeId) noexcept;

    // Fired when a comp node child is removed from a parent comp node.
    void CompNodeChildRemoved(
        uint64_t parentCompNodeId,
        uint64_t childCompNodeId) noexcept;

    // =====================================================================
    // WUC (Windows.UI.Composition) Visual Tree Events (WucTree)
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
    // VisualKind: the concrete visual type (WucVisualTreeProfiler::VisualKind: 0 Unknown,
    // 1 Visual, 2 ContainerVisual, 3 SpriteVisual, 4 LayerVisual, 5 ShapeVisual,
    // 6 RedirectVisual).
    //
    // Visual properties are not carried on the insert/root events. XAML reports each property
    // it sets on a visual through WucVisualPropertyChanged / WucVisualTransformChanged /
    // WucVisualObjectChanged at the setter, so the consumer keeps a per-visual property bag
    // that is current (an insert-time snapshot was stale: most spine visuals are inserted
    // before their properties are pushed). Properties can be reported before the visual is
    // inserted. WucVisualCreated resets a VisualId's bag, because a new visual can reuse the
    // address of a destroyed one. Visuals whose properties the app sets (hand-in visuals, and
    // hand-off visuals XAML adopts as a primary visual) are read in full when XAML takes them.
    // =====================================================================

    // Fired when XAML creates a WUC visual. The consumer resets that VisualId's property bag to
    // the WUC defaults (the per-address reset ElementCreatedInfo gives element ids).
    void WucVisualCreated(
        uint64_t visualId,
        uint32_t visualKind) noexcept;

    // Fired when a WUC visual is inserted as a child of another WUC visual. An insert of a
    // visual that's already parented is a move.
    void WucVisualChildInserted(
        uint64_t parentVisualId,
        uint64_t childVisualId,
        uint64_t ownerCompNodeId,
        int32_t index,
        uint32_t visualKind) noexcept;

    // Fired when a WUC visual is removed from its parent's children collection.
    void WucVisualChildRemoved(
        uint64_t parentVisualId,
        uint64_t childVisualId) noexcept;

    // Fired when *all* children of a WUC container are removed at once (RemoveAll),
    // e.g. when an element re-renders its content. The consumer should drop every
    // child currently recorded under ParentVisualId.
    void WucVisualChildrenCleared(
        uint64_t parentVisualId) noexcept;

    // Fired when a WUC visual is set as the root of a composition target/island
    // (ContentIsland::Root or XamlIslandRoot::SetRootVisual). This is how the very
    // top of a tree - and each windowed-popup subtree - gets attached; it never goes
    // through a children collection, so it must be reported separately.
    void WucVisualRootSet(
        uint64_t visualId,
        uint64_t targetId,
        uint64_t ownerCompNodeId,
        uint32_t visualKind) noexcept;

    // Fired when a composition target/island root is cleared (Root set to null).
    void WucVisualRootCleared(
        uint64_t targetId) noexcept;

    // Fired when XAML sets a scalar or vector property on a WUC visual (or when an app-owned
    // visual is read). Property is a WucVisualTreeProfiler::VisualProperty value; X/Y/Z/W carry
    // its components (unused ones are 0; booleans are 0/1; enums are their numeric value).
    void WucVisualPropertyChanged(
        uint64_t visualId,
        uint32_t property,
        float x,
        float y,
        float z,
        float w) noexcept;

    // Fired when XAML sets a visual's TransformMatrix. 'matrix' is row-major M11..M44, emitted
    // as the fields M11..M44.
    void WucVisualTransformChanged(
        uint64_t visualId,
        _In_reads_(16) const float* matrix) noexcept;

    // Fired when XAML sets an object-valued property (clip, brush, shadow, ...) on a WUC visual.
    // ObjectId is the object's identity (0 when cleared). For Source it is the source visual's
    // VisualId. TypeName is the object's runtime class name without its namespace.
    void WucVisualObjectChanged(
        uint64_t visualId,
        uint32_t property,
        uint64_t objectId,
        PCWSTR typeName) noexcept;

    // =====================================================================
    // Surface Events (Surfaces, SurfaceTextures)
    // =====================================================================

    // Reports the lifecycle and logical dimensions of a DComp surface. EstimatedSizeBytes
    // is width-with-gutters * height-with-gutters * pixel stride. It is useful for attribution,
    // but is not exact committed VRAM because DComp may atlas, share, or sparsely tile surfaces.
    void DCompSurfaceMemoryChanged(
        uint64_t surfaceId,
        bool isAllocation,
        uint64_t estimatedSizeBytes,
        uint32_t width,
        uint32_t height,
        uint32_t pixelStride,
        bool isVirtual) noexcept;

    // Reports the transient D3D11 texture exposed by DComp during BeginDraw. For atlased
    // surfaces, Width/Height describe the shared backing texture and OffsetX/OffsetY locate
    // this surface's update region within it. The resource must not be retained after EndDraw.
    void DCompSurfaceTextureObserved(
        uint64_t surfaceId,
        uint64_t resourceId,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        uint32_t arraySize,
        uint32_t format,
        uint32_t sampleCount,
        int32_t offsetX,
        int32_t offsetY) noexcept;

    // Associates a rendered IVisual with the DComp surface used by one of its brush roles.
    // SurfaceId == 0 clears the role. Role 0 is the brush texture; role 1 is the mask texture.
    // VisualId joins the existing WUC events, whose OwnerCompNodeId joins CompPeerLinked.
    void VisualSurfaceChanged(
        uint64_t visualId,
        uint64_t surfaceId,
        uint32_t role) noexcept;

    // =====================================================================
    // Memory Snapshot Events (HeapSnapshots, GpuSnapshots)
    // =====================================================================

    // Snapshot of XAML allocator-owned CPU heap blocks. This excludes GPU texture backing and
    // process allocations owned by other components/runtimes. Only emitted where the allocator
    // keeps outstanding-allocation counters (see XamlProfilerTraceHeapSnapshot).
    void XamlHeapSnapshot(
        uint64_t heapHandle,
        bool isPrivateHeap,
        uint64_t outstandingBytes,
        uint64_t outstandingAllocationCount,
        uint64_t cumulativeAllocatedBytes,
        uint64_t cumulativeAllocationCount,
        uint64_t cumulativeDeallocationCount) noexcept;

    // Driver-reported memory usage for this process on the D3D device's adapter. This is the
    // closest available view of committed GPU memory, but allocations owned by DWM/DComp may
    // be charged outside the app process.
    void GpuMemorySnapshot(
        uint64_t localCurrentUsage,
        uint64_t localBudget,
        bool localAvailable,
        uint64_t nonLocalCurrentUsage,
        uint64_t nonLocalBudget,
        bool nonLocalAvailable) noexcept;
}

#if XCP_COUNT_OUTSTANDING_ALLOC
// Emits a XamlHeapSnapshot event from the XAML allocator's counters. It exists only in builds
// that keep the outstanding-allocation counters, so other builds never emit XamlHeapSnapshot
// rather than emitting zeros. Callers gate it on IsEnabled(XamlProfilerKeywords::HeapSnapshots).
// Inline, so it is compiled only into the files that call it, which already depend on the
// allocator; no library needs another library's copy.
inline __declspec(noinline) void XamlProfilerTraceHeapSnapshot() noexcept
{
    XamlProfilerTracing::XamlHeapSnapshot(
        XcpAllocation::GetHeapHandle(),
        XcpAllocation::IsUsingPrivateHeap(),
        XcpAllocation::GetOutstandingAllocationSize(),
        XcpAllocation::GetOutstandingAllocationCount(),
        XcpAllocation::GetAllocationSize(),
        XcpAllocation::GetAllocationCount(),
        XcpAllocation::GetDeallocationCount());
}
#endif // XCP_COUNT_OUTSTANDING_ALLOC

#endif // XAMLPROFILER_ENABLED
