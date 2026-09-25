// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include "WindowPlacementCapture.h"
// For a real full-screen window and the engine's own view of it.
#include "WindowPlacementAdapter.h"

using namespace DirectUI::WindowPlacementPersistence;

namespace
{
    // Capture reports physical pixels, so the test verifies geometry in the same space.
    class PhysicalCoordinateScope
    {
    public:
        PhysicalCoordinateScope() noexcept
        {
            m_previous = ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            if (m_previous == nullptr)
            {
                m_previous = ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
            }
        }

        ~PhysicalCoordinateScope() noexcept
        {
            if (m_previous != nullptr) ::SetThreadDpiAwarenessContext(m_previous);
        }

        PhysicalCoordinateScope(const PhysicalCoordinateScope&) = delete;
        PhysicalCoordinateScope& operator=(const PhysicalCoordinateScope&) = delete;

    private:
        DPI_AWARENESS_CONTEXT m_previous{};
    };

    // A real top-level window. Capture runs against the window manager, so these tests use
    // one instead of a double. The window starts hidden, which is how a WinUI window starts.
    class TestWindow
    {
    public:
        explicit TestWindow(HWND parent = nullptr)
        {
            m_hwnd = ::CreateWindowExW(
                0, L"STATIC", L"WindowPlacementCaptureTests",
                parent ? WS_CHILD : WS_OVERLAPPEDWINDOW,
                120, 140, 640, 480,
                parent, nullptr, ::GetModuleHandleW(nullptr), nullptr);
        }

        ~TestWindow() { Destroy(); }

        TestWindow(const TestWindow&) = delete;
        TestWindow& operator=(const TestWindow&) = delete;

        HWND Get() const noexcept { return m_hwnd; }

        void Destroy() noexcept
        {
            if (m_hwnd != nullptr)
            {
                ::DestroyWindow(m_hwnd);
                m_hwnd = nullptr;
            }
            Pump();
        }

        void Show(int command) noexcept
        {
            ::ShowWindow(m_hwnd, command);
            Pump();
        }

        void Move(int dx, int dy) noexcept
        {
            RECT rect{};
            VERIFY_IS_TRUE(::GetWindowRect(m_hwnd, &rect) != FALSE);
            VERIFY_IS_TRUE(::MoveWindow(
                m_hwnd, rect.left + dx, rect.top + dy,
                rect.right - rect.left, rect.bottom - rect.top, FALSE) != FALSE);
            Pump();
        }

        static void Pump() noexcept
        {
            MSG message{};
            while (::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
            {
                ::TranslateMessage(&message);
                ::DispatchMessageW(&message);
            }
        }

    private:
        HWND m_hwnd{};
    };

    // An engine-shaped capture value, as TryReadEnginePlacement would produce it.
    NativeRequest CapturedValue()
    {
        NativeRequest value{};
        value.NormalRect = {100, 80, 800, 600};
        value.WorkArea = {0, 0, 1920, 1040};
        value.Dpi = 96;
        value.DeviceName = u"\\\\.\\DISPLAY1";
        value.ShowCommand = NativeShowCommand::Normal;
        return value;
    }

    Snapshot SnappedSnapshot()
    {
        Snapshot previous{};
        previous.NormalRect = {100, 80, 800, 600};
        previous.WorkArea = {0, 0, 1920, 1040};
        previous.Dpi = 96;
        previous.DisplayDeviceName = u"\\\\.\\DISPLAY1";
        previous.PlacementState = State::Snapped;
        previous.SnapRect = Rect{0, 0, 960, 1040};
        return previous;
    }

    GUID TestDesktopId()
    {
        return {0x11112222, 0x3333, 0x4444, {0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC}};
    }

    void VerifySameRect(const Rect& expected, const Rect& actual)
    {
        VERIFY_ARE_EQUAL(expected.X, actual.X);
        VERIFY_ARE_EQUAL(expected.Y, actual.Y);
        VERIFY_ARE_EQUAL(expected.Width, actual.Width);
        VERIFY_ARE_EQUAL(expected.Height, actual.Height);
    }

    void VerifySameSnapshot(const Snapshot& expected, const Snapshot& actual)
    {
        VerifySameRect(expected.NormalRect, actual.NormalRect);
        VerifySameRect(expected.WorkArea, actual.WorkArea);
        VERIFY_ARE_EQUAL(expected.Dpi, actual.Dpi);
        VERIFY_ARE_EQUAL(expected.PlacementState, actual.PlacementState);
        VERIFY_IS_TRUE(expected.DisplayDeviceName == actual.DisplayDeviceName);
        VERIFY_ARE_EQUAL(expected.SnapRect.has_value(), actual.SnapRect.has_value());
        if (expected.SnapRect && actual.SnapRect)
        {
            VerifySameRect(*expected.SnapRect, *actual.SnapRect);
        }
        VERIFY_ARE_EQUAL(expected.VirtualDesktopId.has_value(), actual.VirtualDesktopId.has_value());
        if (expected.VirtualDesktopId && actual.VirtualDesktopId)
        {
            VERIFY_IS_TRUE(::IsEqualGUID(*expected.VirtualDesktopId, *actual.VirtualDesktopId) != FALSE);
        }
    }
}

class WindowPlacementCaptureTests
{
public:
    TEST_CLASS(WindowPlacementCaptureTests);

    TEST_METHOD(ResolvesPresenterKindWithoutQueryingWhenFeatureIsOff)
    {
        int queryCount = 0;
        const auto presenter = ResolvePresenterKind(false, [&queryCount]
        {
            ++queryCount;
            return false;
        });

        VERIFY_ARE_EQUAL(PresenterKind::Overlapped, presenter);
        VERIFY_ARE_EQUAL(0, queryCount);
    }

    TEST_METHOD(ResolvesPresenterKindFromSizingSupportWhenFeatureIsOn)
    {
        for (const bool supportsSizing : {true, false})
        {
            int queryCount = 0;
            const auto presenter = ResolvePresenterKind(true, [&queryCount, supportsSizing]
            {
                ++queryCount;
                return supportsSizing;
            });

            VERIFY_ARE_EQUAL(
                supportsSizing ? PresenterKind::Overlapped : PresenterKind::NonOverlapped,
                presenter);
            VERIFY_ARE_EQUAL(1, queryCount);
        }
    }

    // --- Reconciliation: the WinUI state rules on top of the engine value. ---

    TEST_METHOD(ReconcilesEngineShowCommands)
    {
        Snapshot result{};

        auto value = CapturedValue();
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, nullptr, result));
        VERIFY_ARE_EQUAL(State::Normal, result.PlacementState);
        VerifySameRect(value.NormalRect, result.NormalRect);
        VerifySameRect(value.WorkArea, result.WorkArea);
        VERIFY_ARE_EQUAL(96, result.Dpi);
        VERIFY_IS_TRUE(result.DisplayDeviceName == value.DeviceName);
        VERIFY_IS_FALSE(result.SnapRect.has_value());

        value.ShowCommand = NativeShowCommand::Maximize;
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, nullptr, result));
        VERIFY_ARE_EQUAL(State::Maximized, result.PlacementState);

        value.ShowCommand = NativeShowCommand::Minimize;
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, nullptr, result));
        VERIFY_ARE_EQUAL(State::Minimized, result.PlacementState);

        value.ShowCommand = NativeShowCommand::Normal;
        value.Flags.Arranged = true;
        value.ArrangeRect = Rect{960, 0, 960, 1040};
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, nullptr, result));
        VERIFY_ARE_EQUAL(State::Snapped, result.PlacementState);
        VERIFY_IS_TRUE(result.SnapRect.has_value());
        VerifySameRect(*value.ArrangeRect, *result.SnapRect);
    }

    TEST_METHOD(ReconcilesEngineRestoreFlags)
    {
        Snapshot result{};

        auto value = CapturedValue();
        value.ShowCommand = NativeShowCommand::Minimize;
        value.Flags.RestoreToMaximized = true;
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, nullptr, result));
        VERIFY_ARE_EQUAL(State::MinimizedFromMaximized, result.PlacementState);

        // The engine flag wins over snapped history.
        const auto previous = SnappedSnapshot();
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, &previous, result));
        VERIFY_ARE_EQUAL(State::MinimizedFromMaximized, result.PlacementState);

        value.Flags.RestoreToMaximized = false;
        value.Flags.RestoreToArranged = true;
        value.ArrangeRect = Rect{960, 0, 960, 1040};
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, nullptr, result));
        VERIFY_ARE_EQUAL(State::MinimizedFromSnapped, result.PlacementState);
        VerifySameRect(*value.ArrangeRect, *result.SnapRect);
    }

    TEST_METHOD(MaximizedRestoreTakesPrecedenceOverArrangedRestore)
    {
        // Defensive reconciliation coverage: GetPlacement does not currently emit
        // RestoreToArranged. A maximized restore must not retain a snap rectangle.
        auto value = CapturedValue();
        value.ShowCommand = NativeShowCommand::Minimize;
        value.Flags.RestoreToMaximized = true;
        value.Flags.RestoreToArranged = true;
        value.ArrangeRect = Rect{960, 0, 960, 1040};
        const auto previous = SnappedSnapshot();
        Snapshot result = previous;

        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, &previous, result));
        VERIFY_ARE_EQUAL(State::MinimizedFromMaximized, result.PlacementState);
        VERIFY_IS_FALSE(result.SnapRect.has_value());
    }

    TEST_METHOD(KeepsSnappedRestoreHistoryWhenMinimized)
    {
        // The engine cannot report restore-to-snapped from an already minimized window.
        // Minimizing sends both WM_MOVE and WM_SIZE, so the history has to survive repeated
        // captures, not just the first one.
        const auto snapped = SnappedSnapshot();
        auto value = CapturedValue();
        value.ShowCommand = NativeShowCommand::Minimize;

        Snapshot first{};
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, &snapped, first));
        VERIFY_ARE_EQUAL(State::MinimizedFromSnapped, first.PlacementState);
        VerifySameRect(*snapped.SnapRect, *first.SnapRect);

        Snapshot second{};
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, &first, second));
        VERIFY_ARE_EQUAL(State::MinimizedFromSnapped, second.PlacementState);
        VerifySameRect(*snapped.SnapRect, *second.SnapRect);
    }

    TEST_METHOD(KeepsMeaningfulStateWhileHidden)
    {
        // IsWindowArranged goes false as soon as the window is hidden, so snapped state
        // survives hiding only through tracked state. Geometry still comes from the new
        // capture, because a hidden window can move.
        const auto snapped = SnappedSnapshot();
        auto value = CapturedValue();
        value.NormalRect = {300, 200, 800, 600};

        Snapshot hidden{};
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, false, &snapped, hidden));
        VERIFY_ARE_EQUAL(State::Snapped, hidden.PlacementState);
        VerifySameRect(*snapped.SnapRect, *hidden.SnapRect);
        VerifySameRect(value.NormalRect, hidden.NormalRect);

        // A visible window that reports no arranged state really is normal again.
        Snapshot restored{};
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, &snapped, restored));
        VERIFY_ARE_EQUAL(State::Normal, restored.PlacementState);
        VERIFY_IS_FALSE(restored.SnapRect.has_value());
    }

    TEST_METHOD(KeepsMeaningfulStateWithoutShowCommand)
    {
        // SW_HIDE carries no show state, so the last meaningful one is kept.
        auto value = CapturedValue();
        value.ShowCommand = NativeShowCommand::NoChange;
        value.Flags.KeepHidden = true;
        value.Flags.NoActivate = true;
        value.NormalRect = {300, 200, 800, 600};

        Snapshot previous{};
        previous.NormalRect = {100, 80, 800, 600};
        previous.WorkArea = {0, 0, 1920, 1040};
        previous.Dpi = 96;
        previous.DisplayDeviceName = u"\\\\.\\DISPLAY1";
        previous.PlacementState = State::Maximized;

        Snapshot result{};
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, false, &previous, result));
        VERIFY_ARE_EQUAL(State::Maximized, result.PlacementState);
        VerifySameRect(value.NormalRect, result.NormalRect);

        // With no tracked state at all, initial valid geometry is Normal.
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, false, nullptr, result));
        VERIFY_ARE_EQUAL(State::Normal, result.PlacementState);
    }

    TEST_METHOD(DowngradesSnappedStateWithoutSnapRect)
    {
        // A snapped state needs an arrange rectangle. Report the state that is left rather
        // than failing capture over missing history.
        auto previous = SnappedSnapshot();
        previous.SnapRect.reset();

        auto value = CapturedValue();
        value.ShowCommand = NativeShowCommand::NoChange;
        value.Flags.KeepHidden = true;
        value.Flags.NoActivate = true;

        Snapshot result{};
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, false, &previous, result));
        VERIFY_ARE_EQUAL(State::Normal, result.PlacementState);

        value.ShowCommand = NativeShowCommand::Minimize;
        value.Flags = {};
        VERIFY_IS_TRUE(WindowPlacementCapture::TryReconcile(value, true, &previous, result));
        VERIFY_ARE_EQUAL(State::Minimized, result.PlacementState);
    }

    TEST_METHOD(RejectsUnusableCaptureValue)
    {
        // A normal rectangle off the reported work area cannot make a valid snapshot.
        auto value = CapturedValue();
        value.NormalRect = {5000, 5000, 800, 600};

        Snapshot result = SnappedSnapshot();
        result.VirtualDesktopId = TestDesktopId();
        const auto before = result;
        VERIFY_IS_FALSE(WindowPlacementCapture::TryReconcile(value, true, nullptr, result));
        VerifySameSnapshot(before, result);
    }

    // --- Capture from a live window. ---

    TEST_METHOD(RejectsWindowsThatCannotBeCaptured)
    {
        WindowPlacementCapture cache;
        VERIFY_IS_FALSE(cache.TryCapture(nullptr, PresenterKind::Overlapped));
        VERIFY_IS_FALSE(cache.TryCapture(
            reinterpret_cast<HWND>(static_cast<intptr_t>(1)), PresenterKind::Overlapped));
        VERIFY_IS_NULL(cache.TryGetPlacement());
        VERIFY_ARE_EQUAL(0u, cache.LifetimeToken());
    }

    TEST_METHOD(CapturesShownWindow)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        VERIFY_IS_NOT_NULL(window.Get());
        window.Show(SW_SHOWNORMAL);

        WindowPlacementCapture cache;
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));

        const auto* placement = cache.TryGetPlacement();
        VERIFY_IS_NOT_NULL(placement);
        VERIFY_ARE_EQUAL(State::Normal, placement->PlacementState);
        VERIFY_IS_TRUE(IsValid(*placement));
        VERIFY_IS_TRUE(placement->Dpi >= 96);
        VERIFY_IS_FALSE(placement->DisplayDeviceName.empty());
        VERIFY_IS_FALSE(placement->VirtualDesktopId.has_value());

        RECT rect{};
        VERIFY_IS_TRUE(::GetWindowRect(window.Get(), &rect) != FALSE);
        VERIFY_ARE_EQUAL(rect.right - rect.left, placement->NormalRect.Width);
        VERIFY_ARE_EQUAL(rect.bottom - rect.top, placement->NormalRect.Height);
    }

    TEST_METHOD(CapturesMovesWhileHidden)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        VERIFY_IS_NOT_NULL(window.Get());

        WindowPlacementCapture cache;
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        const auto before = *cache.TryGetPlacement();

        window.Move(40, 30);
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        const auto after = *cache.TryGetPlacement();

        VERIFY_ARE_EQUAL(before.NormalRect.X + 40, after.NormalRect.X);
        VERIFY_ARE_EQUAL(before.NormalRect.Y + 30, after.NormalRect.Y);
        VERIFY_ARE_EQUAL(before.NormalRect.Width, after.NormalRect.Width);
        VERIFY_ARE_EQUAL(State::Normal, after.PlacementState);
    }

    TEST_METHOD(CapturesNormalShowAliases)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        VERIFY_IS_NOT_NULL(window.Get());
        WindowPlacementCapture cache;

        // Exercise the native show paths. GetWindowPlacement may itself canonicalize
        // these aliases before capture sees them.
        for (const int command : {SW_SHOWNOACTIVATE, SW_SHOWNA, SW_SHOW, SW_RESTORE})
        {
            window.Show(SW_HIDE);
            window.Show(command);
            VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
            VERIFY_ARE_EQUAL(State::Normal, cache.TryGetPlacement()->PlacementState);
            VERIFY_IS_TRUE(IsValid(*cache.TryGetPlacement()));
        }
    }

    TEST_METHOD(CapturesMaximizedAndMinimizedTransitions)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        VERIFY_IS_NOT_NULL(window.Get());

        WindowPlacementCapture cache;
        window.Show(SW_SHOWMAXIMIZED);
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        VERIFY_ARE_EQUAL(State::Maximized, cache.TryGetPlacement()->PlacementState);
        const auto maximized = *cache.TryGetPlacement();

        // Minimizing a maximized window keeps its restore state, on this capture and the
        // next one. A minimized window reports (-32000, -32000), so the snapshot must still
        // describe its restore geometry and its own monitor.
        window.Show(SW_MINIMIZE);
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        VERIFY_ARE_EQUAL(State::MinimizedFromMaximized, cache.TryGetPlacement()->PlacementState);

        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        const auto minimized = *cache.TryGetPlacement();
        VERIFY_ARE_EQUAL(State::MinimizedFromMaximized, minimized.PlacementState);
        VERIFY_IS_TRUE(IsValid(minimized));
        VerifySameRect(maximized.NormalRect, minimized.NormalRect);
        VERIFY_IS_TRUE(maximized.DisplayDeviceName == minimized.DisplayDeviceName);

        // Restoring a minimized-from-maximized window returns it to maximized, and
        // restoring again returns it to normal.
        window.Show(SW_RESTORE);
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        VERIFY_ARE_EQUAL(State::Maximized, cache.TryGetPlacement()->PlacementState);

        window.Show(SW_RESTORE);
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        VERIFY_ARE_EQUAL(State::Normal, cache.TryGetPlacement()->PlacementState);
        VerifySameRect(maximized.NormalRect, cache.TryGetPlacement()->NormalRect);
    }

    TEST_METHOD(KeepsLastOverlappedSnapshot)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        VERIFY_IS_NOT_NULL(window.Get());
        window.Show(SW_SHOWNORMAL);

        WindowPlacementCapture cache;
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        const auto overlapped = *cache.TryGetPlacement();

        // A non-overlapped presenter has no overlapped geometry to capture, so the cache
        // keeps the complete snapshot, including its state and monitor data.
        window.Move(60, 60);
        VERIFY_IS_FALSE(cache.TryCapture(window.Get(), PresenterKind::NonOverlapped));
        VerifySameRect(overlapped.NormalRect, cache.TryGetPlacement()->NormalRect);
        VERIFY_ARE_EQUAL(overlapped.PlacementState, cache.TryGetPlacement()->PlacementState);

        // A destroyed window cannot be captured either, and the snapshot has to survive for
        // a save during teardown.
        const HWND destroyed = window.Get();
        window.Destroy();
        VERIFY_IS_FALSE(cache.TryCapture(destroyed, PresenterKind::Overlapped));
        VERIFY_IS_NOT_NULL(cache.TryGetPlacement());
        VerifySameRect(overlapped.NormalRect, cache.TryGetPlacement()->NormalRect);
    }

    TEST_METHOD(FullScreenWindowKeepsThePreFullScreenSnapshot)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        VERIFY_IS_NOT_NULL(window.Get());
        window.Show(SW_SHOWNORMAL);

        WindowPlacementCapture cache;
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        const auto overlapped = *cache.TryGetPlacement();
        const auto token = cache.LifetimeToken();
        VERIFY_ARE_EQUAL(State::Normal, overlapped.PlacementState);

        // An app that drops its caption and thick frame and fills the monitor is full
        // screen to the engine, but it still reports an overlapped presenter. So this
        // reaches the full-screen guard in TryCapture, not the presenter check that
        // the AppWindow FullScreen presenter takes.
        PlacementEx fullScreen{};
        VERIFY_IS_TRUE(fullScreen.EnterFullScreen(window.Get()));
        TestWindow::Pump();

        // The rest of the test only means anything if the window really got there.
        PlacementEx observed{};
        VERIFY_IS_TRUE(PlacementEx::GetPlacement(
            window.Get(), &observed, CaptureFlags::SkipVirtualDesktopId));
        VERIFY_IS_TRUE(observed.IsFullScreen());

        // Capture refuses the full-screen window and keeps the complete overlapped
        // snapshot, so a save during teardown still describes the restore position.
        VERIFY_IS_FALSE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        VERIFY_IS_NOT_NULL(cache.TryGetPlacement());
        VerifySameSnapshot(overlapped, *cache.TryGetPlacement());
        VERIFY_ARE_EQUAL(token, cache.LifetimeToken());

        // Leaving full screen restores the window, and capture works again.
        VERIFY_IS_TRUE(fullScreen.ExitFullScreen(window.Get()));
        TestWindow::Pump();
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        VERIFY_ARE_EQUAL(State::Normal, cache.TryGetPlacement()->PlacementState);
        VerifySameRect(overlapped.NormalRect, cache.TryGetPlacement()->NormalRect);
        VERIFY_ARE_EQUAL(token, cache.LifetimeToken());
    }

    TEST_METHOD(CorrelatesVirtualDesktopIdWithWindowLifetime)
    {
        PhysicalCoordinateScope coordinates;
        const auto id = TestDesktopId();
        WindowPlacementCapture cache;

        // No window bound yet, so there is nothing a late result could belong to.
        VERIFY_IS_FALSE(cache.TrySetVirtualDesktopId(cache.LifetimeToken(), id));

        TestWindow first;
        VERIFY_IS_NOT_NULL(first.Get());
        VERIFY_IS_TRUE(cache.TryCapture(first.Get(), PresenterKind::Overlapped));
        const auto token = cache.LifetimeToken();
        VERIFY_ARE_NOT_EQUAL(0u, token);

        VERIFY_IS_FALSE(cache.TrySetVirtualDesktopId(token, GUID{}));
        VERIFY_IS_FALSE(cache.TryGetPlacement()->VirtualDesktopId.has_value());

        VERIFY_IS_TRUE(cache.TrySetVirtualDesktopId(token, id));
        VERIFY_IS_TRUE(cache.TryGetPlacement()->VirtualDesktopId.has_value());

        // Capture never clears the safely cached id.
        first.Move(10, 10);
        VERIFY_IS_TRUE(cache.TryCapture(first.Get(), PresenterKind::Overlapped));
        VERIFY_IS_TRUE(cache.TryGetPlacement()->VirtualDesktopId.has_value());
        VERIFY_IS_TRUE(::IsEqualGUID(id, *cache.TryGetPlacement()->VirtualDesktopId) != FALSE);

        // A closed window stops matching, so a late query result is dropped.
        cache.Detach();
        VERIFY_IS_FALSE(cache.TrySetVirtualDesktopId(token, id));

        // A different window never inherits the previous window's desktop id.
        TestWindow second;
        VERIFY_IS_NOT_NULL(second.Get());
        VERIFY_IS_TRUE(cache.TryCapture(second.Get(), PresenterKind::Overlapped));
        VERIFY_ARE_NOT_EQUAL(token, cache.LifetimeToken());
        VERIFY_IS_FALSE(cache.TryGetPlacement()->VirtualDesktopId.has_value());
        VERIFY_IS_FALSE(cache.TrySetVirtualDesktopId(token, id));
    }

    TEST_METHOD(SchedulesVirtualDesktopRefreshOnlyForADisplayedOpenWindow)
    {
        VERIFY_IS_TRUE(ShouldScheduleVirtualDesktopRefresh(false, true));
        VERIFY_IS_FALSE(ShouldScheduleVirtualDesktopRefresh(true, true));
        VERIFY_IS_FALSE(ShouldScheduleVirtualDesktopRefresh(false, false));
    }

    TEST_METHOD(RunsVirtualDesktopRefreshOnlyForAnEnrolledOpenWindow)
    {
        // A window that never enrolled can never save, so the shell must not be asked.
        VERIFY_IS_TRUE(ShouldRunVirtualDesktopRefresh(false, true));
        VERIFY_IS_FALSE(ShouldRunVirtualDesktopRefresh(true, true));
        VERIFY_IS_FALSE(ShouldRunVirtualDesktopRefresh(false, false));
        VERIFY_IS_FALSE(ShouldRunVirtualDesktopRefresh(true, false));
    }

    TEST_METHOD(RefreshAppliesOnlyASuccessfulQueryToALiveWindow)
    {
        PhysicalCoordinateScope coordinates;
        const auto id = TestDesktopId();
        WindowPlacementCapture cache;

        const auto succeed = [&id](HWND, GUID& result) noexcept
        {
            result = id;
            return true;
        };
        const auto fail = [](HWND, GUID&) noexcept { return false; };

        // Nothing is bound yet, so there is no token for a result to attach to. The query
        // must not even run.
        bool ran = false;
        VERIFY_IS_FALSE(TryRefreshVirtualDesktopId(
            cache, nullptr, [&ran](HWND, GUID&) noexcept { ran = true; return true; }));
        VERIFY_IS_FALSE(ran);

        TestWindow window;
        VERIFY_IS_NOT_NULL(window.Get());
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));

        // An unavailable or failing shell query leaves the cached placement usable.
        VERIFY_IS_FALSE(TryRefreshVirtualDesktopId(cache, window.Get(), fail));
        VERIFY_IS_NOT_NULL(cache.TryGetPlacement());
        VERIFY_IS_FALSE(cache.TryGetPlacement()->VirtualDesktopId.has_value());

        VERIFY_IS_TRUE(TryRefreshVirtualDesktopId(cache, window.Get(), succeed));
        VERIFY_IS_TRUE(cache.TryGetPlacement()->VirtualDesktopId.has_value());
        VERIFY_IS_TRUE(::IsEqualGUID(id, *cache.TryGetPlacement()->VirtualDesktopId) != FALSE);
    }

    TEST_METHOD(RefreshDropsAResultThatOutlivesItsWindow)
    {
        PhysicalCoordinateScope coordinates;
        const auto id = TestDesktopId();
        WindowPlacementCapture cache;

        TestWindow first;
        VERIFY_IS_NOT_NULL(first.Get());
        VERIFY_IS_TRUE(cache.TryCapture(first.Get(), PresenterKind::Overlapped));

        TestWindow second;
        VERIFY_IS_NOT_NULL(second.Get());

        // The window is retired and replaced while the query is outstanding. The token taken
        // before the query no longer matches, so the result is dropped instead of being
        // attached to the wrong window.
        const auto rebindDuringQuery = [&](HWND, GUID& result)
        {
            cache.Detach();
            VERIFY_IS_TRUE(cache.TryCapture(second.Get(), PresenterKind::Overlapped));
            result = id;
            return true;
        };

        VERIFY_IS_FALSE(TryRefreshVirtualDesktopId(cache, first.Get(), rebindDuringQuery));
        VERIFY_IS_NOT_NULL(cache.TryGetPlacement());
        VERIFY_IS_FALSE(cache.TryGetPlacement()->VirtualDesktopId.has_value());
    }

    TEST_METHOD(RejectsRecaptureOfDetachedWindow)    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        VERIFY_IS_NOT_NULL(window.Get());
        WindowPlacementCapture cache;
        VERIFY_IS_TRUE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        const auto token = cache.LifetimeToken();
        const auto id = TestDesktopId();
        VERIFY_IS_TRUE(cache.TrySetVirtualDesktopId(token, id));
        const auto before = *cache.TryGetPlacement();

        cache.Detach();
        cache.Detach(); // Repeated teardown must not forget the retired HWND.
        window.Move(40, 30);
        for (const auto presenter : {PresenterKind::NonOverlapped, PresenterKind::Overlapped})
        {
            VERIFY_IS_FALSE(cache.TryCapture(window.Get(), presenter));
            VERIFY_IS_NOT_NULL(cache.TryGetPlacement());
            VerifySameSnapshot(before, *cache.TryGetPlacement());
            VERIFY_ARE_EQUAL(0u, cache.LifetimeToken());
            VERIFY_IS_FALSE(cache.TrySetVirtualDesktopId(token, id));
            VERIFY_IS_FALSE(cache.TrySetVirtualDesktopId(cache.LifetimeToken(), id));
        }

        TestWindow replacement;
        VERIFY_IS_NOT_NULL(replacement.Get());
        // A failed replacement must not clear the detached state or issue a token.
        VERIFY_IS_FALSE(cache.TryCapture(replacement.Get(), PresenterKind::NonOverlapped));
        VERIFY_ARE_EQUAL(0u, cache.LifetimeToken());
        VERIFY_IS_FALSE(cache.TryCapture(window.Get(), PresenterKind::Overlapped));
        VerifySameSnapshot(before, *cache.TryGetPlacement());
        VERIFY_IS_TRUE(cache.TryCapture(replacement.Get(), PresenterKind::Overlapped));
        VERIFY_ARE_NOT_EQUAL(0u, cache.LifetimeToken());
        VERIFY_ARE_EQUAL(token + 1, cache.LifetimeToken());
        VERIFY_IS_FALSE(cache.TryGetPlacement()->VirtualDesktopId.has_value());
        VERIFY_IS_FALSE(cache.TrySetVirtualDesktopId(token, id));
        VERIFY_IS_TRUE(cache.TrySetVirtualDesktopId(cache.LifetimeToken(), id));
    }

    TEST_METHOD(FailedCaptureDoesNotRebindOrDiscardSnapshot)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow first;
        TestWindow second;
        VERIFY_IS_NOT_NULL(first.Get());
        VERIFY_IS_NOT_NULL(second.Get());
        // A live child HWND passes IsWindow but is rejected by PlacementEx::GetPlacement.
        TestWindow child(first.Get());
        VERIFY_IS_NOT_NULL(child.Get());
        WindowPlacementCapture cache;
        static_assert(noexcept(cache.TryCapture(nullptr, PresenterKind::Overlapped)));

        VERIFY_IS_FALSE(cache.TryCapture(child.Get(), PresenterKind::Overlapped));
        VERIFY_IS_FALSE(cache.TryCapture(second.Get(), PresenterKind::NonOverlapped));
        VERIFY_IS_NULL(cache.TryGetPlacement());
        VERIFY_ARE_EQUAL(0u, cache.LifetimeToken());

        VERIFY_IS_TRUE(cache.TryCapture(first.Get(), PresenterKind::Overlapped));
        const auto token = cache.LifetimeToken();
        const auto id = TestDesktopId();
        VERIFY_IS_TRUE(cache.TrySetVirtualDesktopId(token, id));
        const auto before = *cache.TryGetPlacement();

        VERIFY_IS_FALSE(cache.TryCapture(nullptr, PresenterKind::Overlapped));
        VERIFY_IS_FALSE(cache.TryCapture(child.Get(), PresenterKind::Overlapped));
        VERIFY_IS_FALSE(cache.TryCapture(second.Get(), PresenterKind::NonOverlapped));
        VERIFY_IS_NOT_NULL(cache.TryGetPlacement());
        VerifySameSnapshot(before, *cache.TryGetPlacement());
        VERIFY_ARE_EQUAL(token, cache.LifetimeToken());

        // Failure did not consume a token or change the binding or its desktop overlay.
        VERIFY_IS_TRUE(cache.TryCapture(first.Get(), PresenterKind::Overlapped));
        VERIFY_ARE_EQUAL(token, cache.LifetimeToken());
        VERIFY_IS_TRUE(cache.TryGetPlacement()->VirtualDesktopId.has_value());
        VERIFY_IS_TRUE(::IsEqualGUID(id, *cache.TryGetPlacement()->VirtualDesktopId) != FALSE);
        VERIFY_IS_TRUE(cache.TryCapture(second.Get(), PresenterKind::Overlapped));
        VERIFY_ARE_EQUAL(token + 1, cache.LifetimeToken());
        VERIFY_IS_FALSE(cache.TryGetPlacement()->VirtualDesktopId.has_value());
        VERIFY_IS_FALSE(cache.TrySetVirtualDesktopId(token, id));
    }
};
