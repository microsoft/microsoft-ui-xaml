// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowPlacementAdapter.h"

#include <algorithm>

namespace DirectUI::WindowPlacementPersistence
{
    namespace
    {
        bool IsUsableMonitor(const MonitorDescription& monitor) noexcept
        {
            return monitor.Dpi > 0 && monitor.WorkArea.Width > 0 && monitor.WorkArea.Height > 0;
        }

        TargetState MapState(
            State saved,
            PlacementReason reason,
            bool snappingEnabled,
            bool& restoreToMaximized,
            bool& restoreToSnapped) noexcept
        {
            restoreToMaximized = false;
            restoreToSnapped = false;

            if (reason == PlacementReason::ApplicationRestart)
            {
                switch (saved)
                {
                    case State::Minimized:
                        return TargetState::Minimized;
                    case State::MinimizedFromMaximized:
                        restoreToMaximized = true;
                        return TargetState::Minimized;
                    case State::MinimizedFromSnapped:
                        restoreToSnapped = snappingEnabled;
                        return TargetState::Minimized;
                    default:
                        break;
                }
            }

            switch (saved)
            {
                case State::Maximized:
                case State::MinimizedFromMaximized:
                    return TargetState::Maximized;
                case State::Snapped:
                case State::MinimizedFromSnapped:
                    return snappingEnabled ? TargetState::Snapped : TargetState::Normal;
                default:
                    return TargetState::Normal;
            }
        }
    }

    bool IsCascadePermitted(
        CascadeBehavior behavior,
        PlacementReason reason,
        bool hasNonEmptyId,
        bool automaticPersistenceOptIn,
        bool hasExplicitPlacement) noexcept
    {
        if (reason == PlacementReason::ApplicationRestart) return false;
        if (!hasNonEmptyId) return false;

        switch (behavior)
        {
            case CascadeBehavior::Disabled:
                return false;
            case CascadeBehavior::Enabled:
                return true;
            case CascadeBehavior::Automatic:
                return automaticPersistenceOptIn && !hasExplicitPlacement;
        }
        return false;
    }

    bool SelectMonitor(
        const Snapshot& placement,
        const Topology& topology,
        MonitorDescription& monitor)
    {
        return TrySelectEngineMonitor(placement, topology, monitor);
    }

    bool ComputePlacement(
        const PlacementRequest& request,
        const Topology& topology,
        PlacementPlan& plan)
    {
        if (!IsValid(request.Source)) return false;

        Snapshot source = request.Source;
        bool cascaded = false;
        PlacementEx placement{};
        if (!TryCreateEnginePlacement(
                source, request.Constraints.AllowSizing, placement))
        {
            return false;
        }

        // A peer supplies a whole placement, so its normal rectangle is cascaded in the
        // peer's own coordinate space before display adjustment.
        if (request.Cascade.Permitted &&
            request.Cascade.Offset > 0 &&
            request.SourceKind == PlacementSource::Peer)
        {
            if (TryCascadeEngine(placement, request.Cascade.Offset)) cascaded = true;
        }

        MonitorDescription target{};
        if (request.Reason == PlacementReason::Launch &&
            request.LaunchMonitorHint.has_value() &&
            request.LaunchMonitorHint->Dpi > 0 &&
            request.LaunchMonitorHint->WorkArea.Width > 0 &&
            request.LaunchMonitorHint->WorkArea.Height > 0)
        {
            target = *request.LaunchMonitorHint;
        }
        else if (!TrySelectEngineMonitor(source, topology, target))
        {
            return false;
        }

        if (!TryMoveEngineToMonitor(placement, target)) return false;

        bool restoreToMaximized = false;
        bool restoreToSnapped = false;
        auto state = MapState(
            source.PlacementState,
            request.Reason,
            request.Constraints.SnappingEnabled,
            restoreToMaximized,
            restoreToSnapped);

        std::optional<Rect> snapRect;
        if (state == TargetState::Snapped || restoreToSnapped)
        {
            NativeRequest adjusted{};
            if (TryReadEnginePlacement(placement, adjusted) && adjusted.ArrangeRect)
                snapRect = adjusted.ArrangeRect;
            else
            {
                restoreToSnapped = false;
                if (state == TargetState::Snapped) state = TargetState::Normal;
            }
        }

        NativeRequest adjusted{};
        if (!TryReadEnginePlacement(placement, adjusted)) return false;
        auto normal = adjusted.NormalRect;
        auto width = int64_t{normal.Width};
        auto height = int64_t{normal.Height};
        if (request.Constraints.MinWidth > 0) width = (std::max)(width, int64_t{request.Constraints.MinWidth});
        if (request.Constraints.MinHeight > 0) height = (std::max)(height, int64_t{request.Constraints.MinHeight});
        if (request.Constraints.MaxWidth > 0) width = (std::min)(width, int64_t{request.Constraints.MaxWidth});
        if (request.Constraints.MaxHeight > 0) height = (std::min)(height, int64_t{request.Constraints.MaxHeight});
        if (width <= 0 || height <= 0 || width > INT32_MAX || height > INT32_MAX) return false;
        normal.Width = static_cast<int32_t>(width);
        normal.Height = static_cast<int32_t>(height);
        if (normal.X > INT32_MAX - normal.Width || normal.Y > INT32_MAX - normal.Height) return false;
        Snapshot constrained = source;
        constrained.NormalRect = normal;
        constrained.WorkArea = target.WorkArea;
        constrained.Dpi = target.Dpi;
        constrained.DisplayDeviceName = target.DeviceName;
        constrained.SnapRect = snapRect;
        if (!TryCreateEnginePlacement(constrained, request.Constraints.AllowSizing, placement) ||
            !TryMoveEngineToMonitor(placement, target) ||
            !TryReadEnginePlacement(placement, adjusted))
        {
            return false;
        }
        normal = adjusted.NormalRect;

        // Explicit placement permits a position adjustment only, so its adjusted size,
        // monitor, and state survive the cascade.
        if (request.Cascade.Permitted &&
            request.Cascade.Offset > 0 &&
            request.SourceKind == PlacementSource::Explicit &&
            request.Cascade.PeerAnchorNormalRect.has_value())
        {
            Snapshot explicitCascade = constrained;
            explicitCascade.NormalRect = *request.Cascade.PeerAnchorNormalRect;
            if (TryCreateEnginePlacement(explicitCascade, request.Constraints.AllowSizing, placement) &&
                TryMoveEngineToMonitor(placement, target) &&
                TryCascadeEngine(placement, request.Cascade.Offset) &&
                TryReadEnginePlacement(placement, adjusted))
            {
                normal = {adjusted.NormalRect.X, adjusted.NormalRect.Y, normal.Width, normal.Height};
                cascaded = true;
            }
        }

        PlacementPlan result{};
        result.NormalRect = normal;
        result.SnapRect = snapRect;
        result.State = state;
        result.RestoreToMaximized = restoreToMaximized;
        result.RestoreToSnapped = restoreToSnapped;
        result.TargetMonitor = target;
        result.Cascaded = cascaded;
        if (request.Reason == PlacementReason::ApplicationRestart)
        {
            result.VirtualDesktopId = source.VirtualDesktopId;
        }

        plan = result;
        return true;
    }

    bool BuildNativeRequest(
        const PlacementPlan& plan,
        const NativeApplyOptions& options,
        NativeRequest& request)
    {
        if (!IsUsableMonitor(plan.TargetMonitor)) return false;
        if (plan.NormalRect.Width <= 0 || plan.NormalRect.Height <= 0) return false;

        // Hidden application must not show the window and repair visibility afterward.
        if (options.KeepHidden && plan.State != TargetState::Normal) return false;

        NativeRequest result{};
        result.NormalRect = plan.NormalRect;
        result.WorkArea = plan.TargetMonitor.WorkArea;
        result.Dpi = plan.TargetMonitor.Dpi;
        result.DeviceName = plan.TargetMonitor.DeviceName;
        result.VirtualDesktopId = plan.VirtualDesktopId;

        switch (plan.State)
        {
            case TargetState::Maximized:
                result.ShowCommand = NativeShowCommand::Maximize;
                break;
            case TargetState::Minimized:
                result.ShowCommand = NativeShowCommand::Minimize;
                break;
            default:
                result.ShowCommand = options.KeepHidden ?
                    NativeShowCommand::NoChange : NativeShowCommand::Normal;
                break;
        }

        if (plan.State == TargetState::Snapped)
        {
            if (!plan.SnapRect.has_value()) return false;
            result.ArrangeRect = plan.SnapRect;
            result.Flags.Arranged = true;
        }
        else if (plan.RestoreToSnapped)
        {
            if (!plan.SnapRect.has_value()) return false;
            result.ArrangeRect = plan.SnapRect;
            result.Flags.RestoreToArranged = true;
        }

        result.Flags.RestoreToMaximized = plan.RestoreToMaximized;
        result.Flags.KeepHidden = options.KeepHidden;
        result.Flags.NoActivate = options.NoActivate || options.KeepHidden;
        result.Flags.AllowSizing = options.AllowSizing;
        result.Flags.UseVirtualDesktopId = plan.VirtualDesktopId.has_value();

        request = result;
        return true;
    }

    bool AllowsSourceRetry(ApplyOutcome outcome) noexcept
    {
        return outcome == ApplyOutcome::NotAttempted || outcome == ApplyOutcome::FailedBeforeStart;
    }
}
