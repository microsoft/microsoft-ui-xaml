// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DCompAnimationConversionContext.h"
#include "RepeatBehavior.h"
#include "TimeMgr.h"
#include <KeySpline.h>
#include <EasingFunctions.h>

wrl::ComPtr<ixp::IScalarKeyFrameAnimation> CompositionAnimationConversionContext::CreateEmptyScalarAnimation()
{
    wrl::ComPtr<ixp::IScalarKeyFrameAnimation> animation;
    IFCFAILFAST(m_compositor->CreateScalarKeyFrameAnimation(animation.GetAddressOf()));
    return animation;
}

wrl::ComPtr<ixp::IScalarKeyFrameAnimation> CompositionAnimationConversionContext::CreateScalarLinearAnimation(
    float from,
    float to,
    bool isByAnimation,
    float by,
    bool hasHandoff,
    _In_opt_ CEasingFunctionBase* pEasingFunction)
{
    wrl::ComPtr<ixp::IScalarKeyFrameAnimation> animation;
    IFCFAILFAST(m_compositor->CreateScalarKeyFrameAnimation(animation.GetAddressOf()));

    wrl::ComPtr<ixp::ICompositionEasingFunction> easingFunction;

    if (!hasHandoff)
    {
        // If we're pushing some BeginTimes into the animation (because of a repeat or reverse on some outer
        // Storyboard), then the first key time won't be 0.0. If there's no key frame specified at time 0, then
        // Composition will hold the current value until we reach the first key time.
        float normalizedStartTime = m_beginTimeInsideAnimation / GetDurationWithSpeedRatio();
        animation->InsertKeyFrame(normalizedStartTime, from);
    }

    if (pEasingFunction != nullptr)
    {
        pEasingFunction->GetWUCEasingFunction(m_easingFunctionStatics.Get(), m_compositor.Get(), easingFunction.GetAddressOf());
    }
    else
    {
// can linear easing functions be shared?
        wrl::ComPtr<ixp::ILinearEasingFunction> linear;
        IFCFAILFAST(m_easingFunctionStatics->CreateLinearEasingFunction(m_compositor.Get(), linear.GetAddressOf()));
        IFCFAILFAST(linear.As(&easingFunction));
    }

    // If we have a forced duration for repeat/reverse, then the final value might be before normalized time 1.0.
    float normalizedEndTime = m_durationWithSpeedRatio / GetDurationWithSpeedRatio();

    // By with handoff is a special case. Since the ending key frame references the starting key frame, we have to add it
    // as an expression of the form "this.StartingValue + 1234". To animations (handoff or not) and By animations without
    // handoff can use a fixed end.
    if (isByAnimation && hasHandoff)
    {
        wchar_t expression[50] = {};
        swprintf_s(expression, ARRAYSIZE(expression), L"this.StartingValue + %.4f", by);

        wrl::ComPtr<ixp::IKeyFrameAnimation> kfa;
        IFCFAILFAST(animation.As(&kfa));
        IFCFAILFAST(kfa->InsertExpressionKeyFrameWithEasingFunction(normalizedEndTime, wrl_wrappers::HStringReference(expression).Get(), easingFunction.Get()));
    }
    else
    {
        IFCFAILFAST(animation->InsertKeyFrameWithEasingFunction(normalizedEndTime, to, easingFunction.Get()));
    }

    return animation;
}

wrl::ComPtr<ixp::IColorKeyFrameAnimation> CompositionAnimationConversionContext::CreateEmptyColorAnimation()
{
    wrl::ComPtr<ixp::IColorKeyFrameAnimation> animation;
    IFCFAILFAST(m_compositor->CreateColorKeyFrameAnimation(animation.GetAddressOf()));
    IFCFAILFAST(animation->put_InterpolationColorSpace(ixp::CompositionColorSpace_Rgb));
    return animation;
}

wrl::ComPtr<ixp::IColorKeyFrameAnimation> CompositionAnimationConversionContext::CreateColorLinearAnimation(
    wu::Color from,
    wu::Color to,
    // No "by" support - Xaml ColorAnimation supports it but it doesn't make much sense to go 0x20 more opaque and 0x80 redder
    bool hasHandoff,
    _In_opt_ CEasingFunctionBase* pEasingFunction)
{
    wrl::ComPtr<ixp::IColorKeyFrameAnimation> animation;
    IFCFAILFAST(m_compositor->CreateColorKeyFrameAnimation(animation.GetAddressOf()));
    IFCFAILFAST(animation->put_InterpolationColorSpace(ixp::CompositionColorSpace_Rgb));

    wrl::ComPtr<ixp::ICompositionEasingFunction> easingFunction;

    if (!hasHandoff)
    {
        // If we're pushing some BeginTimes into the animation (because of a repeat or reverse on some outer
        // Storyboard), then the first key time won't be 0.0. If there's no key frame specified at time 0, then
        // Composition will hold the current value until we reach the first key time.
        float normalizedStartTime = m_beginTimeInsideAnimation / GetDurationWithSpeedRatio();
        animation->InsertKeyFrame(normalizedStartTime, from);
    }

    if (pEasingFunction != nullptr)
    {
        pEasingFunction->GetWUCEasingFunction(m_easingFunctionStatics.Get(), m_compositor.Get(), easingFunction.GetAddressOf());
    }
    else
    {
// can linear easing functions be shared?
        wrl::ComPtr<ixp::ILinearEasingFunction> linear;
        IFCFAILFAST(m_easingFunctionStatics->CreateLinearEasingFunction(m_compositor.Get(), linear.GetAddressOf()));
        IFCFAILFAST(linear.As(&easingFunction));
    }

    IFCFAILFAST(animation->InsertKeyFrameWithEasingFunction(1.0f, to, easingFunction.Get()));

    return animation;
}

wrl::ComPtr<ixp::ICompositionEasingFunction> CompositionAnimationConversionContext::CreateLinearEasingFunction()
{
    wrl::ComPtr<ixp::ILinearEasingFunction> linear;
    IFCFAILFAST(m_easingFunctionStatics->CreateLinearEasingFunction(m_compositor.Get(), linear.GetAddressOf()));

    wrl::ComPtr<ixp::ICompositionEasingFunction> easingFunction;
    IFCFAILFAST(linear.As(&easingFunction));

    return easingFunction;
}

wrl::ComPtr<ixp::ICompositionEasingFunction> CompositionAnimationConversionContext::CreateSplineEasingFunction(_In_opt_ CKeySpline* keySpline)
{
    wrl::ComPtr<ixp::ICompositionEasingFunction> easingFunction;

    if (keySpline != nullptr)
    {
        wrl::ComPtr<ixp::ICubicBezierEasingFunction> bezier;
        wfn::Vector2 controlPoint1 = keySpline->GetControlPoint1();
        wfn::Vector2 controlPoint2 = keySpline->GetControlPoint2();

        IFCFAILFAST(m_easingFunctionStatics->CreateCubicBezierEasingFunction(
            m_compositor.Get(),
            controlPoint1,
            controlPoint2,
            bezier.ReleaseAndGetAddressOf()));

        IFCFAILFAST(bezier.As(&easingFunction));
    }
    else
    {
        easingFunction = CreateLinearEasingFunction();
    }

    return easingFunction;
}

wrl::ComPtr<ixp::ICompositionEasingFunction> CompositionAnimationConversionContext::CreateDiscreteEasingFunction()
{
    // A Xaml discrete key frame at time T with value X will hold the animation's value at the value of the previous key frame
    // and jump to X at time T. This corresponds to a WUC easing function that finishes instantly.

    wrl::ComPtr<ixp::IStepEasingFunction> stepEasingFunction;
    IFCFAILFAST(m_easingFunctionStatics->CreateStepEasingFunction(m_compositor.Get(), stepEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(stepEasingFunction->put_IsFinalStepSingleFrame(true));

    wrl::ComPtr<ixp::ICompositionEasingFunction> easingFunction;
    IFCFAILFAST(stepEasingFunction.As(&easingFunction));
    return easingFunction;
}

wrl::ComPtr<ixp::ICompositionEasingFunction> CompositionAnimationConversionContext::CreateOtherEasingFunction(_In_opt_ CDependencyObject* xamlEasingFunction)
{
    wrl::ComPtr<ixp::ICompositionEasingFunction> easingFunction;

    if (xamlEasingFunction != nullptr)
    {
        CEasingFunctionBase* pEaseNoRef = static_cast<CEasingFunctionBase*>(xamlEasingFunction);
        pEaseNoRef->GetWUCEasingFunction(m_easingFunctionStatics.Get(), m_compositor.Get(), easingFunction.ReleaseAndGetAddressOf());
    }
    else
    {
        easingFunction = CreateLinearEasingFunction();
    }

    return easingFunction;
}

CompositionAnimationConversionResult CompositionAnimationConversionContext::GetNormalizedKeyTime(
    const float keyTime,
    _Out_ float* pNormalizedKeyTime)
{
    // Initialize the output parameter to a default value
    *pNormalizedKeyTime = 0.0f;
    
    float keyTimeWithSpeedRatio = keyTime / m_speedRatio;
    float normalizedKeyTime = (m_beginTimeInsideAnimation + keyTimeWithSpeedRatio) / GetDurationWithSpeedRatio();

    if (normalizedKeyTime > 1.0f)
    {
        return CompositionAnimationConversionResult::CannotHaveKeyFramesBeyondDuration;
    }
    else if (normalizedKeyTime < 0.0f)
    {
        return CompositionAnimationConversionResult::CannotHaveKeyFramesWithNegativeKeyTime;
    }
    else
    {
        *pNormalizedKeyTime = normalizedKeyTime;
        return CompositionAnimationConversionResult::Success;
    }
}

#pragma endregion DComp animation creation
