// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"
#include "ScrollOptions.g.h"

class ScrollOptions :
    public winrt::implementation::ScrollOptionsT<ScrollOptions>
{
public:
    ~ScrollOptions()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollOptions(winrt::AnimationMode const& animationMode);
    ScrollOptions(winrt::AnimationMode const& animationMode, winrt::SnapPointsMode const& snapPointsMode);

    winrt::AnimationMode AnimationMode();
    void AnimationMode(winrt::AnimationMode const& animationMode);

    winrt::SnapPointsMode SnapPointsMode();
    void SnapPointsMode(winrt::SnapPointsMode const& snapPointsMode);

private:
    winrt::AnimationMode m_animationMode{ winrt::AnimationMode::Enabled };
    winrt::SnapPointsMode m_snapPointsMode{ winrt::SnapPointsMode::Default };
};
