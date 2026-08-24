// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <unknwn.h>

// Producer-side helpers that fire XamlProfilerTracing WUC-visual-tree events so an
// out-of-process profiler can reconstruct the full Windows.UI.Composition IVisual tree
// (the dense tree DCompTreeHelper dumps as <VisualTree>).
//
// Every "inserted"/"root set" notification extracts and ships the inserted visual's full
// property set (same XML-attribute format DCompTreeHelper uses), so the consumer never has
// to read back from the live tree. All extraction work is gated behind
// XamlProfilerTracing::IsEnabled(), so these calls are nearly free when no one is listening.
//
// Visuals are passed as IUnknown* purely for identity/extraction; the helper QIs to
// WUComp::IVisual internally. Pass any interface pointer of the visual - the emitted id is
// normalized to the IVisual* identity to match DCompTreeHelper's "wucVisual" attribute.
//
// Compiled into chk/Debug builds only (XAMLPROFILER_ENABLED defined alongside DBG when
// Configuration==Debug). In retail this header is empty and WucVisualTreeProfiler.cpp is
// excluded from the build; every call site #includes this header inside its own
// #ifdef XAMLPROFILER_ENABLED (SwipeTestHooks convention).
#ifdef XAMLPROFILER_ENABLED
namespace WucVisualTreeProfiler
{
    // Cheap cached check: true only when a session is subscribed to the XamlProfilerTracing
    // provider. Forwards to XamlProfilerTracing::IsEnabled() (a cached bool kept fresh by ETW's
    // enable/disable callback - no per-call syscall). Call sites use this to skip ALL emit work
    // (including building the arguments passed to the Notify* helpers below) when nobody listens.
    bool IsEnabled();

    // A WUC visual 'childVisual' was inserted into 'parentVisual's children collection.
    // ownerCompNodeId is the HWCompNode that owns childVisual (0 if unknown). index is the
    // child position when known, or -1 when the call site only knows "top"/"bottom".
    void NotifyChildInserted(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* childVisual,
        uint64_t ownerCompNodeId,
        int32_t index);

    // 'childVisual' was removed from 'parentVisual's children collection.
    void NotifyChildRemoved(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* childVisual);

    // All children of 'parentVisual' were removed at once (IVisualCollection::RemoveAll).
    void NotifyChildrenCleared(
        _In_opt_ IUnknown* parentVisual);

    // 'visual' was set as the root of a composition target/island (ContentIsland::Root or
    // XamlIslandRoot::SetRootVisual). targetId identifies the target/island; ownerCompNodeId
    // is the owning HWCompNode (0 if unknown).
    void NotifyRootSet(
        _In_opt_ IUnknown* visual,
        uint64_t targetId,
        uint64_t ownerCompNodeId);

    // The root of the composition target/island identified by targetId was cleared (null).
    void NotifyRootCleared(uint64_t targetId);

    // =====================================================================
    // Pick-overlay suppression
    //
    // The profiler's pick-mode tap injects a transparent capture overlay (a Popup hosting a
    // Border) into the target app. XAML renders that overlay into its own comp node + WUC
    // visual spine, which would otherwise surface as phantom nodes in the very trees the
    // profiler is inspecting. The tap stamps the overlay element's Name with c_overlayNamePrefix;
    // the core detects that Name when the element's comp peer is created and marks the comp node
    // as suppressed BEFORE any of its tree events are emitted. Suppression propagates to
    // descendant comp nodes, and every comp-node / WUC-visual emission below is gated on it.
    // =====================================================================

    // Name (GetDebugLabel) prefix the tap stamps onto its overlay Popup/Border. Must match the
    // tap's StartPick stamping (Tap.cpp).
    constexpr const wchar_t* c_overlayNamePrefix = L"__xp_pick_overlay";

    // Marks compNodeId as a profiler-owned overlay node whose comp-node-tree and WUC-visual
    // events must NOT be emitted. No-op when compNodeId == 0.
    void SuppressCompNode(uint64_t compNodeId);

    // Clears a previous SuppressCompNode. Call when the comp peer is torn down so a later,
    // unrelated comp node reusing the same pointer address is not wrongly suppressed.
    void UnsuppressCompNode(uint64_t compNodeId);

    // True if compNodeId was marked via SuppressCompNode. Used to gate emissions.
    bool IsCompNodeSuppressed(uint64_t compNodeId);
}

#endif // XAMLPROFILER_ENABLED
