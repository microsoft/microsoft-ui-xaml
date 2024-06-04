// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"
#include "ParallelTimeline.h"
#include <VisualTransitionCompletedData.h>
#include "wilhelper.h"

class CTimeline;
class CTransition;
class VisualTransitionCompletedData;
struct LayoutTransitionCompletedData;

// Object created for <Storyboard> tag
class CStoryboard final
    : public CParallelTimeline
{
private:
    CStoryboard(_In_ CCoreServices *pCore)
        : CParallelTimeline(pCore)
        , m_fIsStopped(TRUE)
        , m_fIsPaused(FALSE)
        , m_fIsResuming(FALSE)
        , m_fIsBeginning(FALSE)
        , m_fIsSeeking(FALSE)
        , m_fInheritanceContextWalkForVsmProcessed(FALSE)
        , m_fAutoComplete(FALSE)
        , m_fIsTrackedForAnimationTracking(FALSE)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CStoryboard()  // !!! FOR UNIT TESTING ONLY !!!
        : CStoryboard(nullptr)
    {}
#endif

    ~CStoryboard() override;

    DECLARE_CREATE(CStoryboard);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CStoryboard>::Index;
    }

    CDependencyObject* GetStandardNameScopeParent() override;

public:
    _Check_return_ HRESULT UpdateAnimation(
        _In_ const ComputeStateParams &parentParams,
        _Inout_ ComputeStateParams &myParams,
        XDOUBLE beginTime,
        DirectUI::DurationType durationType,
        XFLOAT durationValue,
        _In_ const COptionalDouble &expirationTime,
        _Out_ bool *pIsIndependentAnimation
        ) override;

    _Check_return_ HRESULT ComputeStateImpl(
        _In_ const ComputeStateParams &parentParams,
        _Inout_ ComputeStateParams &myParams,
        _Inout_opt_ bool *pHasNoExternalReferences,
        bool hadIndependentAnimationLastTick,
        _Out_ bool *pHasIndependentAnimation
        ) override;

    _Check_return_ HRESULT OnBegin() override
    {
        // Forward to our local begin call - this is never called on
        // top-level storyboards
        RRETURN(BeginPrivate(/* fIsTopLevel */ FALSE));
    }

    bool IsInActiveState() const override { return !m_fIsPaused && CTimeline::IsInActiveState(); }
    bool IsInStoppedState() override { return m_fIsStopped && CTimeline::IsInStoppedState(); }

    void RequestAutoComplete(bool fValue) { m_fAutoComplete = (fValue && !m_fIsEssential); }

    _Check_return_ HRESULT PausePrivate();
    _Check_return_ HRESULT Pause();
    _Check_return_ HRESULT ResumePrivate();
    _Check_return_ HRESULT Resume();
    _Check_return_ HRESULT BeginPrivate(bool fIsTopLevel);
    _Check_return_ HRESULT Begin();
    _Check_return_ HRESULT StopPrivate();
    _Check_return_ HRESULT Stop();

    _Check_return_ HRESULT SeekPrivate(
        _In_ CTimeSpan *pSeekTime
        );

    _Check_return_ HRESULT Seek(
        _In_ const CValue& seekTime
        );

    _Check_return_ HRESULT SeekAlignedToLastTickPublic(
        _In_ const CValue& seekTime
        );

    _Check_return_ HRESULT SeekAlignedToLastTick(_In_ CTimeSpan *pSeekTime);

    _Check_return_ HRESULT SkipToFillPublic();
    _Check_return_ HRESULT SkipToFill();

    _Check_return_ HRESULT Complete();

    _Check_return_ HRESULT GetDuration(_Out_ DirectUI::DurationType* pDurationType, _Out_ XFLOAT* prDuration);

    void SetInheritanceContextWalkForVsmProcessed(bool fValue)
    {
        m_fInheritanceContextWalkForVsmProcessed = fValue;
    }

    bool InheritanceContextWalkForVsmProcessed() const
    {
        return m_fInheritanceContextWalkForVsmProcessed;
    }

    void AnimationTrackingBeginScenario(
        _In_opt_ CTransition* pTransition,
        _In_opt_ CDependencyObject* pTargetObject
        );

    static void SetStoryboardStartedCallback(
        _In_ std::function<HRESULT(CDependencyObject* /* storyboard */, CDependencyObject* /* target */)> callback)
    { s_storyboardStartedCallback = callback; }

    void SynchronizeDCompAnimationAfterResume(double timeManagerTime) override;

    // Test hook to manually fire the "DComp animation completed" event for this entire storyboard
    void FireDCompAnimationCompleted();

    bool IsPaused() const;

protected:
    CompositionAnimationConversionResult MakeCompositionAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext) override;

private:
    _Check_return_ HRESULT CompleteInternal(
        bool handleInfiniteTimelines,
        bool isSynchronous
        );

    _Check_return_ HRESULT SeekInternal(_In_ CTimeSpan *pSeekTime);

    bool IsTopLevelStoryboard();

    bool IsAnimationTrackingAllowed();
    void AnimationStartTracking();
    void AnimationStopTracking();

    xstring_ptr GetNameForTracking(
        _In_opt_ CTransition* transition,
        _In_opt_ CUIElement* target,
        _In_opt_ CDependencyObject* dynamicTimeline,
        _Out_opt_ XUINT16* scenarioPriority);
    xstring_ptr GetDetailsForTracking(_In_ CUIElement* target);

    void EnsureTelemetryName();

public:
    std::unique_ptr<VisualTransitionCompletedData>  m_pVisualTransitionCompletedData;
    LayoutTransitionCompletedData*                  m_pLayoutTransitionCompletedData = nullptr;

    bool m_fIsEssential = false;

private:
    // True when Stop has been called without a subsequent Begin. Mutually exclusive with m_fIsBeginning.
    bool m_fIsStopped : 1;

    // True when Pause has been called without a subsequent Resume. Mutually exclusive with m_fIsResuming.
    bool m_fIsPaused : 1;

    // True from when Resume is called until the next Tick, Pause, or Stop. Mutually exclusive with m_fIsPaused.
    bool m_fIsResuming : 1;

    // True from when Begin is called until the next tick or the next Stop. Mutually exclusive with m_fIsStopped.
    bool m_fIsBeginning : 1;

    // True from when Seek is called until the next tick.  m_rPendingSeekTime is valid only when m_fIsSeeking.
    bool m_fIsSeeking : 1;

    bool  m_fInheritanceContextWalkForVsmProcessed: 1;

    // Flag which define the storyboard marked for autocompletion
    bool m_fAutoComplete: 1;

    // Are we currently being tracked for animation tracking?
    bool m_fIsTrackedForAnimationTracking: 1;

    // The parent timeline's time from the last tick while unpaused.
    XDOUBLE m_lastParentTime        = XDOUBLE_MAX;  // unset sentinel value

    // The pending seek time from the last Seek call.
    XDOUBLE m_rPendingSeekTime      = XDOUBLE_MAX;

    // The delta between the parent's time and the storyboard's local time.
    // This delta is negative, except in rare cases like seeking ahead of the time manager's clock.
    XDOUBLE m_rTimeDelta            = XDOUBLE_MAX;

    // Used to tag the WUC animations created by this storyboard. WUC then uses this telemetry to look at power usage.
    xstring_ptr m_storyboardTelemetryName = xstring_ptr::NullString();

    // We are not interested in animations whose durations are longer than this for animation tracking.
    static const XUINT32 c_AnimationTrackingMaxDurationInS = 5;
    static const XUINT16 c_AnimationTrackingDefaultPriority = 100;
    static const XUINT16 c_AnimationTrackingTransitionPriority = 200;
    static const XUINT16 c_AnimationTrackingTransitionEntrancePriority = 250;

    // Callback we invoke to notify the test framework that we started a storyboard.
    static std::function<HRESULT(CDependencyObject* /* storyboard */, CDependencyObject* /* target */)> s_storyboardStartedCallback;
};

namespace StoryboardHelpers
{
    // These methods will seek the storyboard to the given poistion and
    // synchronously apply all property values.
    HRESULT ZeroSeekAlignedToLastTick(_In_ CStoryboard*);
    HRESULT EndSeekAlignedToLastTick(_In_ CStoryboard*);

    HRESULT ModifyStoryboard(_In_opt_ CStoryboard*, bool forceStart, _In_ const std::function<HRESULT(void)>& modifyFunc);
}
