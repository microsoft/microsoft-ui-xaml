// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ColorKeyFrame.h"
#include "DCompAnimationConversionContext.h"
#include "KeyTime.h"
#include "KeySpline.h"
#include "Animation.h"
#include "EasingFunctions.h"
#include <ColorUtil.h>

#pragma warning(disable:4244) // 'argument' : conversion from 'type1' to 'type2', possible loss of data

using namespace std::placeholders;

CompositionAnimationConversionResult CLinearColorKeyFrame::AddCompositionKeyFrame(
    _In_ CompositionAnimationConversionContext* context,
    _Inout_ WUComp::IKeyFrameAnimation* animation)
{
    float normalizedKeyTime;
    IFC_ANIMATION(context->GetNormalizedKeyTime(m_keyTime->Value().GetTimeSpanInSec(), &normalizedKeyTime));
    const auto& easingFunction = context->CreateLinearEasingFunction();

    wrl::ComPtr<WUComp::IColorKeyFrameAnimation> colorAnimation;
    animation->QueryInterface(IID_PPV_ARGS(colorAnimation.ReleaseAndGetAddressOf()));

    // Value changes linearly, ending at "m_uValue" at time "m_keyTime".
    wu::Color color = ColorUtils::GetWUColor(m_uValue);
    IFCFAILFAST(colorAnimation->InsertKeyFrameWithEasingFunction(
        normalizedKeyTime,
        color,
        easingFunction.Get()));

    return CompositionAnimationConversionResult::Success;
}

CompositionAnimationConversionResult CDiscreteColorKeyFrame::AddCompositionKeyFrame(
    _In_ CompositionAnimationConversionContext* context,
    _Inout_ WUComp::IKeyFrameAnimation* animation)
{
    float normalizedKeyTime;
    IFC_ANIMATION(context->GetNormalizedKeyTime(m_keyTime->Value().GetTimeSpanInSec(), &normalizedKeyTime));

    // Value stays constant from previous key frame, jumping to "m_rValue" at time "m_keyTime".
    const auto& discreteEasingFunction = context->CreateDiscreteEasingFunction();

    wrl::ComPtr<WUComp::IColorKeyFrameAnimation> colorAnimation;
    animation->QueryInterface(IID_PPV_ARGS(colorAnimation.ReleaseAndGetAddressOf()));

    wu::Color color = ColorUtils::GetWUColor(m_uValue);
    IFCFAILFAST(colorAnimation->InsertKeyFrameWithEasingFunction(
        normalizedKeyTime,
        color,
        discreteEasingFunction.Get()));

    return CompositionAnimationConversionResult::Success;
}

CompositionAnimationConversionResult CSplineColorKeyFrame::AddCompositionKeyFrame(
    _In_ CompositionAnimationConversionContext* context,
    _Inout_ WUComp::IKeyFrameAnimation* animation)
{
    float normalizedKeyTime;
    IFC_ANIMATION(context->GetNormalizedKeyTime(m_keyTime->Value().GetTimeSpanInSec(), &normalizedKeyTime));
    const auto& easingFunction = context->CreateSplineEasingFunction(m_pKeySpline);

    wrl::ComPtr<WUComp::IColorKeyFrameAnimation> colorAnimation;
    animation->QueryInterface(IID_PPV_ARGS(colorAnimation.ReleaseAndGetAddressOf()));

    wu::Color color = ColorUtils::GetWUColor(m_uValue);
    IFCFAILFAST(colorAnimation->InsertKeyFrameWithEasingFunction(
        normalizedKeyTime,
        color,
        easingFunction.Get()));

    return CompositionAnimationConversionResult::Success;
}

float CSplineColorKeyFrame::GetEffectiveProgress(float progress)
{
    if (m_pKeySpline != nullptr)
    {
        return m_pKeySpline->GetSplineProgress(progress);
    }
    else
    {
        return 0;
    }
}

CompositionAnimationConversionResult CEasingColorKeyFrame::AddCompositionKeyFrame(
    _In_ CompositionAnimationConversionContext* context,
    _Inout_ WUComp::IKeyFrameAnimation* animation)
{
    float normalizedKeyTime;
    IFC_ANIMATION(context->GetNormalizedKeyTime(m_keyTime->Value().GetTimeSpanInSec(), &normalizedKeyTime));
    const auto& easingFunction = context->CreateOtherEasingFunction(m_pEasingFunction);

    wrl::ComPtr<WUComp::IColorKeyFrameAnimation> colorAnimation;
    animation->QueryInterface(IID_PPV_ARGS(colorAnimation.ReleaseAndGetAddressOf()));

    wu::Color color = ColorUtils::GetWUColor(m_uValue);
    IFCFAILFAST(colorAnimation->InsertKeyFrameWithEasingFunction(
        normalizedKeyTime,
        color,
        easingFunction.Get()));

    return CompositionAnimationConversionResult::Success;
}

float CEasingColorKeyFrame::GetEffectiveProgress(float progress)
{
    return CEasingFunctionBase::EaseValue(m_pEasingFunction, progress);
}
