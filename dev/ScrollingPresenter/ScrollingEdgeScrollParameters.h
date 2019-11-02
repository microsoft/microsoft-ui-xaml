// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingPresenter.h"
#include "ScrollingEdgeScrollParameters.g.h"

class ScrollingEdgeScrollParameters :
    public ReferenceTracker<ScrollingEdgeScrollParameters, winrt::implementation::ScrollingEdgeScrollParametersT, winrt::composable, winrt::composing>
{
public:
    ScrollingEdgeScrollParameters(const winrt::ScrollingPresenter& scrollingPresenter);

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
    tracker_ref<winrt::ScrollingPresenter> m_scrollingPresenter{ this };
};
