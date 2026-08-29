// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DoubleKeyFrame.h"
#include "DCompAnimationConversionContext.h"
#include "KeyTime.h"
#include "KeySpline.h"
#include "Animation.h"
#include "EasingFunctions.h"

#pragma warning(disable:4244) // 'argument' : conversion from 'type1' to 'type2', possible loss of data

using namespace std::placeholders;

CompositionAnimationConversionResult CLinearDoubleKeyFrame::AddCompositionKeyFrame(
    _In_ CompositionAnimationConversionContext* context,
    _Inout_ WUComp::IKeyFrameAnimation* animation)
{
    float normalizedKeyTime;
    IFC_ANIMATION(context->GetNormalizedKeyTime(m_keyTime->Value().GetTimeSpanInSec(), &normalizedKeyTime));
    const auto& easingFunction = context->CreateLinearEasingFunction();

    wrl::ComPtr<WUComp::IScalarKeyFrameAnimation> scalarAnimation;
    animation->QueryInterface(IID_PPV_ARGS(scalarAnimation.ReleaseAndGetAddressOf()));

    // Value changes linearly, ending at "m_rValue" at time "m_keyTime".
    IFCFAILFAST(scalarAnimation->InsertKeyFrameWithEasingFunction(
        normalizedKeyTime,
        m_rValue,
        easingFunction.Get()));

    return CompositionAnimationConversionResult::Success;
}

CompositionAnimationConversionResult CDiscreteDoubleKeyFrame::AddCompositionKeyFrame(
    _In_ CompositionAnimationConversionContext* context,
    _Inout_ WUComp::IKeyFrameAnimation* animation)
{
    float normalizedKeyTime;
    IFC_ANIMATION(context->GetNormalizedKeyTime(m_keyTime->Value().GetTimeSpanInSec(), &normalizedKeyTime));

    // Value stays constant from previous key frame, jumping to "m_rValue" at time "m_keyTime".
    const auto& discreteEasingFunction = context->CreateDiscreteEasingFunction();

    wrl::ComPtr<WUComp::IScalarKeyFrameAnimation> scalarAnimation;
    animation->QueryInterface(IID_PPV_ARGS(scalarAnimation.ReleaseAndGetAddressOf()));

    IFCFAILFAST(scalarAnimation->InsertKeyFrameWithEasingFunction(
        normalizedKeyTime,
        m_rValue,
        discreteEasingFunction.Get()));

    return CompositionAnimationConversionResult::Success;
}

CompositionAnimationConversionResult CSplineDoubleKeyFrame::AddCompositionKeyFrame(
    _In_ CompositionAnimationConversionContext* context,
    _Inout_ WUComp::IKeyFrameAnimation* animation)
{
    float normalizedKeyTime;
    IFC_ANIMATION(context->GetNormalizedKeyTime(m_keyTime->Value().GetTimeSpanInSec(), &normalizedKeyTime));
    const auto& easingFunction = context->CreateSplineEasingFunction(m_pKeySpline);

    wrl::ComPtr<WUComp::IScalarKeyFrameAnimation> scalarAnimation;
    animation->QueryInterface(IID_PPV_ARGS(scalarAnimation.ReleaseAndGetAddressOf()));

    IFCFAILFAST(scalarAnimation->InsertKeyFrameWithEasingFunction(
        normalizedKeyTime,
        m_rValue,
        easingFunction.Get()));

    return CompositionAnimationConversionResult::Success;
}

float CSplineDoubleKeyFrame::GetEffectiveProgress(float progress)
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

CompositionAnimationConversionResult CEasingDoubleKeyFrame::AddCompositionKeyFrame(
    _In_ CompositionAnimationConversionContext* context,
    _Inout_ WUComp::IKeyFrameAnimation* animation)
{
    float normalizedKeyTime;
    IFC_ANIMATION(context->GetNormalizedKeyTime(m_keyTime->Value().GetTimeSpanInSec(), &normalizedKeyTime));
    const auto& easingFunction = context->CreateOtherEasingFunction(m_pEasingFunction);

    wrl::ComPtr<WUComp::IScalarKeyFrameAnimation> scalarAnimation;
    animation->QueryInterface(IID_PPV_ARGS(scalarAnimation.ReleaseAndGetAddressOf()));

    IFCFAILFAST(scalarAnimation->InsertKeyFrameWithEasingFunction(
        normalizedKeyTime,
        m_rValue,
        easingFunction.Get()));

    return CompositionAnimationConversionResult::Success;
}

float CEasingDoubleKeyFrame::GetEffectiveProgress(float progress)
{
    return CEasingFunctionBase::EaseValue(m_pEasingFunction, progress);
}

