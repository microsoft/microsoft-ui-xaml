// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingEdgeScrollParameters.g.h"

class ScrollingEdgeScrollParameters :
    public winrt::implementation::ScrollingEdgeScrollParametersT<ScrollingEdgeScrollParameters>
{
public:
    ScrollingEdgeScrollParameters()
    {
        SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ScrollingEdgeScrollParameters()
    {
        SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

#pragma region IScrollingEdgeScrollParameters
    winrt::Point PointerPositionAdjustment() const;
    void PointerPositionAdjustment(winrt::Point const& pointerPositionAdjustment);

    double NearEdgeApplicableRange() const;
    void NearEdgeApplicableRange(double nearEdgeApplicableRange);

    double FarEdgeApplicableRange() const;
    void FarEdgeApplicableRange(double farEdgeApplicableRange);

    float NearEdgeVelocity() const;
    void NearEdgeVelocity(float nearEdgeVelocity);

    float FarEdgeVelocity() const;
    void FarEdgeVelocity(float farEdgeVelocity);
#pragma endregion

private:
    static constexpr std::wstring_view s_negativeEdgeApplicableRange{ L"Edge scrolling applicable range must be positive."sv };
    static constexpr std::wstring_view s_smallEdgeVelocity{ L"Edge velocity must be 0 or have an absolute value greater than 30."sv };

    // Any offset impulse velocity smaller than or equal to 30 has no effect on InteractionTracker.
    static constexpr float c_minImpulseOffsetVelocity = 30.0f;

    winrt::Point m_pointerPositionAdjustment{};
    double m_nearEdgeApplicableRange{};
    double m_farEdgeApplicableRange{};
    float m_nearEdgeVelocity{};
    float m_farEdgeVelocity{};
};
