// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Timeline.h"
#include "DCompAnimationConversionContext.h"
#include "RepeatBehavior.h"
#include <TimeMgr.h>
#include <DCompTreeHost.h>
#include "TimeSpan.h"

CTimeline::CTimeline(_In_opt_ CCoreServices *pCore)
    : CDependencyObject(pCore)
    , m_fAutoReverse(false)
    , m_rSpeedRatio(1.0f)
    , m_fillBehavior(DirectUI::FillBehavior::HoldEnd)
    , m_pBeginTime(NULL)
    , m_rCurrentProgress(0.0f)
    , m_rCurrentTime(0.0f)
    , m_clockState(DirectUI::ClockState::Stopped)
    , m_pTargetDependencyProperty(NULL)
    , m_pManualTargetDependencyProperty(NULL)
    , m_strTargetName()
    , m_strTargetProperty()
    , m_nIteration(0)
    , m_fInitialized(FALSE)
    , m_fIsInTimeManager(FALSE)
    , m_IsCompletedEventFired(FALSE)
    , m_pEventList(NULL)
    , m_fReleaseManagedPeer(FALSE)
    , m_hasIndependentAnimation(FALSE)
    , m_completedHandlerRegisteredCount(0)
    , m_pDynamicTimelineParent(NULL)
    , m_hasPendingThemeChange(FALSE)
    , m_isDCompAnimationDirty(true)
    , m_isDCompAnimationDirtyInSubtree(true)
    , m_forceDCompAnimationDirtyInSubtree(true)
    , m_isDCompAnimationInstanceDirty(true)
    , m_shouldAttachDCompAnimationInstance(true)
    , m_isWaitingForDCompAnimationCompleted(false)
    , m_isExpiredWhileWaitingForDCompAnimationCompleted(false)
    , m_hasPendingPauseForDComp(false)
    , m_hasPendingResumeForDComp(false)
    , m_hasPendingSeekForDComp(false)
    , m_shouldSynchronizeDCompAnimationAfterResume(false)
    , m_pendingDCompSeekTime(0)
{
    m_wucAnimationCompletedToken.value = 0;
}

bool CTimeline::IsFilling() const
{
    return m_clockState == DirectUI::ClockState::Filling;
}

_Check_return_ CompositionAnimationConversionResult CTimeline::MakeCompositionAnimationsWithProperties(_Inout_ CompositionAnimationConversionContext* myContext)
{
    // Each timeline tracks the result of trying to create a Composition animation (or, for ParallelTimelines, whether
    // all children were successfully converted). If we have a timeline that can't be converted to a Composition
    // animation, we'll tick it dependently on the UI thread.
    CompositionAnimationConversionResult result = CompositionAnimationConversionResult::Success;

    if (!OfTypeByIndex<KnownTypeIndex::DispatcherTimer>())
    {
        if (m_forceDCompAnimationDirtyInSubtree)
        {
            myContext->SetForceCompositionAnimationDirty(true);
        }

        float beginTime = m_pBeginTime != nullptr ? static_cast<float>(m_pBeginTime->m_rTimeSpan) : 0.0f;
        DirectUI::DurationType durationType;
        float naturalDuration;
        GetNaturalDuration(&durationType, &naturalDuration);
        // "Automatic" durations should have already been resolved when we calculated the natural duration.
        FAIL_FAST_ASSERT(durationType != DirectUI::DurationType::Automatic);

        // Note: The local SpeedRatio doesn't apply to BeginTime but does apply to the duration. Pass everything
        // in at the same time and have the context deal with property ordering.
        result = myContext->ApplyBeginTimeSpeedRatioAndDuration(beginTime, m_rSpeedRatio, durationType, naturalDuration);

        if (result == CompositionAnimationConversionResult::Success
            && (m_repeatBehavior != nullptr || m_fAutoReverse))
        {
            result = myContext->SetRepeatAndReverse(m_repeatBehavior, m_fAutoReverse, durationType, naturalDuration);
        }

        //
        // Do not save the result of the MakeCompositionAnimationVirtual call. This is to handle Storyboard scenarios:
        //
        //      <Storyboard>
        //          <DoubleAnimation1 CannotConvert />
        //          <DoubleAnimation2 />
        //          <DoubleAnimation3 />
        //      </Storyboard>
        //
        // If DoubleAnimation1 failed to create a Composition animation, but 2 and 3 succeed, we don't want to mark the
        // entire Storyboard with a failure, because that will cause the entire timing tree to tick dependently. However,
        // we still want to return the error here. The way we handle it is to save the result before making the virtual
        // call, but report the result of the virtual call. This way, Storyboard can return DoubleAnimation1's error
        // without saving it.
        //
        // The other half of this policy is that CAnimation::MakeCompositionAnimationVirtual is responsible for storing
        // an error, if it encounters one. It's free to stomp over the saved result, because the virtual method will not
        // be called unless everything has succeeded up to that point. Storyboard/ParallelTimeline's override will not
        // store the error, but will still return it.
        //
        // Note that if the Storyboard encounters problems in its properties before walking down into its children, we
        // will store the result here and tick its entire timing subtree dependently.
        //
        m_conversionResult = result;
        if (result == CompositionAnimationConversionResult::Success)
        {
            result = MakeCompositionAnimationVirtual(myContext);
        }
    }

    return result;
}

CompositionAnimationConversionResult CTimeline::MakeCompositionAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext)
{
    return CompositionAnimationConversionResult::Success;
}

// RENDERCHANGEDPFN for the property system
/* static */ void CTimeline::SetForceDCompAnimationSubtreeDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Timeline>());
    CTimeline* pTimeline = static_cast<CTimeline*>(pTarget);

    // Use DirtyFlags::Bounds to indicate that we are forcing the entire subtree to be dirty.
    pTimeline->NWPropagateDirtyFlag(flags | DirtyFlags::Bounds);
}

// RENDERCHANGEDPFN for the property system
/* static */ void CTimeline::SetDCompAnimationDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Timeline>());
    CTimeline* pTimeline = static_cast<CTimeline*>(pTarget);

    pTimeline->MarkIsDCompAnimationDirty();
    // The animation instance dirty flag will be marked when we re-create the animation.
    // The "should reattach animation instance" dirty flag will be marked when we re-create the animation instance.

    // Use something other than DirtyFlags::Bounds - we're not forcing the entire subtree dirty.
    pTimeline->NWPropagateDirtyFlag(flags | DirtyFlags::Render);
}

// RENDERCHANGEDPFN for the property system
/* static */ void CTimeline::SetDCompAnimationInSubtreeDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Timeline>());
    CTimeline* pTimeline = static_cast<CTimeline*>(pTarget);

    // Use something other than DirtyFlags::Bounds - we're not forcing the entire subtree dirty.
    pTimeline->NWPropagateDirtyFlag(flags | DirtyFlags::Render);
}

void CTimeline::NWPropagateDirtyFlag(DirtyFlags flags)
{
    // We use the Bounds dirty flag here to force child timelines to re-create their DComp animations. This is used for
    // cases like the speed ratio changing.
    //
    // We choose Bounds because the default RENDERCHANGEDPFN, CDependencyObject::NWSetRenderDirty, propagates the Render
    // dirty flag. The default function is used for things like From/To/By or key frame values, which shouldn't force
    // the entire subtree dirty. In those cases, we'll go through here with DirtyFlags::Render set.
    if (flags_enum::is_set(flags, DirtyFlags::Bounds))
    {
        MarkForceDCompAnimationDirtyInSubtree();
    }

    MarkIsDCompAnimationDirtyInSubtree();

    __super::NWPropagateDirtyFlag(flags);
}

void CTimeline::MarkIsDCompAnimationDirty()
{
    if (!m_isDCompAnimationDirty)
    {
        m_isDCompAnimationDirty = true;
    }
}

void CTimeline::MarkIsDCompAnimationDirtyInSubtree()
{
    if (!m_isDCompAnimationDirtyInSubtree)
    {
        m_isDCompAnimationDirtyInSubtree = true;
    }
}

void CTimeline::MarkForceDCompAnimationDirtyInSubtree()
{
    if (!m_forceDCompAnimationDirtyInSubtree)
    {
        m_forceDCompAnimationDirtyInSubtree = true;
    }
}

void CTimeline::MarkIsDCompAnimationInstanceDirty()
{
    if (!m_isDCompAnimationInstanceDirty)
    {
        m_isDCompAnimationInstanceDirty = true;
    }
}

void CTimeline::MarkShouldAttachDCompAnimationInstance()
{
    if (!m_shouldAttachDCompAnimationInstance)
    {
        m_shouldAttachDCompAnimationInstance = true;
    }
}

void CTimeline::AttachDCompAnimations()
{
    // Do nothing by default - PointAnimation is not implemented through DComp.
}

void CTimeline::AddPendingDCompPause()
{
    // Do not pass the Pause operation to the DComp animation instance, even if we have one. This animation may be marked
    // dirty, meaning we'll delete the DComp animation (that we've already paused) and re-created it on the next tick (and
    // it will start playing). Instead, mark the pending pause operation so that we can resolve it on the next tick.

    m_hasPendingResumeForDComp = false;
    m_hasPendingPauseForDComp = true;
}

void CTimeline::AddPendingDCompResume()
{
    // Pause and Seek are delayed with pending DComp operations because a dirty flag can cause the current DComp animation
    // to be later replaced. Technically Resume doesn't have this problem - even if the DComp animation is deleted and
    // re-created later, the new DComp animation will be playing as if it was resumed. However, we'll handle Resume with
    // a delay as well (like Pause and Seek) in order to keep the special cases to a minimum.

    m_hasPendingPauseForDComp = false;
    m_hasPendingResumeForDComp = true;

    // When an animation pauses, we disconnect its DComp animations. This is done so that the app can overwrite the animated
    // target with static values. A resume operation can put that animation back into the Active state, in which case we need
    // to reattach the DComp animations. Mark them dirty so that we can do so.
    MarkShouldAttachDCompAnimationInstance();
}

void CTimeline::AddPendingDCompSeek(double globalSeekTime)
{
    // Conveniently, Xaml only allows Seek to be called on root storyboards, meaning the seek time is global and is
    // with respect to the timing root. The DComp animations that are created also have been flattened and have
    // segments added with respect to the timing root, meaning the seek time can be piped through directly, without
    // having to account for various begin times and speed ratios.

    // Do not pass the Seek operation to the DComp animation instance, even if we have one. This animation may be marked
    // dirty, meaning we'll delete the DComp animation (that we've already seeked) and re-created it on the next tick (and
    // it will start again at time 0). Instead, mark the pending seek operation so that we can resolve it on the next tick.
    // Note that SeekAlignedToLastTick will need a DComp seek in addition to this, because it's a synchronous API and should
    // update property values immediately. The DComp seek will put the DComp animations in the correct spot for an animation
    // value readback, and the pending seek will apply the seek again on the next tick in case the old DComp animation was
    // thrown away due to dirtiness.

    // Note: Since animations can be modified while they're playing, this is probably still incorrect. Update a dirty animation
    // and SeekATLT in the same handler - it needs to create new DComp animations immediately for readback to work correctly.
    // TODO: DCompAnim: Add some test for this. The bug itself sounds like it should be Won't-fixed because if how rare it is.

    m_hasPendingSeekForDComp = true;
    m_pendingDCompSeekTime = globalSeekTime;

    // If we just came back from a resume, we could be waiting to seek the DComp animation up to the current Xaml time. If
    // an explicit seek came in before that happens, then don't bother with updating the DComp animation to the Xaml time.
    // We'll just go to the explicit seek time instead.
    m_shouldSynchronizeDCompAnimationAfterResume = false;

    // When an animation expires (and transitions into the Filling or Stopped clock states), we disconnect its DComp animations.
    // A Seek operation can put that animation back into the Active state, in which case we need to reattach the DComp animations.
    // Mark them dirty so that we can do so.
    MarkShouldAttachDCompAnimationInstance();
}

void CTimeline::ClearPendingDCompAnimationOperationsRecursive()
{
    m_hasPendingPauseForDComp = false;
    m_hasPendingResumeForDComp = false;
    m_hasPendingSeekForDComp = false;
}

void CTimeline::ResolvePendingDCompAnimationOperations()
{
}

void CTimeline::ResolvePendingDCompAnimationOperationsRecursive()
{
    ResolvePendingDCompAnimationOperations();
}

void CTimeline::SeekDCompAnimationInstances(double globalSeekTime)
{
}

void CTimeline::SynchronizeDCompAnimationAfterResume(double timeManagerTime)
{
    SeekDCompAnimationInstances(timeManagerTime);
}

// Returns whether the timeline loops infinitely.
bool CTimeline::ExtendDurationWithRepeat(_Inout_ float* pDuration, _In_ const RepeatBehaviorVO& repeatBehavior)
{
    // Account for RepeatBehavior
    switch (repeatBehavior.GetRepeatBehaviorType())
    {
        case DirectUI::RepeatBehaviorType::Forever:
            return true;

        case DirectUI::RepeatBehaviorType::Duration:
            // We are given a specific repeat behavior which either clips our duration or extends our repeat time.
            // In both cases, that is the new value.
            *pDuration = static_cast<float>(repeatBehavior.GetDurationInSec());
            break;

        case DirectUI::RepeatBehaviorType::Count:
        default:
            // If we have a count, use it - this defaults to 1.0
            *pDuration *= repeatBehavior.GetCount();
            break;
    }

    return false;
}

float CTimeline::ExtendDurationWithReverse(float duration, bool hasReverse)
{
    return hasReverse ? (duration * 2) : duration;
}

float CTimeline::AdjustDurationWithSpeedRatio(float duration, float speedRatio)
{
    XCP_FAULT_ON_FAILURE(speedRatio > 0);
    return (speedRatio != 1.0f) ? (duration / speedRatio) : duration;
}

float CTimeline::AdjustDurationWithBeginTime(float duration, _In_opt_ const CTimeSpan* pBeginTime)
{
    return (pBeginTime != nullptr) ? (duration + static_cast<float>(pBeginTime->m_rTimeSpan)) : duration;
}

void CTimeline::GetDurationWithProperties(
    _Out_ DirectUI::DurationType* pDurationType,
    _Out_ float *pDurationValue
    )
{
    float duration;
    DirectUI::DurationType durationType;
    GetNaturalDuration(&durationType, &duration);

    switch (durationType)
    {
        case DirectUI::DurationType::TimeSpan:
        {
            duration = ExtendDurationWithReverse(duration, m_fAutoReverse);

            const bool loopsInfinitely = (m_repeatBehavior) ? ExtendDurationWithRepeat(&duration, m_repeatBehavior->Value()) : false;

            if (loopsInfinitely)
            {
                durationType = DirectUI::DurationType::Forever;
            }
            else
            {
                duration = AdjustDurationWithSpeedRatio(duration, m_rSpeedRatio);

                duration = AdjustDurationWithBeginTime(duration, m_pBeginTime);
            }

            break;
        }

        // This timeline lasts forever, no need to apply properties.
        case DirectUI::DurationType::Forever:
        {
            break;
        }

        // Automatic means that this subtree could not be resolved, which cannot happen:
        //  - either this is an animation, which will default to 1sec if automatic
        //  - or this is another storyboard which will recursively resolve to its child times
        //  - or this is another empty storyboard which will default to 0sec
        case DirectUI::DurationType::Automatic:
        default:
        {
            XCP_FAULT_ON_FAILURE(false);
            break;
        }
    }

    *pDurationType = durationType;
    *pDurationValue = duration;
}

void CTimeline::AttachDCompAnimationInstancesToTarget()
{
    // Do nothing
}

void CTimeline::DetachDCompAnimationInstancesFromTarget()
{
    // Do nothing
}

void CTimeline::PauseDCompAnimationsOnSuspend()
{
    // Do nothing
}

void CTimeline::ResumeDCompAnimationsOnResume()
{
    // Do nothing
}

bool CTimeline::ShouldDiscardDCompAnimationOnDetach()
{
    return false;
}

void CTimeline::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    DetachDCompAnimationInstancesFromTarget();

    MarkForceDCompAnimationDirtyInSubtree();
    MarkIsDCompAnimationInstanceDirty();
    MarkShouldAttachDCompAnimationInstance();

    DetachDCompCompletedHandlerOnStop();
    m_wucAnimationCompletedToken.value = 0;
    m_wucScopedBatch.Reset();
    m_wucAnimator.Reset();
}

void CTimeline::OnDCompAnimationTimeEventStatic(_In_opt_ void* pParam)
{
    // DComp will only report a time event from the DWM if the DComp animation instance is still alive. That in turn
    // guarantees that the Xaml timeline holding on to the DComp animation instance is still alive. If the Xaml timeline
    // is deleted, then it will release the DComp animation instance. DComp will then silently ignore the time event on
    // that animation instance reported by the DWM.

    CTimeline *pTimelineNoRef = reinterpret_cast<CTimeline*>(pParam);
    FAIL_FAST_ASSERT(pTimelineNoRef != nullptr);
    pTimelineNoRef->OnDCompAnimationCompleted();
}

void CTimeline::InitializeIteration()
{
    m_fInitialized = true;

    // Mark the DComp animations as dirty - they'll need to be created and attached. If they read new implicit start
    // values, the new start values will be pushed into the new animations.
    // If this timeline turns out to not be independent, then it will not attempt to create DComp animations, and the
    // dirty flags will remain set.
    CTimeline::SetDCompAnimationDirty(this, DirtyFlags::None /* flags - ignored */);
}

void CTimeline::SetWUCScopedBatch(_In_opt_ WUComp::ICompositionScopedBatch* scopedBatch)
{
    m_wucScopedBatch = scopedBatch;
    SetIsWaitingForDCompAnimationCompleted(scopedBatch != nullptr);
}

void CTimeline::SetWUCAnimator(_In_ WUComp::ICompositionAnimatorPartner* animator)
{
    m_wucAnimator = animator;

    // Flush any pending pause/seek/resume operations now. We couldn't do this when we created the animation because we didn't
    // have an animator yet.
    ResolvePendingDCompAnimationOperations();
}

EventRegistrationToken* CTimeline::GetWUCAnimationCompletedToken()
{
    return &m_wucAnimationCompletedToken;
}

void CTimeline::ResetCompletedEventFired()
{
    m_IsCompletedEventFired = FALSE;
}

void CTimeline::DetachDCompCompletedHandlerOnStop()
{
    if (m_wucAnimationCompletedToken.value != 0)
    {
        ASSERT(m_wucScopedBatch != nullptr);
        GetTimeManager()->DetachWUCCompletedHandler(m_wucScopedBatch.Get());
    }
    // If we're running in WUC mode, detaching the completed handler will clear the "waiting for DComp completed notification"
    // flag. If we're running in legacy DComp mode, clear it explicitly.
    else if (IsWaitingForDCompAnimationCompleted())
    {
        SetIsWaitingForDCompAnimationCompleted(false);
    }
}

void CTimeline::OnDCompAnimationCompleted()
{
    if (IsWaitingForDCompAnimationCompleted())
    {
        SetIsWaitingForDCompAnimationCompleted(false);
    }
}

bool CTimeline::IsWaitingForDCompAnimationCompleted() const
{
    return m_isWaitingForDCompAnimationCompleted;
}

void CTimeline::SetIsWaitingForDCompAnimationCompleted(bool isWaiting)
{
    if (IsWaitingForDCompAnimationCompleted() != isWaiting)
    {
        m_isWaitingForDCompAnimationCompleted = isWaiting;

        if (!m_isWaitingForDCompAnimationCompleted)
        {
            // Request another frame to tick this animation. Now that we're no longer waiting for the completed event from
            // DComp (either we got a completion notification or abandoned it), this animation is free to go into the Filling
            // or Stopped state, and to fire the Xaml timeline Completed event.
            RequestAdditionalFrame();
        }

        // Also check whether the parent Storyboard has any child timelines that are still waiting on DComp.
        // In legacy DComp mode, the parent Storyboards will have created dummy DComp animations that they're waiting on.
        // They don't need to check the state of their children.
        CTimeline* parent = GetTimingParent();
        if (parent != nullptr)
        {
            parent->UpdateIsWaitingForDCompAnimationCompleted(IsWaitingForDCompAnimationCompleted());
        }

        // Reset this flag whenever m_isWaitingForDCompAnimationCompleted changes. Either we've just started waiting, in
        // which case we're not expired, or we've just stopped waiting.
        m_isExpiredWhileWaitingForDCompAnimationCompleted = false;
    }
}

void CTimeline::UpdateIsWaitingForDCompAnimationCompleted(bool childIsWaiting)
{
    UNREFERENCED_PARAMETER(childIsWaiting);

    // Regular timelines have no children. This should never be called except for the root timeline.
    ASSERT(GetTimeManager() == nullptr
        || this == GetTimeManager()->GetRootTimeline());
}

bool CTimeline::IsTopLevelTimeline()
{
    // GetTimingParent() is usually the root timeline created by the time manager, except it can be null in unit tests.
    return GetTimingParent() == nullptr
        || (GetTimeManager() != nullptr     // GetTimeManager() returns null in unit tests
            && GetTimeManager()->GetRootTimeline() == GetTimingParent());
}

void CTimeline::SetTimingParent(_In_ CTimeline* parent)
{
    m_timingParent = xref::get_weakref(parent);
}

CTimeline* CTimeline::GetTimingParent() const
{
    if (!m_timingParent.expired())
    {
        return m_timingParent.lock().get();
    }
    return nullptr;
}

void CTimeline::SetTemplatedParentImpl(_In_ CDependencyObject* parent)
{
    m_templatedParent = xref::get_weakref(parent);
}

CDependencyObject* CTimeline::GetTemplatedParent()
{
    return m_templatedParent.lock_noref();
}

bool CTimeline::CanRequestTicksWhileActive()
{
    // If we've expired while waiting for a DComp animation completed notification to come in, then don't request any more
    // ticks. We'll request a tick after that notification comes in.
    return !m_isExpiredWhileWaitingForDCompAnimationCompleted;
}

xstring_ptr CTimeline::GetTargetPathForTracking(_In_ CDependencyObject* target, bool startFromParentOfTarget)
{
    // Build the details from the UI element path.
    CDependencyObject* parent = target;
    if (startFromParentOfTarget)
    {
        parent = target->GetParentInternal(/*fPublic*/false);
    }

    if (parent != nullptr)
    {
        xstring_ptr path = parent->GetUIPathForTracing(false /* followDOParentChain */);
        return path;
    }
    else
    {
        xstring_ptr blank;
        IGNOREHR(xstring_ptr::CloneBuffer(L"", &blank));
        return blank;
    }
}
