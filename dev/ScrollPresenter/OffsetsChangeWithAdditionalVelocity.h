// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChangeBase.h"

class OffsetsChangeWithAdditionalVelocity : public ViewChangeBase
{
public:
    OffsetsChangeWithAdditionalVelocity(
        winrt::float2 offsetsVelocity,
        winrt::float2 anticipatedOffsetsChange,
        winrt::IReference<winrt::float2> inertiaDecayRate);
    ~OffsetsChangeWithAdditionalVelocity();

    winrt::float2 OffsetsVelocity() const
    {
        return m_offsetsVelocity;
    }

    void OffsetsVelocity(winrt::float2 const& offsetsVelocity);

    winrt::float2 AnticipatedOffsetsChange() const
    {
        return m_anticipatedOffsetsChange;
    }

    void AnticipatedOffsetsChange(winrt::float2 const& anticipatedOffsetsChange);

    winrt::IReference<winrt::float2> InertiaDecayRate() const
    {
        return m_inertiaDecayRate;
    }

    void InertiaDecayRate(winrt::IReference<winrt::float2> const& inertiaDecayRate);

private:
    winrt::float2 m_offsetsVelocity{};
    winrt::float2 m_anticipatedOffsetsChange{};
    winrt::IReference<winrt::float2> m_inertiaDecayRate{};
};

