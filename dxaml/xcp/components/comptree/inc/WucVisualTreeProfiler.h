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
// to read back from the live tree. All extraction work is gated behind the inline WucTree
// keyword check (IsEnabled() below), so these calls are nearly free when no one is listening.
//
// Visuals are passed as IUnknown* purely for identity/extraction; the helper QIs to
// WUComp::IVisual internally. Pass any interface pointer of the visual - the emitted id is
// normalized to the IVisual* identity to match DCompTreeHelper's "wucVisual" attribute.
//
// Compiled in whenever XAMLPROFILER_ENABLED is defined, which is every build configuration
// unless the build passes /p:XamlProfilerEnabled=false. Without the define this header is
// empty and WucVisualTreeProfiler.cpp is excluded from the build; every call site #includes
// this header inside its own #ifdef XAMLPROFILER_ENABLED (SwipeTestHooks convention).
#ifdef XAMLPROFILER_ENABLED
#include "XamlProfilerTracing.h"

class HWCompNode;

namespace WucVisualTreeProfiler
{
    // True only when a session has enabled the provider's WucTree group at verbose level. Inline
    // load, compare and branch on the provider's enable state, which ETW keeps current through its
    // enable callback. Call sites use this to skip ALL emit work (including building the
    // arguments passed to the Notify* helpers below) when nobody listens.
    __forceinline bool IsEnabled()
    {
        return XamlProfilerTracing::IsEnabled(XamlProfilerKeywords::WucTree);
    }

    // A WUC visual 'childVisual' was inserted into 'parentVisual's children collection.
    // ownerCompNode is the HWCompNode that owns childVisual (null if unknown); its address is
    // emitted as OwnerCompNodeId, and nothing is emitted when it is a suppressed pick-overlay
    // node. index is the child position when known, or -1 when the call site only knows
    // "top"/"bottom".
    void NotifyChildInserted(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* childVisual,
        _In_opt_ const HWCompNode* ownerCompNode,
        int32_t index);

    // 'childVisual' was removed from 'parentVisual's children collection.
    void NotifyChildRemoved(
        _In_opt_ IUnknown* parentVisual,
        _In_opt_ IUnknown* childVisual);

    // All children of 'parentVisual' were removed at once (IVisualCollection::RemoveAll).
    void NotifyChildrenCleared(
        _In_opt_ IUnknown* parentVisual);

    // 'visual' was set as the root of a composition target/island (ContentIsland::Root or
    // XamlIslandRoot::SetRootVisual). targetId identifies the target/island; ownerCompNode
    // is the owning HWCompNode (null if unknown), handled as in NotifyChildInserted.
    void NotifyRootSet(
        _In_opt_ IUnknown* visual,
        uint64_t targetId,
        _In_opt_ const HWCompNode* ownerCompNode);

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
    // (HWCompNode::MarkProfilerSuppressed) BEFORE any of its tree events are emitted. The mark
    // propagates to descendant comp nodes, and every comp-node / WUC-visual emission gates on
    // HWCompNode::IsProfilerSuppressed(). The mark is a bit on the node itself, so there is no
    // global registry, no lock, and no cleanup on comp-node teardown.
    // =====================================================================

    // Name (GetDebugLabel) prefix the tap stamps onto its overlay Popup/Border. Must match the
    // tap's StartPick stamping (Tap.cpp).
    constexpr const wchar_t* c_overlayNamePrefix = L"__xp_pick_overlay";
}

#endif // XAMLPROFILER_ENABLED
