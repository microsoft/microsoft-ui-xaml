// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include "WindowPlacementCoordinator.h"
#include "WindowPlacementPolicy.h"

#include <vector>

using namespace DirectUI::WindowPlacementPersistence;

// Show and Activate reach the same coordinator but have different native display
// semantics, so the coordinator has to tell the host which operation asked and whether
// that operation is allowed to request activation. These tests cover that routing,
// separately from the phase and enrollment rules in WindowPlacementCoordinatorTests.
namespace
{
    struct DisplayCall
    {
        CoordinatorOperation Operation;
        bool DoNotActivate;
    };

    InitialRequest Request(bool doNotActivate = false, bool skip = false)
    {
        InitialRequest request;
        request.DoNotActivate = doNotActivate;
        request.SkipInitialPlacement = skip;
        return request;
    }

    class RecordingHost final : public IWindowPlacementCoordinatorHost
    {
    public:
        bool Closed{false};
        bool Visible{false};
        bool DisplayResult{true};
        bool ApplyResult{true};
        int ApplyCount{0};
        std::vector<DisplayCall> Displays;
        // Simulates the WM_SHOWWINDOW that a real framework display sends from inside
        // the native display call.
        bool NotifyNativeDuringDisplay{false};
        WindowPlacementCoordinator* Coordinator{nullptr};

        bool IsClosed() const noexcept override { return Closed; }
        bool IsVisible() const noexcept override { return Visible; }

        bool ApplyPlacement(const PlacementPass&) override
        {
            ++ApplyCount;
            return ApplyResult;
        }

        bool Display(CoordinatorOperation operation, bool doNotActivate) override
        {
            if (NotifyNativeDuringDisplay) Coordinator->NotifyNativeDisplayed();
            Displays.push_back({operation, doNotActivate});
            if (DisplayResult) Visible = true;
            return DisplayResult;
        }

        bool TryCapturePlacementAndSave() noexcept override { return true; }
    };
}

class WindowPlacementDisplayRoutingTests
{
public:
    TEST_CLASS(WindowPlacementDisplayRoutingTests);

    TEST_METHOD(FirstShowDisplaysAsShowAndHonorsDoNotActivate)
    {
        RecordingHost host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(true), u"main", 4, true));
        VERIFY_ARE_EQUAL(size_t{1}, host.Displays.size());
        VERIFY_IS_TRUE(CoordinatorOperation::Show == host.Displays[0].Operation);
        VERIFY_IS_TRUE(host.Displays[0].DoNotActivate);
    }

    TEST_METHOD(SkippingFirstShowStillDisplaysAsShow)
    {
        RecordingHost host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(true, true), u"main", 4, true));
        VERIFY_ARE_EQUAL(0, host.ApplyCount);
        VERIFY_ARE_EQUAL(size_t{1}, host.Displays.size());
        VERIFY_IS_TRUE(CoordinatorOperation::Show == host.Displays[0].Operation);
        VERIFY_IS_TRUE(host.Displays[0].DoNotActivate);
    }

    TEST_METHOD(FirstActivateDisplaysAsActivateAndAlwaysRequestsActivation)
    {
        RecordingHost host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, true));
        VERIFY_ARE_EQUAL(size_t{1}, host.Displays.size());
        VERIFY_IS_TRUE(CoordinatorOperation::Activate == host.Displays[0].Operation);
        VERIFY_IS_FALSE(host.Displays[0].DoNotActivate);
    }

    TEST_METHOD(LaterShowOnAHiddenWindowUsesItsOwnDoNotActivate)
    {
        RecordingHost host;
        WindowPlacementCoordinator coordinator(host);
        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));

        host.Visible = false;
        VERIFY_IS_TRUE(coordinator.Show(Request(true), u"main", 4, true));
        VERIFY_ARE_EQUAL(size_t{2}, host.Displays.size());
        VERIFY_IS_TRUE(CoordinatorOperation::Show == host.Displays[1].Operation);
        VERIFY_IS_TRUE(host.Displays[1].DoNotActivate);

        // No second placement pass runs after the phase has ended.
        VERIFY_ARE_EQUAL(1, host.ApplyCount);
    }

    TEST_METHOD(LaterShowOnAVisibleWindowDoesNotDisplayAgain)
    {
        RecordingHost host;
        WindowPlacementCoordinator coordinator(host);
        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_ARE_EQUAL(size_t{1}, host.Displays.size());
    }

    TEST_METHOD(LaterActivateOnAVisibleWindowStillDisplays)
    {
        RecordingHost host;
        WindowPlacementCoordinator coordinator(host);
        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));

        // Activate restores a minimized window and requests activation, so unlike Show
        // it is not a no-op once the window is already visible.
        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, true));
        VERIFY_ARE_EQUAL(size_t{2}, host.Displays.size());
        VERIFY_IS_TRUE(CoordinatorOperation::Activate == host.Displays[1].Operation);
        VERIFY_ARE_EQUAL(1, host.ApplyCount);
    }

    TEST_METHOD(HiddenApplicationNeverDisplays)
    {
        RecordingHost host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.TryApplyInitialPlacement(Request(), u"main", 4, true));
        VERIFY_ARE_EQUAL(1, host.ApplyCount);
        VERIFY_IS_TRUE(host.Displays.empty());
        VERIFY_IS_FALSE(host.Visible);
    }

    TEST_METHOD(RestartSuppressesFirstDisplayActivationEvenWhenSkippingOrApplyingFails)
    {
        for (bool skip : {false, true})
        {
            RecordingHost host;
            host.ApplyResult = false;
            WindowPlacementCoordinator coordinator(host);
            auto request = Request(false, skip);
            request.Reason = static_cast<int32_t>(PlacementReason::ApplicationRestart);
            VERIFY_IS_TRUE(coordinator.Show(request, u"main", 4, true));
            VERIFY_ARE_EQUAL(skip ? 0 : 1, host.ApplyCount);
            VERIFY_IS_TRUE(host.Displays[0].DoNotActivate);
            host.Visible = false;
            // Later Show ignores Reason, including its initial-only restriction.
            VERIFY_IS_TRUE(coordinator.Show(request, u"main", 4, true));
            VERIFY_IS_FALSE(host.Displays[1].DoNotActivate);
        }
    }

    TEST_METHOD(NativeDisplayDuringAPassIsIgnored)
    {
        // A framework display sends WM_SHOWWINDOW from inside Display(), so the native
        // notification must not retire the pass that is running.
        RecordingHost host;
        WindowPlacementCoordinator coordinator(host);
        host.Coordinator = &coordinator;
        host.NotifyNativeDuringDisplay = true;

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_IS_TRUE(coordinator.HasEnrollment());
        VERIFY_IS_TRUE(coordinator.EnrollmentPlacementId() == u"main");
    }

    TEST_METHOD(NativeDisplayBeforeAnyFrameworkCallSuppressesThePlacementPass)
    {
        RecordingHost host;
        WindowPlacementCoordinator coordinator(host);

        coordinator.NotifyNativeDisplayed();
        host.Visible = true;

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_ARE_EQUAL(0, host.ApplyCount);
        VERIFY_IS_TRUE(host.Displays.empty());

        // Activate still reaches the display step, but as a later operation.
        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, true));
        VERIFY_ARE_EQUAL(0, host.ApplyCount);
        VERIFY_ARE_EQUAL(size_t{1}, host.Displays.size());
        VERIFY_IS_TRUE(CoordinatorOperation::Activate == host.Displays[0].Operation);
        VERIFY_IS_FALSE(coordinator.HasEnrollment());
    }

    TEST_METHOD(AbortedDisplayIsRetriedAsAFirstDisplay)
    {
        RecordingHost host;
        WindowPlacementCoordinator coordinator(host);
        host.DisplayResult = false;

        VERIFY_IS_FALSE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_ARE_EQUAL(size_t{1}, host.Displays.size());

        host.DisplayResult = true;
        VERIFY_IS_TRUE(coordinator.Show(Request(true), u"main", 4, true));
        VERIFY_ARE_EQUAL(2, host.ApplyCount);
        VERIFY_ARE_EQUAL(size_t{2}, host.Displays.size());
        VERIFY_IS_TRUE(CoordinatorOperation::Show == host.Displays[1].Operation);
        VERIFY_IS_TRUE(host.Displays[1].DoNotActivate);
    }
};
