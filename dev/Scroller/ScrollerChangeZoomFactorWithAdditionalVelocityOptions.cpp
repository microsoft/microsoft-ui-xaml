// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollerChangeZoomFactorWithAdditionalVelocityOptions.h"

CppWinRTActivatableClassWithBasicFactory(ScrollerChangeZoomFactorWithAdditionalVelocityOptions);

ScrollerChangeZoomFactorWithAdditionalVelocityOptions::ScrollerChangeZoomFactorWithAdditionalVelocityOptions(
    float additionalVelocity,
    winrt::IReference<float> inertiaDecayRate,
    winrt::float2 centerPoint)
{
    if (!inertiaDecayRate)
    {
        SCROLLER_TRACE_VERBOSE(nullptr, L"%s[0x%p](additionalVelocity: %f, inertiaDecayRate: null, centerPoint: (%f, %f))\n",
            METH_NAME, this, additionalVelocity, centerPoint.x, centerPoint.y);
    }
    else
    {
        SCROLLER_TRACE_VERBOSE(nullptr, L"%s[0x%p](additionalVelocity: %f, inertiaDecayRate: %f, centerPoint: (%f, %f))\n",
            METH_NAME, this, additionalVelocity, inertiaDecayRate.Value(), centerPoint.x, centerPoint.y);
    }

    m_additionalVelocity = additionalVelocity;
    m_inertiaDecayRate = inertiaDecayRate;
    m_centerPoint = centerPoint;
}

float ScrollerChangeZoomFactorWithAdditionalVelocityOptions::AdditionalVelocity()
{
    return m_additionalVelocity;
}

void ScrollerChangeZoomFactorWithAdditionalVelocityOptions::AdditionalVelocity(float additionalVelocity)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_FLT, METH_NAME, this, additionalVelocity);
    m_additionalVelocity = additionalVelocity;
}

winrt::IReference<float> ScrollerChangeZoomFactorWithAdditionalVelocityOptions::InertiaDecayRate()
{
    return safe_cast<winrt::IReference<float>>(m_inertiaDecayRate);
}

void ScrollerChangeZoomFactorWithAdditionalVelocityOptions::InertiaDecayRate(winrt::IReference<float> const& inertiaDecayRate)
{
    if (!inertiaDecayRate)
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"null");
    }
    else
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_FLT, METH_NAME, this, inertiaDecayRate.Value());
    }
    m_inertiaDecayRate = inertiaDecayRate;
}

winrt::float2 ScrollerChangeZoomFactorWithAdditionalVelocityOptions::CenterPoint()
{
    return m_centerPoint;
}

void ScrollerChangeZoomFactorWithAdditionalVelocityOptions::CenterPoint(winrt::float2 const& centerPoint)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, centerPoint.x, centerPoint.y);
    m_centerPoint = centerPoint;
}
