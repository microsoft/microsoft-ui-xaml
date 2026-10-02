// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowPlacementCoordinator.h"
#include "WindowPlacementPolicy.h"

#include <new>
#include <utility>

#if DBG
#include <Windows.h>
#endif

namespace DirectUI::WindowPlacementPersistence
{
    WindowPlacementCoordinator::WindowPlacementCoordinator(
        IWindowPlacementCoordinatorHost& host) noexcept
        : m_host(host)
    {
    }

    bool WindowPlacementCoordinator::CopyPlacementId(
        const char16_t* placementId,
        size_t placementIdLength,
        std::u16string& result) const noexcept
    {
        if (placementId == nullptr && placementIdLength != 0) return false;

        try
        {
            result.assign(placementId ? placementId : u"", placementIdLength);
            return true;
        }
        catch (const std::bad_alloc&)
        {
            return false;
        }
    }

    bool WindowPlacementCoordinator::TryAttachPlacementPeer(
        InitialRequest& request,
        const std::u16string& placementId,
        bool automaticPersistenceOptIn)
    {
        if (request.Placement ||
            request.CascadeBehavior == static_cast<int32_t>(CascadeBehavior::Disabled) ||
            !IsCascadePermitted(
                static_cast<CascadeBehavior>(request.CascadeBehavior),
                static_cast<PlacementReason>(request.Reason),
                !placementId.empty(),
                automaticPersistenceOptIn,
                false))
        {
            return false;
        }

        Snapshot peer{};
        if (!m_host.TryGetPlacementPeer(placementId, peer)) return false;
        request.Placement = std::move(peer);
        return true;
    }

    void WindowPlacementCoordinator::ReportActivateAfterHiddenApplication() noexcept
    {
        if (m_activateAfterHiddenApplicationDiagnosed) return;
        m_activateAfterHiddenApplicationDiagnosed = true;

#if DBG
        ::OutputDebugStringW(
            L"WinUI window placement: the first Activate() on an opted-in window runs the "
            L"automatic placement pass and replaces the placement that "
            L"TryApplyInitialPlacement prepared. Use Show(WindowShowOptions) with "
            L"SkipInitialPlacement for the first display to keep the prepared placement.\n");
#endif
    }

    bool WindowPlacementCoordinator::BeginOperation() noexcept
    {
        if (m_host.IsClosed() || !m_phaseOpen || m_inProgress) return false;
        m_inProgress = true;
        return true;
    }

    bool WindowPlacementCoordinator::BeginDisplayOperation() noexcept
    {
        if (m_host.IsClosed() || m_inProgress) return false;
        m_inProgress = true;
        return true;
    }

    void WindowPlacementCoordinator::RecordFirstDisplayEnrollment(
        std::u16string& placementId,
        bool automaticPersistenceOptIn) noexcept
    {
        m_phaseOpen = false;
        m_enrollmentAutomaticPersistenceOptIn = automaticPersistenceOptIn;
        m_hasEnrollment = automaticPersistenceOptIn && !placementId.empty();
        if (m_hasEnrollment)
        {
            m_enrollmentId = std::move(placementId);
        }
        else
        {
            m_enrollmentId.clear();
        }
    }

    bool WindowPlacementCoordinator::EndDisplayOperation(
        CoordinatorOperation operation,
        bool doNotActivate)
    {
        const bool displayed = m_host.Display(operation, doNotActivate);
        if (displayed) m_everDisplayed = true;
        m_inProgress = false;
        return displayed;
    }

    bool WindowPlacementCoordinator::EndFirstDisplayOperation(
        CoordinatorOperation operation,
        bool doNotActivate,
        std::u16string& placementId,
        bool automaticPersistenceOptIn)
    {
        const bool displayed = m_host.Display(operation, doNotActivate);

        // Only an actual display closes the initial-placement phase and enrolls.
        // A pre-display abort discards this pass so a surviving hidden window retries.
        if (displayed)
        {
            m_everDisplayed = true;
            RecordFirstDisplayEnrollment(placementId, automaticPersistenceOptIn);
        }

        m_inProgress = false;
        return displayed;
    }

    bool WindowPlacementCoordinator::Show(
        const InitialRequest& request,
        const char16_t* placementId,
        size_t placementIdLength,
        bool automaticPersistenceOptIn)
    {
        if (m_host.IsClosed() || m_inProgress) return false;

        // Once the first-display phase is over, only the display option applies.
        if (!m_phaseOpen)
        {
            if (m_host.IsVisible()) return true;
            if (!BeginDisplayOperation()) return false;
            const bool result =
                EndDisplayOperation(CoordinatorOperation::Show, request.DoNotActivate);
            return result;
        }

        std::u16string id;
        if (!CopyPlacementId(placementId, placementIdLength, id) ||
            !IsValidInitialRequest(request, false))
        {
            return false;
        }

        if (!BeginOperation()) return false;

        const bool doNotActivate = request.DoNotActivate ||
            request.Reason == static_cast<int32_t>(PlacementReason::ApplicationRestart);
        if (request.SkipInitialPlacement)
        {
            return EndFirstDisplayOperation(
                CoordinatorOperation::Show, doNotActivate,
                id, automaticPersistenceOptIn);
        }

        auto requestWithPeer = request;
        const bool peerPlacement =
            TryAttachPlacementPeer(requestWithPeer, id, automaticPersistenceOptIn);

        PlacementPass pass{requestWithPeer, id, automaticPersistenceOptIn, true, false, peerPlacement};
        m_host.ApplyPlacement(pass);
        return EndFirstDisplayOperation(
            CoordinatorOperation::Show, doNotActivate,
            id, automaticPersistenceOptIn);
    }

    bool WindowPlacementCoordinator::Activate(
        const char16_t* placementId,
        size_t placementIdLength,
        bool automaticPersistenceOptIn)
    {
        if (m_host.IsClosed() || m_inProgress) return false;

        if (!m_phaseOpen)
        {
            // Unlike Show, a later Activate is not a no-op on a visible window: it still
            // restores a minimized window and requests activation.
            if (!BeginDisplayOperation()) return false;
            return EndDisplayOperation(CoordinatorOperation::Activate, false);
        }

        std::u16string id;
        if (!CopyPlacementId(placementId, placementIdLength, id)) return false;
        if (!BeginOperation()) return false;

        if (automaticPersistenceOptIn && m_hiddenApplicationAttempted)
        {
            ReportActivateAfterHiddenApplication();
        }

        InitialRequest request{};
        const bool peerPlacement = TryAttachPlacementPeer(request, id, automaticPersistenceOptIn);
        PlacementPass pass{request, id, automaticPersistenceOptIn, true, false, peerPlacement};
        m_host.ApplyPlacement(pass);
        return EndFirstDisplayOperation(
            CoordinatorOperation::Activate, false, id, automaticPersistenceOptIn);
    }

    bool WindowPlacementCoordinator::TryApplyInitialPlacement(
        const InitialRequest& request,
        const char16_t* placementId,
        size_t placementIdLength,
        bool automaticPersistenceOptIn)
    {
        if (m_host.IsClosed() || !m_phaseOpen || m_inProgress || m_host.IsVisible()) return false;

        std::u16string id;
        if (!CopyPlacementId(placementId, placementIdLength, id) ||
            !IsValidInitialRequest(request, true))
        {
            return false;
        }

        if (!BeginOperation()) return false;
        auto requestWithPeer = request;
        const bool peerPlacement =
            TryAttachPlacementPeer(requestWithPeer, id, automaticPersistenceOptIn);

        PlacementPass pass{requestWithPeer, std::move(id), automaticPersistenceOptIn, false, true, peerPlacement};
        const bool result = m_host.ApplyPlacement(pass);
        m_hiddenApplicationAttempted = true;
        m_inProgress = false;
        return result;
    }

    void WindowPlacementCoordinator::NotifyNativeDisplayed() noexcept
    {
        if (m_inProgress) return;
        m_phaseOpen = false;
        m_hasEnrollment = false;
        m_enrollmentAutomaticPersistenceOptIn = false;
        m_enrollmentId.clear();
        m_everDisplayed = true;
    }

    bool WindowPlacementCoordinator::OnAcceptedClose() noexcept
    {
        // Only enrolled windows that have been displayed can save
        if (!m_hasEnrollment || !m_everDisplayed || m_host.IsClosed())
        {
            return false;
        }

        // A completed save is not repeated if close handlers call Close again.
        // A failed or skipped attempt is not a completed save, so it does not
        // block a later attempt or the destruction backstop.
        if (m_saveCompleted)
        {
            return false;
        }

        const bool saved = m_host.TryCapturePlacementAndSave();
        if (saved)
        {
            m_saveCompleted = true;
        }
        return saved;
    }

    void WindowPlacementCoordinator::OnDestroy() noexcept
    {
        // Best-effort backstop if no save has completed yet. This may still fail if
        // capture or storage is no longer available.
        if (m_hasEnrollment && m_everDisplayed && !m_saveCompleted)
        {
            if (m_host.TryCapturePlacementAndSave())
            {
                m_saveCompleted = true;
            }
        }
    }

    void WindowPlacementCoordinator::OnSessionEnd() noexcept
    {
        // Session-end deduplication is independent from normal close/destruction.
        // Shutdown may still need to save after this message, and a failed attempt
        // must remain retryable.
        if (m_hasEnrollment && m_everDisplayed && !m_sessionEndSaveCompleted)
        {
            if (m_host.TryCapturePlacementAndSave())
            {
                m_sessionEndSaveCompleted = true;
            }
        }
    }
}
