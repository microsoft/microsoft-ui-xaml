// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include "WindowPlacementApplication.h"
#include "WindowPlacementAdapter.h"
#include "WindowPlacementPhysicalCoordinates.h"
#include "WindowPlacementStore.h"

using namespace DirectUI::WindowPlacementPersistence;

namespace
{
    class TestWindow
    {
    public:
        TestWindow()
        {
            WNDCLASSW wc{};
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = ::GetModuleHandleW(nullptr);
            wc.lpszClassName = L"WindowPlacementApplicationTests";
            ::RegisterClassW(&wc);
            Handle = ::CreateWindowExW(0, wc.lpszClassName, L"Placement application",
                WS_OVERLAPPEDWINDOW, 120, 140, 640, 480,
                nullptr, nullptr, wc.hInstance, this);
            VERIFY_IS_NOT_NULL(Handle);
            Shows = Activations = 0;
        }
        ~TestWindow() { if (::IsWindow(Handle)) ::DestroyWindow(Handle); }

        HWND Handle{};
        int Shows{};
        int Activations{};
        bool Constrain{};
        bool DestroyDuringApply{};

    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            auto self = reinterpret_cast<TestWindow*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (message == WM_NCCREATE)
            {
                self = static_cast<TestWindow*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
                ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (self)
            {
                if (message == WM_SHOWWINDOW && wParam) ++self->Shows;
                if (message == WM_ACTIVATE && LOWORD(wParam) != WA_INACTIVE) ++self->Activations;
                if (message == WM_WINDOWPOSCHANGING && self->DestroyDuringApply)
                {
                    self->DestroyDuringApply = false;
                    ::DestroyWindow(hwnd);
                    return 0;
                }
                if (message == WM_GETMINMAXINFO && self->Constrain)
                {
                    ::DefWindowProcW(hwnd, message, wParam, lParam);
                    auto limits = reinterpret_cast<MINMAXINFO*>(lParam);
                    limits->ptMinTrackSize = {400, 300};
                    limits->ptMaxTrackSize = {500, 400};
                    return 0;
                }
            }
            return ::DefWindowProcW(hwnd, message, wParam, lParam);
        }
    };

    Snapshot Source(HWND hwnd)
    {
        WindowPlacementCapture capture;
        VERIFY_IS_TRUE(capture.TryCapture(hwnd, PresenterKind::Overlapped));
        auto source = *capture.TryGetPlacement();
        source.NormalRect = {source.WorkArea.X + 40, source.WorkArea.Y + 40, 440, 340};
        return source;
    }

    NativeRequest Request(HWND hwnd, const Snapshot& source, bool hidden = false, bool noActivate = false)
    {
        NativeRequest request{};
        NativeApplyOptions options{};
        options.KeepHidden = hidden;
        options.NoActivate = noActivate;
        VERIFY_IS_TRUE(TryComputeNativeRequest(
            hwnd, source, PlacementReason::ApplicationRestart, options, request));
        return request;
    }

    void VerifyRect(const Rect& expected, const Rect& actual)
    {
        VERIFY_ARE_EQUAL(expected.X, actual.X);
        VERIFY_ARE_EQUAL(expected.Y, actual.Y);
        VERIFY_ARE_EQUAL(expected.Width, actual.Width);
        VERIFY_ARE_EQUAL(expected.Height, actual.Height);
    }

    struct Loader
    {
        inline static int Calls{};
        inline static Snapshot Saved{};
        inline static LoadStatus Status{LoadStatus::Loaded};
        static long Load(const char16_t*, size_t, LoadResult& result)
        {
            ++Calls;
            result.Status = Status;
            result.Placement.Placement = Saved;
            return S_OK;
        }
    };
}

class WindowPlacementApplicationTests
{
public:
    TEST_CLASS(WindowPlacementApplicationTests);

    TEST_METHOD(InvalidInputDoesNotChangeWindowOrCache)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        WindowPlacementCapture cache;
        VERIFY_IS_TRUE(cache.TryCapture(window.Handle, PresenterKind::Overlapped));
        const auto before = *cache.TryGetPlacement();
        auto request = Request(window.Handle, Source(window.Handle));
        VERIFY_IS_TRUE(TryApplyNativeRequest(nullptr, request, cache) == ApplyOutcome::FailedBeforeStart);
        request.NormalRect.Width = 0;
        VERIFY_IS_TRUE(TryApplyNativeRequest(window.Handle, request, cache) == ApplyOutcome::FailedBeforeStart);
        VerifyRect(before.NormalRect, cache.TryGetPlacement()->NormalRect);
        VERIFY_ARE_EQUAL(0, window.Shows);
    }

    TEST_METHOD(HiddenNormalApplicationIsRepeatableAndNeverShowsOrActivates)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        const HWND foreground = ::GetForegroundWindow();
        WindowPlacementCapture cache;
        auto source = Source(window.Handle);
        for (int i = 0; i < 2; ++i)
        {
            source.NormalRect.X += 20;
            const auto request = Request(window.Handle, source, true);
            const auto outcome = TryApplyNativeRequest(window.Handle, request, cache);
            if (IsApplyWindowActionSupported())
            {
                VERIFY_IS_TRUE(outcome == ApplyOutcome::Applied);
                // Window Action may enforce additional work-area/chrome fitting.
                // The effective value must describe the HWND, not echo the request.
                WindowPlacementCapture independent;
                VERIFY_IS_TRUE(independent.TryCapture(window.Handle, PresenterKind::Overlapped));
                VerifyRect(independent.TryGetPlacement()->NormalRect, cache.TryGetPlacement()->NormalRect);
                VERIFY_ARE_EQUAL(request.NormalRect.Width, cache.TryGetPlacement()->NormalRect.Width);
                VERIFY_IS_TRUE(cache.TryGetPlacement()->PlacementState == State::Normal);
            }
            else
            {
                VERIFY_IS_TRUE(outcome == ApplyOutcome::FailedBeforeStart);
            }
            VERIFY_IS_FALSE(!!::IsWindowVisible(window.Handle));
            VERIFY_ARE_EQUAL(0, window.Shows);
            VERIFY_ARE_EQUAL(0, window.Activations);
            VERIFY_IS_TRUE(foreground == ::GetForegroundWindow());
        }
    }

    TEST_METHOD(SameMonitorRestorePreservesNonzeroCoordinates)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        WindowPlacementCapture cache;
        auto source = Source(window.Handle);
        source.NormalRect = {53, 71, 500, 400};

        const auto hiddenRequest = Request(window.Handle, source, true);
        if (IsApplyWindowActionSupported())
        {
            VERIFY_IS_TRUE(TryApplyNativeRequest(window.Handle, hiddenRequest, cache) ==
                ApplyOutcome::Applied);
            VerifyRect(source.NormalRect, cache.TryGetPlacement()->NormalRect);

            const auto shownRequest = Request(window.Handle, source);
            VERIFY_IS_TRUE(TryApplyNativeRequest(window.Handle, shownRequest, cache) ==
                ApplyOutcome::Applied);
            VerifyRect(source.NormalRect, cache.TryGetPlacement()->NormalRect);
        }
    }

    TEST_METHOD(MissingSavedSourceUsesFallbackForDefaultHiddenApplication)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        WindowPlacementCapture cache;
        Loader::Calls = 0;
        Loader::Status = LoadStatus::Missing;

        PlacementPass pass{};
        pass.PlacementId = u"test";
        pass.AutomaticPersistenceOptIn = true;
        pass.KeepHidden = true;

        VERIFY_IS_FALSE(TryApplyPlacement(window.Handle, pass, cache, Loader::Load));
        VERIFY_ARE_EQUAL(1, Loader::Calls);
        VERIFY_IS_FALSE(!!::IsWindowVisible(window.Handle));
        if (IsApplyWindowActionSupported())
        {
            VERIFY_IS_NOT_NULL(cache.TryGetPlacement());
        }

        Loader::Status = LoadStatus::Loaded;
    }

    TEST_METHOD(RestrictedRequestsNeverUseLegacyShowAndRepair)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        WindowPlacementCapture cache;
        const auto source = Source(window.Handle);
        for (bool hidden : {true, false})
        {
            const auto request = Request(window.Handle, source, hidden, true);
            VERIFY_IS_TRUE(TryApplyNativeRequest(window.Handle, request, cache, true) ==
                ApplyOutcome::FailedBeforeStart);
            VERIFY_IS_FALSE(!!::IsWindowVisible(window.Handle));
            VERIFY_ARE_EQUAL(0, window.Shows);
            VERIFY_ARE_EQUAL(0, window.Activations);
        }
    }

    TEST_METHOD(LegacyApplicationCapturesActualNormalGeometry)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        WindowPlacementCapture cache;
        auto request = Request(window.Handle, Source(window.Handle));
        VERIFY_IS_TRUE(TryApplyNativeRequest(window.Handle, request, cache, true) == ApplyOutcome::Applied);
        WindowPlacementCapture independent;
        VERIFY_IS_TRUE(independent.TryCapture(window.Handle, PresenterKind::Overlapped));
        VerifyRect(independent.TryGetPlacement()->NormalRect, cache.TryGetPlacement()->NormalRect);
        VERIFY_IS_TRUE(cache.TryGetPlacement()->PlacementState == State::Normal);
        VERIFY_IS_TRUE(!!::IsWindowVisible(window.Handle));
    }

    TEST_METHOD(NativeStatesAndRestoreTargetsAreCapturedWithoutActivation)
    {
        PhysicalCoordinateScope coordinates;
        for (State state : {State::Maximized, State::Minimized, State::MinimizedFromMaximized,
            State::Snapped, State::MinimizedFromSnapped})
        {
            TestWindow window;
            auto source = Source(window.Handle);
            source.PlacementState = state;
            if (state == State::Snapped || state == State::MinimizedFromSnapped)
            {
                source.SnapRect = source.WorkArea;
                source.SnapRect->Width /= 2;
            }
            WindowPlacementCapture cache;
            const auto request = Request(window.Handle, source, false, true);
            const auto outcome = TryApplyNativeRequest(window.Handle, request, cache);
            PlacementEx observed{};
            const bool nativeCaptured = PlacementEx::GetPlacement(
                window.Handle, &observed, CaptureFlags::SkipVirtualDesktopId);
            WEX::Logging::Log::Comment(WEX::Common::String().Format(
                L"State=%d outcome=%d capture=%d show=%u flags=%u normal=(%ld,%ld,%ld,%ld) arrange=(%ld,%ld,%ld,%ld)",
                static_cast<int>(state), static_cast<int>(outcome), nativeCaptured, observed.showCmd,
                static_cast<unsigned>(observed.flags), observed.normalRect.left, observed.normalRect.top,
                observed.normalRect.right, observed.normalRect.bottom, observed.arrangeRect.left,
                observed.arrangeRect.top, observed.arrangeRect.right, observed.arrangeRect.bottom));
            if (IsApplyWindowActionSupported() &&
                (state == State::Snapped || state == State::MinimizedFromSnapped) &&
                outcome == ApplyOutcome::FailedAfterStart)
            {
                // API availability does not guarantee that this OS accepts arranging
                // a newly created window. Failure must not fall back to show/repair.
                VERIFY_IS_TRUE(nativeCaptured);
                VERIFY_IS_FALSE(AllowsSourceRetry(outcome));
                VERIFY_IS_FALSE(!!::IsWindowVisible(window.Handle));
                VERIFY_ARE_EQUAL(0, window.Shows);
                VERIFY_IS_NOT_NULL(cache.TryGetPlacement());
                VERIFY_IS_TRUE(cache.TryGetPlacement()->PlacementState == State::Normal);
            }
            else if (IsApplyWindowActionSupported())
            {
                VERIFY_IS_TRUE(outcome == ApplyOutcome::Applied);
                const auto expected = !IsSnappingEnabled() && source.SnapRect ?
                    (state == State::Snapped ? State::Normal : State::Minimized) : state;
                VERIFY_IS_TRUE(cache.TryGetPlacement()->PlacementState == expected);
                VERIFY_IS_TRUE(!!::IsWindowVisible(window.Handle));
            }
            else
            {
                VERIFY_IS_TRUE(outcome == ApplyOutcome::FailedBeforeStart);
            }
            VERIFY_ARE_EQUAL(0, window.Activations);
        }
    }

    TEST_METHOD(SavedMinimizationRequiresApplicationRestart)
    {
        PhysicalCoordinateScope coordinates;
        for (auto reason : {PlacementReason::Default, PlacementReason::ApplicationRestart})
        {
            TestWindow window;
            auto source = Source(window.Handle);
            source.PlacementState = State::Minimized;
            NativeApplyOptions options{};
            options.NoActivate = true;
            NativeRequest request{};
            VERIFY_IS_TRUE(TryComputeNativeRequest(window.Handle, source, reason, options, request));
            const bool restart = reason == PlacementReason::ApplicationRestart;
            VERIFY_IS_TRUE(request.ShowCommand ==
                (restart ? NativeShowCommand::Minimize : NativeShowCommand::Normal));

            PlacementEx placement{};
            VERIFY_IS_TRUE(TryCreateEnginePlacement(request, placement));
            VERIFY_ARE_EQUAL(static_cast<UINT>(restart ? SW_MINIMIZE : SW_NORMAL), placement.showCmd);

            WindowPlacementCapture cache;
            const auto outcome = TryApplyNativeRequest(window.Handle, request, cache);
            WEX::Logging::Log::Comment(WEX::Common::String().Format(
                L"Saved minimized: reason=%d engineShow=%u outcome=%d iconic=%d visible=%d",
                static_cast<int>(reason), placement.showCmd, static_cast<int>(outcome),
                !!::IsIconic(window.Handle), !!::IsWindowVisible(window.Handle)));
            if (IsApplyWindowActionSupported())
            {
                // NoActivate prohibits fallback. Applied requires SetPlacement's native
                // Window Action call to return true and effective capture to succeed.
                VERIFY_IS_TRUE(outcome == ApplyOutcome::Applied);
                VERIFY_IS_TRUE(cache.TryGetPlacement()->PlacementState ==
                    (restart ? State::Minimized : State::Normal));
                VerifyRect(request.NormalRect, cache.TryGetPlacement()->NormalRect);
                VERIFY_ARE_EQUAL(restart, !!::IsIconic(window.Handle));
                VERIFY_IS_TRUE(!!::IsWindowVisible(window.Handle));
            }
            else
            {
                VERIFY_IS_TRUE(outcome == ApplyOutcome::FailedBeforeStart);
            }
            VERIFY_ARE_EQUAL(0, window.Activations);
        }
    }

    TEST_METHOD(SavedSnapApplicationOnANewWindow)
    {
        PhysicalCoordinateScope coordinates;
        const bool snapping = IsSnappingEnabled();
        for (State state : {State::Snapped, State::MinimizedFromSnapped})
        {
            TestWindow window;
            auto source = Source(window.Handle);
            source.PlacementState = state;
            source.SnapRect = source.WorkArea;
            source.SnapRect->Width /= 2;

            NativeApplyOptions options{};
            options.NoActivate = true;
            NativeRequest request{};
            VERIFY_IS_TRUE(TryComputeNativeRequest(
                window.Handle, source, PlacementReason::ApplicationRestart, options, request));

            const bool minimizing = state == State::MinimizedFromSnapped;
            // The arrange action is only requested when the user has snapping enabled.
            VERIFY_ARE_EQUAL(snapping && !minimizing, request.Flags.Arranged);
            VERIFY_ARE_EQUAL(snapping && minimizing, request.Flags.RestoreToArranged);
            VERIFY_ARE_EQUAL(snapping, request.ArrangeRect.has_value());
            VERIFY_IS_TRUE(request.ShowCommand ==
                (minimizing ? NativeShowCommand::Minimize : NativeShowCommand::Normal));

            PlacementEx placement{};
            VERIFY_IS_TRUE(TryCreateEnginePlacement(request, placement));
            VERIFY_ARE_EQUAL(static_cast<UINT>(minimizing ? SW_MINIMIZE : SW_NORMAL), placement.showCmd);
            VERIFY_ARE_EQUAL(snapping && !minimizing, placement.HasFlag(PlacementFlags::Arranged));
            VERIFY_ARE_EQUAL(snapping && minimizing, placement.HasFlag(PlacementFlags::RestoreToArranged));

            WindowPlacementCapture cache;
            const auto outcome = TryApplyNativeRequest(window.Handle, request, cache);
            PlacementEx observed{};
            const bool nativeCaptured = PlacementEx::GetPlacement(
                window.Handle, &observed, CaptureFlags::SkipVirtualDesktopId);
            WEX::Logging::Log::Comment(WEX::Common::String().Format(
                L"Saved snap: state=%d snapping=%d arranged=%d restoreToArranged=%d engineShow=%u "
                L"outcome=%d capture=%d observedShow=%u observedFlags=%u iconic=%d visible=%d",
                static_cast<int>(state), snapping, request.Flags.Arranged,
                request.Flags.RestoreToArranged, placement.showCmd, static_cast<int>(outcome),
                nativeCaptured, observed.showCmd, static_cast<unsigned>(observed.flags),
                !!::IsIconic(window.Handle), !!::IsWindowVisible(window.Handle)));

            if (!IsApplyWindowActionSupported())
            {
                VERIFY_IS_TRUE(outcome == ApplyOutcome::FailedBeforeStart);
            }
            else if (outcome == ApplyOutcome::Applied)
            {
                const auto expected = snapping ?
                    state : (minimizing ? State::Minimized : State::Normal);
                VERIFY_IS_TRUE(cache.TryGetPlacement()->PlacementState == expected);
                VERIFY_ARE_EQUAL(minimizing, !!::IsIconic(window.Handle));
                VERIFY_IS_TRUE(!!::IsWindowVisible(window.Handle));
            }
            else
            {
                // NoActivate prohibits legacy fallback, so this is the OS declining the
                // Window Action. The window must be left hidden and unchanged.
                VERIFY_IS_TRUE(outcome == ApplyOutcome::FailedAfterStart);
                VERIFY_IS_TRUE(nativeCaptured);
                VERIFY_IS_FALSE(AllowsSourceRetry(outcome));
                VERIFY_IS_FALSE(!!::IsWindowVisible(window.Handle));
                VERIFY_ARE_EQUAL(0, window.Shows);
            }
            VERIFY_ARE_EQUAL(0, window.Activations);
        }
    }

    TEST_METHOD(MinimizedFromSnappedRestoresToSavedSnapBounds)
    {
        PhysicalCoordinateScope coordinates;
        if (!IsApplyWindowActionSupported() || !IsSnappingEnabled())
        {
            WEX::Logging::Log::Comment(
                L"Skipping restore-to-snap verification because the required OS capability is unavailable.");
            return;
        }

        TestWindow window;
        auto source = Source(window.Handle);
        source.PlacementState = State::MinimizedFromSnapped;
        source.SnapRect = source.WorkArea;
        source.SnapRect->Width /= 2;

        const auto request = Request(window.Handle, source, false, true);
        WindowPlacementCapture cache;
        const auto outcome = TryApplyNativeRequest(window.Handle, request, cache);
        VERIFY_IS_TRUE(outcome == ApplyOutcome::Applied);
        VERIFY_IS_TRUE(!!::IsIconic(window.Handle));

        ::ShowWindow(window.Handle, SW_RESTORE);

        PlacementEx observed{};
        VERIFY_IS_TRUE(PlacementEx::GetPlacement(
            window.Handle, &observed, CaptureFlags::SkipVirtualDesktopId));
        WEX::Logging::Log::Comment(WEX::Common::String().Format(
            L"Restore from minimized snap: saved=(%ld,%ld,%ld,%ld) observed=(%ld,%ld,%ld,%ld) "
            L"flags=%u iconic=%d",
            source.SnapRect->X, source.SnapRect->Y,
            source.SnapRect->X + source.SnapRect->Width,
            source.SnapRect->Y + source.SnapRect->Height,
            observed.arrangeRect.left, observed.arrangeRect.top,
            observed.arrangeRect.right, observed.arrangeRect.bottom,
            static_cast<unsigned>(observed.flags), !!::IsIconic(window.Handle)));
        VERIFY_IS_FALSE(!!::IsIconic(window.Handle));
        VERIFY_IS_TRUE(observed.HasFlag(PlacementFlags::Arranged));
        VERIFY_ARE_EQUAL(source.SnapRect->X, observed.arrangeRect.left);
        VERIFY_ARE_EQUAL(source.SnapRect->Y, observed.arrangeRect.top);
        VERIFY_ARE_EQUAL(
            source.SnapRect->X + source.SnapRect->Width, observed.arrangeRect.right);
        VERIFY_ARE_EQUAL(
            source.SnapRect->Y + source.SnapRect->Height, observed.arrangeRect.bottom);
    }

    TEST_METHOD(UnsupportedHiddenStatesFailBeforeChangingTheWindow)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        auto source = Source(window.Handle);
        NativeApplyOptions options{};
        options.KeepHidden = true;
        for (State state : {State::Maximized, State::Minimized, State::Snapped})
        {
            source.PlacementState = state;
            if (state == State::Snapped) source.SnapRect = source.WorkArea;
            NativeRequest request{};
            VERIFY_IS_FALSE(TryComputeNativeRequest(
                window.Handle, source, PlacementReason::ApplicationRestart, options, request));
        }
        VERIFY_ARE_EQUAL(0, window.Shows);
        VERIFY_ARE_EQUAL(0, window.Activations);
    }

    TEST_METHOD(SelectedSourceIsNeverReloadedAfterApplicationStarts)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        WindowPlacementCapture cache;
        Loader::Saved = Source(window.Handle);
        Loader::Calls = 0;
        PlacementPass pass{};
        pass.AutomaticPersistenceOptIn = true;
        pass.PlacementId = u"test";
        window.DestroyDuringApply = true;
        VERIFY_IS_FALSE(TryApplyPlacement(window.Handle, pass, cache, Loader::Load));
        VERIFY_ARE_EQUAL(1, Loader::Calls);
        VERIFY_IS_FALSE(!!::IsWindow(window.Handle));
    }

    TEST_METHOD(ExplicitFailureDoesNotReadSavedSourceAndOptOutDoesNotLoad)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        WindowPlacementCapture cache;
        Loader::Calls = 0;
        Loader::Saved = Source(window.Handle);
        PlacementPass pass{};
        pass.PlacementId = u"test";
        pass.KeepHidden = true;
        VERIFY_IS_FALSE(TryApplyPlacement(window.Handle, pass, cache, Loader::Load));
        VERIFY_ARE_EQUAL(0, Loader::Calls);
        pass.AutomaticPersistenceOptIn = true;
        pass.Request.Placement = Loader::Saved;
        pass.Request.Placement->PlacementState = State::Maximized;
        VERIFY_IS_FALSE(TryApplyPlacement(window.Handle, pass, cache, Loader::Load));
        VERIFY_ARE_EQUAL(0, Loader::Calls);
        VERIFY_ARE_EQUAL(0, window.Shows);
    }

    TEST_METHOD(SavedSourceAppliesAndCanBeReplacedByAnotherHiddenPass)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        WindowPlacementCapture cache;
        Loader::Calls = 0;
        Loader::Saved = Source(window.Handle);
        PlacementPass pass{};
        pass.PlacementId = u"test";
        pass.AutomaticPersistenceOptIn = true;
        pass.KeepHidden = true;
        for (int i = 0; i < 2; ++i)
        {
            Loader::Saved.NormalRect.Width += 20;
            VERIFY_ARE_EQUAL(IsApplyWindowActionSupported(),
                TryApplyPlacement(window.Handle, pass, cache, Loader::Load));
            VERIFY_ARE_EQUAL(i + 1, Loader::Calls);
            if (IsApplyWindowActionSupported())
            {
                VERIFY_ARE_EQUAL(Loader::Saved.NormalRect.Width, cache.TryGetPlacement()->NormalRect.Width);
            }
        }
        VERIFY_ARE_EQUAL(0, window.Shows);
        VERIFY_ARE_EQUAL(0, window.Activations);
    }

    TEST_METHOD(UnrepresentableEngineIntermediatesFailBeforeApplication)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        auto source = Source(window.Handle);
        source.NormalRect = {0, 0, INT32_MAX, 400};
        source.WorkArea = {0, 0, INT32_MAX, 1000};
        VERIFY_IS_TRUE(IsValid(source));
        NativeRequest request{};
        VERIFY_IS_FALSE(TryComputeNativeRequest(
            window.Handle, source, PlacementReason::Default, {}, request));
        VERIFY_ARE_EQUAL(0, window.Shows);
    }

    TEST_METHOD(CurrentWindowConstraintsAndStyleFeedPolicy)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        window.Constrain = true;
        auto source = Source(window.Handle);
        source.NormalRect.Width = 800;
        source.NormalRect.Height = 200;
        auto request = Request(window.Handle, source);
        VERIFY_ARE_EQUAL(500, request.NormalRect.Width);
        VERIFY_ARE_EQUAL(300, request.NormalRect.Height);
        WindowPlacementCapture cache;
        VERIFY_IS_TRUE(TryApplyNativeRequest(window.Handle, request, cache, true) == ApplyOutcome::Applied);
        VerifyRect(request.NormalRect, cache.TryGetPlacement()->NormalRect);
        ::SetWindowLongPtrW(window.Handle, GWL_STYLE,
            ::GetWindowLongPtrW(window.Handle, GWL_STYLE) & ~WS_THICKFRAME);
        request = Request(window.Handle, source);
        VERIFY_IS_FALSE(request.Flags.AllowSizing);
    }

    // Track sizes are reported at the window's DPI, so moving to a monitor with a
    // different DPI must rescale them. One display makes that conversion an identity,
    // so the rule is tested directly.
    TEST_METHOD(TrackSizesScaleToTheTargetMonitorDpi)
    {
        int32_t scaled = -1;

        // Same DPI is an identity, which is all a single display can observe.
        VERIFY_IS_TRUE(TryScaleTrackSize(500, 96, 96, scaled));
        VERIFY_ARE_EQUAL(500, scaled);

        // A higher-DPI target grows the limit, a lower-DPI target shrinks it.
        VERIFY_IS_TRUE(TryScaleTrackSize(500, 96, 192, scaled));
        VERIFY_ARE_EQUAL(1000, scaled);
        VERIFY_IS_TRUE(TryScaleTrackSize(500, 192, 96, scaled));
        VERIFY_ARE_EQUAL(250, scaled);

        // MulDiv rounds to nearest, so an odd ratio must not truncate toward zero.
        VERIFY_IS_TRUE(TryScaleTrackSize(101, 96, 144, scaled));
        VERIFY_ARE_EQUAL(152, scaled);

        // Zero and negative track sizes mean unconstrained and stay unconstrained.
        VERIFY_IS_TRUE(TryScaleTrackSize(0, 96, 192, scaled));
        VERIFY_ARE_EQUAL(0, scaled);
        VERIFY_IS_TRUE(TryScaleTrackSize(-5, 96, 192, scaled));
        VERIFY_ARE_EQUAL(0, scaled);

        // A DPI that cannot produce a ratio fails instead of yielding a bad limit.
        scaled = -1;
        VERIFY_IS_FALSE(TryScaleTrackSize(500, 0, 192, scaled));
        VERIFY_ARE_EQUAL(0, scaled);
        VERIFY_IS_FALSE(TryScaleTrackSize(500, 96, 0, scaled));
        VERIFY_IS_FALSE(TryScaleTrackSize(500, 96, static_cast<UINT>(INT32_MAX) + 1, scaled));

        // Overflow is reported as a failure, never as a negative constraint.
        VERIFY_IS_FALSE(TryScaleTrackSize(INT32_MAX, 1, static_cast<UINT>(INT32_MAX), scaled));
        VERIFY_ARE_EQUAL(0, scaled);
    }

    // PlacementEx::FindClosestMonitor is FromDeviceName() || FromRect(). A saved display that is
    // no longer connected must take the rect fallback and still target a real monitor. One display
    // cannot show which monitor is chosen, but it can show that the fallback runs and matches what
    // the name-match branch would have produced.
    TEST_METHOD(AStaleSavedDisplayNameFallsBackToTheConnectedMonitor)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        const auto source = Source(window.Handle);
        VERIFY_IS_FALSE(source.DisplayDeviceName.empty());
        const auto matched = Request(window.Handle, source);
        VERIFY_IS_TRUE(matched.DeviceName == source.DisplayDeviceName);

        auto removed = source;
        removed.DisplayDeviceName = u"\\\\.\\DISPLAY_REMOVED";
        VERIFY_IS_TRUE(IsValid(removed));
        VERIFY_IS_FALSE(removed.DisplayDeviceName == source.DisplayDeviceName);

        // The unknown name must not fail the request or leak into the result.
        const auto fallback = Request(window.Handle, removed);
        VERIFY_IS_TRUE(fallback.DeviceName == source.DisplayDeviceName);
        VerifyRect(matched.NormalRect, fallback.NormalRect);
        VerifyRect(matched.WorkArea, fallback.WorkArea);
        VERIFY_ARE_EQUAL(matched.Dpi, fallback.Dpi);
    }

    // PlacementEx::MoveToMonitor rescales from the placement's own saved work area and DPI to the
    // target monitor's, so a record saved at a different display scale rescales on one display too.
    TEST_METHOD(APlacementSavedAtAHigherDpiRescalesOnTheConnectedMonitor)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        const auto source = Source(window.Handle);
        VERIFY_IS_TRUE(source.Dpi >= 96);

        auto saved = source;
        saved.Dpi = source.Dpi * 2;
        VERIFY_IS_TRUE(IsValid(saved));

        // The work area is unchanged, so only the DPI ratio moves the geometry.
        const auto request = Request(window.Handle, saved);
        VERIFY_ARE_EQUAL(source.Dpi, request.Dpi);
        VERIFY_ARE_EQUAL(saved.NormalRect.X, request.NormalRect.X);
        VERIFY_ARE_EQUAL(saved.NormalRect.Y, request.NormalRect.Y);
        VERIFY_ARE_EQUAL(saved.NormalRect.Width / 2, request.NormalRect.Width);
        VERIFY_ARE_EQUAL(saved.NormalRect.Height / 2, request.NormalRect.Height);

        // Echoing the saved size through unchanged would fail the assertions above.
        VERIFY_ARE_NOT_EQUAL(saved.NormalRect.Width, request.NormalRect.Width);
        VERIFY_ARE_NOT_EQUAL(saved.NormalRect.Height, request.NormalRect.Height);

        // A record saved at the monitor's own DPI is left alone, so the rescale is not
        // something the production path does to every restore.
        const auto unscaled = Request(window.Handle, source);
        VerifyRect(source.NormalRect, unscaled.NormalRect);
    }

    // STARTUPINFOW::hStdOutput is the launch monitor hint for a shell launch and the real
    // standard output handle for a redirected launch. A probe on the build host measured
    // dwFlags=0x00000100 with a live hStdOutput=0x8A8 for a redirected launch, and
    // hStdOutput=0x0 for direct, explorer.exe, and ShellExecute launches. So the only
    // observed launches that populate the field are the ones where it is not a monitor.
    TEST_METHOD(LaunchMonitorHintIgnoresARedirectedStandardOutputHandle)
    {
        STARTUPINFOW startup{sizeof(startup)};
        HMONITOR monitor = reinterpret_cast<HMONITOR>(static_cast<INT_PTR>(-1));

        // No hint at all. This is what every observed shell launch reports.
        VERIFY_IS_FALSE(TryGetLaunchMonitorHint(startup, monitor));
        VERIFY_IS_NULL(monitor);

        // The measured redirected launch: a live pipe handle that is not a monitor.
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdOutput = reinterpret_cast<HANDLE>(static_cast<INT_PTR>(0x8A8));
        monitor = reinterpret_cast<HMONITOR>(static_cast<INT_PTR>(-1));
        VERIFY_IS_FALSE(TryGetLaunchMonitorHint(startup, monitor));
        VERIFY_IS_NULL(monitor);

        // The standard-handle flag decides the meaning, so it is rejected even alongside
        // other flags and even when the handle value would pass a topology lookup.
        startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        startup.hStdOutput = reinterpret_cast<HANDLE>(
            ::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
        VERIFY_IS_NOT_NULL(startup.hStdOutput);
        VERIFY_IS_FALSE(TryGetLaunchMonitorHint(startup, monitor));
        VERIFY_IS_NULL(monitor);

        // A redirected launch that left the handle null is still not a hint.
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdOutput = nullptr;
        VERIFY_IS_FALSE(TryGetLaunchMonitorHint(startup, monitor));

        // A real hint is still consumed. STARTF_USEMONITOR is 0x400 and is not in the
        // public SDK headers, so both the flagged and unflagged forms are accepted.
        HANDLE primary = reinterpret_cast<HANDLE>(
            ::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
        startup.dwFlags = 0;
        startup.hStdOutput = primary;
        VERIFY_IS_TRUE(TryGetLaunchMonitorHint(startup, monitor));
        VERIFY_ARE_EQUAL(primary, reinterpret_cast<HANDLE>(monitor));

        startup.dwFlags = 0x00000400 | STARTF_USESHOWWINDOW;
        monitor = nullptr;
        VERIFY_IS_TRUE(TryGetLaunchMonitorHint(startup, monitor));
        VERIFY_ARE_EQUAL(primary, reinterpret_cast<HANDLE>(monitor));

        // The accepted handle is a real monitor, so the caller's lookup can succeed.
        MONITORINFO info{sizeof(info)};
        VERIFY_IS_TRUE(::GetMonitorInfoW(monitor, &info) != FALSE);
    }

    TEST_METHOD(DestructionDuringNativeApplyIsNotReportedAsSuccess)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        WindowPlacementCapture cache;
        VERIFY_IS_TRUE(cache.TryCapture(window.Handle, PresenterKind::Overlapped));
        const auto before = *cache.TryGetPlacement();
        const auto request = Request(window.Handle, Source(window.Handle));
        window.DestroyDuringApply = true;
        const auto outcome = TryApplyNativeRequest(window.Handle, request, cache, true);
        VERIFY_IS_TRUE(outcome == ApplyOutcome::FailedAfterStart);
        VERIFY_IS_FALSE(AllowsSourceRetry(outcome));
        VERIFY_IS_FALSE(!!::IsWindow(window.Handle));
        VerifyRect(before.NormalRect, cache.TryGetPlacement()->NormalRect);
    }

    TEST_METHOD(CoordinatorRunsRealApplicationAndSkipDoesNotReapply)
    {
        PhysicalCoordinateScope coordinates;
        TestWindow window;
        class Host final : public IWindowPlacementCoordinatorHost
        {
        public:
            HWND Hwnd{};
            int Applies{};
            WindowPlacementCapture Capture;
            bool IsClosed() const noexcept override { return !::IsWindow(Hwnd); }
            bool IsVisible() const noexcept override { return !!::IsWindowVisible(Hwnd); }
            bool ApplyPlacement(const PlacementPass& pass) override
            {
                ++Applies;
                return TryApplyPlacement(Hwnd, pass, Capture);
            }
            bool Display(CoordinatorOperation, bool noActivate) override
            {
                ::ShowWindow(Hwnd, noActivate ? SW_SHOWNA : SW_SHOW);
                return IsVisible();
            }
            bool TryCapturePlacementAndSave() noexcept override { return false; }
        } host;
        host.Hwnd = window.Handle;
        WindowPlacementCoordinator coordinator(host);
        InitialRequest request{};
        request.Placement = Source(window.Handle);
        const bool applied = coordinator.TryApplyInitialPlacement(request, u"", 0, false);
        VERIFY_ARE_EQUAL(IsApplyWindowActionSupported(), applied);
        VERIFY_IS_TRUE(coordinator.IsInitialPlacementPhaseOpen());
        VERIFY_IS_FALSE(coordinator.HasEnrollment());
        request = {};
        request.SkipInitialPlacement = true;
        request.DoNotActivate = true;
        VERIFY_IS_TRUE(coordinator.Show(request, u"", 0, false));
        VERIFY_ARE_EQUAL(1, host.Applies);
        VERIFY_IS_FALSE(coordinator.IsInitialPlacementPhaseOpen());
    }
};
