// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowPlacementCapture.h"

// Use the adapter's engine configuration so PlacementEx has one layout everywhere.
#include "WindowPlacementAdapter.h"
#include "WindowPlacementPhysicalCoordinates.h"

#include <utility>

namespace DirectUI::WindowPlacementPersistence
{
    namespace
    {
        // Engine flags that never travel with a captured value. FullScreen is handled
        // before translation. The virtual desktop id is skipped during capture and comes
        // from TrySetVirtualDesktopId instead. GetPlacement never sets the other two.
        constexpr PlacementFlags NonRequestFlags =
            PlacementFlags::FullScreen |
            PlacementFlags::VirtualDesktopId |
            PlacementFlags::AllowPartiallyOffScreen |
            PlacementFlags::NoApplyWindowAction;

        // Flags describing a restore target. Only a minimized value can carry them.
        constexpr PlacementFlags RestoreFlags =
            PlacementFlags::RestoreToMaximized |
            PlacementFlags::RestoreToArranged;

        bool IsEmptyGuid(const GUID& id) noexcept
        {
            if (id.Data1 || id.Data2 || id.Data3) return false;
            for (auto value : id.Data4)
            {
                if (value) return false;
            }
            return true;
        }

        bool WasSnapped(State state) noexcept
        {
            return state == State::Snapped || state == State::MinimizedFromSnapped;
        }

        // GetWindowPlacement reports show-state aliases, and it can report an arranged or
        // restore flag that the matching request form does not allow. The request
        // vocabulary uses the canonical normal/maximize/minimize commands, and SW_HIDE only
        // validates as "keep hidden, do not change the show state". Combinations a request
        // cannot carry are dropped here; reconciliation recovers the state from the
        // previously cached snapshot instead.
        bool TryCanonicalizeShowCommand(PlacementEx& placement) noexcept
        {
            if (PlacementEx::IsMinimizeShowCmd(placement.showCmd))
            {
                placement.showCmd = SW_MINIMIZE;
                placement.flags &= ~PlacementFlags::Arranged;
                if (placement.HasFlag(PlacementFlags::RestoreToMaximized))
                {
                    placement.flags &= ~PlacementFlags::RestoreToArranged;
                }
            }
            else if (placement.showCmd == SW_SHOWMAXIMIZED)
            {
                placement.showCmd = SW_MAXIMIZE;
                placement.flags &= ~(PlacementFlags::Arranged | RestoreFlags);
            }
            else if (PlacementEx::IsRestoreShowCmd(placement.showCmd) ||
                placement.showCmd == SW_SHOW ||
                placement.showCmd == SW_SHOWNOACTIVATE ||
                placement.showCmd == SW_SHOWNA)
            {
                placement.showCmd = SW_NORMAL;
                placement.flags &= ~RestoreFlags;
            }
            else if (placement.showCmd == SW_HIDE)
            {
                placement.flags &= ~(PlacementFlags::Arranged | RestoreFlags);
                placement.flags |= PlacementFlags::KeepHidden | PlacementFlags::NoActivate;
            }
            else
            {
                return false;
            }
            return true;
        }
    }

    bool WindowPlacementCapture::TryReconcile(
        const NativeRequest& captured,
        bool isVisible,
        const Snapshot* previous,
        Snapshot& result)
    {
        Snapshot reconciled{};
        reconciled.NormalRect = captured.NormalRect;
        reconciled.WorkArea = captured.WorkArea;
        reconciled.Dpi = captured.Dpi;
        reconciled.DisplayDeviceName = captured.DeviceName;

        const State previousState = previous ? previous->PlacementState : State::Normal;
        const std::optional<Rect> previousSnapRect = previous ? previous->SnapRect : std::nullopt;

        switch (captured.ShowCommand)
        {
        case NativeShowCommand::Maximize:
            reconciled.PlacementState = State::Maximized;
            break;

        case NativeShowCommand::Minimize:
            // The engine reports restore-to-maximized. It cannot reconstruct
            // restore-to-snapped from an already minimized window, so that history comes
            // from the previous snapshot.
            if (captured.Flags.RestoreToMaximized)
            {
                reconciled.PlacementState = State::MinimizedFromMaximized;
            }
            else if (captured.Flags.RestoreToArranged)
            {
                reconciled.PlacementState = State::MinimizedFromSnapped;
                reconciled.SnapRect = captured.ArrangeRect;
            }
            else if (WasSnapped(previousState))
            {
                reconciled.PlacementState = State::MinimizedFromSnapped;
                reconciled.SnapRect = previousSnapRect;
            }
            else
            {
                reconciled.PlacementState = State::Minimized;
            }
            break;

        case NativeShowCommand::NoChange:
            // SW_HIDE: the window has no show state of its own, so keep the last
            // meaningful one. Its geometry is still the geometry captured just now.
            reconciled.PlacementState = previousState;
            if (WasSnapped(previousState)) reconciled.SnapRect = previousSnapRect;
            break;

        case NativeShowCommand::Normal:
        default:
            if (captured.Flags.Arranged)
            {
                reconciled.PlacementState = State::Snapped;
                reconciled.SnapRect = captured.ArrangeRect;
            }
            else if (!isVisible && previousState == State::Snapped)
            {
                // IsWindowArranged goes false as soon as the window is hidden. Hiding alone
                // does not replace meaningful state.
                reconciled.PlacementState = State::Snapped;
                reconciled.SnapRect = previousSnapRect;
            }
            else
            {
                reconciled.PlacementState = State::Normal;
            }
            break;
        }

        // A snapped state without an arrange rectangle is not representable. Report the
        // state that is left rather than failing capture over missing history.
        if (WasSnapped(reconciled.PlacementState) && !reconciled.SnapRect.has_value())
        {
            reconciled.PlacementState = reconciled.PlacementState == State::Snapped ?
                State::Normal : State::Minimized;
        }

        if (!IsValid(reconciled)) return false;

        result = std::move(reconciled);
        return true;
    }

    bool WindowPlacementCapture::TryCapture(HWND hwnd, PresenterKind presenter,
        const NativeRequest* applied) noexcept
    {
        if (m_isDetached && hwnd == m_window) return false;
        if (hwnd == nullptr || !::IsWindow(hwnd)) return false;

        const bool isNewWindow = hwnd != m_window;
        if (isNewWindow && m_nextLifetimeToken == 0) return false;

        // A non-overlapped window has no overlapped geometry to capture. Keeping the last
        // valid overlapped snapshot is the required behavior, not a silent failure.
        if (presenter != PresenterKind::Overlapped) return false;

        PhysicalCoordinateScope coordinates;
        if (!coordinates.IsValid()) return false;

        PlacementEx captured{};
        if (!PlacementEx::GetPlacement(hwnd, &captured, CaptureFlags::SkipVirtualDesktopId))
        {
            return false;
        }
        if (captured.HasFlag(PlacementFlags::FullScreen)) return false;

        captured.flags &= ~NonRequestFlags;
        if (!TryCanonicalizeShowCommand(captured)) return false;

        // Build separately so an ordinary capture failure does not discard the cache or
        // partially rebind a window. Allocation failure terminates at the noexcept boundary,
        // consistent with this library's exception-disabled build.
        NativeRequest value{};
        if (!TryReadEnginePlacement(captured, value)) return false;
        if (applied && value.ShowCommand == NativeShowCommand::Minimize &&
            !value.Flags.RestoreToMaximized && applied->Flags.RestoreToArranged)
        {
            value.Flags.RestoreToArranged = true;
            value.ArrangeRect = applied->ArrangeRect;
        }
        Snapshot reconciled{};
        if (!TryReconcile(value, ::IsWindowVisible(hwnd) != FALSE,
            isNewWindow || applied ? nullptr : TryGetPlacement(), reconciled))
        {
            return false;
        }

        // Publish only after all fallible work succeeds. A new window never inherits
        // history or desktop identity, and only successful rebinding consumes a token.
        reconciled.VirtualDesktopId = isNewWindow ? std::nullopt : m_virtualDesktopId;
        static_assert(noexcept(m_placement = std::move(reconciled)),
            "Publishing a capture must not throw.");
        m_placement = std::move(reconciled);
        if (isNewWindow)
        {
            m_window = hwnd;
            m_isDetached = false;
            m_virtualDesktopId.reset();
            m_lifetimeToken = m_nextLifetimeToken++;
        }
        return true;
    }

    const Snapshot* WindowPlacementCapture::TryGetPlacement() const noexcept
    {
        return m_placement.has_value() ? &m_placement.value() : nullptr;
    }

    void WindowPlacementCapture::Detach() noexcept
    {
        m_isDetached = true;
        m_lifetimeToken = 0;
    }

    bool WindowPlacementCapture::TrySetVirtualDesktopId(uint64_t lifetimeToken, const GUID& id)
    {
        if (lifetimeToken == 0 || lifetimeToken != m_lifetimeToken || IsEmptyGuid(id))
        {
            return false;
        }

        m_virtualDesktopId = id;
        if (m_placement.has_value()) m_placement->VirtualDesktopId = id;
        return true;
    }

    bool TryQueryVirtualDesktopId(HWND hwnd, GUID& id) noexcept
    {
        if (hwnd == nullptr) return false;

        GUID queried{};
        if (!GetVirtualDesktopId(hwnd, &queried)) return false;
        if (IsEmptyGuid(queried)) return false;

        id = queried;
        return true;
    }
}
