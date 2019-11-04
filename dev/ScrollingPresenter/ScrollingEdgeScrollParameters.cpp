// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollingPresenterTypeLogging.h"
#include "ScrollingEdgeScrollParameters.h"

ScrollingEdgeScrollParameters::ScrollingEdgeScrollParameters(
    /*const winrt::ScrollingPresenter& scrollingPresenter*/)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    //SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, scrollingPresenter);

    //m_scrollingPresenter.set(scrollingPresenter);
}

winrt::Point ScrollingEdgeScrollParameters::PointerPositionAdjustment() const
{
    return m_pointerPositionAdjustment;
}

void ScrollingEdgeScrollParameters::PointerPositionAdjustment(winrt::Point const& pointerPositionAdjustment)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::PointToString(pointerPositionAdjustment).c_str());

    m_pointerPositionAdjustment = pointerPositionAdjustment;
}

double ScrollingEdgeScrollParameters::NearEdgeApplicableRange() const
{
    return m_nearEdgeApplicableRange;
}

void ScrollingEdgeScrollParameters::NearEdgeApplicableRange(double nearEdgeApplicableRange)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, nearEdgeApplicableRange);

    m_nearEdgeApplicableRange = nearEdgeApplicableRange;
}

double ScrollingEdgeScrollParameters::FarEdgeApplicableRange() const
{
    return m_farEdgeApplicableRange;
}

void ScrollingEdgeScrollParameters::FarEdgeApplicableRange(double farEdgeApplicableRange)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, farEdgeApplicableRange);

    m_farEdgeApplicableRange = farEdgeApplicableRange;
}

float ScrollingEdgeScrollParameters::NearEdgeVelocity() const
{
    return m_nearEdgeVelocity;
}

void ScrollingEdgeScrollParameters::NearEdgeVelocity(float nearEdgeVelocity)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, nearEdgeVelocity);

    m_nearEdgeVelocity = nearEdgeVelocity;
}

float ScrollingEdgeScrollParameters::FarEdgeVelocity() const
{
    return m_farEdgeVelocity;
}

void ScrollingEdgeScrollParameters::FarEdgeVelocity(float farEdgeVelocity)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, farEdgeVelocity);

    m_farEdgeVelocity = farEdgeVelocity;
}
