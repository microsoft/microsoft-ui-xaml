// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ScrollingEdgeScrollParameters.h"

winrt::Point ScrollingEdgeScrollParameters::PointerPositionAdjustment() const
{
    return m_pointerPositionAdjustment;
}

void ScrollingEdgeScrollParameters::PointerPositionAdjustment(winrt::Point const& pointerPositionAdjustment)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::PointToString(pointerPositionAdjustment).c_str());

    m_pointerPositionAdjustment = pointerPositionAdjustment;
}

double ScrollingEdgeScrollParameters::NearEdgeApplicableRange() const
{
    return m_nearEdgeApplicableRange;
}

void ScrollingEdgeScrollParameters::NearEdgeApplicableRange(double nearEdgeApplicableRange)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, nearEdgeApplicableRange);

    if (nearEdgeApplicableRange < 0)
    {
        throw winrt::hresult_error(E_INVALIDARG, s_negativeEdgeApplicableRange);
    }

    m_nearEdgeApplicableRange = nearEdgeApplicableRange;
}

double ScrollingEdgeScrollParameters::FarEdgeApplicableRange() const
{
    return m_farEdgeApplicableRange;
}

void ScrollingEdgeScrollParameters::FarEdgeApplicableRange(double farEdgeApplicableRange)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, farEdgeApplicableRange);

    if (farEdgeApplicableRange < 0)
    {
        throw winrt::hresult_error(E_INVALIDARG, s_negativeEdgeApplicableRange);
    }

    m_farEdgeApplicableRange = farEdgeApplicableRange;
}

float ScrollingEdgeScrollParameters::NearEdgeVelocity() const
{
    return m_nearEdgeVelocity;
}

void ScrollingEdgeScrollParameters::NearEdgeVelocity(float nearEdgeVelocity)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, nearEdgeVelocity);

    if (nearEdgeVelocity != 0.0f && nearEdgeVelocity >= -c_minImpulseOffsetVelocity && nearEdgeVelocity <= c_minImpulseOffsetVelocity)
    {
        throw winrt::hresult_error(E_INVALIDARG, s_smallEdgeVelocity);
    }

    m_nearEdgeVelocity = nearEdgeVelocity;
}

float ScrollingEdgeScrollParameters::FarEdgeVelocity() const
{
    return m_farEdgeVelocity;
}

void ScrollingEdgeScrollParameters::FarEdgeVelocity(float farEdgeVelocity)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, farEdgeVelocity);

    if (farEdgeVelocity != 0.0f && farEdgeVelocity >= -c_minImpulseOffsetVelocity && farEdgeVelocity <= c_minImpulseOffsetVelocity)
    {
        throw winrt::hresult_error(E_INVALIDARG, s_smallEdgeVelocity);
    }

    m_farEdgeVelocity = farEdgeVelocity;
}
