// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"
#include "EnumDefs.g.h"
#include "XcpList.h"
#include "Request.h"
#include <ComputeStateParams.h>
#include "Duration.h"
#include "RepeatBehavior.h"
#include "DCompAnimationConversionContext.h"
#include <dcompinternal.h>
#include <dcompprivate.h>

class CTimeSpan;
class CTimelineGroup;
class CREATEPARAMETERS;
struct IFrameScheduler;
enum class CompositionAnimationConversionResult : byte;
class CompositionAnimationConversionContext;
class CTimeManager;

// A double that may or may not have a defined value.
class COptionalDouble
{
public:
    COptionalDouble()
    {
        m_fHasValue = FALSE;
    }

    XINT32 m_fHasValue{};
    XDOUBLE m_rValue{};
};

// Base class for timeline objects
class CTimeline :
    public CDependencyObject
{
protected:
    CTimeline(_In_opt_ CCoreServices *pCore);
    ~CTimeline() override;

public:
    DECLARE_CREATE(CTimeline);

    _Check_return_ HRESULT InitInstance() final;

    //
    // Recursive walk that creates Composition animations. The context that gets passed in is free to be changed by this
    // timeline, with properties pushed onto it. This timeline is responsible for copying this context before calling any
    // timeline children, and merging the children's contexts back into its own.
    //
    _Check_return_ CompositionAnimationConversionResult MakeCompositionAnimationsWithProperties(_Inout_ CompositionAnimationConversionContext* myContext);

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    bool ShouldRaiseEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent = false, _In_opt_ CEventArgs *pArgs = NULL) final
    {
        UNREFERENCED_PARAMETER(hEvent);
        UNREFERENCED_PARAMETER(fInputEvent);
        UNREFERENCED_PARAMETER(pArgs);
        return (m_pEventList != NULL);
    }

    KnownTypeIndex GetTypeIndex() const override;

    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _Out_opt_ CValue *pResult,
        _In_ bool fHandledEventsToo = false) override;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) override;

    CDependencyObject* GetTemplatedParent() final;

    // A Timeline has event requests with the EventManager iff it is in the TimeManager
    bool IsFiringEvents() final { return m_fIsInTimeManager; }

    _Check_return_ HRESULT OnManagedPeerCreated(XUINT32 fIsCustomDOType, XUINT32 fIsGCRoot) final;

    _Check_return_ HRESULT ComputeState(
        _In_ const ComputeStateParams &parentParams,
        _Inout_opt_ bool *pHasNoExternalReferences
        );

    virtual _Check_return_ HRESULT UpdateAnimation(
        _In_ const ComputeStateParams &parentParams,
        _Inout_ ComputeStateParams &myParams,
        XDOUBLE beginTime,
        DirectUI::DurationType durationType,
        XFLOAT durationValue,
        _In_ const COptionalDouble &expirationTime,
        _Out_ bool *pIsIndependentAnimation
        )
    {
        UNREFERENCED_PARAMETER(beginTime);
        UNREFERENCED_PARAMETER(durationType);
        UNREFERENCED_PARAMETER(durationValue);
        UNREFERENCED_PARAMETER(expirationTime);
        *pIsIndependentAnimation = FALSE;
        return S_OK;
    }

    virtual void GetNaturalDuration(
        _Out_ DirectUI::DurationType* pDurationType,
        _Out_ XFLOAT* pDurationValue
        );

    virtual bool IsInterestingForAnimationTracking();

    virtual void AnimationTrackingCollectInfoNoRef(
        _Inout_ CDependencyObject** ppTarget,
        _Inout_ CDependencyObject** ppDynamicTimeline
        );

    virtual bool IsDurationForProgressDifferent() const { return false; }

    virtual void GetDurationForProgress(_Out_ DirectUI::DurationType *pDurationType, _Out_ XFLOAT *prDurationValue) { GetNaturalDuration(pDurationType, prDurationValue); }

    virtual void InitializeIteration();

    virtual _Check_return_ HRESULT FinalizeIteration();

    // Callbacks indicating addition or removal from time manager
    virtual _Check_return_ HRESULT OnAddToTimeManager();
    virtual _Check_return_ HRESULT OnRemoveFromTimeManager();

    virtual _Check_return_ HRESULT OnBegin() { RRETURN(E_FAIL); }

    void SetDynamicTimelineParent(_In_ CTimeline* pGeneratingParent)
    {
        ReplaceInterface(m_pDynamicTimelineParent, pGeneratingParent);
    }

    CTimeline* GetDynamicTimelineParent() const
    {
        return m_pDynamicTimelineParent;
    }

    void SetTimingParent(_In_ CTimeline* parent);
    CTimeline* GetTimingParent() const;

    CTimeline* GetRootTimingParent() const;
    _Check_return_ HRESULT CheckCanBeModified();
    bool CanBeModified() const;

    bool IsTopLevelTimeline();

    _Check_return_ HRESULT SetTargetObject( _In_ CDependencyObject *pTarget );

    void SetTargetProperty(_In_ const CDependencyProperty* pDP)
    {
        m_pManualTargetDependencyProperty = pDP;
    }

    CDependencyObject* GetTargetObjectNoRef() const;
    xref_ptr<CDependencyObject> GetTargetObject() const;

    const CDependencyProperty* GetTargetProperty();

    virtual bool IsInActiveState() const { return ((m_clockState == DirectUI::ClockState::Active) || (m_clockState == DirectUI::ClockState::NotStarted)); }
    virtual bool IsInStoppedState() { return ((m_clockState == DirectUI::ClockState::Stopped) || (m_clockState == DirectUI::ClockState::NotStarted)); }
    bool IsFilling() const;
    XUINT32 HasPendingThemeChange() { return m_hasPendingThemeChange; }

    void ResetCompletedEventFired();
    XUINT32 IsCompletedEventFired() { return m_IsCompletedEventFired; }

    virtual bool IsAnimation() { return false; }

    bool HasIndependentAnimation() { return m_hasIndependentAnimation; }

    virtual HRESULT FireCompletedEvent();

    bool IsInTimeManager() const { return m_fIsInTimeManager; }

    void ResolveName(
        _In_ const xstring_ptr& strName,
        _In_opt_ CTimeline *pParentTimeline,
        _Outptr_ CDependencyObject **ppObject
        );

    _Check_return_ HRESULT RequestTickForPendingThemeChange();

    virtual bool IsFinite();

    virtual void AttachDCompAnimations();

    // Called when the animation starts. Sets the current DComp animation to the new target. If the animation is
    // dirty, then it will be regenerated the next time that animations tick on the UI thread, and will be set on
    // the target again. We can optimize away the double set, but currently we only track DComp animation dirtiness
    // on parallel timelines. The extra set only writes to a field of a Xaml object and will not be committed to
    // DComp, so it's not currently a big concern.
    virtual void AttachDCompAnimationInstancesToTarget();

    // Called when the animation stops or switches targets/target properties, before the target object or target
    // property fields get reset. The DComp animation itself is sometimes kept around and can be set on the new target.
    virtual void DetachDCompAnimationInstancesFromTarget();

    // Determines whether the DComp animation (builder) should be discarded when the animation stops. Animations
    // that read their From values from a target could see the From value change when the animation starts again,
    // so a new DComp animation is needed.
    virtual bool ShouldDiscardDCompAnimationOnDetach();

    virtual void AddPendingDCompPause();
    virtual void AddPendingDCompResume();
    virtual void AddPendingDCompSeek(double globalSeekTime);
    virtual void SeekDCompAnimationInstances(double globalSeekTime);
    virtual void ResolvePendingDCompAnimationOperations();
    virtual void ClearPendingDCompAnimationOperationsRecursive();

    // Used for WUC animations. The non-recursive version is called as part of legacy DComp animation's "make animation
    // instances" walk, which is already recursive.
    virtual void ResolvePendingDCompAnimationOperationsRecursive();

    virtual void PauseDCompAnimationsOnSuspend();
    virtual void ResumeDCompAnimationsOnResume();
    bool ShouldSynchronizeDCompAnimationAfterResume() { return m_shouldSynchronizeDCompAnimationAfterResume; }
    virtual void SynchronizeDCompAnimationAfterResume(double timeManagerTime);

    void ReleaseDCompResources() override;

    static bool ExtendDurationWithRepeat(_Inout_ float* pDuration, _In_ const RepeatBehaviorVO& repeatBehavior);
    static float ExtendDurationWithReverse(float duration, bool hasReverse);
    static float AdjustDurationWithSpeedRatio(float duration, float speedRatio);
    static float AdjustDurationWithBeginTime(float duration, _In_opt_ const CTimeSpan* pBeginTime);

    void GetDurationWithProperties(
        _Out_ DirectUI::DurationType* pDurationType,
        _Out_ float* pDurationValue
        );

    void NWPropagateDirtyFlag(DirtyFlags flags) override;

    // RENDERCHANGEDPFN for the property system
    // Marks m_forceDCompAnimationDirtyInSubtree on this element and propagates it up the tree.
    static void SetForceDCompAnimationSubtreeDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    // RENDERCHANGEDPFN for the property system
    // Marks m_isDCompAnimationDirty on this element and propagates m_isDCompAnimationDirtyInSubtree up the tree.
    static void SetDCompAnimationDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    // RENDERCHANGEDPFN for the property system
    // Marks m_isDCompAnimationDirtyInSubtree on this element and propagates it up the tree.
    static void SetDCompAnimationInSubtreeDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    void SetUpForTesting() { m_hasIndependentAnimation = TRUE; }

    void MarkIsDCompAnimationDirty();
    void MarkIsDCompAnimationDirtyInSubtree();
    void MarkForceDCompAnimationDirtyInSubtree();

    void OnDCompAnimationCompleted();

    virtual void DetachDCompCompletedHandlerOnStop();

    // The WUC scoped batch for a WUC animation is created when the animation is started, not when the animation is created.
    // The time manager then subscribes to the completed event and notifies the Xaml animation of the scoped batch that contains
    // it.
    void SetWUCScopedBatch(_In_opt_ WUComp::ICompositionScopedBatch* scopedBatch);

    // The WUC animator for a WUC animation is created when the animation is started, not when the animation is created.
    // The time manager then notifies the Xaml animation of the animator that was created for it. The animator is used for
    // pause/seek/resume scenarios.
    void SetWUCAnimator(_In_ WUComp::ICompositionAnimatorPartner* animator);

    EventRegistrationToken* GetWUCAnimationCompletedToken();

    bool IsWaitingForDCompAnimationCompleted() const;
    void SetIsWaitingForDCompAnimationCompleted(bool isWaiting);
    virtual void UpdateIsWaitingForDCompAnimationCompleted(bool childIsWaiting);

    static xstring_ptr GetTargetPathForTracking(_In_ CDependencyObject* target, bool startFromParentOfTarget);

protected:

    void SetTemplatedParentImpl(_In_ CDependencyObject* parent) final;

    virtual _Check_return_ HRESULT ComputeStateImpl(
        _In_ const ComputeStateParams &parentParams,
        _Inout_ ComputeStateParams &myParams,
        _Inout_opt_ bool *pHasNoExternalReferences,
        bool hadIndependentAnimationLastTick,
        _Out_ bool *pHasIndependentAnimation
        );

    _Check_return_ HRESULT RequestTickToActivePeriodBegin(
        _In_ const ComputeStateParams &parentParams,
        _In_ const ComputeStateParams &myParams,
        XDOUBLE beginTime
        ) const;

    _Check_return_ HRESULT RequestTickToActivePeriodEnd(
        _In_ const ComputeStateParams &parentParams,
        _In_ const ComputeStateParams &myParams,
        _In_ const COptionalDouble &expirationTime
        ) const;

    virtual _Check_return_ HRESULT ResolveLocalTarget(
        _In_ CCoreServices *pCoreServices,
        _In_opt_ CTimeline *pParentTimeline
        );

    const xref::weakref_ptr<CDependencyObject>& GetTargetObjectWeakRef() const { return m_targetObjectWeakRef; }

    // This method encapsulates the lifetime checks for active Timelines.
    void CheckHasNoExternalRefs(_Inout_opt_ bool *pbHasNoExternalReferences);

    const CDependencyProperty* GetTargetDependencyProperty() { return m_pTargetDependencyProperty; }

    void ReleaseTarget();

    void ComputeExpirationTime(
        _In_ XDOUBLE rBeginTime,
        _In_ DirectUI::DurationType durationType,
        _In_ XFLOAT rDurationValue,
        _In_ XFLOAT rAppliedSpeedRatio,
        _Out_ COptionalDouble *poptExpirationTime
        );

    static void OnDCompAnimationTimeEventStatic(
        _In_opt_ void* pParam
        );

    void MarkIsDCompAnimationInstanceDirty();
    void MarkShouldAttachDCompAnimationInstance();

    virtual bool CanRequestTicksWhileActive();

    virtual CompositionAnimationConversionResult MakeCompositionAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext);

private:
    void ComputeEffectiveDuration(
        _In_ DirectUI::DurationType durationType,
        _In_ XFLOAT rDurationValue,
        _In_ XFLOAT rAppliedSpeedRatio,
        _Out_ COptionalDouble *poptEffectiveDuration
        );

    void ComputeCurrentClockState(
        _Inout_ ComputeStateParams &clampedParentParams,
        _In_ XDOUBLE rBeginTime,
        _In_ COptionalDouble *poptExpirationTime,
        bool hadIndependentAnimationLastTick,
        _Out_ bool *pShouldComputeProgress
        );

    virtual void ComputeLocalProgressAndTime(
        XDOUBLE rBeginTime,
        XDOUBLE rParentTime,
        DirectUI::DurationType durationType,
        XFLOAT rDurationValue,
        _In_ COptionalDouble *poptExpirationTime,
        bool hadIndependentAnimationLastTick,
        _Out_ bool *pIsInReverseSegment
        );

    bool IsExpired(
        _In_ const COptionalDouble *poptExpirationTime,
        XDOUBLE rParentTime
        ) const;

    void RequestAdditionalFrame();

public:
#if defined(__XAML_UNITTESTS__)
    // !!! FOR UNIT TESTING ONLY !!!

    void SetDuration(CDuration* duration)
    {
        m_duration = duration->ValueWrapper();
    }

    void SetRepeatBehavior(CRepeatBehavior* repeatBehavior)
    {
        m_repeatBehavior = repeatBehavior->ValueWrapper();
    }

#endif

// With DComp animations, we make 3 passes through the active timelines in the time manager. The first creates
// DComp animation objects, and is guided by dirty flags. The second creates DComp animation instances from the
// DComp animations, and the third attaches the DComp animation instances to the DComp objects in the tree. The
// second and third walks aren't guided by dirty flags - they'll walk all timelines, but they will no-op if the
// corresponding dirty flag isn't set.

// Precalculated current global time
public:    double                                             m_rCurrentTime;

    // If we can't make convert this to a Composition animation, then we'll tick it dependently on the UI thread.
    CompositionAnimationConversionResult m_conversionResult { CompositionAnimationConversionResult::Success };

// Used in cases where the storyboard pauses or seeks immediately after beginning, so there is no DComp animation
// instance created yet. We'll apply the pause or the seek when creating the animation instance.
protected: double                                             m_pendingDCompSeekTime;

protected: EventRegistrationToken                             m_wucAnimationCompletedToken;
public:    CXcpList<REQUEST>*                                 m_pEventList;
public:    xref_ptr<RepeatBehaviorVO::Wrapper>                m_repeatBehavior;
public:    CTimeSpan*                                         m_pBeginTime;
public:    xref_ptr<DurationVO::Wrapper>                      m_duration;
public:    xstring_ptr                                        m_strTargetName;
public:    xstring_ptr                                        m_strTargetProperty;
protected: xref::weakref_ptr<CTimeline>                       m_timingParent;
protected: xref::weakref_ptr<CDependencyObject>               m_manualTargetObjectWeakRef;
protected: xref::weakref_ptr<CDependencyObject>               m_targetObjectWeakRef;
protected: const CDependencyProperty*                         m_pManualTargetDependencyProperty;
protected: const CDependencyProperty*                         m_pTargetDependencyProperty;

// Used for animation completed events
protected: wrl::ComPtr<WUComp::ICompositionScopedBatch>       m_wucScopedBatch;

// Used for pause/seek/resume
protected: wrl::ComPtr<WUComp::ICompositionAnimatorPartner>   m_wucAnimator;

// DynamicTimelineParent is the parent that generated this timeline. This is needed to resolve names correctly, especially if the
// timeline has been removed from its parent (the dynamic timline) and been put somewhere else (for instance a storyboard
// that generated transitions in VSM).
private:   CTimeline*                                         m_pDynamicTimelineParent;

// TemplatedParent as a handle -- m_hTemplatedParent is stored as an XHANDLE to prevent anyone from
// trying to party on the pointer directly.  It should only be used for lookup of named targed in the '
// template namescope associated with m_hTemplatedParent.  CCoreServices::GetNamedObject() will not attempt to dereference
// the passed NamescopeOwner if the bTemplateNamescope parameter is TRUE
private:   xref::weakref_ptr<CDependencyObject>               m_templatedParent;

public:    DirectUI::FillBehavior                             m_fillBehavior;

// Precalculated current clock state
public:    DirectUI::ClockState                               m_clockState;

public:    float                                              m_rSpeedRatio;

// Computed properties for our timeline
// Precalculated current progress value
protected: float                                              m_rCurrentProgress;

// Precalculated counter of times the animation looped
protected: XUINT32                                            m_nIteration                                       : 29;

// Flag to see if we ever started the timeline
protected: XUINT32                                            m_fInitialized                                     : 1;

// Whether this Timeline is in Active state (timing state)
protected: XUINT32                                            m_fIsInTimeManager                                 : 1;

// See if we fired the completed event
protected: XUINT32                                            m_IsCompletedEventFired                            : 1;

// The number of Completed handlers that were registered. The event manager will add this many
// references on this CTimeline when this CTimeline is in the time manager. If this CTimeline
// is not in the time manager, then the registered handlers do not cause the event manager
// to AddRef this object.
//
// If this CTimeline runs forever, these extra references should be ignored when checking for
// external references to this CTimeline. Given that the check is only performed when this
// CTimeline is in the time manager, there will be this many references from the event manager
// that can be safely ignored.
private:   XUINT32                                            m_completedHandlerRegisteredCount                  : 28;

// Tick was requested to process Animation's new theme during Filling state
private:   XUINT32                                            m_hasPendingThemeChange                            : 1;

// True if this there is an active independent animation in the timing tree
private:   XUINT32                                            m_hasIndependentAnimation                          : 1;

private:   XUINT32                                            m_fReleaseManagedPeer                              : 1;
public:    bool                                               m_fAutoReverse;

// True iff this timeline's DComp animation no longer matches its Xaml animation, and needs to be updated. This is
// the dirty flag set on any property change.
protected: bool                                               m_isDCompAnimationDirty                            : 1;

// True iff we need to walk this timeline when updating DComp animations.
protected: bool                                               m_isDCompAnimationDirtyInSubtree                   : 1;

// Some timeline properties can force the DComp animations of an entire timing subtree to be re-created, even if the
// child timelines aren't dirty. This can happen when a storyboard's speed ratio is changed, for example.
//
// Note that a child being dirtied can also means the storyboard itself is marked dirty. This catches cases like a
// child extending its duration from 5 seconds to 5 minutes - if the storyboard has an automatic duration, its own
// duration will be affected as well, which can affect the reverse and repeat behaviors of its other children. Also,
// no timing object can be modified (and dirtied) unless the timing object is stopped, so regenerating all children
// of the dirty storyboard will not interrupt any animations in progress.
protected: bool                                               m_forceDCompAnimationDirtyInSubtree                : 1;

// Xaml animations don't hold on to DComp animations, but rather DComp animation instances. This flag is set to true
// when a timeline updates its DComp animations, so that we can return and make animation instances.
protected: bool                                               m_isDCompAnimationInstanceDirty                    : 1;

// There are situations where an animation holds on to a DComp animation instance that's not attached to the tree, such
// as if the animation is expired (and holding the final value) or if the storyboard is paused. In these cases, we write
// the static value in the DComp tree instead, which allows the app to overwrite the animated value with a static one.
// The animation will take over the target property once it is restarted or seeked back to an active time (in the case
// of an expired animation), or resumed (in the case of pause). In the meantime, the DComp animations associated with the
// animation is still valid and up-to-date, so we don't want to release it. We just need a way to reattach it once the
// timeline is active again, which is the purpose of this flag.
protected: bool                                               m_shouldAttachDCompAnimationInstance               : 1;

// Whether we're waiting on an animation completed notification from DComp. Xaml animations will not be considered complete
// until they have received the completed notification from the corresponding DComp animations. They will hold their final
// values and remain in the Active state until then.
//
// A Storyboard with multiple animations inside will mark this flag in two cases:
//
//   1. With WUC animations, a Storyboard will mark this flag if it contains any timeline that has this flag marked. That
//      is, if a Storyboard contains an animation that is waiting on a DComp completion notification, then that Storyboard
//      is also waiting on a DComp completion notification. Whenever any animation receives a DComp completed notification,
//      we walk up the timing tree and update the state of all parent Storyboards.
//
//   2. With legacy DComp animations, a Storyboard will create a dummy DComp animation and add a time event to subscribe
//      to its completion notification. In this case, a Storyboard won't care about its children when setting this flag.
//      This flag depends entirely on whether the dummy DComp animation has been created and whether it has fired its
//      completion notification.
protected: bool                                               m_isWaitingForDCompAnimationCompleted              : 1;

// Whether we've expired (ran past the finish time) in Xaml while still waiting for the completed notification from DComp.
// In this case the animation should hold its final value while waiting for the notification. It should also stop requesting
// more ticks. When the completed notification comes in, we'll request a tick to end the animation.
protected: bool                                               m_isExpiredWhileWaitingForDCompAnimationCompleted  : 1;

// Used in cases where the storyboard pauses or seeks immediately after beginning, so there is no DComp animation
// instance created yet. We'll apply the pause or the seek when creating the animation instance.
// TODO: DCompAnim: When adding animation readback, all pause/seek/resumes will need to be queued until right before we commit.
// This is because we don't want these actions to affect property readback in the same frame (for WinBlue compat), but
// the DComp readback API will take them into account as soon as they're called. Therefore we can't call them until the
// app is done with readback, which means we can't push them through when ticking the time manager before calling out to
// app code. SeekATLT should still push the seek immediately.
protected: bool                                               m_hasPendingPauseForDComp                          : 1;

protected: bool                                               m_hasPendingResumeForDComp                         : 1;
protected: bool                                               m_hasPendingSeekForDComp                           : 1;

// When the app is suspended (or the window loses visibility), we pause DComp animations so that they don't keep ticking
// in the DWM and needlessy use power. However, the Xaml clock is still considered to be ticking. When we resume, we should
// update all DComp animations with the Xaml time to keep them in sync. This flag should only be set between the resume and
// the next frame.
protected: bool                                               m_shouldSynchronizeDCompAnimationAfterResume       : 1;

public:
    static bool s_allowDependentAnimations;
    static XFLOAT s_timeTolerance;  // The amount of tolerance (in seconds) allowed for floating point error when comparing times

protected:
    // Any animations that have a begin delay longer than this are not interesting for animation tracking.
    // This value allows us to short-circuit out of the long tail of staggered animations for
    // theme transitions too.
    static const XUINT32 c_AnimationTrackingMaxBeginTimeInMs = 100;
};
