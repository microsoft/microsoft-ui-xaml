// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ScrollingScrollOptions.h"

#include "ScrollingScrollOptions.properties.cpp"

ScrollingScrollOptions::ScrollingScrollOptions(
    winrt::AnimationMode const& animationMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str());

    m_animationMode = animationMode;
}

ScrollingScrollOptions::ScrollingScrollOptions(
    winrt::AnimationMode const& animationMode,
    winrt::SnapPointsMode const& snapPointsMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str(),
        TypeLogging::SnapPointsModeToString(snapPointsMode).c_str());

    m_animationMode = animationMode;
    m_snapPointsMode = snapPointsMode;
}

winrt::AnimationMode ScrollingScrollOptions::AnimationMode() const
{
    return m_animationMode;
}

void ScrollingScrollOptions::AnimationMode(winrt::AnimationMode const& animationMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str());

    m_animationMode = animationMode;
}

winrt::SnapPointsMode ScrollingScrollOptions::SnapPointsMode() const
{
    return m_snapPointsMode;
}

void ScrollingScrollOptions::SnapPointsMode(winrt::SnapPointsMode const& snapPointsMode)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::SnapPointsModeToString(snapPointsMode).c_str());

    m_snapPointsMode = snapPointsMode;
}

