// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowPlacementPolicy.h"

// Do not include the PlacementEx headers here. They are configured by
// WindowPlacementAdapter.h, and including them with a different configuration gives
// PlacementEx a different layout in this translation unit. Only HWND is needed.
#include <windows.h>

namespace DirectUI::WindowPlacementPersistence
{
    // The presenter WinUI is using. Only an overlapped window produces new capture
    // geometry; a full-screen or compact-overlay window keeps its last overlapped snapshot.
    enum class PresenterKind
    {
        Overlapped,
        NonOverlapped
    };

    // Legacy windows are overlapped and must not query the new windowing APIs.
    template <typename SupportsSizing>
    PresenterKind ResolvePresenterKind(bool newWindowingApisEnabled, SupportsSizing supportsSizing)
    {
        if (!newWindowingApisEnabled) return PresenterKind::Overlapped;
        return supportsSizing() ? PresenterKind::Overlapped : PresenterKind::NonOverlapped;
    }

    // Per-window placement cache.
    //
    // PlacementEx::GetPlacement owns geometry, monitor data, DPI, arranged state, and
    // restore-to-maximized. This class owns only what the engine cannot report from an HWND:
    // the last meaningful show state, snapped restore history, the last valid overlapped
    // snapshot, and the safely cached virtual desktop id.
    class WindowPlacementCapture
    {
    public:
        // Captures from the window and updates the cache. Capture never queries the virtual
        // desktop, so it is safe from a window message handler. A failed capture leaves the
        // cached snapshot unchanged, which is what keeps a full-screen or compact-overlay
        // window on its last valid overlapped placement. Successfully capturing a different
        // window resets tracked state. Allocation failure terminates instead of unwinding
        // through a window callback, consistent with the exception-disabled product build.
        // A successful native application can supply restore-to-snapped history that
        // GetPlacement cannot observe while minimized. Other fields still come from HWND.
        bool TryCapture(HWND hwnd, PresenterKind presenter,
            const NativeRequest* applied = nullptr) noexcept;

        // The placement that public capture and saving should use, or null when nothing
        // valid has been captured.
        const Snapshot* TryGetPlacement() const noexcept;

        // Stops capturing. The cached snapshot stays available for a save during teardown,
        // and outstanding lifetime tokens stop matching. The retired HWND cannot be
        // captured again while detached; a different HWND can start a new binding.
        void Detach() noexcept;

        // Identifies the currently bound window. An asynchronous virtual-desktop query
        // passes the token it started with, so a late result cannot attach to a closed or
        // recycled HWND. Zero means no window is bound.
        uint64_t LifetimeToken() const noexcept { return m_lifetimeToken; }

        // Overlays a safely obtained non-empty desktop id on this and later captures.
        // Fails without changing the cache for a stale token or an empty id.
        bool TrySetVirtualDesktopId(uint64_t lifetimeToken, const GUID& id);

        // Reconciles one engine capture with the previously cached snapshot. TryCapture
        // calls this after reading the engine value. It is separate from the window so the
        // state rules, including snapped states that cannot be staged reliably in a test,
        // are testable without one. Output is replaced only on success.
        static bool TryReconcile(
            const NativeRequest& captured,
            bool isVisible,
            const Snapshot* previous,
            Snapshot& result);

    private:
        std::optional<Snapshot> m_placement;
        std::optional<GUID> m_virtualDesktopId;
        HWND m_window{}; // Retained while detached to reject recapture of the retired HWND.
        bool m_isDetached{};
        uint64_t m_lifetimeToken{};
        // Monotonic, so a token is never reused after a window is detached or replaced.
        // Zero means the token space is exhausted; no further bindings can be made.
        uint64_t m_nextLifetimeToken{1};
    };

    // Queries the shell for the window's virtual desktop id. This is a cross-apartment COM
    // call. It must never run inline from a display call, from WM_ENDSESSION, or from any
    // input-synchronous message, where it can fail with RPC_E_CANTCALLOUT_ININPUTSYNCCALL or
    // block. Returns false whenever the shell cannot answer, including the common case of a
    // window the taskbar does not know about yet.
    bool TryQueryVirtualDesktopId(HWND hwnd, GUID& id) noexcept;

    // True when a virtual-desktop refresh is worth scheduling. A closed window, or one that
    // is not being displayed, has nothing the shell can answer for. The lifetime token is
    // deliberately not part of this decision: the cache may not be bound yet at display
    // time, and the token is what correlates the *result*, not the request.
    inline bool ShouldScheduleVirtualDesktopRefresh(bool isClosed, bool displayed) noexcept
    {
        return !isClosed && displayed;
    }

    // True when a scheduled refresh is still worth running once the posted message arrives.
    // The id is only ever read back out of a saved record, and only an enrolled window can
    // save, so a window that never enrolled must not pay for a shell call on every display.
    // Enrollment is recorded after the first Display() returns, so this is a run-time check
    // rather than a second schedule-time one.
    inline bool ShouldRunVirtualDesktopRefresh(bool isClosed, bool hasEnrollment) noexcept
    {
        return !isClosed && hasEnrollment;
    }

    // Runs one virtual-desktop refresh against the cache. The token is taken before the
    // query, so a result that arrives after the window closed or was replaced is dropped
    // instead of being attached to the wrong window. A failed or unavailable query leaves
    // the cache, and therefore the next save, untouched.
    template <typename Query>
    bool TryRefreshVirtualDesktopId(WindowPlacementCapture& cache, HWND hwnd, Query query)
    {
        const uint64_t lifetimeToken = cache.LifetimeToken();
        if (lifetimeToken == 0) return false;

        GUID id{};
        if (!query(hwnd, id)) return false;

        return cache.TrySetVirtualDesktopId(lifetimeToken, id);
    }
}
