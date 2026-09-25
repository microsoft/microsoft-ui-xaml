// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include "WindowPlacementPolicy.h"

#include <tuple>
#include <utility>

using namespace DirectUI::WindowPlacementPersistence;

namespace
{
    // Synthetic topology. Injected geometry exercises selection and adjustment only; it
    // is not evidence about real monitor notifications or native placement results.
    MonitorDescription Monitor(const char16_t* name, Rect workArea, int32_t dpi)
    {
        MonitorDescription monitor;
        monitor.DeviceName = name;
        monitor.WorkArea = workArea;
        monitor.Dpi = dpi;
        return monitor;
    }

    const MonitorDescription& Primary()
    {
        static const auto monitor = Monitor(u"\\\\.\\DISPLAY1", {0, 0, 1920, 1000}, 96);
        return monitor;
    }

    const MonitorDescription& Secondary()
    {
        static const auto monitor = Monitor(u"\\\\.\\DISPLAY2", {1920, 0, 2560, 1400}, 192);
        return monitor;
    }

    Topology BothMonitors()
    {
        return Topology{{Primary(), Secondary()}};
    }

    Topology PrimaryOnly()
    {
        return Topology{{Primary()}};
    }

    Snapshot SavedOnSecondary(State state = State::Normal)
    {
        Snapshot placement;
        placement.NormalRect = {2000, 100, 800, 600};
        placement.WorkArea = Secondary().WorkArea;
        placement.Dpi = Secondary().Dpi;
        placement.DisplayDeviceName = Secondary().DeviceName;
        placement.PlacementState = state;
        if (state == State::Snapped || state == State::MinimizedFromSnapped)
        {
            placement.SnapRect = Rect{1920, 0, 1280, 1400};
        }
        return placement;
    }

    PlacementRequest RequestFor(const Snapshot& source)
    {
        PlacementRequest request;
        request.Source = source;
        return request;
    }

    int64_t Right(const Rect& rect) { return int64_t{rect.X} + rect.Width; }
    int64_t Bottom(const Rect& rect) { return int64_t{rect.Y} + rect.Height; }

    bool IsWithin(const Rect& rect, const Rect& workArea)
    {
        return rect.X >= workArea.X && rect.Y >= workArea.Y &&
            Right(rect) <= Right(workArea) && Bottom(rect) <= Bottom(workArea);
    }
}

class WindowPlacementPolicyTests
{
public:
    TEST_CLASS(WindowPlacementPolicyTests);

    TEST_METHOD(MatchesSavedDeviceNameEvenWhenGeometryChanged)
    {
        auto moved = Secondary();
        moved.WorkArea = {-2560, -200, 2560, 1400};
        Topology topology{{Primary(), moved}};

        MonitorDescription selected;
        VERIFY_IS_TRUE(SelectMonitor(SavedOnSecondary(), topology, selected));
        VERIFY_IS_TRUE(selected.DeviceName == Secondary().DeviceName);
    }

    TEST_METHOD(MissingMonitorSelectsLargestIntersection)
    {
        auto source = SavedOnSecondary();
        source.DisplayDeviceName = u"\\\\.\\DISPLAY9";
        source.NormalRect = {1800, 100, 400, 300};

        MonitorDescription selected;
        VERIFY_IS_TRUE(SelectMonitor(source, BothMonitors(), selected));
        // 120 columns overlap DISPLAY1, 280 columns overlap DISPLAY2.
        VERIFY_IS_TRUE(selected.DeviceName == Secondary().DeviceName);
    }

    TEST_METHOD(MissingMonitorFallsBackToNearestNotPrimary)
    {
        auto source = SavedOnSecondary();
        source.DisplayDeviceName = u"\\\\.\\DISPLAY9";
        source.NormalRect = {5000, 100, 400, 300};
        source.WorkArea = {4800, 0, 1000, 800};

        MonitorDescription selected;
        VERIFY_IS_TRUE(SelectMonitor(source, BothMonitors(), selected));
        VERIFY_IS_TRUE(selected.DeviceName == Secondary().DeviceName);
    }

    TEST_METHOD(EmptyTopologyHasNoTarget)
    {
        MonitorDescription selected;
        VERIFY_IS_FALSE(SelectMonitor(SavedOnSecondary(), Topology{}, selected));

        PlacementPlan plan;
        VERIFY_IS_FALSE(ComputePlacement(RequestFor(SavedOnSecondary()), Topology{}, plan));
    }

    TEST_METHOD(InvalidSourceProducesNoPlan)
    {
        auto source = SavedOnSecondary();
        source.NormalRect = {90000, 100, 800, 600}; // No intersection with the saved work area.

        PlacementPlan plan;
        plan.NormalRect = {7, 7, 7, 7};
        VERIFY_IS_FALSE(ComputePlacement(RequestFor(source), BothMonitors(), plan));
        VERIFY_ARE_EQUAL(plan.NormalRect.X, 7);
    }

    TEST_METHOD(RelocatesAndScalesForLowerDpiMonitor)
    {
        auto source = SavedOnSecondary();
        source.DisplayDeviceName = u"\\\\.\\DISPLAY9";

        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(source), PrimaryOnly(), plan));
        VERIFY_IS_TRUE(plan.TargetMonitor.DeviceName == Primary().DeviceName);
        // 192 -> 96 DPI halves the physical size.
        VERIFY_ARE_EQUAL(plan.NormalRect.Width, 400);
        VERIFY_ARE_EQUAL(plan.NormalRect.Height, 300);
        VERIFY_IS_TRUE(IsWithin(plan.NormalRect, Primary().WorkArea));
    }

    TEST_METHOD(ScalesSizeUpForHigherDpiMonitor)
    {
        Snapshot source;
        source.NormalRect = {100, 100, 800, 600};
        source.WorkArea = Primary().WorkArea;
        source.Dpi = 96;
        source.DisplayDeviceName = Secondary().DeviceName;

        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(source), BothMonitors(), plan));
        VERIFY_ARE_EQUAL(plan.NormalRect.Width, 1600);
        VERIFY_ARE_EQUAL(plan.NormalRect.Height, 1200);
    }

    TEST_METHOD(WorkAreaChangeKeepsRelativePosition)
    {
        Snapshot source;
        source.NormalRect = {1500, 100, 300, 200};
        source.WorkArea = {0, 0, 1920, 1000};
        source.Dpi = 96;
        source.DisplayDeviceName = u"\\\\.\\DISPLAY1";

        // The taskbar moved to the left edge and shrank the work area.
        Topology topology{{Monitor(u"\\\\.\\DISPLAY1", {80, 0, 1840, 1000}, 96)}};

        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(source), topology, plan));
        VERIFY_IS_TRUE(IsWithin(plan.NormalRect, topology.Monitors[0].WorkArea));
        VERIFY_IS_TRUE(plan.NormalRect.X > 1400);
    }

    TEST_METHOD(OffScreenWindowIsBroughtBackOnScreen)
    {
        Snapshot source;
        source.NormalRect = {1800, 900, 600, 400};
        source.WorkArea = {0, 0, 1920, 1000};
        source.Dpi = 96;

        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(source), PrimaryOnly(), plan));
        VERIFY_IS_TRUE(IsWithin(plan.NormalRect, Primary().WorkArea));
    }

    TEST_METHOD(OversizedResizableWindowIsFittedButFixedSizeIsNot)
    {
        Snapshot source;
        source.NormalRect = {0, 0, 1900, 980};
        source.WorkArea = {0, 0, 1920, 1000};
        source.Dpi = 96;

        Topology small{{Monitor(u"\\\\.\\DISPLAY1", {0, 0, 1024, 600}, 96)}};

        auto request = RequestFor(source);
        PlacementPlan fitted;
        VERIFY_IS_TRUE(ComputePlacement(request, small, fitted));
        VERIFY_IS_TRUE(fitted.NormalRect.Width <= 1024);
        VERIFY_IS_TRUE(fitted.NormalRect.Height <= 600);

        request.Constraints.AllowSizing = false;
        PlacementPlan unfitted;
        VERIFY_IS_TRUE(ComputePlacement(request, small, unfitted));
        VERIFY_ARE_EQUAL(unfitted.NormalRect.Width, 1900);
        // Right/bottom are checked first, so the caption stays reachable.
        VERIFY_ARE_EQUAL(unfitted.NormalRect.X, 0);
        VERIFY_ARE_EQUAL(unfitted.NormalRect.Y, 0);
    }

    TEST_METHOD(CurrentConstraintsOverrideSavedSize)
    {
        auto request = RequestFor(SavedOnSecondary());
        request.Constraints.MinWidth = 1200;
        request.Constraints.MaxHeight = 200;

        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), plan));
        VERIFY_ARE_EQUAL(plan.NormalRect.Width, 1200);
        VERIFY_ARE_EQUAL(plan.NormalRect.Height, 200);
        VERIFY_IS_TRUE(IsWithin(plan.NormalRect, Secondary().WorkArea));
    }

    TEST_METHOD(MapsEverySavedStateForOrdinaryDisplay)
    {
        const std::pair<State, TargetState> expected[] = {
            {State::Normal, TargetState::Normal},
            {State::Maximized, TargetState::Maximized},
            {State::Minimized, TargetState::Normal},
            {State::MinimizedFromMaximized, TargetState::Maximized},
            {State::Snapped, TargetState::Snapped},
            {State::MinimizedFromSnapped, TargetState::Snapped},
        };

        for (const auto& [saved, target] : expected)
        {
            PlacementPlan plan;
            VERIFY_IS_TRUE(ComputePlacement(RequestFor(SavedOnSecondary(saved)), BothMonitors(), plan));
            VERIFY_IS_TRUE(plan.State == target);
            VERIFY_IS_FALSE(plan.RestoreToMaximized);
            VERIFY_IS_FALSE(plan.RestoreToSnapped);
        }
    }

    TEST_METHOD(ApplicationRestartPreservesMinimization)
    {
        const std::tuple<State, bool, bool> expected[] = {
            {State::Minimized, false, false},
            {State::MinimizedFromMaximized, true, false},
            {State::MinimizedFromSnapped, false, true},
        };

        for (const auto& [saved, restoreMax, restoreSnap] : expected)
        {
            auto request = RequestFor(SavedOnSecondary(saved));
            request.Reason = PlacementReason::ApplicationRestart;

            PlacementPlan plan;
            VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), plan));
            VERIFY_IS_TRUE(plan.State == TargetState::Minimized);
            VERIFY_ARE_EQUAL(plan.RestoreToMaximized, restoreMax);
            VERIFY_ARE_EQUAL(plan.RestoreToSnapped, restoreSnap);
        }
    }

    TEST_METHOD(UnavailableSnappingFallsBackToNormal)
    {
        auto request = RequestFor(SavedOnSecondary(State::Snapped));
        request.Constraints.SnappingEnabled = false;

        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), plan));
        VERIFY_IS_TRUE(plan.State == TargetState::Normal);
        VERIFY_IS_FALSE(plan.SnapRect.has_value());

        // A restart-minimized window stays minimized and restores to normal.
        auto restart = RequestFor(SavedOnSecondary(State::MinimizedFromSnapped));
        restart.Reason = PlacementReason::ApplicationRestart;
        restart.Constraints.SnappingEnabled = false;

        PlacementPlan restartPlan;
        VERIFY_IS_TRUE(ComputePlacement(restart, BothMonitors(), restartPlan));
        VERIFY_IS_TRUE(restartPlan.State == TargetState::Minimized);
        VERIFY_IS_FALSE(restartPlan.RestoreToSnapped);
    }

    TEST_METHOD(SnapRectKeepsWorkAreaEdgeRelationship)
    {
        auto source = SavedOnSecondary(State::Snapped);
        source.DisplayDeviceName = u"\\\\.\\DISPLAY9"; // Force relocation to DISPLAY1.

        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(source), PrimaryOnly(), plan));
        VERIFY_IS_TRUE(plan.State == TargetState::Snapped);
        VERIFY_IS_TRUE(plan.SnapRect.has_value());
        // Left-snapped stays flush with the new left edge and full work-area height.
        VERIFY_ARE_EQUAL(plan.SnapRect->X, Primary().WorkArea.X);
        VERIFY_ARE_EQUAL(plan.SnapRect->Height, Primary().WorkArea.Height);
        VERIFY_IS_TRUE(plan.SnapRect->Width < Primary().WorkArea.Width);
    }

    TEST_METHOD(LaunchMonitorHintOverridesSavedMonitor)
    {
        auto request = RequestFor(SavedOnSecondary());
        request.LaunchMonitorHint = Primary();

        PlacementPlan ignored;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), ignored));
        VERIFY_IS_TRUE(ignored.TargetMonitor.DeviceName == Secondary().DeviceName);

        request.Reason = PlacementReason::Launch;
        PlacementPlan hinted;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), hinted));
        VERIFY_IS_TRUE(hinted.TargetMonitor.DeviceName == Primary().DeviceName);

        // An unusable hint falls back to normal saved-monitor selection.
        request.LaunchMonitorHint = Monitor(u"\\\\.\\DISPLAY7", {0, 0, 0, 0}, 0);
        PlacementPlan fallback;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), fallback));
        VERIFY_IS_TRUE(fallback.TargetMonitor.DeviceName == Secondary().DeviceName);
    }

    TEST_METHOD(RestartRestoresSavedDesktopAndOthersIgnoreIt)
    {
        auto source = SavedOnSecondary();
        source.VirtualDesktopId = GUID{0x1, 0x2, 0x3, {0x4}};

        PlacementPlan ordinary;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(source), BothMonitors(), ordinary));
        VERIFY_IS_FALSE(ordinary.VirtualDesktopId.has_value());

        auto request = RequestFor(source);
        request.Reason = PlacementReason::ApplicationRestart;
        PlacementPlan restart;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), restart));
        VERIFY_IS_TRUE(restart.VirtualDesktopId.has_value());
    }

    TEST_METHOD(CascadeDecisionFollowsBehaviorAndReason)
    {
        VERIFY_IS_TRUE(IsCascadePermitted(
            CascadeBehavior::Automatic, PlacementReason::Default, true, true, false));
        VERIFY_IS_FALSE(IsCascadePermitted(
            CascadeBehavior::Automatic, PlacementReason::Default, true, true, true));
        VERIFY_IS_FALSE(IsCascadePermitted(
            CascadeBehavior::Automatic, PlacementReason::Default, true, false, false));
        VERIFY_IS_TRUE(IsCascadePermitted(
            CascadeBehavior::Enabled, PlacementReason::Default, true, false, true));
        VERIFY_IS_FALSE(IsCascadePermitted(
            CascadeBehavior::Enabled, PlacementReason::Default, false, true, false));
        VERIFY_IS_FALSE(IsCascadePermitted(
            CascadeBehavior::Disabled, PlacementReason::Default, true, true, false));
        VERIFY_IS_FALSE(IsCascadePermitted(
            CascadeBehavior::Enabled, PlacementReason::ApplicationRestart, true, true, false));
    }

    TEST_METHOD(PeerPlacementIsCascadedBeforeAdjustment)
    {
        auto source = SavedOnSecondary();
        auto request = RequestFor(source);
        request.SourceKind = PlacementSource::Peer;
        request.Cascade.Permitted = true;
        request.Cascade.Offset = 60;

        PlacementPlan plain;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(source), BothMonitors(), plain));
        PlacementPlan cascaded;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), cascaded));

        VERIFY_IS_TRUE(cascaded.Cascaded);
        VERIFY_ARE_EQUAL(cascaded.NormalRect.X, plain.NormalRect.X + 60);
        VERIFY_ARE_EQUAL(cascaded.NormalRect.Y, plain.NormalRect.Y + 60);
        VERIFY_ARE_EQUAL(cascaded.NormalRect.Width, plain.NormalRect.Width);
    }

    TEST_METHOD(CascadeWrapsAtWorkAreaEdges)
    {
        Snapshot source;
        source.NormalRect = {1300, 700, 600, 280};
        source.WorkArea = {0, 0, 1920, 1000};
        source.Dpi = 96;
        source.DisplayDeviceName = u"\\\\.\\DISPLAY1";

        auto request = RequestFor(source);
        request.SourceKind = PlacementSource::Peer;
        request.Cascade.Permitted = true;
        request.Cascade.Offset = 60;

        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(request, PrimaryOnly(), plan));
        VERIFY_IS_TRUE(plan.Cascaded);
        VERIFY_ARE_EQUAL(plan.NormalRect.X, Primary().WorkArea.X);
        VERIFY_ARE_EQUAL(plan.NormalRect.Y, Primary().WorkArea.Y);
    }

    TEST_METHOD(SavedPlacementIsNeverCascaded)
    {
        auto request = RequestFor(SavedOnSecondary());
        request.Cascade.Permitted = true;
        request.Cascade.Offset = 60;

        PlacementPlan plain;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(SavedOnSecondary()), BothMonitors(), plain));
        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), plan));

        VERIFY_IS_FALSE(plan.Cascaded);
        VERIFY_ARE_EQUAL(plan.NormalRect.X, plain.NormalRect.X);
    }

    TEST_METHOD(ExplicitCascadeMovesPositionOnly)
    {
        auto request = RequestFor(SavedOnSecondary(State::Maximized));
        request.SourceKind = PlacementSource::Explicit;
        request.Cascade.Permitted = true;
        request.Cascade.Offset = 60;
        request.Cascade.PeerAnchorNormalRect = Rect{2200, 300, 100, 100};

        PlacementPlan plain;
        VERIFY_IS_TRUE(ComputePlacement(
            RequestFor(SavedOnSecondary(State::Maximized)), BothMonitors(), plain));
        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), plan));

        VERIFY_IS_TRUE(plan.Cascaded);
        VERIFY_ARE_EQUAL(plan.NormalRect.X, 2260);
        VERIFY_ARE_EQUAL(plan.NormalRect.Y, 360);
        // Size, monitor, and state come from the explicit placement, not the peer.
        VERIFY_ARE_EQUAL(plan.NormalRect.Width, plain.NormalRect.Width);
        VERIFY_IS_TRUE(plan.TargetMonitor.DeviceName == plain.TargetMonitor.DeviceName);
        VERIFY_IS_TRUE(plan.State == TargetState::Maximized);
    }

    TEST_METHOD(ExplicitCascadeWithoutPeerKeepsUncascadedResult)
    {
        auto request = RequestFor(SavedOnSecondary());
        request.SourceKind = PlacementSource::Explicit;
        request.Cascade.Permitted = true;
        request.Cascade.Offset = 60;

        PlacementPlan plain;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(SavedOnSecondary()), BothMonitors(), plain));
        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), plan));

        VERIFY_IS_FALSE(plan.Cascaded);
        VERIFY_ARE_EQUAL(plan.NormalRect.X, plain.NormalRect.X);
    }

    TEST_METHOD(UnavailableCascadeOffsetDoesNotCascade)
    {
        auto request = RequestFor(SavedOnSecondary());
        request.SourceKind = PlacementSource::Peer;
        request.Cascade.Permitted = true;
        request.Cascade.Offset = 0;

        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), plan));
        VERIFY_IS_FALSE(plan.Cascaded);
    }

    TEST_METHOD(NativeRequestCarriesStateThroughFlags)
    {
        auto request = RequestFor(SavedOnSecondary(State::MinimizedFromSnapped));
        request.Reason = PlacementReason::ApplicationRestart;

        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(request, BothMonitors(), plan));

        NativeRequest native;
        VERIFY_IS_TRUE(BuildNativeRequest(plan, {}, native));
        VERIFY_IS_TRUE(native.ShowCommand == NativeShowCommand::Minimize);
        VERIFY_IS_TRUE(native.Flags.RestoreToArranged);
        VERIFY_IS_FALSE(native.Flags.Arranged);
        VERIFY_IS_TRUE(native.ArrangeRect.has_value());
        VERIFY_ARE_EQUAL(native.Dpi, Secondary().Dpi);
        VERIFY_IS_TRUE(native.DeviceName == Secondary().DeviceName);
    }

    TEST_METHOD(NativeRequestUsesTargetSizingNotSavedSizing)
    {
        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(SavedOnSecondary()), BothMonitors(), plan));

        NativeApplyOptions options;
        options.AllowSizing = false;
        NativeRequest native;
        VERIFY_IS_TRUE(BuildNativeRequest(plan, options, native));
        VERIFY_IS_FALSE(native.Flags.AllowSizing);
        VERIFY_IS_FALSE(native.Flags.NoActivate);
    }

    TEST_METHOD(HiddenApplicationNeverShowsOrActivates)
    {
        PlacementPlan normal;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(SavedOnSecondary()), BothMonitors(), normal));

        NativeApplyOptions hidden;
        hidden.KeepHidden = true;
        NativeRequest native;
        VERIFY_IS_TRUE(BuildNativeRequest(normal, hidden, native));
        VERIFY_IS_TRUE(native.ShowCommand == NativeShowCommand::NoChange);
        VERIFY_IS_TRUE(native.Flags.KeepHidden);
        VERIFY_IS_TRUE(native.Flags.NoActivate);

        // S37: the hidden maximized/minimized/snapped transition is not solved here, so
        // hidden application reports failure instead of displaying the window.
        PlacementPlan maximized;
        VERIFY_IS_TRUE(ComputePlacement(
            RequestFor(SavedOnSecondary(State::Maximized)), BothMonitors(), maximized));
        NativeRequest rejected;
        VERIFY_IS_FALSE(BuildNativeRequest(maximized, hidden, rejected));
    }

    TEST_METHOD(NonActivatingApplicationIsRequested)
    {
        PlacementPlan plan;
        VERIFY_IS_TRUE(ComputePlacement(RequestFor(SavedOnSecondary()), BothMonitors(), plan));

        NativeApplyOptions options;
        options.NoActivate = true;
        NativeRequest native;
        VERIFY_IS_TRUE(BuildNativeRequest(plan, options, native));
        VERIFY_IS_TRUE(native.Flags.NoActivate);
        VERIFY_IS_FALSE(native.Flags.KeepHidden);
        VERIFY_IS_TRUE(native.ShowCommand == NativeShowCommand::Normal);
    }

    TEST_METHOD(NoSourceRetryAfterNativeApplicationBegins)
    {
        VERIFY_IS_TRUE(AllowsSourceRetry(ApplyOutcome::NotAttempted));
        VERIFY_IS_TRUE(AllowsSourceRetry(ApplyOutcome::FailedBeforeStart));
        VERIFY_IS_FALSE(AllowsSourceRetry(ApplyOutcome::FailedAfterStart));
        VERIFY_IS_FALSE(AllowsSourceRetry(ApplyOutcome::Applied));
    }
};
