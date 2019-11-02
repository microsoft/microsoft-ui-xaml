// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
//#include "TypeLogging.h"
//#include "ScrollingPresenterTypeLogging.h"
#include "ScrollingEdgeScrollParameters.h"

ScrollingEdgeScrollParameters::ScrollingEdgeScrollParameters(
    const winrt::ScrollingPresenter& scrollingPresenter)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, scrollingPresenter);

    m_scrollingPresenter.set(scrollingPresenter);
}

winrt::Point ScrollingEdgeScrollParameters::PointerPositionAdjustment() const
{
    return winrt::Point{};
}

void ScrollingEdgeScrollParameters::PointerPositionAdjustment(winrt::Point const& pointerPositionAdjustment)
{

}

double ScrollingEdgeScrollParameters::NearEdgeApplicableRange() const
{
    return 0.0;
}

void ScrollingEdgeScrollParameters::NearEdgeApplicableRange(double nearEdgeApplicableRange)
{

}

double ScrollingEdgeScrollParameters::FarEdgeApplicableRange() const
{
    return 0.0;
}

void ScrollingEdgeScrollParameters::FarEdgeApplicableRange(double farEdgeApplicableRange)
{

}

float ScrollingEdgeScrollParameters::NearEdgeVelocity() const
{
    return 0.0f;
}

void ScrollingEdgeScrollParameters::NearEdgeVelocity(float nearEdgeVelocity)
{

}

float ScrollingEdgeScrollParameters::FarEdgeVelocity() const
{
    return 0.0f;
}

void ScrollingEdgeScrollParameters::FarEdgeVelocity(float farEdgeVelocity)
{

}
