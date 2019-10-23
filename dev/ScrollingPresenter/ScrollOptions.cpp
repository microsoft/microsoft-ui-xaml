// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollingPresenterTypeLogging.h"
#include "ScrollOptions.h"

CppWinRTActivatableClassWithBasicFactory(ScrollOptions);

ScrollOptions::ScrollOptions(
    winrt::ScrollingAnimationMode const& animationMode)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str());

    m_animationMode = animationMode;
}

ScrollOptions::ScrollOptions(
    winrt::ScrollingAnimationMode const& animationMode,
    winrt::ScrollingSnapPointsMode const& snapPointsMode)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str(),
        TypeLogging::SnapPointsModeToString(snapPointsMode).c_str());

    m_animationMode = animationMode;
    m_snapPointsMode = snapPointsMode;
}

winrt::ScrollingAnimationMode ScrollOptions::AnimationMode() const
{
    return m_animationMode;
}

void ScrollOptions::AnimationMode(winrt::ScrollingAnimationMode const& animationMode)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::AnimationModeToString(animationMode).c_str());

    m_animationMode = animationMode;
}

winrt::ScrollingSnapPointsMode ScrollOptions::SnapPointsMode() const
{
    return m_snapPointsMode;
}

void ScrollOptions::SnapPointsMode(winrt::ScrollingSnapPointsMode const& snapPointsMode)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this,
        TypeLogging::SnapPointsModeToString(snapPointsMode).c_str());

    m_snapPointsMode = snapPointsMode;
}

