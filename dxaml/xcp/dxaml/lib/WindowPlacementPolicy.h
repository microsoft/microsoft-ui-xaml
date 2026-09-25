// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowPlacementRecord.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace DirectUI::WindowPlacementPersistence
{
    // Placement policy. This unit owns monitor selection, geometry adjustment, state
    // mapping, and cascading. It never touches an HWND, storage, or the public API, so
    // every rule here can be tested against a synthetic topology.

    struct MonitorDescription
    {
        std::u16string DeviceName;
        Rect WorkArea;
        int32_t Dpi{96};
    };

    struct Topology
    {
        std::vector<MonitorDescription> Monitors;
    };

    enum class PlacementReason
    {
        Default,
        Launch,
        ApplicationRestart
    };

    enum class CascadeBehavior
    {
        Automatic,
        Enabled,
        Disabled
    };

    enum class PlacementSource
    {
        Saved,
        Peer,
        Explicit
    };

    // Current-window constraints. These come from the target window, never from the
    // saved source window. Zero means unconstrained.
    struct WindowConstraints
    {
        bool AllowSizing{true};
        bool SnappingEnabled{true};
        int32_t MinWidth{};
        int32_t MinHeight{};
        int32_t MaxWidth{};
        int32_t MaxHeight{};
    };

    struct CascadeInput
    {
        bool Permitted{false};
        // Caption plus frame offset in physical pixels, in the coordinate space the
        // cascade runs in. Zero or negative disables cascading, which is how a caller
        // reports that it could not read system metrics safely.
        int32_t Offset{};
        // Anchor used only when explicit placement is cascaded. It must already be in
        // target-monitor coordinates.
        std::optional<Rect> PeerAnchorNormalRect;
    };

    struct PlacementRequest
    {
        Snapshot Source;
        PlacementSource SourceKind{PlacementSource::Saved};
        PlacementReason Reason{PlacementReason::Default};
        WindowConstraints Constraints;
        CascadeInput Cascade;
        // Honored only for PlacementReason::Launch.
        std::optional<MonitorDescription> LaunchMonitorHint;
    };

    enum class TargetState
    {
        Normal,
        Maximized,
        Minimized,
        Snapped
    };

    struct PlacementPlan
    {
        Rect NormalRect{};
        std::optional<Rect> SnapRect;
        TargetState State{TargetState::Normal};
        bool RestoreToMaximized{false};
        bool RestoreToSnapped{false};
        MonitorDescription TargetMonitor;
        std::optional<GUID> VirtualDesktopId;
        bool Cascaded{false};
    };

    // The cascade decision table. It uses the initial opt-in and id, not whether storage
    // would succeed. ApplicationRestart suppresses cascading for every value.
    bool IsCascadePermitted(
        CascadeBehavior behavior,
        PlacementReason reason,
        bool hasNonEmptyId,
        bool automaticPersistenceOptIn,
        bool hasExplicitPlacement) noexcept;

    // Matches the saved display device name first, then the monitor with the largest
    // intersection with the saved normal rectangle, then the nearest monitor.
    bool SelectMonitor(
        const Snapshot& placement,
        const Topology& topology,
        MonitorDescription& monitor);

    // Fails, without publishing a plan, when the source is invalid, the topology is
    // empty, or an adjustment cannot be represented safely.
    bool ComputePlacement(
        const PlacementRequest& request,
        const Topology& topology,
        PlacementPlan& plan);

    // Backend-neutral native request. The public API does not depend on native flag
    // values, so the policy layer describes the operation and the native adapter binds
    // it to PlacementEx.
    enum class NativeShowCommand
    {
        Normal,
        Maximize,
        Minimize,
        NoChange
    };

    struct NativeFlags
    {
        bool NoActivate{false};
        bool KeepHidden{false};
        bool Arranged{false};
        bool RestoreToArranged{false};
        bool RestoreToMaximized{false};
        bool AllowSizing{true};
        bool UseVirtualDesktopId{false};
    };

    struct NativeRequest
    {
        Rect NormalRect{};
        Rect WorkArea{};
        int32_t Dpi{};
        std::u16string DeviceName;
        std::optional<Rect> ArrangeRect;
        std::optional<GUID> VirtualDesktopId;
        NativeShowCommand ShowCommand{NativeShowCommand::Normal};
        NativeFlags Flags;
    };

    struct NativeApplyOptions
    {
        bool KeepHidden{false};
        bool NoActivate{false};
        // Read from the target window at application time, never from the saved source.
        bool AllowSizing{true};
    };

    // Fails when the caller asks for a combination the native path cannot honor without
    // showing or activating first and repairing afterward. Hidden application is limited
    // to normal geometry; the hidden maximized/minimized/snapped transition is the open
    // S37 integration gate, and reporting failure is what lets hidden application return
    // false instead of briefly displaying the window.
    bool BuildNativeRequest(
        const PlacementPlan& plan,
        const NativeApplyOptions& options,
        NativeRequest& request);

    enum class ApplyOutcome
    {
        NotAttempted,
        Applied,
        FailedBeforeStart,
        FailedAfterStart
    };

    // Once native application begins, failure does not cause another peer search or a
    // storage load.
    bool AllowsSourceRetry(ApplyOutcome outcome) noexcept;
}
