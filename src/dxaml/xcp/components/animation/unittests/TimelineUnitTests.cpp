// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimelineUnitTests.h"
#include "Timeline.h"
#include "RepeatBehavior.h"
#include "DoubleAnimation.h"
#include "Timespan.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

void TimelineUnitTests::ValidateDurationRepeat()
{
    XFLOAT duration_out;
    bool isInfinite_out;

    // Asdf iterations
    {
        duration_out = 3.0f;

        // Time span should be ignored
        CRepeatBehavior repeat(DirectUI::RepeatBehaviorType::Count, 100.0, 1.6f);

        isInfinite_out = CTimeline::ExtendDurationWithRepeat(&duration_out, repeat.ValueWrapper()->Value());
        VERIFY_ARE_EQUAL(4.8f, duration_out);
        VERIFY_ARE_EQUAL(false, isInfinite_out);
    }

    // Asdf time
    {
        duration_out = 3.0f;

        // Iteration count should be ignored
        CRepeatBehavior repeat(DirectUI::RepeatBehaviorType::Duration, 246.8, 1000.0f);

        isInfinite_out = CTimeline::ExtendDurationWithRepeat(&duration_out, repeat.ValueWrapper()->Value());
        VERIFY_ARE_EQUAL(246.8f, duration_out);
        VERIFY_ARE_EQUAL(false, isInfinite_out);
    }

    // Asdf time, when each iteration is instant
    {
        duration_out = 0.0f;    // TimeSpan repeats should work even if each iteration has 0 duration.

        // Iteration count should be ignored
        CRepeatBehavior repeat(DirectUI::RepeatBehaviorType::Duration, 2.2, 1000.0f);

        isInfinite_out = CTimeline::ExtendDurationWithRepeat(&duration_out, repeat.ValueWrapper()->Value());
        VERIFY_ARE_EQUAL(2.2f, duration_out);
        VERIFY_ARE_EQUAL(false, isInfinite_out);
    }

    // Forever
    {
        duration_out = 1000.0f;

        // Both time span and iteration count should be ignored
        CRepeatBehavior repeat(DirectUI::RepeatBehaviorType::Forever, 10.0, 10.0f);

        isInfinite_out = CTimeline::ExtendDurationWithRepeat(&duration_out, repeat.ValueWrapper()->Value());
        VERIFY_ARE_EQUAL(1000.0f, duration_out);    // Unchanged for infinite repeats
        VERIFY_ARE_EQUAL(true, isInfinite_out);
    }
}

void TimelineUnitTests::ValidateDurationReverse()
{
    VERIFY_ARE_EQUAL(10.0f, CTimeline::ExtendDurationWithReverse(5.0f, true));
    VERIFY_ARE_EQUAL(5.0f, CTimeline::ExtendDurationWithReverse(5.0f, false));

    VERIFY_ARE_EQUAL(0.0f, CTimeline::ExtendDurationWithReverse(0.0f, true));
    VERIFY_ARE_EQUAL(0.0f, CTimeline::ExtendDurationWithReverse(0.0f, false));

    // Contrived, but ExtendDurationWithReverse doesn't care.
    VERIFY_ARE_EQUAL(-400.0f, CTimeline::ExtendDurationWithReverse(-200.0f, true));
    VERIFY_ARE_EQUAL(-200.0f, CTimeline::ExtendDurationWithReverse(-200.0f, false));
}

void TimelineUnitTests::ValidateDurationSpeedRatio()
{
    VERIFY_ARE_EQUAL(12.5f, CTimeline::AdjustDurationWithSpeedRatio(5.0f, 0.4f));
    VERIFY_ARE_EQUAL(2.5f, CTimeline::AdjustDurationWithSpeedRatio(5.0f, 2.0f));

    VERIFY_ARE_EQUAL(0.0f, CTimeline::AdjustDurationWithSpeedRatio(0.0f, 0.4f));
    VERIFY_ARE_EQUAL(0.0f, CTimeline::AdjustDurationWithSpeedRatio(0.0f, 2.0f));

    // Contrived, but AdjustDurationWithSpeedRatio doesn't care.
    VERIFY_ARE_EQUAL(-500.0f, CTimeline::AdjustDurationWithSpeedRatio(-200.0f, 0.4f));
    VERIFY_ARE_EQUAL(-100.0f, CTimeline::AdjustDurationWithSpeedRatio(-200.0f, 2.0f));
}

void TimelineUnitTests::ValidateDurationBeginTime()
{
    VERIFY_ARE_EQUAL(123.321f, CTimeline::AdjustDurationWithBeginTime(123.321f, nullptr));

    {
        CTimeSpan beginTime(0.0);

        VERIFY_ARE_EQUAL(234.432f, CTimeline::AdjustDurationWithBeginTime(234.432f, &beginTime));
    }

    {
        CTimeSpan beginTime(20.0);

        VERIFY_ARE_EQUAL(365.543f, CTimeline::AdjustDurationWithBeginTime(345.543f, &beginTime));
    }

    {
        CTimeSpan beginTime(-100.0);

        VERIFY_ARE_EQUAL(356.654f, CTimeline::AdjustDurationWithBeginTime(456.654f, &beginTime));
    }
}

void TimelineUnitTests::ValidateGetDurationWithProperties()
{
    DirectUI::DurationType durationType_out;
    XFLOAT durationValue_out = 0;

    // Durations from children with properties
    {
        CDoubleAnimation anim;

        CDuration duration(DirectUI::DurationType::TimeSpan, 20);
        anim.SetDuration(&duration);

        anim.m_fAutoReverse = true;

        CRepeatBehavior repeat(DirectUI::RepeatBehaviorType::Count, 100.0, 1.6f); // Time span should be ignored
        anim.SetRepeatBehavior(&repeat);

        anim.m_rSpeedRatio = 1.2f;

        CTimeSpan beginTime(-10.0);
        anim.m_pBeginTime = &beginTime;

        anim.GetDurationWithProperties(&durationType_out, &durationValue_out);
        VERIFY_ARE_EQUAL(DirectUI::DurationType::TimeSpan, durationType_out);
        VERIFY_ARE_EQUAL(20.0f * 2 * 1.6f / 1.2f - 10, durationValue_out);
    }
}

} } } } } }
