// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChangeBase.h"

class ZoomFactorChangeWithAdditionalVelocity : public ViewChangeBase
{
public:
    ZoomFactorChangeWithAdditionalVelocity(
        float zoomFactorVelocity,
        float anticipatedZoomFactorChange,
        winrt::IReference<winrt::float2> centerPoint,
        winrt::IReference<float> inertiaDecayRate);
    ~ZoomFactorChangeWithAdditionalVelocity();

    float ZoomFactorVelocity() const
    {
        return m_zoomFactorVelocity;
    }

    void ZoomFactorVelocity(float zoomFactorVelocity);

    float AnticipatedZoomFactorChange() const
    {
        return m_anticipatedZoomFactorChange;
    }

    void AnticipatedZoomFactorChange(float anticipatedZoomFactorChange);

    winrt::IReference<winrt::float2> CenterPoint() const
    {
        return m_centerPoint;
    }

    winrt::IReference<float> InertiaDecayRate() const
    {
        return m_inertiaDecayRate;
    }

private:
    float m_zoomFactorVelocity{};
    float m_anticipatedZoomFactorChange{};
    winrt::IReference<winrt::float2> m_centerPoint{};
    winrt::IReference<float> m_inertiaDecayRate{};
};

