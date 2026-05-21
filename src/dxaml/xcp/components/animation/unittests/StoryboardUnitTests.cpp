// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "StoryboardUnitTests.h"
#include "DoubleAnimation.h"
#include "DoubleAnimationUsingKeyFrames.h"
#include "Storyboard.h"
#include "TimelineCollection.h"
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
#include <KeySpline.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

void StoryboardUnitTests::SimpleStoryboards()
{
    {
        LOG_OUTPUT(L"<Storyboard>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 1.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' BeginTime='0:0:2' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);
        CTimeSpan da1_begin(2); da1.m_pBeginTime = &da1_begin;

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            2.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 1.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' AutoReverse='True' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);
        da1.m_fAutoReverse = true;

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' RepeatBehavior='2x' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Count, 0, 2.0f); da1.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' RepeatBehavior='0:0:2' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 2, 0.0f); da1.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' RepeatBehavior='Forever' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Forever, 0, 0.0f); da1.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Forever, 2.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' RepeatBehavior='0:0:2' AutoReverse='True' />");  // Tricky - repeats for 2 seconds, accounting for the reverse
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 2, 0.0f); da1.SetRepeatBehavior(&rep);
        da1.m_fAutoReverse = true;

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,
            true);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }
}

void StoryboardUnitTests::SimpleStoryboardsWithProperties()
{
    {
        LOG_OUTPUT(L"<Storyboard Duration='0:0:1'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CDuration sb1_dur(DirectUI::DurationType::TimeSpan, 1); sb1.SetDuration(&sb1_dur);

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 1.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard BeginTime='0:0:2'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CTimeSpan sb1_begin(2); sb1.m_pBeginTime = &sb1_begin;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            2.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 1.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard AutoReverse='True'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        sb1.m_fAutoReverse = true;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard RepeatBehavior='2x'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Count, 0, 2.0f); sb1.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard RepeatBehavior='0:0:2'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 2, 0.0f); sb1.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard RepeatBehavior='Forever'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Forever, 0, 0.0f); sb1.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Forever, 2.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard RepeatBehavior='0:0:2' AutoReverse='True'>");  // Tricky - repeats for 2 seconds, accounting for the reverse
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 2, 0.0f); sb1.SetRepeatBehavior(&rep);
        sb1.m_fAutoReverse = true;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,
            true);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard RepeatBehavior='0:0:4'>"); // Tricky - repeats for 4 seconds, accounting for the reverse
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 4, 0.0f); sb1.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' AutoReverse='True' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);
        da1.m_fAutoReverse = true;

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 4.0f,
            true);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }
    
    {
        LOG_OUTPUT(L"<Storyboard RepeatBehavior='0:0:4'>"); // Tricky - repeats for 4 seconds, accounting for the reverse
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 4, 0.0f); sb1.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1' AutoReverse='True' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da_dur);
        da1.m_fAutoReverse = true;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:1.5' />");
        CDoubleAnimation da2; da2.SetUpForTesting(); da2.m_vBaseValue = 123.0f; da2.m_vFrom = 4.0f; da2.m_vTo = 10.0f;
        CDuration da2_dur(DirectUI::DurationType::TimeSpan, 1.5); da2.SetDuration(&da2_dur);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));
        IFCFAILFAST(sb1.m_pChild->Append(&da2));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 4.0f,
            true);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da2.m_spWUCAnimation, 2.0, 2,   // Duration expanded to 2.0 because it's not repeating
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da2.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da2.m_spWUCAnimation, 0.75f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard SpeedRatio='2'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        sb1.m_rSpeedRatio = 2;

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:2' SpeedRatio='2' RepeatBehavior='0:0:4' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 2); da1.SetDuration(&da_dur);
        da1.m_rSpeedRatio = 2;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 4, 0.0f); da1.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 0.5, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 2.0f,     // Both RepeatBehavior and Duration account for local SpeedRatio
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }
}

void StoryboardUnitTests::StoryboardsWithDuration()
{
    {
        LOG_OUTPUT(L"<Storyboard Duration='0:0:3'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CDuration sb1_dur(DirectUI::DurationType::TimeSpan, 3); sb1.SetDuration(&sb1_dur);

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:2' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 2); da1.SetDuration(&da_dur);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 2.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 1.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);
        
        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard Duration='0:0:1'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CDuration sb1_dur(DirectUI::DurationType::TimeSpan, 1); sb1.SetDuration(&sb1_dur);

        LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:2' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 2); da1.SetDuration(&da_dur);

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::CannotBeClippedByParent, result);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        LOG_OUTPUT(L"");
    }
}

void StoryboardUnitTests::NestedStoryboards()
{
    {
        LOG_OUTPUT(L"<Storyboard>");
        CStoryboard sb2; sb2.SetUpForTesting();
        CTimelineCollection timelines2; sb2.m_pChild = &timelines2;

        LOG_OUTPUT(L"    <Storyboard>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;

        LOG_OUTPUT(L"        <DoubleAnimation Duration='0:0:1' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da_dur(DirectUI::DurationType::TimeSpan, 2); da1.SetDuration(&da_dur);

        LOG_OUTPUT(L"    </Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb2.m_pChild->Append(&sb1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb2.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 2.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 1.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);
        
        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        VERIFY_SUCCEEDED(timelines2.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard RepeatBehavior='3x'>");
        CStoryboard sb2; sb2.SetUpForTesting();
        CTimelineCollection timelines2; sb2.m_pChild = &timelines2;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Count, 0, 3.0f); sb2.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"    <Storyboard AutoReverse='True'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        sb1.m_fAutoReverse = true;

        LOG_OUTPUT(L"        <DoubleAnimation Duration='0:0:1' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da1_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da1_dur);

        LOG_OUTPUT(L"    </Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb2.m_pChild->Append(&sb1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb2.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 6.0f,
            true);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        VERIFY_SUCCEEDED(timelines2.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard RepeatBehavior='3x'>");
        CStoryboard sb2; sb2.SetUpForTesting();
        CTimelineCollection timelines2; sb2.m_pChild = &timelines2;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Count, 0, 3.0f); sb2.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"    <Storyboard>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;

        LOG_OUTPUT(L"        <DoubleAnimation Duration='0:0:1' AutoReverse='True' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da1_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da1_dur);
        da1.m_fAutoReverse = true;

        LOG_OUTPUT(L"    </Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb2.m_pChild->Append(&sb1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb2.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 6.0f,
            true);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        VERIFY_SUCCEEDED(timelines2.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard RepeatBehavior='3x'>");
        CStoryboard sb2; sb2.SetUpForTesting();
        CTimelineCollection timelines2; sb2.m_pChild = &timelines2;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Count, 0, 3.0f); sb2.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"    <Storyboard Duration='0:0:2'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CDuration sb1_dur(DirectUI::DurationType::TimeSpan, 2); sb1.SetDuration(&sb1_dur);

        LOG_OUTPUT(L"        <DoubleAnimation Duration='0:0:1' AutoReverse='True' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da1_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da1_dur);
        da1.m_fAutoReverse = true;

        LOG_OUTPUT(L"    </Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb2.m_pChild->Append(&sb1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb2.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 6.0f,
            true);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        VERIFY_SUCCEEDED(timelines2.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard Duration='0:0:2' RepeatBehavior='3x'>");
        CStoryboard sb2; sb2.SetUpForTesting();
        CTimelineCollection timelines2; sb2.m_pChild = &timelines2;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Count, 0, 3.0f); sb2.SetRepeatBehavior(&rep);
        CDuration sb2_dur(DirectUI::DurationType::TimeSpan, 2); sb2.SetDuration(&sb2_dur);

        LOG_OUTPUT(L"    <Storyboard>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;

        LOG_OUTPUT(L"        <DoubleAnimation Duration='0:0:1' AutoReverse='True' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da1_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da1_dur);
        da1.m_fAutoReverse = true;

        LOG_OUTPUT(L"    </Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb2.m_pChild->Append(&sb1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb2.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 6.0f,
            true);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        VERIFY_SUCCEEDED(timelines2.Clear());
        LOG_OUTPUT(L"");
    }

    {
        LOG_OUTPUT(L"<Storyboard RepeatBehavior='0:0:6'>");
        CStoryboard sb2; sb2.SetUpForTesting();
        CTimelineCollection timelines2; sb2.m_pChild = &timelines2;
        CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Duration, 6.0, 0); sb2.SetRepeatBehavior(&rep);

        LOG_OUTPUT(L"    <Storyboard Duration='0:0:2'>");
        CStoryboard sb1; sb1.SetUpForTesting();
        CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
        CDuration sb1_dur(DirectUI::DurationType::TimeSpan, 2); sb1.SetDuration(&sb1_dur);

        LOG_OUTPUT(L"        <DoubleAnimation Duration='0:0:1' AutoReverse='True' />");
        CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
        CDuration da1_dur(DirectUI::DurationType::TimeSpan, 1); da1.SetDuration(&da1_dur);
        da1.m_fAutoReverse = true;

        LOG_OUTPUT(L"        <DoubleAnimation Duration='0:0:1' />");
        CDoubleAnimation da2; da2.SetUpForTesting(); da2.m_vBaseValue = 123.0f; da2.m_vFrom = 4.0f; da2.m_vTo = 10.0f;
        CDuration da2_dur(DirectUI::DurationType::TimeSpan, 1); da2.SetDuration(&da2_dur);

        LOG_OUTPUT(L"    </Storyboard>");
        IFCFAILFAST(sb1.m_pChild->Append(&da1));
        IFCFAILFAST(sb1.m_pChild->Append(&da2));

        LOG_OUTPUT(L"</Storyboard>");
        IFCFAILFAST(sb2.m_pChild->Append(&sb1));

        CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
        CompositionAnimationConversionResult result = sb2.MakeCompositionAnimationsWithProperties(&context);
        VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da1.m_spWUCAnimation, 1.0, 2,
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 6.0f,
            true);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

        DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
            da2.m_spWUCAnimation, 2.0, 2,   // Duration expanded to 2.0 because it's not repeating
            0.0,
            WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Count, 3.0f,
            false);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da2.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
        DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da2.m_spWUCAnimation, 0.5f, 10.0f, WUCEFType::Linear);

        // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
        // and have been deleted themselves.
        VERIFY_SUCCEEDED(timelines1.Clear());
        VERIFY_SUCCEEDED(timelines2.Clear());
        LOG_OUTPUT(L"");
    }
}

void StoryboardUnitTests::ProgressRingStoryboard()
{
    LOG_OUTPUT(L"<Storyboard RepeatBehavior='Forever'>");
    CStoryboard sb1; sb1.SetUpForTesting();
    CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
    CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Forever, 123.321, 10); sb1.SetRepeatBehavior(&rep);

    LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:2' />");
    CDoubleAnimation da1; da1.SetUpForTesting(); da1.m_vBaseValue = 123.0f; da1.m_vFrom = 4.0f; da1.m_vTo = 10.0f;
    CDuration da1_dur(DirectUI::DurationType::TimeSpan, 2); da1.SetDuration(&da1_dur);

    LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:2' BeginTime='0:0:1' />");
    CDoubleAnimation da2; da2.SetUpForTesting(); da2.m_vBaseValue = 123.0f; da2.m_vFrom = 4.0f; da2.m_vTo = 10.0f;
    CDuration da2_dur(DirectUI::DurationType::TimeSpan, 2); da2.SetDuration(&da2_dur);
    CTimeSpan da2_begin(1); da2.m_pBeginTime = &da2_begin;

    LOG_OUTPUT(L"    <DoubleAnimation Duration='0:0:2' BeginTime='0:0:2' />");
    CDoubleAnimation da3; da3.SetUpForTesting(); da3.m_vBaseValue = 123.0f; da3.m_vFrom = 4.0f; da3.m_vTo = 10.0f;
    CDuration da3_dur(DirectUI::DurationType::TimeSpan, 2); da3.SetDuration(&da3_dur);
    CTimeSpan da3_begin(2); da3.m_pBeginTime = &da3_begin;

    LOG_OUTPUT(L"</Storyboard>");
    IFCFAILFAST(sb1.m_pChild->Append(&da1));
    IFCFAILFAST(sb1.m_pChild->Append(&da2));
    IFCFAILFAST(sb1.m_pChild->Append(&da3));

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
    CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
        da1.m_spWUCAnimation, 4.0, 2,   // Duration extended to 4.0s
        0.0,
        WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Forever, 1.0f,
        false);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 4.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.5f, 10.0f, WUCEFType::Linear);

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
        da2.m_spWUCAnimation, 4.0, 2,   // Duration extended to 4.0s
        0.0,    // No BeginTime - it's baked into the animation itself
        WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Forever, 1.0f,
        false);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da2.m_spWUCAnimation, 0.25f, 4.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da2.m_spWUCAnimation, 0.75f, 10.0f, WUCEFType::Linear);

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
        da3.m_spWUCAnimation, 4.0, 2,   // Duration extended to 4.0s
        0.0,    // No BeginTime - it's baked into the animation itself
        WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Forever, 1.0f,
        false);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da3.m_spWUCAnimation, 0.5f, 4.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da3.m_spWUCAnimation, 1.0f, 10.0f, WUCEFType::Linear);

    // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
    // and have been deleted themselves.
    VERIFY_SUCCEEDED(timelines1.Clear());
    LOG_OUTPUT(L"");
}

void StoryboardUnitTests::ProgressRingStoryboardKeyFrames()
{
    LOG_OUTPUT(L"<Storyboard RepeatBehavior='Forever'>");
    CStoryboard sb1; sb1.SetUpForTesting();
    CTimelineCollection timelines1; sb1.m_pChild = &timelines1;
    CRepeatBehavior rep(DirectUI::RepeatBehaviorType::Forever, 123.321, 10); sb1.SetRepeatBehavior(&rep);

    LOG_OUTPUT(L"    <DoubleAnimationUsingKeyFrames Duration='0:0:3' BeginTime='0:0:1'>");
    CDoubleAnimationUsingKeyFrames da1; da1.SetUpForTesting();
    CDuration dur(DirectUI::DurationType::TimeSpan, 3); da1.SetDuration(&dur);
    CTimeSpan da1_begin(1); da1.m_pBeginTime = &da1_begin;
    CDoubleKeyFrameCollection keyFrames; da1.m_pKeyFrames = &keyFrames;

    LOG_OUTPUT(L"      <SplineDoubleKeyFrame KeyTime='0.25' Value='30' />");
    CSplineDoubleKeyFrame kf1;
    CKeyTime kt1(0.25); kf1.SetKeyTime(&kt1);
    CKeySpline sp1; kf1.m_pKeySpline = &sp1;
    kf1.m_rValue = 30.0f;
    IFCFAILFAST(da1.m_pKeyFrames->Append(&kf1));

    LOG_OUTPUT(L"      <SplineDoubleKeyFrame KeyTime='1.25' Value='10' />");
    CSplineDoubleKeyFrame kf2;
    CKeyTime kt2(1.25); kf2.SetKeyTime(&kt2);
    kf2.m_rValue = 10.0f;
    IFCFAILFAST(da1.m_pKeyFrames->Append(&kf2));

    LOG_OUTPUT(L"      <SplineDoubleKeyFrame KeyTime='1.5' Value='50' />");
    CSplineDoubleKeyFrame kf3;
    CKeyTime kt3(1.5); kf3.SetKeyTime(&kt3);
    CKeySpline sp3; kf3.m_pKeySpline = &sp3;
    kf3.m_rValue = 50.0f;
    IFCFAILFAST(da1.m_pKeyFrames->Append(&kf3));

    LOG_OUTPUT(L"    </DoubleAnimationUsingKeyFrames>");
    LOG_OUTPUT(L"</Storyboard>");
    IFCFAILFAST(sb1.m_pChild->Append(&da1));

    CompositionAnimationConversionContext context = DCompAnimationUnitTestHelper::MakeCompositionContext();
    CompositionAnimationConversionResult result = sb1.MakeCompositionAnimationsWithProperties(&context);
    VERIFY_ARE_EQUAL(CompositionAnimationConversionResult::Success, result);

    DCompAnimationUnitTestHelper::VerifyWUCKeyFrameAnimation(
        da1.m_spWUCAnimation, 4.0, 4, 
        0.0,
        WUComp::AnimationIterationBehavior::AnimationIterationBehavior_Forever, 1.0f,
        false);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, 0.0f, 0.0f, WUCEFType::Null);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, (1.0f + 0.25f)/4.0f, 30.0f, WUCEFType::CubicSpline, 0.0f, 0.0f, 1.0f, 1.0f);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, (1.0f + 1.25f)/4.0f, 10.0f, WUCEFType::Linear);
    DCompAnimationUnitTestHelper::VerifyWUCKeyFrame(da1.m_spWUCAnimation, (1.0f + 1.5f)/4.0f, 50.0f, WUCEFType::CubicSpline, 0.0f, 0.0f, 1.0f, 1.0f);

    // Clear the collection now so it doesn't try to release its items when it gets deleted. Those items are on the stack
    // and have been deleted themselves.
    keyFrames.clear();

    // Remove the children so it doesn't try to release them when it gets deleted. Those items are on the stack
    // and have been deleted themselves.
    VERIFY_SUCCEEDED(timelines1.Clear());
    LOG_OUTPUT(L"");
}

} } } } } }
