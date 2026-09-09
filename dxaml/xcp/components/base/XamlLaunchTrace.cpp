// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <atomic>
#include <XamlLaunchPhase.h>
#include <XamlTelemetry.h>

namespace
{
    std::atomic<uint64_t> s_nextLaunchIdentity{0};

    uint64_t NextLaunchIdentity() noexcept
    {
        return s_nextLaunchIdentity.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    void SetCallbackResult(XamlLaunchObservation& observation, uint32_t result) noexcept
    {
        observation.previousPhase = XamlLaunchPhase::ApplicationInitialization;
        observation.nextPhase = FAILED(static_cast<HRESULT>(result)) ?
            XamlLaunchPhase::Aborted : XamlLaunchPhase::FirstFrameScheduling;
        observation.ordinal = 3;
        observation.frameNumber = 0;
        observation.drawAttemptId = 0;
        observation.result = result;
        if (FAILED(static_cast<HRESULT>(result)))
        {
            observation.flags |= static_cast<uint32_t>(XamlLaunchObservationFlags::CallbackFailed);
        }
    }
}

thread_local XamlLaunchStartupScope* XamlLaunchStartupScope::s_current = nullptr;

XamlLaunchStartupScope::XamlLaunchStartupScope() noexcept
    : m_previous(s_current)
    , m_startupId(NextLaunchIdentity())
{
    s_current = this;
    XamlLaunchObservation observation;
    observation.startupId = m_startupId;
    observation.entryKind = XamlLaunchEntryKind::ApplicationStart;
    observation.nextPhase = XamlLaunchPhase::FrameworkInitialization;
    observation.ordinal = 1;
    XamlTelemetry::LaunchPhaseTransition(observation);
}

XamlLaunchStartupScope::~XamlLaunchStartupScope()
{
    s_current = m_previous;
}

XamlLaunchTrace::XamlLaunchTrace() noexcept
{
    m_observation.coreId = NextLaunchIdentity();
    auto scope = XamlLaunchStartupScope::s_current;
    if (scope && !scope->m_claimed)
    {
        scope->m_claimed = true;
        m_observation.startupId = scope->m_startupId;
        m_observation.entryKind = XamlLaunchEntryKind::ApplicationStart;
    }
    else
    {
        m_observation.startupId = m_observation.coreId;
        m_observation.flags = static_cast<uint32_t>(XamlLaunchObservationFlags::NoApplicationStart);
    }
}

XamlLaunchObservation XamlLaunchTrace::BeginOnLaunched() noexcept
{
    // Warm activations do not start a new core-initialization sequence.
    if (m_callbackStarted)
    {
        return {};
    }

    m_callbackStarted = true;
    WriteBoundary(XamlLaunchPhase::FrameworkInitialization, XamlLaunchPhase::ApplicationInitialization, 2);
    return m_observation;
}

XamlLaunchObservation XamlLaunchTrace::EndOnLaunched(
    const XamlLaunchObservation& callbackStart,
    XamlLaunchTrace* currentCoreTrace,
    uint32_t result) noexcept
{
    if (callbackStart.ordinal == 0)
    {
        return {};
    }

    if (currentCoreTrace && currentCoreTrace->m_observation.coreId == callbackStart.coreId)
    {
        SetCallbackResult(currentCoreTrace->m_observation, result);
        currentCoreTrace->WriteBoundary(
            XamlLaunchPhase::ApplicationInitialization, currentCoreTrace->m_observation.nextPhase, 3, 0, 0, result);
        return currentCoreTrace->m_observation;
    }

    // App code can tear down or replace the core. Retain only value-type identity
    // across the callout, and never attach its return to the replacement core.
    auto observation = callbackStart;
    SetCallbackResult(observation, result);
    observation.flags |= static_cast<uint32_t>(XamlLaunchObservationFlags::CoreUnavailableAtCallbackReturn);
    XamlTelemetry::LaunchPhaseTransition(observation);
    return observation;
}

uint64_t XamlLaunchTrace::BeginDrawAttempt() noexcept
{
    return m_frameCompleted ? 0 : ++m_drawAttemptId;
}

void XamlLaunchTrace::BeginLayout(uint32_t frameNumber, uint64_t drawAttemptId) noexcept
{
    if (m_layoutAttemptId == 0)
    {
        m_layoutAttemptId = drawAttemptId;
        WriteBoundary(XamlLaunchPhase::FirstFrameScheduling, XamlLaunchPhase::InitialLayout, 4, frameNumber, drawAttemptId);
    }
}

void XamlLaunchTrace::BeginProduction(uint32_t frameNumber, uint64_t drawAttemptId) noexcept
{
    if (m_layoutAttemptId != 0 && !m_productionStarted)
    {
        m_productionStarted = true;
        WriteBoundary(XamlLaunchPhase::InitialLayout, XamlLaunchPhase::FirstFrameProduction, 5, frameNumber, drawAttemptId);
    }
}

void XamlLaunchTrace::EndDraw(uint32_t frameNumber, uint64_t drawAttemptId, bool frameDrawn, uint32_t result) noexcept
{
    if (m_productionStarted && !m_frameCompleted && frameDrawn)
    {
        m_frameCompleted = true;
        WriteBoundary(XamlLaunchPhase::FirstFrameProduction, XamlLaunchPhase::Complete, 6, frameNumber, drawAttemptId, result);
    }
}

void XamlLaunchTrace::WriteBoundary(
    XamlLaunchPhase previousPhase,
    XamlLaunchPhase nextPhase,
    uint32_t ordinal,
    uint32_t frameNumber,
    uint64_t drawAttemptId,
    uint32_t result) noexcept
{
    if (ordinal != m_lastOrdinal + 1)
    {
        m_observation.flags |= static_cast<uint32_t>(XamlLaunchObservationFlags::NonCanonicalOrder);
    }
    if (m_layoutAttemptId != 0 && drawAttemptId != 0 && drawAttemptId != m_layoutAttemptId)
    {
        m_observation.flags |= static_cast<uint32_t>(XamlLaunchObservationFlags::MultipleDrawAttempts);
    }

    m_lastOrdinal = ordinal;
    m_observation.previousPhase = previousPhase;
    m_observation.nextPhase = nextPhase;
    m_observation.ordinal = ordinal;
    m_observation.frameNumber = frameNumber;
    m_observation.drawAttemptId = drawAttemptId;
    m_observation.result = result;
    XamlTelemetry::LaunchPhaseTransition(m_observation);
}
