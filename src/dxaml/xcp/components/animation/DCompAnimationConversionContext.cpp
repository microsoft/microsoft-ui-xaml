// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DCompAnimationConversionContext.h"
#include "RepeatBehavior.h"
#include "TimeMgr.h"

CompositionAnimationConversionContext::CompositionAnimationConversionContext(_In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics, _In_ ixp::ICompositor* compositor, const float initialSpeedRatio)
    : m_easingFunctionStatics(easingFunctionStatics)
    , m_compositor(compositor)
    , m_speedRatio(initialSpeedRatio)
{
}

CompositionAnimationConversionContext::CompositionAnimationConversionContext(_In_ const CompositionAnimationConversionContext& other)
{
    m_compositor = other.m_compositor;
    m_easingFunctionStatics = other.m_easingFunctionStatics;

    m_beginTime = other.m_beginTime;
    m_beginTimeInsideAnimation = other.m_beginTimeInsideAnimation;

    m_speedRatio = other.m_speedRatio;

    m_durationWithSpeedRatio = other.m_durationWithSpeedRatio;
    m_hasInfiniteDuration = other.m_hasInfiniteDuration;

    m_hasRepeat = other.m_hasRepeat;
    m_hasReverse = other.m_hasReverse;
    m_loopsForever = other.m_loopsForever;
    m_finalLengthWithRepeatAndReverseAndSpeedRatio = other.m_finalLengthWithRepeatAndReverseAndSpeedRatio;
    m_forcedDurationForRepeatAndReverseWithSpeedRatio = other.m_forcedDurationForRepeatAndReverseWithSpeedRatio;

    m_forceCompositionAnimationDirty = other.m_forceCompositionAnimationDirty;

    m_storyboardTelemetryName = other.m_storyboardTelemetryName;
}

_Check_return_ CompositionAnimationConversionResult CompositionAnimationConversionContext::ApplyBeginTimeSpeedRatioAndDuration(
    float beginTime,
    float speedRatio,
    DirectUI::DurationType durationType,
    float duration)
{
    if (duration == 0.0f)
    {
        return CompositionAnimationConversionResult::CannotHaveZeroDuration;
    }
    else if (beginTime < 0.0f || duration < 0.0f)
    {
        return CompositionAnimationConversionResult::CannotHaveNegativeTime;
    }
    else if (beginTime > CompositionMaximumTimeLimit || duration > CompositionMaximumTimeLimit)
    {
        return CompositionAnimationConversionResult::CannotExceedMaximumTimeLimit;
    }
    else if (duration < CompositionMinimumDuration)
    {
        return CompositionAnimationConversionResult::CannotHaveLessThanMinimumDuration;
    }

    // The local SpeedRatio doesn't apply to BeginTime. Make sure to apply BeginTime before updating SpeedRatio.
    // (Any SpeedRatio inherited from timeline parents will apply to the BeginTime.)
    float beginTimeWithSpeedRatio = beginTime / m_speedRatio;

    // We can either set the BeginTime on the Composition animation directly, or we can collect them and push them
    // into the animation itself. The decision depends on whether we're repeating or reversing. See comment on
    // m_beginTimeInsideAnimation for more details.
    if (!m_hasRepeat && !m_hasReverse)
    {
        m_beginTime += beginTimeWithSpeedRatio;
    }
    else
    {
        m_beginTimeInsideAnimation += beginTimeWithSpeedRatio;
    }

    // Update the SpeedRatio.
    m_speedRatio *= speedRatio;

    if (durationType == DirectUI::DurationType::Forever)
    {
        // We allow infinite durations while we're walking down the tree. A Storyboard that holds an infinitely
        // repeating animation will report that its own duration is infinite, which is accurate. This is still
        // allowable. The only durations that are not allowed to be infinite are anything that repeats or reverses,
        // or the animations themselves.
        
        // If any timing parent has restricted the duration at all, then it would be clipping this timeline, which
        // is disallowed.
        if (!m_hasInfiniteDuration)
        {
            return CompositionAnimationConversionResult::CannotBeClippedByParent;
        }
    }
    else
    {
        // The local SpeedRatio does apply to the duration. Make sure to update duration after updating SpeedRatio.
        float durationWithSpeedRatio = duration / m_speedRatio + m_beginTimeInsideAnimation;
        if (m_durationWithSpeedRatio > 0.0  // The first time a duration is set, this will be 0.
            && durationWithSpeedRatio > m_durationWithSpeedRatio)
        {
            return CompositionAnimationConversionResult::CannotBeClippedByParent;
        }
        m_durationWithSpeedRatio = durationWithSpeedRatio;
        m_hasInfiniteDuration = false;
    }

    return CompositionAnimationConversionResult::Success;
}

_Check_return_ CompositionAnimationConversionResult CompositionAnimationConversionContext::SetRepeatAndReverse(
    _In_ const xref_ptr<RepeatBehaviorVO::Wrapper> repeat,
    const bool hasReverse,
    DirectUI::DurationType naturalDurationType,
    float naturalDuration)
{
    const bool hasRepeat = 
        (repeat != nullptr)
        // Some timelines have default values of repeat=1x. These don't count as actually having a repeat.
        && !(repeat->Value().GetRepeatBehaviorType() == DirectUI::RepeatBehaviorType::Count
             && repeat->Value().GetCount() == 1.0f);

    if (hasRepeat || hasReverse)
    {
        if (hasRepeat
            && (m_hasRepeat
                // Repeat must be outside reverse. Already having a reverse means we can't have a repeat,
                // since we're walking down the timing tree.
                || m_hasReverse))
        {
            return CompositionAnimationConversionResult::CannotNestRepeatBehavior;
        }
        else if (hasReverse && m_hasReverse)
            // Reverse can be inside repeat. Already having a repeat when we encounter a reverse is okay.
        {
            return CompositionAnimationConversionResult::CannotNestAutoReverse;
        }
        else if (naturalDurationType == DirectUI::DurationType::Forever)
        {
            return CompositionAnimationConversionResult::CannotHaveInfiniteDuration;
        }
        else if (naturalDuration == 0.0f)
        {
            return CompositionAnimationConversionResult::CannotHaveZeroDuration;
        }
        else if (naturalDuration < 0.0f)
        {
            return CompositionAnimationConversionResult::CannotHaveNegativeTime;
        }
        else if (naturalDuration > CompositionMaximumTimeLimit)
        {
            return CompositionAnimationConversionResult::CannotExceedMaximumTimeLimit;
        }
        else if (naturalDuration < CompositionMinimumDuration)
        {
            return CompositionAnimationConversionResult::CannotHaveLessThanMinimumDuration;
        }

        if (hasRepeat)
        {
            float finalLengthWithRepeatAndReverseAndSpeedRatio = 0.0f;
            bool loopsForever = false;
            switch (repeat->Value().GetRepeatBehaviorType())
            {
                case DirectUI::RepeatBehaviorType::Count:
                {
                    // For RepeatBehavior="3x", take the natural duration and multiply by the number of iterations to
                    // get the final duration...
                    float iterations = repeat->Value().GetCount();
                    finalLengthWithRepeatAndReverseAndSpeedRatio = iterations * naturalDuration / m_speedRatio;

                    // ...and if there is an AutoReverse="True" on the same Timeline, then multiply that duration by 2.
                    if (hasReverse)
                    {
                        finalLengthWithRepeatAndReverseAndSpeedRatio *= 2;
                    }
                    break;
                }

                case DirectUI::RepeatBehaviorType::Duration:
                {
                    // For RepeatBehavior="0:0:3", the explicit time specified by the RepeatBehavior is the final
                    // duration. Having AutoReverse="True" on the same Timeline makes no difference.
                    finalLengthWithRepeatAndReverseAndSpeedRatio = static_cast<float>(repeat->Value().GetDurationInSec()) / m_speedRatio;
                    break;
                }

                case DirectUI::RepeatBehaviorType::Forever:
                {
                    loopsForever = true;
                    break;
                }
            }

            if (!loopsForever && finalLengthWithRepeatAndReverseAndSpeedRatio <= 0.0f)
            {
                return CompositionAnimationConversionResult::CannotHaveNonpositiveRepeat;
            }
            else
            {
                // These values are only set once there's a repeat or a reverse, and there shouldn't be any yet.
                ASSERT(m_beginTimeInsideAnimation == 0.0f);
                ASSERT(m_finalLengthWithRepeatAndReverseAndSpeedRatio == 0.0f);
                ASSERT(!m_loopsForever);

                m_hasRepeat = true;
                m_finalLengthWithRepeatAndReverseAndSpeedRatio = finalLengthWithRepeatAndReverseAndSpeedRatio;
                m_loopsForever = loopsForever;
                m_forcedDurationForRepeatAndReverseWithSpeedRatio = naturalDuration / m_speedRatio;
            }
        }
        // Just a reverse without any repeat on this Timeline.
        else if (hasReverse)
        {
            // If there's no repeat on any ancestor Timeline, then we need to set the ultimate length of the repeated
            // animation (m_finalLengthWithRepeatAndReverseAndSpeedRatio) as well as the forced duration of a single
            // iteration of the animation (m_forcedDurationForRepeatAndReverseWithSpeedRatio).
            if (!m_hasRepeat)
            {
                ASSERT(m_finalLengthWithRepeatAndReverseAndSpeedRatio == 0.0f);
                ASSERT(m_forcedDurationForRepeatAndReverseWithSpeedRatio == 0.0f);

                m_finalLengthWithRepeatAndReverseAndSpeedRatio = 2 * naturalDuration / m_speedRatio;
                m_forcedDurationForRepeatAndReverseWithSpeedRatio = naturalDuration / m_speedRatio;
            }
            //
            // Or, if there was a repeat on an ancestor timeline, we need to cut the forced duration of a single
            // iteration of the animation in half (m_forcedDurationForRepeatAndReverseWithSpeedRatio). The scenario
            // that we're covering is this:
            //
            //      <Storyboard RepeatBehavior="3x">
            //          <DoubleAnimation Duration="0:0:1" AutoReverse="True" />
            //      <Storyboard>
            //
            // Here, the Storyboard has a NaturalDuration of 2s (because it accounts for the AutoReverse of its child).
            // It sets 2s as the forced duration of a single iteration of the animation. But the actual duration needed
            // is 1s. The DoubleAnimation will divide the forced duration by 2, because it knows that it has an
            // AutoReverse, and the ancestor that set a forced duration included this AutoReverse in its calculations,
            // which needs to be divided back out.
            //
            // The interesting part is if we have this:
            //
            //      <Storyboard RepeatBehavior="3x">
            //          <DoubleAnimation Duration="0:0:1" AutoReverse="True" />
            //          <ColorAnimation Duration="0:0:1.5" />
            //      <Storyboard>
            //
            // Here the DoubleAnimation is in the situation described above, but the ColorAnimation has no AutoReverse.
            // It will keep using the 2s calculated by the Storyboard as its forced duration., which is exactly what we
            // want. We end up with DoubleAnimation having Duration=1 and IterationCount=6, and ColorAnimation having
            // Duration=2 and IterationCount=3. Note that DoubleAnimation and ColorAnimation will each use a copy of the
            // conversion context computed by the Storyboard, which makes this possible.
            //
            // We also need to make sure the AutoReverse on this Timeline is consistent with the RepeatBehavior on the
            // ancestor. This means there can't be a BeginTime accumulated between them, and they must match after
            // dividing by 2. We can't make Composition animations for things like:
            //
            //      <!-- Enforces m_forcedDurationForRepeatAndReverseWithSpeedRatio=2 -->
            //      <Storyboard Duration="0:0:2" RepeatBehavior="3x">
            //
            //          <!-- BeginTime means the repeat above and the reverse here have different periods -->
            //          <DoubleAnimation Duration="0:0:1" AutoReverse="True" BeginTime="0:0:1" />
            //
            //          <!-- The repeat above and the reverse here also have different periods -->
            //          <ColorAnimation Duration="0:0:1.5" AutoReverse="True" />
            //      <Storyboard>
            //
            else
            {
                ASSERT(m_finalLengthWithRepeatAndReverseAndSpeedRatio > 0.0f);
                ASSERT(m_forcedDurationForRepeatAndReverseWithSpeedRatio > 0.0f);

                double forcedDurationWithSpeedRatio = naturalDuration / m_speedRatio;
                if (m_beginTimeInsideAnimation != 0.0f
                    || forcedDurationWithSpeedRatio != m_forcedDurationForRepeatAndReverseWithSpeedRatio / 2)
                {
                    return CompositionAnimationConversionResult::CannotHaveDifferentForcedDurations;
                }

                m_forcedDurationForRepeatAndReverseWithSpeedRatio = static_cast<float>(forcedDurationWithSpeedRatio);
            }
        }

        if (hasReverse)
        {
            m_hasReverse = true;
        }
    }

    return CompositionAnimationConversionResult::Success;
}

float CompositionAnimationConversionContext::GetDurationWithSpeedRatio() const
{
    //
    // The duration of an animation may be dictated by the duration of a repeating or reversing ancestor timeline.
    // See comment on m_forcedDurationForRepeatAndReverseWithSpeedRatio for details.
    //
    // Also, both of these durations account for m_beginTimeInsideAnimation, which is what we want. This duration
    // is meant to be set on the Composition animation.
    //

    if (m_hasRepeat || m_hasReverse)
    {
        return m_forcedDurationForRepeatAndReverseWithSpeedRatio;
    }
    else
    {
        return m_durationWithSpeedRatio;
    }
}

_Check_return_ CompositionAnimationConversionResult CompositionAnimationConversionContext::ApplyProperties(_In_ ixp::IKeyFrameAnimation* animation)
{
    // By this point something must have set a finite duration. The allowable scenario is an infinitely looping
    // animation inside a Storyboard, in which case that animation would have set a finite duration already.
    if (m_hasInfiniteDuration)
    {
        return CompositionAnimationConversionResult::CannotHaveInfiniteDuration;
    }

    if (m_beginTime > 0.0)
    {
        wf::TimeSpan startTime = { static_cast<INT64>(m_beginTime * 10000000) };    // 1000 * 1000 * 10 - duration is the number of 100-ns ticks.
        IFCFAILFAST(animation->put_DelayTime(startTime));

        wrl::ComPtr<ixp::IKeyFrameAnimation3> kfa3;
        IFCFAILFAST(animation->QueryInterface(IID_PPV_ARGS(&kfa3)));
        IFCFAILFAST(kfa3->put_DelayBehavior(ixp::AnimationDelayBehavior_SetInitialValueAfterDelay));
    }

    // m_beginTimeInsideAnimation will be pushed into the animation itself. No need to deal with it here.

    if (m_hasReverse)
    {
        wrl::ComPtr<ixp::IKeyFrameAnimation2> kfa2;
        IFCFAILFAST(animation->QueryInterface(IID_PPV_ARGS(&kfa2)));
        IFCFAILFAST(kfa2->put_Direction(ixp::AnimationDirection_Alternate));
    }

    if (m_hasRepeat
        // Note: For Composition animations, having an animation play forwards then backwards counts as two
        // iterations, so we still have to set an IterationCount if there's just a reverse.
        || m_hasReverse)
    {
        if (m_loopsForever)
        {
            IFCFAILFAST(animation->put_IterationBehavior(ixp::AnimationIterationBehavior_Forever));
        }
        else
        {
            float iterations = m_finalLengthWithRepeatAndReverseAndSpeedRatio / m_forcedDurationForRepeatAndReverseWithSpeedRatio;

            // Note: FractionalReal does a subtraction with a cast to an int32, so this will detect a remainder for large
            // values that can't fit into an int32 and fail. We'll live with that restriction.
            if (FractionReal(iterations) != 0.0f)
            {
                return CompositionAnimationConversionResult::CannotHaveFractionalRepeat;
            }

            IFCFAILFAST(animation->put_IterationBehavior(ixp::AnimationIterationBehavior_Count));
            IFCFAILFAST(animation->put_IterationCount(static_cast<INT32>(iterations)));
        }
    }

    double durationWithSpeedRatio = GetDurationWithSpeedRatio();
    wf::TimeSpan duration = { static_cast<INT64>(durationWithSpeedRatio * 10000000) };  // 1000 * 1000 * 10 - duration is the number of 100-ns ticks.
    IFCFAILFAST(animation->put_Duration(duration));

    return CompositionAnimationConversionResult::Success;
}
