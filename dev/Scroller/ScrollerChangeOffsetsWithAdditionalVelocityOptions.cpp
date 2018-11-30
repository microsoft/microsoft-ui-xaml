// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollerChangeOffsetsWithAdditionalVelocityOptions.h"

CppWinRTActivatableClassWithBasicFactory(ScrollerChangeOffsetsWithAdditionalVelocityOptions);

ScrollerChangeOffsetsWithAdditionalVelocityOptions::ScrollerChangeOffsetsWithAdditionalVelocityOptions(
    winrt::float2 additionalVelocity,
    winrt::IReference<winrt::float2> inertiaDecayRate)
{
    if (!inertiaDecayRate)
    {
        SCROLLER_TRACE_VERBOSE(nullptr, L"%s[0x%p](additionalVelocity: (%f, %f), inertiaDecayRate: null)\n",
            METH_NAME, this, additionalVelocity);
    }
    else
    {
        SCROLLER_TRACE_VERBOSE(nullptr, L"%s[0x%p](additionalVelocity: (%f, %f), inertiaDecayRate: (%f, %f))\n",
            METH_NAME, this, additionalVelocity.x, additionalVelocity.y, inertiaDecayRate.Value().x, inertiaDecayRate.Value().y);
    }

    m_additionalVelocity = additionalVelocity;
    m_inertiaDecayRate = inertiaDecayRate;
}

winrt::float2 ScrollerChangeOffsetsWithAdditionalVelocityOptions::AdditionalVelocity()
{
    return m_additionalVelocity;
}

void ScrollerChangeOffsetsWithAdditionalVelocityOptions::AdditionalVelocity(winrt::float2 const& additionalVelocity)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, additionalVelocity.x, additionalVelocity.y);
    m_additionalVelocity = additionalVelocity;
}

winrt::IReference<winrt::float2> ScrollerChangeOffsetsWithAdditionalVelocityOptions::InertiaDecayRate()
{
    return safe_cast<winrt::IReference<winrt::float2>>(m_inertiaDecayRate);
}

void ScrollerChangeOffsetsWithAdditionalVelocityOptions::InertiaDecayRate(winrt::IReference<winrt::float2> const& inertiaDecayRate)
{
    if (!inertiaDecayRate)
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"null");
    }
    else
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, inertiaDecayRate.Value().x, inertiaDecayRate.Value().y);
    }
    m_inertiaDecayRate = inertiaDecayRate;
}
