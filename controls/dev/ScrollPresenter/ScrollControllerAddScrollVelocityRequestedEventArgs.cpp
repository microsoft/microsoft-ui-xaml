// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenter.h"
#include "ScrollControllerAddScrollVelocityRequestedEventArgs.h"

#include "ScrollControllerAddScrollVelocityRequestedEventArgs.properties.cpp"

ScrollControllerAddScrollVelocityRequestedEventArgs::ScrollControllerAddScrollVelocityRequestedEventArgs(
    float offsetVelocity,
    winrt::IReference<float> inertiaDecayRate)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
        TypeLogging::NullableFloatToString(inertiaDecayRate).c_str(), offsetVelocity);

    m_offsetVelocity = offsetVelocity;
    m_inertiaDecayRate = inertiaDecayRate;
}

float ScrollControllerAddScrollVelocityRequestedEventArgs::OffsetVelocity() const
{
    return m_offsetVelocity;
}

winrt::IReference<float> ScrollControllerAddScrollVelocityRequestedEventArgs::InertiaDecayRate() const
{
    return m_inertiaDecayRate;
}

int32_t ScrollControllerAddScrollVelocityRequestedEventArgs::CorrelationId() const
{
    return m_correlationId;
}

void ScrollControllerAddScrollVelocityRequestedEventArgs::CorrelationId(int32_t correlationId)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, correlationId);

    m_correlationId = correlationId;
}
