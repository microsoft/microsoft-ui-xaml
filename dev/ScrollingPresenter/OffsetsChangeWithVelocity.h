// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChangeBase.h"

class OffsetsChangeWithVelocity : public ViewChangeBase
{
public:
    OffsetsChangeWithVelocity(
        winrt::float2 offsetsVelocity);
    ~OffsetsChangeWithVelocity();

    winrt::float2 OffsetsVelocity() const
    {
        return m_offsetsVelocity;
    }

    void OffsetsVelocity(winrt::float2 const& offsetsVelocity);

    bool HasCorrection() const
    {
        return m_hasCorrection;
    }

    void HasCorrection(bool hasCorrection)
    {
        m_hasCorrection = hasCorrection;
    }


private:
    winrt::float2 m_offsetsVelocity{};
    bool m_hasCorrection{ false };
};
