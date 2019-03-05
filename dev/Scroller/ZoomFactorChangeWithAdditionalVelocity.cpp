// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ZoomFactorChangeWithAdditionalVelocity.h"

ZoomFactorChangeWithAdditionalVelocity::ZoomFactorChangeWithAdditionalVelocity(
    float zoomFactorVelocity,
    winrt::IReference<winrt::float2> centerPoint,
    winrt::IReference<float> inertiaDecayRate) :
        m_zoomFactorVelocity(zoomFactorVelocity),
        m_centerPoint(centerPoint),
        m_inertiaDecayRate(inertiaDecayRate)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(), TypeLogging::NullableFloatToString(inertiaDecayRate).c_str(), zoomFactorVelocity);
}

ZoomFactorChangeWithAdditionalVelocity::~ZoomFactorChangeWithAdditionalVelocity()
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

void ZoomFactorChangeWithAdditionalVelocity::ZoomFactorVelocity(float zoomFactorVelocity)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_FLT, METH_NAME, this, zoomFactorVelocity);

    m_zoomFactorVelocity = zoomFactorVelocity;
}
