// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PivotCommon.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

// These callbacks represent the entire set of 'actions'
// the Pivot control can be take. They are called exclusively from
// PivotStateMachine in reaction to events fired from the Pivot
// control.
struct IPivotStateMachineCallbacks
{
    virtual bool IsHeaderItemsCarouselEnabled() const = 0;
    virtual unsigned GetPivotPanelMultiplier() const = 0;
    _Check_return_ virtual HRESULT SetPivotSectionOffset(_In_ DOUBLE offset) = 0;
    _Check_return_ virtual HRESULT SetViewportOffset(_In_ DOUBLE offset, _In_ BOOLEAN animate, _Out_ bool *success) = 0;
    _Check_return_ virtual HRESULT SetPivotSectionWidth(_In_ FLOAT width) = 0;
    _Check_return_ virtual HRESULT SetSelectedIndex(_In_ INT32 idx, _In_ BOOLEAN updateVisual, _In_ BOOLEAN updateIndex, _In_ BOOLEAN updateItem, _In_ PivotAnimationDirection animationHint) = 0;
    _Check_return_ virtual HRESULT SetParallaxRelationship(double sectionOffset, double sectionWidth, float viewportSize) = 0;
    _Check_return_ virtual HRESULT SetSnappingBehavior(_In_ BOOLEAN mandatory) = 0;
    _Check_return_ virtual HRESULT SetViewportEnabled(_In_ BOOLEAN enabled) = 0;
    _Check_return_ virtual HRESULT StartFlyOutAnimation(_In_ DOUBLE from, _In_ DOUBLE headerOffset, _In_ bool toLeft) = 0;
    _Check_return_ virtual HRESULT StartFlyInAnimation(_In_ DOUBLE to, _In_ bool fromLeft) = 0;
    _Check_return_ virtual HRESULT GetIsInDManipAnimation(_Out_ bool *isInDManipAnimation) = 0;
    _Check_return_ virtual HRESULT CancelDManipAnimations() = 0;
    _Check_return_ virtual HRESULT UpdateScrollViewerDragDirection(_In_ PivotAnimationDirection direction) = 0;
    _Check_return_ virtual HRESULT UpdateFocusFollower() = 0;
};

class PivotStateMachine
{
public:
    PivotStateMachine(_In_ IPivotStateMachineCallbacks* pCallbacks);
    _Check_return_ HRESULT Initialize(_In_ INT32 startingIndex, _In_ INT32 itemCount, _In_ BOOLEAN isLocked);

    // These events are called from Pivot to cause changes in the state
    // of the state machine and to cause the correct actions to be performed.
    _Check_return_ HRESULT ApplyTemplateEvent(_In_ BOOLEAN isCorrect);
    _Check_return_ HRESULT MeasureEvent(_In_ wf::Size availableSize);
    _Check_return_ HRESULT ArrangeEvent(_In_ wf::Size finalSize);
    _Check_return_ HRESULT ViewChangedEvent(_In_ BOOLEAN isInertial, _In_ DOUBLE nextOffset, _In_ DOUBLE finalOffset, _In_ DOUBLE sectionOffset);
    _Check_return_ HRESULT FinalViewEvent();
    _Check_return_ HRESULT SelectedIndexChangedEvent(_In_ INT32 newIndex, _In_ BOOLEAN isFromHeaderTap = FALSE, _In_ BOOLEAN skipAnimation = FALSE);
    _Check_return_ HRESULT ItemsCollectionChangedEvent(_In_ INT32 newItemCount, _In_ INT32 changeIdx, _In_ wfc::CollectionChange changeType);
    _Check_return_ HRESULT IsLockedChangedEvent(_In_ BOOLEAN isLocked);
    _Check_return_ HRESULT AnimationCompleteEvent();
    _Check_return_ HRESULT HeaderMeasureEvent(float viewportSize);
    _Check_return_ HRESULT IsHeaderItemsCarouselEnabledChangedEvent();
    _Check_return_ HRESULT HeaderStateChangedEvent(_In_ bool usingStaticHeaders);
    void HeaderWidthsChangedEvent(_In_ std::vector<DOUBLE> &newHeaderWidths);

#if defined(DBG)
    __inline BOOLEAN IsPendingItemsHostValidation() const { return m_fPendingItemHostValidation; }
#endif

    enum PivotState
    {
        PivotState_Uninitialized = 0,
        PivotState_MeasurePending,
        PivotState_ArrangePending,
        // NOTE: Ensure all 'initialized' states comes
        // after Idle state. Lol.
        PivotState_Idle,
        PivotState_InNonInertial,
        PivotState_InFinishTransition,
        PivotState_InFlyOutProgrammaticInertial,
        PivotState_InFlyInProgrammaticInertial,
        PivotState_InSnapBackInertial,
        PivotState_ProgrammaticInertial,
        // LockedPending is a state we shift to when we needed
        // to cancel pending manipulations and recenter the viewport.
        PivotState_LockedPending,
        PivotState_Locked
    };

    PivotState GetPivotState()
    {
        return m_state;
    }

private:
#if defined(DBG)
    static const WCHAR* c_pivotStateStrings[];
#endif

    _Check_return_ HRESULT Transition(_In_ PivotState newState);
    void BeginTransition()
    {
        // http://osgvsowi/19632331 -- We've hit some hard-to-repro re-entrancy failures here, and we've added a
        // fail-fast in all builds to help track it down.  The general order of events is this:
        //  1. In response to an AccessKey press, PivotItem::OnProgrammaticHeaderItemTapped is called
        //  2. This triggers a BeginTransition to the PivotState_ProgrammaticInertial state
        //  3. We then call SetViewportEnabled(FALSE) to tell the ScrollViewer to not allow the user to scroll
        //      because a programatic animation is in progress
        //  4. This triggers a call to UpdateLayout (in ScrollViewer::GetManipulationPrimaryContent)
        //  5. If the Pivot's layout happens to be dirty, in PivotStateMachine::MeasureEvent we transition to
        //      the PivotState_ArrangePending state
        //  6. Since we are already in the middle of the transition we began in (2), the transition in (5) is
        //      invalid.
        FAIL_FAST_ASSERT(!m_fWithinTransition);
        m_fWithinTransition = TRUE;
    }
    void EndTransition() { ASSERT(m_fWithinTransition); m_fWithinTransition = FALSE; }
    BOOLEAN IsInitialized() const { return m_state >= PivotState_Idle; }
    bool InScrollingStaticHeadersMode() const;
    bool IsHeaderItemsCarouselEnabled() const;

    IPivotStateMachineCallbacks* m_callbackPtr;
    PivotState m_state;

    DOUBLE m_pivotSectionWidth;
    DOUBLE m_staticViewportOffset;
    DOUBLE m_predictedViewportOffset;

    INT32 m_currentIndex;
    INT32 m_nextIndex;
    INT32 m_totalItems;
    std::vector<DOUBLE> m_headerWidths;

    wf::Size m_lastArrangeSize;
    PivotAnimationDirection m_direction;
    BOOLEAN m_fIsLocked;
    BOOLEAN m_fPendingInitialization;
    BOOLEAN m_fPendingArrange;
    BOOLEAN m_fPendingOutOfSequenceMove;
    BOOLEAN m_fPendingViewportEnabledUpdate;
    BOOLEAN m_fPendingFirstItemLayout;
    BOOLEAN m_fPendingItemHostValidation;
    BOOLEAN m_fSnapPointBehaviorRelaxed;
    BOOLEAN m_fProgrammaticViewChangeInProgress;
    BOOLEAN m_fWithinTransition;

    bool m_usingStaticHeaders;
    float m_viewportSize;

    bool m_pendingZoomToRect;
    DOUBLE m_pendingZoomToRectOffset;

    DOUBLE m_pendingHeaderShiftOffset;

    bool m_manualAnimationInProgress;

    struct
    {
        bool isPending;
        double offset;
        bool shouldAnimate;
    } m_deferredSetViewportOffsetData;

    void SlideIndex(_Inout_ INT32* idx, _In_ PivotAnimationDirection dir) const;
    PivotAnimationDirection DetermineDirection(_In_ INT32 idx, _In_ BOOLEAN isFromHeaderTap) const;
    double GetMidpointViewportOffset() const;
    _Check_return_ HRESULT SetViewportOffset(DOUBLE offset, BOOLEAN animate);
    _Check_return_ HRESULT RevertToIdle();

    static const DOUBLE c_pivotFlickAnimationDuration;
    static const DOUBLE c_pivotAnimationRatio;

#if defined(DBG)
    // Keep this number unneccesarily low for debug builds to spot glitches.
    // In production it can be very large because the viewport is very large.
    static const UINT c_sectionNormalizationThreshold = 5;
#else
    static const UINT c_sectionNormalizationThreshold = 300;
#endif
    static const UINT c_sectionWidthDividerForOffsetComparisons = 4;
};

} } } } XAML_ABI_NAMESPACE_END
