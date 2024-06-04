// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct IFrameScheduler;

struct ComputeStateParams
{
    ComputeStateParams()
        : time(0)
        , hasTime(false)
        , isReversed(false)
        , isPaused(false)
        , isAncestorWaitingForDCompAnimationCompleted(true)
        , cannotConvertToCompositionAnimation(false)
        , speedRatio(1)
        , pFrameSchedulerNoRef(nullptr)
    {
    }

    double time;
    bool hasTime;
    bool isReversed;

    // In RS1+ we leave paused storyboards in the time manager, so that they keep their targets marked as animating, so that
    // the paused WUC animator stays connected to the tree. The alternative is to detach the WUC animator and use a static
    // value, except Xaml doesn't know what the value is since it's animated independently in the DWM. We don't want paused
    // storyboards to keep requesting ticks, however, so we mark a flag down here and check it before requesting more ticks.
    bool isPaused;

    // CTimeline::m_isWaitingForDCompAnimationCompleted is a flag that marks whether we're waiting for an animation completed
    // event for the DComp animation corresponding to the Xaml timeline. If we haven't, then we hold off on switching the clock
    // state away from Active, so that we don't rebuild the DComp tree and cut off the DComp animation before it's complete.
    //
    // With legacy DComp animations, when a timing tree contains animations of the same duration as the storyboard, then there's
    // a race condition. Both the storyboard and the animation inside are waiting for their DComp animations to complete. If the
    // animation is notified first, then all is good - it goes into the Filling or Stopped state, and the storyboard will request
    // a tick to move itself to the Filling or Stopped state once it receives notification from its own DComp animation. However,
    // if the storyboard is notified first, then it will go into the Filling or Stopped state, then it will be removed from the
    // list of active timelines in the time manager. When the animation inside receives its notification, it will request
    // another tick, but it won't ever be ticked again because its storyboard is no longer in the time manager. That animation
    // will then be permanently left in the Active state.
    //
    // The fix is to assume that child timelines have received their DComp notifications whenever parent timelines have
    // received theirs. Then the animation is free to go to the Filling or Stopped state, even if the storyboard receives
    // notification first. This is not a problem for animations that have longer durations than their storyboards - in order
    // to go to Filling or Completed, the animation must also pass a "time has expired" check, which will never happen if the
    // parent storyboard has a shorter duration than the child animation.
    //
    // This also solves the problem of animations that target collapsed subtrees. WUC animations already take care of this
    // case by explicitly connecting all WUC animations with their targets, even if their targets come from collapsed subtrees,
    // but legacy DComp animations have no such mechanism. Instead, a Xaml animation that targets a collapsed subtree will
    // create a legacy DComp animation that never gets attached and will wait on it to fire a completed notification that it
    // will never fire. The thing that saves us is the parent Storyboard, which will wait for a dummy DComp animation to fire
    // a completed notification, then forcefully complete all its children, including the blocked Xaml animation.
    bool isAncestorWaitingForDCompAnimationCompleted : 1;

    // If an animation can't be converted to a Composition animation, then we'll tick it dependently on the UI thread.
    // If a Storyboard can't be converted to a Composition animation, then we'll tick its entire timing subtree on the
    // UI thread. Note that a Storyboard won't mark itself with a conversion failure if a child timeline cannot convert
    // (See CParallelTimeline::MakeCompositionAnimationVirtual).
    bool cannotConvertToCompositionAnimation : 1;

    float speedRatio;
    IFrameScheduler *pFrameSchedulerNoRef;
};
