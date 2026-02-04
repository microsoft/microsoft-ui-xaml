// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "AnimationUnitTests.h"
#include "DoubleAnimation.h"
#include "KeyTime.h"
#include "DoubleKeyFrame.h"
#include "Duration.h"
#include "KeySpline.h"
#include "DCompAnimationUnitTestHelper.h"

using namespace std::placeholders;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

void AnimationUnitTests::ValidateGetNaturalDurationFromKeyFrames()
{
    DirectUI::DurationType durationType_out;
    XFLOAT durationValue_out = 0;

    {
        LOG_OUTPUT(L"Null KeyFrames");
        std::vector<CDependencyObject*> emptyKeyFrames;

        CAnimation::GetNaturalDurationFromKeyFrames(
            emptyKeyFrames,
            &durationType_out,
            &durationValue_out
            );

        VERIFY_ARE_EQUAL(DirectUI::DurationType::TimeSpan, durationType_out);
        VERIFY_ARE_EQUAL(1.0f, durationValue_out);
    }

    {
        LOG_OUTPUT(L"KeyFrames out of order");

        CLinearDoubleKeyFrame frame0;
        CKeyTime time456(456.789); frame0.SetKeyTime(&time456);

        CLinearDoubleKeyFrame frame1;
        CKeyTime time123(123.321); frame1.SetKeyTime(&time123);

        std::vector<CDependencyObject*> keyFrames;
        keyFrames.push_back(&frame0);
        keyFrames.push_back(&frame1);

        CAnimation::GetNaturalDurationFromKeyFrames(
            keyFrames,
            &durationType_out,
            &durationValue_out
            );

        VERIFY_ARE_EQUAL(DirectUI::DurationType::TimeSpan, durationType_out);
        VERIFY_ARE_EQUAL(123.321f, durationValue_out);
    }

    {
        // WinBlue tolerated a key frame with no key time. We'll do the same in Threshold.
        LOG_OUTPUT(L"KeyFrame with null KeyTime");

        CLinearDoubleKeyFrame noKeyTime;

        std::vector<CDependencyObject*> keyFrames;
        keyFrames.push_back(&noKeyTime);

        CAnimation::GetNaturalDurationFromKeyFrames(
            keyFrames,
            &durationType_out,
            &durationValue_out
            );

        VERIFY_ARE_EQUAL(DirectUI::DurationType::TimeSpan, durationType_out);
        VERIFY_ARE_EQUAL(0, durationValue_out);
    }

    {
        LOG_OUTPUT(L"KeyFrames with null KeyTime");

        CLinearDoubleKeyFrame frame0;

        CLinearDoubleKeyFrame frame1;
        CKeyTime time456(456.789); frame1.SetKeyTime(&time456);

        std::vector<CDependencyObject*> keyFrames;
        keyFrames.push_back(&frame0);
        keyFrames.push_back(&frame1);

        CAnimation::GetNaturalDurationFromKeyFrames(
            keyFrames,
            &durationType_out,
            &durationValue_out
            );

        VERIFY_ARE_EQUAL(DirectUI::DurationType::TimeSpan, durationType_out);
        VERIFY_ARE_EQUAL(456.789f, durationValue_out);
    }

    {
        // The second key frame has no KeyTime, which is implicit 0 duration. We should keep searching backwards
        // in the list and hit the first one.
        LOG_OUTPUT(L"KeyFrames out of order with null KeyTime");

        CLinearDoubleKeyFrame frame0;
        CKeyTime time456(456.789); frame0.SetKeyTime(&time456);

        CLinearDoubleKeyFrame frame1;

        std::vector<CDependencyObject*> keyFrames;
        keyFrames.push_back(&frame0);
        keyFrames.push_back(&frame1);

        CAnimation::GetNaturalDurationFromKeyFrames(
            keyFrames,
            &durationType_out,
            &durationValue_out
            );

        VERIFY_ARE_EQUAL(DirectUI::DurationType::TimeSpan, durationType_out);
        VERIFY_ARE_EQUAL(456.789f, durationValue_out);
    }
}

void AnimationUnitTests::ValidateGetNaturalDuration_DA()
{
    DirectUI::DurationType durationType_out;
    XFLOAT durationValue_out = 0;

    // Implicit duration - 1 second
    {
        CDoubleAnimation anim;

        anim.GetNaturalDuration(&durationType_out, &durationValue_out);
        VERIFY_ARE_EQUAL(DirectUI::DurationType::TimeSpan, durationType_out);
        VERIFY_ARE_EQUAL(1.0f, durationValue_out);
    }

    // Explicit duration - timespan
    {
        CDuration dur(DirectUI::DurationType::TimeSpan, 987.654);

        CDoubleAnimation anim;
        anim.SetDuration(&dur);

        anim.GetNaturalDuration(&durationType_out, &durationValue_out);
        VERIFY_ARE_EQUAL(DirectUI::DurationType::TimeSpan, durationType_out);
        VERIFY_ARE_EQUAL(987.654f, durationValue_out);
    }

    // Explicit duration - automatic
    {
        CDuration dur(DirectUI::DurationType::Automatic, 321.321);

        CDoubleAnimation anim;
        anim.SetDuration(&dur);

        anim.GetNaturalDuration(&durationType_out, &durationValue_out);
        // Automatic is expected to be converted to a TimeSpan of 1.0.
        VERIFY_ARE_EQUAL(DirectUI::DurationType::TimeSpan, durationType_out);
        VERIFY_ARE_EQUAL(1.0f, durationValue_out);
    }

    // Explicit duration - forever
    {
        CDuration dur(DirectUI::DurationType::Forever, 135.79);

        CDoubleAnimation anim;
        anim.SetDuration(&dur);

        anim.GetNaturalDuration(&durationType_out, &durationValue_out);
        VERIFY_ARE_EQUAL(DirectUI::DurationType::Forever, durationType_out);
        VERIFY_ARE_EQUAL(0.0f, durationValue_out);   // Duration not reported for Forever.
    }

    // Explicit duration - negative timespan
    // Negative times would be sanitized before they get stored in CDuration::m_rTimeSpan. This is a contrived
    // scenario but GetNaturalDuration shouldn't care about the sign of the explicit value.
    {
        CDuration dur(DirectUI::DurationType::TimeSpan, -10.10);

        CDoubleAnimation anim;
        anim.SetDuration(&dur);

        anim.GetNaturalDuration(&durationType_out, &durationValue_out);
        VERIFY_ARE_EQUAL(DirectUI::DurationType::TimeSpan, durationType_out);
        VERIFY_ARE_EQUAL(-10.10f, durationValue_out);
    }
}

/*
Note for future tests:

This never repeats, as expected. One iteration lasts forever.
<Storyboard>
    <DoubleAnimationUsingKeyFrames Storyboard.TargetName="asdf" Storyboard.TargetProperty="(Canvas.Left)" From="0" To="1000" RepeatBehavior="3x" Duration="Forever" />
</Storyboard>

This repeats 3x in 3 seconds. The Duration="Forever" is ignored, because the animation uses key frames.
<Storyboard>
    <DoubleAnimationUsingKeyFrames Storyboard.TargetName="asdf" Storyboard.TargetProperty="(Canvas.Left)" RepeatBehavior="3x" Duration="Forever">
        <DiscreteDoubleKeyFrame Value="0" KeyTime="0" />
        <LinearDoubleKeyFrame Value="1000" KeyTime="0:0:1" />
    </DoubleAnimationUsingKeyFrames>
</Storyboard>
*/

} } } } } }
