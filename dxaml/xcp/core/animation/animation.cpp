// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Animation.h"
#include "Timemgr.h"
#include "KeyFrame.h"
#include "KeyFrameCollection.h"
#include "KeyTime.h"
#include "TimelineGroup.h"
#include "Storyboard.h"
#include "EasingFunctions.h"
#include "RepeatBehavior.h"
#include "PointerAnimationUsingKeyFrames.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <DCompTreeHost.h>
#include <GraphicsUtility.h>
#include <DXamlServices.h>
#include <DeferredAnimationOperation.h>
#include <UIThreadScheduler.h>
#include "theming\inc\Theme.h"
#include <FeatureFlags.h>
#include "DCompAnimationConversionContext.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;

CAnimation::~CAnimation()
{
    if (m_pKeyFrames)
    {
        m_pKeyFrames->SetTimingOwner(NULL);
    }
    ReleaseInterface(m_pKeyFrames);

    ReleaseInterface(m_pEasingFunction);
}

_Check_return_ HRESULT CAnimation::UpdateAnimation(
    _In_ const ComputeStateParams &parentParams,
    _Inout_ ComputeStateParams &myParams,
    XDOUBLE beginTime,
    DurationType durationType,
    XFLOAT durationValue,
    _In_ const COptionalDouble &expirationTime,
    _Out_ bool *pIsIndependentAnimation
    )
{
    CTimeManager *pTimeManager = GetTimeManager();
    ASSERT(pTimeManager); // needs to be valid, since we are animating.

    bool tickOnUIThread = false;
    bool isIndependentAnimation = false;

    // If we have a target object
    if (GetTargetObjectWeakRef())
    {
        // If we have a weak ref object but no actual object, we need to clear ourselves from
        // the hash table iff we are the active animation on the property.  This helps make
        // sure that animations acting on deleted objects get cleaned up.
        if (GetTargetObjectWeakRef().expired())
        {
            auto pOtherAnimation = pTimeManager->GetAnimationOnProperty(
                GetTargetObjectWeakRef(),
                m_pTargetDependencyProperty->GetIndex());

            if (pOtherAnimation == this)
            {
                // remove ourselves
                pTimeManager->ClearAnimationOnProperty(
                    GetTargetObjectWeakRef(),
                    m_pTargetDependencyProperty->GetIndex());
            }

            // Release the target object if we succeed in removing ourselves from the tree
            ReleaseTarget();
        }
        // If the target is valid and this animation has control of the target, update the animation.
        // If this animation does not have control of the target but is handing off to another animation,
        // update the animation as well. It will not update the target but will pass the handoff value
        // to the other animation.
        else if (m_hasControlOfTarget)
        {
            //
            // Determine whether this animation is independent, and notify target UIElements if so.
            // This must be done every frame, because whether an animation is dependent or not can change
            // dynamically as its target is attached or detached from different contexts (e.g. Brush.Transform
            // vs. UIElement.RenderTransform) or if a target element moves into or out of HW cache.
            //
            // PC doesn't tick all animations.
            // - Don't update or fire events for dependent continuous animations on the UI thread.
            // - All discrete animations and only independent continuous animations can tick on the UI thread.
            //
            // The render thread can tick all its animations since they're all independent.
            IFC_RETURN(ShouldTickOnUIThread(
                myParams.isPaused,
                myParams.cannotConvertToCompositionAnimation,
                &tickOnUIThread,
                &isIndependentAnimation));

            if (tickOnUIThread)      // tick only certain animations on the UI thread for Jupiter.
            {
                // TODO: TICK: Consider preventing UI thread ticks for independent animations in most scenarios, since they're no-ops.
                // TODO: TICK: This should be possible, but state would need to be updated when recovering from idle (e.g. hit-testing).
                switch (m_clockState)
                {
                case DirectUI::ClockState::NotStarted:
                    if (parentParams.pFrameSchedulerNoRef != NULL)
                    {
                        // Request a tick to the active period when running forwards.
                        IFC_RETURN(RequestTickToActivePeriodBegin(parentParams, myParams, beginTime));
                    }

                    break;

                case DirectUI::ClockState::Stopped:
                    // This animation does not need to tick again once stopped.
                    if (m_fInitialized)
                    {
                        if (!m_IsCompletedEventFired)
                        {
                            IFC_RETURN(FireCompletedEvent());
                        }
                    }
                    IFC_RETURN(FinalizeIteration());

                    if (parentParams.pFrameSchedulerNoRef != NULL)
                    {
                        // Request a tick to the active period when running in reverse.
                        IFC_RETURN(RequestTickToActivePeriodEnd(parentParams, myParams, expirationTime));
                    }

                    break;

                case DirectUI::ClockState::Active:
                {
                    if (myParams.isPaused)
                    {
                        TraceTickPausedAnimationHelper();
                    }

                    if (m_fUsesKeyFrames)
                    {
                        IFC_RETURN(UpdateAnimationUsingKeyFrames(!!isIndependentAnimation));

                        if (CanRequestTicksWhileActive() && !myParams.isPaused)
                        {
                            IFC_RETURN(RequestNextTickForKeyFrames(myParams, durationType, durationValue));
                        }
                    }
                    else
                    {
                        if (CanRequestTicksWhileActive() && parentParams.pFrameSchedulerNoRef != NULL && !myParams.isPaused)
                        {
                            // For animations w/o key frames, always request to tick again as soon as possible.
                            IFC_RETURN(parentParams.pFrameSchedulerNoRef->RequestAdditionalFrame(0, RequestFrameReason::AnimationTick));
                        }

                        CValue currentValue;

                        XFLOAT rEasedProgress = CEasingFunctionBase::EaseValue(m_pEasingFunction, m_rCurrentProgress);

                        InterpolateCurrentValue(rEasedProgress);
                        PostInterpolateValues();

                        IFC_RETURN(ValueAssign(currentValue, AssignmentOperand::CurrentValue));

                        IFC_RETURN(DoAnimationValueOperation(
                            currentValue,
                            AnimationValueSet,
                            isIndependentAnimation));
                    }
                    break;
                }

                case DirectUI::ClockState::Filling:
                {
                    // This animation does not need to tick again once filling.
                    if (m_fInitialized)
                    {
                        if (!m_IsCompletedEventFired)
                        {
                            IFC_RETURN(FireCompletedEvent());
                        }

                        // Update the filling frame
                        if (m_fUsesKeyFrames)
                        {
                            IFC_RETURN(UpdateAnimationUsingKeyFrames(!!isIndependentAnimation));
                            // No need to schedule the next frame - this animation is filling so there isn't a next frame.
                        }
                        else
                        {
                            CValue currentValue;

                            XFLOAT rEasedProgress = CEasingFunctionBase::EaseValue(m_pEasingFunction, m_rCurrentProgress);

                            InterpolateCurrentValue(rEasedProgress);
                            PostInterpolateValues();

                            IFC_RETURN(ValueAssign(currentValue, AssignmentOperand::CurrentValue));

                            IFC_RETURN(DoAnimationValueOperation(
                                currentValue,
                                AnimationValueSet,
                                isIndependentAnimation));
                        }
                    }

                    if (parentParams.pFrameSchedulerNoRef != NULL)
                    {
                        // Request a tick to the active period when running in reverse.
                        IFC_RETURN(RequestTickToActivePeriodEnd(parentParams, myParams, expirationTime));
                    }

                    break;
                }

                default:
                    ASSERT(FALSE, L"Unsupported state");
                }
            }
        }
    }

    //
    // There a bit of a chicken-and-egg problem. An animation that can't be converted to a Composition animation
    // will tick dependently as a fallback, so we have to attempt conversion before we can confirm whether an animation
    // is independent. But an animation that's dependent shouldn't be trying to make a Composition animation in the
    // first place, so we have to decide whether an animation is independent before trying to convert.
    //
    // We resolve this by attempting to create a Composition animation for everything. Then we decide whether they're
    // independent, and if they aren't then we go back and release the Composition animation that we created earlier.
    //
    if (!isIndependentAnimation && m_spWUCAnimation != nullptr)
    {
        m_spWUCAnimation.Reset();
    }

    *pIsIndependentAnimation = isIndependentAnimation;

    return S_OK;
}

_Check_return_ HRESULT CAnimation::UpdateAnimationUsingKeyFrames(bool isIndependentAnimation)
{
    CKeyFrame *pKeyFrame = nullptr;
    CValue value;
    const XUINT32 frameCount = m_pKeyFrames != nullptr ? m_pKeyFrames->GetCount() : 0;
    XUINT32 nCurrentSegment = 0;
    XFLOAT rCumulativePercentSpan = 0;
    XFLOAT rSegmentPercentSpan = 0;
    XFLOAT rSegmentProgress = 0;

    if (frameCount == 0)
    {
        return S_OK;
    }

    const auto& sortedKeyFrames = m_pKeyFrames->GetSortedCollection();
    XUINT32 sortedFrameCount = static_cast<XUINT32>(sortedKeyFrames.size());
    FAIL_FAST_ASSERT(sortedFrameCount == frameCount);

#if DEBUG
    // NULL keytimes are invalid, and they are filtered out early when keyframes are initialized

    for (auto& kf : sortedKeyFrames)
    {
        ASSERT((static_cast<CKeyFrame*>(kf))->m_keyTime != nullptr);
    }
#endif

    // Set the end points of the interpolation to the base value
    IFC_RETURN(ValueAssign(AssignmentOperand::From, AssignmentOperand::BaseValue));
    IFC_RETURN(ValueAssign(AssignmentOperand::To, AssignmentOperand::BaseValue));

    // Set the From Value to either the first keyframe or the base value
    pKeyFrame = static_cast<CKeyFrame*>(sortedKeyFrames[0]);
    if (pKeyFrame->m_keyTime->Value().GetPercent() > 0)
    {
        // If the first time is non-zero, we interpolate from the base value
        rSegmentPercentSpan = pKeyFrame->m_keyTime->Value().GetPercent();
    }
    else
    {
        // If the first keyframe is at time=0, we interpolate from the first keyfame
        IFC_RETURN(pKeyFrame->GetValue(m_pDPValue, &value));
        IFC_RETURN(ValueAssign(AssignmentOperand::From, value));
    }

    // Set the To Value to be the first keyframe
    IFC_RETURN(pKeyFrame->GetValue(m_pDPValue, &value));
    IFC_RETURN(ValueAssign(AssignmentOperand::To, value));

    // Per-KeyFrame Segment value computation: check that we are interpolating between the correct
    //      keyframe values.
    while (nCurrentSegment < frameCount
        && static_cast<CKeyFrame*>(sortedKeyFrames[nCurrentSegment])->m_keyTime->Value().GetPercent() <= m_rCurrentProgress)
    {
    // Move to the next segment
        nCurrentSegment++;
        IFC_RETURN(ValueAssign(AssignmentOperand::From, AssignmentOperand::To));
        rCumulativePercentSpan += rSegmentPercentSpan;

    // Check to see if we are at the end
        if (nCurrentSegment < frameCount)
        {
            pKeyFrame = static_cast<CKeyFrame*>(sortedKeyFrames[nCurrentSegment]);
            IFC_RETURN(pKeyFrame->GetValue(m_pDPValue, &value));
            IFC_RETURN(ValueAssign(AssignmentOperand::To, value));
            rSegmentPercentSpan = pKeyFrame->m_keyTime->Value().GetPercent() - rCumulativePercentSpan;
        }
        else
        {
        // We need to hold the last value for the time after the last segment
            pKeyFrame = static_cast<CKeyFrame*>(sortedKeyFrames[frameCount - 1]);
            IFC_RETURN(pKeyFrame->GetValue(m_pDPValue, &value));
            IFC_RETURN(ValueAssign(AssignmentOperand::To, value));

            if (rCumulativePercentSpan < 1.0f)
            {
                rSegmentPercentSpan = 1.0f - rCumulativePercentSpan;
            }
            else
            {
                // We may be at the case that this is the last segment at the end time,
                //   so the effective duration of the interval is zero.  Override this
                //   to prevent a divide by zero later.
                rSegmentPercentSpan = 1.0f;
            }
        }
    }

    // Progress computation: scale the overall progress to this particular keyframe segment
    //      and apply any time-compresion due to keysplines on a per keyframe basis.

    // Compute the linear progress of this segment
    rSegmentProgress = (m_rCurrentProgress - rCumulativePercentSpan) / rSegmentPercentSpan;

    // Now interpolate according to the current keyframe type
    if (nCurrentSegment < frameCount)
    {
        rSegmentProgress = static_cast<CKeyFrame*>(sortedKeyFrames[nCurrentSegment])->GetEffectiveProgress(rSegmentProgress);
    }
    else
    {
    // We are dealing with the last segment or single segment, hold that last value
        rSegmentProgress = static_cast<CKeyFrame*>(sortedKeyFrames[frameCount - 1])->GetEffectiveProgress(rSegmentProgress);
    }

    InterpolateCurrentValue(rSegmentProgress);
    PostInterpolateValues();

    if (GetTargetObjectWeakRef() && !GetTargetObjectWeakRef().expired())
    {
        CValue currentValue;

        IFC_RETURN(ValueAssign(currentValue, AssignmentOperand::CurrentValue));

        IFC_RETURN(DoAnimationValueOperation(
            currentValue,
            AnimationValueSet,
            isIndependentAnimation));
    }

    return S_OK;
}

_Check_return_ HRESULT CAnimation::RequestNextTickForKeyFrames(
    _In_ const ComputeStateParams &myParams,
    DurationType durationType,
    float durationValue
    )
{
    // Schedule frames as needed, with special care to tick infrequently for discrete key frames (i.e. only
    // on the transitions between frames).
    if (myParams.pFrameSchedulerNoRef != nullptr && m_pKeyFrames != nullptr)
    {
        // Key frame animations always resolve their duration to a timespan.
        ASSERT(durationType == DirectUI::DurationType::TimeSpan);

        const auto& sortedKeyFrames = m_pKeyFrames->GetSortedCollection();
        const XUINT32 frameCount = sortedKeyFrames.size();

        FAIL_FAST_ASSERT(frameCount == m_pKeyFrames->GetCount());

        XUINT32 nCurrentSegment = 0;
        while (nCurrentSegment < frameCount && (static_cast<CKeyFrame*>(sortedKeyFrames[nCurrentSegment]))->m_keyTime->Value().GetPercent() <= m_rCurrentProgress)
        {
             nCurrentSegment++;
        }

        // If current progress is within a continuously-interpolated key-frame segment, request an immediate tick.
        if (nCurrentSegment < frameCount && !(static_cast<CKeyFrame*>(sortedKeyFrames[nCurrentSegment]))->IsDiscrete())
        {
            // For continuously-interpolated key frames, always request to tick again as soon as possible.
            IFC_RETURN(myParams.pFrameSchedulerNoRef->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::AnimationTick));
        }
        else
        {
            // Otherwise, we're in a case where we can schedule a future tick, and potentially go idle
            // until the animated value will change again.

            // The next key-time is either the one we're interpolating to or from, depending on the direction
            // time is progressing.
            const XINT32 currentSegment = static_cast<XINT32>(nCurrentSegment);
            const XINT32 targetSegment = myParams.isReversed ? currentSegment - 1 : currentSegment;

            XDOUBLE targetTime;
            if (targetSegment < 0)
            {
                // We're in the first segment running backwards. Target the beginning of the animation.
                ASSERT(myParams.isReversed);
                targetTime = 0.0;
            }
            else if (static_cast<XUINT32>(targetSegment) >= frameCount)
            {
                // We're in the final segment running forwards, beyond the final key-time. The animated value will
                // hold from this point until the Duration has elapsed, so target the end of the animation.
                ASSERT(!myParams.isReversed);
                targetTime = durationValue;
            }
            else
            {
                // The target is one of the key-frame's key-times.
                ASSERT(targetSegment >= 0 && static_cast<XUINT32>(targetSegment) < frameCount);

                XFLOAT keyTimeProgress = (static_cast<CKeyFrame*>(sortedKeyFrames[targetSegment]))->m_keyTime->Value().GetPercent();

                // Convert from progress percentage back in millisecond interval.
                targetTime = keyTimeProgress * durationValue;
            }

            // Calculate the elapsed time from now until the target is reached.
            XDOUBLE tickIntervalInSeconds;
            if (myParams.isReversed)
            {
                ASSERT(m_rCurrentTime >= targetTime);
                tickIntervalInSeconds = m_rCurrentTime - targetTime;
            }
            else
            {
                ASSERT(targetTime >= m_rCurrentTime);
                tickIntervalInSeconds = targetTime - m_rCurrentTime;
            }

            IFC_RETURN(myParams.pFrameSchedulerNoRef->RequestAdditionalFrame(
                XcpCeiling(tickIntervalInSeconds * 1000.0 / myParams.speedRatio),
                RequestFrameReason::AnimationTick));
        }
    }

    return S_OK;
}

// Perform begin time initializations
_Check_return_ HRESULT CAnimation::OnBegin()
{
    HRESULT hr = S_OK;
    xref::weakref_ptr<CDependencyObject> previouslyAnimatedDOWeakRef;
    KnownPropertyIndex previouslyAnimatedPropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
    xref_ptr<CAnimation> pOtherAnimation;
    CTimeManager *pTimeManager = GetTimeManager();

    XFLOAT rNaturalDuration = 0.0f;
    DurationType durationType;

    auto core = GetContext();

    // we are in the middle of an animation - this has to be valid
    ASSERT(pTimeManager);

    // Before we go on to a new target, remove our old one
    if (GetTargetObjectWeakRef() && m_pTargetDependencyProperty != nullptr)
    {
        // See if we are still animating this property
        pOtherAnimation = pTimeManager->GetAnimationOnProperty(
            GetTargetObjectWeakRef(),
            m_pTargetDependencyProperty->GetIndex());

        if (pOtherAnimation == this)
        {
            // save previous DO/DP if we were indeed animating it
            previouslyAnimatedDOWeakRef = GetTargetObjectWeakRef();
            previouslyAnimatedPropertyIndex = m_pTargetDependencyProperty->GetIndex();

            // Remove ourselves from the list
            pTimeManager->ClearAnimationOnProperty(
                GetTargetObjectWeakRef(),
                m_pTargetDependencyProperty->GetIndex());
        }
        pOtherAnimation.reset();
    }

    // We always want to resolve target, even in the case of manually set targets.
    // This may update the target object and target dependency property returned by
    // GetTargetObjectWeakRef and GetTargetDependencyPropertyIndex, respectively.
    IFC(ResolveLocalTarget(core, GetTimingParent()));

    // Validate strictness of the property being animated.  Although we also validate all the
    // way down in UpdateEffectiveValue(), validating here will generate a failure for Storyboard.Start()
    // and will help developers understand the failure if they violate strictness rules.
    IFC(GetTargetObjectWeakRef().lock()->ValidateStrictnessOnProperty(m_pTargetDependencyProperty));

    // We have a valid target, make sure we get the correct base value and
    // do proper handoff behavior.

    // See if anything else is animating this property
    pOtherAnimation = pTimeManager->GetAnimationOnProperty(
        GetTargetObjectWeakRef(),
        m_pTargetDependencyProperty->GetIndex());

    // Set marks and get the ultimate base value
    // Need to get the NonAnimatedBaseValue before resolving default To,From,By
    if (pOtherAnimation)
    {
        // It is not expected that the other animation is this one, since the first thing
        // we do in this function is remove ourselves from the property.
        ASSERT(pOtherAnimation != this);

        // Prevent animation composition in the same timing tree.
        if (pOtherAnimation->GetRootTimingParent() == GetRootTimingParent())
        {
            // Remove this animation from the list
            pTimeManager->ClearAnimationOnProperty(
                GetTargetObjectWeakRef(),
                m_pTargetDependencyProperty->GetIndex());

            // Trigger an error
            if (CTimeManager::ShouldFailFast())
            {
                IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
            }
            else
            {
                IFC(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_BEGIN_ANIM_COMPOSITION));
            }
        }
    }

    //
    // Read the base value from the animation target only, even if there's a pending handoff. Handoffs are only processed
    // when ticking animations. OnBegin is called outside the tick, so the handoff value hasn't been written by the handoff
    // source yet. It's possible that this animation had OnBegin called on it twice in the same frame. The first call could
    // set up a handoff and mark this animation as pending, and the second call must not process the handoff.
    //
    IFC(ReadBaseValuesFromTargetOrHandoff(
        pOtherAnimation,
        previouslyAnimatedDOWeakRef,
        &previouslyAnimatedPropertyIndex
        ));

    // Gather the real duration
    GetDurationForProgress(&durationType, &rNaturalDuration);

    m_isZeroDuration = (rNaturalDuration == 0)
        || (m_repeatBehavior && RepeatBehaviorVOHelper::IsZeroLength(m_repeatBehavior->Value()));

    {
        CTimeline* parentTimeline = GetTimingParent();

        while (parentTimeline != nullptr)
        {
            if (parentTimeline->m_duration && DurationVOHelper::IsZeroLength(parentTimeline->m_duration->Value())
                || parentTimeline->m_repeatBehavior && RepeatBehaviorVOHelper::IsZeroLength(parentTimeline->m_repeatBehavior->Value())
                )
            {
                m_isZeroDuration = true;
                break;
            }

            parentTimeline = parentTimeline->GetTimingParent();
        }
    }

    // Calculate interpolation factors for key-frames, if any.
    if (m_fUsesKeyFrames)
    {
        // We absolutely need this to exist
        IFCPTR(m_pKeyFrames);

        // Make sure our keyframes are initialized to the expected state
        IFC(m_pKeyFrames->InitializeKeyFrames(rNaturalDuration));
    }

    //
    // Always register the new animation on the target object+property, even if there's a queued deferred handoff.
    // The next time that something looks up the animation in the target, it will find this one. This will be used
    // to cancel the pending continuous handoff to this animation.
    //
    IFC(RegisterAnimationOnTarget());

    //
    // Take control of the target, regardless of whether there's a deferred handoff. We want synchronous changes in
    // this new animation to affect the target.
    //
    TakeControlOfTarget(pOtherAnimation);

    {
        CWindowRenderTarget* pRenderTargetNoRef = core->NWGetWindowRenderTarget();
        ASSERT(pRenderTargetNoRef != nullptr);

        DCompTreeHost* pDCompTreeHostNoRef = pRenderTargetNoRef->GetDCompTreeHost();
        ASSERT(pDCompTreeHostNoRef != nullptr);

        if (pDCompTreeHostNoRef->GetMainDevice() != nullptr)
        {
            AttachDCompAnimationInstancesToTarget();
        }
    }

    // We need to reset this flag every time we call Begin on the parent Storyboard
    ResetCompletedEventFired();

    if (EventEnabledAnimationInfo())
    {
        auto pTarget = GetTargetObjectWeakRef().lock();
        xstring_ptr strPropertyName = m_pTargetDependencyProperty->GetName();

        TraceAnimationInfo(reinterpret_cast<UINT64>(pTarget.get()), !strPropertyName.IsNullOrEmpty() ? strPropertyName.GetBuffer() : L"Not Available - Custom Property");
    }

Cleanup:
    if (FAILED(hr))
    {
        // release target weak ref object if begin didn't succeed
        ReleaseTarget();
    }
    RRETURN(hr);
}

_Check_return_ HRESULT CAnimation::ReadBaseValuesFromTargetOrHandoff(
    _In_opt_ CAnimation *pPrecedingAnimation,
    _In_ const xref::weakref_ptr<CDependencyObject>& pPreviouslyAnimatedDOWeakRef,
    _In_opt_ const KnownPropertyIndex *pPreviouslyAnimatedPropertyIndex
    )
{
    // Get the current value of the object+property as the base value.
    IFC_RETURN(GetAnimationBaseValue());

    // The non-animated base value is what will be restored as the property value when the animation ends.
    //
    // If there is a preceding animation handing off to this one, the preceding animation stored the correct base
    // value. Copy it.
    if (pPrecedingAnimation != nullptr)
    {
        CValue precedingAnimationNonAnimatedBaseValue;

        IFC_RETURN(pPrecedingAnimation->ValueAssign(
            precedingAnimationNonAnimatedBaseValue,
            AssignmentOperand::NonAnimatedBaseValue));

        IFC_RETURN(ValueAssign(
            AssignmentOperand::NonAnimatedBaseValue,
            precedingAnimationNonAnimatedBaseValue));

        //
        // The m_hasHandoff flag marks a DComp animation as having a handoff from some previous animation, and the DWM will
        // stitch together the starting value to make a smooth transition. However, not every Xaml animation creates a DComp
        // animation. Animations that have zero duration complete instantly and don't create DComp animations, so even though
        // in Xaml this animation might be picking up from some previous animation B, in DComp there may not be a B' to pick
        // up from, in which case we don't want to set the m_hasHandoff flag to avoid picking up from the wrong animation.
        // So we verify that the previous animation in fact has a duration. If the duration was zero, then the value update
        // happened entirely in Xaml, which means we already have the up-to-date value and there's no need to stitch things
        // together in the DWM.
        //
        // The specific scenario is this: Xaml animation A runs and applies DComp animation A'. Then Xaml animation B runs
        // with zero duration. Then Xaml animation C runs and applies DComp animation C'. If C' was marked with a handoff,
        // it will continue from A' instead of the static value specified by B. In this case, once the instantaneous animation
        // B completes, it tears down the composition node for its target (since the target doesn't have any reasons to have
        // a comp node anymore), which leaves the target's DComp resource with A'. When C begins, the animation target gets
        // a comp node again, and reuses its DComp resource (with A' set on it from before). C' applies to the DComp resource,
        // and DComp detects a handoff from A' to C'. Another solution would be to explicitly update the DComp resource with
        // a static value and detach A' once the comp node isn't needed anymore, but that is a more complicated fix that
        // involves walking the target element's properties and clearing out animations on all the DComp resources. Note that
        // the resources themselves must be kept around so that continuous handoff can work in the normal case.
        //

        m_hasHandoff = !pPrecedingAnimation->IsFilling() && !pPrecedingAnimation->IsZeroDuration();
    }
    // If the animation was not running previously, or it was running but upon restarting has a different target
    // object+property, the current value is the ultimate base value. This value is safe to use as the base value
    // because it is not animated - if it was, there would have been a preceding animation.
    else if (  !pPreviouslyAnimatedDOWeakRef
             || pPreviouslyAnimatedDOWeakRef != GetTargetObjectWeakRef()
             || pPreviouslyAnimatedPropertyIndex == nullptr
             || *pPreviouslyAnimatedPropertyIndex != m_pTargetDependencyProperty->GetIndex())
    {
        auto targetObject = GetTargetObjectWeakRef().lock();

        // There is one exception to the statement above. If there is no preceding animation,
        // it is still possible for the property to be animated. This is the case if the property
        // is being modified by a Setter in a VisualState. In that particular case, using the
        // base value is not safe, and instead, we should use the non-animated base value of
        // the property as the non-animated base value of the animation. For any other cases,
        // using the base value of the animation as the non-animated base value is OK.
        if (targetObject->IsAnimatedProperty(m_pTargetDependencyProperty))
        {
            CValue value;
            IFC_RETURN(targetObject->GetAnimationBaseValue(m_pTargetDependencyProperty, &value));
            IFC_RETURN(ValueAssign(
                AssignmentOperand::NonAnimatedBaseValue,
                value));
        }
        else
        {
            IFC_RETURN(ValueAssign(
                AssignmentOperand::NonAnimatedBaseValue,
                AssignmentOperand::BaseValue));
        }

        m_hasHandoff = false;
    }
    else
    {
        // If the animation began again but is targeting the same property and object, it is handing
        // off to itself. The non-animated base value remains the same as before this animation ever started
        // on that object+property.

        // We're running animations in DComp, and we can't always hand off to ourselves. Suppose 2 calls came in
        // back-to-back without a commit in between. The first call starts the animation for the first time, finds no
        // target, and is marked as not being a handoff. The second call comes in immediately afterwards, finds this
        // animation, which marks it as having a handoff (to itself). Since we never committed in between, we'll create
        // a DComp animation with handoff marked and commit that, and DComp will find no prior animation and assume 0
        // for the handoff value.
        //
        // In this case we shouldn't count the second call as having found a handoff. Since the first call was never
        // committed, it effectively never happened, so we leave the handoff flag alone. We detect this case by explicitly
        // checking the DComp animation instance set on the target. If this animation was never ticked, then it won't have
        // written any DComp animation instance on its target yet. Otherwise, we leave the handoff flag alone.
        CDependencyObject* pDOTargetNoRef = pPreviouslyAnimatedDOWeakRef.lock();
        if (pDOTargetNoRef != nullptr)
        {
            KnownPropertyIndex dcompAnimationProperty = CAnimation::MapToDCompAnimationProperty(*pPreviouslyAnimatedPropertyIndex);
            if (dcompAnimationProperty != KnownPropertyIndex::UnknownType_UnknownProperty
                && pDOTargetNoRef->GetDCompAnimation(dcompAnimationProperty) != nullptr)
            {
                m_hasHandoff = true;
            }
        }
    }

    // Initialize value stores for interpolation. Key-frames don't expose From/To/By, so this is unnecessary for them.
    if (!m_fUsesKeyFrames)
    {
        // For continuous handoffs, it's possible that the app has set an explicit From value before the
        // deferred handoff happened. Don't read the From value again in that case.
        if (IsPropertyDefault(m_pDPFrom))
        {
            IFC_RETURN(ValueAssign(AssignmentOperand::From, AssignmentOperand::BaseValue));
        }
        else
        {
            // If there's an explicit From value, then there is no handoff. This animation starts from that explicit value.
            m_hasHandoff = false;
        }

        if (IsPropertyDefault(m_pDPTo))
        {
            if (IsPropertyDefault(m_pDPBy))
            {
                // If this animation has a default To and no By, go back to non-animated value.
                IFC_RETURN(ValueAssign(AssignmentOperand::To, AssignmentOperand::NonAnimatedBaseValue));
                m_isToAnimation = true; // Count this as an explicit To - handoff won't affect the final value.
            }
            else
            {
                ComputeToUsingFromAndBy();
                m_isToAnimation = false; // This is a By animation - handoff will affect the final value too.
            }
        }
        else
        {
            m_isToAnimation = true;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CAnimation::RegisterAnimationOnTarget()
{
    CTimeManager *pTimeManager = GetTimeManager();

    // Set this animation as the current one on the object+property.
    IFC_RETURN(pTimeManager->SetAnimationOnProperty(
        GetTargetObjectWeakRef(),
        m_pTargetDependencyProperty->GetIndex(),
        this
        ));

    return S_OK;
}

void CAnimation::TakeControlOfTarget(
    _In_ CAnimation *pPreviousAnimation
    )
{
    if (pPreviousAnimation != NULL)
    {
        ASSERT(pPreviousAnimation->m_hasControlOfTarget);
        pPreviousAnimation->ReleaseControlOfTarget();
    }

    m_hasControlOfTarget = true;
}

void CAnimation::ReleaseControlOfTarget()
{
    DetachDCompAnimationInstancesFromTarget();
    m_hasControlOfTarget = false;
}

_Check_return_ HRESULT CAnimation::FinalizeIteration()
{
    if (m_clockState != DirectUI::ClockState::Stopped)
    {
        m_clockState = DirectUI::ClockState::Stopped;
    }

    if (GetTargetObjectWeakRef() && m_pTargetDependencyProperty != nullptr)
    {
        // Get the time manager
        CTimeManager *pTimeManager = GetTimeManager();
        // we are in the middle of an animation - this has to be valid
        ASSERT(pTimeManager);

        // See if we are animating this property
        auto pOtherAnimation = pTimeManager->GetAnimationOnProperty(
            GetTargetObjectWeakRef(),
            m_pTargetDependencyProperty->GetIndex());

        if (pOtherAnimation == this)
        {
            // Remove ourselves from the list
            pTimeManager->ClearAnimationOnProperty(
                GetTargetObjectWeakRef(),
                m_pTargetDependencyProperty->GetIndex());
        }

        // only set value if initialized
        if (m_fInitialized && !GetTargetObjectWeakRef().expired())
        {
            CValue dummy;

            // AnimationValueClear resets to base value if a HoldEnd value is not
            // provided.
            IFC_RETURN(DoAnimationValueOperation(
                dummy, // pValue (No HoldEnd value)
                AnimationValueClear,
                FALSE /*isValueChangeIndependent*/
                ));
        }
    }

    // Once the animation stops, mark it as no longer having control over the target. Otherwise, when this animation
    // starts again, it will detach animation instances from its old target via OnBegin -> ResolveLocalTarget ->
    // ReleaseTarget. Those animation instance may now belong to some other Xaml animation altogether.
    ReleaseControlOfTarget();

    IFC_RETURN(CTimeline::FinalizeIteration());

    return S_OK;
}

// Resolve target name and target property
_Check_return_ HRESULT CAnimation::ResolveLocalTarget(
    _In_ CCoreServices *pCoreServices,
    _In_opt_ CTimeline *pParentTimeline
    )
{
    HRESULT hr = S_OK;

    IFC( CTimeline::ResolveLocalTarget( pCoreServices, pParentTimeline ) )

    // If we haven't resolved these yet, then we should report an error here
    if (!GetTargetObjectWeakRef() || m_pTargetDependencyProperty == nullptr)
    {
        if (CTimeManager::ShouldFailFast())
        {
            IFCFAILFAST(static_cast<HRESULT>(E_NER_INVALID_OPERATION));
        }
        else
        {
            IFC(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_BEGIN_NO_TARGET));
        }
    }

    // Now we verify that our target accepts our type
    // (validation for custom properties happens during SetValue)
    if (!m_pTargetDependencyProperty->Is<CCustomDependencyProperty>())
    {
        bool bIsIncompatibleType = FALSE;

        if (OfTypeByIndex<KnownTypeIndex::DoubleAnimation>() ||
            OfTypeByIndex<KnownTypeIndex::DoubleAnimationUsingKeyFrames>() ||
            OfTypeByIndex<KnownTypeIndex::PointerAnimationUsingKeyFrames>()
            )
        {
            if (m_pTargetDependencyProperty->GetPropertyType()->m_nIndex != KnownTypeIndex::Double)
            {
                bIsIncompatibleType = TRUE;
            }
        }
        else if (OfTypeByIndex<KnownTypeIndex::PointAnimation>()
            || OfTypeByIndex<KnownTypeIndex::PointAnimationUsingKeyFrames>())
        {
            if (m_pTargetDependencyProperty->GetPropertyType()->m_nIndex != KnownTypeIndex::Point)
            {
                bIsIncompatibleType = TRUE;
            }
        }
        else if (OfTypeByIndex<KnownTypeIndex::ColorAnimation>()
            || OfTypeByIndex<KnownTypeIndex::ColorAnimationUsingKeyFrames>())
        {
            if (m_pTargetDependencyProperty->GetPropertyType()->m_nIndex != KnownTypeIndex::Color)
            {
                bIsIncompatibleType = TRUE;
            }
        }
        else if (OfTypeByIndex<KnownTypeIndex::ObjectAnimationUsingKeyFrames>())
        {
            // No restrictions here
        }
        else
        {
            // Unrecognized type of animation
            IFC(E_FAIL);
        }

        if (bIsIncompatibleType)
        {
            // if type is incompatible, create error and parameters and return error HR
            HRESULT hrToOriginate = E_NER_INVALID_OPERATION;
            xephemeral_string_ptr parameters[2];

            GetClassName().Demote(&parameters[0]);
            m_pTargetDependencyProperty->GetName().Demote(&parameters[1]);

            if (CTimeManager::ShouldFailFast())
            {
                IFCFAILFAST(hrToOriginate);
            }
            else
            {
                IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_RUNTIME_SB_BEGIN_INCOMPATIBLE_TYPE, 2, parameters));
                IFC(hrToOriginate);
            }
        }
    }

Cleanup:
    if ( FAILED(hr) )
    {
        // release target weak ref object if resolution didn't succeed
        ReleaseTarget();
    }
    RRETURN(hr);
}

// Override to base SetValue to allow adding a child timeline
_Check_return_ HRESULT CAnimation::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP == m_pDPTo || args.m_pDP == m_pDPBy || args.m_pDP == m_pDPFrom)
    {
        // For a Keyframe type animation, make sure we don't set
        // the non-keyframe properties
        if (m_fUsesKeyFrames)
        {
            IFC_RETURN(E_INVALIDARG);
        }
    }

    // Animation doesn't like to be null, so we set just replace the null
    // with the dummy object to fool SetValue
    if (IsAnimationValueNull(args.m_pDP, args.m_value))
    {
        // Can't call ClearValue() directly because it calls SetValue(),
        //  resulting in stack overflow.  SetPropertyIsDefault() is sufficient
        //  for us because animations can't be styled.
        SetPropertyIsDefault(args.m_pDP);
    }
    else
    {
        // Default is to let base class handle this
        IFC_RETURN(CTimeline::SetValue(args));

        // if we just set a keyframe collection, set the timing parent on that
        if (args.m_pDP->GetIndex() == KnownPropertyIndex::DoubleAnimationUsingKeyFrames_KeyFrames ||
            args.m_pDP->GetIndex() == KnownPropertyIndex::ColorAnimationUsingKeyFrames_KeyFrames ||
            args.m_pDP->GetIndex() == KnownPropertyIndex::PointAnimationUsingKeyFrames_KeyFrames ||
            args.m_pDP->GetIndex() == KnownPropertyIndex::ObjectAnimationUsingKeyFrames_KeyFrames)
        {
            // if setting the keyframes property succeeded, then set timing owner of the collection to be us
            ASSERT(m_pKeyFrames);
            if (m_pKeyFrames)
            {
                m_pKeyFrames->SetTimingOwner(this);
            }
        }
    }

    return S_OK;
}

// Override to base GetValue, mainly to return a null for default
// From/By/To values to simulate behavior of nullable types.
_Check_return_ HRESULT CAnimation::GetValue(
    _In_ const CDependencyProperty *pdp,
    _Inout_ CValue *pValue)
{
    if ( ( pdp == m_pDPTo ||
           pdp == m_pDPBy ||
           pdp == m_pDPFrom ) &&
         IsPropertyDefault(pdp) )
    {
        pValue->SetObjectNoRef(nullptr);
    }
    else
    {
        IFC_RETURN(CDependencyObject::GetValue(pdp, pValue));
    }

    return S_OK;
}

// Update animated value to new theme
_Check_return_ HRESULT CAnimation::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh)
{
    CTimeline *pTimingParent = NULL;

    // Change animation's properties to new theme
    IFC_RETURN(CTimeline::NotifyThemeChangedCore(theme, fForceRefresh));

    // Request a new tick to process theme change.
    if (m_fInitialized
        && GetTargetObjectWeakRef()
        && GetTargetObjectWeakRef().lock())
    {
        pTimingParent = GetTimingParent();
        if (pTimingParent)
        {
            IFC_RETURN(pTimingParent->RequestTickForPendingThemeChange());
        }
    }

    return S_OK;
}

// Set/Clear animated value to the DP of a DO.  But if this is a custom
// property, we need to set it asynchronously to avoid re-entrancy.
_Check_return_ HRESULT CAnimation::DoAnimationValueOperation(
    _In_ CValue& value,
    AnimationValueOperation operation,
    bool isValueChangeIndependent
    )
{
    const bool shouldStoreSourceInfo = DXamlServices::ShouldStoreSourceInformation();
    HRESULT hr = S_OK;
    bool peggedPeer = false;
    bool peerIsPendingDelete = false;

    auto pDO = GetTargetObjectWeakRef().lock();

    // Either we're setting a non-null value, or we're clearing.
    ASSERT((operation == AnimationValueClear) ^ (!value.IsUnset()));

    //If XamlDiagnostics is enabled, we should keep a strong reference to this CAnimation
    //for source info purposes
    xref_ptr<CDependencyObject> sourceInfo = nullptr;
    if (shouldStoreSourceInfo)
    {
        sourceInfo = xref_ptr<CDependencyObject>(this);
    }

    if (m_hasControlOfTarget)
    {
        // Animation can run while the target object's delete is pending. Although the
        // core object is currently alive, the peer may have been deleted or may be
        // queued for release in UIAffinityReleaseQueue. No-op if delete is
        // pending. It peer exists and is valid, peg it during this operation.
        pDO->TryPegPeer(&peggedPeer, &peerIsPendingDelete);
        if (peerIsPendingDelete)
        {
            goto Cleanup;
        }

        // For core properties, just set the property directly
        if (!m_pTargetDependencyProperty->Is<CCustomDependencyProperty>())
        {
            if (operation == AnimationValueSet)
            {
                ASSERT(!value.IsUnset());

                // For independent animations, the composition thread handles rendering animated values.
                // The UI thread still updates property values, but the updates do _not_ need to propagate dirty
                // flags - generating a UI thread frame for an independent value change is wasted work.
                value.GetCustomData().SetIsIndependent(!!isValueChangeIndependent);

                if (EventEnabledSetAnimationBegin() && !isValueChangeIndependent)
                {
                    TraceAnimation(operation, pDO, true /*startOfTrace*/);
                }

                //If XamlDiagnostics is enabled, keep a strong reference to this animation so it can get
                //the source info
                hr = pDO->SetAnimatedValue(m_pTargetDependencyProperty, value, sourceInfo);

                if (EventEnabledSetAnimationEnd() && !isValueChangeIndependent)
                {
                    TraceAnimation(operation, pDO, false /*startOfTrace*/);
                }
            }
            else
            {
                // TODO: Do we need to generate a UI thread frame when clearing the last value?  Currently we never set isValueChangeIndependent when clearing
                ASSERT(!isValueChangeIndependent);

                ASSERT(operation == AnimationValueClear);

                if (EventEnabledSetAnimationBegin() && !isValueChangeIndependent)
                {
                    TraceAnimation(operation, pDO, true /*startOfTrace*/);
                }

                hr = pDO->ClearAnimatedValue(m_pTargetDependencyProperty, value);

                if (EventEnabledSetAnimationEnd() && !isValueChangeIndependent)
                {
                    TraceAnimation(operation, pDO, false /*startOfTrace*/);
                }
            }

            if (FAILED(hr))
            {
                HRESULT hrToOriginate = E_NER_ARGUMENT_EXCEPTION;
                xephemeral_string_ptr parameters[2];

                ASSERT(parameters[0].IsNull());
                m_pTargetDependencyProperty->GetName().Demote(&parameters[1]);

                if (CTimeManager::ShouldFailFast())
                {
                    IFCFAILFAST(hrToOriginate);
                }
                else
                {
                    IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_PARSER_INVALID_ATTR_VALUE, 2, parameters));
                    IFC(hrToOriginate);
                }
            }
        }
        // For custom properties, queue the call to set the property, so that we're
        // not calling out into app code until we're in a safe state.
        else
        {
            // Only create and queue the deferred animation if the managed peer is still available.
            // If the peg succeeded, we know the managed peer is available.
            if (peggedPeer)
            {
                // Queue the deferred operation
                GetContext()->EnqueueDeferredAnimationOperation(
                    std::make_shared<CDeferredAnimationOperation>(
                        std::make_pair(pDO, m_pTargetDependencyProperty->GetIndex()),
                        value,
                        !!peggedPeer,
                        (operation == AnimationValueSet)
                        ? CDeferredAnimationOperation::DeferredOperation::Set
                        : CDeferredAnimationOperation::DeferredOperation::Clear,
                        sourceInfo));

                // CDeferredAnimationOperation's dtor will unpeg
                peggedPeer = FALSE;
            }
        }
    }

Cleanup:
    if (peggedPeer)
    {
        pDO->UnpegManagedPeer();
    }

    RRETURN(hr);
}

CDOCollection *CAnimation::GetKeyFrameCollection()
{
    return m_pKeyFrames;
}

// Returns TRUE if this animation could not be independently animated but is allowed
// to animate by default anyway.
bool CAnimation::IsAllowedDependentAnimation()
{
    // Object key frame animations could never interpolate smoothly and (typically) don't
    // target properties that can be independently animated.  Since they're mostly useful
    // for UI logic changes, allowing them to run on the UI thread by default is okay.  The
    // same is true for zero-duration animations, which are often used as 'setters' in XAML.
    // A zero-duration animation could never be independently animated since it would be
    // complete on the UI thread frame that picked it up.  As such, don't bother setting up
    // the data structures to allow it to independently-animate even if the target property
    // were on the accepted list.
    // Duration defaults to 1-second if it isn't set - only capture zero-duration animations here.
    return (m_duration && DurationVOHelper::IsZeroLength(m_duration->Value()))
        || (m_fUsesKeyFrames && OfTypeByIndex<KnownTypeIndex::ObjectAnimationUsingKeyFrames>());
}

// Check the element that uses the specified target object. If the
// element is under a bitmap cache, then the animation targeting the
// target object can't be independent.
_Check_return_ HRESULT CAnimation::CheckElementUsingAnimatedTarget(
    _In_ CDependencyObject *pTargetObject,
    _Inout_ IATargetVector *pIATargets,
    _Inout_ bool *pIsIndependentAnimation
    )
{
    bool isIndependentAnimation = *pIsIndependentAnimation;

    auto parentCount = pTargetObject->GetParentCount();
    for (size_t i = 0; i < parentCount; i++)
    {
        CDependencyObject *pParent = pTargetObject->GetParentItem(i);

        if (pParent)
        {
            IFC_RETURN(FindIndependentAnimationTargetsRecursive(
                pParent,
                pTargetObject,
                pIATargets,
                &isIndependentAnimation
                ));

            // If we encounter any dependent consumer of this animation, treat the entire animation as dependent. The
            // exception is if we're targeting a TransitionTarget - see block below.
            if (!isIndependentAnimation)
            {
                break;
            }
        }
    }

    //
    // If the animation target is rooted in a TransitionTarget, consider it independent. The case we want to allow is a
    // standalone TransitionTarget that isn't publicly associated with a UIElement, which is used by windowed popups.
    // Note that we make this check on the way back down the chain of recursive calls. We don't want to short circuit on
    // the way up the tree because we still want to find and notify the animating UIElement if one exists.
    //
    // Note that we can also put this logic in the loop above, effectively allowing TransitionTargets to be considered
    // independent and continuing on to check the rest of the parents. In practice since TransitionTargets are internal
    // objects, the only animations that target things inside them are internally generated animations, which we assume
    // to be independent without making further checks.
    //
    if (!isIndependentAnimation && pTargetObject->OfTypeByIndex<KnownTypeIndex::TransitionTarget>())
    {
        isIndependentAnimation = true;

        IATarget target;
        // The animation types are handled at the UIElement level, not the DO level.
        // Target DOs of other types just get a generic notification that it is being independently animated.
        target.animationType = IndependentAnimationType::None;
        target.targetWeakRef = xref::get_weakref(pTargetObject);
        pIATargets->push_back(target);
    }

    *pIsIndependentAnimation = isIndependentAnimation;
    return S_OK;
}

// Find the element that is a valid target for primitive composition
_Check_return_ HRESULT CAnimation::FindIndependentAnimationTargets(
    _In_ CDependencyObject *pTargetObject,
    _In_opt_ CDependencyObject *pSender,
    _Inout_opt_ IATargetVector *pIATargets,
    _Out_ bool *pIsIndependentAnimation
    )
{
    bool isIndependentAnimation;
    IFC_RETURN(FindIndependentAnimationTargetsRecursive(
        pTargetObject,
        pSender,
        pIATargets,
        &isIndependentAnimation
        ));

    if (pIATargets != NULL)
    {
        // After finding all potential targets, apply one additional check.
        // Some consumers (like a ResourceDictionary) are allowed as long as all other consumers
        // are independent, but this animation should only be independent if there are other consumers.
        isIndependentAnimation = isIndependentAnimation && !pIATargets->empty();

        // Also, ensure the direct target object itself is in the list of IATargets. All UIElement targets were already
        // added by the search above, but if the target was any other type it should be added here as well.
        if (isIndependentAnimation && !pTargetObject->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            IATarget target;

            // The animation types are handled at the UIElement level, not the DO level.
            // Target DOs of other types just get a generic notification that it is being independently animated.
            target.animationType = IndependentAnimationType::None;
            target.targetWeakRef = xref::get_weakref(pTargetObject);
            pIATargets->push_back(target);
        }
    }

    *pIsIndependentAnimation = isIndependentAnimation;
    return S_OK;
}

_Check_return_ HRESULT CAnimation::FindIndependentAnimationTargetsRecursive(
    _In_ CDependencyObject *pTargetObject,
    _In_opt_ CDependencyObject *pSender,
    _Inout_opt_ IATargetVector *pIATargets,
    _Out_ bool *pIsIndependentAnimation
    )
{
    // Default to dependent.
    *pIsIndependentAnimation = FALSE;
    RRETURN(S_OK);
}

// Determines if an animation can run independently or should be ticked on the UIThread.
// Notify target elements if they are being independently-animated this frame.
_Check_return_ HRESULT CAnimation::ShouldTickOnUIThread(
    bool isStoryboardPaused,
    bool cannotConvertToCompositionAnimation,
    _Out_ bool *pTickOnUIThread,
    _Out_ bool *pIsIndependentAnimation)
{
    bool tickOnUIThread = false;
    bool isIndependentAnimation = false;

    // TODO: HWPC: The logic used to attach to targets verifies the animation can be independent, but so does CloneForComposition
    if (!GetTargetObjectWeakRef().expired()
        && GetTargetObjectWeakRef().lock()  // the target still exists...
        && m_pTargetDependencyProperty != nullptr)  // we have a target property...
    {
        // If the animation is not continuous, it's okay to treat it as independent since there's no 'smoothness'
        // sacrificed by running it on the UI thread.  These animations will not run on the render thread, so
        // skip the walk up looking for IA targets.
        if (IsAllowedDependentAnimation())
        {
            tickOnUIThread = TRUE;
        }
        else
        {
            IATargetVectorWrapper iaTargets;

            // For continuous animations, only those that qualify as independent will be ticked on the UI thread.
            IFC_RETURN(FindIndependentAnimationTargets(
                GetTargetObjectWeakRef().lock(),
                NULL /* sender, for recursive calls */,
                &iaTargets.m_vector, // NOTE: WeakRefs in this list are owned by this method
                &isIndependentAnimation
                ));

            if (isIndependentAnimation)
            {
                // Independent animations should tick on the UI thread.  Dependent animations should not.
                tickOnUIThread = TRUE;

                // If we were unable to convert this animation (or an ancestor timeline) to a Composition animation,
                // then we'll allow it to be ticked on the UI thread as a fallback. This animation is no longer
                // considered to be independent. This means the animation target will no longer be marked as being
                // independently animated, which skips us creating Composition expressions and hooking up the
                // Composition animation.
                // Note: This means Xaml is free to re-rasterize every frame of a scale animation again.
                if (m_conversionResult != CompositionAnimationConversionResult::Success
                    || cannotConvertToCompositionAnimation)
                {
                    isIndependentAnimation = FALSE;
                }
                // Animations in the Filling state are generally treated as independent so that they can
                // repeat smoothly, without waiting for a UI thread frame to kick off the animation again.
                // However, there are optimizations in the render walk that change depending on what types of
                // animations are active - e.g. text won't re-realize while the scale is changing, and zero opacity
                // content will be rendered if the opacity is animating to prevent glitching.
                //
                // We don't need to generate extra UI thread frames every time an animation flips to Filling and
                // back to re-evaluate realizations and these other optimizations. Instead, an animation is treated
                // as independent until its entire timing tree expires, i.e. the root Storyboard completes, or the root
                // Storyboard is Paused. The final tick here will be treated as dependent to allow realizations to update.
                // A render walk would already be needed this frame anyway, since the compositor tree will be updated for
                // the expiring independent animation.
                //
                // As a perf optimization this is most efficient because it invalidates the tree as little as possible, but
                // there are some behavioral implications for generating new realizations:
                //
                // Ex. DoubleAnimation of a ScaleTransform affecting a TextBlock
                // 1. DoubleAnimation.Duration == Storyboard.Duration
                //     The last animation tick on the ScaleTransform will be 'dependent' to allow for re-realization.
                // 2. DoubleAnimation.Duration < Storyboard.Duration
                //     The animation tick transitioning to Filling will still be independent. A final tick when the Storyboard
                //     itself expires will tick the animation again and allow for re-realization.
                // 3. DoubleAnimation.Duration > Storyboard.Duration
                //     The animation will never transition to Filling itself, but the final tick still needs to be dependent
                //     to allow for re-realization.
                // 4. RepeatBehavior.Count > 1
                //     The animation will transition to Filling multiple times, but the 'dependent' tick will occur only at
                //     the end of the last iteration.
                // 5. RepeatBehavior == Forever
                //     There will never be a 'dependent' tick of the animation, so the TextBlock will never be allowed
                //     to re-realize.
                //
                // Note that we only allow this final dependent tick if the animation qualified as independent - we still
                // want animations that don't qualify to no-op.
                //
                // Normally, when an animation is paused, we detach the DComp animation and fall back to a static value. However,
                // with independent animations ticking in the DWM, we don't know what that static value is. For RS2+, we'll leave
                // paused animations (with the paused WUC animators) attached to the DComp tree. That requires keeping paused Xaml
                // animations marked as independent.
                else if (isIndependentAnimation &&
                    (!isStoryboardPaused && !GetRootTimingParent()->IsInActiveState()))
                {
                    isIndependentAnimation = FALSE;
                }
                // Mark that the target elements need composition nodes if this is a Tick() that will be followed by a Draw().
                // Even if the animation itself hasn't started or is filling, we'll clone the state over so that the animation can
                // resume being active independently of the UI thread.
                else if (GetTimeManager()->ShouldCollectIATargets())
                {
                    ASSERT(!iaTargets.m_vector.empty());

                    GetTimeManager()->CollectIATargets(&iaTargets.m_vector);
                }
            }
        }

        // If the animation would normally be treated as dependent, it can still be ticked on the UI thread
        // if dependent animations are allowed and this animation was specifically opt-ed in.
        if (!tickOnUIThread && s_allowDependentAnimations && m_enableDependentAnimation)
        {
            tickOnUIThread = TRUE;
        }

        // If the animation doesn't qualify as independent, report it on the first tick that's ignored.
        //
        // The animation being judged dependent or not can change based on the state of the tree from
        // frame to frame while the animation is running. We keep a state bit to prevent warning output
        // every tick, and to also catch cases where the animation changes from independent to dependent.
        if (tickOnUIThread)
        {
            m_fFiredDependentAnimationWarning = false;
        }
        else if (!m_fFiredDependentAnimationWarning)
        {
            auto targetObject = GetTargetObjectWeakRef().lock();
            if (targetObject->GetParentCount() == 0)
            {
                // We regularly see warnings for Transform and Projection objects that aren't currently parented.
                // Skip warnings if the target isn't parented, which usually means the animation was started and
                // then target was removed from the tree before the first tick.
            }
            else
            {
                xstring_ptr propertyName = m_pTargetDependencyProperty->GetName();

                IGNOREHR(gps->DebugOutputSzNoEndl(
                    L"WARNING: Animation of \"%s\" on \"%s\" is not independent and will be skipped\r\n",
                    !propertyName.IsNullOrEmpty() ? propertyName.GetBuffer() : L"[Custom Property]",
                    targetObject->GetDebugLabel().GetBuffer()
                    ));
            }

            m_fFiredDependentAnimationWarning = true;
        }
    }

    *pTickOnUIThread = tickOnUIThread;
    *pIsIndependentAnimation = isIndependentAnimation;

    return S_OK;
}

_Check_return_ HRESULT CAnimation::GetAnimationTargets(
    _Out_ xref::weakref_ptr<CDependencyObject>* ppTargetRef,
    _Outptr_ const CDependencyProperty** ppProperty)
{
    if (m_pTargetDependencyProperty == nullptr)
    {
        // we always expect these to be in synch
        ASSERT(!GetTargetObjectWeakRef());

        // This goes up the Timeline tree to resolve any targets.
        IFC_RETURN(ResolveLocalTarget(GetContext(), GetTimingParent()));
    }
    *ppTargetRef = GetTargetObjectWeakRef();
    *ppProperty = m_pTargetDependencyProperty;

    return S_OK;
}

// Fire Complete event for the animation when it has a target object
// and it is ticking on the UI thread.
_Check_return_ HRESULT CAnimation::FireCompletedEvent()
{
    ASSERT(!IsCompletedEventFired());

    bool tickOnUIThread = false;
    bool isIndependentAnimation_dontCare = false;

    IFC_RETURN(ShouldTickOnUIThread(
        false /* isStoryboardPaused - used to determine isIndependentAnimation */,
        false /* cannotConvertToCompositionAnimation - affects whether the animation is considered independent */,
        &tickOnUIThread,
        &isIndependentAnimation_dontCare));

    if (GetTargetObjectWeakRef() && tickOnUIThread)
    {
         IFC_RETURN(CTimeline::FireCompletedEvent());
    }
    return S_OK;
}

void CAnimation::TraceAnimation(_In_ const AnimationValueOperation &operation, _In_ CDependencyObject * pDO, _In_ bool startOfTrace)
{
    auto pUIElement = do_pointer_cast<CUIElement>(pDO);
    if (pUIElement)
    {
        auto pParent = pDO->GetUIElementParentInternal();
        auto propertyName = m_pTargetDependencyProperty->GetName();

        if (operation == AnimationValueSet && startOfTrace)
        {
            TraceSetAnimationBegin(reinterpret_cast<XUINT64>(pDO), reinterpret_cast<XUINT64>(pParent), propertyName.GetBuffer());
        }
        else if (operation == AnimationValueSet)
        {
            TraceSetAnimationEnd(reinterpret_cast<XUINT64>(pDO), reinterpret_cast<XUINT64>(pParent), propertyName.GetBuffer());
        }
        else if (startOfTrace)
        {
            TraceClearAnimationBegin(reinterpret_cast<XUINT64>(pDO), reinterpret_cast<XUINT64>(pParent), propertyName.GetBuffer());
        }
        else
        {
            TraceClearAnimationEnd(reinterpret_cast<XUINT64>(pDO), reinterpret_cast<XUINT64>(pParent), propertyName.GetBuffer());
        }
    }
}
