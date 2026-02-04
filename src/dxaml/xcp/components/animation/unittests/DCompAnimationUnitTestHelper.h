// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include <vector>

#ifndef NTDDI_WIN11_GE
#define NTDDI_WIN11_GE 0x0A000010
#endif

#include "DCompAnimationConversionContext.h"
#include <EasingFunctions.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

enum WUCEFType
{
    Null,
    Linear,
    CubicSpline
};

class DCompAnimationUnitTestHelper
{
public:
    static void VerifyWithTolerance(
        float expected,
        float tolerance,
        float actual
        );

    // WUC animations have a ton of "derived" interfaces, and none of them are actually derived from each other.
    // We have to manually QI everything before doing a comparison, otherwise the pointers to different interfaces on the same object
    // don't point to the same address.
    static void VerifyWUCAnimationsAreSameObject(
        bool shouldHaveAnimation,
        CDoubleAnimation* animation,
        CDependencyObject* targetObject,
        KnownPropertyIndex targetProperty
        );

#pragma region ::Windows::UI::Composition

    static CompositionAnimationConversionContext MakeCompositionContext();

    static void VerifyWUCKeyFrameAnimation(
        _In_ wrl::ComPtr<WUComp::IKeyFrameAnimation> animation,
        double duration,
        int keyFrameCount,
        double beginTime = 0,
        WUComp::AnimationIterationBehavior iterationBehavior = WUComp::AnimationIterationBehavior_Count,
        float iterationCount = 1.0f,
        bool autoReverse = false);

    static void VerifyWUCKeyFrame(
        _In_ wrl::ComPtr<WUComp::IKeyFrameAnimation> animation,
        float time,
        float value,
        WUCEFType easingFunctionType);

    static void VerifyWUCKeyFrame(
        _In_ wrl::ComPtr<WUComp::IKeyFrameAnimation> animation,
        float time,
        float value,
        WUCEFType easingFunctionType,
        float controlPoint1X,
        float controlPoint1Y,
        float controlPoint2X,
        float controlPoint2Y);

#pragma endregion
};

class TestEasingFunction final : public CEasingFunctionImpl
{
public:
    TestEasingFunction()
        : CEasingFunctionImpl(nullptr)
    {
    }

    float EaseInCore(float normalizedTime) override
    {
        return normalizedTime;
    }
};

} } } } } }
