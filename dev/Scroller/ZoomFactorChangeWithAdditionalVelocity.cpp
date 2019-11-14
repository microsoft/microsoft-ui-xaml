// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollerTypeLogging.h"
#include "ZoomFactorChangeWithAdditionalVelocity.h"

ZoomFactorChangeWithAdditionalVelocity::ZoomFactorChangeWithAdditionalVelocity(
    float zoomFactorVelocity,
    float anticipatedZoomFactorChange,
    winrt::IReference<winrt::float2> centerPoint,
    winrt::IReference<float> inertiaDecayRate) :
        m_zoomFactorVelocity(zoomFactorVelocity),
        m_anticipatedZoomFactorChange(anticipatedZoomFactorChange),
        m_centerPoint(centerPoint),
        m_inertiaDecayRate(inertiaDecayRate)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_FLT_FLT, METH_NAME, this,
        zoomFactorVelocity, anticipatedZoomFactorChange);
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::NullableFloat2ToString(centerPoint).c_str(), TypeLogging::NullableFloatToString(inertiaDecayRate).c_str());
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

void ZoomFactorChangeWithAdditionalVelocity::AnticipatedZoomFactorChange(float anticipatedZoomFactorChange)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_FLT, METH_NAME, this, anticipatedZoomFactorChange);

    m_anticipatedZoomFactorChange = anticipatedZoomFactorChange;
}
