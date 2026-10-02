// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowPlacementCapture.h"
#include "WindowPlacementCoordinator.h"
#include "WindowPlacementDiagnostics.h"

namespace DirectUI::WindowPlacementPersistence
{
    // Supplies the cascade anchor for explicitly supplied placement. It is called only
    // after the target monitor is resolved, because the peer must be on that monitor.
    // An absent callback, a failed lookup, or an invalid peer leaves the explicit
    // result uncascaded.
    struct PeerAnchorSource
    {
        bool (*TryGetAnchor)(
            void* context,
            const std::u16string& targetDeviceName,
            Snapshot& peer){nullptr};
        void* Context{nullptr};
    };

    // The launch monitor hint arrives in STARTUPINFOW::hStdOutput, but that field is the
    // standard output handle when STARTF_USESTDHANDLES is set, and the two uses are
    // mutually exclusive. A process started with redirected output must not have its file
    // or pipe handle reinterpreted as an HMONITOR. Returns the candidate handle only when
    // the field is a hint; the caller still validates it against the live topology.
    bool TryGetLaunchMonitorHint(const STARTUPINFOW& startup, HMONITOR& monitor) noexcept;

    // Display device names come from different Win32 calls at different times, so they
    // are compared case-insensitively by ordinal rather than byte for byte.
    bool ArePlacementDeviceNamesEqual(
        const std::u16string& left,
        const std::u16string& right) noexcept;

    // The acceptance rule for one candidate in a peer walk. An empty required device
    // name accepts any monitor. A candidate that fails this rule is skipped, and the
    // caller must keep walking, because a peer lower in Z order may still qualify.
    bool IsAcceptablePlacementPeer(
        const Snapshot& candidate,
        const std::u16string& requiredDeviceName) noexcept;

    // Converts one WM_GETMINMAXINFO track size from the window's current DPI to the
    // target monitor's DPI. A value of zero or less means unconstrained and stays zero.
    // Returns false when the ratio cannot be applied, including a zero or out-of-range
    // DPI and the overflow that MulDiv reports as a negative result. This is the only
    // place production rescales track sizes, and it is an identity when the window and
    // the monitor share a DPI.
    bool TryScaleTrackSize(
        LONG value,
        UINT windowDpi,
        UINT monitorDpi,
        int32_t& scaled) noexcept;

    // Pull-based candidate supply for a peer walk. TryGetNext produces the next
    // candidate in Z order, topmost first, and returns false when the walk is
    // exhausted. The source owns its own cursor, so the walk itself stays testable.
    struct PlacementPeerCandidateSource
    {
        bool (*TryGetNext)(void* context, Snapshot& candidate){nullptr};
        void* Context{nullptr};
    };

    // Returns the topmost candidate that satisfies IsAcceptablePlacementPeer. A
    // rejected candidate does not end the walk, so a lower peer on the required
    // monitor is still found.
    bool TryFindAcceptablePlacementPeer(
        const PlacementPeerCandidateSource& candidates,
        const std::u16string& requiredDeviceName,
        Snapshot& peer) noexcept;

    // One source-selection pass. Explicit placement never falls through to storage.
    // Missing/unavailable storage and unsupported native operations are best effort.
    bool TryApplyPlacement(
        HWND hwnd,
        const PlacementPass& pass,
        WindowPlacementCapture& capture,
        long (*load)(const char16_t*, size_t, LoadResult&) = ReadPersistPlacementCore,
        const PeerAnchorSource& peerAnchor = {});

    // Live monitor selection is delegated to PlacementEx (not the synthetic topology
    // selector). Current HWND track constraints and snapping policy feed ComputePlacement.
    bool TryComputeNativeRequest(
        HWND hwnd,
        const Snapshot& source,
        PlacementReason reason,
        const NativeApplyOptions& options,
        PlacementSource sourceKind,
        bool cascadePermitted,
        const PeerAnchorSource& peerAnchor,
        NativeRequest& request);

    bool TryComputeNativeRequest(
        HWND hwnd,
        const Snapshot& source,
        PlacementReason reason,
        const NativeApplyOptions& options,
        NativeRequest& request);

    // The cache is refreshed even after a partial native failure. Applied means both
    // native application and effective capture succeeded. No caller should retry a
    // different source after FailedAfterStart.
    ApplyOutcome TryApplyNativeRequest(
        HWND hwnd,
        const NativeRequest& request,
        WindowPlacementCapture& capture,
        bool forceLegacyBackend = false,
        PlacementFailure* failure = nullptr);
}
