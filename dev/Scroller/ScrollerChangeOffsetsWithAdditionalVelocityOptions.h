// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"

#include "ScrollerChangeOffsetsWithAdditionalVelocityOptions.g.h"

class ScrollerChangeOffsetsWithAdditionalVelocityOptions :
    public winrt::implementation::ScrollerChangeOffsetsWithAdditionalVelocityOptionsT<ScrollerChangeOffsetsWithAdditionalVelocityOptions>
{
public:
    ~ScrollerChangeOffsetsWithAdditionalVelocityOptions()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollerChangeOffsetsWithAdditionalVelocityOptions(
        winrt::float2 additionalVelocity,
        winrt::IReference<winrt::float2> inertiaDecayRate);

    winrt::float2 AdditionalVelocity();
    void AdditionalVelocity(winrt::float2 const& additionalVelocity);

    winrt::IReference<winrt::float2> InertiaDecayRate();
    void InertiaDecayRate(winrt::IReference<winrt::float2> const& inertiaDecayRate);

private:
    winrt::float2 m_additionalVelocity{};
    winrt::IReference<winrt::float2> m_inertiaDecayRate{};
};
