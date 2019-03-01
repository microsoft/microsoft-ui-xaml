// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "OffsetsChangeWithAdditionalVelocity.h"

OffsetsChangeWithAdditionalVelocity::OffsetsChangeWithAdditionalVelocity(
    winrt::float2 offsetsVelocity,
    winrt::IReference<winrt::float2> inertiaDecayRate) :
        m_offsetsVelocity(offsetsVelocity),
        m_inertiaDecayRate(inertiaDecayRate)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::Float2ToString(offsetsVelocity).c_str(), TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str());
}

OffsetsChangeWithAdditionalVelocity::~OffsetsChangeWithAdditionalVelocity()
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

void OffsetsChangeWithAdditionalVelocity::OffsetsVelocity(winrt::float2 const& offsetsVelocity)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::Float2ToString(offsetsVelocity).c_str());

    m_offsetsVelocity = offsetsVelocity;
}

void OffsetsChangeWithAdditionalVelocity::InertiaDecayRate(winrt::IReference<winrt::float2> const& inertiaDecayRate)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str());

    m_inertiaDecayRate = inertiaDecayRate;
}
