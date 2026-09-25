// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include "WindowPlacementApplication.h"
#include "WindowPlacementCoordinator.h"

using namespace DirectUI::WindowPlacementPersistence;

namespace
{
    Snapshot ValidPlacement()
    {
        Snapshot placement;
        placement.NormalRect = {0, 0, 800, 600};
        placement.WorkArea = {0, 0, 1920, 1080};
        placement.Dpi = 96;
        return placement;
    }

    InitialRequest Request(bool skip = false)
    {
        InitialRequest request;
        request.SkipInitialPlacement = skip;
        return request;
    }

    class Host final : public IWindowPlacementCoordinatorHost
    {
    public:
        bool Closed{false};
        bool Visible{false};
        int ApplyCount{0};
        int DisplayCount{0};
        bool ApplyResult{true};
        bool DisplayResult{true};
        bool ReenterOnApply{false};
        bool CloseOnApply{false};
        bool ReenterResult{true};
        int SaveCount{0};
        bool SaveResult{true};
        WindowPlacementCoordinator* Coordinator{nullptr};
        PlacementPass LastPass{};
        CoordinatorOperation LastDisplayOperation{CoordinatorOperation::TryApplyInitialPlacement};
        bool LastDoNotActivate{false};
        bool PeerAvailable{false};
        Snapshot PeerPlacement{};
        int PeerQueryCount{0};

        bool IsClosed() const noexcept override { return Closed; }
        bool IsVisible() const noexcept override { return Visible; }

        bool TryGetPlacementPeer(
            const std::u16string& placementId,
            Snapshot& placement) noexcept override
        {
            ++PeerQueryCount;
            if (!PeerAvailable || placementId.empty()) return false;
            placement = PeerPlacement;
            return true;
        }

        bool ApplyPlacement(const PlacementPass& pass) override
        {
            ++ApplyCount;
            LastPass = pass;
            if (CloseOnApply) Closed = true;
            if (ReenterOnApply)
            {
                ReenterResult = Coordinator->TryApplyInitialPlacement(
                    Request(), u"reentrant", 10, true);
            }
            return ApplyResult;
        }

        bool Display(CoordinatorOperation operation, bool doNotActivate) override
        {
            ++DisplayCount;
            LastDisplayOperation = operation;
            LastDoNotActivate = doNotActivate;
            if (Closed) return false;
            if (DisplayResult) Visible = true;
            return DisplayResult;
        }

        bool TryCapturePlacementAndSave() noexcept override
        {
            ++SaveCount;
            return SaveResult;
        }
    };
}

class WindowPlacementCoordinatorTests
{
public:
    TEST_CLASS(WindowPlacementCoordinatorTests);

    TEST_METHOD(HiddenApplicationsAreRepeatableAndDoNotEnroll)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        host.Coordinator = &coordinator;

        VERIFY_IS_TRUE(coordinator.TryApplyInitialPlacement(Request(), u"one", 3, true));
        VERIFY_IS_TRUE(coordinator.TryApplyInitialPlacement(Request(), u"two", 3, true));
        VERIFY_ARE_EQUAL(2, host.ApplyCount);
        VERIFY_IS_TRUE(coordinator.IsInitialPlacementPhaseOpen());
        VERIFY_IS_FALSE(coordinator.HasEnrollment());
        VERIFY_IS_FALSE(coordinator.HasEverBeenDisplayed());
    }

    TEST_METHOD(PeerPlacementIsMarkedForCascading)
    {
        Host host;
        host.PeerAvailable = true;
        host.PeerPlacement = ValidPlacement();
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"group", 5, true));
        VERIFY_IS_TRUE(host.LastPass.PeerPlacement);
    }

    TEST_METHOD(HiddenPeerPlacementIsMarkedForCascading)
    {
        Host host;
        host.PeerAvailable = true;
        host.PeerPlacement = ValidPlacement();
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.TryApplyInitialPlacement(Request(), u"group", 5, true));
        VERIFY_IS_TRUE(host.LastPass.PeerPlacement);
    }

    TEST_METHOD(SkipFirstShowDoesNotRunPlacement)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(true), u"main", 4, true));
        VERIFY_ARE_EQUAL(0, host.ApplyCount);
        VERIFY_ARE_EQUAL(1, host.DisplayCount);
        VERIFY_IS_FALSE(coordinator.IsInitialPlacementPhaseOpen());
        VERIFY_IS_TRUE(coordinator.HasEnrollment());
    }

    TEST_METHOD(SkipRejectsPlacementAndHiddenApplication)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        auto invalid = Request(true);
        invalid.Placement = ValidPlacement();

        VERIFY_IS_FALSE(coordinator.Show(invalid, u"main", 4, true));
        VERIFY_IS_FALSE(coordinator.TryApplyInitialPlacement(Request(true), u"main", 4, true));
        VERIFY_ARE_EQUAL(0, host.ApplyCount);
        VERIFY_ARE_EQUAL(0, host.DisplayCount);
        VERIFY_IS_TRUE(coordinator.IsInitialPlacementPhaseOpen());
    }

    TEST_METHOD(FirstActivateRunsAutomaticPlacement)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.TryApplyInitialPlacement(Request(), u"main", 4, true));
        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, true));
        VERIFY_ARE_EQUAL(2, host.ApplyCount);
        VERIFY_ARE_EQUAL(1, host.DisplayCount);
        VERIFY_IS_TRUE(host.LastPass.FirstDisplay);
        VERIFY_IS_FALSE(host.LastPass.KeepHidden);
        VERIFY_IS_TRUE(coordinator.DiagnosedActivateAfterHiddenApplication());
    }

    TEST_METHOD(ActivateDiagnosticIsReportedOnceAndChangesNothing)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_FALSE(coordinator.HasAttemptedHiddenApplication());
        VERIFY_IS_TRUE(coordinator.TryApplyInitialPlacement(Request(), u"main", 4, true));
        VERIFY_IS_TRUE(coordinator.HasAttemptedHiddenApplication());
        VERIFY_IS_FALSE(coordinator.DiagnosedActivateAfterHiddenApplication());

        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, true));
        VERIFY_IS_TRUE(coordinator.DiagnosedActivateAfterHiddenApplication());
        VERIFY_IS_TRUE(coordinator.HasEnrollment());
        VERIFY_IS_FALSE(coordinator.IsInitialPlacementPhaseOpen());

        // A later Activate() is outside the first display phase and reports nothing new.
        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, true));
        VERIFY_ARE_EQUAL(2, host.ApplyCount);
        VERIFY_ARE_EQUAL(2, host.DisplayCount);
    }

    TEST_METHOD(ActivateDiagnosticIsNotReportedWithoutOptIn)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.TryApplyInitialPlacement(Request(), u"main", 4, false));
        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, false));
        VERIFY_IS_FALSE(coordinator.DiagnosedActivateAfterHiddenApplication());
    }

    TEST_METHOD(ActivateDiagnosticIsNotReportedWithoutHiddenApplication)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, true));
        VERIFY_IS_FALSE(coordinator.DiagnosedActivateAfterHiddenApplication());
    }

    TEST_METHOD(ActivateDiagnosticIsNotReportedAfterFirstShow)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_IS_FALSE(coordinator.HasAttemptedHiddenApplication());
        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, true));
        VERIFY_IS_FALSE(coordinator.DiagnosedActivateAfterHiddenApplication());
    }

    TEST_METHOD(ActivateDiagnosticIsNotReportedForARejectedActivate)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.TryApplyInitialPlacement(Request(), u"main", 4, true));
        host.Closed = true;
        VERIFY_IS_FALSE(coordinator.Activate(u"main", 4, true));
        VERIFY_IS_FALSE(coordinator.DiagnosedActivateAfterHiddenApplication());
        VERIFY_ARE_EQUAL(1, host.ApplyCount);
    }

    TEST_METHOD(AutomaticShowUsesLivePeerBeforeStorage)
    {
        Host host;
        host.PeerAvailable = true;
        host.PeerPlacement = ValidPlacement();
        host.PeerPlacement.NormalRect.X = 100;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_IS_TRUE(host.LastPass.FirstDisplay);
        VERIFY_ARE_EQUAL(4u, host.LastPass.PlacementId.size());
        VERIFY_ARE_EQUAL(1, host.PeerQueryCount);
    }

    TEST_METHOD(AutomaticActivateUsesLivePeerBeforeStorage)
    {
        Host host;
        host.PeerAvailable = true;
        host.PeerPlacement = ValidPlacement();
        host.PeerPlacement.NormalRect.X = 100;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, true));
        VERIFY_ARE_EQUAL(1, host.PeerQueryCount);
        VERIFY_IS_TRUE(host.LastPass.PeerPlacement);
        VERIFY_IS_TRUE(host.LastPass.Request.Placement.has_value());
        VERIFY_ARE_EQUAL(100, host.LastPass.Request.Placement->NormalRect.X);
    }

    TEST_METHOD(AutomaticActivateWithoutOptInDoesNotUsePeer)
    {
        Host host;
        host.PeerAvailable = true;
        host.PeerPlacement = ValidPlacement();
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, false));
        VERIFY_ARE_EQUAL(0, host.PeerQueryCount);
        VERIFY_IS_FALSE(host.LastPass.PeerPlacement);
    }

    TEST_METHOD(AutomaticShowWithoutOptInDoesNotUsePeer)
    {
        Host host;
        host.PeerAvailable = true;
        host.PeerPlacement = ValidPlacement();
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, false));
        VERIFY_ARE_EQUAL(4u, host.LastPass.PlacementId.size());
        VERIFY_ARE_EQUAL(0, host.PeerQueryCount);
    }

    TEST_METHOD(EnabledCascadeQueriesPeerWithoutAutomaticPersistence)
    {
        Host host;
        host.PeerAvailable = true;
        host.PeerPlacement = ValidPlacement();
        WindowPlacementCoordinator coordinator(host);
        auto request = Request();
        request.CascadeBehavior = static_cast<int32_t>(CascadeBehavior::Enabled);

        VERIFY_IS_TRUE(coordinator.Show(request, u"main", 4, false));
        VERIFY_ARE_EQUAL(1, host.PeerQueryCount);
    }

    TEST_METHOD(DisabledCascadeDoesNotQueryPeer)
    {
        Host host;
        host.PeerAvailable = true;
        host.PeerPlacement = ValidPlacement();
        WindowPlacementCoordinator coordinator(host);
        auto request = Request();
        request.CascadeBehavior = static_cast<int32_t>(CascadeBehavior::Disabled);

        VERIFY_IS_TRUE(coordinator.Show(request, u"main", 4, true));
        VERIFY_ARE_EQUAL(0, host.PeerQueryCount);
    }

    TEST_METHOD(LaterShowIgnoresInvalidPlacementFields)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));

        host.Visible = false;
        auto invalid = Request();
        invalid.Reason = 99;
        invalid.CascadeBehavior = 99;
        invalid.Placement = ValidPlacement();
        VERIFY_IS_TRUE(coordinator.Show(invalid, u"ignored", 7, false));
        VERIFY_ARE_EQUAL(1, host.ApplyCount);
        VERIFY_ARE_EQUAL(2, host.DisplayCount);
    }

    TEST_METHOD(ReentrantOperationIsRejected)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        host.Coordinator = &coordinator;
        host.ReenterOnApply = true;

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_IS_FALSE(host.ReenterResult);
        VERIFY_ARE_EQUAL(1, host.ApplyCount);
        VERIFY_ARE_EQUAL(1, host.DisplayCount);
    }

    TEST_METHOD(NativeDisplayEndsPhaseWithoutEnrollment)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        coordinator.NotifyNativeDisplayed();

        VERIFY_IS_FALSE(coordinator.IsInitialPlacementPhaseOpen());
        VERIFY_IS_FALSE(coordinator.HasEnrollment());
        VERIFY_IS_TRUE(coordinator.HasEverBeenDisplayed());
        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_ARE_EQUAL(0, host.ApplyCount);
    }

    TEST_METHOD(InvalidInputHasNoSideEffects)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        auto invalid = Request();
        invalid.Reason = 99;

        VERIFY_IS_FALSE(coordinator.Show(invalid, u"main", 4, true));
        VERIFY_ARE_EQUAL(0, host.ApplyCount);
        VERIFY_ARE_EQUAL(0, host.DisplayCount);
        VERIFY_IS_TRUE(coordinator.IsInitialPlacementPhaseOpen());
        VERIFY_IS_FALSE(coordinator.HasEnrollment());
    }

    TEST_METHOD(AbortedFirstShowLeavesPhaseOpenAndRetriesWithFreshSnapshot)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        host.DisplayResult = false;

        VERIFY_IS_FALSE(coordinator.Show(Request(), u"first", 5, true));
        VERIFY_ARE_EQUAL(1, host.ApplyCount);
        VERIFY_ARE_EQUAL(1, host.DisplayCount);
        VERIFY_IS_TRUE(coordinator.IsInitialPlacementPhaseOpen());
        VERIFY_IS_FALSE(coordinator.IsOperationInProgress());
        VERIFY_IS_FALSE(coordinator.HasEnrollment());
        VERIFY_IS_FALSE(coordinator.HasEverBeenDisplayed());

        // The retry is a complete initial pass that uses its own snapshot.
        host.DisplayResult = true;
        VERIFY_IS_TRUE(coordinator.Show(Request(), u"second", 6, true));
        VERIFY_ARE_EQUAL(2, host.ApplyCount);
        VERIFY_ARE_EQUAL(2, host.DisplayCount);
        VERIFY_IS_TRUE(host.LastPass.FirstDisplay);
        VERIFY_IS_FALSE(coordinator.IsInitialPlacementPhaseOpen());
        VERIFY_IS_TRUE(coordinator.HasEnrollment());
        VERIFY_IS_TRUE(coordinator.HasEverBeenDisplayed());
        VERIFY_IS_TRUE(coordinator.EnrollmentPlacementId() == u"second");
    }

    TEST_METHOD(AbortedSkippingFirstShowDoesNotEnroll)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        host.DisplayResult = false;

        VERIFY_IS_FALSE(coordinator.Show(Request(true), u"main", 4, true));
        VERIFY_ARE_EQUAL(0, host.ApplyCount);
        VERIFY_ARE_EQUAL(1, host.DisplayCount);
        VERIFY_IS_TRUE(coordinator.IsInitialPlacementPhaseOpen());
        VERIFY_IS_FALSE(coordinator.HasEnrollment());
    }

    TEST_METHOD(AbortedFirstActivateLeavesPhaseOpen)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        host.DisplayResult = false;

        VERIFY_IS_FALSE(coordinator.Activate(u"main", 4, true));
        VERIFY_IS_TRUE(coordinator.IsInitialPlacementPhaseOpen());
        VERIFY_IS_FALSE(coordinator.HasEnrollment());
        VERIFY_IS_FALSE(coordinator.HasEverBeenDisplayed());

        host.DisplayResult = true;
        VERIFY_IS_TRUE(coordinator.Activate(u"main", 4, true));
        VERIFY_ARE_EQUAL(2, host.ApplyCount);
        VERIFY_IS_TRUE(coordinator.HasEnrollment());
    }

    TEST_METHOD(AbortThenCloseNeverEnrollsOrDisplays)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        host.CloseOnApply = true;

        VERIFY_IS_FALSE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_IS_FALSE(coordinator.HasEverBeenDisplayed());
        VERIFY_IS_FALSE(coordinator.HasEnrollment());

        // A closed window cannot retry, and it never became eligible to save.
        VERIFY_IS_FALSE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_ARE_EQUAL(1, host.ApplyCount);
        VERIFY_IS_FALSE(coordinator.HasEnrollment());
    }

    TEST_METHOD(DisplayAfterFailedPlacementStillEnrollsAndEndsPhase)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        host.ApplyResult = false;

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_IS_FALSE(coordinator.IsInitialPlacementPhaseOpen());
        VERIFY_IS_TRUE(coordinator.HasEnrollment());
        VERIFY_IS_TRUE(coordinator.HasEverBeenDisplayed());
    }

    TEST_METHOD(DisplayWithoutOptInEndsPhaseWithoutEnrollment)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, false));
        VERIFY_IS_FALSE(coordinator.IsInitialPlacementPhaseOpen());
        VERIFY_IS_FALSE(coordinator.HasEnrollment());
        VERIFY_IS_FALSE(coordinator.EnrollmentAutomaticPersistenceOptIn());
    }

    TEST_METHOD(AcceptedCloseSavesEnrolledPlacementOnce)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        VERIFY_IS_TRUE(coordinator.OnAcceptedClose());
        VERIFY_IS_FALSE(coordinator.OnAcceptedClose());
        coordinator.OnDestroy();

        VERIFY_ARE_EQUAL(1, host.SaveCount);
        VERIFY_IS_TRUE(coordinator.HasSaveCompleted());
    }

    TEST_METHOD(FailedCloseSaveIsRetriedByTheDestroyBackstop)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);
        host.SaveResult = false;

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));

        // A failed attempt is not a completed save, so it cannot mask missing data.
        VERIFY_IS_FALSE(coordinator.OnAcceptedClose());
        VERIFY_ARE_EQUAL(1, host.SaveCount);
        VERIFY_IS_FALSE(coordinator.HasSaveCompleted());

        host.SaveResult = true;
        coordinator.OnDestroy();
        VERIFY_ARE_EQUAL(2, host.SaveCount);
        VERIFY_IS_TRUE(coordinator.HasSaveCompleted());
    }

    TEST_METHOD(DestroySavesEnrolledPlacementAsBackstop)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        coordinator.OnDestroy();

        VERIFY_ARE_EQUAL(1, host.SaveCount);
        VERIFY_IS_TRUE(coordinator.HasSaveCompleted());
    }

    TEST_METHOD(SessionEndSavesEnrolledPlacementOnce)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        coordinator.OnSessionEnd();
        coordinator.OnSessionEnd();

        VERIFY_ARE_EQUAL(1, host.SaveCount);
        VERIFY_IS_FALSE(coordinator.HasSaveCompleted());
    }

    TEST_METHOD(SessionEndDoesNotSuppressNormalTeardownSave)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, true));
        coordinator.OnSessionEnd();
        VERIFY_ARE_EQUAL(1, host.SaveCount);

        VERIFY_IS_TRUE(coordinator.OnAcceptedClose());
        VERIFY_ARE_EQUAL(2, host.SaveCount);
        VERIFY_IS_TRUE(coordinator.HasSaveCompleted());
    }

    TEST_METHOD(CloseDoesNotSaveWithoutEnrollment)
    {
        Host host;
        WindowPlacementCoordinator coordinator(host);

        VERIFY_IS_TRUE(coordinator.Show(Request(), u"main", 4, false));
        VERIFY_IS_FALSE(coordinator.OnAcceptedClose());
        coordinator.OnDestroy();

        VERIFY_ARE_EQUAL(0, host.SaveCount);
        VERIFY_IS_FALSE(coordinator.HasSaveCompleted());
    }
};
