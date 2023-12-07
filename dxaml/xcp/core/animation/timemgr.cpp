// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Timemgr.h"
#include "Animation.h"
#include "Storyboard.h"
#include "DCompAnimationConversionContext.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <GraphicsUtility.h>
#include <DCompTreeHost.h>

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of a time manager object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTimeManager::Create(
    _In_ CCoreServices *pCore,
    _In_ IPALTickableClock *pIClock,
    _Outptr_ CTimeManager **ppObject
    )
{
    HRESULT hr = S_OK;

    *ppObject = new CTimeManager(pCore, pIClock);

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
CTimeManager::CTimeManager(
    _In_ CCoreServices *pCore,
    _In_ IPALTickableClock *pIClock
    )
    : CDependencyObject(pCore)
    , m_isLoaded(FALSE)
    , m_lockTimeToZero(TRUE)
    , m_unlockRequested(FALSE)
    , m_processIATargets(FALSE)
    , m_independentTimelinesChanged(FALSE)
    , m_pIClock(pIClock)
    , m_rTimeStarted(0)
    , m_rLastTickTime(0)
    , m_pTimelineListHead(NULL)
    , m_pSnappedTimelineHeadNoRef(nullptr)
    , m_pTimelinePreviousHead(NULL)
    , m_pRootTimeline(NULL)
    , m_IACountersSwapState(false)
    , m_hadActiveFiniteAnimations(FALSE)
    , m_clockOverride(-1)
{
    m_requiresThreadSafeAddRefRelease = true;
    m_pIClock->AddRef();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CTimeManager::~CTimeManager()
{
    ReleaseInterface(m_pIClock);
    Reset();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resets this object back to creation state
//
//------------------------------------------------------------------------
void
CTimeManager::Reset()
{
    ResetWUCCompletedEvents();

    // when calling DeleteTimelineList(), the node being pointed to here is also deleted.
    m_pTimelinePreviousHead = NULL;

    DeleteTimelineList(m_pTimelineListHead);
    m_pTimelineListHead = NULL;

    m_pSnappedTimelineHeadNoRef = nullptr;

    // Destroy the root Timeline
    ReleaseInterface(m_pRootTimeline);

    // Reset the hash table
    m_hashTable.clear();

    // Reset State
    m_rTimeStarted = 0;
    m_isLoaded = FALSE;

    // Clear any unprocessed counters; there's no need to mark them if we're resetting.
    GetPendingIACounters().clear();
    GetPendingIACounters().shrink_to_fit();

    // Processing targets with no current frame will clear all existing markings.
    VERIFYHR(UpdateIATargets());

    // Clear the processed counters as well.
    GetProcessedIACounters().clear();
    GetProcessedIACounters().shrink_to_fit();
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Deletes a linked list of timelines
//
//---------------------------------------------------------------------------
void
CTimeManager::DeleteTimelineList(_In_opt_ TimelineListNode * pNode)
{
    if (pNode != NULL)
    {
        // Detach list from previous link
        TimelineListNode *pPrev = pNode->m_pPreviousNoRef;
        if (pPrev != NULL)
        {
            pPrev->m_pNextNoRef = NULL;
        }

        // Release all Timelines and nodes in linked list
        while (pNode != NULL)
        {
            // get current pointer, and save next pointer for next iteration
            TimelineListNode *pTemp = pNode;
            pNode = pTemp->m_pNextNoRef;

            CTimeline *pTimeline = pTemp->m_pTimeline;
            if ( pTimeline )
            {
                pTimeline->SetTimingParent( NULL );
                IGNOREHR( pTimeline->OnRemoveFromTimeManager() );
                ReleaseInterface(pTimeline);
            }

            delete pTemp;
            pTemp = NULL;
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to links a TimelineListNode to the head of a list.
//
//------------------------------------------------------------------------
void
CTimeManager::InsertNodeAtHead(
    _In_ TimelineListNode *pNewHead,
    _In_opt_ TimelineListNode *pPreviousHead
    )
{
    pNewHead->m_pNextNoRef = pPreviousHead;

    if (pPreviousHead != NULL)
    {
        ASSERT(pPreviousHead->m_pPreviousNoRef == NULL);
        pPreviousHead->m_pPreviousNoRef = pNewHead;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets an estimate of the 'wall clock' time in the TimeManager's frame of reference
//      (i.e. since this time manager started).
//      This time is _not_ aligned to the last tick - in fact it always reports a greater value.
//
//      This time is safe to use any time on the UI thread, especially for async work that
//      happens outside of ticking/generating a frame.
//
//------------------------------------------------------------------------
XDOUBLE
CTimeManager::GetEstimatedNextTickTime() const
{
    return (m_pIClock && m_isLoaded && !m_lockTimeToZero)
         ? m_pIClock->GetNextTickTimeInSeconds() - m_rTimeStarted
         : 0.0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the last time that the TimeManager ticked, relative to when it first ticked.
//
//      This time is _not safe_ to use any time on the UI thread, especially for async work.
//      It is only reliable inside frame generation after the TimeManager has been ticked.
//      Up until that point, the time returned is unpredictable since the UI thread may
//      have been idle for an indefinite period.
//
//------------------------------------------------------------------------
XFLOAT
CTimeManager::GetLastTickTime() const
{
    return static_cast<XFLOAT>(m_rLastTickTime);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update the timing tree
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTimeManager::Tick(
    bool newTimelinesOnly,
    bool processIATargets,
    _In_opt_ IDCompositionDesktopDevicePartner3* pDCompDevice,
    _In_opt_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_opt_ WUComp::ICompositor* wucCompositor,
    _In_ IFrameScheduler *pFrameScheduler,
    const bool tickOnlyTimers,
    _Out_opt_ bool *checkForAnimationComplete,
    _Out_opt_ bool *hasActiveFiniteAnimations
    )
{
    HRESULT hr = S_OK;
    CTimeline *pTimeline = nullptr;
    TimelineListNode *pCurrNode = m_pTimelineListHead;
    bool bHasActiveFiniteAnimations = false;

    // We only want to check for the completion of all finite animations if we've successfully retrieved the animations complete event
    bool bCheckForAnimationsComplete = !!GetContext()->HasAnimationEvents();

    XDOUBLE rTimeCurrent = 0.0f;

    ComputeStateParams parentParams;

    EnsureTimeStarted();

    // Save this bit so animations can determine whether they need to mark independent-animation
    // targets by calling ShouldCollectIATargets during this Tick().
    m_processIATargets = processIATargets;

    // The list of pending animation targets should be processed each frame.
    // The only time we can already have pending targets when ticking is if this is a second tick
    // in the same frame to update just the newly added timelines.
    ASSERT(GetPendingIACounters().empty() || newTimelinesOnly);

    // Compute global clock
    if (!newTimelinesOnly && !m_lockTimeToZero)
    {
        m_rLastTickTime = m_pIClock->GetLastTickTimeInSeconds() - m_rTimeStarted;
    }
    rTimeCurrent = m_rLastTickTime;

    // If not loaded or if there are no active timelines, do nothing
    if (!m_isLoaded || m_pTimelineListHead == nullptr)
    {
        goto Cleanup;
    }

    // New timelines are added to the front of the list of animations. We tick animations multiple times each frame,
    // because things like layout can add new animations. We don't want to tick any animation more than once, so we
    // keep a marker for the part of the list that has already been ticked.
    //
    // Save the current head of the list of animations. At the end of ticking, we'll update the marker to this saved
    // value. We don't use m_pTimelineListHead when we update the marker, because ticking the current animations could
    // have added new animations to the head of the list. These new animations haven't been ticked yet, and should be
    // in front of the marker so that we can tick them.
    //
    // Timelines can be removed from the time manager as it ticks, and we have to update this saved head of the list
    // as timelines are removed. Otherwise this node (representing the tail of the timeline list that has already been
    // ticked) may itself no longer be in the timeline list.
    m_pSnappedTimelineHeadNoRef = m_pTimelineListHead;

    if (!newTimelinesOnly)
    {
        m_pTimelinePreviousHead = nullptr;
    }

    if (m_clockOverride >= 0)
    {
        rTimeCurrent = m_clockOverride;
    }

    if (pCurrNode != nullptr && !s_slowDownAnimationsLoaded)
    {
        static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        s_slowDownAnimations = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::SlowDownAnimations, true /* disableCaching */);
        s_slowDownAnimationsLoaded = true;
    }

    const float initialSpeedRatio = s_slowDownAnimations ? 0.1f : 1.0f;

    parentParams.hasTime = true;
    parentParams.time = rTimeCurrent;
    parentParams.pFrameSchedulerNoRef = pFrameScheduler;
    parentParams.isReversed = false;
    parentParams.speedRatio = initialSpeedRatio;
    parentParams.isPaused = false;

    while (pCurrNode != m_pTimelinePreviousHead && pCurrNode != nullptr)
    {
        pTimeline = pCurrNode->m_pTimeline;

        if (pTimeline
            && (!tickOnlyTimers || pTimeline->OfTypeByIndex<KnownTypeIndex::DispatcherTimer>()))
        {
            bool hasNoExternalReferences = true;

            if (!tickOnlyTimers && pDCompDevice != nullptr)
            {
                // This is the UI thread. Make Composition animations.
                CompositionAnimationConversionContext compContext(easingFunctionStatics, wucCompositor, initialSpeedRatio);

                // Disregard this returned result. Storyboards return a failure if any part of them failed to convert
                // to a Composition animation. There may be other timelines inside that successfully converted, and we
                // shouldn't skip this Storyboard just because part of it failed to convert.
                //
                // Each timeline in this timing tree has recorded its own conversion result. The parts that failed to
                // convert will tick dependently during ComputeState. The parts that succeeded will attach them to
                // animation targets during AttachDCompAnimations.
                pTimeline->MakeCompositionAnimationsWithProperties(&compContext);
            }

            IFC(pTimeline->ComputeState(parentParams, &hasNoExternalReferences));

            if (!tickOnlyTimers && pDCompDevice != nullptr)
            {
                pTimeline->AttachDCompAnimations();
                pTimeline->ResolvePendingDCompAnimationOperationsRecursive();

                if (pTimeline->ShouldSynchronizeDCompAnimationAfterResume())
                {
                    pTimeline->SynchronizeDCompAnimationAfterResume(rTimeCurrent);
                }
            }

            // With independent animations ticking in the DWM, we can't replace DComp animations with static values because
            // we don't know what that static value is. We'll leave paused animations (with the paused WUC animators) attached
            // to the DComp tree. That requires keeping paused Xaml animations in the time manager.
            bool isStoryboardPaused = pTimeline->OfTypeByIndex<KnownTypeIndex::Storyboard>()
                && static_cast<CStoryboard*>(pTimeline)->IsPaused();

            // We want to remove this Timeline if it's not active and if we are not keeping it alive.
            if (hasNoExternalReferences
                    || (!pTimeline->IsInActiveState() && !isStoryboardPaused && !pTimeline->HasPendingThemeChange()))
            {
                TimelineListNode *pNodeToRemoveNoRef = pCurrNode;
                pCurrNode = pCurrNode->m_pNextNoRef;
                IFC(RemoveTimeline(pTimeline, pNodeToRemoveNoRef));
            }
            else
            {
                // We want to check for whether or not the current timeline is finite only if the following conditions are met:
                //
                // 1. We want to check for animations having completed in the first place;
                // 2. We haven't already found a finite animation;
                // 3. The current timeline is active; and
                // 4. The current timeline actually is an animation.
                //
                if (bCheckForAnimationsComplete &&
                    !bHasActiveFiniteAnimations &&
                    pTimeline->IsInActiveState() &&
                    !pTimeline->OfTypeByIndex<KnownTypeIndex::DispatcherTimer>())
                {
                    bHasActiveFiniteAnimations = !!pTimeline->IsFinite();
                }

                pCurrNode = pCurrNode->m_pNextNoRef;
            }
        }
        else
        {
            pCurrNode = pCurrNode->m_pNextNoRef;
        }
    }

    // Run a pass over the hash table to clean up any leftover registered animations
    for (auto iter = m_hashTable.begin(); iter != m_hashTable.end(); /* manual */)
    {
        if (iter->first.m_weakRef.expired())
        {
            iter = m_hashTable.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

Cleanup:
    if (checkForAnimationComplete != nullptr)
    {
        *checkForAnimationComplete = bCheckForAnimationsComplete;
    }
    if (hasActiveFiniteAnimations != nullptr)
    {
        *hasActiveFiniteAnimations = bHasActiveFiniteAnimations;
    }

    m_pTimelinePreviousHead = m_pSnappedTimelineHeadNoRef;
    m_pSnappedTimelineHeadNoRef = nullptr;
    m_processIATargets = FALSE;

    RRETURN(hr);
}

void CTimeManager::TickTimers(_In_ IFrameScheduler *pFrameScheduler)
{
    IFCFAILFAST(Tick(
        FALSE /* newTimelinesOnly - ignored */,
        FALSE /* processIATargets - ignored */,
        nullptr /* pDCompDevice - ignored */,
        nullptr /* easingFunctionStatics - ignored */,
        nullptr /* wucCompositor - ignored */,
        pFrameScheduler,
        true /* tickOnlyTimers */,
        nullptr /* checkForAnimationComplete */,
        nullptr /* hasActiveFiniteAnimations */
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update state associated with the HasAnimation and AnimationComplete
//      events provided for test automation.  To be called from Tick after
//      traversing the full list of animations.
//
//------------------------------------------------------------------------
void CTimeManager::UpdateAnimationEvents(bool hasActiveFiniteAnimations)
{
    // If we've gone through the full list of animations and have found no active finite animations,
    // and if we had active finite animations the last time through, then we want to set the animations
    // complete event.
    // In addition, if the status of has/doesn't have animations has now changed,
    // we want to set or reset the has animations event accordingly.

    if (m_hadActiveFiniteAnimations && !hasActiveFiniteAnimations)
    {
        auto core = GetContext();
        IFCFAILFAST(core->SetHasAnimationsEventSignaledStatus(FALSE /* bSignaled */));
        IFCFAILFAST(core->SetAnimationsCompleteEvent());
    }
    else if (!m_hadActiveFiniteAnimations && hasActiveFiniteAnimations)
    {
        IFCFAILFAST(GetContext()->SetHasAnimationsEventSignaledStatus(TRUE /* bSignaled */));
    }

    m_hadActiveFiniteAnimations = hasActiveFiniteAnimations;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a Timeline to the list of objects we tick
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTimeManager::AddTimeline(_In_ CTimeline *pTimeline)
{
    HRESULT hr = S_OK;
    TimelineListNode *pNewNode = NULL;

    if (pTimeline == NULL)
    {
        goto Cleanup;
    }

    // Timelines only get marked as independent after being ticked, so the state should
    // never be set for timelines newly added to the time manager.
    ASSERT(!pTimeline->HasIndependentAnimation());

    // Ensure we have a root time group
    if (m_pRootTimeline == NULL)
    {
        CREATEPARAMETERS cp(pTimeline->GetContext());
        IFC(CTimeline::Create( (CDependencyObject**)(&m_pRootTimeline), &cp ));
    }

    pNewNode = new TimelineListNode();

    // set timing parent on the child timeline
    // Note: Although pTimeline's timing parent is set to the root timeline, it's not added to the root timeline's child collection.
    pTimeline->SetTimingParent(m_pRootTimeline);
    IFC( pTimeline->OnAddToTimeManager() );

    // update pointers and addref appropriately
    pNewNode->m_pTimeline = pTimeline;
    AddRefInterface(pTimeline);
    // note: ticking with the 'newTimelines' option relies on inserting new nodes at the front
    InsertNodeAtHead(pNewNode, m_pTimelineListHead);
    m_pTimelineListHead = pNewNode;
    pNewNode = NULL;

    // Do not update m_pSnappedTimelineHeadNoRef. This new timeline has already missed out on being ticked,
    // and should be in front of the marker for the ticked tail end of the list.

Cleanup:
    delete pNewNode;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Remove a timeline from the list of objects we tick.  This takes a
//      second optional parameter which is the node to start the search
//      on (typically the node of the timeline being removed).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTimeManager::RemoveTimeline(
    _In_ CTimeline *pTimeline,
    _In_opt_ TimelineListNode *pCurrNode
    )
{

    // Record whether the timeline being removed had independent animations.
    if (pTimeline->HasIndependentAnimation())
    {
        NotifyIndependentAnimationChange();
    }

    if (pCurrNode == NULL)
    {
        pCurrNode = m_pTimelineListHead;
    }

    // Find and remove the timeline from the root time group.
    while (pCurrNode != NULL)
    {
        if (pTimeline == pCurrNode->m_pTimeline)
        {
            if (pCurrNode->m_pPreviousNoRef != NULL)
            {
                pCurrNode->m_pPreviousNoRef->m_pNextNoRef = pCurrNode->m_pNextNoRef;
            }

            if (pCurrNode->m_pNextNoRef != NULL)
            {
                pCurrNode->m_pNextNoRef->m_pPreviousNoRef = pCurrNode->m_pPreviousNoRef;
            }

            if (m_pTimelineListHead == pCurrNode)
            {
                ASSERT(m_pTimelineListHead->m_pPreviousNoRef == NULL);
                m_pTimelineListHead = pCurrNode->m_pNextNoRef;
                ASSERT(pCurrNode->m_pPreviousNoRef == NULL);
            }

            if (m_pSnappedTimelineHeadNoRef == pCurrNode)
            {
                // The node where we started ticking the time manager has been removed. Update it to point to the next node
                // in the list. When we're done ticking, this will become the marker for the tail of the list that has
                // already been ticked.
                m_pSnappedTimelineHeadNoRef = pCurrNode->m_pNextNoRef;
            }

            if (m_pTimelinePreviousHead == pCurrNode)
            {
                // The previous head is a marker used stop walking the list when ticking only new animations.
                // It's allowed to have a previous node.
                m_pTimelinePreviousHead = pCurrNode->m_pNextNoRef;
            }

            delete pCurrNode;
            pCurrNode = NULL;

            // set this Timeline for deletion
            xref_ptr<CTimeline> pTimelineToRelease;
            pTimelineToRelease.attach(pTimeline);

            // set Timing parent to NULL and notify Timeline
            pTimeline->SetTimingParent(NULL);
            IFC_RETURN(pTimeline->OnRemoveFromTimeManager());

            break;
        }
        pCurrNode = pCurrNode->m_pNextNoRef;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if independent animation targets should be marked
//      during this tick.
//
//------------------------------------------------------------------------
bool
CTimeManager::ShouldCollectIATargets()
{
    return m_processIATargets;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds up a map of all target elements and the number of types
//      of animations they are affected by this frame.
//
//------------------------------------------------------------------------
void CTimeManager::CollectIATargets(_In_ const IATargetVector *pTargets)
{
    ASSERT(ShouldCollectIATargets());

    auto& pendingIACounters = GetPendingIACounters();

    // Stash the info about each element and the type of animation targeting it for processing later this frame.
    for (auto it = pTargets->begin(), end = pTargets->end(); it != end; ++it)
    {
        // We must have a target right now, otherwise it should have been added.
        // The target elements are ref-counted because arbitrary code runs in between CollectIATargets, when ticking,
        // and UpdateIATargets, later in the frame, so the elements may have been destroyed in-between.
        // They're weak-ref because animation code generally doesn't hold strong refs to the tree, and there's
        // no need for marking targets to impact when they can be released.
        ASSERT(!it->targetWeakRef.expired());

        pendingIACounters[it->targetWeakRef].Add(it->animationType);
    }
}

_Check_return_ HRESULT CTimeManager::UpdateAllIATargets(
    _In_ const IACounters& oldAnimations,
    _In_ const IACounters& newAnimations,
    _In_ CDependencyObject *target)
{
    IFC_RETURN(UpdateIATarget(oldAnimations.m_hasUnspecifiedIA, newAnimations.m_hasUnspecifiedIA, target, IndependentAnimationType::None));
    IFC_RETURN(UpdateIATarget(oldAnimations.m_hasOffsetIA, newAnimations.m_hasOffsetIA, target, IndependentAnimationType::Offset));
    IFC_RETURN(UpdateIATarget(oldAnimations.m_hasTransformIA, newAnimations.m_hasTransformIA, target, IndependentAnimationType::Transform));
    IFC_RETURN(UpdateIATarget(oldAnimations.m_hasProjectionIA, newAnimations.m_hasProjectionIA, target, IndependentAnimationType::ElementProjection));
    IFC_RETURN(UpdateIATarget(oldAnimations.m_hasTransform3DIA, newAnimations.m_hasTransform3DIA, target, IndependentAnimationType::ElementTransform3D));
    IFC_RETURN(UpdateIATarget(oldAnimations.m_hasElementOpacityIA, newAnimations.m_hasElementOpacityIA, target, IndependentAnimationType::ElementOpacity));
    IFC_RETURN(UpdateIATarget(oldAnimations.m_hasElementClipIA, newAnimations.m_hasElementClipIA, target, IndependentAnimationType::ElementClip));
    IFC_RETURN(UpdateIATarget(oldAnimations.m_hasTransitionOpacityIA, newAnimations.m_hasTransitionOpacityIA, target, IndependentAnimationType::TransitionOpacity));
    IFC_RETURN(UpdateIATarget(oldAnimations.m_hasTransitionClipIA, newAnimations.m_hasTransitionClipIA, target, IndependentAnimationType::TransitionClip));
    IFC_RETURN(UpdateIATarget(oldAnimations.m_hasBrushColorIA, newAnimations.m_hasBrushColorIA, target, IndependentAnimationType::BrushColor));

    return S_OK;
}
//------------------------------------------------------------------------
//
//  Synopsis:
//      After all IA targets have been collected for this frame, compares
//      the mapping to the previous frame, and notifies target elements
//      of any changes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTimeManager::UpdateIATargets()
{
    auto& pendingIACounters = GetPendingIACounters();
    auto& processedIACounters = GetProcessedIACounters();

    for (const auto& pending : pendingIACounters)
    {
        // If the weak ref is no longer valid, do nothing.
        auto targetWeakRef = pending.first;
        auto target = targetWeakRef.lock_noref();

        if (target)
        {
            // If the target was animated both this frame and last, diff the counts and only notify
            // the element of changes.
            auto processedIter = processedIACounters.find(targetWeakRef);

            if (processedIter != processedIACounters.end())
            {
                IACounters oldAnimations = processedIter->second;

                // Remove the entry for this element from the previous frame map to simplify iteration over that later.
                processedIACounters.erase(processedIter);

                IFC_RETURN(UpdateAllIATargets(oldAnimations, pending.second, target));
            }
            else
            {
                IFC_RETURN(UpdateAllIATargets({}, pending.second, target));
            }
        }
    }

    // Iterate through all the remaining counts from the previous frame to unmark elements that are no longer animated.
    for (const auto& processed : processedIACounters)
    {
        // If the weak ref is no longer valid, do nothing.
        auto target = processed.first.lock_noref();

        if (target)
        {
            // Any elements animated in both frames should have been removed from the previous list already to
            // trim this walk.
            ASSERT(pendingIACounters.count(processed.first) == 0);

            // If the element is no longer animated this frame, notify it of all old animations being gone.
            IFC_RETURN(UpdateAllIATargets(processed.second, {}, target));
        }
    }

    // The current map becomes the previous map, to be used next frame.
    processedIACounters.clear();
    m_IACountersSwapState ^= true;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to update a single IA target for this frame.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTimeManager::UpdateIATarget(
    bool hasPrevFrame,
    bool hasThisFrame,
    _In_ CDependencyObject *pTarget,
    IndependentAnimationType animType)
{
    if (hasThisFrame && !hasPrevFrame)
    {
        IFC_RETURN(pTarget->SetRequiresComposition(
            CompositionRequirement::IndependentAnimation,
            animType));
    }
    else if (!hasThisFrame && hasPrevFrame)
    {
        pTarget->UnsetRequiresComposition(
            CompositionRequirement::IndependentAnimation,
            animType);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This notification will cause the compositor threads clone to be
//      updated.  It must be called whenever an independent animation changes.
//      This includes:
//      - adding/removing an independent animation from the time manager
//      - changing an animation from dependent to independent or vice versa
// TODO: Also need to detect whether there were DP changes for an actively running independent animation
//
//------------------------------------------------------------------------
void
CTimeManager::NotifyIndependentAnimationChange()
{
    m_independentTimelinesChanged = TRUE;
}

void CTimeManager::ResetIndependentTimelinesChanged()
{
    m_independentTimelinesChanged = FALSE;
}

bool CTimeManager::HasActiveAnimations()
{
    if (HasActiveTimelines())
    {
        // Check there is at least one timeline which is actually an animation (as opposed to a timer).
        TimelineListNode *pCurrent = m_pTimelineListHead;

        while (pCurrent != NULL)
        {
            if (pCurrent->m_pTimeline->GetTypeIndex() != DependencyObjectTraits<CDispatcherTimer>::Index)
            {
                return true;
            }
            pCurrent = pCurrent->m_pNextNoRef;
        }

        return false;
    }
    else
    {
        return false;
    }
}

void CTimeManager::StopAllTimelinesAfterTest()
{
    while (m_pTimelineListHead != nullptr)
    {
        // The reference from m_pTimelineListHead may be the last reference on this timeline. Don't let the timeline delete itself
        // in the middle of calling Stop.
        xref_ptr<CTimeline> timeline(m_pTimelineListHead->m_pTimeline);

        if (timeline->OfTypeByIndex<KnownTypeIndex::Storyboard>())
        {
            CStoryboard* storyboard = static_cast<CStoryboard*>(timeline.get());
            IFCFAILFAST(storyboard->Stop());    // Stop will call RemoveTimeline
        }
        else
        {
            IFCFAILFAST(RemoveTimeline(timeline, nullptr));
        }
    }

    // Clear the list of objects to go explicitly update. Since the test is ending, we expect all these expressions to be closed.
    // Updating them will return an error.
    m_targetDOs.clear();
}