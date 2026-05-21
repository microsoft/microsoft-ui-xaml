// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlLogging.h>
#include "DoubleAnimationUsingKeyFramesUnitTests.h"
#include <DCompAnimationUnitTestHelper.h>
#include "DoubleAnimationUsingKeyFrames.h"
#include <Duration.h>
#include "DoubleKeyFrame.h"
#include <KeyTime.h>
#include <KeySpline.h>
#include <RepeatBehavior.h>
#include <CDependencyObject.h>
#include <TypeTableStructs.h>
#include "Timespan.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

void DoubleAnimationUsingKeyFramesUnitTests::ValidateLinearDoubleKeyFrames()
{
    LOG_OUTPUT(L"Linear key frames");

    CDoubleAnimationUsingKeyFrames da1; da1.SetUpForTesting();
    CDuration dur(DirectUI::DurationType::TimeSpan, 3); da1.SetDuration(&dur);
    CDoubleKeyFrameCollection keyFrames; da1.m_pKeyFrames = &keyFrames;

    CLinearDoubleKeyFrame kf1;
    CKeyTime kt1(1); kf1.SetKeyTime(&kt1);
    kf1.m_rValue = 30.0f;
    IFCFAILFAST(da1.m_pKeyFrames->Append(&kf1));

    CLinearDoubleKeyFrame kf2;
    CKeyTime kt2(2); kf2.SetKeyTime(&kt2);
    kf2.m_rValue = 10.0f;
    IFCFAILFAST(da1.m_pKeyFrames->Append(&kf2));

    CLinearDoubleKeyFrame kf3;
    CKeyTime kt3(3); kf3.SetKeyTime(&kt3);
    kf3.m_rValue = 50.0f;
    IFCFAILFAST(da1.m_pKeyFrames->Append(&kf3));

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
    CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 3.0, 4);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f/3.0f, 30.0f, WUCEFType::Linear);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 2.0f/3.0f, 10.0f, WUCEFType::Linear);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 3.0f/3.0f, 50.0f, WUCEFType::Linear);

    // Clear the collection now so it doesn't try to release its items when it gets deleted. Those items are on the stack
    // and have been deleted themselves.
    keyFrames.clear();
}

void DoubleAnimationUsingKeyFramesUnitTests::ValidateLinearDoubleKeyFramesWUC()
{
    ValidateLinearDoubleKeyFrames();
}

void DoubleAnimationUsingKeyFramesUnitTests::ValidateSplineDoubleKeyFrames()
{
    {
        LOG_OUTPUT(L"Spline key frames");

        CDoubleAnimationUsingKeyFrames da1; da1.SetUpForTesting();
        CDuration dur(DirectUI::DurationType::TimeSpan, 3); da1.SetDuration(&dur);
        CDoubleKeyFrameCollection keyFrames; da1.m_pKeyFrames = &keyFrames;

        CSplineDoubleKeyFrame kf1;
        CKeyTime kt1(0.25); kf1.SetKeyTime(&kt1);
        CKeySpline sp1; kf1.m_pKeySpline = &sp1;
        kf1.m_rValue = 30.0f;
        IFCFAILFAST(da1.m_pKeyFrames->Append(&kf1));  // 4 segments from 0 to 30

        CSplineDoubleKeyFrame kf2;
        CKeyTime kt2(1.25); kf2.SetKeyTime(&kt2);
        kf2.m_rValue = 10.0f;
        IFCFAILFAST(da1.m_pKeyFrames->Append(&kf2));  // No spline, this is linear from 30 to 10

        CSplineDoubleKeyFrame kf3;
        CKeyTime kt3(1.5); kf3.SetKeyTime(&kt3);
        CKeySpline sp3; kf3.m_pKeySpline = &sp3;
        kf3.m_rValue = 50.0f;
        IFCFAILFAST(da1.m_pKeyFrames->Append(&kf3));  // 4 segments from 10 to 50

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 3.0, 4);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.25f/3.0f, 30.0f, WUCEFType::CubicSpline, 0.0f, 0.0f, 1.0f, 1.0f);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.25f/3.0f, 10.0f, WUCEFType::Linear);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.5f/3.0f, 50.0f, WUCEFType::CubicSpline, 0.0f, 0.0f, 1.0f, 1.0f);

        // Clear the collection now so it doesn't try to release its items when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        keyFrames.clear();
    }

    {
        LOG_OUTPUT(L"Clipped by duration - error");

        CDoubleAnimationUsingKeyFrames da1; da1.SetUpForTesting();
        CDuration dur(DirectUI::DurationType::TimeSpan, 20.5); da1.SetDuration(&dur);
        CDoubleKeyFrameCollection keyFrames; da1.m_pKeyFrames = &keyFrames;

        CSplineDoubleKeyFrame kf1;
        CKeyTime kt1(50); kf1.SetKeyTime(&kt1);
        CKeySpline sp1; kf1.m_pKeySpline = &sp1;
        kf1.m_rValue = 100.0f;
        IFCFAILFAST(da1.m_pKeyFrames->Append(&kf1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotHaveKeyFramesBeyondDuration, result);

        // Clear the collection now so it doesn't try to release its items when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        keyFrames.clear();
    }
}

void DoubleAnimationUsingKeyFramesUnitTests::ValidateSplineDoubleKeyFramesWUC()
{
    ValidateSplineDoubleKeyFrames();
}

void DoubleAnimationUsingKeyFramesUnitTests::ValidateInferDurationWUC()
{
    LOG_OUTPUT(L"Linear key frames");

    CDoubleAnimationUsingKeyFrames da1; da1.SetUpForTesting();
    // No duration set
    CDoubleKeyFrameCollection keyFrames; da1.m_pKeyFrames = &keyFrames;

    CLinearDoubleKeyFrame kf1;
    CKeyTime kt1(1); kf1.SetKeyTime(&kt1);
    kf1.m_rValue = 30.0f;
    IFCFAILFAST(da1.m_pKeyFrames->Append(&kf1));

    CLinearDoubleKeyFrame kf2;
    CKeyTime kt2(2); kf2.SetKeyTime(&kt2);
    kf2.m_rValue = 10.0f;
    IFCFAILFAST(da1.m_pKeyFrames->Append(&kf2));

    CLinearDoubleKeyFrame kf3;
    CKeyTime kt3(3); kf3.SetKeyTime(&kt3);
    kf3.m_rValue = 50.0f;
    IFCFAILFAST(da1.m_pKeyFrames->Append(&kf3));

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
    CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 3.0, 4);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f/3.0f, 30.0f, WUCEFType::Linear);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 2.0f/3.0f, 10.0f, WUCEFType::Linear);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 3.0f/3.0f, 50.0f, WUCEFType::Linear);

    // Clear the collection now so it doesn't try to release its items when it gets deleted. Those items are on the stack
    // and have been deleted themselves.
    keyFrames.clear();
}

} } } } } }
