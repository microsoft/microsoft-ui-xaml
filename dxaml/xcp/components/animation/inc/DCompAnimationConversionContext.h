// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "EnumDefs.g.h"
#include "RepeatBehaviorVO.h"

class CTimeManager;
class CTimeline;
class RepeatBehaviorVO;
class CEasingFunctionBase;
class CKeySpline;

enum class CompositionAnimationConversionResult : byte
{
    Success,
    CannotHaveNegativeTime,
    CannotExceedMaximumTimeLimit,
    CannotHaveInfiniteDuration,
    CannotHaveZeroDuration,
    CannotHaveLessThanMinimumDuration,
    CannotBeClippedByParent,
    CannotNestRepeatBehavior,
    CannotHaveNonpositiveRepeat,
    CannotHaveFractionalRepeat,
    CannotNestAutoReverse,
    CannotHaveDifferentForcedDurations,
    CannotHaveKeyFramesBeyondDuration,
    CannotHaveKeyFramesWithNegativeKeyTime,
};

//
// Holds information necessary to transform a XAML timing tree into a ICompositionAnimation.
// A context is passed down the timing tree, and collects BeginTimes and SpeedRatios to be consumed by the animations
// at the leaves.
//
// A Composition KeyFrameAnimation has limitations on the properties that it supports, so not all Xaml timing trees
// can be converted. We'll have Xaml tick the animations that we can't convert on the UI thread.
//
// A KeyFrameAnimation has a single repeat and a single reverse, and both must apply to the same duration. The reverse
// is effectively inside the repeat. We can have Xaml support multiple repeats/reverses or different durations by
// unrolling the animation, but that can get complicated if the duration has fractional iterations or if we have to
// run easing functions backwards, so we don't bother.
//
// The key frames inside the KeyFrameAnimation are also limited to [0..1] as their key times, and cannot be added
// outside the duration of the animation.
//
// So these are the limitations on Xaml timing trees:
//    - AutoReverse must be the innermost property, with a possible RepeatBehavior outside it.
//    - There can be at most one AutoReverse and one RepeatBehavior. They must apply to the same natural duration.
//    - The RepeatBehavior must be an integer number of iterations, or half iterations of there is an AutoReverse.
//    - No BeginTime can be negative. The cumulative BeginTime must be between 0 and 24 days. Any BeginTime before
//      the repeat/reverse can be set on the Composition animation directly. Any BeginTime inside the repeat/reverse
//      will be baked into the animation itself.
//    - There can be explicit durations on Storyboards, but they must not clip the timelines inside them.
//    - The durations of animations must be between 0 and 24 days. Animations can't naturally run forever, but can
//      loop forever. This means that Storyboards can run forever (if they hold an animation that's looping forever),
//      as long as they're not also looping/reversing themselves.
//    - We can accommodate all speed ratio properties by multiplying it into the key times.
//    - We can accommodate nested Storyboards, provided they don't break any of the above rules.
//
class CompositionAnimationConversionContext
{
public:
    CompositionAnimationConversionContext(_In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics, _In_ ixp::ICompositor* compositor, const float initialSpeedRatio);
    CompositionAnimationConversionContext(_In_ const CompositionAnimationConversionContext& other);

    bool IsForceCompositionAnimationDirty() const { return m_forceCompositionAnimationDirty; }
    void SetForceCompositionAnimationDirty(bool value) { m_forceCompositionAnimationDirty = value; }

    double GetBeginTime() const { return m_beginTime; }
    float GetSpeedRatio() const { return m_speedRatio; }

    // The local SpeedRatio of a timeline doesn't apply to its BeginTime but does apply to its Duration. Instead of
    // having the caller deal with the property ordering, we have them pass everything in together and take care
    // of it ourselves.
    _Check_return_ CompositionAnimationConversionResult ApplyBeginTimeSpeedRatioAndDuration(
        float beginTime,
        float speedRatio,
        DirectUI::DurationType naturalDurationType,
        float naturalDuration);

    bool HasRepeat() const { return m_hasRepeat; }
    bool HasReverse() const { return m_hasReverse; }
    _Check_return_ CompositionAnimationConversionResult SetRepeatAndReverse(
        _In_ const xref_ptr<RepeatBehaviorVO::Wrapper> repeat,
        const bool hasReverse,
        DirectUI::DurationType naturalDurationType,
        float naturalDuration);

    float GetDurationWithSpeedRatio() const;

    _Check_return_ CompositionAnimationConversionResult GetNormalizedKeyTime(const float keyTime, _Out_ float* pNormalizedKeyTime);

    _Check_return_ CompositionAnimationConversionResult ApplyProperties(_In_ ixp::IKeyFrameAnimation* animation);

public:
    wrl::ComPtr<ixp::IScalarKeyFrameAnimation> CreateEmptyScalarAnimation();

    wrl::ComPtr<ixp::IScalarKeyFrameAnimation> CreateScalarLinearAnimation(
        float from,
        float to,
        bool isByAnimation,
        float by,
        bool hasHandoff,
        _In_opt_ CEasingFunctionBase* pEasingFunction);

    wrl::ComPtr<ixp::IColorKeyFrameAnimation> CreateEmptyColorAnimation();

    wrl::ComPtr<ixp::IColorKeyFrameAnimation> CreateColorLinearAnimation(
        wu::Color from,
        wu::Color to,
        bool hasHandoff,
        _In_opt_ CEasingFunctionBase* pEasingFunction);

    wrl::ComPtr<ixp::ICompositionEasingFunction> CreateLinearEasingFunction();
    wrl::ComPtr<ixp::ICompositionEasingFunction> CreateSplineEasingFunction(_In_opt_ CKeySpline* keySpline);
    wrl::ComPtr<ixp::ICompositionEasingFunction> CreateDiscreteEasingFunction();
    wrl::ComPtr<ixp::ICompositionEasingFunction> CreateOtherEasingFunction(_In_opt_ CDependencyObject* xamlEasingFunction);

private:
    wrl::ComPtr<ixp::ICompositor> m_compositor;
    wrl::ComPtr<ixp::ICompositionEasingFunctionStatics> m_easingFunctionStatics;

    //
    // Note: Composition allows a maximum of 24 days for DelayTime and for Duration...
    // This is within precision limits of single-precision floating point (23 bits for the mantissa, which is
    // 8,388,607, vs 24*24*60*60 = 2,073,600), so we can safely use "float" for our time measurements.
    //
    const float CompositionMaximumTimeLimit = 24 * 24 * 60 * 60;
    // ... and a minimum of 1ms for Duration. (BeginTime just needs to be positive.)
    const float CompositionMinimumDuration = 0.001f;

    //
    // The combined begin time of all ancestors, with respect to the root timeline.
    // The local speed ratio does not reduce the local begin time, but speed ratios of ancestor timelines do.
    //
    // Note that some BeginTimes have to be rolled into the animation. For example:
    //
    //      <Storyboard BeginTime="0:0:2" RepeatBehavior="3x">
    //          <DoubleAnimation BeginTime="0:0:1" />
    //      </Storyboard>
    //
    // Here, the 2 second delay can be passed to Composition directly. The 1 second delay is inside the looping
    // Storyboard and applies to each iteration, so it must be baked into the key frames of the animation. We track
    // these BeginTimes separately.
    //
    // Note that once we start collecting BeginTimes to be baked into the animation, we can never update the outer
    // BeginTimes again.
    //
    float m_beginTime {0.0f};
    float m_beginTimeInsideAnimation {0.0f};

    // The combined speed ratio of all ancestors. The root timeline has effective speed ratio 1.
    float m_speedRatio {1.0f};

    // The duration to be passed to Composition. Note that this includes the buffer time in m_beginTimeInsideAnimation.
    // We collect this as we walk down the timing tree. Since the timeline parent isn't allowed to clip its children,
    // this duration can only ever get shorter as we walk down the timing tree.
    float m_durationWithSpeedRatio {0.0};
    // Storyboards that hold infinitely looping animations will have an infinite duration. We allow this scenario, as
    // long as the animations themselves do not have an infinite duration.
    bool m_hasInfiniteDuration {true};

    bool m_hasRepeat {false};
    bool m_hasReverse {false};
    bool m_loopsForever {false};
    //
    // Xaml has AutoReverse as well as two ways of specifying finite repeat: counts and durations. Composition has
    // only one property for both reverse and repeat: IterationCount. This creates a couple of different scenarios
    // for converting Xaml to Composition:
    //
    //   1. For a RepeatBehavior of a certain count, we need to multiply the count by 2 to account for AutoReverse:
    //
    //          <Storyboard RepeatBehavior="3x">
    //              <DoubleAnimation Duration="0:0:1" AutoReverse="True" />
    //          </Storyboard>
    //
    //      This needs to become a Composition animation with IterationCount=6 and Direction=Alternate, so that the
    //      iterations run forwards, backwards, forwards, backwards, forwards, backwards.
    //
    //      If the animation inside didn't have an AutoReverse, then we want IterationCount=3 and Direction=Normal
    //      for 3 forward iterations. AutoReverse doubles the IterationCount.
    //
    //   2. For a RepeatBehavior of a certain duration, we do not multiply by anything to account for AutoReverse:
    //
    //          <Storyboard RepeatBehavior="0:0:6">
    //              <DoubleAnimation Duration="0:0:1" AutoReverse="True" />
    //          </Storyboard>
    //
    //      This Storyboard runs for 6 seconds, period. The Composition animation should also have IterationCount=6
    //      and Direction=Alternate.
    //
    //      If the animation inside didn't have an AutoReverse, then we still want IterationCount=6 because of the
    //      duration restriction. We just set Direction=Normal instead. AutoReverse has no effect on the
    //      IterationCount here.
    //
    // We store a duration to handle both of these cases. In the examples above, this would be 6 seconds.
    //
    //    - For case 1, the Storyboard has a natural duration of 2s, because it accounts for AutoReverse on the
    //      DoubleAnimation. We then multiply it by 3 to get 6s. At the animation level, we can divide 6s by the
    //      natural duration of 1s to get IterationCount=6.
    //
    //      If the DoubleAnimation didn't have an AutoReverse, then the Storyboard would have a natural duration
    //      of 1s, and we multiply it by 3 to get 3s. At the animation level this becomes IterationCount=3.
    //
    //    - For case 2, we take the duration specified by the RepeatBehavior directly as the final duration. If the
    //      DoubleAnimation had no AutoReverse, we still use the RepeatBehavior duration of 6s. At the animation
    //      level this becomes IterationCount=6 regardless.
    //
    // Also note that doing things this way allows for fractional repeats. For example,
    //
    //      <Storyboard RepeatBehavior="1.5x">
    //          <DoubleAnimation Duration="0:0:1" AutoReverse="True" />
    //      </Storyboard>
    //
    // This produces a 1-second Composition animation with 3 iterations: forwards, backwards, then forwards. We record
    // a limit of 3s on the Storyboard, which the DoubleAnimation turns into IterationCount=3 later.
    //
    // This field has no meaning if there isn't a reverse or a repeat.
    //
    float m_finalLengthWithRepeatAndReverseAndSpeedRatio {0.0f};
    //
    // If there is a repeat or reverse, we may have to artificially inflate the duration of an animation. The
    // scenario is this:
    //
    //      <Storyboard Duration="0:0:5" RepeatBehavior="3x" AutoReverse="true">
    //          <DoubleAnimation Duration="0:0:3" />
    //      </Storyboard>
    //
    // Here, even though the animation is 3 seconds long, it reverses and repeats on a 5 second period, so we
    // must create an animation for 5 seconds.
    //
    // This field has no meaning if there isn't a reverse or a repeat.
    //
    // The forced duration must be the same for reverse and repeat. In the example above, if AutoReverse was set
    // on the inner animation, then we would be reversing on a period of 3 seconds but repeating on a period of
    // 5 seconds. We cannot create such a corresponding Composition animation.
    //
    // This field is used in conjunction with m_beginTimeInsideAnimation to handle BeginTimes inside a looping
    // Storyboard.
    //
    float m_forcedDurationForRepeatAndReverseWithSpeedRatio {0.0};

    // If an ancestor timeline is dirty, it can force the regeneration of its entire timing subtree. Some examples
    // include the natural duration changing or the speed ratio changing.
    bool m_forceCompositionAnimationDirty {false};

public:
    // Used to tag the WUC animations created by this storyboard. WUC then uses this telemetry to look at power usage.
    xstring_ptr m_storyboardTelemetryName;
};

// Don't log animation conversion failures. They're too noisy and are not an exceptional code path.
// If we see an animation that can't be converted, we'll fall back to dependent animations. This is not considered
// an error.
#define IFC_ANIMATION(x) \
    { \
        CompositionAnimationConversionResult _result_ = (x); \
        __pragma(warning(suppress:4127)) \
        if (_result_ != CompositionAnimationConversionResult::Success) \
        { \
            return _result_; \
        } \
    }