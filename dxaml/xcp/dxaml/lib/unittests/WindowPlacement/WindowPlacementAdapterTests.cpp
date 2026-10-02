// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowPlacementAdapter.h"
#include <WexTestClass.h>

#include <limits>

using namespace DirectUI::WindowPlacementPersistence;

namespace
{
    NativeRequest Request()
    {
        NativeRequest request{};
        request.NormalRect = {-1800, 100, 800, 600};
        request.WorkArea = {-1920, 0, 1920, 1040};
        request.Dpi = 96;
        request.DeviceName = u"\\\\.\\DISPLAY2";
        return request;
    }

    void VerifyRect(const Rect& value, const RECT& engine)
    {
        VERIFY_ARE_EQUAL(value.X, engine.left);
        VERIFY_ARE_EQUAL(value.Y, engine.top);
        VERIFY_ARE_EQUAL(value.Width, engine.right - engine.left);
        VERIFY_ARE_EQUAL(value.Height, engine.bottom - engine.top);
    }

    void VerifyRejected(const NativeRequest& request)
    {
        PlacementEx sentinel{};
        sentinel.dpi = 123;
        VERIFY_IS_FALSE(TryCreateEnginePlacement(request, sentinel));
        VERIFY_ARE_EQUAL(123u, sentinel.dpi);
    }

    void VerifyRejected(const PlacementEx& placement)
    {
        auto sentinel = Request();
        sentinel.Dpi = 123;
        VERIFY_IS_FALSE(TryReadEnginePlacement(placement, sentinel));
        VERIFY_ARE_EQUAL(123, sentinel.Dpi);
        VERIFY_IS_TRUE(sentinel.DeviceName == Request().DeviceName);
    }
}

class WindowPlacementAdapterTests
{
public:
    TEST_CLASS(WindowPlacementAdapterTests);

    TEST_METHOD(WindowActionMatchesSdkLayout)
    {
        VERIFY_ARE_EQUAL(sizeof(void*) == 8 ? size_t{96} : size_t{84}, sizeof(WINDOW_ACTION));
        VERIFY_ARE_EQUAL(sizeof(void*) == 8 ? size_t{40} : size_t{32}, offsetof(WINDOW_ACTION, placementState));
        VERIFY_ARE_EQUAL(sizeof(void*) == 8 ? size_t{88} : size_t{80}, offsetof(WINDOW_ACTION, monitorTopologyId));
        VERIFY_ARE_EQUAL(0x100, static_cast<int>(WAK_FIT_TO_MONITOR));
        VERIFY_ARE_EQUAL(0x80, static_cast<int>(WAM_RESTORE_TO_ARRANGED));
    }

    TEST_METHOD(TranslatesAllSixPolicyStates)
    {
        for (int state = 0; state != 6; ++state)
        {
            const auto source = Request();
            PlacementPlan plan{};
            plan.NormalRect = source.NormalRect;
            plan.TargetMonitor = { source.DeviceName, source.WorkArea, source.Dpi };
            plan.State = state == 0 ? TargetState::Normal :
                state == 1 ? TargetState::Maximized :
                state == 3 ? TargetState::Snapped : TargetState::Minimized;
            plan.RestoreToMaximized = state == 4;
            plan.RestoreToSnapped = state == 5;
            if (state == 3 || state == 5) plan.SnapRect = Rect{-1920, 0, 960, 1040};

            NativeRequest request{};
            VERIFY_IS_TRUE(BuildNativeRequest(plan, {}, request));
            PlacementEx engine{};
            VERIFY_IS_TRUE(TryCreateEnginePlacement(request, engine));
            VERIFY_IS_TRUE(engine.IsValid());
            VERIFY_ARE_EQUAL(state == 0 || state == 3 ? UINT{SW_NORMAL} :
                state == 1 ? UINT{SW_MAXIMIZE} : UINT{SW_MINIMIZE}, engine.showCmd);
            VERIFY_ARE_EQUAL(state == 3, engine.HasFlag(PlacementFlags::Arranged));
            VERIFY_ARE_EQUAL(state == 4, engine.HasFlag(PlacementFlags::RestoreToMaximized));
            VERIFY_ARE_EQUAL(state == 5, engine.HasFlag(PlacementFlags::RestoreToArranged));
            VERIFY_IS_FALSE(engine.HasFlag(PlacementFlags::AllowPartiallyOffScreen));
            VERIFY_IS_FALSE(engine.HasFlag(PlacementFlags::FullScreen));

            NativeRequest result{};
            VERIFY_IS_TRUE(TryReadEnginePlacement(engine, result));
            VERIFY_IS_TRUE(result.ShowCommand == request.ShowCommand);
            VERIFY_ARE_EQUAL(request.Flags.Arranged, result.Flags.Arranged);
            VERIFY_ARE_EQUAL(request.Flags.RestoreToArranged, result.Flags.RestoreToArranged);
            VERIFY_ARE_EQUAL(request.Flags.RestoreToMaximized, result.Flags.RestoreToMaximized);
            VerifyRect(result.NormalRect, engine.normalRect);
            VerifyRect(result.WorkArea, engine.workArea);
            VERIFY_IS_TRUE(result.DeviceName == request.DeviceName);
            VERIFY_ARE_EQUAL(request.Dpi, result.Dpi);
            VERIFY_ARE_EQUAL(request.ArrangeRect.has_value(), result.ArrangeRect.has_value());
            if (result.ArrangeRect) VerifyRect(*result.ArrangeRect, engine.arrangeRect);
        }
    }

    TEST_METHOD(TranslatesCallTimeFlagsWithoutStartupPolicy)
    {
        auto request = Request();
        request.ShowCommand = NativeShowCommand::NoChange;
        request.Flags.KeepHidden = true;
        request.Flags.NoActivate = true;
        request.Flags.AllowSizing = false;
        request.Flags.UseVirtualDesktopId = true;
        request.VirtualDesktopId = GUID{0x12345678, 0x1234, 0x5678, {1, 2, 3, 4, 5, 6, 7, 8}};
        PlacementEx engine{};
        VERIFY_IS_TRUE(TryCreateEnginePlacement(request, engine));
        VERIFY_ARE_EQUAL(UINT{SW_HIDE}, engine.showCmd);
        VERIFY_IS_TRUE(engine.flags ==
            (PlacementFlags::KeepHidden | PlacementFlags::NoActivate | PlacementFlags::VirtualDesktopId));
        VERIFY_IS_TRUE(IsEqualGUID(*request.VirtualDesktopId, engine.virtualDesktopId) != 0);

        NativeRequest result{};
        VERIFY_IS_TRUE(TryReadEnginePlacement(engine, result));
        VERIFY_IS_TRUE(result.ShowCommand == NativeShowCommand::NoChange);
        VERIFY_IS_TRUE(result.Flags.KeepHidden && result.Flags.NoActivate);
        VERIFY_IS_FALSE(result.Flags.AllowSizing);
        VERIFY_IS_TRUE(result.Flags.UseVirtualDesktopId);
        VERIFY_IS_TRUE(result.VirtualDesktopId.has_value());
        VERIFY_IS_TRUE(IsEqualGUID(*request.VirtualDesktopId, *result.VirtualDesktopId) != 0);
    }

    TEST_METHOD(ObservesEngineMonitorAndSnapAdjustment)
    {
        auto request = Request();
        request.Flags.Arranged = true;
        request.ArrangeRect = Rect{-1920, 0, 960, 1040};
        PlacementEx engine{};
        VERIFY_IS_TRUE(TryCreateEnginePlacement(request, engine));

        // Synthetic monitor data goes directly to the imported geometry engine.
        MonitorData target{};
        target.workArea = {0, 40, 2560, 1480};
        target.dpi = 144;
        VERIFY_SUCCEEDED(StringCchCopy(target.deviceName, ARRAYSIZE(target.deviceName), L"Target"));
        engine.MoveToMonitor(target);

        NativeRequest result{};
        VERIFY_IS_TRUE(TryReadEnginePlacement(engine, result));
        VerifyRect(result.NormalRect, engine.normalRect);
        VerifyRect(result.WorkArea, target.workArea);
        VERIFY_IS_TRUE(result.ArrangeRect.has_value());
        VerifyRect(*result.ArrangeRect, engine.arrangeRect);
        VERIFY_ARE_EQUAL(1200, result.NormalRect.Width);
        VERIFY_ARE_EQUAL(900, result.NormalRect.Height);
        VERIFY_ARE_EQUAL(1280, result.ArrangeRect->Width);
        VERIFY_ARE_EQUAL(1440, result.ArrangeRect->Height);
        VERIFY_ARE_EQUAL(144, result.Dpi);
        VERIFY_IS_TRUE(result.DeviceName == u"Target");
        VERIFY_IS_TRUE(result.Flags.Arranged);
        VERIFY_IS_TRUE(request.DeviceName == Request().DeviceName);
        VERIFY_ARE_EQUAL(800, request.NormalRect.Width);
    }

    TEST_METHOD(ObservesEngineFittingAndCascade)
    {
        auto request = Request();
        request.NormalRect = {-810, 430, 800, 600};
        PlacementEx engine{};
        VERIFY_IS_TRUE(TryCreateEnginePlacement(request, engine));
        engine.Cascade();

        NativeRequest result{};
        VERIFY_IS_TRUE(TryReadEnginePlacement(engine, result));
        VerifyRect(result.NormalRect, engine.normalRect);
        VERIFY_ARE_EQUAL(800, result.NormalRect.Width);
        VERIFY_ARE_EQUAL(600, result.NormalRect.Height);
        VERIFY_IS_TRUE(result.NormalRect.X >= request.WorkArea.X);
        VERIFY_IS_TRUE(result.NormalRect.Y >= request.WorkArea.Y);

        engine.normalRect = {-100, 100, 700, 700};
        engine.normalRect = PlacementEx::KeepRectOnMonitor(engine.normalRect, engine.workArea);
        VERIFY_IS_TRUE(TryReadEnginePlacement(engine, result));
        VerifyRect(result.NormalRect, engine.normalRect);
        VERIFY_ARE_EQUAL(-800, result.NormalRect.X);
    }

    TEST_METHOD(CascadeUsesInjectedOffsetAndRejectsNativeOverflow)
    {
        auto request = Request();
        request.NormalRect = {-1800, 100, 800, 600};
        PlacementEx engine{};
        VERIFY_IS_TRUE(TryCreateEnginePlacement(request, engine));

        VERIFY_IS_TRUE(TryCascadeEngine(engine, 37));
        VERIFY_ARE_EQUAL(-1763, engine.normalRect.left);
        VERIFY_ARE_EQUAL(137, engine.normalRect.top);

        request.NormalRect = {(std::numeric_limits<int32_t>::max)() - 1, 100, 1, 1};
        request.WorkArea = {(std::numeric_limits<int32_t>::max)() - 1000, 0, 1000, 1000};
        VERIFY_IS_TRUE(TryCreateEnginePlacement(request, engine));
        VERIFY_IS_FALSE(TryCascadeEngine(engine, 1));
    }

    TEST_METHOD(RejectsInvalidRequestGeometryWithoutPublishing)
    {
        auto request = Request();
        request.NormalRect.Width = 0;
        VerifyRejected(request);
        request = Request();
        request.NormalRect.X = (std::numeric_limits<int32_t>::max)();
        VerifyRejected(request);
        request = Request();
        request.WorkArea.Height = -1;
        VerifyRejected(request);
        request = Request();
        request.Dpi = 95;
        VerifyRejected(request);
        request = Request();
        request.NormalRect.X = 2000;
        VerifyRejected(request);
        request = Request();
        request.DeviceName.assign(32, u'x');
        VerifyRejected(request);
        request.DeviceName = std::u16string(u"a\0b", 3);
        VerifyRejected(request);
    }

    TEST_METHOD(RejectsInconsistentStateAndOptionalMetadata)
    {
        auto request = Request();
        request.Flags.Arranged = true;
        VerifyRejected(request);
        request = Request();
        request.ArrangeRect = Rect{-1920, 0, 960, 1040};
        VerifyRejected(request);
        request = Request();
        request.Flags.RestoreToMaximized = true;
        VerifyRejected(request);
        request = Request();
        request.ShowCommand = NativeShowCommand::Minimize;
        request.Flags.RestoreToMaximized = true;
        request.Flags.RestoreToArranged = true;
        request.ArrangeRect = Rect{-1920, 0, 960, 1040};
        VerifyRejected(request);
        request = Request();
        request.Flags.UseVirtualDesktopId = true;
        VerifyRejected(request);
        request = Request();
        request.VirtualDesktopId = GUID{};
        VerifyRejected(request);
        request = Request();
        request.ShowCommand = static_cast<NativeShowCommand>(99);
        VerifyRejected(request);
    }

    TEST_METHOD(RejectsUnsafeHiddenValueCombinations)
    {
        auto request = Request();
        request.ShowCommand = NativeShowCommand::NoChange;
        VerifyRejected(request);
        request.Flags.KeepHidden = true;
        VerifyRejected(request);
        request.Flags.NoActivate = true;
        request.ShowCommand = NativeShowCommand::Maximize;
        VerifyRejected(request);
        request.ShowCommand = NativeShowCommand::Minimize;
        VerifyRejected(request);
        request.ShowCommand = NativeShowCommand::NoChange;
        request.Flags.Arranged = true;
        request.ArrangeRect = Rect{-1920, 0, 960, 1040};
        VerifyRejected(request);
    }

    TEST_METHOD(RejectsUnrepresentableEngineValuesWithoutPublishing)
    {
        PlacementEx valid{};
        VERIFY_IS_TRUE(TryCreateEnginePlacement(Request(), valid));
        auto invalid = valid;
        invalid.normalRect = {(std::numeric_limits<LONG>::min)(), 0,
            (std::numeric_limits<LONG>::max)(), 100};
        VerifyRejected(invalid);
        invalid = valid;
        invalid.dpi = (std::numeric_limits<UINT>::max)();
        VerifyRejected(invalid);
        invalid = valid;
        std::fill(std::begin(invalid.deviceName), std::end(invalid.deviceName), L'x');
        VerifyRejected(invalid);
        invalid = valid;
        invalid.flags |= PlacementFlags::FullScreen;
        VerifyRejected(invalid);
        invalid = valid;
        invalid.flags |= PlacementFlags::AllowPartiallyOffScreen;
        VerifyRejected(invalid);
        invalid = valid;
        invalid.showCmd = 99;
        VerifyRejected(invalid);
        invalid = valid;
        invalid.flags |= PlacementFlags::Arranged;
        VerifyRejected(invalid);
    }

};
