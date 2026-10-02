// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowPlacementPublic.h"

#include <string>

namespace DirectUI::WindowPlacementPersistence
{
    enum class CoordinatorOperation
    {
        Show,
        Activate,
        TryApplyInitialPlacement
    };

    struct PlacementPass
    {
        InitialRequest Request;
        std::u16string PlacementId;
        bool AutomaticPersistenceOptIn{false};
        bool FirstDisplay{false};
        bool KeepHidden{false};
        bool PeerPlacement{false};
    };

    class IWindowPlacementCoordinatorHost
    {
    public:
        virtual ~IWindowPlacementCoordinatorHost() = default;

        virtual bool IsClosed() const noexcept = 0;
        virtual bool IsVisible() const noexcept = 0;

        // Returns a current placement from another shown window in the same cascade
        // group. The host owns discovery and must not mutate the peer.
        virtual bool TryGetPlacementPeer(
            const std::u16string&,
            Snapshot&) noexcept
        {
            return false;
        }

        // Called after all input and lifecycle validation. The host owns source
        // selection, policy, and native application for this one pass.
        virtual bool ApplyPlacement(const PlacementPass& pass) = 0;

        // Show and Activate have different native display semantics, so the host is told
        // which one asked. TryApplyInitialPlacement never displays and never calls this.
        virtual bool Display(CoordinatorOperation operation, bool doNotActivate) = 0;

        // Called to capture and save current placement once the coordinator has
        // confirmed enrollment and display. The host takes the current id and
        // Boolean snapshot itself, after close handlers have run, because an
        // enrolled window may save to a different id than it enrolled with.
        // Returns true only for a completed write.
        virtual bool TryCapturePlacementAndSave() noexcept = 0;
    };

    class WindowPlacementCoordinator final
    {
    public:
        explicit WindowPlacementCoordinator(IWindowPlacementCoordinatorHost& host) noexcept;

        bool Show(
            const InitialRequest& request,
            const char16_t* placementId,
            size_t placementIdLength,
            bool automaticPersistenceOptIn);

        bool Activate(
            const char16_t* placementId,
            size_t placementIdLength,
            bool automaticPersistenceOptIn);

        bool TryApplyInitialPlacement(
            const InitialRequest& request,
            const char16_t* placementId,
            size_t placementIdLength,
            bool automaticPersistenceOptIn);

        // Native/AppWindow display bypasses the framework placement pipeline.
        void NotifyNativeDisplayed() noexcept;

        // Handle close event: captures and saves placement for an accepted close.
        // Returns true only when the write completed.
        bool OnAcceptedClose() noexcept;

        // Called when the window is being destroyed but close wasn't handled normally.
        // Best-effort backstop if no save has completed yet.
        void OnDestroy() noexcept;

        // Called for a confirmed session end. This has independent deduplication from
        // normal close/destruction because teardown may still need its own save.
        void OnSessionEnd() noexcept;

        bool IsInitialPlacementPhaseOpen() const noexcept { return m_phaseOpen; }
        bool IsOperationInProgress() const noexcept { return m_inProgress; }
        bool HasEnrollment() const noexcept { return m_hasEnrollment; }
        bool HasEverBeenDisplayed() const noexcept { return m_everDisplayed; }
        const std::u16string& EnrollmentPlacementId() const noexcept { return m_enrollmentId; }
        bool EnrollmentAutomaticPersistenceOptIn() const noexcept
        {
            return m_enrollmentAutomaticPersistenceOptIn;
        }
        bool HasSaveCompleted() const noexcept { return m_saveCompleted; }

        // True once a TryApplyInitialPlacement pass has run on this window.
        bool HasAttemptedHiddenApplication() const noexcept
        {
            return m_hiddenApplicationAttempted;
        }

        // Development diagnostic only. An opted-in first Activate() runs the automatic
        // pass and replaces whatever TryApplyInitialPlacement prepared. This flag records
        // that the diagnostic was reported. It is not an event, not a failure, and not
        // part of the behavioral contract.
        bool DiagnosedActivateAfterHiddenApplication() const noexcept
        {
            return m_activateAfterHiddenApplicationDiagnosed;
        }

    private:
        void ReportActivateAfterHiddenApplication() noexcept;
        bool BeginOperation() noexcept;
        bool BeginDisplayOperation() noexcept;
        bool EndDisplayOperation(CoordinatorOperation operation, bool doNotActivate);
        // Runs the display step of a first-display pass. On an actual display it closes
        // the phase and records enrollment; an abort leaves both untouched.
        bool EndFirstDisplayOperation(
            CoordinatorOperation operation,
            bool doNotActivate,
            std::u16string& placementId,
            bool automaticPersistenceOptIn);
        bool CopyPlacementId(
            const char16_t* placementId,
            size_t placementIdLength,
            std::u16string& result) const noexcept;
        // Show, Activate, and TryApplyInitialPlacement share one peer-source rule, so an
        // opted-in first Activate selects a peer the same way a first Show does.
        // Returns true only when a peer was found and stored in the request.
        bool TryAttachPlacementPeer(
            InitialRequest& request,
            const std::u16string& placementId,
            bool automaticPersistenceOptIn);
        void RecordFirstDisplayEnrollment(
            std::u16string& placementId,
            bool automaticPersistenceOptIn) noexcept;

        IWindowPlacementCoordinatorHost& m_host;
        bool m_phaseOpen{true};
        bool m_inProgress{false};
        bool m_hasEnrollment{false};
        bool m_everDisplayed{false};
        bool m_enrollmentAutomaticPersistenceOptIn{false};
        std::u16string m_enrollmentId;
        bool m_saveCompleted{false};
        bool m_sessionEndSaveCompleted{false};
        bool m_hiddenApplicationAttempted{false};
        bool m_activateAfterHiddenApplicationDiagnosed{false};
    };
}
