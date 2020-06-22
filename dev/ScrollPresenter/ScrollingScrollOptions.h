// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollPresenter.h"
#include "ScrollingScrollOptions.g.h"

class ScrollingScrollOptions :
    public winrt::implementation::ScrollingScrollOptionsT<ScrollingScrollOptions>
{
public:
    ScrollingScrollOptions(winrt::AnimationMode const& animationMode);
    ScrollingScrollOptions(winrt::AnimationMode const& animationMode, winrt::SnapPointsMode const& snapPointsMode);

    ~ScrollingScrollOptions()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    static constexpr winrt::AnimationMode s_defaultAnimationMode{ winrt::AnimationMode::Auto };
    static constexpr winrt::SnapPointsMode s_defaultSnapPointsMode{ winrt::SnapPointsMode::Default };

    winrt::AnimationMode AnimationMode() const;
    void AnimationMode(winrt::AnimationMode const& animationMode);

    winrt::SnapPointsMode SnapPointsMode() const;
    void SnapPointsMode(winrt::SnapPointsMode const& snapPointsMode);

private:
    winrt::AnimationMode m_animationMode{ s_defaultAnimationMode };
    winrt::SnapPointsMode m_snapPointsMode{ s_defaultSnapPointsMode };
};
