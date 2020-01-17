// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "OffsetsChangeWithAdditionalVelocity.h"

OffsetsChangeWithAdditionalVelocity::OffsetsChangeWithAdditionalVelocity(
    winrt::float2 offsetsVelocity,
    winrt::float2 anticipatedOffsetsChange,
    winrt::IReference<winrt::float2> inertiaDecayRate) :
        m_offsetsVelocity(offsetsVelocity),
        m_anticipatedOffsetsChange(anticipatedOffsetsChange),
        m_inertiaDecayRate(inertiaDecayRate)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR_STR, METH_NAME, this,
        TypeLogging::Float2ToString(offsetsVelocity).c_str(),
        TypeLogging::Float2ToString(anticipatedOffsetsChange).c_str(),
        TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str());
}

OffsetsChangeWithAdditionalVelocity::~OffsetsChangeWithAdditionalVelocity()
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

void OffsetsChangeWithAdditionalVelocity::OffsetsVelocity(winrt::float2 const& offsetsVelocity)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::Float2ToString(offsetsVelocity).c_str());

    m_offsetsVelocity = offsetsVelocity;
}

void OffsetsChangeWithAdditionalVelocity::AnticipatedOffsetsChange(winrt::float2 const& anticipatedOffsetsChange)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::Float2ToString(anticipatedOffsetsChange).c_str());

    m_anticipatedOffsetsChange = anticipatedOffsetsChange;
}

void OffsetsChangeWithAdditionalVelocity::InertiaDecayRate(winrt::IReference<winrt::float2> const& inertiaDecayRate)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::NullableFloat2ToString(inertiaDecayRate).c_str());

    m_inertiaDecayRate = inertiaDecayRate;
}
