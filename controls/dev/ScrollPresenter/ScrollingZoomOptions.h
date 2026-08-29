// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollPresenter.h"
#include "ScrollingZoomOptions.g.h"

class ScrollingZoomOptions :
    public winrt::implementation::ScrollingZoomOptionsT<ScrollingZoomOptions>
{
public:
    ScrollingZoomOptions(winrt::ScrollingAnimationMode const& animationMode);
    ScrollingZoomOptions(winrt::ScrollingAnimationMode const& animationMode, winrt::ScrollingSnapPointsMode const& snapPointsMode);

    ~ScrollingZoomOptions()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    static constexpr winrt::ScrollingAnimationMode s_defaultAnimationMode{ winrt::ScrollingAnimationMode::Auto };
    static constexpr winrt::ScrollingSnapPointsMode s_defaultSnapPointsMode{ winrt::ScrollingSnapPointsMode::Default };

    winrt::ScrollingAnimationMode AnimationMode() const;
    void AnimationMode(winrt::ScrollingAnimationMode const& animationMode);

    winrt::ScrollingSnapPointsMode SnapPointsMode() const;
    void SnapPointsMode(winrt::ScrollingSnapPointsMode const& snapPointsMode);

private:
    winrt::ScrollingAnimationMode m_animationMode{ s_defaultAnimationMode };
    winrt::ScrollingSnapPointsMode m_snapPointsMode{ s_defaultSnapPointsMode };
};

