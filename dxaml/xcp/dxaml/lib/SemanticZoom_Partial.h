// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a scrollable area that can contain either a ZoomedInView of
//      content or a ZoomedOutView used to navigate around the content via zoom
//      gestures.

#pragma once

#include "SemanticZoom.g.h"
#include "DirectManipulationStateChangeHandler.h"

namespace DirectUI
{
    // Represents a scrollable area that can contain either a normal view of
    // content or a SemanticZoom used to navigate around the content via zoom
    // gestures.
    PARTIAL_CLASS(SemanticZoom)
        , private DirectManipulationStateChangeHandler
    {
        private:
            enum SemanticZoomPhase
            {
                SemanticZoomPhase_Idle,
                // we are switching because of an API call (mousewheel, api or a click).
                // we will stay in this phase until we have completed the animation
                SemanticZoomPhase_API_SwitchingViews,
                // we are switching because of DM: we have been notified of a zoomchange
                // and we have passed the threshold.
                SemanticZoomPhase_DM_SwitchingViews,
                // we have let go of the fingers and currently we are animating the
                // viewport from that location/zf to the regular zf
                SemanticZoomPhase_DM_CompletingViews,
            };

            // Reference to the template part hosting the ZoomedInView.
            TrackerPtr<xaml::IFrameworkElement> m_tpZoomedInPresenterPart;

            // Reference to the template part hosting the ZoomedOutView.
            TrackerPtr<xaml::IFrameworkElement> m_tpZoomedOutPresenterPart;

            // Reference to the template parts used to offset the views and apply scales
            TrackerPtr<xaml_media::ICompositeTransform> m_tpZoomedOutTransform;
            TrackerPtr<xaml_media::ICompositeTransform> m_tpZoomedInTransform;
            TrackerPtr<xaml_media::ICompositeTransform> m_tpManipulatedElementTransform;

            // Reference to the scrollViewer
            TrackerPtr<xaml_controls::IScrollViewer> m_tpScrollViewer;

            // Reference to the ZoomOutButton template part
            TrackerPtr<xaml_controls::IButton> m_tpZoomOutButton;

            // Reference to a throw-away timer that will trigger visibility on the alternate view (perf)
            TrackerPtr<xaml::IDispatcherTimer> m_tpAlternateViewTimer;

            // This field is set to TRUE while this SemanticZoom is being initialized (according to the ISupportInitialize
            // interface). It is FALSE during all other times.
            BOOLEAN m_isInitializing;

            // If the IsZoomedInViewActive property changes while SemanticZoom is still being initialized, this flag will
            // be set and the view change postponed. When initialization completes (EndInit), the view is changed to the
            // correct one. This avoids the problems incurred when the view is changed when one or both views are not
            // initialized yet.
            BOOLEAN m_isPendingViewChange;

            // Whether we are currently processing a keyboard input event.
            BOOLEAN m_isProcessingKeyboardInput;

            // Whether we are currently processing a pointer input event.
            BOOLEAN m_isProcessingPointerInput;

            // Whether we are currently cancelling a JumpList (ex: processing back button)
            BOOLEAN m_isCancellingJumpList;

            // args to use when the view change is complete. Created during ViewChange.
            TrackerPtr<xaml_controls::ISemanticZoomViewChangedEventArgs> m_tpCompletedArgs;

            // pre-calculated caches for the zoompoints used for the different views
            wf::Point m_zoomPoint;                 // actual centerpoint as registered
            wf::Point m_zoomPointForZoomedInView;  // point as used foro the zoomedinview
            wf::Point m_zoomPointForZoomedOutView; // point as used foro the zoomedoutview

            // indicates which view was active when we started zooming
            BOOLEAN m_zoomOriginatesFromZoomedInView;
            // naming is identical to DUI implementation, please keep as-is
            FLOAT _upperThresholdLow;
            FLOAT _upperThresholdHigh;
            FLOAT _lowerThresholdLow;
            FLOAT _lowerThresholdHigh;

            // These fields indicate whether we are able to animate to a particular state
            BOOLEAN m_isZoomedInViewAnimationHooked;
            BOOLEAN m_isZoomedOutViewAnimationHooked;

            // indicates whether we are imitating gestures (ctrl-mousewheel)
            BOOLEAN m_emulatingGesture;

            EventRegistrationToken m_sizeChangedToken;
            EventRegistrationToken m_zoomedInViewSizeChangedToken;
            EventRegistrationToken m_zoomedOutViewSizeChangedToken;
            EventRegistrationToken m_elementZoomOutButtonClickToken;


            // the phase we are in, this can be something like idle, 'changing because of API calls'
            SemanticZoomPhase m_changePhase;
            // hack to allow us to temporarily lock the phase so that the property change method
            // will not force it to API. Used when calling the property change because of DM input (fingers).
            BOOLEAN m_phaseChangeLockDuringViewSwitch;

            wf::Point m_manipulatedElementOffset;

            // indicates that we have performed the Initialize calls, which means we will have to close with Complete calls
            BOOLEAN m_calledInitializeViewChangeSinceManipulationStart;

            // the value that we compare against to determine if zoom is truly occurring.
            // this is important in the 'interrupted' scenarios where we were in a zoom animation and the
            // user has put his finger down again to for instance start dragging. The cumulative factor now
            // is just what the zoom happened to be at the moment of interruption, because that value is
            // only reset at the start of viewport creation.
            FLOAT m_cumulativeZoomFactorAtStartOfManipulation;

            // Set when we call ChangeViews(), and cleared in OnViewChangeCompleted().
            BOOLEAN m_isProcessingViewChange;

            // Cached value of IsZoomOutButtonEnabled property, to avoid access penalty when proccessing OnPointerMoved().
            BOOLEAN m_isZoomOutButtonEnabled;

            // Prevents creation of an automation peer until
            // requested by automation as an optimization.
            BOOLEAN m_hasAutomationPeer;

        public:
            // Initializes a new instance of the SemanticZoom class.
            SemanticZoom();

            // Destroys an instance of the SemanticZoom class.
            ~SemanticZoom() override;

            // Handles custom property changed events and calls their
            // OnPropertyChanged methods.
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            // Get the view container template parts.
            IFACEMETHOD(OnApplyTemplate)()
                override;

            // Change to the correct visual state for the SemanticZoom.
            _Check_return_ HRESULT ChangeVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions)
                override;

            // Toggle the active view.
            _Check_return_ HRESULT ToggleActiveViewImpl();

            // Toggle the active view as a result of a HeaderItem tap
            _Check_return_ HRESULT ToggleActiveViewFromHeaderItem(_Out_ BOOLEAN* bToggled);

            // Sets internal flags and then calls ToggleActiveView(), so that when
            // focus changes as a result of ToggleActiveView(), the new item is focused
            // using the specified FocusState
            _Check_return_ HRESULT ToggleActiveViewWithFocusState(
                _In_ xaml::FocusState focusState);

            // Handles when a key is pressed down on the SemanticZoom.
            IFACEMETHOD(OnKeyDown)(
                _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
                override;

            // Handles when the mouse wheel spins to change active views.
            IFACEMETHOD(OnPointerWheelChanged)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
                override;

            // Called by SemanticZoomAutomationPeer to Toggle
            _Check_return_ HRESULT AutomationSemanticZoomOnToggle();

            // Returns the currently displayed presenter for automation.
            _Check_return_ HRESULT AutomationGetActivePresenter(_Outptr_opt_ xaml::IFrameworkElement** ppActivePresenter);

            // DirectManipulationStateChangeHandler implementation
            _Check_return_ HRESULT NotifyStateChange(
                _In_ DMManipulationState state,
                _In_ FLOAT xCumulativeTranslation,
                _In_ FLOAT yCumulativeTranslation,
                _In_ FLOAT zCumulativeFactor,
                _In_ FLOAT xCenter,
                _In_ FLOAT yCenter,
                _In_ BOOLEAN isInertial,
                _In_ BOOLEAN isTouchConfigurationActivated,
                _In_ BOOLEAN isBringIntoViewportConfigurationActivated) noexcept override;

            // Keyboard and Pointer input can cause a change in focus inside SemanticZoom.
            // While inside of a method handling input processing, we set a flag indicating whether
            // that input is from keyboard or a flag indicating whether that input is from pointer.
            // Then, when Focus(FocusState) gets called, we check these flags to determine the
            // correct FocusState to pass in.
            BOOLEAN GetIsProcessingKeyboardInput()
            {
                return m_isProcessingKeyboardInput;
            }

            BOOLEAN GetIsProcessingPointerInput()
            {
                return m_isProcessingPointerInput;
            }

            BOOLEAN GetIsCancellingJumpList() const
            {
                return m_isCancellingJumpList;
            }

            // Used to handle back button presses on phone
            _Check_return_ HRESULT OnBackButtonPressedImpl(_Out_ BOOLEAN* returnValue);

        private:
            // setup timer to trigger creation of alternate view
            _Check_return_ HRESULT SetupAlternateView(_In_opt_ IInspectable* pUnused1, _In_opt_ IInspectable* pUnused2);
            _Check_return_ HRESULT SetAlternateViewTimer();

            // sets the thresholds
            _Check_return_ HRESULT UpdateThresholds();

            // helper function to calculate the bounds we would like to be at
            _Check_return_ HRESULT CalculateBounds(_Out_ XRECTF* pBounds);

            // Raise the ViewChangeStarted event.
            _Check_return_ HRESULT OnViewChangeStarted(
                _In_ xaml_controls::ISemanticZoomViewChangedEventArgs* e);

            // Raise the ViewChangeCompleted event.
            _Check_return_ HRESULT OnViewChangeCompleted(
                _In_ xaml_controls::ISemanticZoomViewChangedEventArgs* e);

            _Check_return_ HRESULT ViewChangeAnimationFinished(_In_opt_ IInspectable* pUnused1, _In_opt_ IInspectable* pUnused2);

            // Associate the SemanticZoom with an ISemanticZoomInformation view.
            _Check_return_ HRESULT InitializeSemanticZoomInformation(
                _In_ IInspectable* pOldValue,
                _In_ IInspectable* pNewValue,
                _In_ BOOLEAN isZoomedInView);

            // Change from one view to another.
            _Check_return_ HRESULT ChangeViews() noexcept;

            // Calculate the position and bring the active view to view without
            // animations by re-evaluating the bounds we should be.
            _Check_return_ HRESULT ResetViewsAndSnapToActiveView();

            // reacting to size changes on itself
            _Check_return_ HRESULT OnSizeChanged(_In_ IInspectable* pSender,_In_ xaml::ISizeChangedEventArgs* pArgs);

            // Called whenever the ZoomOutButton is clicked.
            _Check_return_ HRESULT OnZoomOutButtonClick(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // Helper function to add an EventHandler to the Completed event of the given VisualState.
            _Check_return_ HRESULT AddStoryboardCompletedHandler(
                _In_ xaml::IVisualState* pState,
                _In_ HRESULT (SemanticZoom::*pHandler)(IInspectable*, IInspectable*));

            // When the ZoomOutButtonVisible state's Storyboard completes, kicks off the delay-hide transition.
            _Check_return_ HRESULT OnZoomOutButtonVisibleStoryboardCompleted(
                _In_ IInspectable* pUnused1,
                _In_ IInspectable* pUnused2);

            // Shows the ZoomOutButton.
            _Check_return_ HRESULT ShowZoomOutButton();

            // Hides the ZoomOutButton.
            _Check_return_ HRESULT HideZoomOutButton(
                _In_ bool bUseTransitions);

            // Creates or gets the automation peers for the two presenter's children and
            // reparents them to this control so when automation builds the tree from
            // one of them the tree stays self-consistent.
            _Check_return_ HRESULT AutomationReparentPresenters(
                _In_ xaml_automation_peers::ISemanticZoomAutomationPeer* sezoPeer);

            // Registers or unregisters back key listener based on whether the ZOV is active or not
            _Check_return_ HRESULT ToggleBackKeyListener(
                _In_ BOOLEAN bIsZoomedOutViewActive);

        protected:
            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

            // PointerMoved event handler.
            IFACEMETHOD(OnPointerMoved)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // Called when the element enters the tree.
            _Check_return_ HRESULT EnterImpl(
                _In_ bool isLive,
                _In_ bool skipNameRegistration,
                _In_ bool coercedIsEnabled,
                _In_ bool useLayoutRounding) final;
    };
}
