// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChangeBase.h"

class OffsetsChangeWithAdditionalVelocity : public ViewChangeBase
{
public:
    OffsetsChangeWithAdditionalVelocity(
        winrt::float2 offsetsVelocity,
        winrt::IReference<winrt::float2> inertiaDecayRate);
    ~OffsetsChangeWithAdditionalVelocity();

    winrt::float2 GetOffsetsVelocity()
    {
        return m_offsetsVelocity;
    }
    //void OffsetsVelocity(winrt::float2 const& offsetsVelocity);

    winrt::IReference<winrt::float2> GetInertiaDecayRate()
    {
        return safe_cast<winrt::IReference<winrt::float2>>(m_inertiaDecayRate);
    }
    //void InertiaDecayRate(winrt::IReference<winrt::float2> const& inertiaDecayRate);

private:
    winrt::float2 m_offsetsVelocity{};
    winrt::IReference<winrt::float2> m_inertiaDecayRate{};
};

