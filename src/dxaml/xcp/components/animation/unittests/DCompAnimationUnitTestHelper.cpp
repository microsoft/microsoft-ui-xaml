// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DCompAnimationUnitTestHelper.h"
#include "MockEasingFunctions.h"
#include <DoubleAnimation.h>
#include <CoInitHelper.h>
#include <MockDComp-UnitTestHelpers.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

void DCompAnimationUnitTestHelper::VerifyWithTolerance(
    float expected,
    float tolerance,
    float actual
    )
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);
    VERIFY_IS_GREATER_THAN_OR_EQUAL(expected + tolerance, actual);
    VERIFY_IS_LESS_THAN_OR_EQUAL(expected - tolerance, actual);
}

void DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(
    bool shouldHaveAnimation,
    CDoubleAnimation* animation,
    CDependencyObject* targetObject,
    KnownPropertyIndex targetProperty
    )
{
    const auto& animationOnTarget = targetObject->GetWUCDCompAnimation(targetProperty);

    if (shouldHaveAnimation)
    {
        // Do an explicit QI. Don't trust casting.

        Microsoft::WRL::ComPtr<IUnknown> expected;
        Microsoft::WRL::ComPtr<IUnknown> actual;

        animation->m_spWUCAnimation.As(&expected);
        animationOnTarget->QueryInterface(IID_PPV_ARGS(actual.ReleaseAndGetAddressOf()));

        VERIFY_ARE_EQUAL(expected, actual);
    }
    else
    {
        VERIFY_IS_NULL(animationOnTarget.get());
    }
}

CompositionAnimationConversionContext DCompAnimationUnitTestHelper::MakeCompositionContext()
{
    wrl::ComPtr<WUComp::ICompositor> spMockCompositor;
    VERIFY_SUCCEEDED(MockDComp::CreateMockCompositor(&spMockCompositor));

    wrl::ComPtr<MockEasingFunctionStatics> easingFunctionStatics;
    easingFunctionStatics.Attach(new MockEasingFunctionStatics());

    return CompositionAnimationConversionContext(easingFunctionStatics.Get(), spMockCompositor.Get(), 1.0f);
}

void DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
    _In_ wrl::ComPtr<WUComp::IKeyFrameAnimation> animation,
    double duration,
    int keyFrameCount,
    double beginTime,
    WUComp::AnimationIterationBehavior iterationBehavior,
    float iterationCount,
    bool autoReverse)
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    auto info = MockDComp::GetInfoForKeyFrameAnimation(animation.Get());
    VERIFY_ARE_EQUAL(static_cast<INT64>(duration * 1000 * 1000 * 10), info.m_duration.Duration);
    VERIFY_ARE_EQUAL(static_cast<INT64>(beginTime * 1000 * 1000 * 10), info.m_delayTime.Duration);
    VERIFY_ARE_EQUAL(iterationBehavior, info.m_iterationBehavior);
    if (iterationBehavior == WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count)
    {
        VERIFY_ARE_EQUAL(iterationCount, info.m_iterationCount);
    }
    if (autoReverse)
    {
        VERIFY_ARE_EQUAL(WUComp::AnimationDirection::AnimationDirection_Alternate, info.m_direction);
    }

    VERIFY_ARE_EQUAL(static_cast<size_t>(keyFrameCount), info.m_keyFrameCount);
}

void DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(
    _In_ wrl::ComPtr<WUComp::IKeyFrameAnimation> animation,
    float time,
    float value,
    WUCEFType easingFunctionType)
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    MockDComp::KeyFrameAnimationHelper helper(animation.Get());
    VERIFY_IS_TRUE(helper.HasKeyFrame(time));
    auto keyframe = helper.GetKeyFrame(time);
    VERIFY_ARE_EQUAL(value, keyframe.m_value);

    switch (easingFunctionType)
    {
        case WUCEFType::Null:
        {
            VERIFY_IS_NULL(keyframe.m_easingFunction.Get());
            break;
        }

        case WUCEFType::Linear:
        {
            wrl::ComPtr<WUComp::ILinearEasingFunction> easingFunction;
            keyframe.m_easingFunction.As(&easingFunction);
            VERIFY_IS_NOT_NULL(easingFunction.Get());
            break;
        }

        case WUCEFType::CubicSpline:
        {
            wrl::ComPtr<WUComp::ICubicBezierEasingFunction> easingFunction;
            keyframe.m_easingFunction.As(&easingFunction);
            VERIFY_IS_NOT_NULL(easingFunction.Get());
            break;
        }
    }
}

void DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(
    _In_ wrl::ComPtr<WUComp::IKeyFrameAnimation> animation,
    float time,
    float value,
    WUCEFType easingFunctionType,
    float controlPoint1X,
    float controlPoint1Y,
    float controlPoint2X,
    float controlPoint2Y)
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    VerifyWUCKeyFrame(animation, time, value, easingFunctionType);

    MockDComp::KeyFrameAnimationHelper helper(animation.Get());
    VERIFY_IS_TRUE(helper.HasKeyFrame(time));
    auto keyframe = helper.GetKeyFrame(time);
    VERIFY_IS_NOT_NULL(keyframe.m_easingFunction);

    wrl::ComPtr<WUComp::ICubicBezierEasingFunction> easingFunction;
    keyframe.m_easingFunction.As(&easingFunction);

    wfn::Vector2 controlPoint1;
    wfn::Vector2 controlPoint2;
    IFCFAILFAST(easingFunction->get_ControlPoint1(&controlPoint1));
    IFCFAILFAST(easingFunction->get_ControlPoint2(&controlPoint2));
    VERIFY_ARE_EQUAL(controlPoint1X, controlPoint1.X);
    VERIFY_ARE_EQUAL(controlPoint1Y, controlPoint1.Y);
    VERIFY_ARE_EQUAL(controlPoint2X, controlPoint2.X);
    VERIFY_ARE_EQUAL(controlPoint2Y, controlPoint2.Y);
}

} } } } } }
