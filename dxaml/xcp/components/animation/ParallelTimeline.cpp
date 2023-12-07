// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ParallelTimeline.h"
#include "DCompAnimationConversionContext.h"
#include "Duration.h"
#include "TimelineCollection.h"
#include <DependencyObjectDCompRegistry.h>
#include <XamlBehaviorMode.h>

CParallelTimeline::~CParallelTimeline()
{
    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->UnregisterObject(this);
    }
}

void CParallelTimeline::AttachDCompAnimations()
{
    if (m_pChild != nullptr)
    {
        for (auto item : *m_pChild)
        {
            static_cast<CTimeline*>(item)->AttachDCompAnimations();
        }
    }
}

void CParallelTimeline::AddPendingDCompPause()
{
    __super::AddPendingDCompPause();

    if (m_pChild != nullptr)
    {
        for (auto item : *m_pChild)
        {
            static_cast<CTimeline*>(item)->AddPendingDCompPause();
        }
    }
}

void CParallelTimeline::AddPendingDCompResume()
{
    __super::AddPendingDCompResume();

    if (m_pChild != nullptr)
    {
        for (auto item : *m_pChild)
        {
            static_cast<CTimeline*>(item)->AddPendingDCompResume();
        }
    }
}

void CParallelTimeline::AddPendingDCompSeek(double globalSeekTime)
{
    __super::AddPendingDCompSeek(globalSeekTime);

    if (m_pChild != nullptr)
    {
        for (auto item : *m_pChild)
        {
            static_cast<CTimeline*>(item)->AddPendingDCompSeek(globalSeekTime);
        }
    }
}

void CParallelTimeline::ClearPendingDCompAnimationOperationsRecursive()
{
    __super::ClearPendingDCompAnimationOperationsRecursive();

    if (m_pChild != nullptr)
    {
        for (auto item : *m_pChild)
        {
            static_cast<CTimeline*>(item)->ClearPendingDCompAnimationOperationsRecursive();
        }
    }
}

void CParallelTimeline::ResolvePendingDCompAnimationOperationsRecursive()
{
    // Base implementation is responsible for calling the non-recursive version on "this".
    __super::ResolvePendingDCompAnimationOperationsRecursive();

    if (m_pChild != nullptr)
    {
        for (auto item : *m_pChild)
        {
            static_cast<CTimeline*>(item)->ResolvePendingDCompAnimationOperationsRecursive();
        }
    }
}

void CParallelTimeline::SeekDCompAnimationInstances(double globalSeekTime)
{
    if (m_pChild != nullptr)
    {
        for (auto item : *m_pChild)
        {
            static_cast<CTimeline*>(item)->SeekDCompAnimationInstances(globalSeekTime);
        }
    }

    m_shouldSynchronizeDCompAnimationAfterResume = false;
}

// Compute the duration when one is not specified in the markup.
// ParallelTimeline overrides this method to return the value of its
// longest child Timeline.
void CParallelTimeline::GetNaturalDuration(
    _Out_ DirectUI::DurationType* pDurationType,
    _Out_ XFLOAT *pDurationValue
    )
{
    float duration = 0;
    DirectUI::DurationType durationType = DirectUI::DurationType::TimeSpan;

    if (m_duration &&
        m_duration->Value().GetDurationType() == DirectUI::DurationType::TimeSpan)
    {
        // Versions of Xaml before Threshold did not respect an explicit Duration="Forever" on
        // a ParallelTimeline, and will instead resolve its natural duration from its children.
        duration = static_cast<float>(m_duration->Value().GetTimeSpanInSec());
        durationType = m_duration->Value().GetDurationType();
    }
    else if (m_pChild != nullptr)
    {
        bool hasInfiniteDurationChild = false;

        for (auto iter = m_pChild->begin(); !hasInfiniteDurationChild && (iter != m_pChild->end()); ++iter)
        {
            auto pTimelineNoRef = static_cast<CTimeline*>(*iter);

            float childDuration;
            DirectUI::DurationType childDurationType;
            pTimelineNoRef->GetDurationWithProperties(&childDurationType, &childDuration);

            switch (childDurationType)
            {
                case DirectUI::DurationType::TimeSpan:
                {
                    if (childDuration > duration)
                    {
                        duration = childDuration;
                        durationType = childDurationType;
                    }
                    break;
                }

                case DirectUI::DurationType::Forever:
                {
                    hasInfiniteDurationChild = true;
                    duration = childDuration;
                    durationType = childDurationType;
                    break;
                }

                // Automatic means that the subtree could not be resolved, which cannot happen due to the recursive nature of this call:
                //  - either the child is an animation, which will default to 1sec if automatic
                //  - or the child is another storyboard which will recursively resolve to its child times
                //  - or the child is another empty storyboard which will default to 0sec
                case DirectUI::DurationType::Automatic:
                default:
                {
                    XCP_FAULT_ON_FAILURE(false);
                    break;
                }
            }
        }
    }

    *pDurationType = durationType;
    *pDurationValue = duration;
}

void CParallelTimeline::PauseDCompAnimationsOnSuspend()
{
    if (m_pChild != nullptr)
    {
        for (auto item : *m_pChild)
        {
            static_cast<CTimeline*>(item)->PauseDCompAnimationsOnSuspend();
        }
    }
}

void CParallelTimeline::ResumeDCompAnimationsOnResume()
{
    // Normally a paused Storyboard will not be in the list of active timelines in the time manager, but since pause is not
    // synchronous, the suspend could come in after the pause call but before the tick. In that case the Xaml storyboard will
    // have m_fIsPaused marked yet will remain in the active state.
    //
    // We don't need to make any special exceptions for such storyboards. They should be paused on suspend just like any other
    // Xaml timelines - their DComp animations won't be paused yet since we didn't get a chance to tick before the suspend came
    // in. We can also safely resume their DComp animations after being unsuspended - the Xaml pause is still queued, and we
    // will re-pause the DComp animations on the next tick.

    if (m_pChild != nullptr)
    {
        for (auto item : *m_pChild)
        {
            static_cast<CTimeline*>(item)->ResumeDCompAnimationsOnResume();
        }
    }

    m_shouldSynchronizeDCompAnimationAfterResume = true;
}

void CParallelTimeline::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();
}

CompositionAnimationConversionResult CParallelTimeline::MakeCompositionAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext)
{
    CompositionAnimationConversionResult result = CompositionAnimationConversionResult::Success;

    if (m_pChild != nullptr &&
        (m_isDCompAnimationDirtyInSubtree || myContext->IsForceCompositionAnimationDirty()))
    {
        for (auto item : *m_pChild)
        {
            CompositionAnimationConversionContext childContext(*myContext);

            // Try to convert every child timeline. Don't short circuit as soon as one fails. We'll tick the failed
            // ones dependently on the UI thread, but the ones that succeeded can still run independently.
            CompositionAnimationConversionResult childResult = static_cast<CTimeline*>(item)->MakeCompositionAnimationsWithProperties(&childContext);

            if (result == CompositionAnimationConversionResult::Success)
            {
                result = childResult;
            }
        }

        m_isDCompAnimationDirtyInSubtree = false;
        m_forceDCompAnimationDirtyInSubtree = false;
    }

    return result;
}

void CParallelTimeline::DetachDCompCompletedHandlerOnStop()
{
    __super::DetachDCompCompletedHandlerOnStop();

    if (m_pChild != nullptr)
    {
        for (const auto& item : *m_pChild)
        {
            static_cast<CTimeline*>(item)->DetachDCompCompletedHandlerOnStop();
        }
    }
}

void CParallelTimeline::UpdateIsWaitingForDCompAnimationCompleted(bool childIsWaiting)
{
    if (childIsWaiting && !IsWaitingForDCompAnimationCompleted())
    {
        // We weren't waiting for DComp but a child is now waiting for DComp - we're now waiting as well.
        SetIsWaitingForDCompAnimationCompleted(true);
    }
    else if (!childIsWaiting && IsWaitingForDCompAnimationCompleted())
    {
        // We were waiting for DComp but a child is no longer waiting - check all children before marking ourselves
        // as not waiting.

        bool hasWaitingChild = false;

        if (m_pChild != nullptr)
        {
            for (const auto& item : *m_pChild)
            {
                CTimeline* timeline = static_cast<CTimeline*>(item);

                if (timeline->IsWaitingForDCompAnimationCompleted())
                {
                    hasWaitingChild = true;
                    break;
                }
            }
        }

        if (!hasWaitingChild)
        {
            SetIsWaitingForDCompAnimationCompleted(false);
        }
    }
}
