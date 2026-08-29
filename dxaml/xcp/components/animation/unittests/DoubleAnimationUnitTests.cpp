// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "DoubleAnimationUnitTests.h"
#include "DoubleAnimation.h"
#include "DCompAnimationUnitTestHelper.h"
#include "KeyTime.h"
#include "DoubleKeyFrame.h"
#include "Duration.h"
#include "RepeatBehavior.h"
#include "TranslateTransform.h"
#include "ScaleTransform.h"
#include "RotateTransform.h"
#include "SkewTransform.h"
#include "CompositeTransform.h"
#include "UIElement.h"
#include "PlaneProjection.h"
#include "CompositeTransform3D.h"
#include "Timespan.h"
#include "MockEasingFunctions.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

void DoubleAnimationUnitTests::ValidateDurationWUC()
{
    {
        LOG_OUTPUT(L"Negative - error");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        CDuration dur(DirectUI::DurationType::TimeSpan, -2);
        da1.SetDuration(&dur);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotHaveNegativeTime, result);
    }

    {
        LOG_OUTPUT(L"Positive - okay");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);
    }

    {
        LOG_OUTPUT(L"Small - okay");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        CDuration dur(DirectUI::DurationType::TimeSpan, 0.001);
        da1.SetDuration(&dur);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);
    }

    {
        LOG_OUTPUT(L"Too small - error");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        CDuration dur(DirectUI::DurationType::TimeSpan, 0.0001);
        da1.SetDuration(&dur);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotHaveLessThanMinimumDuration, result);
    }

    {
        LOG_OUTPUT(L"Big - okay");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        CDuration dur(DirectUI::DurationType::TimeSpan, 24 * 24 * 60 * 60);
        da1.SetDuration(&dur);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);
    }

    {
        LOG_OUTPUT(L"Too big - error");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        CDuration dur(DirectUI::DurationType::TimeSpan, 24 * 24 * 60 * 60 + 1);
        da1.SetDuration(&dur);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotExceedMaximumTimeLimit, result);
    }
}

void DoubleAnimationUnitTests::ValidateReverseWUC()
{
    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

    CDoubleAnimation da1; da1.SetUpForTesting();
    da1.m_vBaseValue = 123.0f;
    da1.m_vFrom = 4.0f;
    da1.m_vTo = 10.0f;
    CDuration dur(DirectUI::DurationType::TimeSpan, 2);
    da1.SetDuration(&dur);
    da1.m_fAutoReverse = true;

    CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
        da1.m_spWUCAnimation, 2.0, 2,
        0.0,
        WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,     // 2 iterations - one for forward and one for reverse
        true);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);
}

void DoubleAnimationUnitTests::ValidateRepeatWUC()
{
    {
        LOG_OUTPUT(L"Iterations");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Count, 0, 3.0f);
        da1.SetRepeatBehavior(&rep);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 2.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 3.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);
    }

    {
        LOG_OUTPUT(L"Partial iterations - error");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Count, 0, 3.1f);
        da1.SetRepeatBehavior(&rep);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotHaveFractionalRepeat, result);
    }

    {
        LOG_OUTPUT(L"TimeSpan");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 4.0, 1);
        da1.SetRepeatBehavior(&rep);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 2.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);
    }

    {
        LOG_OUTPUT(L"Partial iterations from TimeSpan - error");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 4.2, 1);
        da1.SetRepeatBehavior(&rep);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotHaveFractionalRepeat, result);
    }

    {
        LOG_OUTPUT(L"Forever");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Forever, 123.321, 10);
        da1.SetRepeatBehavior(&rep);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 2.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Forever, 123.123f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);
    }

    {
        LOG_OUTPUT(L"0 iterations - error");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Count, 0, 0);
        da1.SetRepeatBehavior(&rep);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotHaveNonpositiveRepeat, result);
    }

    {
        LOG_OUTPUT(L"0 time - error");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 0, 0);
        da1.SetRepeatBehavior(&rep);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotHaveNonpositiveRepeat, result);
    }

    {
        LOG_OUTPUT(L"0.5 iterations - error");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Count, 0, 0.5);
        da1.SetRepeatBehavior(&rep);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotHaveFractionalRepeat, result);
    }

    {
        LOG_OUTPUT(L"TimeSpan shorter than natural duration - error");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 1, 0);
        da1.SetRepeatBehavior(&rep);

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotHaveFractionalRepeat, result);
    }
}

void DoubleAnimationUnitTests::ValidateBeginTimeWUC()
{
    {
        LOG_OUTPUT(L"Positive");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        CTimeSpan begin(3.1); da1.m_pBeginTime = &begin;

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 2.0, 2,
            3.1f,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 1.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);
    }

    {
        LOG_OUTPUT(L"Negative - error");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        CTimeSpan begin(-1.1);
        da1.m_pBeginTime = &begin;

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotHaveNegativeTime, result);
    }
}

void DoubleAnimationUnitTests::ValidateSpeedRatioWUC()
{
    {
        LOG_OUTPUT(L"Faster");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        da1.m_rSpeedRatio = 1.5;

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 2.0 / 1.5, 2);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);
    }

    {
        LOG_OUTPUT(L"Slower");
        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

        CDoubleAnimation da1; da1.SetUpForTesting();
        da1.m_vBaseValue = 123.0f;
        da1.m_vFrom = 4.0f;
        da1.m_vTo = 10.0f;
        CDuration dur(DirectUI::DurationType::TimeSpan, 2);
        da1.SetDuration(&dur);
        da1.m_rSpeedRatio = 0.5;

        CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 2.0 / 0.5, 2);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);
    }
}

void DoubleAnimationUnitTests::ValidateOffsetAnimation(
    _In_ const wchar_t* outputText,
    KnownPropertyIndex propIndex,
    bool hasXAnimation,
    bool hasYAnimation
    )
{
    LOG_OUTPUT(L"%s", outputText);
    xref_ptr<CUIElement> target1;
    target1.attach(new CUIElement);
    auto weak1 = xref::get_weakref(target1);
    CDependencyProperty dp1; dp1.SetIndex(propIndex);
    CDoubleAnimation da1; da1.SetUpForTesting();

    da1.m_vBaseValue = 123.0f;
    da1.m_vFrom = 0.0f;
    da1.m_vTo = 1.0f;
    CDuration dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&dur);
    da1.SetTargetObjectWeakRef(weak1);
    da1.SetTargetDependencyProperty(&dp1);

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

    CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);
    da1.AttachDCompAnimations();

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 1.0, 2);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 1.0f, WUCEFType::Linear);

    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasXAnimation, &da1, target1, KnownPropertyIndex::UIElement_OffsetXAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasYAnimation, &da1, target1, KnownPropertyIndex::UIElement_OffsetYAnimation);
}

void DoubleAnimationUnitTests::ValidateOffsetWUCAnimation()
{
    ValidateOffsetAnimation(
        L"Canvas.Left", KnownPropertyIndex::Canvas_Left,
        true, false
        );

    ValidateOffsetAnimation(
        L"Canvas.Top", KnownPropertyIndex::Canvas_Top,
        false, true
        );
}

void DoubleAnimationUnitTests::ValidateTranslateAnimation(
    _In_ const wchar_t* outputText,
    KnownPropertyIndex propIndex,
    bool hasXAnimation,
    bool hasYAnimation
    )
{
    LOG_OUTPUT(L"%s", outputText);
    xref_ptr<CTranslateTransform> target1;
    target1.attach(new CTranslateTransform);
    auto weak1 = xref::get_weakref(target1);
    CDependencyProperty dp1; dp1.SetIndex(propIndex);
    CDoubleAnimation da1; da1.SetUpForTesting();

    da1.m_vBaseValue = 123.0f;
    da1.m_vFrom = 0.0f;
    da1.m_vTo = 1.0f;
    CDuration dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&dur);
    da1.SetTargetObjectWeakRef(weak1);
    da1.SetTargetDependencyProperty(&dp1);

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

    CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);
    da1.AttachDCompAnimations();

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 1.0, 2);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 1.0f, WUCEFType::Linear);

    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasXAnimation, &da1, target1, KnownPropertyIndex::TranslateTransform_XAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasYAnimation, &da1, target1, KnownPropertyIndex::TranslateTransform_YAnimation);
}

void DoubleAnimationUnitTests::ValidateTranslateWUCAnimation()
{
    ValidateTranslateAnimation(
        L"X", KnownPropertyIndex::TranslateTransform_X,
        true, false
        );

    ValidateTranslateAnimation(
        L"Y", KnownPropertyIndex::TranslateTransform_Y,
        false, true
        );
}

void DoubleAnimationUnitTests::ValidateScaleAnimation(
    _In_ const wchar_t* outputText,
    KnownPropertyIndex propIndex,
    bool hasCenterXAnimation,
    bool hasCenterYAnimation,
    bool hasScaleXAnimation,
    bool hasScaleYAnimation
    )
{
    LOG_OUTPUT(L"%s", outputText);
    xref_ptr<CScaleTransform> target1;
    target1.attach(new CScaleTransform);
    auto weak1 = xref::get_weakref(target1);
    CDependencyProperty dp1; dp1.SetIndex(propIndex);
    CDoubleAnimation da1; da1.SetUpForTesting();

    da1.m_vBaseValue = 123.0f;
    da1.m_vFrom = 0.0f;
    da1.m_vTo = 1.0f;
    CDuration dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&dur);
    da1.SetTargetObjectWeakRef(weak1);
    da1.SetTargetDependencyProperty(&dp1);

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

    CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);
    da1.AttachDCompAnimations();

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 1.0, 2);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 1.0f, WUCEFType::Linear);

    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterXAnimation, &da1, target1, KnownPropertyIndex::ScaleTransform_CenterXAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterYAnimation, &da1, target1, KnownPropertyIndex::ScaleTransform_CenterYAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasScaleXAnimation, &da1, target1, KnownPropertyIndex::ScaleTransform_ScaleXAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasScaleYAnimation, &da1, target1, KnownPropertyIndex::ScaleTransform_ScaleYAnimation);
}

void DoubleAnimationUnitTests::ValidateScaleWUCAnimation()
{
    ValidateScaleAnimation(
        L"CenterX", KnownPropertyIndex::ScaleTransform_CenterX,
        true, false, false, false
        );

    ValidateScaleAnimation(
        L"CenterY", KnownPropertyIndex::ScaleTransform_CenterY,
        false, true, false, false
        );

    ValidateScaleAnimation(
        L"ScaleX", KnownPropertyIndex::ScaleTransform_ScaleX,
        false, false, true, false
        );

    ValidateScaleAnimation(
        L"ScaleY", KnownPropertyIndex::ScaleTransform_ScaleY,
        false, false, false, true
        );
}

void DoubleAnimationUnitTests::ValidateRotateAnimation(
    _In_ const wchar_t* outputText,
    KnownPropertyIndex propIndex,
    bool hasCenterXAnimation,
    bool hasCenterYAnimation,
    bool hasAngleAnimation
    )
{
    LOG_OUTPUT(L"%s", outputText);
    xref_ptr<CRotateTransform> target1;
    target1.attach(new CRotateTransform);
    auto weak1 = xref::get_weakref(target1);
    CDependencyProperty dp1; dp1.SetIndex(propIndex);
    CDoubleAnimation da1; da1.SetUpForTesting();

    da1.m_vBaseValue = 123.0f;
    da1.m_vFrom = 0.0f;
    da1.m_vTo = 1.0f;
    CDuration dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&dur);
    da1.SetTargetObjectWeakRef(weak1);
    da1.SetTargetDependencyProperty(&dp1);

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

    CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);
    da1.AttachDCompAnimations();

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 1.0, 2);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 1.0f, WUCEFType::Linear);

    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterXAnimation, &da1, target1, KnownPropertyIndex::RotateTransform_CenterXAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterYAnimation, &da1, target1, KnownPropertyIndex::RotateTransform_CenterYAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasAngleAnimation, &da1, target1, KnownPropertyIndex::RotateTransform_AngleAnimation);
}

void DoubleAnimationUnitTests::ValidateRotateWUCAnimation()
{
    ValidateRotateAnimation(
        L"CenterX", KnownPropertyIndex::RotateTransform_CenterX,
        true, false, false
        );

    ValidateRotateAnimation(
        L"CenterY", KnownPropertyIndex::RotateTransform_CenterY,
        false, true, false
        );

    ValidateRotateAnimation(
        L"Angle", KnownPropertyIndex::RotateTransform_Angle,
        false, false, true
        );
}

void DoubleAnimationUnitTests::ValidateSkewAnimation(
    _In_ const wchar_t* outputText,
    KnownPropertyIndex propIndex,
    bool hasCenterXAnimation,
    bool hasCenterYAnimation,
    bool hasAngleXAnimation,
    bool hasAngleYAnimation
    )
{
    LOG_OUTPUT(L"%s", outputText);
    xref_ptr<CSkewTransform> target1;
    target1.attach(new CSkewTransform);
    auto weak1 = xref::get_weakref(target1);
    CDependencyProperty dp1; dp1.SetIndex(propIndex);
    CDoubleAnimation da1; da1.SetUpForTesting();

    da1.m_vBaseValue = 123.0f;
    da1.m_vFrom = 0.0f;
    da1.m_vTo = 1.0f;
    CDuration dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&dur);
    da1.SetTargetObjectWeakRef(weak1);
    da1.SetTargetDependencyProperty(&dp1);

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

    CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);
    da1.AttachDCompAnimations();

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 1.0, 2);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 1.0f, WUCEFType::Linear);

    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterXAnimation, &da1, target1, KnownPropertyIndex::SkewTransform_CenterXAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterYAnimation, &da1, target1, KnownPropertyIndex::SkewTransform_CenterYAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasAngleXAnimation, &da1, target1, KnownPropertyIndex::SkewTransform_AngleXAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasAngleYAnimation, &da1, target1, KnownPropertyIndex::SkewTransform_AngleYAnimation);
}

void DoubleAnimationUnitTests::ValidateSkewWUCAnimation()
{
    ValidateSkewAnimation(
        L"CenterX", KnownPropertyIndex::SkewTransform_CenterX,
        true, false, false, false
        );

    ValidateSkewAnimation(
        L"CenterY", KnownPropertyIndex::SkewTransform_CenterY,
        false, true, false, false
        );

    ValidateSkewAnimation(
        L"AngleX", KnownPropertyIndex::SkewTransform_AngleX,
        false, false, true, false
        );

    ValidateSkewAnimation(
        L"AngleY", KnownPropertyIndex::SkewTransform_AngleY,
        false, false, false, true
        );
}

void DoubleAnimationUnitTests::ValidateCompositeAnimation(
    _In_ const wchar_t* outputText,
    KnownPropertyIndex propIndex,
    bool hasCenterXAnimation,
    bool hasCenterYAnimation,
    bool hasScaleXAnimation,
    bool hasScaleYAnimation,
    bool hasSkewXAnimation,
    bool hasSkewYAnimation,
    bool hasRotationAnimation,
    bool hasTranslateXAnimation,
    bool hasTranslateYAnimation
    )
{
    LOG_OUTPUT(L"%s", outputText);
    xref_ptr<CCompositeTransform> target1;
    target1.attach(new CCompositeTransform);
    auto weak1 = xref::get_weakref(target1);
    CDependencyProperty dp1; dp1.SetIndex(propIndex);
    CDoubleAnimation da1; da1.SetUpForTesting();

    da1.m_vBaseValue = 123.0f;
    da1.m_vFrom = 0.0f;
    da1.m_vTo = 1.0f;
    CDuration dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&dur);
    da1.SetTargetObjectWeakRef(weak1);
    da1.SetTargetDependencyProperty(&dp1);

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

    CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);
    da1.AttachDCompAnimations();

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 1.0, 2);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 1.0f, WUCEFType::Linear);

    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterXAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform_CenterXAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterYAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform_CenterYAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasScaleXAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform_ScaleXAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasScaleYAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform_ScaleYAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasSkewXAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform_SkewXAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasSkewYAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform_SkewYAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasRotationAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform_RotateAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasTranslateXAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform_TranslateXAnimation);
    DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasTranslateYAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform_TranslateYAnimation);
}

void DoubleAnimationUnitTests::ValidateCompositeWUCAnimation()
{
    ValidateCompositeAnimation(
        L"CenterX", KnownPropertyIndex::CompositeTransform_CenterX,
        true, false, false, false, false, false, false, false, false
        );

    ValidateCompositeAnimation(
        L"CenterY", KnownPropertyIndex::CompositeTransform_CenterY,
        false, true, false, false, false, false, false, false, false
        );

    ValidateCompositeAnimation(
        L"ScaleX", KnownPropertyIndex::CompositeTransform_ScaleX,
        false, false, true, false, false, false, false, false, false
        );

    ValidateCompositeAnimation(
        L"ScaleY", KnownPropertyIndex::CompositeTransform_ScaleY,
        false, false, false, true, false, false, false, false, false
        );

    ValidateCompositeAnimation(
        L"SkewX", KnownPropertyIndex::CompositeTransform_SkewX,
        false, false, false, false, true, false, false, false, false
        );

    ValidateCompositeAnimation(
        L"SkewY", KnownPropertyIndex::CompositeTransform_SkewY,
        false, false, false, false, false, true, false, false, false
        );

    ValidateCompositeAnimation(
        L"Rotation", KnownPropertyIndex::CompositeTransform_Rotation,
        false, false, false, false, false, false, true, false, false
        );

    ValidateCompositeAnimation(
        L"TranslateX", KnownPropertyIndex::CompositeTransform_TranslateX,
        false, false, false, false, false, false, false, true, false
        );

    ValidateCompositeAnimation(
        L"TranslateY", KnownPropertyIndex::CompositeTransform_TranslateY,
        false, false, false, false, false, false, false, false, true
        );
}

void DoubleAnimationUnitTests::ValidatePlaneProjectionAnimation(
    _In_ const wchar_t* outputText,
    KnownPropertyIndex propIndex,
    bool hasCenterXAnimation,
    bool hasCenterYAnimation,
    bool hasCenterZAnimation,
    bool hasRotationXAnimation,
    bool hasRotationYAnimation,
    bool hasRotationZAnimation,
    bool hasLocalOffsetXAnimation,
    bool hasLocalOffsetYAnimation,
    bool hasLocalOffsetZAnimation,
    bool hasGlobalOffsetXAnimation,
    bool hasGlobalOffsetYAnimation,
    bool hasGlobalOffsetZAnimation
    )
{
    LOG_OUTPUT(L"%s", outputText);

    xref_ptr<CDependencyObject> planeProjectionDO;
    #pragma warning(suppress: 6387) // C6387: CREATEPARAMETERS::{ctor} declares the parameter as _In_, but this unittest can get away without that.
    CREATEPARAMETERS cp(nullptr);
    VERIFY_SUCCEEDED(CPlaneProjection::Create(reinterpret_cast<CDependencyObject**>(planeProjectionDO.ReleaseAndGetAddressOf()), &cp));

    xref_ptr<CPlaneProjection> target1(static_cast<CPlaneProjection*>(planeProjectionDO.get()));
    auto weak1 = xref::get_weakref(target1);
    CDependencyProperty dp1; dp1.SetIndex(propIndex);
    CDoubleAnimation da1; da1.SetUpForTesting();

    da1.m_vBaseValue = 123.0f;
    da1.m_vFrom = 0.0f;
    da1.m_vTo = 1.0f;
    CDuration dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&dur);
    da1.SetTargetObjectWeakRef(weak1);
    da1.SetTargetDependencyProperty(&dp1);

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

    CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);
    da1.AttachDCompAnimations();

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 1.0, 2);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 1.0f, WUCEFType::Linear);

    {
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterXAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_CenterOfRotationXAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterYAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_CenterOfRotationYAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterZAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_CenterOfRotationZAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasRotationXAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_RotationXAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasRotationYAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_RotationYAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasRotationZAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_RotationZAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasLocalOffsetXAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_LocalOffsetXAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasLocalOffsetYAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_LocalOffsetYAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasLocalOffsetZAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_LocalOffsetZAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasGlobalOffsetXAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_GlobalOffsetXAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasGlobalOffsetYAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_GlobalOffsetYAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasGlobalOffsetZAnimation, &da1, target1, KnownPropertyIndex::PlaneProjection_GlobalOffsetZAnimation);
    }
}

void DoubleAnimationUnitTests::ValidatePlaneProjectionWUCAnimation()
{
    ValidatePlaneProjectionAnimation(
        L"CenterOfRotationX", KnownPropertyIndex::PlaneProjection_CenterOfRotationX,
        true, false, false, false, false, false, false, false, false, false, false, false
        );

    ValidatePlaneProjectionAnimation(
        L"CenterOfRotationY", KnownPropertyIndex::PlaneProjection_CenterOfRotationY,
        false, true, false, false, false, false, false, false, false, false, false, false
        );

    ValidatePlaneProjectionAnimation(
        L"CenterOfRotationZ", KnownPropertyIndex::PlaneProjection_CenterOfRotationZ,
        false, false, true, false, false, false, false, false, false, false, false, false
        );

    ValidatePlaneProjectionAnimation(
        L"RotationX", KnownPropertyIndex::PlaneProjection_RotationX,
        false, false, false, true, false, false, false, false, false, false, false, false
        );

    ValidatePlaneProjectionAnimation(
        L"RotationY", KnownPropertyIndex::PlaneProjection_RotationY,
        false, false, false, false, true, false, false, false, false, false, false, false
        );

    ValidatePlaneProjectionAnimation(
        L"RotationZ", KnownPropertyIndex::PlaneProjection_RotationZ,
        false, false, false, false, false, true, false, false, false, false, false, false
        );

    ValidatePlaneProjectionAnimation(
        L"LocalOffsetX", KnownPropertyIndex::PlaneProjection_LocalOffsetX,
        false, false, false, false, false, false, true, false, false, false, false, false
        );

    ValidatePlaneProjectionAnimation(
        L"LocalOffsetY", KnownPropertyIndex::PlaneProjection_LocalOffsetY,
        false, false, false, false, false, false, false, true, false, false, false, false
        );

    ValidatePlaneProjectionAnimation(
        L"LocalOffsetZ", KnownPropertyIndex::PlaneProjection_LocalOffsetZ,
        false, false, false, false, false, false, false, false, true, false, false, false
        );

    ValidatePlaneProjectionAnimation(
        L"GlobalOffsetX", KnownPropertyIndex::PlaneProjection_GlobalOffsetX,
        false, false, false, false, false, false, false, false, false, true, false, false
        );

    ValidatePlaneProjectionAnimation(
        L"GlobalOffsetY", KnownPropertyIndex::PlaneProjection_GlobalOffsetY,
        false, false, false, false, false, false, false, false, false, false, true, false
        );

    ValidatePlaneProjectionAnimation(
        L"GlobalOffsetZ", KnownPropertyIndex::PlaneProjection_GlobalOffsetZ,
        false, false, false, false, false, false, false, false, false, false, false, true
        );
}

void DoubleAnimationUnitTests::ValidateCompositeTransform3DAnimation(
    _In_ const wchar_t* outputText,
    KnownPropertyIndex propIndex,
    bool hasCenterXAnimation,
    bool hasCenterYAnimation,
    bool hasCenterZAnimation,
    bool hasScaleXAnimation,
    bool hasScaleYAnimation,
    bool hasScaleZAnimation,
    bool hasRotationXAnimation,
    bool hasRotationYAnimation,
    bool hasRotationZAnimation,
    bool hasTranslateXAnimation,
    bool hasTranslateYAnimation,
    bool hasTranslateZAnimation
    )
{
    LOG_OUTPUT(L"%s", outputText);
    xref_ptr<CCompositeTransform3D> target1;
    target1.attach(new CCompositeTransform3D);
    auto weak1 = xref::get_weakref(target1);
    CDependencyProperty dp1; dp1.SetIndex(propIndex);
    CDoubleAnimation da1; da1.SetUpForTesting();

    da1.m_vBaseValue = 123.0f;
    da1.m_vFrom = 0.0f;
    da1.m_vTo = 1.0f;
    CDuration dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&dur);
    da1.SetTargetObjectWeakRef(weak1);
    da1.SetTargetDependencyProperty(&dp1);

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();

    CompositionAnimationConversionResult result = da1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);
    da1.AttachDCompAnimations();

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(da1.m_spWUCAnimation, 1.0, 2);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 1.0f, WUCEFType::Linear);

    {
        WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterXAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_CenterXAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterYAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_CenterYAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasCenterZAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_CenterZAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasScaleXAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_ScaleXAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasScaleYAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_ScaleYAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasScaleZAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_ScaleZAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasRotationXAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_RotationXAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasRotationYAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_RotationYAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasRotationZAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_RotationZAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasTranslateXAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_TranslateXAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasTranslateYAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_TranslateYAnimation);
        DCompAnimationUnitTestHelper::VerifyWUCAnimationsAreSameObject(hasTranslateZAnimation, &da1, target1, KnownPropertyIndex::CompositeTransform3D_TranslateZAnimation);
    }
}

void DoubleAnimationUnitTests::ValidateCompositeTransform3DWUCAnimation()
{
    ValidateCompositeTransform3DAnimation(
        L"CenterX", KnownPropertyIndex::CompositeTransform3D_CenterX,
        true, false, false, false, false, false, false, false, false, false, false, false
        );

    ValidateCompositeTransform3DAnimation(
        L"CenterY", KnownPropertyIndex::CompositeTransform3D_CenterY,
        false, true, false, false, false, false, false, false, false, false, false, false
        );

    ValidateCompositeTransform3DAnimation(
        L"CenterZ", KnownPropertyIndex::CompositeTransform3D_CenterZ,
        false, false, true, false, false, false, false, false, false, false, false, false
        );

    ValidateCompositeTransform3DAnimation(
        L"ScaleX", KnownPropertyIndex::CompositeTransform3D_ScaleX,
        false, false, false, true, false, false, false, false, false, false, false, false
        );

    ValidateCompositeTransform3DAnimation(
        L"ScaleY", KnownPropertyIndex::CompositeTransform3D_ScaleY,
        false, false, false, false, true, false, false, false, false, false, false, false
        );

    ValidateCompositeTransform3DAnimation(
        L"ScaleZ", KnownPropertyIndex::CompositeTransform3D_ScaleZ,
        false, false, false, false, false, true, false, false, false, false, false, false
        );

    ValidateCompositeTransform3DAnimation(
        L"RotationX", KnownPropertyIndex::CompositeTransform3D_RotationX,
        false, false, false, false, false, false, true, false, false, false, false, false
        );

    ValidateCompositeTransform3DAnimation(
        L"RotationY", KnownPropertyIndex::CompositeTransform3D_RotationY,
        false, false, false, false, false, false, false, true, false, false, false, false
        );

    ValidateCompositeTransform3DAnimation(
        L"RotationZ", KnownPropertyIndex::CompositeTransform3D_RotationZ,
        false, false, false, false, false, false, false, false, true, false, false, false
        );

    ValidateCompositeTransform3DAnimation(
        L"TranslateX", KnownPropertyIndex::CompositeTransform3D_TranslateX,
        false, false, false, false, false, false, false, false, false, true, false, false
        );

    ValidateCompositeTransform3DAnimation(
        L"TranslateY", KnownPropertyIndex::CompositeTransform3D_TranslateY,
        false, false, false, false, false, false, false, false, false, false, true, false
        );

    ValidateCompositeTransform3DAnimation(
        L"TranslateZ", KnownPropertyIndex::CompositeTransform3D_TranslateZ,
        false, false, false, false, false, false, false, false, false, false, false, true
        );
}

} } } } } }
