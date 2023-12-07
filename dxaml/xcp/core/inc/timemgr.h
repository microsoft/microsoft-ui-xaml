// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IndependentAnimationType.h"
#include "CDependencyObject.h"
#include <vector_map.h>
#include <array>
#include <unordered_set>
#include <weakref_ptr.h>
#include <NamespaceAliases.h>
#include <IATarget.h>
#include <fwd/windows.ui.composition.h>

class CStoryboard;
class CTimeline;
class CAnimation;
class CCoreServices;
struct IPALTickableClock;
struct IFrameScheduler;
struct IDCompositionDesktopDevicePartner3;
class CTranslateTransform;
class AnimationTelemetry;

// Used to keep a persistent count of the number and type of independent animations affecting a given CDependencyObject.
struct IACounters
{
    void Add(IndependentAnimationType animType)
    {
        switch (animType)
        {
            case IndependentAnimationType::None:
                m_hasUnspecifiedIA = true;
                break;

            case IndependentAnimationType::Offset:
                m_hasOffsetIA = true;
                break;

            case IndependentAnimationType::Transform:
                m_hasTransformIA = true;
                break;

            case IndependentAnimationType::ElementProjection:
                m_hasProjectionIA = true;
                break;

            case IndependentAnimationType::ElementTransform3D:
                m_hasTransform3DIA = true;
                break;

            case IndependentAnimationType::ElementOpacity:
                m_hasElementOpacityIA = true;
                break;

            case IndependentAnimationType::ElementClip:
                m_hasElementClipIA = true;
                break;

            case IndependentAnimationType::TransitionOpacity:
                m_hasTransitionOpacityIA = true;
                break;

            case IndependentAnimationType::TransitionClip:
                m_hasTransitionClipIA = true;
                break;

            case IndependentAnimationType::BrushColor:
                m_hasBrushColorIA = true;
                break;

            default:
                ASSERT(FALSE);
        }
    }

    union
    {
        struct
        {
            bool m_hasUnspecifiedIA       : 1;
            bool m_hasOffsetIA            : 1;
            bool m_hasTransformIA         : 1;
            bool m_hasProjectionIA        : 1;
            bool m_hasTransform3DIA       : 1;
            bool m_hasElementOpacityIA    : 1;
            bool m_hasElementClipIA       : 1;
            bool m_hasTransitionOpacityIA : 1;
            bool m_hasTransitionClipIA    : 1;
            bool m_hasBrushColorIA        : 1;
        };

        uint16_t m_asOne                    = 0;
    };
};

using IATargetCountMap = containers::vector_map<xref::weakref_ptr<CDependencyObject>, IACounters>;

//------------------------------------------------------------------------
//
//  Class:  CTimeManager
//
//  Synopsis:
//      CTimeManager class
//
//------------------------------------------------------------------------
class CTimeManager final : public CDependencyObject
{
public:
    static _Check_return_ HRESULT Create(
        _In_ CCoreServices *pCore,
        _In_ IPALTickableClock *pIClock,
        _Outptr_ CTimeManager **ppObject
        );

    _Check_return_ HRESULT Tick(
        bool newTimelinesOnly,
        bool processIATargets,
        _In_opt_ IDCompositionDesktopDevicePartner3* pDCompDevice,
        _In_opt_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_opt_ WUComp::ICompositor* wucCompositor,
        _In_ IFrameScheduler *pFrameScheduler,
        // Suspend-exempt media apps can still run when minimized. We want to keep DispatcherTimers running for them, while
        // not ticking other animations.
        const bool tickOnlyTimers,
        _Out_opt_ bool *checkForAnimationComplete,
        _Out_opt_ bool *hasActiveFiniteAnimations
        );

    void TickTimers(_In_ IFrameScheduler *pFrameScheduler);

    _Check_return_ XINT32 IsRunning() const { return m_isLoaded; }

    void Reset();

    _Check_return_ HRESULT AddTimeline(_In_ CTimeline *pTimeline);

    _Check_return_ HRESULT RemoveTimeline(_In_ CTimeline *pTimeline) { return RemoveTimeline(pTimeline, NULL); }

    _Check_return_ CTimeline * GetRootTimeline() { return m_pRootTimeline; }

    XDOUBLE GetEstimatedNextTickTime() const;

    XFLOAT GetLastTickTime() const;

    // Associate an animation with a specific DP
    _Check_return_ HRESULT SetAnimationOnProperty(
        _In_ const xref::weakref_ptr<CDependencyObject>& DOWeakRef,
        _In_ KnownPropertyIndex nPropertyIndex,
        _In_ CAnimation *pAnimation);

    void ClearAnimationOnProperty(
        _In_ const xref::weakref_ptr<CDependencyObject>& DOWeakRef,
        _In_ KnownPropertyIndex nPropertyIndex);

    // Get animation on a given property
    _Check_return_ xref_ptr<CAnimation> GetAnimationOnProperty(
        _In_ const xref::weakref_ptr<CDependencyObject>& DOWeakRef,
        _In_ KnownPropertyIndex nPropertyIndex);

    bool HasActiveTimelines()
    {
        return (m_pTimelineListHead != NULL);
    }

    bool HasActiveAnimations();

    bool HaveIndependentTimelinesChanged() const
    {
        return m_independentTimelinesChanged;
    }

    //
    // Primitive composition methods
    //
    bool ShouldCollectIATargets();
    void CollectIATargets(_In_ const IATargetVector *pTargets);
    _Check_return_ HRESULT UpdateIATargets();

    void NotifyIndependentAnimationChange();
    void ResetIndependentTimelinesChanged();

    void UnlockTime()
    {
        ASSERT(m_lockTimeToZero);

        m_unlockRequested = TRUE;
        m_lockTimeToZero = FALSE;
        m_independentTimelinesChanged = TRUE; // Make sure we commit DComp animations
    }

    bool IsLocked() const
    {
        return m_lockTimeToZero;
    }

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) final;

    void UpdateAnimationEvents(bool hasActiveFiniteAnimations);

    void PauseDCompAnimationsOnSuspend();
    void ResumeDCompAnimationsOnResume();

    void SetTimeManagerClockOverrideConstant(double newTime);
    void StopAllTimelinesAfterTest();

    // An animation just started under a scoped batch. Add an entry to the scoped batch : Xaml animation map and subscribe
    // to the completed event.
    // Also notify the Xaml animation of the scoped batch and the animator that were just created for it.
    // Returns whether the Xaml animation was successfully found.
    bool AddActiveWUCAnimation(
        _In_ CDependencyObject* targetObject,
        _In_ KnownPropertyIndex targetPropertyIndex,
        _In_ WUComp::ICompositionScopedBatch* scopedBatch,
        _In_ WUComp::ICompositionAnimatorPartner* animator);

    // WUC::CompositionScopedBatch::Completed handler. Do a lookup to find the corresponding Xaml animation and notify it.
    HRESULT OnWUCAnimationCompleted(
        _In_ IInspectable* sender,
        _In_ WUComp::ICompositionBatchCompletedEventArgs* args);

    // Forcefully marks that the WUC animation has completed, even when it hasn't.
    // Used when we skip a WUC animation (e.g. a projection where the element size is 0)
    void MarkWUCAnimationCompleted(
        _In_ CDependencyObject* targetObject,
        _In_ KnownPropertyIndex targetPropertyIndex);

    void DetachWUCCompletedHandler(_In_ WUComp::ICompositionScopedBatch* scopedBatch);

    static void StartWUCAnimation(
        _In_ WUComp::ICompositor* compositor,
        _In_ WUComp::ICompositionObjectPartner* animatingObject,
        _In_ LPCWSTR propertyName,
        _In_ WUComp::ICompositionAnimation* animation,
        _In_ CDependencyObject* targetObject,
        _In_ KnownPropertyIndex targetPropertyIndex,
        _In_opt_ CTimeManager* timeManager);

    // see banner on m_targetDOs
    void AddTargetDO(_In_ CDependencyObject* targetDO);
    void UpdateTargetDOs(_In_ WUComp::ICompositor* wucCompositor);
    bool HasTargetDOs() const;

    static bool ShouldFailFast();

private:
    CTimeManager(_In_ CCoreServices *pCore, _In_ IPALTickableClock *pClock);
    ~CTimeManager() override;

    struct HashKey
    {
        xref::weakref_ptr<CDependencyObject> m_weakRef;
        KnownPropertyIndex m_propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;

        //HashKey() = default;
        HashKey(const xref::weakref_ptr<CDependencyObject>& weakRef, KnownPropertyIndex propertyIndex)
            : m_weakRef(weakRef)
            , m_propertyIndex(propertyIndex)
        {}

        bool operator==(const HashKey& other) const { return m_weakRef == other.m_weakRef && m_propertyIndex == other.m_propertyIndex; }

        HashKey(const HashKey&) = default;
        HashKey& operator=(const HashKey&) = default;
        HashKey(HashKey&& other) WI_NOEXCEPT
            : m_weakRef(std::move(other.m_weakRef))
            , m_propertyIndex(other.m_propertyIndex)
        {}
        HashKey& operator=(HashKey&& other) WI_NOEXCEPT
        {
            if (this != &other)
            {
                m_weakRef = std::move(other.m_weakRef);
                m_propertyIndex = other.m_propertyIndex;
            }
            return *this;
        }
    };

    struct hasher
    {
        size_t operator()(const HashKey& key) const WI_NOEXCEPT
        {
            return key.m_weakRef.hash() ^ static_cast<size_t>(key.m_propertyIndex);
        }
    };

    typedef std::unordered_map<HashKey, xref_ptr<CAnimation>, hasher> HashNodeList;

    // struct used to keep list of active animations
    struct TimelineListNode
    {
        TimelineListNode()
            : m_pTimeline(NULL)
            , m_pNextNoRef(NULL)
            , m_pPreviousNoRef(NULL)
        {
        }

        ~TimelineListNode()
        {
            m_pTimeline = NULL;
            m_pNextNoRef = NULL;
            m_pPreviousNoRef = NULL;
        }

        CTimeline *m_pTimeline;
        TimelineListNode *m_pNextNoRef;
        TimelineListNode *m_pPreviousNoRef;
    };

    // method used to remove a Timeline from the TimeManager without having to search through every node
    _Check_return_ HRESULT RemoveTimeline(
        _In_ CTimeline *pTimeline,
        _In_opt_ TimelineListNode *pCurrNode
        );

    void DeleteTimelineList(_In_opt_ TimelineListNode * pNode);

    void InsertNodeAtHead(
        _In_ TimelineListNode *pNewHead,
        _In_opt_ TimelineListNode *pPreviousHead
        );

    static _Check_return_ HRESULT UpdateIATarget(
        bool hasPrevFrame,
        bool hasThisFrame,
        _In_ CDependencyObject *pTarget,
        IndependentAnimationType animType);

    static _Check_return_ HRESULT UpdateAllIATargets(
        _In_ const IACounters& oldAnimations,
        _In_ const IACounters& newAnimations,
        _In_ CDependencyObject *target);

    void EnsureTimeStarted();

    void ResetWUCCompletedEvents();

private:
    // Has the page loaded event triggered
    bool m_isLoaded : 1;

    // Locks the time to zero until the splash screen is dismissed
    bool m_lockTimeToZero : 1;

    // We have requested to unlock the time at the first future tick
    bool m_unlockRequested : 1;

    // True if this is a tick that will be followed by a draw, in which case targets for independent animation
    // should be marked.  They'll need to be cleaned up after drawing with a call to CleanupIATargets.
    bool m_processIATargets : 1;

    // True if the timing tree has changed, or properties on timelines in the tree have changed, since last the DComp device was committed.
    bool m_independentTimelinesChanged : 1;

    // Stores whether the manager had active finite animations last time it checked.
    bool m_hadActiveFiniteAnimations : 1;

    // Our clock
    IPALTickableClock *m_pIClock;

    // The time on the clock the first time the time manager ticked.
    XDOUBLE m_rTimeStarted;

    // The time the last time the time manager ticked. Takes the start time into account.
    XDOUBLE m_rLastTickTime;

    // Contains all the triggered time nodes
    TimelineListNode *m_pTimelineListHead;

    //
    // In each frame, we want to tick all timelines exactly once. This is made more complicated by the fact that timelines could be
    // added to and removed from the time manager while it's ticking, so we need a way to mark down what was ticked and what wasn't.
    // We'll tick animations in two places: once at the beginning of the frame for all currently registered animations, and again
    // after layout/events to tick any extra timelines that were added since the first tick. We'll add new animations to the head
    // of the list, which means we can mark the tail of the list as "already been ticked".
    //
    // There are two tricky pieces with this scheme:
    //
    //   1. Animations can be added to the head of the time manager while it's ticking. This means we need to snap the head of the
    //      list when we begin ticking, rather than snapping it at the end of the tick (when a bunch of new animations has shown up
    //      at the head).
    //
    //   2. Animations can be removed from the time manager while it's ticking. This means we can't snap a static value to use as the
    //      head of the list - that node could itself be removed from the time manager, which would mean the marker is no longer in
    //      the list. Instead, we have to update the snapped value as timeline nodes are removed from the time manager.
    //

    // The snapped head of the list. Set when we start ticking the time manager, updated as timeline nodes are removed from it.
    // Cleared when it's actually saved as the marker for the part of the timeline list that we've already ticked.
    TimelineListNode *m_pSnappedTimelineHeadNoRef;

    // The marker pointing to the (tail) part of the list that we've already ticked. Updated after we're done ticking, from the
    // snapped head of the list.
    TimelineListNode *m_pTimelinePreviousHead;

    // Internal root Timeline, which is a stub right now
    CTimeline *m_pRootTimeline;

    // Map of Animation objects to memory location of animatable properties
    HashNodeList m_hashTable;

    std::array<IATargetCountMap, 2> m_IACounters;

    bool m_IACountersSwapState;

    // Counts of independent animations for target elements being collected this frame.
    IATargetCountMap& GetPendingIACounters()
    {
        return m_IACounters[(m_IACountersSwapState) ? 0 : 1];
    }

    // Counts of independent animations for target elements marked as of last frame.
    IATargetCountMap& GetProcessedIACounters()
    {
        return m_IACounters[(m_IACountersSwapState) ? 1 : 0];
    }

    // If set to a nonnegative number, overrides the xaml clock. Every tick will tick at this constant time. The real clock
    // will still move forward.
    double m_clockOverride;

    // When a Xaml animation stops applying to its target, we immediately detach the DComp animation from its DComp target via
    // ForceUnsetDCompAnimation. This was done to prevent inactive Xaml animations from continuing to tick in the DWM because
    // we never walked to the animated target, but this is a problem for handoff. While Xaml handoff involves animation A being
    // cleared from its target before animation B takes over, DComp requires that A' be still set on the target when B' takes
    // over, which means immediately detaching the DComp animation from its target will break handoff in DComp.
    //
    // We chose to live with this limitation for everything except TranslateTransforms, because that's the one that matters for
    // the Office cursor. For TranslateTransforms, when an animation stops applying to a TranslateTransform, we'll add it to this
    // list. Then, after all Xaml animations have ticked and new ones had a chance to pick up from old ones, we'll update all
    // TranslateTransforms on this list to detach the DComp animations from the ones that are now unanimated.
    std::unordered_set<xref_ptr<CTranslateTransform>> m_translatesWithEndingAnimations;

    // If set, all Storyboard-based animations are slowed down.
    static bool s_slowDownAnimations;
    static bool s_slowDownAnimationsLoaded;

    // We see a lot of dumps where an animation can't resolve its target or target property. Those cases bubble up an error so that
    // VS can surface the error, but that means the context around the bug is lost in the dumps. This bool makes us fail fast whenever
    // something goes wrong with animations.
    static bool s_animationFailFast;
    static bool s_animationFailFastLoaded;

    // WinRT DComp uses scoped batches to deliver completed notifications. A WUC::CompositionScopedBatch has a Completed event
    // that fires whenever all animations inside the batch have completed. Xaml will put each animation into its own scoped batch
    // and keep a map of scoped batch -> Xaml animation here. When a completed event fires, we find the corresponding Xaml animation
    // and notify it that its DComp animation is done.
    //
    // DComp will fire the completed notification even if an animation completes prematurely, so when a Xaml animation explicitly
    // stops we have to take it out of the map so that the completed event finds nothing and no-ops.
    //
    // Xaml storyboards also have completed events, but they don't have any corresponding animations in the tree. We'll track the
    // children of each storyboard and mark the storyboard as "DComp animation complete" when each of its children has completed its
    // DComp animations (or doesn't have one).
    std::unordered_map<xref_ptr<WUComp::ICompositionScopedBatch>, xref_ptr<CTimeline>> m_activeWUCAnimations;

    // A list of all DOs that are targeted by WUC animations. Since we depend on the WUC completed event, we need to make sure we
    // attach and start all the WUC animations that we created. Sometimes the target of an animation is in a collapsed part of the
    // tree, in which case it won't be visited as part of the render walk and won't kick off its WUC animation. We have to explicitly
    // walk to it afterwards. DOs that have been walked won't have their dirty flags set and will no-op during the explicit walk.
    //
    // Another alternative is to just mark the animations as completed, rather than starting them. This is less desirable. The app
    // can uncollapse the tree while a hidden animation is playing, in which case we should show the element with the animation in
    // progress, which means we need to actually start the animation when the element was still collapsed.
    //
    // This collection is also used to ensure that we've detached WUC animations from their expressions when the Xaml animation is
    // stopped. A Xaml animation using WUC involves 3 objects:
    //      WUC visual -> WUC expression -> WUC animation
    //
    // Normally when a Xaml animation stops, we just disconnect the expression from the DComp visual:
    //      WUC visual    WUC expression -> WUC animation
    //
    // That's enough to stop things from moving on screen, but that's not enough to stop the WUC animation in the DWM, because it's
    // still attached to the expression. We'll have to detach the WUC animation from the expression as well. We do that by updating
    // the expression for the Xaml target object, which will update its expression with a static value and detach the animation. If
    // we detach too early then it could break handoff scenarios, so we wait until the end of the frame, when new animations have had
    // a chance to take over already.
    std::vector<xref_ptr<CDependencyObject>> m_targetDOs;
};