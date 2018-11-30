// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"

#include "ScrollerChangeZoomFactorWithAdditionalVelocityOptions.g.h"

class ScrollerChangeZoomFactorWithAdditionalVelocityOptions :
    public winrt::implementation::ScrollerChangeZoomFactorWithAdditionalVelocityOptionsT<ScrollerChangeZoomFactorWithAdditionalVelocityOptions>
{
public:
    ~ScrollerChangeZoomFactorWithAdditionalVelocityOptions()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollerChangeZoomFactorWithAdditionalVelocityOptions(
        float additionalVelocity,
        winrt::IReference<float> inertiaDecayRate,
        winrt::float2 centerPoint);

    float AdditionalVelocity();
    void AdditionalVelocity(float additionalVelocity);

    winrt::IReference<float> InertiaDecayRate();
    void InertiaDecayRate(winrt::IReference<float> const& inertiaDecayRate);

    winrt::float2 CenterPoint();
    void CenterPoint(winrt::float2 const& centerPoint);

private:
    float m_additionalVelocity{};
    winrt::IReference<float> m_inertiaDecayRate{};
    winrt::float2 m_centerPoint{};
};
