// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowPlacementApplication.h"
#include "WindowPlacementAdapter.h"
#include "WindowPlacementPhysicalCoordinates.h"
#include "WindowPlacementStore.h"

#include <utility>

namespace DirectUI::WindowPlacementPersistence
{
    namespace
    {
        bool TryGetLaunchMonitor(MonitorData& monitor)
        {
            STARTUPINFOW startup{sizeof(startup)};
            ::GetStartupInfoW(&startup);
            HMONITOR hint{};
            if (!TryGetLaunchMonitorHint(startup, hint)) return false;
            // Only the monitor hint is consumed. Never replay launcher show commands.
            return MonitorData::FromHandle(hint, &monitor);
        }
    }

    bool TryGetLaunchMonitorHint(const STARTUPINFOW& startup, HMONITOR& monitor) noexcept
    {
        monitor = nullptr;
        // STARTUPINFOW overloads hStdOutput. It is a monitor handle only when the launcher
        // asked the system for one, which cannot be combined with STARTF_USESTDHANDLES. A
        // process started with redirected output carries a real file or pipe handle here, so
        // the flag decides the meaning rather than whether GetMonitorInfo rejects the value.
        if (!startup.hStdOutput || (startup.dwFlags & STARTF_USESTDHANDLES) != 0) return false;
        monitor = reinterpret_cast<HMONITOR>(startup.hStdOutput);
        return true;
    }

    bool ArePlacementDeviceNamesEqual(
        const std::u16string& left,
        const std::u16string& right) noexcept
    {
        return left.size() <= INT_MAX && right.size() <= INT_MAX &&
            ::CompareStringOrdinal(
                reinterpret_cast<LPCWCH>(left.data()), static_cast<int>(left.size()),
                reinterpret_cast<LPCWCH>(right.data()), static_cast<int>(right.size()),
                TRUE) == CSTR_EQUAL;
    }

    bool IsAcceptablePlacementPeer(
        const Snapshot& candidate,
        const std::u16string& requiredDeviceName) noexcept
    {
        return IsValid(candidate) &&
            (requiredDeviceName.empty() ||
                ArePlacementDeviceNamesEqual(candidate.DisplayDeviceName, requiredDeviceName));
    }

    bool TryFindAcceptablePlacementPeer(
        const PlacementPeerCandidateSource& candidates,
        const std::u16string& requiredDeviceName,
        Snapshot& peer) noexcept
    {
        if (!candidates.TryGetNext) return false;

        Snapshot candidate{};
        while (candidates.TryGetNext(candidates.Context, candidate))
        {
            if (IsAcceptablePlacementPeer(candidate, requiredDeviceName) &&
                TryCopySnapshot(candidate, peer))
            {
                return true;
            }
        }
        return false;
    }

    bool TryScaleTrackSize(
        LONG value,
        UINT windowDpi,
        UINT monitorDpi,
        int32_t& scaled) noexcept
    {
        scaled = 0;
        if (value <= 0) return true;
        if (!windowDpi || !monitorDpi || windowDpi > INT32_MAX || monitorDpi > INT32_MAX)
        {
            return false;
        }
        const int result = ::MulDiv(
            static_cast<int>(value),
            static_cast<int>(monitorDpi),
            static_cast<int>(windowDpi));
        if (result < 0) return false; // MulDiv reports overflow as -1.
        scaled = result;
        return true;
    }

    bool TryComputeNativeRequest(
        HWND hwnd,
        const Snapshot& source,
        PlacementReason reason,
        const NativeApplyOptions& options,
        PlacementSource sourceKind,
        bool cascadePermitted,
        const PeerAnchorSource& peerAnchor,
        NativeRequest& request)
    {
        if (!::IsWindow(hwnd)) return false;
        PhysicalCoordinateScope coordinates;
        if (!coordinates.IsValid()) return false;

        const bool allowSizing = (::GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_THICKFRAME) != 0;
        PlacementEx engine{};
        if (!TryCreateEnginePlacement(source, allowSizing, engine)) return false;

        MonitorData monitor{};
        if (reason == PlacementReason::Launch)
        {
            TryGetLaunchMonitor(monitor);
        }
        if (!monitor.handle && !engine.FindClosestMonitor(&monitor)) return false;

        MonitorDescription target{};
        target.DeviceName.assign(reinterpret_cast<const char16_t*>(monitor.deviceName));
        target.WorkArea = {monitor.workArea.left, monitor.workArea.top,
            RECTWIDTH(monitor.workArea), RECTHEIGHT(monitor.workArea)};
        target.Dpi = static_cast<int32_t>(monitor.dpi);

        // Explicit placement permits a position adjustment only, so its own target
        // monitor is resolved first and the peer must already be on that monitor.
        std::optional<Rect> peerAnchorNormalRect;
        if (cascadePermitted && sourceKind == PlacementSource::Explicit && peerAnchor.TryGetAnchor)
        {
            Snapshot anchor{};
            if (peerAnchor.TryGetAnchor(peerAnchor.Context, target.DeviceName, anchor) &&
                IsValid(anchor))
            {
                peerAnchorNormalRect = anchor.NormalRect;
            }
        }

        // Ask the current HWND, including its presenter, instead of persisting source
        // constraints. Track sizes are expressed at the window's current DPI.
        const UINT dpi = ::GetDpiForWindow(hwnd);
        if (!dpi || !monitor.dpi || monitor.dpi > INT32_MAX) return false;
        MINMAXINFO limits{};
        ::SendMessageW(hwnd, WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&limits));
        if (!::IsWindow(hwnd)) return false; // A callback may have closed the window.

        PlacementRequest policy{};
        policy.Source = source;
        policy.SourceKind = sourceKind;
        policy.Reason = reason;
        policy.Cascade.Permitted = cascadePermitted;
        policy.Cascade.Offset = GetSystemMetrics(SM_CXSIZEFRAME) +
            GetSystemMetrics(SM_CXPADDEDBORDER);
        policy.Cascade.PeerAnchorNormalRect = peerAnchorNormalRect;
        policy.Constraints.AllowSizing = allowSizing;
        policy.Constraints.SnappingEnabled = allowSizing && IsSnappingEnabled();
        if (!TryScaleTrackSize(limits.ptMinTrackSize.x, dpi, monitor.dpi, policy.Constraints.MinWidth) ||
            !TryScaleTrackSize(limits.ptMinTrackSize.y, dpi, monitor.dpi, policy.Constraints.MinHeight) ||
            !TryScaleTrackSize(limits.ptMaxTrackSize.x, dpi, monitor.dpi, policy.Constraints.MaxWidth) ||
            !TryScaleTrackSize(limits.ptMaxTrackSize.y, dpi, monitor.dpi, policy.Constraints.MaxHeight))
        {
            return false;
        }

        // A single, already selected engine monitor bypasses synthetic selection rules.
        Topology topology{{target}};
        PlacementPlan plan{};
        auto nativeOptions = options;
        nativeOptions.AllowSizing = allowSizing;
        return ComputePlacement(policy, topology, plan) &&
            BuildNativeRequest(plan, nativeOptions, request);
    }

    bool TryComputeNativeRequest(
        HWND hwnd,
        const Snapshot& source,
        PlacementReason reason,
        const NativeApplyOptions& options,
        NativeRequest& request)
    {
        return TryComputeNativeRequest(
            hwnd,
            source,
            reason,
            options,
            PlacementSource::Saved,
            false,
            {},
            request);
    }

    ApplyOutcome TryApplyNativeRequest(
        HWND hwnd,
        const NativeRequest& request,
        WindowPlacementCapture& capture,
        bool forceLegacyBackend,
        PlacementFailure* failure)
    {
        // The engine's bool-only contract supplies no normalized HRESULT. Do not
        // sample ambient last-error state or invent an error for these failures.
        if (failure) *failure = {PlacementFailureCategory::Apply, S_OK};
        PlacementEx placement{};
        if (!::IsWindow(hwnd) || !TryCreateEnginePlacement(request, placement))
        {
            return ApplyOutcome::FailedBeforeStart;
        }
        if (request.Flags.KeepHidden && ::IsWindowVisible(hwnd))
        {
            return ApplyOutcome::FailedBeforeStart;
        }
        PhysicalCoordinateScope coordinates;
        if (!coordinates.IsValid()) return ApplyOutcome::FailedBeforeStart;

        const bool requiresWindowAction = request.Flags.KeepHidden ||
            request.Flags.NoActivate || request.Flags.UseVirtualDesktopId;
        if (requiresWindowAction && (forceLegacyBackend || !IsApplyWindowActionSupported()))
        {
            return ApplyOutcome::FailedBeforeStart;
        }
        if (forceLegacyBackend) placement.flags |= PlacementFlags::NoApplyWindowAction;

        // The imported legacy path ignores NoActivate and may briefly show a hidden
        // window. Do not use it for restricted operations, even when Window Action fails.
        const bool applied = PlacementEx::SetPlacement(hwnd, &placement, !requiresWindowAction);
        // Capture the HWND, not SetPlacement's mutable working value. The native call
        // can change geometry before failing, so refresh on either outcome.
        const bool captured = capture.TryCapture(hwnd, PresenterKind::Overlapped,
            applied ? &request : nullptr);
        if (applied && failure)
        {
            *failure = {captured ? PlacementFailureCategory::None : PlacementFailureCategory::Capture, S_OK};
        }
        return applied && captured ? ApplyOutcome::Applied : ApplyOutcome::FailedAfterStart;
    }

    bool TryApplyPlacement(HWND hwnd, const PlacementPass& pass, WindowPlacementCapture& capture,
        long (*load)(const char16_t*, size_t, LoadResult&),
        const PeerAnchorSource& peerAnchor)
    {
        const bool loadsSavedPlacement = !pass.Request.Placement &&
            pass.AutomaticPersistenceOptIn && !pass.PlacementId.empty();
        PlacementFailureScope diagnostic(
            loadsSavedPlacement ? PlacementOperation::Load : PlacementOperation::Apply);
        if (!::IsWindow(hwnd) || !IsValidInitialRequest(pass.Request, pass.KeepHidden))
        {
            diagnostic.Record(PlacementFailureCategory::Apply);
            return false;
        }

        std::optional<Snapshot> source = pass.Request.Placement;
        PlacementSource sourceKind = pass.PeerPlacement ?
            PlacementSource::Peer :
            (source ? PlacementSource::Explicit : PlacementSource::Saved);
        const auto reason = static_cast<PlacementReason>(pass.Request.Reason);
        const auto cascadeBehavior = static_cast<CascadeBehavior>(pass.Request.CascadeBehavior);
        const bool hasId = !pass.PlacementId.empty();
        const bool cascadePermitted = IsCascadePermitted(
            cascadeBehavior,
            reason,
            hasId,
            pass.AutomaticPersistenceOptIn,
            source.has_value());

        if (loadsSavedPlacement)
        {
            LoadResult loaded{};
            const auto hr = load(pass.PlacementId.data(), pass.PlacementId.size(), loaded);
            if (SUCCEEDED(hr) && loaded.Status == LoadStatus::Loaded)
            {
                source = loaded.Placement.Placement;
            }
            else if (FAILED(hr) || loaded.Status != LoadStatus::Missing)
            {
                diagnostic.Record(loaded.Category, FAILED(hr) ? hr : loaded.Error);
            }
        }

        const bool hasSelectedSource = source.has_value();
        if (!source)
        {
            // Use the current window placement as the fallback source when there is
            // no peer or saved value. This also lets hidden preparation apply the
            // normal fallback for reasons other than Launch.
            if (!capture.TryCapture(hwnd, PresenterKind::Overlapped))
            {
                diagnostic.Record(PlacementFailureCategory::Capture);
                return false;
            }
            const auto* fallback = capture.TryGetPlacement();
            if (!fallback)
            {
                diagnostic.Record(PlacementFailureCategory::Capture);
                return false;
            }
            source = *fallback;
        }

        NativeApplyOptions options{};
        options.KeepHidden = pass.KeepHidden;
        options.NoActivate = pass.Request.DoNotActivate || reason == PlacementReason::ApplicationRestart;
        NativeRequest request{};
        if (!TryComputeNativeRequest(
                hwnd,
                *source,
                reason,
                options,
                sourceKind,
                cascadePermitted,
                peerAnchor,
                request))
        {
            diagnostic.Record(PlacementFailureCategory::Apply);
            return false;
        }
        PlacementFailure failure;
        const auto outcome = TryApplyNativeRequest(hwnd, request, capture, false, &failure);
        if (outcome != ApplyOutcome::Applied) diagnostic.Record(failure.Category, failure.Error);
        // There is deliberately no second source lookup, including after partial failure.
        return hasSelectedSource && outcome == ApplyOutcome::Applied;
    }
}
