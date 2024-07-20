// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PowerModeRequestor.h"

void
CPowerModeRequestor::DisablePowerSaving()
{
    EnsurePowerRequestHandle();

    if (m_isPowerSavingEnabled &&
        m_hPowerRequest.get() != INVALID_HANDLE_VALUE)
    {
        // We need to make requests with both PowerRequestDisplayRequired and  PowerRequestSystemRequired
        // request types to ensure proper behavior on desktops and tablets (with Connected Standby support).
        m_isPowerSavingEnabled =
            !(::PowerSetRequest(m_hPowerRequest.get(), PowerRequestDisplayRequired) &&
              ::PowerSetRequest(m_hPowerRequest.get(), PowerRequestSystemRequired));
    }

    ASSERT(!m_isPowerSavingEnabled);
}

void
CPowerModeRequestor::RestorePowerSaving()
{
    EnsurePowerRequestHandle();

    if (!m_isPowerSavingEnabled &&
        m_hPowerRequest.get() != INVALID_HANDLE_VALUE)
    {
        m_isPowerSavingEnabled =
            (::PowerClearRequest(m_hPowerRequest.get(), PowerRequestDisplayRequired) &&
             ::PowerClearRequest(m_hPowerRequest.get(), PowerRequestSystemRequired));
    }

    ASSERT(m_isPowerSavingEnabled);
}


void
CPowerModeRequestor::EnsurePowerRequestHandle()
{
    if (m_hPowerRequest.get() == INVALID_HANDLE_VALUE)
    {
        REASON_CONTEXT reasonContent = {0};
        reasonContent.Version = POWER_REQUEST_CONTEXT_VERSION;
        reasonContent.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
        reasonContent.Reason.SimpleReasonString = const_cast<LPWSTR>(L"Full window media playback");    // CPP COMPLIANCE BUG: 19219332

        m_hPowerRequest.reset(::PowerCreateRequest(&reasonContent));
        if (m_hPowerRequest.get() == INVALID_HANDLE_VALUE)
        {
            TRACE(TraceAlways, L"PowerCreateRequest() failed.");
        }
    }
}

