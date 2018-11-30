// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "Scroller.h"
#include "ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs.h"

CppWinRTActivatableClassWithBasicFactory(ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs);

ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs::ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs(
    float additionalVelocity,
    winrt::IReference<float> inertiaDecayRate)
{
    if (!inertiaDecayRate)
    {
        SCROLLER_TRACE_VERBOSE(nullptr, L"%s[0x%p](additionalVelocity: %f, inertiaDecayRate: null)\n", METH_NAME, this, additionalVelocity);
    }
    else
    {
        SCROLLER_TRACE_VERBOSE(nullptr, L"%s[0x%p](additionalVelocity: %f, inertiaDecayRate: %f)\n", METH_NAME, this, additionalVelocity, inertiaDecayRate.Value());
    }

    m_additionalVelocity = additionalVelocity;
    m_inertiaDecayRate = inertiaDecayRate;
}

float ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs::AdditionalVelocity()
{
    return m_additionalVelocity;
}

winrt::IReference<float> ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs::InertiaDecayRate()
{
    return m_inertiaDecayRate;
}

int32_t ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs::ViewChangeId()
{
    return m_viewChangeId;
}

void ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs::ViewChangeId(int32_t viewChangeId)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, viewChangeId);
    m_viewChangeId = viewChangeId;
}
